/*
 * module_resp.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */
#include <ccblkfn.h>
#include <string.h>

#include "module_resp.h"

#define _MODULE_RESP_INFO_

#ifdef _MODULE_RESP_INFO_
#define RESP_DBG_INFO(fmt, arg...) AIO_printf("\r\n[RESP] "fmt, ##arg)
#else
#define RESP_DBG_INFO(fmt, arg...)
#endif

#define RESP_CARRIER_FREQ   62800
#define RESP_SAMPLE_TIME    10     //sample TIME 10ms(100Hz)

#define RESP_VALUE_NORMAL   ((~(u32)(1250 / 2500.0 * 0x7FFFFF)+1)&0x00FFFFFF) //-1.25V
#define RESP_VALUE_OFFSET   ((u32)(100 / 2500.0 * 0x7FFFFF))                   //100mmV对应的ADC值
#define RESP_VALUE_MAX      ((u32)(RESP_VALUE_NORMAL + RESP_VALUE_OFFSET))
#define RESP_VALUE_MIN      ((u32)(RESP_VALUE_NORMAL - RESP_VALUE_OFFSET))

#pragma section("sdram0_bank3")
resp_device g_resp_dev;
#pragma section("sdram0_bank3")
resp_device * p_resp_dev = &g_resp_dev;
bool g_IsRESP_Upload;

//StatusFlag_TypeDef  g_RESP_Status;
//unsigned long g_RESP_Result;
RESP_Channel_TypeDef g_RESP_channel;
//bool g_IsRESP_Upload;
u8Limits_TypeDef g_RR_Limits;

//<!-- RESP algorithm define Begin -->
#define RESP_Sampling_Buffer_Len    (1000 / RESP_SAMPLE_TIME + 1)
#pragma section("sdram0_bank3")
long g_resp_result[RESP_Sampling_Buffer_Len];
#pragma section("sdram0_bank3")
unsigned long g_resp_tick[RESP_Sampling_Buffer_Len];
u16  g_resp_receive_offset,g_resp_algorithm_offset,g_resp_raw_offset;
//<!-- RESP algorithm define End -->

void RESP_Reset(EnableOrDisable status);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void RESP_Init(void);
#pragma section("sdram0_bank3")
void InitTimer0(void);
#pragma section("sdram0_bank3")
void InitTimer0_gpio(void);
#pragma section("sdram0_bank3")
void InitTimer0_config(void);
#pragma section("sdram0_bank3")
void Timer0_setWorkState(const EnableOrDisable state);
#pragma section("sdram0_bank3")
void RESP_SelectIO_Init(void);
#pragma section("sdram0_bank3")
void RESP_ResetIO_Init(void);
#pragma section("sdram0_bank3")
void RESP_AsphyxiaUpload(u8 RR);
#pragma section("sdram0_bank3")
void RESP_AlarmUpload(u8 type);

#pragma section("sdram0_bank3")
void RESP_SetChannel(u8 channel);
#pragma section("sdram0_bank3")
RETURN_TypeDef RESP_setUploadDataType(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
RETURN_TypeDef RESP_setThreshold(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
RETURN_TypeDef RESP_setCarrierWave(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
RETURN_TypeDef RESP_setAsphyxiaTime(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
RETURN_TypeDef RESP_DebugInterface(UartProtocolPacket *pPacket);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

/******************************************************************************/
/******************************** Timer0 Functions Start***********************/
static void InitTimer0_gpio(void)
{
    *pPORTG_FER |= PG6;         //enable PG6 peripheral function.
    *pPORTG_MUX &= ~(3<<6);     //set PG6 as TMR0 interface.mask bit[7:6]
    *pPORTG_MUX |= (2<<6);      //set bit[7:6]=b#10
}

/**
  * @brief  This function initializes the TIM0 for RESP module carrier Freq.
  * @param  void
  * @retval void
  */
static void InitTimer0_config(void)
{
    unsigned long sclk = getSystemCLK();
    unsigned long ratio = 0;
    
/*****************************************************************************
 *
 *   pwm_out mode, interrupt disable, count to end of period
 *   SCLK for counter
 * 
 ****************************************************************************/
    *pTIMER0_CONFIG= 0x0009;

    sclk += (RESP_CARRIER_FREQ /2);     //round up before divide by RESP_CARRIER_FREQ
    ratio = sclk / RESP_CARRIER_FREQ;
    *pTIMER0_PERIOD= ratio;         //timer0 = ratio * (1/132)us
    *pTIMER0_WIDTH= (ratio / 2);    //timer width, 50%
}

static void Timer0_setWorkState(const EnableOrDisable state)
{
    if (ENABLE == state)
    {
        *pTIMER_ENABLE = 0x0001;
    }
    else
    {
        *pTIMER_DISABLE = 0x0001;
    }
}

static void InitTimer0(void)
{
    InitTimer0_gpio();
    InitTimer0_config();
}
/******************************** Timer0 Functions Stop ***********************/
/******************************************************************************/

static void RESP_SelectIO_Init(void)
{
	*pPORTGIO_INEN &= ~PG2;			/* input buffer disable */
	*pPORTGIO_DIR |= PG2;			/* output */
	*pPORTGIO_EDGE &= ~PG2;         // Level Sensitivity
    *pPORTGIO_MASKA_CLEAR = PG2;
	*pPORTGIO_CLEAR = PG2;

    *pPORTFIO_INEN &= ~PF11;         /* input buffer disable */
    *pPORTFIO_DIR |= PF11;           /* output */
    *pPORTFIO_EDGE &= ~PF11;         // Level Sensitivity
    *pPORTFIO_MASKA_CLEAR = PF11;
    *pPORTFIO_CLEAR = PF11;
}

void RESP_ChannelSelect(const RESP_Channel_TypeDef channel)
{
    RESP_Reset(DISABLE);
    if (RESP_RA_LL == channel)
    {
        *pPORTGIO_CLEAR = PG2;
        *pPORTFIO_CLEAR = PF11;
    }
    else if (RESP_RA_LA == channel)
    {
        *pPORTGIO_CLEAR = PG2;
        *pPORTFIO_SET = PF11;
    }
    g_RESP_channel = channel;
}

static void RESP_ResetIO_Init(void) //PG3 output
{
	*pPORTGIO_INEN &= ~PG3;			/* input buffer disable */
	*pPORTGIO_DIR |= PG3;			/* output */

	/* clear interrupt settings */
	*pPORTGIO_EDGE &= ~PG3;         // Level Sensitivity
    *pPORTGIO_MASKA_CLEAR = PG3;
    
	*pPORTGIO_SET = PG3;
}

void RESP_Reset(EnableOrDisable status)
{
    if (ENABLE == status)
    {
        *pPORTGIO_CLEAR = PG3;
    }
    else
    {
        *pPORTGIO_SET = PG3;
    }
}

static void RESP_NormalLevelHandler(void)
{
    //u32 curResp = g_RESP_Result;
    u32 curResp = p_resp_dev->adc_val;
    if ((curResp > RESP_VALUE_MAX) || (curResp < RESP_VALUE_MIN))
    {
        RESP_Reset(ENABLE);
        RESP_DBG_INFO("RESP_Reset -> Enable!");
    }
    else
    {
        RESP_Reset(DISABLE);
    }
}


//<!-- RESP ADC Result API Begin -->
void RESP_adc_store_data(void)
{
    p_resp_dev->tick = SysTick_Get();
    
    g_resp_result[g_resp_receive_offset] = p_resp_dev->adc_val;
    g_resp_tick[g_resp_receive_offset] = p_resp_dev->tick;
    
    g_resp_receive_offset += 1;
    if(g_resp_receive_offset >= RESP_Sampling_Buffer_Len)
    {
        g_resp_receive_offset -= RESP_Sampling_Buffer_Len;
    }
    if(g_resp_algorithm_offset == g_resp_receive_offset)    // overflow
    {
        g_resp_algorithm_offset += 1;
        if(g_resp_algorithm_offset >= RESP_Sampling_Buffer_Len)
        {
            g_resp_algorithm_offset -= RESP_Sampling_Buffer_Len;
        }
    }
    if(g_resp_raw_offset == g_resp_receive_offset)    // overflow
    {
        g_resp_raw_offset += 1;
        if(g_resp_raw_offset >= RESP_Sampling_Buffer_Len)
        {
            g_resp_raw_offset -= RESP_Sampling_Buffer_Len;
        }
    }
}

long RESP_GetResult(void)
{
    s32 s32Index = g_resp_receive_offset - 1;
    if(s32Index < 0)    s32Index += RESP_Sampling_Buffer_Len;

    return g_resp_result[s32Index];
}

static u16 RESP_GetDataLen(void)
{
    s32 s32Len;

    s32Len = g_resp_receive_offset - g_resp_raw_offset;
    if(s32Len < 0)  s32Len += RESP_Sampling_Buffer_Len;

    return (u16)s32Len;
}

static u16 RESP_PopResult(u32* pu32Tick, s32* p_adc)
{
    u16 s32Len = RESP_GetDataLen();

    if(s32Len)
    {
        *pu32Tick = g_resp_tick[g_resp_raw_offset];
        *p_adc = g_resp_result[g_resp_raw_offset];

        g_resp_raw_offset += 1;
        if(g_resp_raw_offset >= RESP_Sampling_Buffer_Len)
        {
            g_resp_raw_offset -= RESP_Sampling_Buffer_Len;
        }
    }
    return s32Len;
}

static u16 RESP_GetAlgoDataLen(void)
{
    s32 s32Len = g_resp_receive_offset - g_resp_algorithm_offset;
    if(s32Len < 0)  s32Len += RESP_Sampling_Buffer_Len;

    return (u16)s32Len;
}

u16 RESP_PopAlgoResult(u32* pu32Tick, s32* pVal)
{
    u16 s32Len = RESP_GetAlgoDataLen();

    if(s32Len)
    {
        *pu32Tick = g_resp_tick[g_resp_algorithm_offset];
        *pVal = s24_to_s32(g_resp_result[g_resp_algorithm_offset]);

        g_resp_algorithm_offset += 1;
        if(g_resp_algorithm_offset >= RESP_Sampling_Buffer_Len)
        {
            g_resp_algorithm_offset -= RESP_Sampling_Buffer_Len;
        }
    }

    return s32Len;
}

void RESP_DataReset(void)
{
    g_resp_algorithm_offset = g_resp_receive_offset;
    g_resp_raw_offset = g_resp_receive_offset;
}
//<!-- RESP ADC Result API End -->

void RESP_Init(void)
{
    /*g_RESP_Status*/p_resp_dev->status = IDEL_FLAG;
    //g_RESP_Result = 0;
    p_resp_dev->adc_val = 0;
    p_resp_dev->upload_type = FMT_PACKET;
    g_IsRESP_Upload = true;
    g_resp_receive_offset = g_resp_algorithm_offset = g_resp_raw_offset = 0;
    memset(g_resp_result, 0, sizeof(g_resp_result));
    memset(g_resp_tick, 0, sizeof(g_resp_tick));
    g_RR_Limits.max = 60;
    g_RR_Limits.min = 10;
    
    InitTimer0();
    Timer0_setWorkState(ENABLE);//enable 62.8KHz clock

    RESP_SelectIO_Init();
    RESP_ResetIO_Init();
    RESP_Reset(DISABLE);
    RESP_ChannelSelect(RESP_RA_LL);
    //RESP_ChannelSelect(RESP_RA_LA);
}

void RESP_StartSample(void)
{
    if (IDEL_FLAG == /*g_RESP_Status*/p_resp_dev->status)
    {
        /*g_RESP_Status*/p_resp_dev->status = WORK_FLAG;
    }
}

void RESP_AsphyxiaUpload(u8 RR)
{
    static unsigned long ulNextSampleTime = 0;
    
    if ((IsOnTime(ulNextSampleTime)) && (true == g_IsRESP_Upload)) // is on time or over time
    {
#ifdef _UPLOAD_1HZ_DATA_
        if(RR < g_RR_Limits.min)
        {
            u8 pValue[1];
            pValue[0] = 0x01;   //0x00 for narmal;0x01 resp asphyxia alarm
            UploadDataByID(AIO_TX_RESP_ASPHYXIA_ID, TRUE, (char *)pValue, sizeof(pValue));
        }
#endif
        ulNextSampleTime += getTickNumberByMS(1000);
    }
}

void RESP_AlarmUpload(u8 type)
{
    static unsigned long ulNextSampleTime = 0;
    
    if ((IsOnTime(ulNextSampleTime)) && (true == g_IsRESP_Upload)) // is on time or over time
    {
#ifdef _UPLOAD_1HZ_DATA_
        if(Suffocation == type)
        {
            u8 pValue[1];
            pValue[0] = 0x01;   //0x00 for narmal;0x01 resp asphyxia alarm
            UploadDataByID(AIO_TX_RESP_ASPHYXIA_ID, TRUE, (char *)pValue, sizeof(pValue));
        }
#endif
        ulNextSampleTime += getTickNumberByMS(1000);
    }
}

void RESP_ReslultUpload(void)
{

    char pValue[8];
    u32 tick = 0;
    s32 resp = 0;
    u16 nLen = 0;
    if ((FMT_UPLOAD_OFF == p_resp_dev->upload_type) \
        || (false == g_IsRESP_Upload))//判断是否需要上传数据
    {
        return;
    }
    else if (FMT_PACKET == p_resp_dev->upload_type)
    {
        //目前buffer是否有数据更新
        nLen = RESP_PopResult(&tick, &resp);
        if (!nLen) return;//无数据更新，退出
    }
    else if (FMT_FILTER_DATA == p_resp_dev->upload_type)
    {
        resp_algorithm_PopFilterResult(&tick, &resp);
    }

#ifdef _UPLOAD_RESP_ADC_
    pValue[0] = (char)g_RESP_channel;
    
    pValue[1] = (char)(resp >> 16) & 0xFF;      //RESP Vaule MSB
    pValue[2] = (char)(resp >> 8) & 0xFF;
    pValue[3] = (char)(resp >> 0) & 0xFF;       //RESP Vaule LSB
    
    pValue[4] = (char)(tick >> 24) & 0xFF;    //System Tick MSB
    pValue[5] = (char)(tick >> 16) & 0xFF;
    pValue[6] = (char)(tick >> 8) & 0xFF;
    pValue[7] = (char)(tick >> 0) & 0xFF;     //System Tick LSB

    UploadDataByID(AIO_TX_RESP_REALTIME_ID, TRUE, pValue, sizeof(pValue));
#endif

#ifdef _PRINT_RESP_SAMPLE_
    //AIO_printf("\r\n[RESP] 0x%08X",resp);
    //return;
    if (resp & (1<<23))
    {
        resp = ((~resp)+1);
        AIO_printf("\r\n[RESP] -%d\t", (resp & 0x7FFFFF));
    }
    else
    {
        AIO_printf("\r\n[RESP] %d\t", resp);
    }
#endif
}

void RESP_Handler(void)
{
    RESP_ReslultUpload();
}

/*******************************************************************************
* RESP TEST or DEBUG API Start
*******************************************************************************/
void RESP_SetChannel(u8 channel)
{
    if (channel == (u8)RESP_RA_LL)
    {
        RESP_ChannelSelect(RESP_RA_LL);
    }
    else if (channel == (u8)RESP_RA_LA)
    {
        RESP_ChannelSelect(RESP_RA_LA);
    }
    else if (channel == (u8)RESP_AUTO)//自适应
    {
        //TODO
        RESP_ChannelSelect(RESP_RA_LL);
    }
}

RETURN_TypeDef RESP_setUploadDataType(UartProtocolPacket *pPacket)
{
    u8 type = pPacket->DataAndCRC[0];
    switch (type)
    {
    case 0:
        p_resp_dev->upload_type = FMT_UPLOAD_OFF;
        break;
    case 1:
        p_resp_dev->upload_type = FMT_PACKET;
        break;
    case 2:
        p_resp_dev->upload_type = FMT_FILTER_DATA;
        break;
    default:
        break;
    }
    return RETURN_OK;
}

RETURN_TypeDef RESP_setThreshold(UartProtocolPacket *pPacket)
{
    if (3 != pPacket->Length) return RETURN_ERROR;

    if (pPacket->DataAndCRC[0])
    {
        //manual mode.
    }
    else
    {
        //auto mode.
    }
    
    g_RR_Limits.max = pPacket->DataAndCRC[1];
    g_RR_Limits.min = pPacket->DataAndCRC[2];
    
    l_resp_DebugInterface(RESP_SetHighBoundary, (char *)&g_RR_Limits.max, 1);
    l_resp_DebugInterface(RESP_SetLowBoundary, (char *)&g_RR_Limits.min, 1);
    return RETURN_OK;
}

RETURN_TypeDef RESP_setCarrierWave(UartProtocolPacket *pPacket)
{
    if (1 != pPacket->Length) return RETURN_ERROR;
    
    if (pPacket->DataAndCRC[0]) //close carrier wave
    {
        Timer0_setWorkState(DISABLE);
    }
    else //open
    {
        Timer0_setWorkState(ENABLE);
    }
    return RETURN_OK;
}

RETURN_TypeDef RESP_setAsphyxiaTime(UartProtocolPacket *pPacket)
{
    if (1 != pPacket->Length) return RETURN_ERROR;
    char time = (char)pPacket->DataAndCRC[0];
    
    if (time < 10) time = 10;
    if (time > 40) time = 40;
    
    l_resp_DebugInterface(RESP_SetSuffocationAlertDelay, &time, 1);
    return RETURN_OK;
}

RETURN_TypeDef RESP_DebugInterface(UartProtocolPacket *pPacket)
{
    switch(pPacket->DataAndCRC[0])
    {
    case 0x01:
        break;
    default:
        break;
    }
    return RETURN_OK;
}
/*******************************************************************************
* RESP TEST or DEBUG API stop
*******************************************************************************/


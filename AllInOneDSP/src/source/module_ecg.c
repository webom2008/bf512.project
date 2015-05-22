/*
 * module_ecg.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */
#include <ccblkfn.h>
#include <string.h>

#include "module_ecg.h"


//#define _PRINTF_VOLTAGE_
#define _PRINTF_INT_
//#define _PRINTF_HEX_

//#define _MODULE_ECG_INFO_
#ifdef _MODULE_ECG_INFO_
#define ECG_DBG_INFO(fmt, arg...) AIO_printf("\r\n[ECG] "fmt, ##arg)
#else
#define ECG_DBG_INFO(fmt, arg...)
#endif

#define ECG_DEFAULT_MODE    ECG_MODE_5PROBE//ECG_MODE_3PROBE
#define ECG_CAL_OVERTURN_TIME   500 //overturn time is 500ms(1Hz) CAL signal.
#define ECG_PRINT_RESULT_TIME   2
#define PACE_RefreshPeriodMS    2000

#define ECG1_DATA_VALID     (1<<5)
#define ECG2_DATA_VALID     (1<<4)
#define ECGV_DATA_VALID     (1<<3)

typedef enum
{ 
    ECG_3P_ECG1_I   = 0x01,
    ECG_3P_ECG1_II  = 0x02,
    ECG_3P_ECG1_III = 0x03,
    ECG_5P_I_II_V   = 0x04,
} ECG_DATA_MODE_TypeDef;


TRUE_OR_FALSE g_3probe_run, g_5probe_run;
ECG_3P_Channel_TypeDef g_3probe_channel;
ECG_5P_Channel_TypeDef g_5probe_ECG1, g_5probe_ECG2;
volatile unsigned long g_3PorbeResult[3], g_5PorbeResult[4];
volatile unsigned long g_5Porbe_aVR, g_5Porbe_aVL, g_5Porbe_aVF;
TIM3_HANDLER_TypeDef g_Tim3_target;
bool g_IsECG_Upload, isECGUploadFilter;
#pragma section("sdram0_bank3")
CPACE_STRUCT g_pace_state;

#pragma section("sdram0_bank3")
long g_ecg_result[ECG_Sampling_Buffer_Len][3];//I,II,V;
#pragma section("sdram0_bank3")
unsigned long g_ecg_tick[ECG_Sampling_Buffer_Len];
#pragma section("sdram0_bank3")
char g_ecg_pace[ECG_Sampling_Buffer_Len];
u16  g_ecg_receive_offset,g_ecg_raw_offset,g_ecg_algorithm_offset;
static unsigned long defibrillate_pulse_tick;
static char is_defibrillate_run;

void (*Tim3_HandlerSET[TIMER3_FUNC_LEN])(void);
void ECG_ReslultUpload(void);
void ECG_3ProbeChannelSelect(const ECG_3P_Channel_TypeDef channel);
void ECG_5ProbeChannelSelect(const ECG_5P_Channel_TypeDef ch1, 
                            const ECG_5P_Channel_TypeDef ch2);

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void ECG_Init(void);
#pragma section("sdram0_bank3")
void ECG1_SelectIO_Init(void);
#pragma section("sdram0_bank3")
void ECG2_SelectIO_Init(void);
#pragma section("sdram0_bank3")
void ECGV_SelectIO_Init(void);
#pragma section("sdram0_bank3")
void ECG_DRV_SelectIO_Init(void);
#pragma section("sdram0_bank3")
void ECG_CAL_DRV_IO_Init(void);
#pragma section("sdram0_bank3")
void ECG_Defibrillate_IO_Init(void);
#pragma section("sdram0_bank3")
void ECG_PACE_SelectIO_Init(void);
#pragma section("sdram0_bank3")
void ECG_PACE_DetectIO_Init(void);
#pragma section("sdram0_bank3")
void ECG_PACE_Interrupts_Init(void);
#pragma section("sdram0_bank3")
void ECG_SampleTimer_Init(void);
#pragma section("sdram0_bank3")
void ECG_SampleTimer_Interrupts(void);
#pragma section("sdram0_bank3")
void Tim3_HandlerSET_Init(void);
#pragma section("sdram0_bank3")
void Timer3_Interrupts(void);
#pragma section("sdram0_bank3")
void ECG_SampleTimer_SetInterrupt(const EnableOrDisable status);
#pragma section("sdram0_bank3")
RETURN_TypeDef set_ecg_upload_type(const u8 type);
#pragma section("sdram0_bank3")
RETURN_TypeDef ECG_DebugInterface(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
RETURN_TypeDef ECG_setAlarm(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
void ECG_SetProbeMode(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
void ECG_RespondProbeMode(void);
#pragma section("sdram0_bank3")
void ECG_SetECG12Channel(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
void ECG_SetSTAnalyzeSW(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
void ECG_SetArrhythmiaSW(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
void ECG_SetNotchFilterSW(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
void ECG_SetPaceSW(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
void ECG_SetPaceChannel(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
void ECG_SetPaceOvershootSw(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
void ECG_SetSTMesureSetting(UartProtocolPacket *pPacket);



//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->


static void ECG1_SelectIO_Init(void)
{
    /*setup PF6 and PH0 as an output*/
	*pPORTFIO_INEN &= ~PF6;			/* input buffer disable */
	*pPORTFIO_DIR |= PF6;			/* output */
	*pPORTHIO_INEN &= ~PH0;		    /* input buffer disable */
	*pPORTHIO_DIR |= PH0;			/* output */

	/* clear interrupt settings */
	*pPORTFIO_EDGE &= ~PF6;         // Level Sensitivity
    *pPORTFIO_MASKA_CLEAR = PF6;
	*pPORTHIO_EDGE &= ~PH0;         // Level Sensitivity
    *pPORTHIO_MASKA_CLEAR = PH0;

    /* now clear the flag */
	*pPORTFIO_CLEAR = PF6;
    *pPORTHIO_CLEAR = PH0;
}

static void ECG2_SelectIO_Init(void)
{
	*pPORTFIO_INEN &= ~(PF12 | PF13);			/* input buffer disable */
	*pPORTFIO_DIR |= (PF12 | PF13);			/* output */

	/* clear interrupt settings */
	*pPORTFIO_EDGE &= ~(PF12 | PF13);         // Level Sensitivity
    *pPORTFIO_MASKA_CLEAR = (PF12 | PF13);

    /* now clear the flag */
	*pPORTFIO_CLEAR = PF12;
    *pPORTFIO_CLEAR = PF13;
}

static void ECGV_SelectIO_Init(void)
{
	*pPORTFIO_INEN &= ~PF14;		/* input buffer disable */
	*pPORTFIO_DIR |= PF14;			/* output */

	/* clear interrupt settings */
	*pPORTFIO_EDGE &= ~PF14;         // Level Sensitivity
    *pPORTFIO_MASKA_CLEAR = PF14;

    /* now clear the flag */
	*pPORTFIO_CLEAR = PF14;
}

/**
  * @brief  ECG driver select io initialize SELB(PF8) SLEA(PF7)
  */
static void ECG_DRV_SelectIO_Init(void)
{
	*pPORTFIO_INEN &= ~(PF7 | PF8);			/* input buffer disable */
	*pPORTFIO_DIR |= (PF7 | PF8);			/* output */

	/* clear interrupt settings */
	*pPORTFIO_EDGE &= ~(PF7 | PF8);         // Level Sensitivity
    *pPORTFIO_MASKA_CLEAR = (PF7 | PF8);

    /* now clear the flag */
	*pPORTFIO_CLEAR = PF8;
    *pPORTFIO_CLEAR = PF7;
}

/**
  * @brief  ECG CAL driver io initialize PH5(CAL_DRV) for 1Hz output
  */
static void ECG_CAL_DRV_IO_Init(void)
{
	*pPORTHIO_INEN &= ~PH5;		/* input buffer disable */
	*pPORTHIO_DIR |= PH5;			/* output */

	/* clear interrupt settings */
	*pPORTHIO_EDGE &= ~PH5;         // Level Sensitivity
    *pPORTHIO_MASKA_CLEAR = PH5;

    /* now clear the flag */
	*pPORTHIO_CLEAR = PH5;
}

/**
  * @brief  ECG Defibrillate driver io initialize PF0 output
  */
static void ECG_Defibrillate_IO_Init(void)
{
	*pPORTFIO_INEN &= ~PF0;		/* input buffer disable */
	*pPORTFIO_DIR |= PF0;			/* output */

	/* clear interrupt settings */
	*pPORTFIO_EDGE &= ~PF0;         // Level Sensitivity
    *pPORTFIO_MASKA_CLEAR = PF0;

    /* now clear the flag */
    *pPORTFIO_CLEAR = PF0;
}

void ECG_SetDefibrillate(void)
{
    defibrillate_pulse_tick = SysTick_Get() + getTickNumberByMS(100);
    is_defibrillate_run = 1;
	*pPORTFIO_SET = PF0;
}

static void ECG_Defibrillate_Handle(void)
{
    if ((1 == is_defibrillate_run) && (true == IsOnTime(defibrillate_pulse_tick)))
    {
        *pPORTFIO_CLEAR = PF0;
        is_defibrillate_run = 0;
    }
}

/**
  * @brief  ECG PACE Detect Select IO Init
  */
static void ECG_PACE_SelectIO_Init(void)
{
    /*setup PF6 and PH0 as an output*/
	*pPORTGIO_INEN &= ~PG0;			/* input buffer disable */
	*pPORTGIO_DIR |= PG0;			/* output */
	*pPORTGIO_INEN &= ~PG1;		    /* input buffer disable */
	*pPORTGIO_DIR |= PG1;			/* output */

	/* clear interrupt settings */
	*pPORTGIO_EDGE &= ~PG0;         // Level Sensitivity
    *pPORTGIO_MASKA_CLEAR = PG0;
	*pPORTGIO_EDGE &= ~PG1;         // Level Sensitivity
    *pPORTGIO_MASKA_CLEAR = PG1;

    /* now clear the flag */
	*pPORTGIO_CLEAR = PG0;
    *pPORTGIO_CLEAR = PG1;
}

void ECG_PACE_SetInterrupt(const EnableOrDisable status)
{
    if (ENABLE == status)
    {
        *pPORTFIO_MASKA_SET = PF15;      //Enable PF Interrupt A
    }
    else
    {
        *pPORTFIO_MASKA_CLEAR = PF15;      //Disable PF Interrupt A
    }
}

/**
  * @brief  ECG PACE Detect IO Init
  */
static void ECG_PACE_DetectIO_Init(void)
{
    *pPORTFIO_INEN |= PF15;  //input buffer enable
    *pPORTFIO_DIR &= ~PF15;  //input mode

    *pPORTFIO_EDGE |= PF15;          //Edge Sensitivity
    *pPORTFIO_BOTH  &= ~PF15;        //enabled single edge for edge-sensitivity
    //*pPORTFIO_POLAR &= ~PF15;      //active rising edge.
    *pPORTFIO_POLAR |= PF15;         //active falling edge.
    
   ECG_PACE_SetInterrupt(DISABLE);
}

static void ECG_PACE_Interrupts_Init(void)
{
    //Step1:Enable the SIC(SIC_IMASK) interrupt
    unsigned long imask1 = *pSIC_IMASK1;
    imask1 |= (1 << 13);    //PH A interrupt
    *pSIC_IMASK1 = imask1;

    //Step2(Optionanl):Program the interrupt priority (SIC_IARx)
    /* PF interrupt A bits 23:20 of IAR5, setup for  IVG9 : 2 */
    unsigned long iar5 = *pSIC_IAR5;
    iar5  &= ~(0x0F << 20);
    iar5  |= (2 << 20);
    *pSIC_IAR5 = iar5;

    //Step3:Set IVGx bit in the IMASK
    register_handler(ik_ivg9, IVG9_ISR);//The Same
}

/**
  * @brief  ECG PACE Detect Function
  */
static void ECG_PACE_SetChannel(PACE_Channel_TypeDef channel)
{
    unsigned long ulPort_Reg = 0;
    
    if (CH_PACE1 == channel)
    {
        *pPORTGIO_CLEAR = PG1;
        *pPORTGIO_CLEAR = PG0;

    }
    else if (CH_PACE2 == channel)
    {
        *pPORTGIO_CLEAR = PG1;
        *pPORTGIO_SET = PG0;

    }
    else if (CH_PACEV == channel)
    {
        *pPORTGIO_SET = PG1;
        *pPORTGIO_CLEAR = PG0;
    }
}


static void ECG_PACE_Handler(void)
{
    static unsigned long ulNextChangeTime = 0;
    
    if(IsOnTime(ulNextChangeTime)) 
    {
        g_pace_state.oldState = g_pace_state.newState;
        g_pace_state.newState = false;
        ECG_PACE_SetChannel(g_pace_state.channel);
        ECG_PACE_SetInterrupt(ENABLE);

        if(true == g_pace_state.oldState)
        {
            ECG_DBG_INFO("Detect PACE Singal.");
        }
        ulNextChangeTime += getTickNumberByMS(PACE_RefreshPeriodMS);
    }
}

static void ECG_HR_RR_Upload(void)
{
    static unsigned long ulNextChangeTime = 0;
    
    if ((true == IsOnTime(ulNextChangeTime)) && (true == g_IsECG_Upload))
    {
        u16 HR = 0;
        u8 RR = 0;
        u8 pValue[3] = {0,};
        
        ecg_algorithm_getHR(&HR);
        pValue[0] = (u8)(HR >> 8) & 0xFF;   //HR高字节
        pValue[1] = (u8)(HR & 0xFF);        //HR低字节
        
        resp_algorithm_getRR(&RR);
        if (RR > 150)
        {
            pValue[2] = 0xFF;//RR呼吸率,0xFF表示无效
        }
        else
        {
            pValue[2] = RR;
        }
#ifdef _UPLOAD_1HZ_DATA_
        UploadDataByID(AIO_TX_ECG_HR_RR_ID, TRUE, (char *)pValue, sizeof(pValue));
#endif
        ulNextChangeTime += getTickNumberByMS(1000);
    }
}

void ECG_ArrhythmiaUpload(u8 alert)
{
    u8 pValue[3] = {0,};
    
    pValue[1] = 0;//Reserved
    pValue[2] = 0;//Reserved

    pValue[0] = (u8)(alert & 0xFF);
        
    if (true == g_IsECG_Upload)
    {
        UploadDataByID(AIO_TX_ARRHYTHMIA_RESULT_ID, TRUE, (char *)pValue, sizeof(pValue));
    }
}

void ECG_lead_handler(void)
{
    static unsigned long ulNextChangeTime = 300000;
    u8 lead_info = 0;
    
    if((true == IsOnTime(ulNextChangeTime)) && (true == g_IsECG_Upload))
    {
        u8 pValue[3] = {0,};
        
        /* lead_info
         * detect reslut for probe fall off and overload
         * |-bit7-|-bit6-|-bit5-|-bit4-|-bit3-|-bit2-|-bit1-|-bit0-|
         * |ECGV  |ECG2  |ECG1  |V1_OFF|RL_OFF|LL_OFF|RA_OFF|LA_OFF|
         */
        aio_stm32_readByID(ECG_PROBE_RESULT, &lead_info, 1);
        if (TRUE == g_5probe_run)
        {
            SETBIT(pValue[0], 7);
            SETBIT(pValue[1], 7);
            pValue[0] |= (lead_info & 0x1F);
            pValue[2] = 0x07;
        }
        else//g_3probe_run
        {
            pValue[0] |= (lead_info & 0x07);
            pValue[2] = 0x01;
        }

        pValue[1] = 0; //reserved

        if (pValue[0])//alarm upload
        {
            ECG_DBG_INFO("ECG_lead_handler lead 0x%X",pValue[0]);
#ifdef _UPLOAD_1HZ_DATA_
            UploadDataByID(AIO_TX_ECG_LEAD_INFO_ID, TRUE, (char *)pValue, sizeof(pValue));
#endif
        }

        pValue[0] = (lead_info >> 5) & 0x03;
        if (pValue[0])
        {
            ECG_DBG_INFO("ECG_lead_handler overload 0x%X",pValue[0]);
#ifdef _UPLOAD_1HZ_DATA_
            UploadDataByID(AIO_TX_ECG_OVERLOAD_ID, TRUE, (char *)pValue, 1);
#endif
        }
        //AIO_printf("\r\nECG_lead_handler");
        ulNextChangeTime += getTickNumberByMS(1000);
    }
}

static void ECG1_ChannelSelect(const ECG12_Channel_TypeDef channel)
{
    switch (channel)
    {
        case ECG12_RA_LA_I:   //b#00
            *pPORTHIO_CLEAR = PH0;
            *pPORTFIO_CLEAR = PF6;
            break;
            
        case ECG12_RA_LL_II:   //b#01
            *pPORTHIO_CLEAR = PH0;
            *pPORTFIO_SET = PF6;
            break;
            
        case ECG12_LA_LL_III:   //b#10
            *pPORTHIO_SET = PH0;
            *pPORTFIO_CLEAR = PF6;
            break;
            
        case ECG12_CAL:     //b#11
            *pPORTHIO_SET = PH0;
            *pPORTFIO_SET = PF6;
            break;
            
        default:
            break;
    }
}

static void ECG2_ChannelSelect(const ECG12_Channel_TypeDef channel)
{
    switch (channel)
    {
        case ECG12_RA_LA_I:   //b#00
            *pPORTFIO_CLEAR = PF13;
            *pPORTFIO_CLEAR = PF12;
            break;
            
        case ECG12_RA_LL_II:   //b#01
            *pPORTFIO_CLEAR = PF13;
            *pPORTFIO_SET = PF12;
            break;
            
        case ECG12_LA_LL_III:   //b#10
            *pPORTFIO_SET = PF13;
            *pPORTFIO_CLEAR = PF12;
            break;
            
        case ECG12_CAL:     //b#11
            *pPORTFIO_SET = PF13;
            *pPORTFIO_SET = PF12;
            break;
            
        default:
            break;
    }
}

static void ECGV_ChannelSelect(const ECGV_Channel_TypeDef channel)
{
    if (ECGV_CAL == channel)
    {
        *pPORTFIO_CLEAR = PF14;
    }
    else
    {
        *pPORTFIO_SET = PF14;
    }
}

static void ECG_DRV_ChannelSelect(const ECG_DRV_Channel_TypeDef channel)
{
    switch (channel)
    {
        case ECG_DRV_RA:   //b#00
        	*pPORTFIO_CLEAR = PF8;
            *pPORTFIO_CLEAR = PF7;
            break;
            
        case ECG_DRV_LA:   //b#01
        	*pPORTFIO_CLEAR = PF8;
            *pPORTFIO_SET = PF7;
            break;
            
        case ECG_DRV_LL:   //b#10
        	*pPORTFIO_SET = PF8;
            *pPORTFIO_CLEAR = PF7;
            break;
            
        case ECG_DRV_RL:  //b#11
            *pPORTFIO_SET = PF8;
            *pPORTFIO_SET = PF7;
            break;
            
        default:
            break;
    }
}

void ECG_CAL_DRV_ModeSet(EnableOrDisable status)
{
    if (ENABLE == status)
    {
        ECG1_ChannelSelect(ECG12_CAL);
        ECG2_ChannelSelect(ECG12_CAL);
        ECGV_ChannelSelect(ECGV_CAL);
    }
    else
    {
        if (TRUE == g_3probe_run)
        { 
            ECG_3ProbeChannelSelect(g_3probe_channel);
        }
        else
        {
            ECG_5ProbeChannelSelect(g_5probe_ECG1, g_5probe_ECG2);
            ECGV_ChannelSelect(ECGV_V1);
        }
    }
}

//Timer 2 initialize
static void ECG_SampleTimer_Init(void)
{
    unsigned long sclk = getSystemCLK();
    unsigned long ratio = 0;
    
    /*****************************************************************************
     *
     *   pwm_out mode, interrupt enable, count to end of period
     * 
     ****************************************************************************/
    *pTIMER2_CONFIG = 0x0059;

    sclk += (ECG_SAMPLE_FREQ /2);     //round up before divide by RESP_CARRIER_FREQ
    ratio = sclk / ECG_SAMPLE_FREQ;
    
    *pTIMER2_PERIOD= ratio;         //timer2 = ratio * (1/132)us
    *pTIMER2_WIDTH= (ratio / 2);    //timer width, 50%
}

void ECG_SampleTimer_SetInterrupt(const EnableOrDisable status)
{
    if (ENABLE == status)
    {
        *pTIMER_ENABLE = (1<<2);
    }
    else
    {
        *pTIMER_DISABLE = (1<<2);
    }
}

static void ECG_SampleTimer_Interrupts(void)
{
    //Step1:Enable the SIC(SIC_IMASK) interrupt
    unsigned long imask1 = *pSIC_IMASK1;
    imask1 |= (1 << 2);    //Tmier2 interrupt
    *pSIC_IMASK1 = imask1;

    //Step2(Optionanl):Program the interrupt priority (SIC_IARx)
    /* Timer2 bits 11:8 of IAR4, setup for  IVG8 : 1 */
    unsigned long iar4 = *pSIC_IAR4;
    iar4  &= ~(0x0F << 8);
    iar4  |= (1 << 8);
    *pSIC_IAR4 = iar4;

    //Step3:Set IVGx bit in the IMASK
    register_handler(ik_ivg8, IVG8_ISR);//The Same
}

/*****************************************************************************
 *
 *   Timer3 interface.
 * 
 ****************************************************************************/
static void Timer3_SetInterrupt(const EnableOrDisable status)
{
    if (ENABLE == status)
    {
        *pTIMER_ENABLE = (1<<3);
        *pSIC_IMASK1 |= (1 << 3);
    }
    else
    {
        *pTIMER_DISABLE = (1<<3);
        *pSIC_IMASK1 &= ~(1<<3);
    }
}

void Timer3_Configure(const unsigned int n1us, const TIM3_HANDLER_TypeDef target)
{
    unsigned long sclk = getSystemCLK();
    unsigned long ratio = 0, freq = 0;
    
    Timer3_SetInterrupt(DISABLE);
    g_Tim3_target = target;
    
/*****************************************************************************
 *
 *   pwm_out mode, interrupt enable, count to end of period
 * 
 ****************************************************************************/
    *pTIMER3_CONFIG = 0x0059;

    freq = 1000000 / n1us;//1/(n*1us)=10^6/n Hz

    sclk += (freq /2);     //round up before divide by freq
    ratio = sclk / freq;
    
    *pTIMER3_PERIOD= ratio;
    *pTIMER3_WIDTH= (ratio / 2);    //timer width, 50%
    
    Timer3_SetInterrupt(ENABLE);
}

static void Timer3_Interrupts(void)
{
    //Step1:Enable the SIC(SIC_IMASK) interrupt
    unsigned long imask1 = *pSIC_IMASK1;
    imask1 |= (1 << 3);    //Tmier3 interrupt
    *pSIC_IMASK1 = imask1;

    //Step2(Optionanl):Program the interrupt priority (SIC_IARx) 
    /* Timer3 bits 15:12 of IAR4, setup for  IVG10 : 3 */
    unsigned long iar4 = *pSIC_IAR4;
    iar4  &= 0xFFFF0FFF;
    iar4  |= 0x00003000;
    *pSIC_IAR4 = iar4;

    //Step3:Set IVGx bit in the IMASK
    register_handler(ik_ivg10, IVG10_ISR);
}

static void Tim3_HandlerSET_Init(void)
{
    Tim3_HandlerSET[TIM3_ECG_ADC_R] = ECG_ADC_SaveData;
    Tim3_HandlerSET[TIM3_ECG_3Probe] = ECG_ADC_3Probe_Handler;
}

//End of Timer3 interface.

void ECG_3ProbeChannelSelect(const ECG_3P_Channel_TypeDef channel)
{
    switch (channel)
    {
        case ECG_3PROBE_I:
            ECG1_ChannelSelect(ECG12_RA_LA_I);
            ECG_DRV_ChannelSelect(ECG_DRV_LL);
            break;
        case ECG_3PROBE_II:
            ECG1_ChannelSelect(ECG12_RA_LL_II);
            ECG_DRV_ChannelSelect(ECG_DRV_LA);
            break;
        case ECG_3PROBE_III:
            ECG1_ChannelSelect(ECG12_LA_LL_III);
            ECG_DRV_ChannelSelect(ECG_DRV_RA);
            break;
        default:
            break;
    }
    g_3probe_channel = channel;
}

void ECG_5ProbeChannelSelect(const ECG_5P_Channel_TypeDef ch1, 
                            const ECG_5P_Channel_TypeDef ch2)
{
    switch (ch1)    //ECG_ADC ECG1 channel target
    {
        case ECG_5PROBE_I:
            ECG1_ChannelSelect(ECG12_RA_LA_I);
            break;
        case ECG_5PROBE_II:
            ECG1_ChannelSelect(ECG12_RA_LL_II);
            break;
        case ECG_5PROBE_III:
            ECG1_ChannelSelect(ECG12_LA_LL_III);
            break;
        default:
            break;
    }
    
    switch (ch2)    //ECG_ADC ECG2 channel target
    {
        case ECG_5PROBE_I:
            ECG2_ChannelSelect(ECG12_RA_LA_I);
            break;
        case ECG_5PROBE_II:
            ECG2_ChannelSelect(ECG12_RA_LL_II);
            break;
        case ECG_5PROBE_III:
            ECG2_ChannelSelect(ECG12_LA_LL_III);
            break;
        default:
            break;
    }
    
    g_5probe_ECG1 = ch1;
    g_5probe_ECG2 = ch2;
    ECG_DRV_ChannelSelect(ECG_DRV_RL);
}

void ECG_PorbeModeSet(ECG_MODE_TypeDef mode)
{
    if (ECG_MODE_3PROBE == mode)
    {
        g_3probe_run = TRUE;
        g_5probe_run = FALSE;
    }
    else
    {
        g_3probe_run = FALSE;
        g_5probe_run = TRUE;
    }
}

void ECG_Init(void)
{
    g_Tim3_target = TIM3_ECG_ADC_R;
    g_IsECG_Upload = true;
    isECGUploadFilter = false;
    g_ecg_receive_offset = g_ecg_raw_offset = g_ecg_algorithm_offset = 0;
    is_defibrillate_run = 0;
    memset((char *)g_3PorbeResult,0,sizeof(g_3PorbeResult));
    memset((char *)g_5PorbeResult,0,sizeof(g_5PorbeResult));
    ECG_PorbeModeSet(ECG_DEFAULT_MODE);

    ECG_ADC_Init();
    ECG1_SelectIO_Init();
    ECG2_SelectIO_Init();
    ECGV_SelectIO_Init();
    ECG_DRV_SelectIO_Init();
    ECG_CAL_DRV_IO_Init();
    ECG_Defibrillate_IO_Init();

    //PACE detect Init
    g_pace_state.newState = false;
    g_pace_state.oldState = false;
    g_pace_state.sw = false;
    g_pace_state.channel = CH_PACE1;
    ECG_PACE_SelectIO_Init();
    ECG_PACE_DetectIO_Init();
    ECG_PACE_Interrupts_Init();

    if (TRUE == g_3probe_run)
    { 
        ECG_3ProbeChannelSelect(ECG_3PROBE_II);
    }
    else
    {
        ECG_5ProbeChannelSelect(ECG_5PROBE_II, ECG_5PROBE_I);
        ECGV_ChannelSelect(ECGV_V1);
    }
    
    ECG_SampleTimer_Init();
    ECG_SampleTimer_Interrupts();

    Tim3_HandlerSET_Init();
    Timer3_Interrupts();
}

void ECG_Handler(void)
{
    ECG_lead_handler();
    ECG_Defibrillate_Handle();
    ECG_HR_RR_Upload();
    ECG_ReslultUpload();
}

void ECG_CAL_DRV_signal(void)
{
    static unsigned long ulNextChangeTime = 0;
    static char s8Status = 0;
    
    if(IsOnTime(ulNextChangeTime)) //CAL_DRV signal 1Hz square wave
    {
        s8Status = !s8Status;
        if(s8Status)
        {
            *pPORTHIO_SET = PH5;    //High level
        }
        else
        {
            *pPORTHIO_CLEAR = PH5;  //Low level
        }
        ulNextChangeTime += getTickNumberByMS(ECG_CAL_OVERTURN_TIME);
    }
}

//<!-- ECG ADC Result API Begin -->
void ECG_adc_store_data(void)
{
    g_ecg_result[g_ecg_receive_offset][0] = 0;
    g_ecg_result[g_ecg_receive_offset][1] = 0;
    g_ecg_result[g_ecg_receive_offset][2] = 0;
    
    if (TRUE == g_5probe_run)
    {
        g_ecg_result[g_ecg_receive_offset][0] = s24_to_s32(g_5PorbeResult[0]);//I
        g_ecg_result[g_ecg_receive_offset][1] = s24_to_s32(g_5PorbeResult[1]);//II
        g_ecg_result[g_ecg_receive_offset][2] = s24_to_s32(g_5PorbeResult[3]);//V
    }
    else //(TRUE == g_3probe_run)
    {
        g_ecg_result[g_ecg_receive_offset][g_3probe_channel] = \
            s24_to_s32(g_3PorbeResult[g_3probe_channel]);
    }
    g_ecg_tick[g_ecg_receive_offset] = SysTick_Get();
    
    if (true == g_pace_state.newState)
    {
        g_pace_state.newState = false;
        g_ecg_pace[g_ecg_receive_offset] = 1;
    }
    else
    {
        g_ecg_pace[g_ecg_receive_offset] = 0;
    }
        
    g_ecg_receive_offset += 1;
    if(g_ecg_receive_offset >= ECG_Sampling_Buffer_Len)  g_ecg_receive_offset -= ECG_Sampling_Buffer_Len;
    if(g_ecg_algorithm_offset == g_ecg_receive_offset)    // overflow
    {
        g_ecg_algorithm_offset += 1;
        if(g_ecg_algorithm_offset >= ECG_Sampling_Buffer_Len)    g_ecg_algorithm_offset -= ECG_Sampling_Buffer_Len;
    }
    if(g_ecg_raw_offset == g_ecg_receive_offset)          // overflow
    {
        g_ecg_raw_offset += 1;
        if(g_ecg_raw_offset >= ECG_Sampling_Buffer_Len)  g_ecg_raw_offset -= ECG_Sampling_Buffer_Len;
    }
}

long ECG_GetResult_I(void)
{
    s32 s32Index;

    s32Index = g_ecg_receive_offset - 1;
    if(s32Index < 0)    s32Index += ECG_Sampling_Buffer_Len;

    return g_ecg_result[s32Index][0];
}

long ECG_GetResult_II(void)
{
    s32 s32Index;

    s32Index = g_ecg_receive_offset - 1;
    if(s32Index < 0)    s32Index += ECG_Sampling_Buffer_Len;

    return g_ecg_result[s32Index][1];
}

long ECG_GetResult_V(void)
{
    s32 s32Index;

    s32Index = g_ecg_receive_offset - 1;
    if(s32Index < 0)    s32Index += ECG_Sampling_Buffer_Len;

    return g_ecg_result[s32Index][2];
}

static u16 ECG_GetDataLen(void)
{
    s32 s32Len;

    s32Len = g_ecg_receive_offset - g_ecg_raw_offset;
    if(s32Len < 0)  s32Len += ECG_Sampling_Buffer_Len;

    return (u16)s32Len;
}

static u16 ECG_PopResult(u32* pu32Tick, s32* ps32I, s32* ps32II, s32* ps32V, char* pPace)
{
    u16 s32Len = ECG_GetDataLen();

    if(s32Len)
    {
        *pu32Tick = g_ecg_tick[g_ecg_raw_offset];
        *ps32I = g_ecg_result[g_ecg_raw_offset][0];
        *ps32II = g_ecg_result[g_ecg_raw_offset][1];
        *ps32V = g_ecg_result[g_ecg_raw_offset][2];
        *pPace = g_ecg_pace[g_ecg_raw_offset];

        g_ecg_raw_offset += 1;
        if(g_ecg_raw_offset >= ECG_Sampling_Buffer_Len)  g_ecg_raw_offset -= ECG_Sampling_Buffer_Len;
    }
    return s32Len;
}

static u16 ECG_GetAlgoDataLen(void)
{
    s32 s32Len;

    s32Len = g_ecg_receive_offset - g_ecg_algorithm_offset;
    if(s32Len < 0)  s32Len += ECG_Sampling_Buffer_Len;

    return (u16)s32Len;
}

u16 ECG_PopAlgoResult(u32* pu32Tick, s32* ps32I, s32* ps32II, s32* ps32V, char* pPace)
{
    u16 s32Len = ECG_GetAlgoDataLen();

    if(s32Len)
    {
        *pu32Tick = g_ecg_tick[g_ecg_algorithm_offset];
        *ps32I = g_ecg_result[g_ecg_algorithm_offset][0];
        *ps32II = g_ecg_result[g_ecg_algorithm_offset][1];
        *ps32V = g_ecg_result[g_ecg_algorithm_offset][2];
        *pPace = g_ecg_pace[g_ecg_algorithm_offset];

        g_ecg_algorithm_offset += 1;
        if(g_ecg_algorithm_offset >= ECG_Sampling_Buffer_Len)    g_ecg_algorithm_offset -= ECG_Sampling_Buffer_Len;
    }

    return s32Len;
}

void ECG_DataReset(void)
{
    g_ecg_raw_offset = g_ecg_receive_offset;
    g_ecg_algorithm_offset = g_ecg_receive_offset;
}
//<!-- ECG ADC Result API End -->

static void ECG_ReslultUpload(void)
{
#ifdef _UPLOAD_ECG_ADC_
    if (false == g_IsECG_Upload)//判断是否需要上传数据
    {
        return;
    }
    
    char pValue[15];
    char pace = 0;
    u32 curTick = SysTick_Get();
    memset(pValue, 0, sizeof(pValue));
    
    s32 ecg[3] = {0,};
    u32 tick = 0;
    u16 nLen = 0;
    
    if (false == isECGUploadFilter)//上传未经滤波的数据
    {
        nLen = ECG_PopResult(&tick, &ecg[0], &ecg[1], &ecg[2], &pace);//数据格式为有符号32位
    }
    else//上传滤波后的数据
    {
        nLen = (u16)ecg_algorithm_PopFilterResult(&tick, &ecg[0], &ecg[1], &ecg[2], &pace);//数据格式为有符号32位
    }
    
    if (!nLen) return;//无数据更新，退出
    
    //有符号32位转为有符号24位
    ecg[0] = s32_to_s24(ecg[0]);
    ecg[1] = s32_to_s24(ecg[1]);
    ecg[2] = s32_to_s24(ecg[2]);
    
    pValue[1] = (char)(ecg[0] >> 16) & 0xFF;      //ECG1 Vaule MSB
    pValue[2] = (char)(ecg[0] >> 8) & 0xFF;
    pValue[3] = (char)(ecg[0] >> 0) & 0xFF;       //ECG1 Vaule LSB
    
    pValue[4] = (char)(ecg[1] >> 16) & 0xFF;      //ECG2 Vaule MSB
    pValue[5] = (char)(ecg[1] >> 8) & 0xFF;
    pValue[6] = (char)(ecg[1] >> 0) & 0xFF;       //ECG2 Vaule LSB
    
    pValue[7] = (char)(ecg[2] >> 16) & 0xFF;      //ECGV Vaule MSB
    pValue[8] = (char)(ecg[2] >> 8) & 0xFF;
    pValue[9] = (char)(ecg[2] >> 0) & 0xFF;       //ECGV Vaule LSB
    
    pValue[10] = (char)(tick >> 24) & 0xFF;    //System Tick MSB
    pValue[11] = (char)(tick >> 16) & 0xFF;
    pValue[12] = (char)(tick >> 8) & 0xFF;
    pValue[13] = (char)(tick >> 0) & 0xFF;     //System Tick LSB

    if (TRUE == g_3probe_run)
    {
        if (ECG_3PROBE_I == g_3probe_channel)
        {
            pValue[0] = ECG1_DATA_VALID | ECG_3P_ECG1_I;
        }
        else if (ECG_3PROBE_II == g_3probe_channel)
        {
            pValue[0] = ECG1_DATA_VALID | ECG_3P_ECG1_II;
        }
        else if (ECG_3PROBE_III == g_3probe_channel)
        {
            pValue[0] = ECG1_DATA_VALID | ECG_3P_ECG1_III;
        }
    }//End of 3 导联情况
    else//5导联情况下
    {
        pValue[0] = ECG1_DATA_VALID | ECG2_DATA_VALID | ECGV_DATA_VALID\
                    | ECG_5P_I_II_V;
    }
    
    pValue[14] = pace; //PACE signal
    //if (pace) AIO_printf("\r\nGet a PACE Signal!");
    UploadDataByID(AIO_TX_ECG_REALTIME_ID, TRUE, pValue, sizeof(pValue));
#endif
}

void ECG_PrintResult(void)
{
#ifdef _PRINT_ECG_SAMPLE_

    s32 ecg[3] = {0,};
    u32 tick = 0;
    u16 nLen = 0;
    char pace = 0;
    
    nLen = ECG_PopResult(&tick, &ecg[0], &ecg[1], &ecg[2], &pace);//数据格式为有符号32位

    if (!nLen) return;//无数据更新，退出

#if defined(_PRINTF_HEX_)
#error ECG_PrintResult()
#elif defined(_PRINTF_INT_)
        AIO_printf("\r\n%d\t%d\t%d\t%d\t%d",tick,ecg[0],ecg[1],ecg[2],pace);
#elif defined(_PRINTF_VOLTAGE_)
#error ECG_PrintResult()
#endif

#endif/* _PRINT_ECG_SAMPLE_ */
}


/*******************************************************************************
* ECG TEST or DEBUG API start
*******************************************************************************/
static RETURN_TypeDef set_ecg_upload_type(const u8 type)
{
    switch(type)
    {
    case 0x01://无数据上传
        g_IsECG_Upload = false;
        break;
    case 0x02://滤波前数据
        isECGUploadFilter = false;
        g_IsECG_Upload = true;
        break;
    case 0x03://滤波后数据
        isECGUploadFilter = true;
        g_IsECG_Upload = true;
        break;
    default:
        return RETURN_ERROR;
    }
    ecg_algorithm_DataReset();
    return RETURN_OK;
}

RETURN_TypeDef ECG_DebugInterface(UartProtocolPacket *pPacket)
{
    u8 u8Type = pPacket->DataAndCRC[0];
    RETURN_TypeDef result = RETURN_ERROR;
    switch(u8Type)
    {
    case 0x01://ECG打印或上传格式
        result = set_ecg_upload_type(pPacket->DataAndCRC[1]);
        break;
    case 0x02://ECG algo run or not
        ecg_algorithm_setRun(pPacket->DataAndCRC[1]);
        result = RETURN_OK;
        break;
    case 0x03:
        l_ecg_DebugInterface(ECG_Tachycardia_Limit_SET, &pPacket->DataAndCRC[1], 2);
        break;
    case 0x04:
        l_ecg_DebugInterface(ECG_Bradycardia_Limit_SET, &pPacket->DataAndCRC[1], 2);
        break;
    default:
        break;
    }
    
    return result;
}

RETURN_TypeDef ECG_setAlarm(UartProtocolPacket *pPacket)
{
    //TODO.ARM-A8 function.@reserved.
    return RETURN_OK;
}


//设置当前导联工作模式
void ECG_SetProbeMode(UartProtocolPacket *pPacket)
{
    if (0 == pPacket->DataAndCRC[1])
    {
        ECG_PorbeModeSet(ECG_MODE_5PROBE);
        ECG_5ProbeChannelSelect(ECG_5PROBE_II, ECG_5PROBE_I);
        ECGV_ChannelSelect(ECGV_V1);
    }
    else if (1 == pPacket->DataAndCRC[1])
    {
        ECG_PorbeModeSet(ECG_MODE_3PROBE);
        ECG_3ProbeChannelSelect(ECG_3PROBE_II);
    }
}

//获取当前工作导联的模式
void ECG_RespondProbeMode(void)
{
    char pVal[2];
    pVal[0] = 0;
    if (TRUE == g_3probe_run)
    {
        pVal[1] = 1;
    }
    else if (TRUE == g_5probe_run)
    {
        pVal[1] = 0;
    }
    UploadDataByID(AIO_RX_PROBE_MODE_ID, TRUE, pVal, 2);
}

void ECG_SetECG12Channel(UartProtocolPacket *pPacket)
{
    if (pPacket->DataAndCRC[0]) //read
    {
        char pVal[2];
        pVal[0] = 0;
        if (TRUE == g_3probe_run)
        {
            pVal[1] = 1 << 4; //ECG1
            pVal[1] += (g_3probe_channel + 1);//bit3~0:channel
            UploadDataByID(AIO_RX_ECG12_CHANNEL_ID, TRUE, pVal, 2);
        }
        else if (TRUE == g_5probe_run)
        {
            u8 ECGn = pPacket->DataAndCRC[1] >> 4;
            if (1 == ECGn)
            {
                pVal[1] = 1 << 4; //ECG1
                pVal[1] += (g_5probe_ECG1 + 1);//bit3~0:channel
            }
            else if (2 == ECGn)
            {
                pVal[1] = 2 << 4; //ECG2
                pVal[1] += (g_5probe_ECG2 + 1);//bit3~0:channel
            }
            UploadDataByID(AIO_RX_ECG12_CHANNEL_ID, TRUE, pVal, 2);
        }
    }
    else //write
    {
        if (TRUE == g_3probe_run)
        {
            if ((pPacket->DataAndCRC[1] >> 4) != 0x01) return;//NOT ECG1,return
            switch (pPacket->DataAndCRC[1] & 0x0F) //bit0~3
            {
                case 0x01:
                    ECG_3ProbeChannelSelect(ECG_3PROBE_I);
                    break;
                case 0x02:
                    ECG_3ProbeChannelSelect(ECG_3PROBE_II);
                    break;
                case 0x03:
                    ECG_3ProbeChannelSelect(ECG_3PROBE_III);
                    break;
                case 0x04://AVR
                case 0x05://AVL
                case 0x06://AVF
                default:
                    break;
            }
        }
        else if (TRUE == g_5probe_run)
        {
            ECG_5P_Channel_TypeDef ch1 = g_5probe_ECG1;
            ECG_5P_Channel_TypeDef ch2 = g_5probe_ECG2;
            ECG_5P_Channel_TypeDef ch;
            switch (pPacket->DataAndCRC[1] & 0x0F) //bit0~3
            {
                case 0x01:
                    ch = ECG_5PROBE_I;
                    break;
                case 0x02:
                    ch = ECG_5PROBE_II;
                    break;
                case 0x03:
                    ch = ECG_5PROBE_III;
                    break;
                case 0x04://AVR
                case 0x05://AVL
                case 0x06://AVF
                default:
                    break;
            }
            
            u8 ECGn = pPacket->DataAndCRC[1] >> 4;
            if (1 == ECGn)
            {
                ch1 = ch;
            }
            else if (2 == ECGn)
            {
                ch2 = ch;
            }
            ECG_5ProbeChannelSelect(ch1, ch2);
        }
    }
}

void ECG_SetSTAnalyzeSW(UartProtocolPacket *pPacket)
{
    l_ecg_DebugInterface(ECG_ST_SW, &pPacket->DataAndCRC[0], 1);
}

void ECG_SetSTMesureSetting(UartProtocolPacket *pPacket)
{
    l_ecg_DebugInterface(ECG_ST_MEASURE_SETTING, &pPacket->DataAndCRC[0], 4);
}

void ECG_SetArrhythmiaSW(UartProtocolPacket *pPacket)
{
    l_ecg_DebugInterface(ECG_ARRHYTHMIA_SW, &pPacket->DataAndCRC[0], 1);
}

void ECG_NotchFilterHandle(u8 sw)
{
    sw &= 0x03; //bit0:50Hz,bit1:60Hz
    l_ecg_DebugInterface(ECG_NOTCH_SELECT, &sw, 1);
}

void ECG_SetNotchFilterSW(UartProtocolPacket *pPacket)
{
    ECG_NotchFilterHandle(pPacket->DataAndCRC[0]);
}


void ECG_SetPaceSW(UartProtocolPacket *pPacket)
{
    u8 sw;
    if (pPacket->DataAndCRC[0])
    {
        sw = 1;
        g_pace_state.sw = true;
        ECG_PACE_SetChannel(g_pace_state.channel);
        ECG_PACE_SetInterrupt(ENABLE);
        //AIO_printf("\r\nPACE Enable!");
    }
    else
    {
        sw = 0;
        g_pace_state.sw = false;
        ECG_PACE_SetInterrupt(DISABLE);
        //AIO_printf("\r\nPACE Disable!");
    }
    l_ecg_DebugInterface(ECG_PACE_SW, &sw, 1);
}

void ECG_SetPaceChannel(UartProtocolPacket *pPacket)
{
    if (1 == pPacket->DataAndCRC[0])
    {
        g_pace_state.channel = CH_PACE1;
    }
    else if (2 == pPacket->DataAndCRC[0])
    {
        g_pace_state.channel = CH_PACE2;
    }
    else if (3 == pPacket->DataAndCRC[0])
    {
        g_pace_state.channel = CH_PACEV;
    }
}

void ECG_SetPaceOvershootSw(UartProtocolPacket *pPacket)
{
    l_ecg_DebugInterface(ECG_PACE_OVERSHOOT_SW, &pPacket->DataAndCRC[0], 1);
}

/*******************************************************************************
* ECG TEST or DEBUG API stop
*******************************************************************************/

EX_INTERRUPT_HANDLER(IVG10_ISR)
{
    unsigned long timerStatus = *pTIMER_STATUS;
    //unsigned int uiTIMASK;
    if (timerStatus & TIMIL3)   //Timer3 interrupt happen.
    {
        *pTIMER_STATUS = TIMIL3;//Must clear IRQ
        Timer3_SetInterrupt(DISABLE);
        //uiTIMASK = cli();
        (*Tim3_HandlerSET[g_Tim3_target])(); 
        //sti(uiTIMASK);
    }
#ifdef _TIM1_TIM3_SAME_INTERRUPT_
    else if (timerStatus & TIMIL1)   //Timer1 interrupt happen.
    {
        *pTIMER_STATUS = TIMIL1;//Must clear IRQ
        Timer1_SetInterrupt(DISABLE);
        //uiTIMASK = cli();
        (*Tim1_HandlerSET[g_Tim1_target])(); 
        //sti(uiTIMASK);
    }
#endif
    else
    {
        AIO_printf("\r\nIVG10_ISR>>>>>>>>>>>>>>>>>>>>>>>>>>>>Error!");
    }
}



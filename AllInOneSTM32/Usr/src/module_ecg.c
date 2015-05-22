#include "module_ecg.h"

#define LOFF_SEL_A_PIN      GPIO_Pin_15
#define LOFF_SEL_A_PORT     GPIOB
#define LOFF_SEL_A_CLK      RCC_APB2Periph_GPIOB
#define LOFF_SEL_B_PIN      GPIO_Pin_14
#define LOFF_SEL_B_PORT     GPIOB
#define LOFF_SEL_B_CLK      RCC_APB2Periph_GPIOB
#define LOFF_SEL_C_PIN      GPIO_Pin_13
#define LOFF_SEL_C_PORT     GPIOB
#define LOFF_SEL_C_CLK      RCC_APB2Periph_GPIOB

#define SEL_A_HIGH()        GPIO_SetBits(LOFF_SEL_A_PORT, LOFF_SEL_A_PIN)
#define SEL_A_LOW()         GPIO_ResetBits(LOFF_SEL_A_PORT, LOFF_SEL_A_PIN)
#define SEL_B_HIGH()        GPIO_SetBits(LOFF_SEL_B_PORT, LOFF_SEL_B_PIN)
#define SEL_B_LOW()         GPIO_ResetBits(LOFF_SEL_B_PORT, LOFF_SEL_B_PIN)
#define SEL_C_HIGH()        GPIO_SetBits(LOFF_SEL_C_PORT, LOFF_SEL_C_PIN)
#define SEL_C_LOW()         GPIO_ResetBits(LOFF_SEL_C_PORT, LOFF_SEL_C_PIN)

#define ECG_DETECT_TIME         100//ms
#define ECG_PROBE_PERIOD        500//ms

#define ECG_PROBE_ON_VAL        (1285)
#define ECG_PROBE_ON_RANGE      (10)     //over or  less 20%
#define ECG_PROBE_ON_VAL_MAX    (ECG_PROBE_ON_VAL * (100 + ECG_PROBE_ON_RANGE) / 100)
#define ECG_PROBE_ON_VAL_MIN    (ECG_PROBE_ON_VAL * (100 - ECG_PROBE_ON_RANGE) / 100)
#define ECG_PROBE_RL_VAL        (1430)
#define ECG_PROBE_RL_RANGE      (10)     //over or  less 20%
#define ECG_PROBE_RL_VAL_MAX    (ECG_PROBE_RL_VAL * (100 + ECG_PROBE_RL_RANGE) / 100)
#define ECG_PROBE_RL_VAL_MIN    (ECG_PROBE_RL_VAL * (100 - ECG_PROBE_RL_RANGE) / 100)

//#define _ECG_PRINT_V_

//#define ECG_DEBUG_INFO
#ifdef ECG_DEBUG_INFO
#define ECG_DBG_INFO(fmt, arg...) printf("\r\n[ECG] "fmt, ##arg)
#else
#define ECG_DBG_INFO(fmt, arg...)
#endif

/*
 * detect reslut for probe fall off and overload
 * |-bit7-|-bit6-|-bit5-|-bit4-|-bit3-|-bit2-|-bit1-|-bit0-|
 * |ECGV  |ECG2  |ECG1  |V1_OFF|LL_OFF|LA_OFF|RL_OFF|RA_OFF|
 */
u8 g_u8ecg_status;

ECG_SelectTypeDef g_ecg_target;
ECG_ResultTypeDef g_ecg_result;
ECG_ResultTypeDef *pECGResult = &g_ecg_result;

static void ECG_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(LOFF_SEL_A_CLK | LOFF_SEL_B_CLK | LOFF_SEL_C_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = LOFF_SEL_A_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(LOFF_SEL_A_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = LOFF_SEL_B_PIN;
    GPIO_Init(LOFF_SEL_B_PORT, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = LOFF_SEL_C_PIN;
    GPIO_Init(LOFF_SEL_C_PORT, &GPIO_InitStructure);
}

static RETURN_Type ECG_SelectChannel(ECG_SelectTypeDef channel)
{
    switch(channel)
    {
        case RA_OFF://000
        {
            SEL_C_LOW();
            SEL_B_LOW();
            SEL_A_LOW();
            break;
        }
        case RL_OFF://001
        {
            SEL_C_LOW();
            SEL_B_LOW();
            SEL_A_HIGH();
            break;
        }
        case LA_OFF://010
        {
            SEL_C_LOW();
            SEL_B_HIGH();
            SEL_A_LOW();
            break;
        }
        case LL_OFF://011
        {
            SEL_C_LOW();
            SEL_B_HIGH();
            SEL_A_HIGH();
            break;
        }
        case V1_OFF://100
        {
            SEL_C_HIGH();
            SEL_B_LOW();
            SEL_A_LOW();
            break;
        }
        case ECG1_OVERLOAD://101
        {
            SEL_C_HIGH();
            SEL_B_LOW();
            SEL_A_HIGH();
            break;
        }
        case ECG2_OVERLOAD://110
        {
            SEL_C_HIGH();
            SEL_B_HIGH();
            SEL_A_LOW();
            break;
        }
        case ECGV_OVERLOAD://111
        {
            SEL_C_HIGH();
            SEL_B_HIGH();
            SEL_A_HIGH();
            break;
        }
        default:
            printf("\r\nECG_SelectChannel error = %d",channel);
            return RETURN_ERROR;
    }
    g_ecg_target = channel;
    return RETURN_SUCCESS;
}

static RETURN_Type ECG_SelectNextChannel(void)
{
    RETURN_Type result = RETURN_ERROR;
    ECG_SelectTypeDef curType = g_ecg_target;
    if (curType == ECGV_OVERLOAD)
    {
        result = ECG_SelectChannel(RA_OFF);
    }
    else
    {
        curType++;
        result = ECG_SelectChannel(curType);
    }
    return result;
}

static RETURN_Type ECG_DetectDataSave(void)
{
    u16 curValue = g_SysADCResult.DET_OVLD_OFF;
    
    switch(g_ecg_target)
    {
        case RA_OFF:
        {
            pECGResult->RA_OFF_Val = curValue;
            break;
        }
        case RL_OFF:
        {
            pECGResult->RL_OFF_Val = curValue;
            break;
        }
        case LA_OFF:
        {
            pECGResult->LA_OFF_Val = curValue;
            break;
        }
        case LL_OFF:
        {
            pECGResult->LL_OFF_Val = curValue;
            break;
        }
        case V1_OFF:
        {
            pECGResult->V1_OFF_Val= curValue;
            break;
        }
        case ECG1_OVERLOAD:
        {
            pECGResult->ECG1_OVERLOAD_Val= curValue;
            break;
        }
        case ECG2_OVERLOAD:
        {
            pECGResult->ECG2_OVERLOAD_Val= curValue;
            break;
        }
        case ECGV_OVERLOAD:
        {
            pECGResult->ECGV_OVERLOAD_Val= curValue;
            break;
        }
        default:
            printf("\r\nECG_DetectDataSave error = %d",g_ecg_target);
            return RETURN_ERROR;
    }
    
    return RETURN_SUCCESS;
}

void ECG_Init(void)
{
    g_u8ecg_status = 0;
    
    ECG_GPIO_Init();
    ECG_SelectChannel(RA_OFF);
}

static char Is_ecgPorbe_narmal(u16 value)
{
    if ((value > ECG_PROBE_ON_VAL_MIN) && (value < ECG_PROBE_ON_VAL_MAX))
    {
        return 1;
    }
    return 0;
}

static char Is_ecgPorbeRL_narmal(u16 value)
{
    if ((value > ECG_PROBE_RL_VAL_MIN) && (value < ECG_PROBE_RL_VAL_MAX))
    {
        return 1;
    }
    return 0;
}

static void ECG_ProbeDetect(void)
{
    static u32 u32NextChangeTime = 0;
    
    if(IsOnTime(u32NextChangeTime))
    {
        g_u8ecg_status = 0xFF;//defalut is alarm
        if (Is_ecgPorbe_narmal(pECGResult->RA_OFF_Val))
        {
            RESETBIT(g_u8ecg_status, RA_OFF);
        }
        else
        {
            ECG_DBG_INFO("RA_OFF");
        }
        
        if (Is_ecgPorbe_narmal(pECGResult->LA_OFF_Val))
        {
            RESETBIT(g_u8ecg_status, LA_OFF);
        }
        else
        {
            ECG_DBG_INFO("LA_OFF");
        }
        
        if (Is_ecgPorbe_narmal(pECGResult->LL_OFF_Val))
        {
            RESETBIT(g_u8ecg_status, LL_OFF);
        }
        else
        {
            ECG_DBG_INFO("LL_OFF");
        }
        
        if (Is_ecgPorbe_narmal(pECGResult->V1_OFF_Val))
        {
            RESETBIT(g_u8ecg_status, V1_OFF);
        }
        else
        {
            ECG_DBG_INFO("V1_OFF");
        }
        
        if (Is_ecgPorbeRL_narmal(pECGResult->RL_OFF_Val))
        {
            RESETBIT(g_u8ecg_status, RL_OFF);
        }
        else
        {
            ECG_DBG_INFO("RL_OFF");
        }
        
        //pECGResult->ECG1_OVERLOAD_Val;
        //pECGResult->ECG2_OVERLOAD_Val;
        //pECGResult->ECGV_OVERLOAD_Val;
        //SETBIT(g_u8ecg_status, RA_OFF);
        u32NextChangeTime += getNextTimeByMS(ECG_PROBE_PERIOD);
    }
}

void ECG_DetectHandler(void)
{
    static u32 u32NextChangeTime = 0;
    
    if(IsOnTime(u32NextChangeTime))
    {
        ECG_DetectDataSave();
        ECG_SelectNextChannel();
        u32NextChangeTime += getNextTimeByMS(ECG_DETECT_TIME);
    }
    ECG_ProbeDetect();
}

void ECG_PrintResult(void)
{
#ifdef _PRINT_ECG_RESULT_
    static u32 u32NextChangeTime = 0;
    
    if(IsOnTime(u32NextChangeTime))
    {
#ifdef _ECG_PRINT_V_
        printf("\r\nRA=%f V,RL=%f V,LA=%f V,LL=%f V,V1=%f V,ECG1=%f V,ECG2=%f V,EVGV=%f V",
            (pECGResult->RA_OFF_Val * 3.3)/0xFFF,
            (pECGResult->RL_OFF_Val * 3.3)/0xFFF,
            (pECGResult->LA_OFF_Val * 3.3)/0xFFF,
            (pECGResult->LL_OFF_Val * 3.3)/0xFFF,
            (pECGResult->V1_OFF_Val * 3.3)/0xFFF,
            (pECGResult->ECG1_OVERLOAD_Val* 3.3)/0xFFF,
            (pECGResult->ECG2_OVERLOAD_Val* 3.3)/0xFFF,
            (pECGResult->ECGV_OVERLOAD_Val* 3.3)/0xFFF);
#else
        printf("\r\nRA=%04d,RL=%04d,LA=%04d,LL=%04d,V1=%04d,ECG1=%04d,ECG2=%04d,EVGV=%04d",
            pECGResult->RA_OFF_Val,
            pECGResult->RL_OFF_Val,
            pECGResult->LA_OFF_Val,
            pECGResult->LL_OFF_Val,
            pECGResult->V1_OFF_Val,
            pECGResult->ECG1_OVERLOAD_Val,
            pECGResult->ECG2_OVERLOAD_Val,
            pECGResult->ECGV_OVERLOAD_Val);
#endif //_ECG_PRINT_V_
        u32NextChangeTime += getNextTimeByMS(1000);
    }
#endif //_PRINT_ECG_RESULT_
}


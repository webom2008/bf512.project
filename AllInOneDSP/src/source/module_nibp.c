/*
 * module_nibp.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 *
 * NOTE: about FAST & SLOW
 *  FAST = 0    | SLOW = 1  |   快速泄气
 *  FAST = 1    | SLOW = 0  |   慢速泄气
 *  FAST = 1    | SLOW = 1  |   气压保持
 */
#include <ccblkfn.h>

#include "module_nibp.h"

#define UPLOAD_NIBP_TAG
#define _NIBP_USED_RISING_STEP_

//#define _MODULE_NIBP_INFO_
#ifdef _MODULE_NIBP_INFO_
#define NIBP_DBG_INFO(fmt, arg...) AIO_printf("\r\n[NIBP] "fmt, ##arg)
#else
#define NIBP_DBG_INFO(fmt, arg...)
#endif

#define NIBP_PWM_FREQ       1000        //PWM输出频率(Hz)
#define TIMER6_MASK         0x0040

#define NIBP_SAMPLE_TIME        10      //10ms=100Hz
#define SLOW_HOLD_TIME_MS       2000    //自动阶梯放气，每个持续时间ms
#define NIBP_NEWBORM_LOWEST_PRESS   3      //mmHg，阶梯放气最低血压临界值，低于此值则进行快速泄压
#define NIBP_ADULT_LOWEST_PRESS     5
#define NIBP_RISING_HOLE_MS     3000    //rising step hold time.
#define NIBP_FALLING_HOLE_PERIOD_CNT    (2.4)
#define NIBP_FALLING_STEP_FILTER_MS     (360) //NOTE:NIBP_DATA_FILTER_SIZE*10ms
#define NIBP_MAX_DUTY_CYCLE             (1000)
#define NIBP_DEFAULT_DUTY_CYCLE         (800)
#define NIBP_DEFAULT_LOW_DUTY_CYCLE     (330)
#define NIBP_NEWBORN_DUTY_CYCLE         (300)
#define NIBP_NEWBORN_LOW_DUTY_CYCLE     (250)
#define NIBP_RISING_STEP_MAX            (50)
#define RISING_FIRST_FIXED_STEP         (10)

#define ADULT_PRESSURE_RANGE_UP     (280)
#define ADULT_PRESSURE_RANGE_DOWN   (80)
#define ADULT_PRESSURE_DEFAULT      (160)
#define ADULT_PRESSURE_MAX          (297)

#define CHILD_PRESSURE_RANGE_UP     (210)
#define CHILD_PRESSURE_RANGE_DOWN   (80)
#define CHILD_PRESSURE_DEFAULT      (140)
#define CHILD_PRESSURE_MAX          (240)

#define NEWBORN_PRESSURE_RANGE_UP   (140)
#define NEWBORN_PRESSURE_RANGE_DOWN (60)
#define NEWBORN_PRESSURE_DEFAULT    (90)
#define NEWBORN_PRESSURE_MAX        (147)


#define NIBP_FAST_PIN       PF3
#define NIBP_SLOW_PIN       PF4
#define NIBP_FAST_LOW()      (*pPORTFIO_CLEAR = NIBP_FAST_PIN)
#define NIBP_FAST_HIGH()     (*pPORTFIO_SET = NIBP_FAST_PIN)
#define NIBP_SLOW_LOW()      (*pPORTFIO_CLEAR = NIBP_SLOW_PIN)
#define NIBP_SLOW_HIGH()     (*pPORTFIO_SET = NIBP_SLOW_PIN)

typedef enum
{ 
    CUFF_GAS_LEAKAGE     = 0,//NIBP袖带充气管漏气
    CUFF_OFF             = 1,
    PUMP_LEAKAGE         = 2,
    CUFF_TYPE_ERROR      = 3,
    AIR_PRESSURE_ERROR   = 4,
    SIGNAL_TOO_WEAK      = 5,
    SIGNAL_SATURATION    = 6,
    OVER_MEASURE_PRESSURE= 7,
    ARM_EXERCISE         = 8,
    OVER_PROTECT_PRESSURE= 9,
    MODULE_FAILED        = 10,
    MEASURE_TIMEOUT      = 11,
    MEASURE_FAILED       = 12,
    RESET_ERROR          = 13,
} NIBP_ALARM_TypeDef;

#pragma section("sdram0_bank3")
pump_device g_pump_dev;
#pragma section("sdram0_bank3")
pump_device *g_pPUMP_dev = &g_pump_dev;
#pragma section("sdram0_bank3")
static NIBP_SimpleResult g_lastNIBPResult;

//unsigned long g_NIBP_ADC_Val;
//#pragma section("sdram0_bank3")
nibp_device g_nibp_dev;
//#pragma section("sdram0_bank3")
nibp_device *g_pNIBP_dev = &g_nibp_dev;
bool g_IsNIBP_Upload;

static u16 g_staicPrepress, g_dynamicPrepress;         //预加压值
static bool g_isRisingTwice;
u16 g_slowRele_mmHg;            //阶梯泄气的目标值
unsigned long g_Tim6Count;                  //TIM6,PWM的相关参数
EnableOrDisable g_NIBP_Status;              //NIBP模块运行状态
EnableOrDisable g_NIBP_PWM_Status;          //PWM的使能状态
EnableOrDisable g_SlowReleTimeStauts;       //阶梯泄气保存定时器的运行状态
unsigned int g_ForceReleTimeIndex;          //TimeoutCounter的索引量，用于保护时间的设置
unsigned int g_SlowReleHoldTimeIndex;       //TimeoutCounter的索引量，用于阶梯泄压保存时间的设置
NIBPWorkMode_TypeDef g_curWorkMode;             //当前工作模式:自动，手动
unsigned long g_AutoTestCycle;              //自动连续测试的周期
u8 g_curCycler;
u32 g_u32nibpAutoNextTime;
bool g_bIsCalculateDrop;
u8 g_u8Venipuncture;
static bool g_bIsPumpStop;
static unsigned long g_measure_max_ms;  //max time for every nibp measure period
static u16 g_overload_mmHg;
static char g_overload_count;
static u32 g_continue_test_tick, g_continue_next_tick;
static NIBPWorkMode_TypeDef g_backup_mode;
static char is_gas_leak_test;
static u32 g_gas_leak_tick;
static u32 g_oversleeve_tick;               //袖套脱落检测
u16 g_Rising_mmHgCur;
static bool g_isRisingStateRele;

const u32 SLOW_RELE_ADC_TAG = 0x00FFFFFF;   //阶梯泄压过程中，上传ADC的数值
const u8 TEST_SLOW_RELE_COUNT = 5;          //阶梯泄气，阈值判断次数
const u8 SLOW_RELE_MASK_COUNT = 15;         //阶梯泄压关闸后(Hold Action)，屏蔽的个数
u8 u8SlowReleRunCount;                      //阶梯泄压瞬间，上传自定义数据
u8 g_u8UploadMaskMaxCount;                  //阶梯泄压瞬间，最大上传TAG数据个数
u8 g_u8SlowReleStepmmHg;                    //阶梯泄压阶梯mmHg值
u16 g_u8SlowReleHoldTimeMS;                 //阶梯泄压阶梯保持时间，单位ms
u8 g_u8SlowReleMin_mmHg;                    //阶梯泄压最小mmHg值

#define AMPLIFY_FACTOR  (10000.0)   //mmHg的值放大倍数
#pragma section("sdram0_bank3")
float g_NIBP_K[NIBP_VERIFY_NUM-1];  //mmHg与ADC线性转换关系的斜率
#pragma section("sdram0_bank3")
float g_NIBP_B[NIBP_VERIFY_NUM-1];  //mmHg与ADC线性转换关系的截距
#pragma section("sdram0_bank3")
static u16 g_risingStep[NIBP_RISING_STEP_MAX];
static u8 g_u8RisingStepCount;
static char isFisrtRisingStep;
static u32 g_nibp_one_time_tick;
unsigned char u8_pressue_over_range_cnt;

//<!-- NIBP algorithm define Begin -->
#define NIBP_Sampling_Buffer_Len    (1000 / NIBP_SAMPLE_TIME + 1)
#pragma section("sdram0_bank3")
unsigned long g_nibp_result_mmHg[NIBP_Sampling_Buffer_Len];
#pragma section("sdram0_bank3")
unsigned long g_nibp_result_adc[NIBP_Sampling_Buffer_Len];
#pragma section("sdram0_bank3")
unsigned long g_nibp_tick[NIBP_Sampling_Buffer_Len];
u16  g_nibp_receive_offset,g_nibp_algorithm_offset, g_nibp_raw_offset;
#pragma section("sdram0_bank3")
static u16 g_nibp_display_mmHg[NIBP_Sampling_Buffer_Len];
static u16  g_mmHg_recv_offset, g_mmHg_send_offset;
//<!-- NIBP algorithm define End -->

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void NIBP_Init(void);
#pragma section("sdram0_bank3")
void NIBP_FAST_IO_Init(void);
#pragma section("sdram0_bank3")
void NIBP_SLOW_IO_Init(void);
#pragma section("sdram0_bank3")
void InitTimer6(void);
#pragma section("sdram0_bank3")
void InitTimer6_gpio(void);
#pragma section("sdram0_bank3")
void InitTimer6_config(void);
#pragma section("sdram0_bank3")
void NIBP_ReslultUpload(void);
#pragma section("sdram0_bank3")
void NIBP_VerifyStatusUpload(void);
#pragma section("sdram0_bank3")
void NIBP_StartStopRespond(const char state);
#pragma section("sdram0_bank3")
void NIBP_SetDefaultMaxPressure(void);
#pragma section("sdram0_bank3")
void NIBP_SetProtectTime(void);
#pragma section("sdram0_bank3")
void NIBP_StepFormula_Init(void);
#pragma section("sdram0_bank3")
void NIBP_SetCycle(u8 cycler);
#pragma section("sdram0_bank3")
void NIBP_SampleOneTime(void);
#pragma section("sdram0_bank3")
void NIBP_ForceStop(void);
#pragma section("sdram0_bank3")
void NIBP_ADC_Average(int sampleCount, unsigned long *pADCAverage);
#pragma section("sdram0_bank3")
void NIBP_VerifyEnter(void);
#pragma section("sdram0_bank3")
void NIBP_VerifyExit(void);
#pragma section("sdram0_bank3")
void NIBP_DebugVerifyRespond(void);
#pragma section("sdram0_bank3")
void NIBP_VerifyBySimulator(const u8 mode);
#pragma section("sdram0_bank3")
void NIBP_VerifyAction(const u8 mmHg);
#pragma section("sdram0_bank3")
void NIBP_STM32VerifyAction(u8 *pBuf, u8 nLen);
#pragma section("sdram0_bank3")
void NIBP_DebugSetmmHgRespond(const u8 nIndex, const u16 mmHg, const u32 adcVal);
#pragma section("sdram0_bank3")
void NIBP_DebugSaveRespond(void);
#pragma section("sdram0_bank3")
RETURN_TypeDef NIBP_DebugInterface(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
RETURN_TypeDef NIBP_VerifyDebug(const u8 *pBuf, const u8 nLen);
#pragma section("sdram0_bank3")
RETURN_TypeDef NIBP_VerifySave2Eepom(void);
#pragma section("sdram0_bank3")
RETURN_TypeDef NIBP_SetUploadMaskCount(u8 count);
#pragma section("sdram0_bank3")
RETURN_TypeDef nibp_debug_start(const u8 *pBuf, const u8 nLen);
#pragma section("sdram0_bank3")
RETURN_TypeDef nibp_debug_start_pwm(void);
#pragma section("sdram0_bank3")
RETURN_TypeDef nibp_debug_stop_pwm(void);
#pragma section("sdram0_bank3")
RETURN_TypeDef nibp_debug_pass_step(const u8 *pBuf);
#pragma section("sdram0_bank3")
void NIBP_resetModule(void);
#pragma section("sdram0_bank3")
void NIBP_getResult(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
void NIBP_setVenipuncture(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
void NIBP_setContinueMode(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
void NIBP_setGasLeakTest(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
RETURN_TypeDef nibp_debug_algo_sw(const u8 *pBuf);
#pragma section("sdram0_bank3")
RETURN_TypeDef nibp_debug_setStepCtl(const u8 *pBuf, const u8 nLen);
#pragma section("sdram0_bank3")
RETURN_TypeDef nibp_debug_StepRsingResult(const u8 *pBuf);
#pragma section("sdram0_bank3")
RETURN_TypeDef nibp_staticPressureTest(const u8 *pBuf);
#pragma section("sdram0_bank3")
bool NIBP_isRightCuffType(const u16 slope);
#pragma section("sdram0_bank3")
bool NIBP_isStm32FastRele(void);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

u32 NIBP_mmHg2ADC(const u16 mmHg);
u32 NIBP_ADC2mmHg(u32 *pADCVal);
void NIBP_StartSample(void);
void NIBP_SampleOneTimeByDebug(void);
void NIBP_SampleAutoTest(void);
void NIBP_ForceFastRelease(void);
void NIBP_AlarmUpload(const NIBP_ALARM_TypeDef type);
void NIBP_refreshSlowReleHoldTime(bool isRising,const int bpm_ms);

static void NIBP_FAST_IO_Init(void)
{
    /*setup PF3 as an output*/
	*pPORTFIO_INEN &= ~NIBP_FAST_PIN;    /* input buffer disable */
	*pPORTFIO_DIR |= NIBP_FAST_PIN;      /* output */

	/* clear interrupt settings */
	*pPORTFIO_EDGE &= ~NIBP_FAST_PIN;         // Level Sensitivity
    *pPORTFIO_MASKA_CLEAR = NIBP_FAST_PIN;

    NIBP_FAST_HIGH();
}

static void NIBP_SLOW_IO_Init(void)
{
    /*setup PF4 as an output*/
	*pPORTFIO_INEN &= ~NIBP_SLOW_PIN;    /* input buffer disable */
	*pPORTFIO_DIR |= NIBP_SLOW_PIN;      /* output */

	/* clear interrupt settings */
	*pPORTFIO_EDGE &= ~NIBP_SLOW_PIN;         // Level Sensitivity
    *pPORTFIO_MASKA_CLEAR = NIBP_SLOW_PIN;

    NIBP_SLOW_HIGH();
}

static void NIBP_SetPressRelease(NIBP_Release_TypeDef mode)
{
    switch (mode)
    {
        case FAST_RELE:
            NIBP_SLOW_LOW();//NIBP_SLOW_HIGH(),解决平时发烫
            NIBP_FAST_LOW();
            break;
        case SLOW_RELE:
            NIBP_SLOW_LOW();
            NIBP_FAST_HIGH();
            break;
        case PRESS_HOLD:
            NIBP_SLOW_HIGH();
            NIBP_FAST_HIGH();
            break;
        default:
            break;
    }
}

/******************************************************************************/
/******************************** Timer6 Functions Start***********************/
static void InitTimer6_gpio(void)
{
    *pPORTG_FER |= PG8;         //enable PG8 peripheral function.
    *pPORTG_MUX &= ~(3<<8);     //set PG8 as TMR6 interface.mask bit[9:8]
    *pPORTG_MUX |= (2<<8);      //set bit[9:8]=b#10
}

/**
  * @brief  This function initializes the TIM6 for NIBP module PWM.
  *     default pwm width = 50%
  * @param  void
  * @retval void
  */
static void InitTimer6_config(void)
{
    unsigned long sclk = getSystemCLK();
    
/*****************************************************************************
 *
 *   pwm_out mode, interrupt disable, count to end of period
 *   SCLK for counter
 * 
 ****************************************************************************/
    *pTIMER6_CONFIG= 0x000D;

    sclk += (NIBP_PWM_FREQ /2);     //round up before divide by NIBP_PWM_FREQ
    g_Tim6Count = sclk / NIBP_PWM_FREQ;
    *pTIMER6_PERIOD = g_Tim6Count;         //timer0 = ratio * (1/132)us
    *pTIMER6_WIDTH = (g_Tim6Count / 2);    //timer width, 50%
}

static void NIBP_SetPWM(EnableOrDisable status)
{
    if (ENABLE == status)
    {
        g_oversleeve_tick = SysTick_Get() + getTickNumberByMS(10000);
        *pTIMER_ENABLE = TIMER6_MASK;
    }
    else
    {
        g_oversleeve_tick = 0;
        *pTIMER_DISABLE = TIMER6_MASK;
    }
    
    g_NIBP_PWM_Status = status;
}

void NIBP_SetPWM_DutyCycles(u16 percent)
{
    if (percent > NIBP_MAX_DUTY_CYCLE)
    {
        percent = NIBP_MAX_DUTY_CYCLE;
    }
    //unsigned long ulStaus = *pTIMER_ENABLE;
    //NIBP_SetPWM(DISABLE);
    *pTIMER6_WIDTH = (g_Tim6Count * percent) / NIBP_MAX_DUTY_CYCLE;
    //if (ulStaus & TIMER6_MASK)
    //{
    //    NIBP_SetPWM(ENABLE);
    //}
}

void NIBP_SetPrepressure(u16 u16mmHg)
{
    if (ADULT == g_PatientType)
    {
        if (u16mmHg > ADULT_PRESSURE_RANGE_UP) 
            u16mmHg = ADULT_PRESSURE_RANGE_UP;
        if (u16mmHg < ADULT_PRESSURE_RANGE_DOWN) 
            u16mmHg = ADULT_PRESSURE_RANGE_DOWN;
    }
    else if (CHILD == g_PatientType)
    {
        if (u16mmHg > CHILD_PRESSURE_RANGE_UP) 
            u16mmHg = CHILD_PRESSURE_RANGE_UP;
        if (u16mmHg < CHILD_PRESSURE_RANGE_DOWN) 
            u16mmHg = CHILD_PRESSURE_RANGE_DOWN;
    }
    else if (NEWBORN == g_PatientType)
    {
        if (u16mmHg > NEWBORN_PRESSURE_RANGE_UP) 
            u16mmHg = NEWBORN_PRESSURE_RANGE_UP;
        if (u16mmHg < NEWBORN_PRESSURE_RANGE_DOWN) 
            u16mmHg = NEWBORN_PRESSURE_RANGE_DOWN;
    }
        
    g_staicPrepress = u16mmHg;
}

static void NIBP_SetDefaultMaxPressure(void)
{
    switch(g_PatientType)
    {
    case ADULT:
        NIBP_SetPrepressure(ADULT_PRESSURE_DEFAULT);
        break;
    case CHILD:
        NIBP_SetPrepressure(CHILD_PRESSURE_DEFAULT);
        break;
    default://NEWBORN
        NIBP_SetPrepressure(NEWBORN_PRESSURE_DEFAULT);
        break;
    }
}

static void NIBP_SetProtectTime(void)
{
    switch(g_PatientType)
    {
    case NEWBORN:
        g_measure_max_ms = 90000;
        break;
    default:
        g_measure_max_ms = 180000;
        break;
    }
}

static void InitTimer6(void)
{
    InitTimer6_gpio();
    InitTimer6_config();
}
/******************************** Timer0 Functions Stop ***********************/
/******************************************************************************/
static bool IsNIBPOverLoad(const u16 mmHg)
{
    u32 u32ADCVale = NIBP_mmHg2ADC(mmHg);
    if (g_pNIBP_dev->adc_val > u32ADCVale)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//阶梯泄压控制
//主要实现:
//1.保持3ms,待采集脉率;
//2.3ms已到,进行Slow模式泄压;
//3.若压力已降到最低值，则快速泄气
static void NIBP_AutoPressReleHanlder(void)
{
    static int slowReleCount = 0;
    int i;
    
    if (DISABLE == g_SlowReleTimeStauts)
    {
        //设置保持时间
        NIBP_refreshSlowReleHoldTime(false, p_nibpResult->pulse_ms);
        ResetTimeout(g_SlowReleHoldTimeIndex, getTickNumberByMS(g_u8SlowReleHoldTimeMS));
        g_SlowReleTimeStauts = ENABLE;
        if (g_slowRele_mmHg < 25)
        {
            g_slowRele_mmHg -= 3;
        }
        else if (NEWBORN == g_PatientType && g_slowRele_mmHg < 50 && g_slowRele_mmHg >= 25)
        {
            g_slowRele_mmHg -= 5;
        }
        else
        {
            g_slowRele_mmHg -= g_u8SlowReleStepmmHg;
#ifdef _NIBP_USED_RISING_STEP_
            for (i=0; i < g_u8RisingStepCount; i++)
            {
                if ((g_risingStep[i] > g_slowRele_mmHg-5)\
                    && (g_risingStep[i] < g_slowRele_mmHg+5))
                {
                    g_slowRele_mmHg -= g_u8SlowReleStepmmHg;
                }
            }
#endif
        }
        return;
    }
    
    if (false == IsTimedout(g_SlowReleHoldTimeIndex))//时间未到，继续气压保持
        return;
    
    //阶梯保持时间已到
    //if ((true == g_bIsCalculateDrop) && (u8SlowReleRunCount > g_u8UploadMaskMaxCount-1))//纠正停止加压后的下降突变
    if (true == g_bIsCalculateDrop)
    {
        g_bIsCalculateDrop = false;
        g_slowRele_mmHg = (g_pNIBP_dev->mmHg_val)/AMPLIFY_FACTOR \
                            - g_u8SlowReleStepmmHg;
    }
    
    //1.启动阶梯泄压
    if (g_slowRele_mmHg >= g_u8SlowReleMin_mmHg)
    {
        u8SlowReleRunCount = 0;
        NIBP_SetPressRelease(SLOW_RELE);
    }
    //2.判断是否测至最低压，完成本次测试
    else if (g_slowRele_mmHg < g_u8SlowReleMin_mmHg)
    {
        g_NIBP_Status = DISABLE;
        g_SlowReleTimeStauts = DISABLE;
        NIBP_SetPressRelease(FAST_RELE);
        NIBP_DBG_INFO(">>>>>>>>>>>>>>>>Fast Rele 1");
        NIBP_AlarmUpload(MEASURE_FAILED);
    }
    
    //3.一个阶梯泄压已完成，则进行保持
    if (false == IsNIBPOverLoad(g_slowRele_mmHg))//实际压力比设定还要低
    {
        slowReleCount++;
    }
    if (slowReleCount > TEST_SLOW_RELE_COUNT)//累计采集N次均比设定值低，则认为该进入阶梯保持状态
    {
        slowReleCount = 0;
        NIBP_SetPressRelease(PRESS_HOLD);
        g_SlowReleTimeStauts = DISABLE; //初始化TimeOut保持时间
    }
}

void NIBP_SetCycle(u8 cycler)
{
    switch (cycler)
    {
        case 0x00:
            g_curWorkMode = MANUAL_TEST;
            break;
            
        case 0x01:
            g_AutoTestCycle = getTickNumberByMS(1 * 60 * 1000);
            g_curWorkMode = AUTO_TEST;
            break;
            
        case 0x02:
            g_AutoTestCycle = getTickNumberByMS(2 * 60 * 1000);
            g_curWorkMode = AUTO_TEST;
            break;
            
        case 0x03:
            g_AutoTestCycle = getTickNumberByMS(3 * 60 * 1000);
            g_curWorkMode = AUTO_TEST;
            break;
            
        case 0x04:
            g_AutoTestCycle = getTickNumberByMS(4 * 60 * 1000);
            g_curWorkMode = AUTO_TEST;
            break;
            
        case 0x0A:
            g_AutoTestCycle = getTickNumberByMS(10 * 60 * 1000);
            g_curWorkMode = AUTO_TEST;
            break;
            
        case 0x0F:
            g_AutoTestCycle = getTickNumberByMS(15 * 60 * 1000);
            g_curWorkMode = AUTO_TEST;
            break;
            
        case 0x1E:
            g_AutoTestCycle = getTickNumberByMS(30 * 60 * 1000);
            g_curWorkMode = AUTO_TEST;
            break;
            
        case 0x3C:
            g_AutoTestCycle = getTickNumberByMS(60 * 60 * 1000);
            g_curWorkMode = AUTO_TEST;
            break;
            
        case 0x5A:
            g_AutoTestCycle = getTickNumberByMS(90 * 60 * 1000);
            g_curWorkMode = AUTO_TEST;
            break;
            
        case 0xB4:
            g_AutoTestCycle = getTickNumberByMS(180 * 60 * 1000);
            g_curWorkMode = AUTO_TEST;
            break;
            
        case 0xF0:
            g_AutoTestCycle = getTickNumberByMS(240 * 60 * 1000);
            g_curWorkMode = AUTO_TEST;
            break;
            
        case 0xFF:
            g_AutoTestCycle = getTickNumberByMS(480 * 60 * 1000);
            g_curWorkMode = AUTO_TEST;
            break;
            
        defalut:
            cycler = 0;
            g_curWorkMode = MANUAL_TEST;
            break; 
    }
    g_curCycler = cycler;
    g_NIBP_Status = DISABLE;
    g_SlowReleTimeStauts = DISABLE;
    NIBP_SetPressRelease(FAST_RELE);
    NIBP_DBG_INFO(">>>>>>>>>>>>>>>>Fast Rele 2");
    if (0 != cycler)
    {
        g_u32nibpAutoNextTime = SysTick_Get() - 1000;
        NIBP_SampleAutoTest();
    }
}

//计算升压过程，各个阶段的转换公式
static void NIBP_StepFormula_Init(void)
{
    u16 y1,y2;//临时存放mmHg值
    u32 x1,x2;//临时存放ADC值
    int i;

    if (TRUE == pEeprom_struct->nibp_IsVerify)//采用校准数据，计算公式
    {
        /***********************************************************************
         *公式为: n(mmHg)*AMPLIFY_FACTOR = k *adc_value + b
         *第一阶梯升压段计算:0mmHg~10mmHg
         *第二阶梯升压段计算:10mmHg~50mmHg
         *第三阶梯升压段计算:50mmHg~100mmHg
         *第四阶梯升压段计算:100mmHg~150mmHg
         *第五阶梯升压段计算:150mmHg~290mmHg
        ***********************************************************************/
        for (i = 0; i < (NIBP_VERIFY_NUM - 1); i++)
        {
            x1 = pEeprom_struct->nibp_Verify_Val[i];
            x2 = pEeprom_struct->nibp_Verify_Val[i+1];
            y1 = pEeprom_struct->nibp_Verify_mmHg[i];
            y2 = pEeprom_struct->nibp_Verify_mmHg[i+1];
            
            g_NIBP_K[i] = (y2 - y1) * AMPLIFY_FACTOR / (x2 - x1);
            //g_NIBP_B[i] = (x2 * y1 - x1 * y2) * AMPLIFY_FACTOR / (x2 - x1);
            g_NIBP_B[i] = y2 * AMPLIFY_FACTOR - g_NIBP_K[i] * x2;
        }
    }
    else//采用默认公式
    {
        x1 = 1428142;
        x2 = 2139413;
        y1 = 101;
        y2 = 180;
        for (i = 0; i < (NIBP_VERIFY_NUM - 1); i++)
        {
            g_NIBP_K[i] = (y2 - y1) * AMPLIFY_FACTOR / (x2 - x1);
            //g_NIBP_B[i] = ((x2 * y1 - x1 * y2) / (x2 - x1))* AMPLIFY_FACTOR;
            g_NIBP_B[i] = y2 * AMPLIFY_FACTOR - g_NIBP_K[i] * x2;
        }
    }
}

//ADC值转换成mmHg值(放大AMPLIFY_FACTOR倍)的转换
static u32 NIBP_ADC2mmHg(u32 *pADCVal)
{
    u32 adc = *pADCVal;

    if (adc < (pEeprom_struct->nibp_Verify_Val[1]))//判断当前ADC所在的区间<10mmHg
    {
        adc = g_NIBP_K[0] * adc + g_NIBP_B[0];
    }
    else if ((adc < (pEeprom_struct->nibp_Verify_Val[2])) \
            && (adc >= (pEeprom_struct->nibp_Verify_Val[1])))//10mmHg <= adc < 50mmHg
    {
        adc = g_NIBP_K[1] * adc + g_NIBP_B[1];
    }
    else if ((adc < (pEeprom_struct->nibp_Verify_Val[3])) \
            && (adc >= (pEeprom_struct->nibp_Verify_Val[2])))//50mmHg <= adc < 100mmHg
    {
        adc = g_NIBP_K[2] * adc + g_NIBP_B[2];
    }
    else if ((adc < (pEeprom_struct->nibp_Verify_Val[4])) \
            && (adc >= (pEeprom_struct->nibp_Verify_Val[3])))//100mmHg <= adc < 150mmHg
    {
        adc = g_NIBP_K[3] * adc + g_NIBP_B[3];
    }
    else if (adc >= (pEeprom_struct->nibp_Verify_Val[4]))//150mmHg <= adc
    {
        adc = g_NIBP_K[4] * adc + g_NIBP_B[4];
    }
    return adc;
}

static u32 NIBP_mmHg2ADC(const u16 mmHg)
{
    u32 u32ADCVale;

    //mmHg * AMPLIFY_FACTOR = k * adc + b ==> adc = (mmHg * AMPLIFY_FACTOR - b)/k

    if (pEeprom_struct->nibp_IsVerify == TRUE)
    {
        if (mmHg < (pEeprom_struct->nibp_Verify_mmHg[1]))//mmHg < 10mmHg
        {
            u32ADCVale = (mmHg * AMPLIFY_FACTOR - g_NIBP_B[0]) / g_NIBP_K[0];//第1阶段
        }
        else if ((mmHg < (pEeprom_struct->nibp_Verify_mmHg[2])) \
                && (mmHg >= (pEeprom_struct->nibp_Verify_mmHg[1])))//10mmHg <= mmHg < 50mmHg
        {
            u32ADCVale = (mmHg * AMPLIFY_FACTOR - g_NIBP_B[1]) / g_NIBP_K[1];//第2阶段
        }
        else if ((mmHg < (pEeprom_struct->nibp_Verify_mmHg[3])) \
                && (mmHg >= (pEeprom_struct->nibp_Verify_mmHg[2])))//50mmHg <= mmHg < 100mmHg
        {
            u32ADCVale = (mmHg * AMPLIFY_FACTOR - g_NIBP_B[2]) / g_NIBP_K[2];//第3阶段
        }
        else if ((mmHg < (pEeprom_struct->nibp_Verify_mmHg[4])) \
                && (mmHg >= (pEeprom_struct->nibp_Verify_mmHg[3])))//100mmHg <= mmHg < 150mmHg
        {
            u32ADCVale = (mmHg * AMPLIFY_FACTOR - g_NIBP_B[3]) / g_NIBP_K[3];//第4阶段
        }
        else if (mmHg >= (pEeprom_struct->nibp_Verify_mmHg[4]))//150mmHg <= mmHg
        {
            u32ADCVale = (mmHg * AMPLIFY_FACTOR - g_NIBP_B[4]) / g_NIBP_K[4];//第5阶段
        }
    }
    else
    {
        u32ADCVale = (mmHg * AMPLIFY_FACTOR - g_NIBP_B[0]) / g_NIBP_K[0];//第1~5阶段均为默认
    }
    
    return u32ADCVale;
}

//<!-- NIBP ADC Result API Begin -->
void NIBP_adc_store_data(void)
{
    if ((VENIPUNCTURE_TEST != g_curWorkMode) && (DISABLE == g_NIBP_Status))
        return;//血压模块不测量，则不上传数据

    g_pNIBP_dev->mmHg_val = NIBP_ADC2mmHg(&(g_pNIBP_dev->adc_val));
    g_pNIBP_dev->tick = SysTick_Get();

    //handle display mmHg value
    g_nibp_display_mmHg[g_mmHg_recv_offset] = (g_pNIBP_dev->mmHg_val + AMPLIFY_FACTOR/2)/AMPLIFY_FACTOR;
    if(++g_mmHg_recv_offset >= NIBP_Sampling_Buffer_Len)  g_mmHg_recv_offset -= NIBP_Sampling_Buffer_Len;
    if(g_mmHg_send_offset == g_mmHg_recv_offset)    // overflow
    {
        if(++g_mmHg_send_offset >= NIBP_Sampling_Buffer_Len)    g_mmHg_send_offset -= NIBP_Sampling_Buffer_Len;
    }

#ifdef UPLOAD_NIBP_TAG
    if (VENIPUNCTURE_TEST != g_curWorkMode)
    {
        if (u8SlowReleRunCount < g_u8UploadMaskMaxCount)//正在泄压阶段，数据位TAG
        {
            u8SlowReleRunCount++;
            g_pNIBP_dev->mmHg_val = SLOW_RELE_ADC_TAG;
        }
        else
        {
            u8SlowReleRunCount = g_u8UploadMaskMaxCount;
        }
    }
#endif
    
    g_nibp_result_mmHg[g_nibp_receive_offset] = g_pNIBP_dev->mmHg_val;
    g_nibp_result_adc[g_nibp_receive_offset] = g_pNIBP_dev->adc_val;
    g_nibp_tick[g_nibp_receive_offset] = g_pNIBP_dev->tick;
    
    g_nibp_receive_offset += 1;
    if(g_nibp_receive_offset >= NIBP_Sampling_Buffer_Len)  g_nibp_receive_offset -= NIBP_Sampling_Buffer_Len;
    if(g_nibp_algorithm_offset == g_nibp_receive_offset)    // overflow
    {
        g_nibp_algorithm_offset += 1;
        if(g_nibp_algorithm_offset >= NIBP_Sampling_Buffer_Len)    g_nibp_algorithm_offset -= NIBP_Sampling_Buffer_Len;
    }
    if(g_nibp_raw_offset == g_nibp_receive_offset)    // overflow
    {
        g_nibp_raw_offset += 1;
        if(g_nibp_raw_offset >= NIBP_Sampling_Buffer_Len)    g_nibp_raw_offset -= NIBP_Sampling_Buffer_Len;
    }
}

static u16 NIBP_GetDataLen(void)
{
    s32 s32Len;

    s32Len = g_nibp_receive_offset - g_nibp_raw_offset;
    if(s32Len < 0)  s32Len += NIBP_Sampling_Buffer_Len;

    return (u16)s32Len;
}

static u16 NIBP_PopResult(u32* pu32Tick, u32* pmmHg, u32* padc)
{
    u16 s32Len = NIBP_GetDataLen();

    if(s32Len)
    {
        *pu32Tick = g_nibp_tick[g_nibp_raw_offset];
        *pmmHg = g_nibp_result_mmHg[g_nibp_raw_offset];
        *padc = g_nibp_result_adc[g_nibp_raw_offset];

        g_nibp_raw_offset += 1;
        if(g_nibp_raw_offset >= NIBP_Sampling_Buffer_Len)  g_nibp_raw_offset -= NIBP_Sampling_Buffer_Len;
    }
    return s32Len;
}

static u16 NIBP_GetAlgoDataLen(void)
{
    s32 s32Len = g_nibp_receive_offset - g_nibp_algorithm_offset;
    if(s32Len < 0)  s32Len += NIBP_Sampling_Buffer_Len;

    return (u16)s32Len;
}

u16 NIBP_PopAlgoResult(u32* pu32Tick, u32* pu32mmHg, u32 *pu32adc)
{
    u16 s32Len = NIBP_GetAlgoDataLen();

    if(s32Len)
    {
        *pu32Tick = g_nibp_tick[g_nibp_algorithm_offset];
        *pu32mmHg = g_nibp_result_mmHg[g_nibp_algorithm_offset];
        *pu32adc = g_nibp_result_adc[g_nibp_algorithm_offset];

        g_nibp_algorithm_offset += 1;
        if(g_nibp_algorithm_offset >= NIBP_Sampling_Buffer_Len)    g_nibp_algorithm_offset -= NIBP_Sampling_Buffer_Len;
    }

    return s32Len;
}

static s16 NIBP_PopDisplay_mmHg(u16* pmmHg)
{
    s16 s16Len = g_mmHg_recv_offset - g_mmHg_send_offset;
    if(s16Len < 0)  s16Len += NIBP_Sampling_Buffer_Len;

    if(s16Len)
    {
        *pmmHg = g_nibp_display_mmHg[g_mmHg_send_offset];
        if(++g_mmHg_send_offset >= NIBP_Sampling_Buffer_Len)
            g_mmHg_send_offset -= NIBP_Sampling_Buffer_Len;
    }
    return s16Len;
}

static void NIBP_DataReset(void)
{
    g_nibp_algorithm_offset = g_nibp_receive_offset;
    g_nibp_raw_offset = g_nibp_receive_offset;
    g_mmHg_send_offset = g_mmHg_recv_offset;
}
//<!-- NIBP ADC Result API End -->

//@return : true -- stm32 fast rele on; false -- stm32 fast rele off
static bool NIBP_isStm32FastRele(void)
{
    int result = -1;
    u8 state = 0;
    result = aio_stm32_readNIBPState(&state);
    if (result > 0 && 1 == state)
    {
        return true;
    }
    return false;
}

static void NIBP_STM32_protectHandle(void)
{
    static unsigned long ulNextChangeTime = 300000;//delay 30s
    static u16 protect_newborn_adc = 0;
    static u16 protect_adult_adc = 0;
    
    if(IsOnTime(ulNextChangeTime)) // is on time or over time
    {
        ulNextChangeTime += getTickNumberByMS(1000);
        if (true == NIBP_isStm32FastRele())
        {
            NIBP_ForceStop();
            NIBP_AlarmUpload(OVER_PROTECT_PRESSURE);
        }
        if (1 != pEeprom_struct->chIsProtectVerify) return;

        //calculate newborn and adult stm adc value by verify value.
        if (0 == protect_newborn_adc)
        {
            float k = (float)((pEeprom_struct->nibp_protect_adc[1] - \
                pEeprom_struct->nibp_protect_adc[0]) * 1.0f / (pEeprom_struct->nibp_protect_mmHg[1] \
                - pEeprom_struct->nibp_protect_mmHg[0]));
            float b = (float)(pEeprom_struct->nibp_protect_adc[1] \
                - pEeprom_struct->nibp_protect_mmHg[1] * k);
            protect_newborn_adc = (u16)(k * 150 + b);
            protect_adult_adc = (u16)(k * 300 + b);
            NIBP_DBG_INFO("\r\nNIBP_STM32_protectHandle");
            NIBP_DBG_INFO("\r\n>>EEPROM mmHg[0]=%d, adc[0]=%d",
                pEeprom_struct->nibp_protect_mmHg[0],
                pEeprom_struct->nibp_protect_adc[0]);
            NIBP_DBG_INFO("\r\n>>EEPROM mmHg[1]=%d, adc[1]=%d",
                pEeprom_struct->nibp_protect_mmHg[1],
                pEeprom_struct->nibp_protect_adc[1]);
            NIBP_DBG_INFO("\r\n>>150mmHg_adc=%d\t300mmHg_adc=%d\ty=%f*x+%f",
                protect_newborn_adc,protect_adult_adc,k,b);
        }
        
        u8 index  = 0;
        u16 maxpress = 0;
        aio_stm32_readProtect(&index, &maxpress);
        if (0 == index) //stm32 use default value now, should refresh data by EEPROM
        {
            NIBP_DBG_INFO("NIBP stm32 protect = %d(ADC)", maxpress);
            index = 1;
            if (NEWBORN == g_PatientType)
            {
                maxpress = protect_newborn_adc;//150mmHg stm32-adc
            }
            else// (ADULT == g_PatientType)
            {
                maxpress = protect_adult_adc;//300mmHg stm32-adc
            }
            
            //set stm32 protect value form eeprom(verified)
            aio_stm32_writeProtect(&index, &maxpress);
        }
        else //index = 1, means have already write.
        {
            if ((NEWBORN == g_PatientType) && (maxpress != protect_newborn_adc))
            {
                maxpress = protect_newborn_adc;//150mmHg stm32-adc
                aio_stm32_writeProtect(&index, &maxpress);
            }
            else if((NEWBORN != g_PatientType) && (maxpress != protect_adult_adc))
            {
                maxpress = protect_adult_adc;//300mmHg stm32-adc
                aio_stm32_writeProtect(&index, &maxpress);
            }
        }
    }
}

void NIBP_Init(void)
{
    g_IsNIBP_Upload = true;
    g_NIBP_Status = DISABLE;
    g_NIBP_PWM_Status = DISABLE;
    g_SlowReleTimeStauts = DISABLE;
    g_pNIBP_dev->verifyStauts = DISABLE;
    g_pNIBP_dev->waitForADCStatus = DISABLE;
    g_u8UploadMaskMaxCount = SLOW_RELE_MASK_COUNT;
    u8SlowReleRunCount = g_u8UploadMaskMaxCount;
    g_u32nibpAutoNextTime = 0;
    g_bIsCalculateDrop = false;
    g_u8SlowReleStepmmHg = 10; //阶梯泄压阶梯mmHg值
    g_u8SlowReleHoldTimeMS = SLOW_HOLD_TIME_MS; //阶梯泄压阶梯保持时间，单位ms
    g_u8SlowReleMin_mmHg = NIBP_ADULT_LOWEST_PRESS;
    g_nibp_receive_offset = g_nibp_algorithm_offset = g_nibp_raw_offset = 0;
    g_mmHg_recv_offset = g_mmHg_send_offset = 0;
    memset(g_nibp_result_mmHg, 0, sizeof(g_nibp_result_mmHg));
    memset(g_nibp_result_adc, 0, sizeof(g_nibp_result_adc));
    memset(g_nibp_tick, 0, sizeof(g_nibp_tick));
    g_measure_max_ms = 180000;
    g_overload_mmHg = ADULT_PRESSURE_MAX;
    is_gas_leak_test = 0;
    g_isRisingTwice = false;
    memset(&g_lastNIBPResult, 0, sizeof(g_lastNIBPResult));

#if 1//-DEBUG140107-
    NIBP_SetCycle(0);//手动测量模式
#else
    NIBP_SetCycle(0x03);
#endif

    COM_ADC_Init();
    
    NIBP_FAST_IO_Init();
    NIBP_SLOW_IO_Init();
    NIBP_SetPressRelease(FAST_RELE);
    NIBP_DBG_INFO(">>>>>>>>>>>>>>>>Fast Rele 3");

    InitTimer6();
    NIBP_SetPWM(DISABLE);
    NIBP_SetPWM_DutyCycles(NIBP_DEFAULT_DUTY_CYCLE);

    NIBP_SetDefaultMaxPressure();

    g_ForceReleTimeIndex = SetTimeout(getTickNumberByMS(g_measure_max_ms));
    if (g_ForceReleTimeIndex > MAX_NUM_COUNTDOWN_TIMERS)
    {
        AIO_printf("\r\nNIBP NIBP_Init->SetTimeout error!");
    }
    
    g_SlowReleHoldTimeIndex = SetTimeout(getTickNumberByMS(g_u8SlowReleHoldTimeMS));
    if (g_SlowReleHoldTimeIndex > MAX_NUM_COUNTDOWN_TIMERS)
    {
        AIO_printf("\r\nNIBP NIBP_Init->SetTimeout error!");
    }

    NIBP_StepFormula_Init();
}

static void NIBP_refreshSlowReleHoldTime(bool isRising, const int bpm_ms)
{
    //设置阶梯泄压保持时间
    if (0 == bpm_ms)
    {
        g_u8SlowReleHoldTimeMS = NIBP_RISING_HOLE_MS;
        if (false == isRising)
        {
            g_dynamicPrepress = g_slowRele_mmHg + RISING_FIRST_FIXED_STEP;//下降阶段没有脉搏阶梯
        }
        NIBP_DBG_INFO(">>NIBP_refreshSlow g_dynamicPrepress=%d",g_dynamicPrepress);
        if (IsOnTime(g_nibp_one_time_tick+getTickNumberByMS(40000)))
        {
            NIBP_ForceStop();
            memset(g_risingStep, 0, sizeof(u16)*NIBP_RISING_STEP_MAX);
            NIBP_AlarmUpload(SIGNAL_TOO_WEAK);
        }
    }
    else
    {
//        if (true == isRising)
//        {
//            //在上升阶段去除头100个数据
//            g_u8SlowReleHoldTimeMS = bpm_ms * NIBP_FALLING_HOLE_PERIOD_CNT + 1000;
//        }
//        else
        {
            g_u8SlowReleHoldTimeMS = bpm_ms * NIBP_FALLING_HOLE_PERIOD_CNT \
                + NIBP_FALLING_STEP_FILTER_MS;
        }

        if (g_u8SlowReleHoldTimeMS < 2000)
        {
            g_u8SlowReleHoldTimeMS = 2000;
        }
        else if (g_u8SlowReleHoldTimeMS > 4000)
        {
            g_u8SlowReleHoldTimeMS = 4000;
        }
    }
}

static void NIBP_setParamEndofRising(u8 SlowReleStepmmHg, int bpm_ms, u16 PressureMaxmmHg)
{
    //设置阶梯泄压高度
    g_u8SlowReleStepmmHg = SlowReleStepmmHg;
    NIBP_refreshSlowReleHoldTime(false, bpm_ms);
    g_slowRele_mmHg = PressureMaxmmHg;
}

static int u16Cmp(const void *a , const void *b)
{
    //return *(u16 *)a - *(u16 *)b;   //由小到大排序
    return *(u16 *)b - *(u16 *)a;// 为由大到小排序
}

static bool NIBP_isRightCuffType(const u16 slope)
{
    //adult mode:PWM=80%,7~21mmHg/s (newborn cuff:52~866 child cuff:49~81)
    if ((ADULT == g_PatientType) && (slope > 50))
    {
        return false;
    }
    //child:PWM=80%,49~81
    if ((CHILD == g_PatientType) && ((slope < 30) || slope > 100))
    {
        return false;
    }
    //newborn:PWM=30%,84~396 (adult cuff:5~6 child cuff:20~27)
    if ((NEWBORN == g_PatientType) && (slope < 40))
    {
        return false;
    }
    return true;
}

static void NIBP_RisingCtrl(void)
{
    u16 rising_slope;
    if (NIBP_ALGO_RISING != g_nibp_algo_state) return;

    if (ENABLE == g_NIBP_PWM_Status)
    {
        if (true == IsNIBPOverLoad(g_Rising_mmHgCur))
        {
            nibp_debug_stop_pwm();
            if (isFisrtRisingStep)
            {
                isFisrtRisingStep = 0;
                ResetTimeout(g_SlowReleHoldTimeIndex, getTickNumberByMS(3000));
                //根据斜率判断袖套类型是否符合
                rising_slope = (g_Rising_mmHgCur*1000) \
                                / getMSByTickcount(SysTick_Get() - g_nibp_one_time_tick);
                NIBP_DBG_INFO("NIBP_RisingCtrl rising_slope=%d mmHg/s",rising_slope);
                if (false == NIBP_isRightCuffType(rising_slope))
                {
                    NIBP_ForceStop();
                    NIBP_AlarmUpload(CUFF_TYPE_ERROR);
                }
            }
            else
            {
                NIBP_refreshSlowReleHoldTime(true, p_nibpResult->pulse_ms);
                ResetTimeout(g_SlowReleHoldTimeIndex, getTickNumberByMS(g_u8SlowReleHoldTimeMS));
            }
        }
        else if (true == IsNIBPOverLoad(g_Rising_mmHgCur-10))
        {
            if (NEWBORN == g_PatientType)
            {
                NIBP_SetPWM_DutyCycles(NIBP_NEWBORN_LOW_DUTY_CYCLE);
            }
            else
            {
                NIBP_SetPWM_DutyCycles(NIBP_DEFAULT_LOW_DUTY_CYCLE);
            }
        }
        u8SlowReleRunCount = 0;
    }
    
    if (!g_u8RisingStepCount)//第一阶段算法不处理，解决回落问题
    {
        u8SlowReleRunCount = 0;
    }

    //rising hold step handle
    if ((DISABLE == g_NIBP_PWM_Status) \
        && (true == g_bIsPumpStop) \
        && (false == g_isRisingStateRele))
    {
        if (true == IsTimedout(g_SlowReleHoldTimeIndex))//时间到
        {
            u8SlowReleRunCount = 0;
            
            if (((p_nibpResult->risingResult > 0)\
                &&(g_Rising_mmHgCur != p_nibpResult->risingResult))\
                || (true == p_nibpResult->isRisingDone)\
                || (!g_u8RisingStepCount))
            {
                if (!g_u8RisingStepCount)//第一阶段算法不处理，解决回落问题
                {
                    g_Rising_mmHgCur = g_dynamicPrepress - RISING_FIRST_FIXED_STEP;
                    g_u8RisingStepCount++;
                }
                else
                {
                    g_Rising_mmHgCur = p_nibpResult->risingResult;
                    if (p_nibpResult->risingResult > (g_overload_mmHg -5))
                    {
                        u8_pressue_over_range_cnt++;
                        g_Rising_mmHgCur = g_overload_mmHg -5;
                        if (u8_pressue_over_range_cnt > 2)
                        {
                            NIBP_ForceStop();
                            NIBP_AlarmUpload(OVER_MEASURE_PRESSURE);
                        }
                    }
                }
                
                if (true == IsNIBPOverLoad(g_Rising_mmHgCur)) //larger than target
                {
                    g_isRisingStateRele = true;
                }
                else //lower than target
                {
                    g_risingStep[g_u8RisingStepCount] = g_Rising_mmHgCur;
                    if (++g_u8RisingStepCount >= NIBP_RISING_STEP_MAX)
                        g_u8RisingStepCount = NIBP_RISING_STEP_MAX-1;
                    nibp_debug_start_pwm();
                }
            }
        }
    }

    //rising state slow release handle
    if (true == g_isRisingStateRele)
    {
        if (true == IsNIBPOverLoad(g_Rising_mmHgCur)) //larger than target
        {
            u8SlowReleRunCount = 0;
            NIBP_SetPressRelease(SLOW_RELE);
        }
        else
        {
            g_risingStep[g_u8RisingStepCount] = g_Rising_mmHgCur;
            if (++g_u8RisingStepCount >= NIBP_RISING_STEP_MAX)
                g_u8RisingStepCount = NIBP_RISING_STEP_MAX-1;
            NIBP_SetPressRelease(PRESS_HOLD);
            NIBP_refreshSlowReleHoldTime(true, p_nibpResult->pulse_ms);
            ResetTimeout(g_SlowReleHoldTimeIndex, getTickNumberByMS(g_u8SlowReleHoldTimeMS));
            g_isRisingStateRele = false;
        }
    }
    
    if ((true == p_nibpResult->isRisingDone)/* \
        && (true == IsNIBPOverLoad(g_Rising_mmHgCur))*/)
    {
        int i;
        u16 slowRele_mmHg = g_Rising_mmHgCur;
        u16 first_step_mmHg = g_risingStep[1];
//        if ((g_staicPrepress - 10 < slowRele_mmHg) \
//            && (slowRele_mmHg < g_staicPrepress))
//        {
//            slowRele_mmHg -= 10;
//        }
        qsort(g_risingStep, g_u8RisingStepCount, sizeof(u16), u16Cmp);
        for (i=0; i < g_u8RisingStepCount;i++)
        {
            if ((slowRele_mmHg > (g_risingStep[i]-5)) \
                && (slowRele_mmHg < (g_risingStep[i]+5)))
            {
                slowRele_mmHg -= 10;
            }
            NIBP_DBG_INFO("g_risingStep[%d]=%d mmHg",i,g_risingStep[i]);
        }
        
        g_nibp_algo_state = NIBP_ALGO_FALLING;
        g_bIsCalculateDrop = false;
        if (g_risingStep[0] > first_step_mmHg)//判断是否有二次上升
        {
            g_isRisingTwice = true;
            NIBP_DBG_INFO("NIBP_RisingCtrl g_isRisingTwice %d > %d!",
                g_risingStep[0], first_step_mmHg);
        }
        NIBP_setParamEndofRising(10, p_nibpResult->pulse_ms, slowRele_mmHg);
        NIBP_DBG_INFO("BPM  = %d ms,MAX = %d mmHg",
                        p_nibpResult->pulse_ms,
                        g_Rising_mmHgCur);
        NIBP_SetPWM(DISABLE);
        NIBP_SetPressRelease(PRESS_HOLD);
        g_bIsPumpStop = false;
        nibp_algorithm_pushLastRet( &g_lastNIBPResult.DP,
                                    &g_lastNIBPResult.SP,
                                    &g_lastNIBPResult.BPM);
    }
}

void NIBP_StartSample(void)
{
    g_COMADC_channel = NIBP_channel;
    g_COMADC_state = 0;
    COM_ADC_DataPin_Init(DRDY_MODE);//sample ADC

    if ((ENABLE == g_NIBP_PWM_Status) && (VENIPUNCTURE_TEST == g_curWorkMode))
    {
        //handle venipuncture function
        if (true == IsNIBPOverLoad(g_u8Venipuncture))
        {
            g_gas_leak_tick = SysTick_Get() + getTickNumberByMS(20000);
            NIBP_SetPWM(DISABLE);
        }
        return;
    }
    
    NIBP_RisingCtrl();

    //过压保护控制1:(max-3)DSP产生告警,2:STM32直接泄气
    if (true == IsNIBPOverLoad(g_overload_mmHg))
    {
        if (g_overload_count++ > 3)
        {
            NIBP_AlarmUpload(OVER_PROTECT_PRESSURE);
            NIBP_DBG_INFO("NIBP_StartSample() g_pNIBP_dev->adc_val=%d(0x%X),g_pNIBP_dev->mmHg=%d",
                g_pNIBP_dev->adc_val,g_pNIBP_dev->adc_val,g_pNIBP_dev->mmHg_val);
            NIBP_ForceStop();
        }
    }
}

static void NIBP_SampleAutoTest(void)
{
    if(IsOnTime(g_u32nibpAutoNextTime)) // is on time or over time
    {
        if (ENABLE != g_NIBP_Status)
        {
            NIBP_SampleOneTimeByDebug();
        }
        g_u32nibpAutoNextTime = SysTick_Get() + g_AutoTestCycle;
    }
}

static void NIBP_startPumpCtrl(void)
{
    u8 state = 0, try_count = 0;
    int result = -1;
    
    if (NEWBORN == g_PatientType)
    {
        NIBP_SetPWM_DutyCycles(NIBP_NEWBORN_DUTY_CYCLE);
    }
    else
    {
        NIBP_SetPWM_DutyCycles(NIBP_DEFAULT_DUTY_CYCLE);
    }

    NIBP_SetPressRelease(PRESS_HOLD);
    if (true == NIBP_isStm32FastRele())//STM32处在血压保护[快速泄压模式]
    {
        do 
        {
            try_count++;
            result = aio_stm32_writeNIBPState(&state);
        } while ((result < 0) && (try_count < 5));
    }
    NIBP_SetPWM(ENABLE);
    
    NIBP_DataReset();
    nibp_algorithm_start();
}

static void NIBP_SampleOneTimeByDebug(void)
{
    u16 lastSP;
    g_NIBP_Status = ENABLE;
    g_u8UploadMaskMaxCount = g_u8UploadMaskMaxCount;
    g_isRisingStateRele = false;
    g_u8RisingStepCount = 0;
    isFisrtRisingStep = 1;
    g_nibp_one_time_tick = SysTick_Get();
    g_overload_count = 0;
    u8_pressue_over_range_cnt = 0;

    //========Handle next g_Rising_mmHgCur Start===========
    if (0 == g_risingStep[0])//若为初始化值
        g_dynamicPrepress = g_staicPrepress;
    if (true == g_isRisingTwice)//若上次测量升压两次以上
    {
        g_dynamicPrepress = g_risingStep[0];
        g_isRisingTwice = false;
    }
    if (0 != g_lastNIBPResult.SP)//根据上次测量结果调整
    {
        lastSP = g_lastNIBPResult.SP / AMPLIFY_FACTOR + 30;
        if (g_dynamicPrepress > lastSP) g_dynamicPrepress = lastSP;
    }
    g_Rising_mmHgCur = g_dynamicPrepress;
    if (g_Rising_mmHgCur > g_overload_mmHg - 5)//判断最大上限值
    {
        g_Rising_mmHgCur = g_overload_mmHg - 10;
    }
    NIBP_DBG_INFO(">>NIBP_SampleOne g_dynamicPrepress=%d g_risingStep[0]=%d",
        g_dynamicPrepress,g_risingStep[0]);
    //========Handle next g_Rising_mmHgCur End===========
    memset(g_risingStep, 0, sizeof(u16)*NIBP_RISING_STEP_MAX);
    
    //setting the force release press timer for protect patient.
    ResetTimeout(g_ForceReleTimeIndex, getTickNumberByMS(g_measure_max_ms));
    NIBP_startPumpCtrl();
}

void NIBP_SampleOneTime(void)
{
    g_curWorkMode = MANUAL_TEST;
    if (ENABLE == g_NIBP_Status)
    {
        NIBP_SetPWM(ENABLE);
    }
    else
    {
        NIBP_SampleOneTimeByDebug();
    }
    g_bIsPumpStop = false;

    NIBP_StartStopRespond(1);
}

void NIBP_ForceStop(void)
{
    g_curWorkMode = MANUAL_TEST;
    g_NIBP_Status = DISABLE;
    g_bIsPumpStop = false;
    g_nibp_algo_state = NIBP_ALGO_IDLE;
    
    NIBP_SetPWM(DISABLE);
    NIBP_SetPressRelease(FAST_RELE);
    NIBP_DBG_INFO(">>>>>>>>>>>>>>>>Fast Rele 4");
    
    NIBP_StartStopRespond(0);
}

static void NIBP_ReslultUpload(void)
{
    char pValue[13];
    u16 sp = (p_nibpResult->SP + AMPLIFY_FACTOR /2) / AMPLIFY_FACTOR;
    u16 dp = (p_nibpResult->DP + AMPLIFY_FACTOR /2) / AMPLIFY_FACTOR;
    u16 map = (p_nibpResult->MAP + AMPLIFY_FACTOR /2) / AMPLIFY_FACTOR;
    u16 bpm = p_nibpResult->BPM;
    unsigned long nextTime = 0;

    if ((sp-dp < 15)|| ((sp-map)-(map-dp)) > 60)
    {
        NIBP_AlarmUpload(MEASURE_FAILED);
        return;
    }
    if (p_nibpResult->reliability > 0.5f)
    {
        NIBP_DBG_INFO("NIBP_ReslultUpload reliability=%f > 0.5",p_nibpResult->reliability);
        NIBP_AlarmUpload(ARM_EXERCISE);
        return;
    }
    g_lastNIBPResult.BPM = p_nibpResult->BPM;
    g_lastNIBPResult.DP= p_nibpResult->DP;
    g_lastNIBPResult.SP = p_nibpResult->SP;
    /*
    * [0]:  bit7,result status;
    *       bit6~5,test mode:manual,auto,continue;
    *       bit4~3,patient type:adult,child,newborn;
    */
    pValue[0] = 0;
    
    /*
    * [1]:  bit7,   reserved
    *       bit6,   movement disturbance 
    *       bit5,   pressure over range
    *       bit4,   pulse is too weak
    *       bit3,   pressure error
    *       bit2,   air leakage
    *       bit1,   oversleeve error
    *       bit0,   selfcheck result
    */
    pValue[1] = 0;

    /*
    * [2]:  bit7~5, reserved
    *       bit4,   test timeout
    *       bit3,   system error
    *       bit2,   air leakage
    *       bit1,   signal overload
    *       bit0,   over range:adult(297mmHg),child(240mmHg),newborn(147mmHg)
    */
    pValue[2] = 0;

    /*
    * [3~4]:    SP:systolic pressure
    */
    pValue[3] = (char)(sp >> 8) & 0xFF;
    pValue[4] = (char)sp & 0xFF;

    /*
    * [5~6]:    MBP:mean blood pressure
    */
    pValue[5] = (char)(map >> 8) & 0xFF;
    pValue[6] = (char)map & 0xFF;
    
    /*
    * [7~8]:    DP:diastolic pressure
    */
    pValue[7] = (char)(dp >> 8) & 0xFF;
    pValue[8] = (char)dp & 0xFF;
    
    /*
    * [9~10]:   pulse
    */
    pValue[9] = (char)(bpm >> 8) & 0xFF;
    pValue[10] = (char)bpm & 0xFF;

    /*
    * [11~12]:  next measuring time second
    */
    if (AUTO_TEST == g_curWorkMode)
    {
        nextTime = g_u32nibpAutoNextTime - SysTick_Get();//tick count
        nextTime = getMSByTickcount(nextTime) / 1000;
    }
    pValue[11] = (char)(nextTime >> 8) & 0xFF;
    pValue[12] = (char)(nextTime & 0xFF);

    UploadDataByID(AIO_TX_NIBP_RESLULT_ID, TRUE, pValue, sizeof(pValue));
}

static void NIBP_VerifyStatusUpload(void)
{
    char value = 0;

    /*
    * [0]:  bit7~4, Reserved
    *       bit3,   NIBP(DSP) whether verify or not
    *       bit2,   NIBP(DSP) verify result ok or error
    *       bit1,   NIBP(STM32) protect whether verify or not
    *       bit0,   NIBP(STM32) protect verify result ok or error
    */
    if (TRUE == pEeprom_struct->nibp_IsVerify)
    {
        SETBIT(value, 3);
    }
    if (pEeprom_struct->chIsProtectVerify)
    {
        SETBIT(value, 1);
    }
    
    UploadDataByID(AIO_TX_NIBP_VERIFY_STATE_ID, TRUE, &value, 1);
}

static void NIBP_StartStopRespond(const char state)
{
#ifdef _UPLOAD_1HZ_DATA_
    char value;

    /*
    * [0]:  0x00,   received stop command
    *       0x01,   received start command
    */
    if (state)
    {
        value = 1;
    }
    else
    {
        value = 0;
    }
    
    UploadDataByID(AIO_TX_NIBP_ACT_RESPOND_ID, TRUE, &value, 1);
#endif
}

static void NIBP_RealTimeDataUpload(void)
{
    u16 nLen = 0;
    u32 tick, mmHg, adc;

    //目前buffer是否有数据更新
    nLen = NIBP_PopResult(&tick, &mmHg, &adc);
    if ((!nLen)|| (false == g_IsNIBP_Upload)) return;//无数据更新，退出

#ifdef _UPLOAD_NIBP_ADC_
    char pValue[4];

    pValue[0] = 0;//bit0:1 for having heartbeat,0 for no heartbeat.

    pValue[1] = (char)(mmHg >> 16) & 0xFF;      
    pValue[2] = (char)(mmHg >> 8) & 0xFF;
    pValue[3] = (char)(mmHg >> 0) & 0xFF;       //NIBP Vaule LSB

    UploadDataByID(AIO_TX_NIBP_REALTIME_ID, TRUE, pValue, sizeof(pValue));
#endif

#ifdef _PRINT_NIBP_SAMPLE_
    if(SLOW_RELE_ADC_TAG == mmHg)
    {
        AIO_printf("\r\n-1");
    }
    else
    {
        AIO_printf("\r\n%d\t0x%X",mmHg,adc);
    }
#endif
}

static void NIBP_Display_mmHg(void)
{
    s16 nLen = 0;
    u16 mmHg;

    //目前buffer是否有数据更新
    nLen = NIBP_PopDisplay_mmHg(&mmHg);
    if ((!nLen)|| (false == g_IsNIBP_Upload)) return;//无数据更新，退出

#ifdef _UPLOAD_NIBP_ADC_
    char pValue[2];
    pValue[0] = (char)(mmHg >> 8) & 0xFF;
    pValue[1] = (char)mmHg& 0xFF;

    UploadDataByID(AIO_TX_NIBP_MMHG_ID, TRUE, pValue, sizeof(pValue));
#endif
}

static void NIBP_ForceFastRelease(void)
{
    unsigned long nextTime;
    g_NIBP_Status = DISABLE;
    g_bIsPumpStop = false;
    g_nibp_algo_state = NIBP_ALGO_IDLE;
    NIBP_SetPWM(DISABLE);
    NIBP_SetPressRelease(FAST_RELE);
    NIBP_DBG_INFO(">>>>>>>>>>>>>>>>Fast Rele 5");
    
    if (AUTO_TEST == g_curWorkMode)
    {
        nextTime = g_u32nibpAutoNextTime - SysTick_Get();//tick count
        nextTime = getMSByTickcount(nextTime) / 1000;

        if (nextTime < 30)//at leaset 30s betweent two test
        {
            g_u32nibpAutoNextTime = SysTick_Get() + getTickNumberByMS(30000);
        }
    }

    if (CONTINUED_TEST == g_curWorkMode)
    {
        g_continue_next_tick = SysTick_Get() + getTickNumberByMS(10000);
    }
    
    if (p_nibpResult->isAllDone)
    {
        NIBP_DBG_INFO( "ALGO RESLULT:"
                    "\n\rDP     = %d"
                    "\n\rSP     = %d"
                    "\n\rMAP    = %d"
                    "\n\rBPM    = %d"
                    "\n\rTime   = %d s",
                    (u16)((p_nibpResult->DP + AMPLIFY_FACTOR /2) / AMPLIFY_FACTOR),
                    (u16)((p_nibpResult->SP + AMPLIFY_FACTOR /2) / AMPLIFY_FACTOR),
                    (u16)((p_nibpResult->MAP + AMPLIFY_FACTOR /2) / AMPLIFY_FACTOR),
                    p_nibpResult->BPM,
                    getMSByTickcount(SysTick_Get() - g_nibp_one_time_tick)/1000);
        NIBP_ReslultUpload();
    }
    if (true == IsTimedout(g_ForceReleTimeIndex))
    {
        NIBP_AlarmUpload(MEASURE_TIMEOUT);
    }
}

static void NIBP_ContinueModeHandle(void)
{
    if (IsOnTime(g_continue_test_tick))
    {
        g_curWorkMode = g_backup_mode;
    }
    else
    {
        if ((DISABLE == g_NIBP_Status) && IsOnTime(g_continue_next_tick))
        {
            NIBP_SampleOneTimeByDebug();
        }
    }
}

void NIBP_Handler(void)
{
    if (AUTO_TEST == g_curWorkMode)
    {
        NIBP_SampleAutoTest();
    }

    if (CONTINUED_TEST == g_curWorkMode)
    {
        NIBP_ContinueModeHandle();
    }

    if (VENIPUNCTURE_TEST == g_curWorkMode)
    {
        if ((1 == is_gas_leak_test) \
            && (0 != g_gas_leak_tick) \
            && (true == IsOnTime(g_gas_leak_tick)))
        {
            //lower than 160mmHg (180mmHg -160mmHg)/20s < 1 mmHg/s
            if (false == IsNIBPOverLoad(160))
            {
                NIBP_AlarmUpload(CUFF_GAS_LEAKAGE);
            }
            g_bIsPumpStop = false;
            NIBP_SetPressRelease(FAST_RELE);
            NIBP_DBG_INFO(">>>>>>>>>>>>>>>>Fast Rele 6");
            is_gas_leak_test = 0;
            g_curWorkMode = g_backup_mode;
        }
    }
    
    if (ENABLE == g_NIBP_Status)
    {
        //handle press release.
        if (NIBP_ALGO_FALLING == g_nibp_algo_state)
        {
            NIBP_AutoPressReleHanlder();
        }
        //protect on time, force to relesase the press
        if ((true == IsTimedout(g_ForceReleTimeIndex)) \
            || (true == p_nibpResult->isAllDone))
        {
            NIBP_DBG_INFO("NIBP_Handler() IsTimedout or done.");
            NIBP_ForceFastRelease();
        }
    }

    if (ENABLE == g_NIBP_PWM_Status)
    {
        if (IsOnTime(g_oversleeve_tick))
        {
            if (false == IsNIBPOverLoad(30)) 
            {
                if (true == IsNIBPOverLoad(10))
                {//10s后依旧未加压至30mmHg认为袖套脱落
                    NIBP_AlarmUpload(CUFF_OFF);
                }
                else
                {//10s后依旧未加压至10mmHg认为泵漏气
                    NIBP_AlarmUpload(PUMP_LEAKAGE);
                }
                NIBP_ForceStop();
            }
        }
    }
    //detect stm32 protect value working mode.
    NIBP_STM32_protectHandle();
    NIBP_RealTimeDataUpload();
    NIBP_Display_mmHg();
}

RETURN_TypeDef NIBP_SetPatientType(u8 newType)
{
    switch(newType)
    {
    case 1: //ADULT
        g_u8SlowReleMin_mmHg = NIBP_ADULT_LOWEST_PRESS;
        g_overload_mmHg = ADULT_PRESSURE_MAX;
        break;
    case 2: //CHILD
        g_u8SlowReleMin_mmHg = NIBP_ADULT_LOWEST_PRESS;
        g_overload_mmHg = CHILD_PRESSURE_MAX;
        break;
    case 3: //NEWBORN
        g_u8SlowReleMin_mmHg = NIBP_NEWBORM_LOWEST_PRESS;
        g_overload_mmHg = NEWBORN_PRESSURE_MAX;
        break;
    default:
        break;
    }
    nibp_algorithm_setPatient((char)newType);
    NIBP_SetDefaultMaxPressure();
    NIBP_SetProtectTime();
    NIBP_resetModule();
    return RETURN_OK;
}

void NIBP_resetModule(void)
{
    g_isRisingTwice = false;
    memset(g_risingStep, 0, sizeof(u16)*NIBP_RISING_STEP_MAX);
    memset(&g_lastNIBPResult, 0, sizeof(g_lastNIBPResult));
}

void NIBP_getResult(UartProtocolPacket *pPacket)
{
    if(0xCC == pPacket->DataAndCRC[0])
    {
        NIBP_ReslultUpload();
    }
}

void NIBP_setVenipuncture(UartProtocolPacket *pPacket)
{
    if(0 == pPacket->DataAndCRC[0]) //stop
    {
        g_bIsPumpStop = false;
        NIBP_SetPressRelease(FAST_RELE);
        NIBP_DBG_INFO(">>>>>>>>>>>>>>>>Fast Rele 7");
        g_curWorkMode = g_backup_mode;
    }
    else if (0x01 == pPacket->DataAndCRC[0]) //start
    {
        /*
        * newborn   :20 30 40 50,default 40
        * child     :20 30 40 50 60 70 80,default 60
        * adult     :20 30 40 50 60 70 80 90 100 110 120,default 80
        */
        g_u8Venipuncture = pPacket->DataAndCRC[1];
        g_backup_mode = g_curWorkMode;
        g_curWorkMode = VENIPUNCTURE_TEST;
        g_NIBP_Status = DISABLE;
        NIBP_startPumpCtrl();
    }
}

void NIBP_setContinueMode(UartProtocolPacket *pPacket)
{
    if (0x01 == pPacket->DataAndCRC[0]) //start
    {
        g_backup_mode = g_curWorkMode;
        g_curWorkMode = CONTINUED_TEST;
        g_continue_test_tick = SysTick_Get() + getTickNumberByMS(5*60*1000);
        NIBP_SampleOneTimeByDebug();
    }
}

void NIBP_setGasLeakTest(UartProtocolPacket *pPacket)
{
    if (0x01 == pPacket->DataAndCRC[0]) //start
    {
        is_gas_leak_test = 1;
        g_u8Venipuncture = 180;
        g_backup_mode = g_curWorkMode;
        g_curWorkMode = VENIPUNCTURE_TEST;
        g_gas_leak_tick = 0;
        g_NIBP_Status = DISABLE;
        NIBP_startPumpCtrl();
    }
}

static void NIBP_AlarmUpload(const NIBP_ALARM_TypeDef type)
{
#ifdef _UPLOAD_1HZ_DATA_
    u8 value = (u8)type;
    if (true == g_IsNIBP_Upload)
    {
        UploadDataByID(AIO_TX_NIBP_ALARM_ID, TRUE, (char *)&value, 1);
    }
#endif

}

/*******************************************************************************
* NIBP TEST or DEBUG API start
*******************************************************************************/
void NIBP_ADC_Average(int sampleCount, unsigned long *pADCAverage)
{
    int i = 0;
    unsigned long res = 0;
    for (i = 0; i < sampleCount; i++)
    {
        g_pNIBP_dev->waitForADCStatus = ENABLE;
        g_COMADC_channel = NIBP_channel;
        g_COMADC_state = 0;
        COM_ADC_DataPin_Init(DRDY_MODE);//sample ADC
        while(ENABLE == g_pNIBP_dev->waitForADCStatus)//wait for ADC sample
        {
            delay_us(10);
        }
        res += g_pNIBP_dev->adc_val;
        Delay1ms(2);
        ExtWatchDog_Reset();
    }
    *pADCAverage = res / sampleCount;
}

//进入血压校准模式，主要关闭阀门
void NIBP_VerifyEnter(void)
{
//    int mmHgVal = 0, i;
//    unsigned long adcVal = 0;
//    RETURN_TypeDef result = RETURN_ERROR;
//    u16 u16Addr = 0;
//    u8 u8Len = 0;
//    
//    NIBP_SetPWM(DISABLE);
//    NIBP_SetPressRelease(FAST_RELE);//先进行泄气
//    NIBP_DBG_INFO(">>>>>>>>>>>>>>>>Fast Rele 8");
//    Delay1ms(100);
    NIBP_SetPressRelease(PRESS_HOLD);//关闭阀门，等待校准操作
    aio_stm32_setVerify(1);
}

void NIBP_VerifyExit(void)
{
    EEPROM_RefreshNIBPVerify();//更新内存变量
    NIBP_SetCycle(g_curCycler);//恢复校准前模式
    aio_stm32_setVerify(0);
}

//主要提供应答信号给上位机，用于血压校准
static void NIBP_DebugVerifyRespond(void)
{
    char value;
    value = 0;//retun 0:success, 1:failed
    UploadDataByID(AIO_RX_NIBP_VERIFY_ID, TRUE, &value, 1);
}

void NIBP_VerifyBySimulator(const u8 mode)
{
    if (0xCC == mode)//进入校正功能
    {
        g_pNIBP_dev->verifyStauts = ENABLE;
        
        //stop upload ecg and resp.
        g_IsECG_Upload = false;
        g_IsRESP_Upload = false;
        g_IsTemp_Upload = false;
        g_IsCommon_Upload = false;
        g_IsSpO2_Upload = false;
        
        NIBP_VerifyEnter();
        NIBP_VerifyStatusUpload();
    }
    else if (0xFF == mode)//退出校正功能
    {
        g_pNIBP_dev->verifyStauts = DISABLE;

        //restore upload ecg and resp.
        g_IsECG_Upload = true;
        g_IsRESP_Upload = true;
        g_IsTemp_Upload = true;
        g_IsCommon_Upload = true;
        g_IsSpO2_Upload = true;
        
        NIBP_VerifyExit();
    }

    //回应上位机，操作已成功
    NIBP_DebugVerifyRespond();
}

void NIBP_VerifyAction(const u8 mmHg)
{
    unsigned long adcVal = 0;
    RETURN_TypeDef result = RETURN_ERROR;
    u16 u16Addr = 0;
    u8 u8Len = 0;
    
    //TODO,close STM32 protect NIBP function.

    
    NIBP_ADC_Average(10, &adcVal);

    if (50 == mmHg)
    {
        pEeprom_struct->nibp_Verify_mmHg[2] = 50;
        pEeprom_struct->nibp_Verify_Val[2] = adcVal;
        pEeprom_struct->nibp_IsVerify = TRUE;
        
        u16Addr = (char *)&(pEeprom_struct->nibp_Verify_mmHg[2]) - (char *)pEeprom_struct;
        result =  EEPROM_BufferWrite(u16Addr, (u8 *)&(pEeprom_struct->nibp_Verify_mmHg[2]), sizeof(u16));
        u16Addr = (char *)&(pEeprom_struct->nibp_Verify_Val[2]) - (char *)pEeprom_struct;
        result |=  EEPROM_BufferWrite(u16Addr, (u8 *)&(pEeprom_struct->nibp_Verify_Val[2]), sizeof(u32));
    }
    else if (150 == mmHg)
    {
        pEeprom_struct->nibp_Verify_mmHg[4] = 150;
        pEeprom_struct->nibp_Verify_Val[4] = adcVal;

        u16Addr = (char *)&(pEeprom_struct->nibp_Verify_mmHg[4]) - (char *)pEeprom_struct;
        result =  EEPROM_BufferWrite(u16Addr, (u8 *)&(pEeprom_struct->nibp_Verify_mmHg[4]), sizeof(u16));
        u16Addr = (char *)&(pEeprom_struct->nibp_Verify_Val[4]) - (char *)pEeprom_struct;
        result |=  EEPROM_BufferWrite(u16Addr, (u8 *)&(pEeprom_struct->nibp_Verify_Val[4]), sizeof(u32));
    }
    else if (250 == mmHg)
    {
        pEeprom_struct->nibp_Verify_mmHg[5] = 250;
        pEeprom_struct->nibp_Verify_Val[5] = adcVal;
        pEeprom_struct->nibp_IsVerify = TRUE;

        u16Addr = (char *)&(pEeprom_struct->nibp_Verify_mmHg[5]) - (char *)pEeprom_struct;
        result =  EEPROM_BufferWrite(u16Addr, (u8 *)&(pEeprom_struct->nibp_Verify_mmHg[5]), sizeof(u16));
        u16Addr = (char *)&(pEeprom_struct->nibp_Verify_Val[5]) - (char *)pEeprom_struct;
        result |=  EEPROM_BufferWrite(u16Addr, (u8 *)&(pEeprom_struct->nibp_Verify_Val[5]), sizeof(u32));
        u16Addr = (char *)&(pEeprom_struct->nibp_IsVerify) - (char *)pEeprom_struct;
        result |=  EEPROM_BufferWrite(u16Addr, (u8 *)&(pEeprom_struct->nibp_IsVerify), sizeof(TRUE_OR_FALSE));
    }
    
    if (RETURN_OK == result)
    {
        AIO_printf("\r\nNIBP_VerifyAction Success!");
    }
    else
    {
        AIO_printf("\r\nNIBP_VerifyAction Error!");
    }

    //TODO,recover STM32 protect NIBP function.
}

static void NIBP_RespondSTM32VerifyEEPROM(void)
{
    char value;
    value = 0xFF;
    UploadDataByID(AIO_RX_NIBP_STM32_PRESS_ID, TRUE, &value, 1);
}

static void NIBP_RespondSTM32Verify(u8 index, u16 *mmHg, u16 *adc)
{
    char pBuf[5];
    pBuf[0] = index;
    pBuf[1] = (*mmHg >> 8) & 0xFF;  //MSB
    pBuf[2] = (*mmHg & 0xFF);       //LSB
    pBuf[3] = (*adc >> 8) & 0xFF;   //MSB
    pBuf[4] = (*adc & 0xFF);        //LSB
    UploadDataByID(AIO_RX_NIBP_STM32_PRESS_ID, TRUE, pBuf, sizeof(pBuf));
}

void NIBP_STM32VerifyAction(u8 *pBuf, u8 nLen)
{
    u8 index = pBuf[0];

    if (index < 2)
    {
        if (3 != nLen) return;
        
        u16 SetmmHg = (pBuf[1] << 8) | pBuf[2];
        u16 adc = 0;
        u8 buf[2] = {0,};
        u8 i;

        for (i = 0; i < 10; i++) 
        {
            aio_stm32_readByID(CURRENT_PRESSURE, buf, 2);
            adc += (buf[0] << 8) | buf[1];
        }
        adc /= 10;//average for 10 count
        
        NIBP_DBG_INFO("STM32 NIBP ADC Index = %d mmHg = %d ADC = 0x%04X", index, SetmmHg, adc);
        //refresh global value.
        EEPROM_saveRAMSTM32NIBPVerify(index, &SetmmHg, &adc);
        NIBP_RespondSTM32Verify(index, &SetmmHg, &adc);
    }
    else if (0xFF == index)
    {
        RETURN_TypeDef result;
        //Save result from global into EEPROM.
        result = EEPROM_saveSTM32NIBPVerify();
        if (RETURN_OK == result)
        {
            NIBP_RespondSTM32VerifyEEPROM();
        }
    }
}

//主要提供应答信号给上位机，用于血压校准
static void NIBP_DebugSetmmHgRespond(const u8 nIndex, const u16 mmHg, const u32 adcVal)
{
    char pValue[7];
    
    pValue[0] = 0x01;//校准接口

    pValue[1] = (char)nIndex;      //Index
    
    pValue[2] = (char)((mmHg >> 8) & 0xFF);//mmHg
    pValue[3] = (char)(mmHg & 0xFF);
    
    pValue[4] = (char)(adcVal >> 16) & 0xFF;      //adcVal Vaule MSB
    pValue[5] = (char)(adcVal >> 8) & 0xFF;
    pValue[6] = (char)(adcVal >> 0) & 0xFF;       //adcVal Vaule LSB

    UploadDataByID(AIO_RX_NIBP_Debug_ID, TRUE, pValue, sizeof(pValue));
}

//分别对6个点进行相关处理
//pBuf[0]:0~5,索引量
//pBuf[1]:mmHg的高字节
//pBuf[2]:mmHg的低字节
static RETURN_TypeDef NIBP_VerifyDebug(const u8 *pBuf, const u8 nLen)
{
    
    u16 set_mmHg = 0;
    unsigned long adcVal = 0;
    u8 u8Index = pBuf[0];
    
    if ((3 != nLen) || (DISABLE == g_pNIBP_dev->verifyStauts))//长度不对，或者尚未进入校准模式
    {
        AIO_printf("\r\nNIBP_VerifyDebug error");
        return RETURN_ERROR;
    }

    set_mmHg = (pBuf[1] << 8) | pBuf[2];//获取当前设置值
    NIBP_ADC_Average(10, &adcVal);

    if (u8Index < NIBP_VERIFY_NUM)
    {
        //更新内存变量的相关值
        pEeprom_struct->nibp_Verify_mmHg[u8Index] = set_mmHg;
        pEeprom_struct->nibp_Verify_Val[u8Index] = adcVal;
        NIBP_DBG_INFO("Done! ADC value = %d", adcVal);
        
        //回应上位机消息，把ADC的值
        NIBP_DebugSetmmHgRespond(u8Index,set_mmHg,adcVal);
        return RETURN_OK;
    }
    return RETURN_ERROR;
}

//主要提供应答信号给上位机，用于血压校准
static void NIBP_DebugSaveRespond(void)
{
    char value;
    value = 0x02;//retun 0:success, 1:failed
    UploadDataByID(AIO_RX_NIBP_Debug_ID, TRUE, &value, 1);
}

static RETURN_TypeDef NIBP_VerifySave2Eepom(void)
{
    u16 u16Addr = 0;
    u8 u8Len = 0;
    RETURN_TypeDef result = RETURN_ERROR;

    //把内存变量写进EEPROM里面去
    pEeprom_struct->nibp_IsVerify = TRUE;
    
    u16Addr = (char *)&(pEeprom_struct->nibp_Verify_mmHg[0]) - (char *)pEeprom_struct;
    u8Len = (char *)&(pEeprom_struct->nibp_IsVerify) + sizeof(TRUE_OR_FALSE) - (char *)&(pEeprom_struct->nibp_Verify_mmHg[0]);
    result =  EEPROM_BufferWrite(u16Addr, (u8 *)&(pEeprom_struct->nibp_Verify_mmHg[0]), u8Len);
    if (RETURN_OK == result)
    {
        NIBP_DBG_INFO("NIBP_VerifySave2Eepom OK");
    }
    else
    {
        NIBP_DBG_INFO("NIBP_VerifySave2Eepom Error");
    }

    //回应上位机消息
    NIBP_DebugSaveRespond();
    return result;
}

static RETURN_TypeDef NIBP_SetUploadMaskCount(u8 count)
{
    g_u8UploadMaskMaxCount = count;
    return RETURN_OK;
}

static RETURN_TypeDef nibp_debug_start_pwm(void)
{
    if (ENABLE == g_NIBP_Status)//已经启动测试了，现在只需要操作PWM
    { 
        if (NEWBORN == g_PatientType)
        {
            NIBP_SetPWM_DutyCycles(NIBP_NEWBORN_DUTY_CYCLE);
        }
        else
        {
            NIBP_SetPWM_DutyCycles(NIBP_DEFAULT_DUTY_CYCLE);
        }
        NIBP_SetPWM(ENABLE);
    }
    else//尚未启动测量，则进行启动
    {
        NIBP_SampleOneTimeByDebug();
    }
    g_bIsPumpStop = false;
    return RETURN_OK;
}

static RETURN_TypeDef nibp_debug_stop_pwm(void)
{
    g_bIsPumpStop = true;
    NIBP_SetPWM(DISABLE);
    return RETURN_OK;
}

static RETURN_TypeDef nibp_debug_start(const u8 *pBuf, const u8 nLen)
{
    //设置最大值，预充压
    NIBP_SetPrepressure((u16)((pBuf[0]<<8)|pBuf[1]));
    //设置阶梯泄压高度
    g_u8SlowReleStepmmHg = pBuf[2];
    //设置阶梯泄压保持时间
    g_u8SlowReleHoldTimeMS = (u16)((pBuf[3]<<8)|pBuf[4]);
    //设置最小值，泄压最小值
    g_u8SlowReleMin_mmHg = pBuf[5];

    g_curWorkMode = DEBUG_TEST;
    NIBP_SampleOneTimeByDebug();//立即执行
    return RETURN_OK;
}

static RETURN_TypeDef nibp_debug_pass_step(const u8 *pBuf)
{
    //设置阶梯泄压高度
    g_u8SlowReleStepmmHg = pBuf[0];
    //设置阶梯泄压保持时间
    g_u8SlowReleHoldTimeMS = (u16)((pBuf[1]<<8)|pBuf[2]);
    
    if (DEBUG_TEST == g_curWorkMode)
    {
        //close pwm,and begin to release pressure
        if (ENABLE == g_NIBP_PWM_Status)
        {
            u8SlowReleRunCount = 0;
            g_bIsCalculateDrop = true;
            NIBP_SetPWM(DISABLE);
        }
    }
    return RETURN_OK;
}

static RETURN_TypeDef nibp_debug_algo_sw(const u8 *pBuf)
{
    if (0 == pBuf[0])
    {
        g_nibp_algo_sw = true;
        nibp_algorithm_start();
    }
    else if (0x01 == pBuf[0])
    {
        g_nibp_algo_sw = false;
    }
    
    NIBP_DataReset();
    return RETURN_OK;
}

static RETURN_TypeDef nibp_debug_setStepCtl(const u8 *pBuf, const u8 nLen)
{
    if (3 != nLen) return RETURN_ERROR;
    u16 maxPress;
    //设置阶梯泄压保持时间
    g_u8SlowReleHoldTimeMS = 60000 / pBuf[0] *3 ; //BPM value
    maxPress = (u16)((pBuf[1] << 8) | pBuf[2]); //Max Press Value
    NIBP_SetPrepressure(maxPress);//-DEBUG140120- dynamic change slowRele by ALGO GUI debug.

    return RETURN_OK;
}


static RETURN_TypeDef nibp_debug_StepRsingResult(const u8 *pBuf)
{//rising_bpm_count不再使用,此算法不再使用无法满足算法平台算法控制
//    u8 rising_bpm_count;
//    u8 falling_step_mmHg;
//    u16 maxPressure;

//    rising_bpm_count = pBuf[0];
//    maxPressure = (u16)((pBuf[1] << 8) | pBuf[2]);
//    falling_step_mmHg = pBuf[3];

//    NIBP_setParamEndofRising(falling_step_mmHg,
//                             rising_bpm_count,
//                             maxPressure);
//    u8SlowReleRunCount = 0;
//    g_bIsCalculateDrop = false;
//    g_bIsPumpStop = false;
//    NIBP_SetPWM(DISABLE);
    return RETURN_OK;
}

static RETURN_TypeDef nibp_staticPressureTest(const u8 *pBuf)
{
    if (!pBuf[0]) //disable static pressure test function
    {
        g_curWorkMode = g_backup_mode;
        NIBP_SetPressRelease(FAST_RELE);
        NIBP_DBG_INFO(">>>>>>>>>>>>>>>>Fast Rele 9");
        aio_stm32_setVerify(0);
    }
    else //enable function
    {
        g_NIBP_Status = DISABLE;
        g_bIsPumpStop = false;
        NIBP_SetPWM(DISABLE);
        NIBP_SetPressRelease(FAST_RELE);
        NIBP_DBG_INFO(">>>>>>>>>>>>>>>>Fast Rele 10");
        //Delay1ms(10);
        g_backup_mode = g_curWorkMode;
        g_curWorkMode = VENIPUNCTURE_TEST;
        NIBP_SetPressRelease(PRESS_HOLD);
        aio_stm32_setVerify(1);
        NIBP_DataReset();
        nibp_algorithm_start();
    }
    return RETURN_OK;
}

RETURN_TypeDef NIBP_DebugInterface(UartProtocolPacket *pPacket)
{
    RETURN_TypeDef result = RETURN_ERROR;
    if (0x01 == pPacket->DataAndCRC[0])//校准接口
    {
        result = NIBP_VerifyDebug(&(pPacket->DataAndCRC[1]),3);
    }
    else if (0x02 == pPacket->DataAndCRC[0])//当前值写入保存到EEPROM
    {
        result = NIBP_VerifySave2Eepom();
    }
    else if (0x03 == pPacket->DataAndCRC[0])//设置阶梯泄压过程，屏蔽上传的数据的个数（即上传TAG信息）
    {
        result = NIBP_SetUploadMaskCount(pPacket->DataAndCRC[1]);
    }
    else if (0x04 == pPacket->DataAndCRC[0])//算法调试接口
    {
        result = nibp_debug_start(&(pPacket->DataAndCRC[1]),6);
    }
    else if (0x05 == pPacket->DataAndCRC[0])//算法调试接口
    {
        g_curWorkMode = DEBUG_TEST;
        result = nibp_debug_start_pwm();
    }
    else if (0x06 == pPacket->DataAndCRC[0])//算法调试接口
    {
        result = nibp_debug_stop_pwm();
    }
    else if (0x07 == pPacket->DataAndCRC[0])//算法调试接口
    {
        result = nibp_debug_pass_step(&(pPacket->DataAndCRC[1]));//DataAndCRC[1]=step,[2:3]=time
    }
    else if (0x08 == pPacket->DataAndCRC[0])//算法调试接口
    {
        result = nibp_debug_algo_sw(&(pPacket->DataAndCRC[1]));
    }
    else if (0x09 == pPacket->DataAndCRC[0])//算法调试接口
    {
        result = nibp_debug_setStepCtl(&(pPacket->DataAndCRC[1]), 3);
    }
    else if (0x0A == pPacket->DataAndCRC[0])//算法调试接口
    {
        result = nibp_debug_StepRsingResult(&(pPacket->DataAndCRC[1]));
    }
    else if (0x0B == pPacket->DataAndCRC[0])
    {
        result = nibp_staticPressureTest(&(pPacket->DataAndCRC[1]));
    }
    
    return result;
}

/*******************************************************************************
* NIBP TEST or DEBUG API stop
*******************************************************************************/



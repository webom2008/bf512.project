/*
 * external_adc_com.c
 *
 *  Created on: 2013-10-17
 *      Author: QiuWeibo
 */
#include "external_adc_com.h"

TIM1_HANDLER_TypeDef g_Tim1_target;
COMADC_Channel_TypeDef  g_COMADC_channel;
char  g_COMADC_state;

void (*Tim1_HandlerSET[TIMER1_FUNC_LEN])(void);
void COM_ADC_SaveData(void);

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void COM_ADC_Init(void);
#pragma section("sdram0_bank3")
void COM_ADC_SCLK_IO_Init(void);
#pragma section("sdram0_bank3")
void COM_ADC_Interrupts_Init(void);
#pragma section("sdram0_bank3")
void Tim1_ConfigInit(void);
#pragma section("sdram0_bank3")
void Timer1_Interrupts(void);
#pragma section("sdram0_bank3")
void Tim1_HandlerSET_Init(void);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

/**
  * @brief  This function initializes the COM_ADC SCLK pin.
  *         setup PF1 as an output
  * @param  void
  * @retval void
  */
static void COM_ADC_SCLK_IO_Init(void)
{
    /*setup PH2 as an output*/
	*pPORTFIO_INEN &= ~COM_ADC_SCLK_PIN;    /* input buffer disable */
	*pPORTFIO_DIR |= COM_ADC_SCLK_PIN;      /* output */

	/* clear interrupt settings */
	*pPORTFIO_EDGE &= ~COM_ADC_SCLK_PIN;         // Level Sensitivity
    *pPORTFIO_MASKA_CLEAR = COM_ADC_SCLK_PIN;

    /* now clear the flag */
	COM_ADC_SCLK_LOW();
}

/* To make a pin function as an interrupt pin:
 * 1.PORTxIO_INEN must be set.
 * 2.PORTx_FER is typically cleared.
 * 3.configure PORTxIO_POLAR,PORTxIO_EDGE,PORTxIO_BOTH register.
 *
 * Note1: PORTxIO_INEN = 1 and Output mode,software can trigger a GPIO interrupt
 *        by writing to the data/set/toggle registers.The interrupt service 
 *        routine should clear the GPIO to acknowledge the request.
 */
void COM_ADC_DataPin_Init(ADCDataPinMode_TypeDef mode)
{
    *pPORTFIO_INEN |= COM_ADC_DATA_PIN;  //input buffer enable
    *pPORTFIO_DIR &= ~COM_ADC_DATA_PIN;  //input mode
    
    if (DIN_MODE == mode)   //normal input mode, not interrrupt.
    {
        *pPORTFIO_EDGE &= ~COM_ADC_DATA_PIN;         // Level Sensitivity
        *pPORTFIO_MASKA_CLEAR = COM_ADC_DATA_PIN;    //Disable Interrupt A
        *pPORTHIO_POLAR &= ~COM_ADC_DATA_PIN;        //High level = 1, Low level =0
    }
    else //DRDY_MODE enable interrupt
    {
        *pPORTFIO_EDGE |= COM_ADC_DATA_PIN;      //Edge Sensitivity
        *pPORTFIO_BOTH  &= ~COM_ADC_DATA_PIN;    //enabled single edge for edge-sensitivity
        *pPORTHIO_POLAR |= COM_ADC_DATA_PIN;        //active 1=falling 0=rising edge.
        *pPORTFIO_MASKA_SET = COM_ADC_DATA_PIN;     //Enable Interrupt A  
    }
}

/**
  * @brief  This function read the COM_ADC data.
  *         note:must read out data in DOUT Mode 348*tosc=348*(1/7.7647 MHz)=44.8us
  * @param  void
  * @retval void
  */
inline static unsigned long COM_ADC_ReadData(void)
{
    unsigned long ulRData = 0;
    unsigned long ulRegVal = 0;
    int i = 0;
    //-DEBUG140108-unsigned int uiTIMASK;
    //-DEBUG140108-uiTIMASK = cli();
    for (i = 0; i < 25; i++)
    {
        COM_ADC_SCLK_HIGH();//rising edge, can read data
        ulRegVal = *pPORTFIO;
        if (ulRegVal & COM_ADC_DATA_PIN)    //Din=1
        {
            ulRData |= 0x1;
        }
        //else    //Din = 0
        ulRData <<= 1;
        COM_ADC_SCLK_LOW();//falling edge, AD1254 shifted data internal. After 60ns Next Data ready.
    }
    //-DEBUG140108-sti(uiTIMASK);
    ulRData >>= 1; //纠正for循环多移位1次
    return ulRData;
}

static void COM_ADC_Interrupts_Init(void)
{
    //Step1:Enable the SIC(SIC_IMASK) interrupt
    unsigned long imask1 = *pSIC_IMASK1;
    imask1 |= (1 << 13);    //PF A interrupt
    *pSIC_IMASK1 = imask1;

    //Step2(Optionanl):Program the interrupt priority (SIC_IARx) 
    /* PF interrupt A bits 23:20 of IAR5, setup for  IVG9 : 2 */
    unsigned long iar5 = *pSIC_IAR5;
    iar5  &= ~(0x0F << 20);
    iar5  |= (2 << 20);
    *pSIC_IAR5 = iar5;

    //Step3:Set IVGx bit in the IMASK
    register_handler(ik_ivg9, IVG9_ISR);
}

/*****************************************************************************
 *
 *   Timer1 interface.
 * 
 ****************************************************************************/
void Timer1_SetInterrupt(const EnableOrDisable status)
{
    if (ENABLE == status)
    {
        *pTIMER_ENABLE = (1 << 1);
        *pSIC_IMASK1 |= (1 << 1);
    }
    else
    {
        *pTIMER_DISABLE = (1 << 1);
        *pSIC_IMASK1 &= ~(1 << 1);
    }
}

/*inline */void Timer1_Configure(const unsigned int n1us, const TIM1_HANDLER_TypeDef target)
{
    unsigned long sclk = getSystemCLK();
    unsigned long ratio = 0, freq = 0;

    Timer1_SetInterrupt(DISABLE);
    g_Tim1_target = target;
    
/*****************************************************************************
 *
 *   pwm_out mode, interrupt enable, count to end of period
 * 
 ****************************************************************************/
    *pTIMER1_CONFIG = 0x0059;

    freq = 1000000 / n1us;//1/(n*1us)=10^6/n Hz

    sclk += (freq /2);     //round up before divide by freq
    ratio = sclk / freq;
    
    *pTIMER1_PERIOD= ratio;
    *pTIMER1_WIDTH= (ratio / 2);    //timer width, 50%
    
    Timer1_SetInterrupt(ENABLE);
}

static void Timer1_Interrupts(void)
{
#ifdef _TIM1_TIM3_SAME_INTERRUPT_
    //Step1:Enable the SIC(SIC_IMASK) interrupt
    unsigned long imask1 = *pSIC_IMASK1;
    imask1 |= (1 << 1);    //Tmier1 interrupt
    *pSIC_IMASK1 = imask1;

    //Step2(Optionanl):Program the interrupt priority (SIC_IARx) 
    /* Timer1 bits 7:4 of IAR4, setup for  IVG10 : 3 */
    unsigned long iar4 = *pSIC_IAR4;
    iar4  &= 0xFFFFFF0F;
    iar4  |= 0x00000030;
    *pSIC_IAR4 = iar4;

    //Step3:Set IVGx bit in the IMASK
    register_handler(ik_ivg10, IVG10_ISR);//The Same
#else
    //Step1:Enable the SIC(SIC_IMASK) interrupt
    unsigned long imask1 = *pSIC_IMASK1;
    imask1 |= (1 << 1);    //Tmier1 interrupt
    *pSIC_IMASK1 = imask1;

    //Step2(Optionanl):Program the interrupt priority (SIC_IARx) 
    /* Timer1 bits 7:4 of IAR4, setup for  IVG13 : 6 */
    unsigned long iar4 = *pSIC_IAR4;
    iar4  &= 0xFFFFFF0F;
    iar4  |= 0x00000060;
    *pSIC_IAR4 = iar4;

    //Step3:Set IVGx bit in the IMASK
    register_handler(ik_ivg13, IVG13_ISR);//The Same

#endif
}

static void COM_ADC_SampleStart(void)
{
    COM_ADC_DataPin_Init(DRDY_MODE);//sample ADC
}

static void Tim1_HandlerSET_Init(void)
{
    Tim1_HandlerSET[TIM1_COM_ADC_R] = COM_ADC_SaveData;
    Tim1_HandlerSET[TIM1_COM_ADC_START] = COM_ADC_SampleStart;
}

static void Tim1_ConfigInit(void)
{
    Timer1_Interrupts();
    Tim1_HandlerSET_Init();
}

//S1:read Dout(unvalid) --> S2:change channel --> S3:read Dout(valid)
static void COM_ADC_SaveData(void)
{
    unsigned long ulRData;
    ulRData = COM_ADC_ReadData();
    //delay_us(25);//通过此替换COM_ADC_ReadData，结果表明最长耗时为900cycles，否则ECG_ADC出错
    
    if (0 == g_COMADC_state)
    {
        g_COMADC_state++;
        AHCT595_SetComADCChannel(g_COMADC_channel);
        Timer1_Configure(ADC_MUX_CHANGE_DELAY, TIM1_COM_ADC_START);//启动定时器
        return;
    }
    
    g_COMADC_state = 0;

    if (NIBP_channel == g_COMADC_channel )
    {
        g_pNIBP_dev->adc_val = ulRData;
        if (ENABLE == g_pNIBP_dev->verifyStauts)
        {
            g_pNIBP_dev->waitForADCStatus = DISABLE;
            return;
        }
        NIBP_adc_store_data();
        
#ifdef _USE_IBP_FUNCTION_
        if (true == IsIBPSample)
        {
            g_COMADC_channel = IBP1_channel;
            COM_ADC_DataPin_Init(DRDY_MODE);
        }
#endif //_USE_IBP_FUNCTION_
        if (true == IsTempNeedSample)
        {
            g_COMADC_channel = TEMP_channel;
            COM_ADC_DataPin_Init(DRDY_MODE);
        }
    } // end of if (NIBP_channel == g_COMADC_channel )
    
#ifdef _USE_IBP_FUNCTION_
    else if (IBP1_channel == g_COMADC_channel )
    {
        g_IBPadc_Val[0] = ulRData;
        g_COMADC_channel = IBP2_channel;
        COM_ADC_DataPin_Init(DRDY_MODE);
    }
    
    else if (IBP2_channel == g_COMADC_channel )
    {
        g_IBPadc_Val[1] = ulRData;
        IsIBPSample = false;
        if (true == IsTempNeedSample)//判断是否需要采集温度
        {
            g_COMADC_channel = TEMP_channel;
            COM_ADC_DataPin_Init(DRDY_MODE);
        }
        IBPReslultUpload();
    }
#endif//_USE_IBP_FUNCTION_

    else if (TEMP_channel == g_COMADC_channel )
    {
        if ((TEMP_CH1 == g_TempChannel) || (TEMP_COM == g_TempChannel))
        {
            g_TempADC_Val[0] = ulRData;
        }
        else//TEMP_COM_R or TEMP_CH2
        {
            g_TempADC_Val[1] = ulRData;
        }
        IsTempNeedSample = false; //完成采集，恢复状态
#ifdef _USE_IBP_FUNCTION_
        if (true == IsIBPSample)
        {
            g_COMADC_channel = IBP1_channel;
            COM_ADC_DataPin_Init(DRDY_MODE);
        }
#endif//_USE_IBP_FUNCTION_
        //Temp2采集完成，数据进行上传
        if ((TEMP_CH2 == g_TempChannel) || (TEMP_COM_R == g_TempChannel))
        {
            TempReslultUpload();
        }
    }
}

void COM_ADC_Init(void)
{
    AHCT595_SetComADCChannel(NIBP_channel);//must delay 2043.5*1/7.7647us=264us

    COM_ADC_SCLK_IO_Init();
    
    COM_ADC_DataPin_Init(DIN_MODE);
    COM_ADC_Interrupts_Init();

    Tim1_ConfigInit();
}

//主要有COM_ADC,PACE检测及处理
EX_INTERRUPT_HANDLER(IVG9_ISR)
{
    unsigned long ulPortF_Reg = *pPORTFIO;//*pPORTFIO_CLEAR;
    static char com_adc_interrupt_count = 0;
    //-DEBUG140108-unsigned int uiTIMASK;
    if (ulPortF_Reg & ECG_PACE_PIN)//pace signal falling happen
    {
        *pPORTFIO_CLEAR = ECG_PACE_PIN;  //clear flag
        g_pace_state.newState = true;    //检测到PACE信号
        //ECG_PACE_SetInterrupt(DISABLE);
    }

    else if (ulPortF_Reg & COM_ADC_DATA_PIN)//edge happen, wait for t2+t3=12*tosc(>1.6us) for data ready
    {
        *pPORTFIO_CLEAR = COM_ADC_DATA_PIN;  //clear flag
        //-DEBUG140108-uiTIMASK = cli();
        COM_ADC_DataPin_Init(DIN_MODE);
        Timer1_Configure(2, TIM1_COM_ADC_R);
        //-DEBUG140108-sti(uiTIMASK);
    }
    else
    {
        AIO_printf("\r\n IVG9_ISR>>>>>>>>>>>>>>>>>>>>>>>>>>>>Error!");
    }
}

#ifndef _TIM1_TIM3_SAME_INTERRUPT_
EX_INTERRUPT_HANDLER(IVG13_ISR)//for Timer1
{
    unsigned long timerStatus = *pTIMER_STATUS;
    //unsigned int uiTIMASK;
    if (timerStatus & TIMIL1)   //Timer1 interrupt happen.
    {
        *pTIMER_STATUS = TIMIL1;//Must clear IRQ
        Timer1_SetInterrupt(DISABLE);
        //uiTIMASK = cli();
        (*Tim1_HandlerSET[g_Tim1_target])(); 
        //sti(uiTIMASK);
    }
    else
    {
        AIO_printf("\r\nIVG13_ISR>>>>>>>>>>>>>>>>>>>>>>>>>>>>Error!");
    }
}
#endif



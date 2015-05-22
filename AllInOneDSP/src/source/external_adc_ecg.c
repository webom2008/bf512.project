/*
 * external_adc_ecg.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */
#include "external_adc_ecg.h"

#define ECG_ADC_CLOCK   8000000

#define ECG_ADC_SCLK_LOW()      (*pPORTHIO_CLEAR = PH2)
#define ECG_ADC_SCLK_HIGH()     (*pPORTHIO_SET = PH2)

ECGADC_Channel_TypeDef  g_ECGADC_channel;
char g_ECGADC_state;

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void ECG_ADC_Init(void);
#pragma section("sdram0_bank3")
void ECG_ADC_Select_IO_Init(void);
#pragma section("sdram0_bank3")
void ECG_ADC_SCLK_IO_Init(void);
#pragma section("sdram0_bank3")
void ECG_ADC_CLK_Init(void);
#pragma section("sdram0_bank3")
void ECG_ADC_Interrupts_Init(void);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

/**
  * @brief  This function initializes the ECG_ADC select pin.
  *         setup PF9 and PF10 as an output
  * @param  void
  * @retval void
  */
static void ECG_ADC_Select_IO_Init(void)
{
    /*setup PF9 and PF10 as an output*/
	*pPORTFIO_INEN &= ~PF9;			/* input buffer disable */
	*pPORTFIO_DIR |= PF9;			/* output */
	*pPORTFIO_INEN &= ~PF10;		/* input buffer disable */
	*pPORTFIO_DIR |= PF10;			/* output */

	/* clear interrupt settings */
	*pPORTFIO_EDGE &= ~PF9;         // Level Sensitivity
    *pPORTFIO_MASKA_CLEAR = PF9;
	*pPORTFIO_EDGE &= ~PF10;         // Level Sensitivity
    *pPORTFIO_MASKA_CLEAR = PF10;

    /* now clear the flag */
	*pPORTFIO_CLEAR = PF9;
	*pPORTFIO_CLEAR = PF10;
}

/**
  * @brief  This function initializes the ECG_ADC SCLK pin.
  *         setup PH2 as an output
  * @param  void
  * @retval void
  */
static void ECG_ADC_SCLK_IO_Init(void)
{
    /*setup PH2 as an output*/
	*pPORTHIO_INEN &= ~PH2;			/* input buffer disable */
	*pPORTHIO_DIR |= PH2;			/* output */

	/* clear interrupt settings */
	*pPORTHIO_EDGE &= ~PH2;         // Level Sensitivity
	*pPORTHIO_POLAR &= ~PH2;        //High level = 1, Low level =0
    *pPORTHIO_MASKA_CLEAR = PH2;

    /* now clear the flag */
	ECG_ADC_SCLK_LOW();
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
void ECG_ADC_DataPin_Init(ADCDataPinMode_TypeDef mode)
{
    *pPORTHIO_INEN |= PH1;  //input buffer enable
    *pPORTHIO_DIR &= ~PH1;  //input mode
    
    if (DIN_MODE == mode)   //normal input mode, not interrrupt.
    {
        *pPORTHIO_EDGE &= ~PH1;         // Level Sensitivity
        *pPORTHIO_MASKA_CLEAR = PH1;    //Disable PH1 Interrupt A
        *pPORTHIO_POLAR &= ~PH1;        //High level = 1, Low level =0
    }
    else //DRDY_MODE enable interrupt
    {
        *pPORTHIO_EDGE |= PH1;      //Edge Sensitivity
        *pPORTHIO_BOTH  &= ~PH1;    //enabled single edge for edge-sensitivity
        *pPORTHIO_POLAR |= PH1;    //active falling edge.
        *pPORTHIO_MASKA_SET = PH1;     //Enable PH1 Interrupt A  
    }
}

static void ECG_ADC_Interrupts_Init(void)
{
    //Step1:Enable the SIC(SIC_IMASK) interrupt
    unsigned long imask0 = *pSIC_IMASK0;
    imask0 |= (1 << 29);    //PH A interrupt
    *pSIC_IMASK0 = imask0;

    //Step2(Optionanl):Program the interrupt priority (SIC_IARx)
    /* PH interrupt A bits 23:20 of IAR3, setup for  IVG8 : 1 */
    unsigned long iar3 = *pSIC_IAR3;
    iar3  &= ~(0x0F << 20);
    iar3  |= (1 << 20);
    *pSIC_IAR3 = iar3;

    //Step3:Set IVGx bit in the IMASK
    register_handler(ik_ivg8, IVG8_ISR);
}

/**
  * @brief  This function select the channel of the ECG_ADC
  *         Note: must wait for 2043.5*tosc after mux change.
  * @param  void
  * @retval void
  */
void ECG_ADC_ChannelSelect(ECGADC_Channel_TypeDef channel)
{
    switch (channel)
    {
        case RESP2_channel:     //PF[10:9] = b#00
            *pPORTFIO_CLEAR = PF10;
            *pPORTFIO_CLEAR = PF9;
            break;
        case ECG1_channel:      //PF[10:9] = b#01
            *pPORTFIO_CLEAR = PF10;
            *pPORTFIO_SET = PF9;
            break;
        case ECG2_channel:      //PF[10:9] = b#10
            *pPORTFIO_SET = PF10;
            *pPORTFIO_CLEAR = PF9;
            break;
        case ECGV_channel:      //PF[10:9] = b#11
            *pPORTFIO_SET = PF10;
            *pPORTFIO_SET = PF9;
            break;
        default:
            break;
    }
}

/**
  * @brief  This function initializes the ECG_ADC CLK.
  *         note: 7.7647 MHz max @sclk = 132 MHz
  * @param  void
  * @retval void
  */
static void ECG_ADC_CLK_Init(void)
{
    unsigned long sclk = getSystemCLK();
    unsigned long ratio = 0;
    
    //configure PG4 IO as TMR5
    *pPORTG_FER |= PG4;         //enable PG4 peripheral function.
    *pPORTG_MUX &= ~(3<<4);     //set PG4 as TMR5 interface.mask bit[5:4]
    *pPORTG_MUX |= (2<<4);      //set bit[5:4]=b#10

   /*****************************************************************************
    *
    *   pwm_out mode, interrupt disable, count to end of period
    *   SCLK for counter
    * 
    ****************************************************************************/
    *pTIMER5_CONFIG = 0x0009;
    
    sclk += (ECG_ADC_CLOCK /2);     //round up before divide by ECG_ADC_CLOCK
    ratio = sclk / ECG_ADC_CLOCK;
    *pTIMER5_PERIOD = ratio;        //timer5 period = ratio * (1/132)us
    *pTIMER5_WIDTH = (ratio / 2);   //timer width, 50%

}

void ECG_ADC_CLK_enable(void)
{
    *pTIMER_ENABLE = (1 << 5);
}

void ECG_ADC_CLK_disable(void)
{
    *pTIMER_DISABLE = (1 << 5);
}

/**
  * @brief  This function read the ECG_ADC data.
  *         note:must read out data in DOUT Mode 348*tosc=348*(1/7.7647 MHz)=44.8us
  * @param  void
  * @retval void
  * @note   -QWB-all function total time(733 cycles) must to be less than  900 cycles,
  *         or will error with COM_ADC
  */
unsigned long ECG_ADC_ReadData(void)
{
    unsigned long ulRData = 0;
    unsigned long ulRegVal = 0;
    int i = 0;

    //-DEBUG140108-unsigned int uiTIMASK;
    //-DEBUG140108-uiTIMASK = cli();
    for (i = 0; i < 25; i++)
    {
        ECG_ADC_SCLK_HIGH();//rising edge, can read data
        ulRegVal = *pPORTHIO;
        if (ulRegVal & PH1)    //Din=1
        {
            ulRData |= 0x1;
        }
        //else    //Din = 0
        ulRData <<= 1;
        ECG_ADC_SCLK_LOW();//falling edge, AD1254 shifted data internal. After 60ns Next Data ready.
    }
    //-DEBUG140108-sti(uiTIMASK);
    ulRData >>= 2; //纠正for循环多移位1次+1(NOTE:result by RESP)
    return ulRData;
}

//S1:read Dout(unvalid) --> S2:change channel --> S3:read Dout(valid)
void ECG_ADC_SaveData(void)
{
    unsigned long ulRData = ECG_ADC_ReadData();

    if (0 == g_ECGADC_state)
    {
        g_ECGADC_state++;
        ECG_ADC_ChannelSelect(g_ECGADC_channel);
        Timer3_Configure(ADC_MUX_CHANGE_DELAY, TIM3_ECG_3Probe);
        return;
    }

    g_ECGADC_state = 0;
    if (TRUE == g_3probe_run)//直接ECG1->ECG_RESP
    {
        if (ECG1_channel == g_ECGADC_channel)//Handle ECG1 channel
        {
            g_ECGADC_channel = RESP2_channel;
            ECG_ADC_DataPin_Init(DRDY_MODE);
            g_3PorbeResult[g_3probe_channel] = ulRData;
            ECG_adc_store_data();
        }
        else if (RESP2_channel == g_ECGADC_channel)//Handle RESP channel
        {
            if (WORK_FLAG == p_resp_dev->status)
            {
                p_resp_dev->adc_val = ulRData;
                p_resp_dev->status = IDEL_FLAG;
                RESP_adc_store_data();
            }
        }  
    } //End of if (TRUE == g_3probe_run)
    else if (TRUE == g_5probe_run)//直接ECG1->ECG2->ECGV->ECG_RESP
    {
        if (ECG1_channel == g_ECGADC_channel)//Handle ECG1 channel
        {
            g_5PorbeResult[g_5probe_ECG1] = ulRData;
            g_ECGADC_channel = ECG2_channel;
            ECG_ADC_DataPin_Init(DRDY_MODE);
        }
        
        else if (ECG2_channel == g_ECGADC_channel)//Handle ECG2 channel
        {
            g_5PorbeResult[g_5probe_ECG2] = ulRData;
            g_ECGADC_channel = ECGV_channel;
            ECG_ADC_DataPin_Init(DRDY_MODE);
        }
        
        else if (ECGV_channel == g_ECGADC_channel)//Handle ECGV channel
        {
            g_5PorbeResult[3] = ulRData;
            
            //if (WORK_FLAG == p_resp_dev->status)
            {
                g_ECGADC_channel = RESP2_channel;
                ECG_ADC_DataPin_Init(DRDY_MODE);
            }
            
            ECG_adc_store_data();
        }

        else if (RESP2_channel == g_ECGADC_channel)//Handle RESP channel
        {
            if (WORK_FLAG == p_resp_dev->status)
            {
                p_resp_dev->adc_val = ulRData;
                p_resp_dev->status = IDEL_FLAG;
                RESP_adc_store_data();
            }
        }
    } //End of else if (TRUE == g_5probe_run)
}

void ECG_ADC_3Probe_Handler(void)
{
    ECG_ADC_DataPin_Init(DRDY_MODE);
}

void ADC_CLK_Init(void)
{
    ECG_ADC_CLK_Init();
    ECG_ADC_CLK_enable();
}

void ADC_Synchronize(void)
{
    ECG_ADC_SCLK_HIGH();
    COM_ADC_SCLK_HIGH();
    delay_us(500);      //4cycles <= 持续时间 <20cycles
    ECG_ADC_SCLK_LOW();
    COM_ADC_SCLK_LOW();
}

void ECG_ADC_Init(void)
{
    ECG_ADC_Select_IO_Init();
    ECG_ADC_ChannelSelect(ECG1_channel);//must delay 2043.5*1/7.7647us=264us
    ECG_ADC_SCLK_IO_Init();
    ECG_ADC_DataPin_Init(DIN_MODE);
    ECG_ADC_Interrupts_Init();
}

//IVG8_ISR:ECG_ADC_Din,ECG_ADC_TIM2(500Hz)
EX_INTERRUPT_HANDLER(IVG8_ISR)
{
    volatile long ulPortH_Reg = *pPORTHIO;//*pPORTHIO_CLEAR;
    volatile long timerStatus = *pTIMER_STATUS;
    static char ecg_adc_interrupt_count = 0;
    static char timer2_count = 0;
    //-DEBUG140108-unsigned int uiTIMASK;
    if (timerStatus & TIMIL2)   //Timer2 interrupt happen.
    {
        *pTIMER_STATUS = TIMIL2;    //Must clear IRQ
        timer2_count++;
        g_ECGADC_channel = ECG1_channel;
        g_ECGADC_state = 0;
        ECG_ADC_DataPin_Init(DRDY_MODE);
        if (timer2_count % 5 == 0)//10ms=100Hz
        {
            timer2_count = 0;
            NIBP_StartSample();
            RESP_StartSample();
        }
    }
    else if (ulPortH_Reg & PH1)//rising : DRDY=36*tosc(>4.8us) falling : t2+t3+t6=12tosc+30ns>2us
    {
        *pPORTHIO_CLEAR = PH1;  //Must clear IRQ
        //-DEBUG140108-uiTIMASK = cli();
        ECG_ADC_DataPin_Init(DIN_MODE);
        Timer3_Configure(2, TIM3_ECG_ADC_R);
        //-DEBUG140108-sti(uiTIMASK);
    }
    else
    {
        AIO_printf("\r\n IVG8_ISR>>>>>>>>>>>>>>>>>>>>>>>>>>>>Error!");
    }
}


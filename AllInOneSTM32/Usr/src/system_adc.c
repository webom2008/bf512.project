#include <string.h>
#include "system_adc.h"

#define ADC_DEBUG_INFO

#ifdef ADC_DEBUG_INFO
#define ADC_DBG_INFO(fmt, arg...) printf("\r\n[ADC] "fmt, ##arg)
#else
#define ADC_DBG_INFO(fmt, arg...)
#endif

#define ADC1_DR_ADDR    ((uint32_t)0x4001244C)

InternalADCResult g_SysADCResult;


/**
  * @brief  configure GPIO for ADC1
  * @param  None
  * @retval None
  *
  * @author QiuWeibo 2013.10.22
  */
static void SysADC_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1| GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;	
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

static void SysADC_RCC_Config(void)
{
    /* ADCCLK 12MHz Max */
//#ifdef SYSTEMCLOCK_20M
    RCC_ADCCLKConfig(RCC_PCLK2_Div2);//20MHz/2=10MHz
//#else
//"Error Please Check define SYSTEM_CLOCK"
//#endif

    /* Enable GPIOA clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    /* Enable GPIOB clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    /* Enable ADC1 clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    /* Enable DMA1 clock */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
}

/**
  * @brief  Init DMA1_CH1 for ADC1
  * @param  None
  * @retval None
  *
  * @author QiuWeibo 2013.10.22
  * @note   Datasheet Page149:Table 54. Summary of DMA1 requests for each channel
  */
static void SysADC_DMA_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    
    //DMA1 channel1 configuration
    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_ADDR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&g_SysADCResult;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = sizeof(InternalADCResult)/sizeof(u16);
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
}

/**
  * @brief  configure for ADC1
  * @param  None
  * @retval None
  *
  * @author QiuWeibo 2013.10.22
  */
static void SysADC_Config(void)
{
    ADC_InitTypeDef ADC_InitStructure;
    
    //ADC1 configuration
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = sizeof(InternalADCResult)/sizeof(u16);
    ADC_Init(ADC1, &ADC_InitStructure);

    //ADC1 regular channel configuration
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_55Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 2, ADC_SampleTime_55Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 3, ADC_SampleTime_55Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 4, ADC_SampleTime_55Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 5, ADC_SampleTime_55Cycles5);

    //Enable ADC1 DMA
    ADC_DMACmd(ADC1, ENABLE);

    //Enable ADC1
    ADC_Cmd(ADC1, ENABLE);

    //Enable ADC1 reset calibration register  
    ADC_ResetCalibration(ADC1);
    //Check the end of ADC1 reset calibration register
    while(ADC_GetResetCalibrationStatus(ADC1));

    //Start ADC1 calibration
    ADC_StartCalibration(ADC1);
    //Check the end of ADC1 calibration
    while(ADC_GetCalibrationStatus(ADC1));
}

static void SysADC_SetConvert(const FunctionalState state)
{
    if (ENABLE == state)
    {
        DMA_Cmd(DMA1_Channel1,ENABLE);//Enable DMA1 channel1
        ADC_Cmd(ADC1,ENABLE);
        ADC_SoftwareStartConvCmd(ADC1,ENABLE);//Start ADC1 Software Conversion
        //while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));//wait for DMA end
    }
    else
    {
        DMA_Cmd(DMA1_Channel1,DISABLE);
        ADC_SoftwareStartConvCmd(ADC1,DISABLE);
        ADC_Cmd(ADC1,DISABLE);
    }
}

void SystemADC_Init(void)
{
    memset(&g_SysADCResult, 0, sizeof(InternalADCResult));
    
    SysADC_GPIO_Config();
    SysADC_RCC_Config();
    
    SysADC_DMA_Init();
    SysADC_Config();

    SysADC_SetConvert(ENABLE);
}

void SystemADC_PrintResult(void)
{
#ifdef _PRINT_INTERNAL_ADC_
    static u32 u32NextChangeTime = 0;
    
    if(IsOnTime(u32NextChangeTime)) // is on time or over time
    {
        //printf("\r\n%d", g_SysADCResult.DET_NIBP);
        printf("\r\n0x%.4X 0x%.4X 0x%.4X 0x%.4X 0x%.4X",
            g_SysADCResult.DET_NIBP,
            g_SysADCResult.DET_D3V3E,
            g_SysADCResult.DET_VDDINT,
            g_SysADCResult.DET_5VAE,
            g_SysADCResult.DET_OVLD_OFF);
        u32NextChangeTime += getNextTimeByMS(10);
    }
#endif //_PRINT_INTERNAL_ADC_
}

int System_wait_ADC_Check(void)
{
    int timeout = 100;
    while((g_SysADCResult.DET_NIBP == 0) && (timeout))
    {
        timeout--;
        delay_ms(100);
    }
    
    if(timeout < 1)
    {
        ADC_DBG_INFO("System ADC Check Timeout!");
    }
    return 0;
}

int System_ADC_Check(void)
{
    System_wait_ADC_Check();
    return 0;
}

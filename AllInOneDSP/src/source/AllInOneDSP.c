/*****************************************************************************
 * AllInOneDSP.c
 *****************************************************************************/
#include "AllInOneDSP.h"


#pragma section("sdram0_bank3")
void System_Init(void);
#pragma section("sdram0_bank3")
void algorithm_init(void);
#pragma section("sdram0_bank3")
void Platform_Init(void);
#pragma section("sdram0_bank3")
void Boot_SelfCheck(void);
#pragma section("sdram0_bank3")
void SetInterrupt(void);

#pragma optimize_off//关闭优化语句
void delay_us(u32 usec);

/*******************************************************************************
** Function name:           delay_us
** Descriptions:            delay microsecond
** input parameters:        usec:microsecond to be delay
** output parameters:       none
** Returned value:          none
*******************************************************************************/
void delay_us(u32 usec)
{
    u32 i = 0;
    u32 j = 0;
    for(i=0;i<usec;i++)
    {
        for(j=0;j<1;j++);
    }
}

/******************************************************************************
** Function name:           delay_ms
** Descriptions:            delay millisecond
** input parameters:        msec:millisecond to be delay
** output parameters:       none
** Returned value:          none
*******************************************************************************/
void delay_ms(u32 msec)
{
    Delay1ms(msec);
}

/******************************************************************************
** Function name:           delay_s
** Descriptions:            delay seconds
** input parameters:        sec:seconds to be delay
** output parameters:       none
** Returned value:          none
*******************************************************************************/
void delay_s(u32 sec)
{
    u32 i = 0;
    for(i=0;i<sec;i++)
    {
        delay_ms(1000);
    }

}

static void System_Init(void)
{
    CLOCK_Init();
    SDRAM_Init();
    SFlash_Init();
    SysTick_Init();    
    Uart2MCU_Init(); 
    ExtWatchDog_Init();
    SystemTWI_Init();
}

static void algorithm_init(void)
{
    ecg_algorithm_init();
    resp_algorithm_init();
    nibp_algorithm_init();
}

static void Platform_Init(void)
{
    ADC_CLK_Init();
    EEPROM_Init();
    AHCT595_Init();
    ECG_Init();
    NIBP_Init();
    RESP_Init();
    Temperature_Init();
    SpO2_Module_Init();
    OnBoardSTM32_Module_Init();
    IBP_Init();
    ADC_Synchronize();
    //LED_Lamp_Init();
    algorithm_init();
    
    SysTick_Start();
    EEPROM_ExamineAll();//需要用到delay1ms函数，故需要再SysTick_Start()之后
    system_gpio_init();
    system_spi_init();
    system_sflash_init();
}

static void Boot_SelfCheck(void)
{
    //SDRAM_SelfCheck();
    SFlash_SelfCheck();
}

//初始化完成后，允许中断
static void SetInterrupt(void)
{
    ECG_SampleTimer_SetInterrupt(ENABLE);
    SpO2_UART_SetInterrupt();
    AIO_UART_SetInterrupt();
}


#ifdef _TEST_DATA_RANGE_
/******************************************************
sizeof(bool)			1 Bytes
sizeof(char)			1 Bytes
sizeof(short int)		2 Bytes
sizeof(int)			    4 Bytes
sizeof(long)			4 Bytes
sizeof(long int)		4 Bytes
sizeof(enum)			4 Bytes
sizeof(float)			4 Bytes
sizeof(double)			4 Bytes
sizeof(long double)		8 Bytes
******************************************************/
void printAllDataRange(void)
{
    AIO_printf("\r\n******************************************************");
    AIO_printf("\r\nsizeof(bool)\t\t\t%d Bytes", sizeof(bool));
    AIO_printf("\r\nsizeof(char)\t\t\t%d Bytes", sizeof(char));
    AIO_printf("\r\nsizeof(short int)\t\t%d Bytes", sizeof(short int));
    AIO_printf("\r\nsizeof(int)\t\t\t\t%d Bytes", sizeof(int));
    AIO_printf("\r\nsizeof(long)\t\t\t%d Bytes", sizeof(long));
    AIO_printf("\r\nsizeof(long int)\t\t%d Bytes", sizeof(long int));
    AIO_printf("\r\nsizeof(enum)\t\t\t%d Bytes", sizeof(TRUE));
    AIO_printf("\r\nsizeof(float)\t\t\t%d Bytes", sizeof(float));
    AIO_printf("\r\nsizeof(double)\t\t\t%d Bytes", sizeof(double));
    AIO_printf("\r\nsizeof(long double)\t\t%d Bytes", sizeof(long double));
    AIO_printf("\r\n******************************************************");
}
#endif

//#define _TEST_MAIN_LOOP_TIME_
#ifdef _TEST_MAIN_LOOP_TIME_
static u32 DebugMainLoopMaxTick(void)
{
    static u32 preTick = 0;
    static u32 delataTick = 0;
    u32 curTick = SysTick_Get();

    if (0 == preTick)
    {
        delataTick = 0;
    }
    else
    {
        delataTick = delataTick > (curTick-preTick)? delataTick :(curTick-preTick);

    }
    preTick = curTick;
    return delataTick;
}

static void DebugMainLoopHandler(void)
{
    static u32 u32NextTime = 0;
    static u32 u32MainLoopTimes = 0;
    u32 max = DebugMainLoopMaxTick();
    u32MainLoopTimes++;

    if(IsOnTime(u32NextTime))   // is on time or over time
    {
        AIO_printf("\r\n[Main]Tick:%ld,Loop:%ld,max:%ld", SysTick_Get(), u32MainLoopTimes,max);
        u32NextTime += getTickNumberByMS(1000);
    }
}
#endif

int main(void)
{
    System_Init();
    Platform_Init();
    Boot_SelfCheck();
    SetInterrupt();
    COMMON_Init(); //must put this at the end of init function.
    
    AIO_printf("\r\nAll In One Board. Compile Date:%s %s V0.6.8", __DATE__, __TIME__);
    AIO_printf("\r\nSystemClock VCO:%d CCLK:%d SCLK:%d", getVCOFreq(), getCodeCLK(), getSystemCLK());
#ifdef _TEST_DATA_RANGE_
    printAllDataRange();
#endif

    //ECG_CAL_DRV_ModeSet(ENABLE);//Just for Test.
    //EEPROM_TestInterface(_EEPROM_TEST_ALL_R_);

    while(1)
    {
        ExtWatchDog_Feed();
        
        ECG_Handler();
        ECG_CAL_DRV_signal();
        ECG_PrintResult();
        RESP_Handler();
        NIBP_Handler();
        Temperature_Handler();
        
        IBP_Handler();
        Probe_Detect_Handler();
        SpO2_Module_Handler();      //轮询检测SpO2是否发送消息
        Uart2MCU_RxPacketCheck();   //轮询检测MCU是否发送消息

        STM32_I2C_Handler();
        COMMON_Handler();
        
        //算法处理部分
        ecg_algorithm_handler();
        resp_algorithm_handler();
        nibp_algorithm_handler();
        
        system_power_server();
        system_check_server();
        //system_gpio_server();
        //system_spi_server();
        //system_sflash_server();
        
        //LED_Lamp_Work();
        //EEPROM_LoopTest();
        //STM32_I2C_Test();
#ifdef _TEST_MAIN_LOOP_TIME_
        DebugMainLoopHandler();
#endif
    }
}


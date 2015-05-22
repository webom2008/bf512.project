#include "main.h"

//#define _DEBUG_MAIN_LOOP_   // check how many main loop per second
#define _MAIN_DEBUG_INFO_

#ifdef _MAIN_DEBUG_INFO_
#define MAIN_DBG_INFO(fmt, arg...) printf("\r\n[MAIN] "fmt, ##arg)
#else
#define MAIN_DBG_INFO(fmt, arg...)
#endif


/*********************************************************************************************************
** Function name:           delay_us
** Descriptions:            delay microsecond
** input parameters:        usec:microsecond to be delay
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void delay_us(u32 usec)
{
    u32 i = 0;
    u32 j = 0;
    for(i=0;i<usec;i++)
    {
        for(j=0;j<1;j++);
    }
}

/*********************************************************************************************************
** Function name:           delay_ms
** Descriptions:            delay millisecond
** input parameters:        msec:millisecond to be delay
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void delay_ms(u32 msec)
{
    u32 u32delay;

    if(msec >= 0x80000000)  msec = 0x80000000 - 1;

    u32delay = SysTick_Get() + msec;

    while(SysTick_Compare(SysTick_Get(), u32delay) == -1) {};
}

/*********************************************************************************************************
** Function name:           delay_s
** Descriptions:            delay seconds
** input parameters:        sec:seconds to be delay
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void delay_s(u32 sec)
{
    u32 i = 0;
    for(i=0;i<sec;i++)
    {
        delay_ms(1000);
    }

}

#ifdef _DEBUG_MAIN_LOOP_
static void DebugMainLoopHandler(void)
{
    static u32 u32NextTime = 0;
    static u32 u32MainLoopTimes = 0;

    u32MainLoopTimes++;

    if(IsOnTime(u32NextTime))   // is on time or over time
    {
        printf("\r\nTick:%ld, Loop:%ld.", SysTick_Get(), u32MainLoopTimes);
        u32NextTime += getNextTimeByMS(1000);
    }
}
#endif

void System_Init(void)
{
    RCC_Configuration();    // system clock init
    System_NVIC_Config();
    SysTick_Start();        // start system tick
    watchdog_init();
    UART_Init();
    i2c_init();
    power_init();
    
    MAIN_DBG_INFO("SysCLCK: %d, HCLK: %d, PCLK1: %d, PCLK2: %d",
            g_RCC_Clocks.SYSCLK_Frequency,
            g_RCC_Clocks.HCLK_Frequency,
            g_RCC_Clocks.PCLK1_Frequency,
            g_RCC_Clocks.PCLK2_Frequency);
}

void Platform_Init(void)
{
    LED_Init();
    SystemADC_Init();
    NIBP_Init();
    ECG_Init();
}

void Boot_SystemCheck(void)
{
    Watchdog_Check();
    SystemCore_Check();
    System_ADC_Check();
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
    while (1)
    {
    }
}
#endif

void print_all_flash(void)
{
    uint32_t i = 0x08000000+0x4000; //start of user app
    uint32_t flashEnd = 0x08010000;
    printf("\r\n");
    for (; i < flashEnd;i++)
    {
        printf("%02X ",*(__IO uint8_t*) i);
        if (i % 100 == 0) printf("\r\n");
    }
    printf("\r\nprint_all_flash finished!");
}

/**
  * @brief  Main function.
  * @param  None
  * @retval None
  * @note   This function execute after SystemInit()-detail from startup_stm32f10x_md_vl.s
  */
int main(void)
{
    System_Init();
    Platform_Init();
    Boot_SystemCheck();
    
    printf("\r\nAll In One Board. Compile Date:%s %s V0.6.0", __DATE__, __TIME__);
    //print_all_flash();
    
    while(1)
    {
        watchdog_server(100);
        NIBP_Handler();
        ECG_DetectHandler();

        /***************** For Debug Interfaces ************/
#ifdef _DEBUG_MAIN_LOOP_
        DebugMainLoopHandler();
#endif
        LED_Handler();
        SystemADC_PrintResult();
        ECG_PrintResult();
        
        i2c_server();
        power_server();
    }
}


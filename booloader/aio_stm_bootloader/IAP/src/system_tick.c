#include <string.h>
#include "misc.h"
#include "commands.h"



/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO u32 u32SystemTick;
RCC_ClocksTypeDef g_RCC_Clocks;

/* Private functions ---------------------------------------------------------*/
u32 IsOnTime(const u32 u32Target);

// System Tick
void SysTick_Start(void)
{
    u32SystemTick = 0;

    //get system clock infomations.
    RCC_GetClocksFreq(&g_RCC_Clocks);
    
    /* Setup SysTick Timer for 1 msec interrupts.
       ------------------------------------------
      1. The SysTick_Config() function is a CMSIS function which configure:
         - The SysTick Reload register with value passed as function parameter.
         - Configure the SysTick IRQ priority to the lowest value (0x0F).
         - Reset the SysTick Counter register.
         - Configure the SysTick Counter clock source to be Core Clock Source (HCLK).
         - Enable the SysTick Interrupt.
         - Start the SysTick Counter.
      
      2. You can change the SysTick Clock source to be HCLK_Div8 by calling the
         SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8) just after the
         SysTick_Config() function call. The SysTick_CLKSourceConfig() is defined
         inside the misc.c file.
    
      3. You can change the SysTick IRQ priority by calling the
         NVIC_SetPriority(SysTick_IRQn,...) just after the SysTick_Config() function 
         call. The NVIC_SetPriority() is defined inside the core_cm3.h file.
    
      4. To adjust the SysTick time base, use the following formula:
                              
           Reload Value = SysTick Counter Clock (Hz) x  Desired Time base (s)
      
         - Reload Value is the parameter to be passed for SysTick_Config() function
         - Reload Value should not exceed 0xFFFFFF
     */
    SysTick_Config(g_RCC_Clocks.HCLK_Frequency / SysTickFreq);
}

void SysTick_Incremental(void)
{
    u32SystemTick++;
}

u32 SysTick_Get(void)
{
    return u32SystemTick;
}

s32 SysTick_Compare(u32 u32Tick1, u32 u32Tick2) // Interval must less than 0x80000000
{
    if(u32Tick1 == u32Tick2)
    {
        return 0;       // ==
    }
    else if(u32Tick1 < u32Tick2)
    {
        if((u32Tick2 - u32Tick1) < 0x80000000)
        {
            return -1;  // <
        }
        else            // Tick Out
        {
            return 1;   // >
        }
    }
    else    // u32Tick1 > u32Tick2
    {
        if((u32Tick1 - u32Tick2) < 0x80000000)
        {
            return 1;   // >
        }
        else            // Tick Out
        {
            return -1;  // <
        }
    }
}

void Delay1ms(u32 u32Delay) // Delay time should be fixed to less than 0x80000000
{
    u32 u32Target;

    if(u32Delay >= 0x80000000)  u32Delay = 0x80000000 - 1;

    u32Target = SysTick_Get() + u32Delay;

    while(SysTick_Compare(SysTick_Get(), u32Target) == -1) {};
}

u32 IsOnTime(const u32 u32Target) // 0: less than target time     1: on time or over time with target
{
    if(SysTick_Compare(SysTick_Get(), u32Target) == -1) // <
    {
        return 0;
    }
    return 1;
}

u32 IsOverTime(const u32 u32Base, const u32 u32Duration)    // 0: less than target time     1: over the target time
{
    u32 u32Target;

    u32Target = u32Base + u32Duration;

    if(SysTick_Compare(SysTick_Get(), u32Target) == 1)  // >
    {
        return 1;
    }

    return 0;
}

u32 getNextTimeByMS(const u32 N1ms)
{
    return (u32)((N1ms * SysTickFreq) / 1000);
}



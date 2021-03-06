/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SYSTEM_TICK_H
#define __SYSTEM_TICK_H

#ifdef __cplusplus
 extern "C" {
#endif 
//#define SysTickFreq    1000    // 1000Hz = 1ms
#define SysTickFreq    100    // 100Hz = 10ms

extern void SysTick_Start(void);        // System tick start running
extern void SysTick_Incremental(void);  // Only for SysTick_Handler()
extern u32 SysTick_Get(void);           // Get the current system time
extern void Delay1ms(u32 u32Delay);     // Delay time should be fixed to less than 0x80000000
extern u32 IsOnTime(const u32 u32Target);     // 0: less than target time     1: on time or over time with target
extern u32 IsOverTime(const u32 u32Base, const u32 u32Duration);    // 0: less than target time     1: over the target time

#ifdef __cplusplus
}
#endif

#endif /*__SYSTEM_TICK_H*/


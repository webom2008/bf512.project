/*
 * system_tick.h
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */

#ifndef SYSTEM_TICK_H_
#define SYSTEM_TICK_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

#define MAX_NUM_COUNTDOWN_TIMERS 5

extern void SysTick_Init(void);
extern void SysTick_Start(void);
extern void Delay1ms(const unsigned long ulMs);
extern void delay(const unsigned long count);
extern unsigned int SetTimeout(const unsigned long ulTicks);
extern bool ResetTimeout(const unsigned int nIndex, const unsigned long ulTicks);
extern unsigned long ClearTimeout(const unsigned int nIndex);
extern bool IsTimedout(const unsigned int nIndex);
extern bool IsOnTime(const unsigned long ulTarget);
extern bool IsOverTime(const unsigned long ulBase, unsigned long ulDuration);
extern unsigned long SysTick_Get(void);
extern unsigned long getTickNumberByMS(const unsigned long N1ms);
extern unsigned long getMSByTickcount(const unsigned long count);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_TICK_H_ */

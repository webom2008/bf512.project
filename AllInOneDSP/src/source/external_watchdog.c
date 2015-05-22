/*
 * external_watchdog.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */
#include "external_watchdog.h"


#define DSP_WATCHDOG_PIN    PF5
#define WATCHDOG_TIMEOUT_MS    800//900ms~2500ms

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void ExtWatchDog_Init(void);
#pragma section("sdram0_bank3")
void ExtWatchDog_IOInit(void);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

static void ExtWatchDog_IOInit(void)
{
	/* setup PF5 as an output */
	*pPORTFIO_INEN &= ~DSP_WATCHDOG_PIN;			/* disable */
	*pPORTFIO_DIR |= DSP_WATCHDOG_PIN;			/* output */

	/* clear interrupt settings */
	*pPORTFIO_EDGE &= ~DSP_WATCHDOG_PIN;
    *pPORTFIO_MASKA_CLEAR = DSP_WATCHDOG_PIN;

    /* now clear the flag */
	*pPORTFIO_CLEAR = DSP_WATCHDOG_PIN;
}

void ExtWatchDog_Init(void)
{
    ExtWatchDog_IOInit();
}

void ExtWatchDog_Reset(void)
{
    static int status = 0;
    
    if(status)
    {
        *pPORTFIO_SET = DSP_WATCHDOG_PIN;
    }
    else
    {
        *pPORTFIO_CLEAR = DSP_WATCHDOG_PIN;
    }
    
    //Pulse width min 100ns
    status = !status;
}

void ExtWatchDog_Feed(void)
{
    static unsigned long ulNextChangeTime = 0;
    
    if(IsOnTime(ulNextChangeTime)) // is on time or over time
    {
        ExtWatchDog_Reset();
        ulNextChangeTime += getTickNumberByMS(WATCHDOG_TIMEOUT_MS/2);
    }
}


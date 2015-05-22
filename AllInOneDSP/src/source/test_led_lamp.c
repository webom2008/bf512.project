/*
 * test_led_lamp.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */
#include "test_led_lamp.h"


#define TEST_LED_TIME   500 //overturn LED Pin is 500ms(1Hz).

void LED_Lamp_Init(void)
{
	/* setup PH6 as an output */
	*pPORTHIO_INEN &= ~PH6;			/* disable */
	*pPORTHIO_DIR |= PH6;			/* output */

	/* clear interrupt settings */
	*pPORTHIO_EDGE &= ~PH6;
    *pPORTHIO_MASKA_CLEAR = PH6;

    /* now clear the flag */
	*pPORTHIO_CLEAR = PH6;
}

void LED_Lamp_Work(void)
{
    static unsigned long ulNextChangeTime = 0;
    static int u8LEDStatus = 0;
    
    if(IsOnTime(ulNextChangeTime)) // is on time or over time
    {
        u8LEDStatus = !u8LEDStatus;
        if(u8LEDStatus)
        {
            *pPORTHIO_SET = PH6;    // turn off LED
        }
        else
        {
            *pPORTHIO_CLEAR = PH6;  // turn on LED
        }
        
        ulNextChangeTime += getTickNumberByMS(TEST_LED_TIME);//500ms
    }
}


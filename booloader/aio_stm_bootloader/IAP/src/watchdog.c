/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           watchdog.c
** Last modified Date:  2013-10-14
** Last Version:        V1.00
** Descriptions:        the specific codes for NIBP target boards
**                      User may modify it as needed
**--------------------------------------------------------------------------------------------------------
** Created by:          zhengxiaobo
** Created date:        2013-11-18
** Version:             V1.00
** Descriptions:        The original version
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-11-18
** Version:             V1.00
** Descriptions:        Add some comment
**
*********************************************************************************************************/
#include "includes.h"

//#define WATCHDOG_DEBUG_INFO

#ifdef WATCHDOG_DEBUG_INFO
#define WATCHDOG_DBG_INFO(fmt, arg...) printf("\r\n[WATCHDOG] "fmt, ##arg)
#else
#define WATCHDOG_DBG_INFO(fmt, arg...)
#endif


#define WDG_PORT        GPIOA
#define WDG_PIN         GPIO_Pin_4

/*********************************************************************************************************
** Function name:           watchdog_pin_init
** Descriptions:            watchdog_pin_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
void watchdog_pin_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* GPIOA Periph clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = WDG_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(WDG_PORT, &GPIO_InitStructure);  

}

/*********************************************************************************************************
** Function name:           watchdog_init
** Descriptions:            watchdog_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int watchdog_init(void)
{
    watchdog_pin_init();
    return 0;
}

/*********************************************************************************************************
** Function name:           watchdog_reset
** Descriptions:            watchdog_reset
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void watchdog_reset(void)
{
    static u8 WDog_Level = 0;
    if(!WDog_Level)
    {
        GPIO_SetBits(WDG_PORT,WDG_PIN);             /*Set WDG_PIN high level*/
    }
    else
    {
        GPIO_ResetBits(WDG_PORT,WDG_PIN);           /*Set WDG_PIN low level*/
    }
    WDog_Level = !WDog_Level;  
}


/*********************************************************************************************************
  END FILE
*********************************************************************************************************/




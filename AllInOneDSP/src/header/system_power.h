/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           power_manager.h
** Last modified Date:  2013-12-02
** Last Version:        V1.0
** Descriptions:        power_manager3¨¬D¨°
**
**--------------------------------------------------------------------------------------------------------
** Created by:          zhengxiaobo
** Created date:        2013-12-02
** Version:             V1.00
** Descriptions:        power_manager.c
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-12-02
** Version:             V1.00
** Descriptions:        power_manager.c
**
** Rechecked by:        
**--------------------------------------------------------------------------------------------------------
** Modified by:         
** Modified date:       
** Version:             
** Descriptions:        
**
** Rechecked by:        
*********************************************************************************************************/
#ifndef __POWER_H_
#define __POWER_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

#ifdef __SYSTEM_POWER_C_

typedef enum
{
    PWR_ACTIVE = 1,
    PWR_SLEEP,
    PWR_DEEP_SLEEP,
    PWR_HIBERNATE,
}POWER_MODE;


typedef enum
{
    PWR_DISABLE = 0,
    PWR_ENABLE = 1,
}FunctionalState;

extern int system_power_init(void);

extern int system_power_server(void);

extern int enter_low_power_mode(POWER_MODE mode,FunctionalState NewState);

#endif //__SYSTEM_POWER_C_

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __WATCHDOG_H_                    */

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

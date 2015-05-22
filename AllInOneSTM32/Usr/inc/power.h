/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           power.h
** Last modified Date:  2013-11-7
** Last Version:        V1.0
** Descriptions:        power程序
**
**--------------------------------------------------------------------------------------------------------
** Created by:          zhengxiaobo
** Created date:        2013-11-7
** Version:             V1.00
** Descriptions:        power函数
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-11-7
** Version:             V1.00
** Descriptions:        添加API函数
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

#include "aio_stm32_conf.h"

#ifdef __POWER_C__

typedef enum
{
    SLEEP,
    STOP,
    LPSTOP,
    STANDBY,
}POWER_MODE;

extern u8 g_power_status;

extern int power_init(void);
extern void power_server(void);
extern void enter_low_power_mode(POWER_MODE mode);
extern void enter_softwareUpdate(u8 *pBuf);

#endif /* __POWER_C__ */

#ifdef __cplusplus
}
#endif /*  __cplusplus                 */

#endif /*  __POWER_H__                    */

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

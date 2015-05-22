/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           watchdog.h
** Last modified Date:  2013-11-18
** Last Version:        V1.0
** Descriptions:        watchdog程序
**
**--------------------------------------------------------------------------------------------------------
** Created by:          zhengxiaobo
** Created date:        2013-11-18
** Version:             V1.00
** Descriptions:        watchdog函数
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-11-18
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
#ifndef __WATCHDOG_H_
#define __WATCHDOG_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "aio_stm32_conf.h"

#ifdef __WATCHDOG_C__

extern int watchdog_init(void);
extern void watchdog_server(const u16 time);
extern void Watchdog_Check(void);

#endif /* __WATCHDOG_C__ */

#ifdef __cplusplus
}
#endif /*  __cplusplus                 */

#endif /*  __WATCHDOG_H__                    */

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

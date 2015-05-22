/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           packet.h
** Last modified Date:  2013-11-18
** Last Version:        V1.0
** Descriptions:        packet程序
**
**--------------------------------------------------------------------------------------------------------
** Created by:          zhengxiaobo
** Created date:        2013-11-18
** Version:             V1.00
** Descriptions:        packet函数
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
#ifndef __PACKET_H_
#define __PACKET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "aio_stm32_conf.h"

#ifdef __PACKET_C__

extern int packet_init(void);
extern void packet_server(void);

#endif /* __PACKET_C__ */

#ifdef __cplusplus
}
#endif /*  __cplusplus                 */

#endif /*  __PACKET_H__                    */

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

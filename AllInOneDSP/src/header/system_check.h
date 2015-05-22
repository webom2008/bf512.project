/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           system_check.h
** Last modified Date:  2013-12-02
** Last Version:        V1.0
** Descriptions:        system_check
**
**--------------------------------------------------------------------------------------------------------
** Created by:          zhengxiaobo
** Created date:        2013-12-02
** Version:             V1.00
** Descriptions:        system_check
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-12-02
** Version:             V1.00
** Descriptions:        system_check
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
#ifndef __SYSTEM_CHECK_H_
#define __SYSTEM_CHECK_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

#ifdef __SYSTEM_CHECK_C_

typedef struct SYSTEM_STATE
{
    u8 power;
    u8 spo2;
    u8 dsp;
    u8 stm32;
    u8 ecg;
    u8 nibp;
    
}system_state;

extern int system_check_init(void);

extern int system_check_server(void);

extern int system_check(void);

#endif //__SYSTEM_CHECK_C_

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SYSTEM_CHECK_H_           */

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

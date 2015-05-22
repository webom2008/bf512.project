/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           system_sflash.h
** Last modified Date:  2013-12-02
** Last Version:        V1.0
** Descriptions:        system_check
**
**--------------------------------------------------------------------------------------------------------
** Created by:          qiuweibo
** Created date:        2013-09-12
** Version:             V1.00
** Descriptions:        system_sflash
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-10-21
** Version:             V1.00
** Descriptions:        system_sflash
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
#ifndef SYSTEM_SFLASH_H_
#define SYSTEM_SFLASH_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

#ifdef __SYSTEM_SFLASH_C_

typedef struct FLASH_DEVICE
{
    u8 *rx_buf;
    u8 *tx_buf;
    u16 rx_len;
    u16 tx_len;
    
}flash_device;

extern flash_device *pFlash_dev;


extern int system_sflash_init(void);
extern int system_sflash_server(void);
extern void SFlash_Init(void);
extern void SFlash_SelfCheck(void);
extern int system_flash_erase(u32 addr,u32 len);
extern int system_flash_write_buf(u32 addr,u8 *buf,u16 len);
extern int system_flash_read_buf(u32 addr,u8 *buf,u16 len);

#endif //__SYSTEM_SFLASH_C_

#ifdef __cplusplus
}
#endif

#endif                                                  /* SYSTEM_SFLASH_H_ */

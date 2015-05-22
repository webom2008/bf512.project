/**
  *(C) COPYRIGHT 2014 CVTE.SEECARE
  ******************************************************************************
  * @file    IAP/inc/main.h
  * @author  CVTE.SEECARE.QiuWeibo
  * @version V1.0.0
  * @date    2014/01/21
  * @brief   Header file for main.c
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "includes.h"

extern char g_bTryLoopRun;

/* Exported types ------------------------------------------------------------*/
typedef enum 
{
    NOEVENT = 0,
    EVENT_OPCOD_NOTYET_READ = 1,
    EVENT_OPCOD_READ =2
} EventStatus;

/* Exported constants --------------------------------------------------------*/
#define I2C_SLAVE_ADDRESS7     0x60

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void iap_i2c_loop(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/

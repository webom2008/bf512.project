/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SYSTEM_CHECK_H
#define __SYSTEM_CHECK_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "aio_stm32_conf.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
extern u8 Sys_Self_Check_Status;
/* Exported functions ------------------------------------------------------- */
extern void SystemCore_Check(void);

#ifdef __cplusplus
}
#endif

#endif /*__SYSTEM_CHECK_H*/


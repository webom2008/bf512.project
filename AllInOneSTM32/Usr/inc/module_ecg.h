/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MODULE_ECG_H
#define __MODULE_ECG_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "aio_stm32_conf.h"

extern u8 g_u8ecg_status;

extern void ECG_Init(void);
extern void ECG_DetectHandler(void);
extern void ECG_PrintResult(void);


#ifdef __cplusplus
}
#endif
#endif /*__MODULE_ECG_H*/


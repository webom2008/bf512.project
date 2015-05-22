/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SYSTEM_ADC_H
#define __SYSTEM_ADC_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "aio_stm32_conf.h"

extern InternalADCResult g_SysADCResult;

extern void SystemADC_Init(void);
extern void SystemADC_PrintResult(void);
extern int System_ADC_Check(void);

#ifdef __cplusplus
}
#endif
#endif /*__SYSTEM_ADC_H*/


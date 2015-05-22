/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TEST_LED_LAMP_H
#define __TEST_LED_LAMP_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "aio_stm32_conf.h"

extern void LED_Init(void);
extern void LED_Handler(void);

#ifdef __cplusplus
}
#endif
#endif /*__TEST_LED_LAMP_H*/


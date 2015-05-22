/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "aio_stm32_conf.h"

void delay_us(u32 usec);
void delay_ms(u32 msec);
void delay_s(u32 sec);

#ifdef __cplusplus
}
#endif

#endif /*__MAIN_H*/


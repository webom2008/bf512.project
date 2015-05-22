/*****************************************************************************
 * AllInOneDSP.h
 *****************************************************************************/

#ifndef ALLINONEDSP_H_
#define ALLINONEDSP_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

extern void delay_us(u32 usec);
extern void delay_ms(u32 msec);
extern void delay_s(u32 sec);

#ifdef __cplusplus
}
#endif

#endif /* ALLINONEDSP_H_ */

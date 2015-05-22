/*
 * system_sdram.h
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */

#ifndef SYSTEM_SDRAM_H_
#define SYSTEM_SDRAM_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

extern void SDRAM_Init(void);
extern void SDRAM_SelfCheck(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_SDRAM_H_ */

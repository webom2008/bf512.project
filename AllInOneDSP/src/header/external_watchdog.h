/*
 * external_watchdog.h
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */

#ifndef EXTERNAL_WATCHDOG_H_
#define EXTERNAL_WATCHDOG_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

extern void ExtWatchDog_Init(void);
extern void ExtWatchDog_Feed(void);
extern void ExtWatchDog_Reset(void);

#ifdef __cplusplus
}
#endif

#endif /* EXTERNAL_WATCHDOG_H_ */

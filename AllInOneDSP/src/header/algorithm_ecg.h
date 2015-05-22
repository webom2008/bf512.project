/*
 * algorithm_ecg.h
 *
 *  Created on: 2013-11-28
 *      Author: QiuWeibo
 */

#ifndef ALGORITHM_ECG_H_
#define ALGORITHM_ECG_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

extern void ecg_algorithm_init(void);
extern void ecg_algorithm_handler(void);
extern int ecg_algorithm_PopFilterResult(u32* pu32Tick, s32* ps32I, s32* ps32II, s32* ps32V, char* pPace);
extern void ecg_algorithm_DataReset(void);
extern int ecg_algorithm_getHR(u16 *pHR);
extern void ecg_algorithm_setRun(u8 state);

#ifdef __cplusplus
}
#endif

#endif /* ALGORITHM_ECG_H_ */


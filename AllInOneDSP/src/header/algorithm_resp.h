/*
 * algorithm_resp.h
 *
 *  Created on: 2013-12-24
 *      Author: QiuWeibo
 */

#ifndef ALGORITHM_RESP_H_
#define ALGORITHM_RESP_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

extern bool g_Is_resp_algo_run;

extern void resp_algorithm_init(void);
extern void resp_algorithm_handler(void);
extern int resp_algorithm_PopFilterResult(u32* pu32Tick, s32* ps32resp);
extern int resp_algorithm_getRR(u8 *pRR);
extern void resp_algorithm_setFilter(char mode);

#ifdef __cplusplus
}
#endif

#endif /* ALGORITHM_RESP_H_ */


/*
 * algorithm_nibp.h
 *
 *  Created on: 2013-12-27
 *      Author: QiuWeibo
 */

#ifndef ALGORITHM_NIBP_H_
#define ALGORITHM_NIBP_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

typedef enum
{ 
    NIBP_ALGO_IDLE,
    NIBP_ALGO_RISING,
    NIBP_ALGO_FALLING,
} NIBP_ALGO_STATE_TypeDef;

extern bool g_Is_nibp_algo_run;
extern bool g_nibp_algo_sw;
extern CNIBPResult *p_nibpResult;
extern NIBP_ALGO_STATE_TypeDef g_nibp_algo_state;

extern void nibp_algorithm_start(void);
extern void nibp_algorithm_init(void);
extern void nibp_algorithm_handler(void);
extern void nibp_algorithm_setPatient(char newType);
extern void nibp_algorithm_pushLastRet(int *pDP, int *pSP, int *pBPM);

#ifdef __cplusplus
}
#endif

#endif /* ALGORITHM_NIBP_H_ */


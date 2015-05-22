/*****************************************************************************
 * RESPAlgorithmV1000.h
 *****************************************************************************/
#ifndef RESP_ALGORITHM_V1000_H_
#define RESP_ALGORITHM_V1000_H_

#ifdef __cplusplus
     extern "C" {
#endif

#include "RESPAlgorithm_conf.h"

#if 1//_RESP_ALGORITHM_V1000_C_

extern CRESPOut* p_respAlgoOut;
extern CRESPIn* p_respAlgoIn;
extern CRESPAlertList resp_AlertList;

extern void RESPInitV1000(void);   // �㷨��ʼ��
extern void RESPRunV1000(void);    // �㷨������

#endif //_RESP_ALGORITHM_V1000_C_

#ifdef __cplusplus
}
#endif
 
#endif /* RESP_ALGORITHM_V1000_H_ */


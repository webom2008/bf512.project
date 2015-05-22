/*****************************************************************************
 * ECGAlgorithmV1000.h
 *****************************************************************************/
#ifndef ECG_ALGORITHMV1000_H_
#define ECG_ALGORITHMV1000_H_
 
#ifdef __cplusplus
extern "C" {
#endif 

#include "ECGAlgorithm_conf.h"

#define ECG_ALGORITHM_INPUT_FS          500
#define ECG_ALGORITHM_INPUT_MAX_TIME    60 // 输入缓冲大小
#define ECG_ALGORITHM_INPUT_MAX_LEN     (ECG_ALGORITHM_INPUT_MAX_TIME * ECG_ALGORITHM_INPUT_FS)

extern CECGOut* pST_DataOut;
extern CECGIn* pST_DataIn;
extern CAlert_List stAlertList;

extern void ECGAlgoInit(u32 FS);   // 算法初始化
extern void ECGAlgoRun(void);    // 算法主过程

extern int ECGV1000DebugInterface( ECG_ALG_DEBUG_TypeDef type,
                                    void* pBuf,
                                    unsigned char nLen);

#ifdef __cplusplus
}
#endif
 
#endif /* ECG_ALGORITHMV1000_H_ */


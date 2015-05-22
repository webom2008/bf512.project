/*****************************************************************************
 * Classifier.h
 *****************************************************************************/
#ifndef CLASSIFIER_H_
#define CLASSIFIER_H_
 
#ifdef __cplusplus
extern "C" {
#endif 

#include "ECGAlgorithm_conf.h"

extern int QuickRClassifier(int* pData, int len, int FS, float RHighThres, int *pIndex, int *pHigh);
extern int FullRClassifier(int* pData, int len, int FS, float RHighThres, int *pIndex, int *pHigh);
extern int PVCClassifier(int* pData, int len, int FS, int *pIndex);


#ifdef __cplusplus
}
#endif
 
#endif /* CLASSIFIER_H_ */


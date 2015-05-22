#ifndef _RESP_V1001_H_
#define _RESP_V1001_H_

#ifdef __cplusplus
     extern "C" {
#endif


//#include "stdint.h"
//#include "stdio.h"
//#include "../RESPAlgorithm.h"

#include "RESPAlgorithm_conf.h"

//namespace V1001 {

// 坐标点
//typedef struct Coordiate_Point {
//	int32_t u32X;
//	int32_t u32Y;
//} CCoordiate_Point;

// 三角波
//typedef struct Triangle_Form {
//	Coordiate_Point stLeft;
//	Coordiate_Point stPeak;
//	Coordiate_Point stRight;
//} CTriangle_Form;

extern void RespAlgoInit(void);

//uint32_t FindTriangleForm(int* pData, uint32_t nDataLen, int threshold, CTriangle_Form* pTFList, uint32_t nMaxLen);  // 找三角波

//float FcoeH(float x, float B, float C);

//float CalcRESPRate(CTriangle_Form* pTF, int nTFCount, int Fs);
//float CalcRESPRate1(float* pHisRespRate, int nHisRespRateCount, float r);

extern void RespAlgoRun(  void* pdataIn,
                int* pRESP_Cal_Buf,
                u32 cal_data_len,
                u32 *pResp_calc_data_num,
                int update_data_len,
                int algorithm_input_fs,
                void* pdataOut,
                CRESPAlertList *pStAlertList);
//void GetPara(float* paraList);

//void CheckAlert(CRESPAlertList *pStAlertList, UINT32 systick, int calDataLen, int Fs);

//float CalcRESPByWave(CTriangle_Form* pTF, int nTFCount, int Fs, int calDataLen);
//float CalcRESPByHisRESP(float* phisR, int &phisRCount, float curRESP, float updateTimeLen);
//}

extern int RespAlgoDebugInterface(RESP_ALG_DEBUG_TypeDef type,
                                char* pBuf,
                                unsigned char nLen);


#ifdef __cplusplus
}
#endif

#endif

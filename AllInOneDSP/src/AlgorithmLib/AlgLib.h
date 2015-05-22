
/*****************************************************************************
 * AlgLib.h
 *****************************************************************************/
#ifndef _ALGLIB_H_
#define _ALGLIB_H_
 
#ifdef __cplusplus
extern "C" {
#endif 

#include "ECGAlgorithm_conf.h"

#define my_max(a,b) ((a)>(b)?(a):(b))
#define my_min(a,b) ((a)<(b)?(a):(b))
#define my_abs(a) ((a)>0?(a):0-(a))

extern int GetDirection(int data);
extern void Diff_N(int* pDin, int nLen, int* pDout);
extern void Diff_F(float* pDin, int nLen, float* pDout);
extern int Mean_N(int* pDin, int nLen);
extern int Mean_ABS_N(int* pDin, int nLen);
extern float Mean_F(float* pDin, int nLen);
extern float Fix(float fIn);
extern int Min_Index_N(int* pt, int nLen, int* pIndex);
extern int Max_Index_N(int* pt, int nLen, int* pIndex);
extern int Max_N(int* pt, int nLen);
extern int Min_N(int* pt, int nLen);
extern int Sum_N(int* pt, int nLen);
extern void Abs_N(int* pt, int nLen);
extern int Max_Index_Abs_N(int* pt, int nLen, int* pIndex);
extern int Max_Abs_N(int* pt, int nLen);
extern void GetDelta(int* din, int len, int* dout);
extern void GetDeltaAbs(int* din, int len, int* dout);
extern void Zoom(int* din, int len, int Max, int* dout);
extern void ArrMulArr(int* din1, int* din2, int len, int* dout);
extern void ArrMulData(int* din1, int din2, int len, int* dout);
extern int IntCmp(const void *a , const void *b);
extern float Max_F(float* pt, int nLen);
extern float Min_F(float* pt, int nLen);
extern int GetMedian_N(int* Data, int DataLen, float cut);

#ifdef __cplusplus
}
#endif

#endif

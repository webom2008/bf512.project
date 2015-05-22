/*****************************************************************************
 * AlgorithmUtilities.h
 *****************************************************************************/
#ifndef ALGORITHM_UTILITIES_H_
#define ALGORITHM_UTILITIES_H_
 
#ifdef __cplusplus
extern "C" {
#endif 

// �����
typedef struct Coordiate_Point 
{
    long u32X;
    long u32Y;
} CCoordiate_Point;

// ���ǲ�
typedef struct Triangle_Form 
{
    CCoordiate_Point stLeft;
    CCoordiate_Point stPeak;
    CCoordiate_Point stRight;
} CTriangle_Form;

typedef struct 
{
    unsigned char max;
    unsigned char min;
} u8Limits_TypeDef;

typedef enum
{ 
    ECG_FILTER_SEL      = 1,
    ECG_ST_SW,              //ST�η�������
    ECG_ST_MEASURE_SETTING,
    ECG_ARRHYTHMIA_SW,      //����ʧ����������
    ECG_NOTCH_SELECT,
    ECG_PACE_SW,            //PACE����
    ECG_PACE_OVERSHOOT_SW,  //��������
    ECG_Tachycardia_Limit_SET,  //����
    ECG_Bradycardia_Limit_SET,  //����
} ECG_ALG_DEBUG_TypeDef;

typedef enum
{ 
    RESP_FILTER_SEL      = 1,
    RESP_SetHighBoundary,
    RESP_SetLowBoundary,
    RESP_SetSuffocationAlertDelay,
} RESP_ALG_DEBUG_TypeDef;

#ifdef __cplusplus
}
#endif
 
#endif /* ALGORITHM_UTILITIES_H_ */


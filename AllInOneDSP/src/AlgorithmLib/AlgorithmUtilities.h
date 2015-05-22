/*****************************************************************************
 * AlgorithmUtilities.h
 *****************************************************************************/
#ifndef ALGORITHM_UTILITIES_H_
#define ALGORITHM_UTILITIES_H_
 
#ifdef __cplusplus
extern "C" {
#endif 

// 坐标点
typedef struct Coordiate_Point 
{
    long u32X;
    long u32Y;
} CCoordiate_Point;

// 三角波
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
    ECG_ST_SW,              //ST段分析开关
    ECG_ST_MEASURE_SETTING,
    ECG_ARRHYTHMIA_SW,      //心率失常分析开关
    ECG_NOTCH_SELECT,
    ECG_PACE_SW,            //PACE开关
    ECG_PACE_OVERSHOOT_SW,  //过冲抑制
    ECG_Tachycardia_Limit_SET,  //过速
    ECG_Bradycardia_Limit_SET,  //过缓
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


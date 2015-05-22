/*****************************************************************************
 * NIBPAlgorithmV1000.h
 *****************************************************************************/
#ifndef NIBP_ALGORITHM_V1000_H_
#define NIBP_ALGORITHM_V1000_H_

#ifdef __cplusplus
     extern "C" {
#endif

#define STEP_RELE_ADC_TAG               (0x00FFFFFF) //阶梯泄压过程中，上传ADC的数值


#define NIBP_SAMPLE_FS                  100
#define NIBP_NEW_WINDOW_TIME            0.2
#define NIBP_NEW_WINDOW_LEN             ((int)(NIBP_NEW_WINDOW_TIME * NIBP_SAMPLE_FS))
#define NIBP_FILTER_WINDOW_TIME         4
#define NIBP_FILTER_WINDOW_LEN          (NIBP_FILTER_WINDOW_TIME * NIBP_SAMPLE_FS)
#define NIBP_STEEP_WINDOW_TIME          4
#define NIBP_STEEP_WINDOW_LEN           (NIBP_STEEP_WINDOW_TIME * NIBP_SAMPLE_FS)
#define NIBP_DATA_FILTER_SIZE           36
#define NIBP_FEATURE_FILTER_SIZE        86

#define MAX_PRESSURE_COEFFICIENT            0.65
#define MAX_PRESSURE_COEFFICIENT_NOBPM      0.90

#define QUEUE_S_LEN                         200
#define FEATURE_LEN                         1000

#define NIBP_PATIENT_ADULT      (1)
#define NIBP_PATIENT_CHILD      (2)
#define NIBP_PATIENT_NEWBORN    (3)

typedef struct _STEEP_UNIT
{
    short Xstart;
    int Ystart;
    short Xend;
    int Yend;
    short Mode;
    short eva;
} STEEP_UNIT;

typedef struct _QUEUE_S
{
    STEEP_UNIT pSteep[QUEUE_S_LEN];
    short nLen;
} QUEUE_S;

typedef struct _FEATURE_LIST
{
    int Pressure[FEATURE_LEN];
    int Energy[FEATURE_LEN];
    short nLen;
} FEATURE_LIST;

extern CNIBPOut *p_nibpAlgoOut;
extern CNIBPIn *p_nibpAlgoIn;
extern char g_nibp_patient_cur;

extern void NIBPInitV1000(void);   // 算法初始化
extern void NIBPRunV1000(void);    // 算法主过程


#ifdef __cplusplus
}
#endif
 
#endif /* NIBP_ALGORITHM_V1000_H_ */


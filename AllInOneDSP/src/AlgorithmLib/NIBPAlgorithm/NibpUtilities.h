/*****************************************************************************
 * NibpUtilities.h
 *****************************************************************************/
#ifndef NIBP_UTILITIES_H_
#define NIBP_UTILITIES_H_
 
#ifdef __cplusplus
extern "C" {
#endif

//******************************************************************************
//<!-- define ���� -->
//******************************************************************************
/*****************************************************************************
 * ��������LIB��Ŀ�������
 * ������__ADSPBF512__,������LIB����DSP;��֮,�����㷨ƽ̨
 *****************************************************************************/
//#define __ADSPBF512__ //DSP�����������궨�壬�����ֶ�define


/******************************************************************************/
//<!-- ������������ Begin -->
#ifndef __ADSPBF512__
typedef INT32   s32;
typedef INT16   s16;
typedef INT8    s8;

typedef UINT32  u32;
typedef UINT16  u16;
typedef UINT8   u8;
#else
#include <bfrom.h>
#endif

//<!-- ������������ End -->
/******************************************************************************/

/******************************************************************************/
//<!-- ����������ؽṹ�� Begin -->
// ��������ݽṹ
typedef struct NIBP_In 
{
    //u32 u32SysTick;
    u32 u32mmHgCur;
    u32 u32mmHgPre;
    int lastBPM;
    int lastDP;
    int lastSP;
    u16 u32mmHgRising;
    bool isOneStepDataReady;
} CNIBPIn;

//<!-- ����������ؽṹ�� End-->
/******************************************************************************/

/******************************************************************************/
//<!-- ���������ؽṹ�� Begin -->
typedef struct NIBPDataItem 
{
    //u32 u32SysTick;
    u32 *pData;
    u32 u32DataNum;
    u32 u32DataMax;
} CNIBPDataItem;

// ���
typedef struct NIBP_Result 
{
    int BPM;
    int DP;
    int SP;
    int MAP;
    int risingResult;
    int pulse_ms;
    float reliability;
    bool isAllDone;
    bool isRisingDone;
} CNIBPResult;

typedef struct NIBP_Out_Item 
{
    CNIBPDataItem stDataRaw;
    CNIBPDataItem stDataFilter;
} CNIBP_Out_Item;

typedef struct NIBP_Out 
{
    CNIBP_Out_Item data;
    CNIBPResult nibpResult;
} CNIBPOut;

//<!-- ���������ؽṹ�� End -->
/******************************************************************************/

#ifdef __cplusplus
}
#endif
 
#endif /* NIBP_UTILITIES_H_ */


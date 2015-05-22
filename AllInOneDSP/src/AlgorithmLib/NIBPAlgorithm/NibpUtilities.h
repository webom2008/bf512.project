/*****************************************************************************
 * NibpUtilities.h
 *****************************************************************************/
#ifndef NIBP_UTILITIES_H_
#define NIBP_UTILITIES_H_
 
#ifdef __cplusplus
extern "C" {
#endif

//******************************************************************************
//<!-- define 定义 -->
//******************************************************************************
/*****************************************************************************
 * 定义生成LIB的目标板类型
 * 若定义__ADSPBF512__,则生成LIB用于DSP;反之,用于算法平台
 *****************************************************************************/
//#define __ADSPBF512__ //DSP编译器环境宏定义，无需手动define


/******************************************************************************/
//<!-- 定义数据类型 Begin -->
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

//<!-- 定义数据类型 End -->
/******************************************************************************/

/******************************************************************************/
//<!-- 数据输入相关结构体 Begin -->
// 输入的数据结构
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

//<!-- 数据输入相关结构体 End-->
/******************************************************************************/

/******************************************************************************/
//<!-- 数据输出相关结构体 Begin -->
typedef struct NIBPDataItem 
{
    //u32 u32SysTick;
    u32 *pData;
    u32 u32DataNum;
    u32 u32DataMax;
} CNIBPDataItem;

// 结果
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

//<!-- 数据输出相关结构体 End -->
/******************************************************************************/

#ifdef __cplusplus
}
#endif
 
#endif /* NIBP_UTILITIES_H_ */


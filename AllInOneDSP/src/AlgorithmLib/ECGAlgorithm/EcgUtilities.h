/*****************************************************************************
 * EcgUtilities.h
 *****************************************************************************/
#ifndef ECG_UTILITIES_H_
#define ECG_UTILITIES_H_
 
#ifdef __cplusplus
extern "C" {
#endif 


#include "AlgorithmUtilities.h"


//******************************************************************************
//<!-- define 定义 -->
//******************************************************************************
/*****************************************************************************
 * 定义生成LIB的目标板类型
 * 若定义__ADSPBF512__,则生成LIB用于DSP;反之,用于算法平台
 *****************************************************************************/
//#define __ADSPBF512__ //DSP编译器环境宏定义，无需手动define

#define ECG_ALERT_MAX_NUM       10

/******************************************************************************/
//<!-- 定义数据类型 Begin -->
#ifdef __cplusplus
    #define __I volatile            /*!< defines 'read only' permissions      */
#else
    #define __I volatile const      /*!< defines 'read only' permissions      */
#endif
#define __O     volatile            /*!< defines 'write only' permissions     */
#define __IO    volatile            /*!< defines 'read / write' permissions   */

#ifndef __ADSPBF512__
typedef long int    s32;
typedef short int   s16;
typedef char        s8;

typedef unsigned long int   u32;
typedef unsigned short int  u16;
typedef unsigned char       u8;
#else
#include <bfrom.h>
#endif

typedef __IO long int   vs32;
typedef __IO short int  vs16;
typedef __IO char       vs8;

typedef __IO unsigned long int  vu32;
typedef __IO unsigned short int vu16;
typedef __IO unsigned char      vu8;

//<!-- 定义数据类型 End -->
/******************************************************************************/

/******************************************************************************/
//<!-- 数据输入相关结构体 Begin -->
typedef struct ECG_In {
	u32 u32SysTick;
	s32  s32ECG1;
	s32  s32ECG2;
	s32  s32ECG3;
    char pace;
} CECGIn;
//<!-- 数据输入相关结构体 End-->
/******************************************************************************/

/******************************************************************************/
//<!-- 数据输出相关结构体 Begin -->
typedef enum
{ 
    ASYSTOLE    = 0,    //停搏
    VFIB        = 1,    //室颤
    VTAC        = 2,    //室速
    VRONT       = 3,    //R on T (ROT)
    VRUN        = 4,    //三个或四个连发室早（RUN）
    VRHYTHM     = 5,    //室性节律
    VCOUPLET    = 6,    //二连发室早（CPT）
    VBIG        = 7,    //室早二联律（BGM）
    VTRIG       = 8,    //室早三联律（TGM）
    VMULTIFOCAL = 9,    //多源性PVC
    IRREGULAR   = 10,   //不规则节律
    TAC         = 11,   //室上性心动过速（TAC）
    BRD         = 12,   //心动过缓（BRD）
    NON_CAPTURE = 13,   //起搏器未俘获（PNC）
    NON_FIRE    = 14,   //起搏器未起搏（PNP）
    MISSING_QRS = 15,   //漏搏（MIS）
    LEARN       = 16,   //正在学习（LRN）
    NORMAL      = 17,   //正常（NML）
    NOISE       = 18,   //噪声信号（NOS）
    
    SIGLE_PVC,          //-TODO-单连发室早
} AlertType_TypeDef;

typedef struct Alert_Item   // 报警项
{
    AlertType_TypeDef type;
	u32 u32AlertIndex;
	u32 u32Time;
} CAlert_Item;

typedef struct Alert_List
{
	CAlert_Item alertList[ECG_ALERT_MAX_NUM];
	u32 u32AlertCount;
} CAlert_List;

typedef struct Label_Info   // 标注信息
{
	CTriangle_Form TF;
	s32 label;
	s32 curveIndex;
} CLabel_Info;

typedef struct Label_Info_List
{
	CLabel_Info* pList;
	u32 nLabelNum;
} CLabel_Info_List;


typedef struct ECG_Result   // 结果
{
    u32 u32HeartRate;
	u32 nHRConfidence;
	s32 nSTAmpDif[7];
	s32 nPVCCount;
	CAlert_List* pAlertList;
    u32 u32QuickQRSTick;
} CECGResult;

typedef struct ECG_Data_Item
{
	u32 u32SysTick;
	s32 s32ECG1;//I
	s32 s32ECG2;//II
	s32 s32ECG3;//V
	s32 s32ECG4;
	s32 s32ECG5;
	s32 s32ECG6;
	s32 s32ECG7;
    char pace;
} CECGDataItem;

typedef struct ECG_Out_Item
{ 
	CECGDataItem stDataRaw;
	CECGDataItem stDataFilter;
	CECGResult stResult;
} CECG_Out_Item;

typedef struct ECG_Out
{ 
	CECG_Out_Item* pData;
	u32 u32ECGDataNum;
    u32 u32ECGDataMax;
} CECGOut;
//<!-- 数据输出相关结构体 End -->
/******************************************************************************/

#ifdef __cplusplus
}
#endif
 
#endif /* ECG_UTILITIES_H_ */


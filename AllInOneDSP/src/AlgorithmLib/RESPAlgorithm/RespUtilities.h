/*****************************************************************************
 * RespUtilities.h
 *****************************************************************************/
#ifndef RESP_UTILITIES_H_
#define RESP_UTILITIES_H_
 
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

#define RESP_ALERT_MAX_NUM					10

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
typedef struct RESP_In 
{
    u32 u32SysTick;
    s32 s32RESP;
} CRESPIn;

//<!-- 数据输入相关结构体 End-->
/******************************************************************************/

/******************************************************************************/
//<!-- 数据输出相关结构体 Begin -->
// 三角波标号表
typedef enum 
{
    RESP_TFL_UNHNOWN = 0,   // 0 -- 未识别
    RESP_TFL_R = 1,         // 1 -- R波
} RESPTFLabel;


// 报警项
typedef enum
{ 
    High_Boundary,
    Low_Boundary,
    Suffocation,

} RESP_AlertType;

typedef struct RESP_Alert_Item 
{
    RESP_AlertType alertType;
    u32 u32AlertIndex;
    u32 u32Time;
} CRESPAlertItem;

typedef struct RESP_Alert_List 
{
    CRESPAlertItem alertList[RESP_ALERT_MAX_NUM];
    u32 u32AlertCount;
} CRESPAlertList;

typedef struct RESP_Data_Item 
{
    u32 u32SysTick;
    s32 s32RESP;
} CRESPDataItem;

// 结果
typedef struct RESP_Result 
{
    u32 u32RESPRate;
    CRESPAlertList* pAlertList;
} CRESPResult;

// 输出的数据结构
typedef struct RESP_Out_Item 
{
    CRESPDataItem stDataRaw;
    CRESPDataItem stDataFilter;
    CRESPResult stResult;
} CRESP_Out_Item;

typedef struct RESP_Out 
{
    CRESP_Out_Item* pData;
    s32 u32RESPDataNum;
    u32 u32RESPDataMax;
} CRESPOut;

//<!-- 数据输出相关结构体 End -->
/******************************************************************************/

#ifdef __cplusplus
}
#endif
 
#endif /* RESP_UTILITIES_H_ */


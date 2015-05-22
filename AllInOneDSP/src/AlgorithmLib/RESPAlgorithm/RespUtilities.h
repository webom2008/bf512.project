/*****************************************************************************
 * RespUtilities.h
 *****************************************************************************/
#ifndef RESP_UTILITIES_H_
#define RESP_UTILITIES_H_
 
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

#define RESP_ALERT_MAX_NUM					10

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
typedef struct RESP_In 
{
    u32 u32SysTick;
    s32 s32RESP;
} CRESPIn;

//<!-- ����������ؽṹ�� End-->
/******************************************************************************/

/******************************************************************************/
//<!-- ���������ؽṹ�� Begin -->
// ���ǲ���ű�
typedef enum 
{
    RESP_TFL_UNHNOWN = 0,   // 0 -- δʶ��
    RESP_TFL_R = 1,         // 1 -- R��
} RESPTFLabel;


// ������
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

// ���
typedef struct RESP_Result 
{
    u32 u32RESPRate;
    CRESPAlertList* pAlertList;
} CRESPResult;

// ��������ݽṹ
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

//<!-- ���������ؽṹ�� End -->
/******************************************************************************/

#ifdef __cplusplus
}
#endif
 
#endif /* RESP_UTILITIES_H_ */


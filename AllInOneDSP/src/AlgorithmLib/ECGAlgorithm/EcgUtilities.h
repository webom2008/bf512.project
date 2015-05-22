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
//<!-- define ���� -->
//******************************************************************************
/*****************************************************************************
 * ��������LIB��Ŀ�������
 * ������__ADSPBF512__,������LIB����DSP;��֮,�����㷨ƽ̨
 *****************************************************************************/
//#define __ADSPBF512__ //DSP�����������궨�壬�����ֶ�define

#define ECG_ALERT_MAX_NUM       10

/******************************************************************************/
//<!-- ������������ Begin -->
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

//<!-- ������������ End -->
/******************************************************************************/

/******************************************************************************/
//<!-- ����������ؽṹ�� Begin -->
typedef struct ECG_In {
	u32 u32SysTick;
	s32  s32ECG1;
	s32  s32ECG2;
	s32  s32ECG3;
    char pace;
} CECGIn;
//<!-- ����������ؽṹ�� End-->
/******************************************************************************/

/******************************************************************************/
//<!-- ���������ؽṹ�� Begin -->
typedef enum
{ 
    ASYSTOLE    = 0,    //ͣ��
    VFIB        = 1,    //�Ҳ�
    VTAC        = 2,    //����
    VRONT       = 3,    //R on T (ROT)
    VRUN        = 4,    //�������ĸ��������磨RUN��
    VRHYTHM     = 5,    //���Խ���
    VCOUPLET    = 6,    //���������磨CPT��
    VBIG        = 7,    //��������ɣ�BGM��
    VTRIG       = 8,    //���������ɣ�TGM��
    VMULTIFOCAL = 9,    //��Դ��PVC
    IRREGULAR   = 10,   //���������
    TAC         = 11,   //�������Ķ����٣�TAC��
    BRD         = 12,   //�Ķ�������BRD��
    NON_CAPTURE = 13,   //����δ����PNC��
    NON_FIRE    = 14,   //����δ�𲫣�PNP��
    MISSING_QRS = 15,   //©����MIS��
    LEARN       = 16,   //����ѧϰ��LRN��
    NORMAL      = 17,   //������NML��
    NOISE       = 18,   //�����źţ�NOS��
    
    SIGLE_PVC,          //-TODO-����������
} AlertType_TypeDef;

typedef struct Alert_Item   // ������
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

typedef struct Label_Info   // ��ע��Ϣ
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


typedef struct ECG_Result   // ���
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
//<!-- ���������ؽṹ�� End -->
/******************************************************************************/

#ifdef __cplusplus
}
#endif
 
#endif /* ECG_UTILITIES_H_ */


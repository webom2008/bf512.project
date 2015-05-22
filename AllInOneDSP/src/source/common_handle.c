/*
 * common_handle.c
 *
 *  Created on: 2013-11-22
 *      Author: QiuWeibo
 */
#include "common_handle.h"

#pragma section("sdram0_bank3")
PATIENT_TypeDef g_PatientType;
#pragma section("sdram0_bank3")
WorkMode_TypeDef g_WorkMode;
bool g_IsCommon_Upload;

const static u8 SOFTWARE_UPDATE_ASK[] = \
    {0xAA,0x99,0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00};
const static u8 SOFTWARE_UPDATE_ANSWER[] = \
    {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA};

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->
#pragma section("sdram0_bank3")
void COMMON_Init(void);
RETURN_TypeDef COM_PowerStatusUpLoad(void);
#pragma section("sdram0_bank3")
RETURN_TypeDef COM_GetSoftVersion(void);
#pragma section("sdram0_bank3")
RETURN_TypeDef COM_GetPatientType(void);
#pragma section("sdram0_bank3")
RETURN_TypeDef COM_SetPatientType(u8 newType);
#pragma section("sdram0_bank3")
RETURN_TypeDef COM_PatientTypeAction(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
RETURN_TypeDef COM_WorkModeAction(UartProtocolPacket *pPacket);
#pragma section("sdram0_bank3")
RETURN_TypeDef COM_GetSelfCheck(void);
#pragma section("sdram0_bank3")
RETURN_TypeDef COM_ResetEventUpLoad(char *pValue);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

static RETURN_TypeDef COM_PowerStatusUpLoad(void)
{
    char pValue[2];

    pValue[0] = 0;//SpO2 Power
    pValue[1] = 0;//AIO Power
    
#ifdef _UPLOAD_1HZ_DATA_
    if (true == g_IsCommon_Upload)
    {
        UploadDataByID(COM_TX_PowerStatus_ID, TRUE, pValue, sizeof(pValue));
    }
#endif

    return RETURN_OK;
}

static void COMMON_PowerHandler(void)
{
    static unsigned long ulNextChangeTime = 0;
    
    if(IsOnTime(ulNextChangeTime)) // is on time or over time
    {
        COM_PowerStatusUpLoad();
        ulNextChangeTime += getTickNumberByMS(1000);//1Hz
    }
}

RETURN_TypeDef COM_GetSoftVersion(void)
{
    char pValue[4];
    
    pValue[0] = DSP_SOFT_VERSION;//DSP Verison
    pValue[1] = 0;//STM32F100 Verion
    pValue[2] = 0;//SeeCare SpO2 Verison
    pValue[3] = 0;//Reserved

    UploadDataByID(COM_SOFTWARE_VERSION_ID, TRUE, pValue, sizeof(pValue));
    return RETURN_OK;
}

static RETURN_TypeDef COM_GetPatientType(void)
{
    char pValue[2];
    pValue[0] = 0;
    pValue[1] = (u8)g_PatientType;
    UploadDataByID(COM_PATIENT_TYPE_ID, TRUE, pValue, sizeof(pValue));
    return RETURN_OK;
}

static RETURN_TypeDef COM_SetPatientType(u8 newType)
{
    switch(newType)
    {
    case 1:
        g_PatientType = ADULT;
        resp_algorithm_setFilter(1);
        break;
    case 2:
        g_PatientType = CHILD;
        resp_algorithm_setFilter(1);
        break;
    case 3:
        g_PatientType = NEWBORN;
        resp_algorithm_setFilter(0);
        break;
    default:
        break;
    }
    NIBP_SetPatientType(newType);
    return RETURN_OK;
}

RETURN_TypeDef COM_PatientTypeAction(UartProtocolPacket *pPacket)
{
    RETURN_TypeDef result = RETURN_ERROR;
    if (0 == pPacket->DataAndCRC[0])//查询
    {
        result = COM_GetPatientType();
    }
    else//设置
    {
        result = COM_SetPatientType(pPacket->DataAndCRC[1]);
    }
    return result;
}

static RETURN_TypeDef COM_GetWorkMode(void)
{
    char pValue[2];
    pValue[0] = 0;
    pValue[1] = (u8)g_WorkMode;
    UploadDataByID(COM_PM_WORK_MODE_ID, TRUE, pValue, sizeof(pValue));
    return RETURN_OK;
}

static RETURN_TypeDef COM_SetWorkMode(u8 newType)
{
    switch(newType)
    {
    case 1:
        g_WorkMode = SURGICAL;
        break;
    case 2:
        g_WorkMode = MONITOR;
        ECG_NotchFilterHandle(0x03);
        break;
    case 3:
        g_WorkMode = DIAGNOSE;
        ECG_NotchFilterHandle(0);
        break;
    default:
        break;
    }
    
    //Filter Select Control interface.
    l_ecg_DebugInterface(ECG_FILTER_SEL, &newType, 1);
    return RETURN_OK;
}

RETURN_TypeDef COM_WorkModeAction(UartProtocolPacket *pPacket)
{
    RETURN_TypeDef result = RETURN_ERROR;
    if (0 == pPacket->DataAndCRC[0])//查询
    {
        result = COM_GetWorkMode();
    }
    else//设置
    {
        result = COM_SetWorkMode(pPacket->DataAndCRC[1]);
    }
    return result;
}

RETURN_TypeDef COM_GetSelfCheck(void)
{
    char pValue[7];

    //SpO2
    pValue[0] = 0;//Power
    pValue[1] = 0;//Info

    //AIO
    pValue[2] = 0;//Power
    pValue[3] = 0;//DSP BF512
    pValue[4] = 0;//STM32F100
    pValue[5] = 0;//ECG:ECG1/ECG2/ECGV
    pValue[6] = 0;//NIBP:NIBP/TEMP1/TEMP2/IBP1/IBP2
    
    UploadDataByID(COM_SELF_CHECK_ID, TRUE, pValue, sizeof(pValue));
    return RETURN_OK;
}

RETURN_TypeDef COM_ResetEventUpLoad(char *pValue)
{
    UploadDataByID(COM_TX_AbnormalReset_ID, TRUE, pValue, 1);
    return RETURN_OK;
}

void COMMON_Handler(void)
{
    COMMON_PowerHandler();
}

void COMMON_Init(void)
{
    g_IsCommon_Upload = true;
    
    COM_SetPatientType((u8)ADULT);
    COM_SetWorkMode((u8)DIAGNOSE);
}

void system_sf_reset(UartProtocolPacket *pPacket)
{
    if ((pPacket->Length == sizeof(SOFTWARE_UPDATE_ASK)) \
        && (0 == memcmp(&pPacket->DataAndCRC[0], &SOFTWARE_UPDATE_ASK[0], pPacket->Length)))
    {
        asm("nop;");
        //u32 bfrom_SysControl(u32 dActionFlags, ADI_SYSCTRL_VALUES *pSysCtrlSettings, void *reserved);
        //bfrom_SysControl(SYSCTRL_SYSRESET, NULL, NULL); /* either */
        bfrom_SysControl(SYSCTRL_SOFTRESET, NULL, NULL); /* or */
        return;
    }
}


/*
 * common_handle.h
 *
 *  Created on: 2013-11-22
 *      Author: QiuWeibo
 */

#ifndef COMMON_HANDLE_H_
#define COMMON_HANDLE_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

extern PATIENT_TypeDef g_PatientType;
extern bool g_IsCommon_Upload;

extern RETURN_TypeDef COM_GetSoftVersion(void);
extern RETURN_TypeDef COM_GetSelfCheck(void);
extern RETURN_TypeDef COM_PatientTypeAction(UartProtocolPacket *pPacket);
extern RETURN_TypeDef COM_WorkModeAction(UartProtocolPacket *pPacket);
extern RETURN_TypeDef COM_ResetEventUpLoad(char *pValue);
extern void COMMON_Handler(void);
extern void COMMON_Init(void);
extern void system_sf_reset(UartProtocolPacket *pPacket);

inline unsigned long s32_to_s24(long Data32)
{
    unsigned long data;
    if (Data32 & 0x80000000)//mask bit 32,negative
    {
        data = Data32 & 0x00FFFFFF;
        data |= 0x00800000;
    }
    else    //positive
    {
        data = Data32 & 0x007FFFFF;
    }
    
    return data;
}

inline long s24_to_s32(unsigned long Data24)
{
    long data;
    if (Data24 & 0x00800000)//mask bit 23,negative
    {
        Data24 = (~Data24 + 1) & 0x007FFFFF;//先转为正数值
        data =  0 - (long)Data24;//转为负值
        return data;
    }
    else    //positive
    {
        return (long)Data24;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* COMMON_HANDLE_H_ */


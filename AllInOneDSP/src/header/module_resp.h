/*
 * module_resp.h
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */

#ifndef MODULE_RESP_H_
#define MODULE_RESP_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

typedef struct RESP_DEVICE
{
    u32 adc_val;
    u32 tick;
    StatusFlag_TypeDef  status;
    UploadFormat_TypeDef upload_type;
    //int (*write)(struct NIBP_DEVICE *pDev);
    //int (*read)(struct NIBP_DEVICE *pDev);
    
} resp_device;

extern bool g_IsRESP_Upload;

extern resp_device * p_resp_dev;

extern void RESP_Init(void);
extern void RESP_Handler(void);
extern void RESP_adc_store_data(void);
extern void RESP_StartSample(void);
extern u16 RESP_PopAlgoResult(u32* pu32Tick, s32* pVal);
extern void RESP_AsphyxiaUpload(u8 RR);
extern void RESP_AlarmUpload(u8 type);

//<!-- For test interface start -->
extern void RESP_SetChannel(u8 channel);
extern RETURN_TypeDef RESP_setUploadDataType(UartProtocolPacket *pPacket);
extern RETURN_TypeDef RESP_setThreshold(UartProtocolPacket *pPacket);
extern RETURN_TypeDef RESP_setCarrierWave(UartProtocolPacket *pPacket);
extern RETURN_TypeDef RESP_setAsphyxiaTime(UartProtocolPacket *pPacket);
extern RETURN_TypeDef RESP_DebugInterface(UartProtocolPacket *pPacket);
//<!-- For test interface end -->

#ifdef __cplusplus
}
#endif

#endif /* MODULE_RESP_H_ */

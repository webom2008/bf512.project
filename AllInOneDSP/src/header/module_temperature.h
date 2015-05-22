/*
 * module_temperature.h
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */

#ifndef MODULE_TEMPERATURE_H_
#define MODULE_TEMPERATURE_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

extern unsigned long g_TempADC_Val[2];
extern bool IsTempNeedSample;
extern TEMP_Channel_TypeDef g_TempChannel;
extern bool g_IsTemp_Upload;

extern void Temperature_Init(void);
extern void Temperature_Handler(void);
extern void TempReslultUpload(void);
extern RETURN_TypeDef TEMP_DebugInterface(UartProtocolPacket *pPacket);

#ifdef __cplusplus
}
#endif

#endif /* MODULE_TEMPERATURE_H_ */

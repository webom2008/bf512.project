/*
 * module_spo2_uart.h
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */

#ifndef MODULE_SPO2_UART_H_
#define MODULE_SPO2_UART_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"


#define UPDATE_MASK_AIO_TIME_S (5)

extern unsigned char g_spo2_mask_count;
extern bool g_IsSpO2_Upload;
extern unsigned char g_is_aio_spo2_error;

extern void SpO2_UART_SetInterrupt(void);
extern void SpO2_Module_Init(void);
extern void SpO2_Module_Handler(void);
extern void SpO2_UART_SendBuf(char* p_chBuf, const unsigned int uiLength);

#ifdef __cplusplus
}
#endif

#endif /* MODULE_SPO2_UART_H_ */

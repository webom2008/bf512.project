/*
 * system_uart.h
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */

#ifndef SYSTEM_UART_H_
#define SYSTEM_UART_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

extern short UartSetBitrate(const unsigned char UartNum, const unsigned long UartBitrate);
extern void Uart2MCU_Init(void);
extern void AIO_UART_SetInterrupt(void);
extern void Uart2MCU_RxPacketCheck(void);
extern RETURN_TypeDef AIO_UART_GetByte(char* pValue);
extern void AIO_UART_SendByte(const char chByte);
extern void AIO_UART_SendBuf(char* p_chBuf, const unsigned int uiLength);
extern int AIO_printf(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_UART_H_ */

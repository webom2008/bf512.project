/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UART_H
#define __UART_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "aio_stm32_conf.h"

#define UART_1      1
#define UART_2      2
#define UART_3      3

#define UART_COM_SEL    UART_1

#if     (UART_COM_SEL == UART_1)
#define UART_COM                USART1
#define UART_COM_CLK            RCC_APB2Periph_USART1
#define UART_COM_GPIO_CLK       RCC_APB2Periph_GPIOA
#define UART_COM_RxGPIO         GPIOA
#define UART_COM_TxGPIO         GPIOA
#define UART_COM_RxPin          GPIO_Pin_10
#define UART_COM_TxPin          GPIO_Pin_9
#define UART_COM_IRQn           USART1_IRQn
#define UART_COM_IRQHandler     USART1_IRQHandler
#elif   (UART_COM_SEL == UART_2)
#define UART_COM                USART2
#define UART_COM_CLK            RCC_APB1Periph_USART2
#define UART_COM_GPIO_CLK       RCC_APB2Periph_GPIOA
#define UART_COM_RxGPIO         GPIOA
#define UART_COM_TxGPIO         GPIOA
#define UART_COM_RxPin          GPIO_Pin_3
#define UART_COM_TxPin          GPIO_Pin_2
#define UART_COM_IRQn           USART2_IRQn
#define UART_COM_IRQHandler     USART2_IRQHandler
#elif   (UART_COM_SEL == UART_3)
#define UART_COM                USART3
#define UART_COM_CLK            RCC_APB1Periph_USART3
#define UART_COM_GPIO_CLK       RCC_APB2Periph_GPIOB
#define UART_COM_RxGPIO         GPIOB
#define UART_COM_TxGPIO         GPIOB
#define UART_COM_RxPin          GPIO_Pin_11
#define UART_COM_TxPin          GPIO_Pin_10
#define UART_COM_IRQn           USART3_IRQn
#define UART_COM_IRQHandler     USART3_IRQHandler
#endif

extern void UART_Init(void);
extern void UART_SendByte(const u8 u8Byte);
extern void UART_SendBuf(u8* u8Buf, const u16 u16Length);
extern s8 UART_GetByte(u8* pValue);

#ifdef __cplusplus
}
#endif

#endif /*__UART_H*/


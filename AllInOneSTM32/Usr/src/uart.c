#include <string.h>
#include "stm32f10x_usart.h"

#include "uart.h"

#define UART_DEBUG_INFO
#ifdef UART_DEBUG_INFO
#define UART_DBG_INFO(fmt, arg...) printf("\r\n[UART] "fmt, ##arg)
#else
#define UART_DBG_INFO(fmt, arg...)
#endif

#define UART_COM_RxBUFSIZE  256
#define UART_COM_TxBUFSIZE  256

__IO u8 u8RxBuf[UART_COM_RxBUFSIZE];
u8 u8TxBuf[UART_COM_TxBUFSIZE];
__IO u16 u16RxIdxStart;
__IO u16 u16RxIdxEnd;
__IO u16 u16TxIdxStart;
u16 u16TxIdxEnd;
u8 u8IsOverBufLenght;

void UART_SendByte(const u8 u8Byte);
void UART_SendBuf(u8* u8Buf, const u16 u16Length);
void UART_RCC_Configuration(void);
void UART_GPIO_Configuration(void);

/******************************************************************************/
/*  UART Receive API                                                          */
/******************************************************************************/
static u8 IsRxBufEmpty(void)
{
    if (u16RxIdxEnd == u16RxIdxStart)
    {
        return 1;   // empty
    }
    else
    {
        return 0;       // not empty
    }
}

s8 UART_GetByte(u8* pValue)
{
    
    if (IsRxBufEmpty())   //u8RxBuf[] is empty
    {
        return BUFFER_EMPTY;
    }
    else
    {
        *pValue = u8RxBuf[u16RxIdxStart++];
        if (u16RxIdxStart >= UART_COM_RxBUFSIZE)
        {
            u16RxIdxStart = 0;    // Circular buffer
            u8IsOverBufLenght = FALSE;
        }
        UART_DBG_INFO("UART_GetByte = %.2x", *pValue);
        return BUFFER_NORMAL;
    }
}

/******************************************************************************/
/*  UART Send API                                                             */
/******************************************************************************/
static u8 IsTxBufFull(const u16 u16IdxEnd)
{
    if(((u16TxIdxStart == 0) && (u16IdxEnd == (UART_COM_TxBUFSIZE - 1)))
        || (u16TxIdxStart == (u16IdxEnd + 1)))
        return 1;   // full
    return 0;       // not full
}

void UART_SendByte(const u8 u8Byte)
{
    while(IsTxBufFull(u16TxIdxEnd)) {}; // wait
    u8TxBuf[u16TxIdxEnd] = u8Byte;
    if(u16TxIdxEnd < (UART_COM_TxBUFSIZE - 1))  u16TxIdxEnd++;
    else    u16TxIdxEnd = 0;    // Circular buffer
    USART_ITConfig(UART_COM, USART_IT_TXE, ENABLE); // Enable UART_COM Transmit interrupts
}

void UART_SendBuf(u8* u8Buf, const u16 u16Length)
{
    u16 u16IdxTemp, u16Len;

    u16IdxTemp = u16TxIdxEnd;
    u16Len = 0;
    while(u16Len < u16Length)
    {
        while(IsTxBufFull(u16IdxTemp)) {};  // wait
        u8TxBuf[u16IdxTemp] = u8Buf[u16Len++];
        u16IdxTemp++;
        if(u16IdxTemp >= UART_COM_TxBUFSIZE)    u16IdxTemp = 0; // Circular buffer
    }

    u16TxIdxEnd = u16IdxTemp;
    USART_ITConfig(UART_COM, USART_IT_TXE, ENABLE); // Enable UART_COM Transmit interrupts
}

/******************************************************************************/
/*  UART Init                                                                 */
/******************************************************************************/
void UART_Init(void)
{
    USART_InitTypeDef USART_InitStructure;

    // Empty Rx and Tx Buffer
    u16RxIdxStart = 0;
    u16RxIdxEnd = 0;
    u16TxIdxStart = 0;
    u16TxIdxEnd = 0;
    u8IsOverBufLenght = FALSE;
    memset((u8 *)u8RxBuf, 0, sizeof(u8RxBuf));
    memset(u8TxBuf, 0, sizeof(u8TxBuf));
    memset(&USART_InitStructure, 0, sizeof(USART_InitTypeDef));

    UART_RCC_Configuration();
    UART_GPIO_Configuration();

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    USART_Init(UART_COM, &USART_InitStructure);         // Configure UART_COM
    USART_ITConfig(UART_COM, USART_IT_RXNE, ENABLE);    // Enable UART_COM Receive interrupts
    USART_Cmd(UART_COM, ENABLE);                        // Enable the UART_COM

    UART_DBG_INFO("UART Init Ready.");
}

static void UART_RCC_Configuration(void)
{
    RCC_APB2PeriphClockCmd(UART_COM_GPIO_CLK | RCC_APB2Periph_AFIO, ENABLE);    // Enable GPIO clock
#if (UART_COM_SEL == UART_1)
    RCC_APB2PeriphClockCmd(UART_COM_CLK, ENABLE);   // Enable UART_COM Clock
#else
    RCC_APB1PeriphClockCmd(UART_COM_CLK, ENABLE);   // Enable UART_COM Clock
#endif
}

static void UART_GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    // Configure UART_COM Rx as input floating
    GPIO_InitStructure.GPIO_Pin = UART_COM_RxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(UART_COM_RxGPIO, &GPIO_InitStructure);

    // Configure UART_COM Tx as alternate function push-pull
    GPIO_InitStructure.GPIO_Pin = UART_COM_TxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(UART_COM_TxGPIO, &GPIO_InitStructure);
}

/******************************************************************************/
/*            STM32F10x Peripherals Interrupt Handlers                        */
/******************************************************************************/
void UART_COM_IRQHandler(void)
{
    if(USART_GetITStatus(UART_COM, USART_IT_RXNE) != RESET)
    {
        //USART_ClearITPendingBit(UART_COM, USART_IT_RXNE);
        UART_DBG_INFO("s=%d e=%d", u16RxIdxStart, u16RxIdxEnd);
        u8RxBuf[u16RxIdxEnd++] = USART_ReceiveData(UART_COM);   // Read one byte from the receive data register
        if(u16RxIdxEnd >= UART_COM_RxBUFSIZE)    // Circular buffer
        {
            u16RxIdxEnd = 0;
            u8IsOverBufLenght = TRUE;
        }
        if((TRUE == u8IsOverBufLenght) && (u16RxIdxEnd >= u16RxIdxStart))// Discard the last byte when overflow
        {
            u16RxIdxStart = u16RxIdxEnd + 1;
            if (u16RxIdxStart >= UART_COM_RxBUFSIZE) u16RxIdxStart = 0;
        }

        //Overflow happen, clear the flag ORE and RXNE.
        if(USART_GetFlagStatus(UART_COM, USART_FLAG_ORE) != RESET)
        {
            USART_ClearFlag(UART_COM, USART_FLAG_ORE);
            USART_ReceiveData(UART_COM);
        }
    }
  
    if(USART_GetITStatus(UART_COM, USART_IT_TXE) != RESET)
    {
        if(u16TxIdxStart == u16TxIdxEnd)    // Send buffer is empty
        {
            USART_ITConfig(USART1, USART_IT_TXE, DISABLE);      // Disable TXE Interrupt when not data to be send
        }
        else
        {
            USART_SendData(UART_COM, u8TxBuf[u16TxIdxStart]);   // Write one byte to the transmit data register
            // next
            u16TxIdxStart++;
            if(u16TxIdxStart >= UART_COM_TxBUFSIZE) u16TxIdxStart = 0;  // Circular buffer
        }
    }
}



/*
 * system_uart.c
 *  
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 *
 *  Note: communicate with control board by UART1
 */
#include <ccblkfn.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "system_uart.h"


#define UART1_BAUD_RATE 256000//230400

#define AIO_UART_COM_RxBUFSIZE  1024    //must less than SDRAM page size(1K Bytes)
#define AIO_UART_COM_TxBUFSIZE  2048    //must less than SDRAM page size(1K Bytes)

#pragma section("sdram0_bank3_page2")
char chAIORxBuf[AIO_UART_COM_RxBUFSIZE];
#pragma section("sdram0_bank3_page3")
char chAIOTxBuf[AIO_UART_COM_TxBUFSIZE];

unsigned int uiAIORxIdxStart;
unsigned int uiAIORxIdxEnd;
unsigned int uiAIOTxIdxStart;
unsigned int uiAIOTxIdxEnd;
bool bAIOIsOverBufLenght;

int AIO_printf(const char* fmt, ...);

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void Uart2MCU_Init(void);
#pragma section("sdram0_bank3")
void UART1_Init(void);
#pragma section("sdram0_bank3")
void Init_UART1_Interrupts(void);
#pragma section("sdram0_bank3")
short UartSetBitrate(const unsigned char UartNum, const unsigned long UartBitrate);
#pragma section("sdram0_bank3")
void AIO_UART_SetInterrupt(void);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

/**
  * @brief  This function initializes the UART as follow configuration.
  *         Bitrate         ->  UartBitrate
  *         Word Length     ->  8-bit word
  *         Stop Bits       ->  1 stop bit
  *         Parity status   ->  no parity
  * @param  UartNum: UART0 or UART1.
  * @param  UartBitrate: uart bit rate.
  * @retval Uart Divisor. If unsuccessful, a negative value is returned.
  */
short UartSetBitrate(const unsigned char UartNum, const unsigned long UartBitrate)
{
    unsigned short UartDivisor = 0;
    unsigned short UartMmrOffset = 0;
    unsigned long sclk = getSystemCLK();
	volatile int temp = 0;

    if (UartBitrate == 0) { return -1; }

    switch (UartNum) 
    {
        case 0: 
            UartMmrOffset = 0x0000; 
            break;
        case 1:
            UartMmrOffset = 0x1C00;
            break;
        default:
            return -1;
    }

    volatile unsigned short *pUartLcr  = (volatile unsigned short*) (UART0_LCR + UartMmrOffset);
    volatile unsigned short *pUartDlh  = (volatile unsigned short*) (UART0_DLH + UartMmrOffset);
    volatile unsigned short *pUartDll  = (volatile unsigned short*) (UART0_DLL + UartMmrOffset);
    volatile unsigned short *pUartGctl = (volatile unsigned short*) (UART0_GCTL + UartMmrOffset);
    volatile unsigned short *pUartRbr = (volatile unsigned short*) (UART0_RBR + UartMmrOffset);
    volatile unsigned short *pUartLsr = (volatile unsigned short*) (UART0_LSR + UartMmrOffset);

    UartDivisor = (sclk / UartBitrate);
    UartDivisor += 8; // round up before divide by 16
    UartDivisor >>= 4;
    
    /***************************************************************************
    *
    *  First of all, enable UART clock. 
    *
    ****************************************************************************/
    *pUartGctl = UCEN;

    *pUartLcr = DLAB;
    *pUartDlh = ((UartDivisor & 0xFF00) >> 8);
    *pUartDll = ( UartDivisor & 0x00FF);
    
    /*****************************************************************************
    *
    *   Clear DLAB again and set UART frame to 8 bits, no parity, 1 stop bit.
    *   (DLAB = 0)
    * 
    ****************************************************************************/
    *pUartLcr = 0x03;
    
    /*****************************************************************************
    *
    *  Finally enable interrupts inside UART module, by setting proper bits 
    *  in the IER register. It is good programming style to clear potential 
    *  UART interrupt latches in advance, by reading RBR, LSR and IIR. 
    * 
    ****************************************************************************/	
    temp = *pUartRbr;	
    temp = *pUartLsr;

    //IER register should be initialize.

    return UartDivisor;
}

/**
  * @brief  This function initializes the UART1 as follow configuration.
  *         Bitrate         ->  115200 bit/s
  *         Word Length     ->  8-bit word
  *         Stop Bits       ->  1 stop bit
  *         Parity status   ->  no parity
  */
static void UART1_Init(void)
{
    short UartDivisor = -1;
    
    *pPORTH_FER |= 0x00C0;  //PH6 PH7 enable peripheral function
    ssync();
    *pPORTH_MUX |= 0x0050;  //configure UART1 RX and UART1 TX pins
    ssync();

    UartDivisor = UartSetBitrate(1, UART1_BAUD_RATE);
    if (UartDivisor < 0)
    {
        //configure error happen.
    }
}

/**
  * @brief  This function initializes the UART1 interrupt.
  *         IVG10, mask Tx and Rx Interrupt.
  */
static void Init_UART1_Interrupts(void)
{
    //Step1:Enable the SIC(SIC_IMASK) interrupt
    unsigned long imask0 = *pSIC_IMASK0;
    imask0 |= (1 << 24);    //Rx interrupt
    imask0 |= (1 << 25);    //Tx interrupt
    *pSIC_IMASK0 = imask0;



    //Step2(Optionanl):Program the interrupt priority (SIC_IARx)
    /* Setup for  IVG12 : 5 */
    unsigned long iar3 = *pSIC_IAR3;
    iar3  &= 0xFFFFFF00;
    iar3  |= 0x00000055;    //Tx[7:4] and RX[3:0] interrupt
    *pSIC_IAR3 = iar3;

    //Step3:Set IVGx bit in the IMASK
    register_handler(ik_ivg12, IVG12_AIO_UART_ISR);
}

void AIO_UART_SetInterrupt(void)
{
    //ERBFI  0x01   Enable Receive Buffer Full Interrupt
    //ETBEI  0x02   Enable Transmit Buffer Empty Interrupt
    //ELSI   0x04   Enable RX Status Interrupt         
    *pUART1_IER = (ELSI|ERBFI);
}

/******************************************************************************/
/*  UART1 Receive API                                                          */
/******************************************************************************/
static int IsAIORxBufEmpty(void)
{
    if (uiAIORxIdxEnd == uiAIORxIdxStart)
    {
        return 1;   // empty
    }
    else
    {
        return 0;       // not empty
    }
}

RETURN_TypeDef AIO_UART_GetByte(char* pValue)
{
    
    if (IsAIORxBufEmpty())   //u8RxBuf[] is empty
    {
        return RETURN_ERROR;
    }
    else
    {
        *pValue = chAIORxBuf[uiAIORxIdxStart++];
        if (uiAIORxIdxStart >= AIO_UART_COM_RxBUFSIZE)
        {
            uiAIORxIdxStart = 0;    // Circular buffer
            bAIOIsOverBufLenght = false;
        }
        return RETURN_OK;
    }
}

/******************************************************************************/
/*  UART1 Send API                                                             */
/******************************************************************************/
static int IsAIOTxBufFull(const unsigned int uiIdxEnd)
{
    if(((uiAIOTxIdxStart == 0) && (uiIdxEnd == (AIO_UART_COM_TxBUFSIZE - 1)))
        || (uiAIOTxIdxStart == (uiIdxEnd + 1)))
    {
        return 1;   // full
    }
    return 0;       // not full
}

void AIO_UART_SendByte(const char chByte)
{
    while(IsAIOTxBufFull(uiAIOTxIdxEnd))
    {
        *pUART1_IER |= ETBEI; // Enable UART_COM Transmit interrupts
        //Delay1ms(1);
    }
    chAIOTxBuf[uiAIOTxIdxEnd] = chByte;
    uiAIOTxIdxEnd++;
    if(uiAIOTxIdxEnd >= AIO_UART_COM_TxBUFSIZE)    uiAIOTxIdxEnd = 0; // Circular buffer
    *pUART1_IER |= ETBEI;       // Enable UART_COM Transmit interrupts
}

void AIO_UART_SendBuf(char* p_chBuf, const unsigned int uiLength)
{
    unsigned int uiIdxTemp, uiLen;

    uiLen = 0;
    int count = 0x1000;
    while(uiLen < uiLength)
    {
        while(IsAIOTxBufFull(uiAIOTxIdxEnd) )//Bug.会一直停留在此，也不进入中断处理
        {
            *pUART1_IER |= ETBEI; // Enable UART_COM Transmit interrupts
            //Delay1ms(1);
            if (!(count--))
            {
                return;
            }
        }
        chAIOTxBuf[uiAIOTxIdxEnd++] = p_chBuf[uiLen++];
        if(uiAIOTxIdxEnd >= AIO_UART_COM_TxBUFSIZE)    uiAIOTxIdxEnd = 0; // Circular buffer
    }
    *pUART1_IER |= ETBEI; // Enable UART_COM Transmit interrupts
}

/*******************************************************************
*   Function:    EX_INTERRUPT_HANDLER(IVG12_AIO_UART_ISR)
*   Description: UART1_ISR
*******************************************************************/
EX_INTERRUPT_HANDLER(IVG12_AIO_UART_ISR)
{	
    volatile int uartLsr;
    //unsigned int uiTIMASK = cli();

    /************ >>>>>>>> Handler for UART1 Start <<<<<<<< *******************/
    uartLsr = *pUART1_LSR;  
    if (uartLsr & DR)//UART1 Receive ready, get data from UART1_RBR
    {
        chAIORxBuf[uiAIORxIdxEnd++] = *pUART1_RBR;   // Read one byte from the receive data register
        if(uiAIORxIdxEnd >= AIO_UART_COM_RxBUFSIZE)    // Circular buffer
        {
            uiAIORxIdxEnd = 0;
            bAIOIsOverBufLenght = true;
        }
        if((true == bAIOIsOverBufLenght) && (uiAIORxIdxEnd >= uiAIORxIdxStart))// Discard the last byte when overflow
        {
            uiAIORxIdxStart = uiAIORxIdxEnd + 1;
            if (uiAIORxIdxStart >= AIO_UART_COM_RxBUFSIZE) uiAIORxIdxStart = 0;
        }
    }
    else if (uartLsr & THRE)//UART1 Send empty, fill data into UART1_THR to send
    {
        if(uiAIOTxIdxStart == uiAIOTxIdxEnd)    // Send buffer is empty
        {
            *pUART1_IER &= ~ETBEI;      // Disable TXE Interrupt when not data to be send
        }
        else
        {
            *pUART1_THR = chAIOTxBuf[uiAIOTxIdxStart];   // Write one byte to the transmit data register
            uiAIOTxIdxStart++;
            if(uiAIOTxIdxStart >= AIO_UART_COM_TxBUFSIZE) uiAIOTxIdxStart = 0;  // Circular buffer
        }
    }
    /************ >>>>>>>> Handler for UART1 Stop <<<<<<<< *******************/
    //sti(uiTIMASK);
}

void Uart2MCU_Init(void)
{
    uiAIORxIdxStart = 0;
    uiAIORxIdxEnd = 0;
    uiAIOTxIdxStart = 0;
    uiAIOTxIdxEnd = 0;
    bAIOIsOverBufLenght = false;

    UART1_Init();
    Init_UART1_Interrupts();
}

void Uart2MCU_RxPacketCheck(void)
{
    Uart_RxPacketCheck();
}

int AIO_printf(const char* fmt, ...)
{
    char czBuffer[1024];
    va_list argptr;
    int nLen = 0;

    memset(czBuffer, 0, sizeof(czBuffer));
    va_start(argptr, fmt);
    vsnprintf(czBuffer, sizeof(czBuffer), fmt, argptr);
    va_end(argptr);
    
    nLen = strlen(czBuffer);
    AIO_UART_SendBuf(czBuffer, nLen);
    //SpO2_UART_SendBuf(czBuffer, nLen);
    
    return nLen;
}


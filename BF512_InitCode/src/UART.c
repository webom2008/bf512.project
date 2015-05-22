/*****************************************************************************
 Include Files
******************************************************************************/

#include "init_platform.h"

/*****************************************************************************
 Symbolic constants / definitions
******************************************************************************/

#if defined __ADSPBF50x__
#define UART0_MMR_OFFSET    0x0000
#define UART1_MMR_OFFSET    0x1C00
#endif

#if defined __ADSPBF51x__
#define UART0_MMR_OFFSET    0x0000
#define UART1_MMR_OFFSET    0x1C00
#endif

#if defined __ADSPBF52x__
#define UART0_MMR_OFFSET    0x0000
#define UART1_MMR_OFFSET    0x1C00
#endif

#if defined __ADSPBF54x__
#define UART0_MMR_OFFSET    0x0000
#define UART1_MMR_OFFSET    0x1C00
#define UART2_MMR_OFFSET    0x1D00
#define UART3_MMR_OFFSET    0x2D00
#endif

#if defined __ADSPBF59x__
#define UART0_MMR_OFFSET    0x0000
#define UART1_MMR_OFFSET    0x0000 // Only one UART interface available
#endif


/*****************************************************************************
 Functions
******************************************************************************/


//***************************************
//*
//* Function Name : short UartPutc(unsigned char UartNum, char c)
//* Description   : This function transmits a character
//*                 by polling THRE bit in the LSR register.
//*
//* Parameters    : The character to transmit, UART number
//* Returns       : NULL. If the UartPutc function is unsuccessful, a negative value is returned.
//* Globals       : none
//*
short UartPutc(unsigned char UartNum, char c)
{
    unsigned short UartMmrOffset = 0;

    switch (UartNum) {
        case 0: UartMmrOffset = UART0_MMR_OFFSET; break;
        case 1: UartMmrOffset = UART1_MMR_OFFSET; break;
        default: asm("EMUEXCPT;"); while(1);
    }

    volatile unsigned short *pUartLsr = (volatile unsigned short*) (UART0_LSR + UartMmrOffset);
    volatile unsigned short *pUartThr = (volatile unsigned short*) (UART0_THR + UartMmrOffset);

    while (!(*pUartLsr & THRE)) { /* wait */ }
    *pUartThr = c;

    return 0;
}



#if (__ADSPBF50x__ == 1) || (__ADSPBF54x__ == 1)


/****************************************************************************
 Name:          UartGetBitrate
 Description:   get current UART Bit Rate
 Input:         u8 UartNum
 Return:        u8 UartBitrate
****************************************************************************/

u32 UartGetBitrate(u8 UartNum)
{
    u16 UartMmrOffset = 0;
    u32 UartBitrate = 0;

    switch (UartNum) {
        case 0: UartMmrOffset = UART0_MMR_OFFSET; break;
        case 1: UartMmrOffset = UART1_MMR_OFFSET; break;
        default: asm("EMUEXCPT;"); while(1);
    }

    volatile u16 *pUartGctl = (volatile u16*) (UART0_GCTL + UartMmrOffset);
    volatile u16 *pUartDlh  = (volatile u16*) (UART0_DLH  + UartMmrOffset);
    volatile u16 *pUartDll  = (volatile u16*) (UART0_DLL  + UartMmrOffset);

    UartBitrate = ( get_sclk_hz() / ( (*pUartDlh << 8) | *pUartDll ) );
    if (!(*pUartGctl & EDBO)) {
        UartBitrate += 8;   /* round up before divide by 16 */
        UartBitrate >>= 4;
    }
    return UartBitrate;
}


/****************************************************************************
 Name:          UartRxFifoClear

 Description:   check Receive FIFO Count Status
                wait for Receive FIFO to be filled and wait for at least one word
                send status feedback to host
 Input:         u8 UartNum, u8 UartBitrate
 Return:        u16 UartMcr -> Restore value for UART_MCR register
****************************************************************************/

u16 UartRxFifoClear(u8 UartNum, u32 UartBitrate)
{
    u16 UartMmrOffset = 0;
    u16 UartMcr = 0;

    switch (UartNum) {
        case 0: UartMmrOffset = UART0_MMR_OFFSET; break;
        case 1: UartMmrOffset = UART1_MMR_OFFSET; break;
        default: asm("EMUEXCPT;"); while(1);
    }

    volatile u16 *pUartThr = (volatile u16*) (UART0_THR + UartMmrOffset);
    volatile u16 *pUartLsr = (volatile u16*) (UART0_LSR + UartMmrOffset);
    volatile u16 *pUartMsr = (volatile u16*) (UART0_MSR + UartMmrOffset);
    volatile u16 *pUartMcr = (volatile u16*) (UART0_MCR + UartMmrOffset);


    UartMcr = *pUartMcr;    /* save UART_MCR register */
    *pUartMcr &= ~ARTS;     /* clear ARTS bit */
    *pUartMcr &= ~MRTS;     /* clear MRTS bit -> de-assert RTS signal */


    /*****************************************************************
     Especially in half duplex mode it may be necessary to wait here.
     A delay loop with the length of one data word ensures that the
     host has finished sending the very last word after RTS/hostwait
     has been deasserted.
     IMPORTANT: If the host is waiting to send the last data word
     than this delay may not be enough. The programmer has to take
     care about this!
    *****************************************************************/

//    for (8*UartBitrate ; UartBitrate > 0 ; UartBitrate--) { asm("nop;"); }


    /**************************************************************
     Signal Receive Buffer Status to the host before changing PLL.
     Four bytes are transmitted back to the host:
        0xBF
        UARTx_MSR
        UARTx_LSR
        0x00 to terminate string
    **************************************************************/

    UartPutc(UartNum,0xBF);
    UartPutc(UartNum,*pUartMsr);
    UartPutc(UartNum,*pUartLsr);
    UartPutc(UartNum,0x00);
    while( (*pUartLsr & TEMT) == 0 ) { /* wait */ }


    return UartMcr;
}


/****************************************************************************
 Name:          UartSetBitrate
 Description:   calculate and set UART Divisor latch registers
                UART0_DLH & UART0_DLL that fits former Bit Rate
                and new system clock
                send status feedback to host
 Input:         u8 UartNum, u8 UartBitrate
 Return:        -
*****************************************************************************/

void UartSetBitrate(u8 UartNum, u32 UartBitrate)
{
    u16 UartMmrOffset = 0;
    u16 UartDivisor = 0;

    switch (UartNum) {
        case 0: UartMmrOffset = UART0_MMR_OFFSET; break;
        case 1: UartMmrOffset = UART1_MMR_OFFSET; break;
        default: asm("EMUEXCPT;"); while(1);
    }

    volatile u16 *pUartGctl = (volatile u16*) (UART0_GCTL + UartMmrOffset);
    volatile u16 *pUartThr  = (volatile u16*) (UART0_THR  + UartMmrOffset);
    volatile u16 *pUartLsr  = (volatile u16*) (UART0_LSR  + UartMmrOffset);
    volatile u16 *pUartDlh  = (volatile u16*) (UART0_DLH  + UartMmrOffset);
    volatile u16 *pUartDll  = (volatile u16*) (UART0_DLL  + UartMmrOffset);

    UartDivisor = ( get_sclk_hz() / UartBitrate );
    if (!(*pUartGctl & EDBO)) {
        UartDivisor += 8;   /* round up before divide by 16 */
        UartDivisor >>= 4;
        }

    *pUartDlh = ( ( UartDivisor >> 8 ) & 0xFF );
    *pUartDll = (   UartDivisor        & 0xFF );


    /************************************************************
     Signal the completion of the autobaud detection to the host.
     Four bytes are transmitted back to the host:
        0xBF
        UART_DLL value
        UART_DLH value
        0x00 to terminate string
    ************************************************************/

    UartPutc(UartNum,0xBF);
    UartPutc(UartNum,*pUartDll);
    UartPutc(UartNum,*pUartDlh);
    UartPutc(UartNum,0x00);
    while( (*pUartLsr & TEMT) == 0 ) { /* wait */ }
}


#endif /* (__ADSPBF50x__ == 1) || (__ADSPBF54x__ == 1) */


#if (__ADSPBF51x__ == 1) || (__ADSPBF52x__ == 1) || (__ADSPBF59x__ == 1)


/****************************************************************************
 Name:          UartGetBitrate
 Description:   get current UART Bit Rate
 Input:         u8 UartNum
 Return:        u8 UartBitrate
****************************************************************************/

u32 UartGetBitrate(u8 UartNum)
{
    u16 UartMmrOffset = 0;
    u32 UartBitrate = 0;

    switch (UartNum) {
        case 0: UartMmrOffset = UART0_MMR_OFFSET; break;
        case 1: UartMmrOffset = UART1_MMR_OFFSET; break;
        default: asm("EMUEXCPT;"); while(1);
    }

    volatile u16 *pUartLcr = (volatile u16*) (UART0_LCR + UartMmrOffset);
    volatile u16 *pUartDlh = (volatile u16*) (UART0_DLH + UartMmrOffset);
    volatile u16 *pUartDll = (volatile u16*) (UART0_DLL + UartMmrOffset);


    *pUartLcr |= DLAB;
    UartBitrate = ( get_sclk_hz() / ( (*pUartDlh << 8) | *pUartDll ) );
    *pUartLcr ^= DLAB;

    UartBitrate += 8;   /* round up before divide by 16 */
    UartBitrate >>= 4;

    return UartBitrate;
}


/****************************************************************************
 Name:          UartSetBitrate
 Description:   calculate and set UART Divisor latch registers
                UARTx_DLH & UARTx_DLL that fits former Bit Rate
                and new system clock
 Input:         u8 UartNum, u8 UartBitrate
 Return:        -
*****************************************************************************/

void UartSetBitrate(u8 UartNum, u32 UartBitrate)
{
    u16 UartDivisor = 0;
    u16 UartMmrOffset = 0;
    u16 UartDll = 0;
    u16 UartDlh = 0;

    switch (UartNum) {
        case 0: UartMmrOffset = UART0_MMR_OFFSET; break;
        case 1: UartMmrOffset = UART1_MMR_OFFSET; break;
        default: asm("EMUEXCPT;"); while(1);
    }

    volatile u16 *pUartThr = (volatile u16*) (UART0_THR + UartMmrOffset);
    volatile u16 *pUartLcr = (volatile u16*) (UART0_LCR + UartMmrOffset);
    volatile u16 *pUartLsr = (volatile u16*) (UART0_LSR + UartMmrOffset);
    volatile u16 *pUartDlh = (volatile u16*) (UART0_DLH + UartMmrOffset);
    volatile u16 *pUartDll = (volatile u16*) (UART0_DLL + UartMmrOffset);

    UartDivisor = (get_sclk_hz() / UartBitrate);
    UartDivisor += 8; // round up before divide by 16
    UartDivisor >>= 4;
    UartDll = ( UartDivisor & 0x00FF);
    UartDlh = ((UartDivisor & 0xFF00) >> 8);

    *pUartLcr |= DLAB;
    *pUartDll = UartDll;
    *pUartDlh = UartDlh;
    *pUartLcr ^= DLAB;

    /************************************************************
     Signal the completion of the autobaud detection to the host.
     Four bytes are transmitted back to the host:
        0xBF
        UART_DLL value
        UART_DLH value
        0x00 to terminate string
    ************************************************************/

    UartPutc(UartNum,0xBF);
    UartPutc(UartNum,UartDll);
    UartPutc(UartNum,UartDlh);
    UartPutc(UartNum,0x00);
    while( (*pUartLsr & TEMT) == 0 ) { /* wait */ }
}


#endif /* (__ADSPBF51x__ == 1) || (__ADSPBF52x__ == 1) || (__ADSPBF59x__ == 1) */


/****************************************************************************
 EOF
*****************************************************************************/

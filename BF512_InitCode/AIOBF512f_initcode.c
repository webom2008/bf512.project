/*****************************************************************************
**                                                                          **
**  Name:   ezboardBF518f_initcode EXAMPLE                                  **
**                                                                          **
******************************************************************************

(C) Copyright 2007-2010 - Analog Devices, Inc.  All rights reserved.

Project Name:   ezboardBF518f_initcode

Version:        v7.0

Last Modified:  04/20/10 ap-muc

Tested
Software:       VisualDSP++ 5.0.8.0 (Update 8)

Supported
Software:       VisualDSP++ 5.0.8.0 (Update 8 and above)

Tested
Hardware:       ADSP-BF518F EZ-Board Rev. 0.1 Silicon Rev. 0.1

Supported
Platforms:      ADSP-BF518F EZ-Board

Connections:    ADDS-HPUSB-ICE

Project
Properties:     Compile : Preprocessor : Additional include directories -> ..\src\
                Link : LDF Preprocessing : Additional include directories -> ..\src\

Hardware
Settings:       default

Purpose:        Initialization example for ADSP-BF518F EZ-Board
                Included are:
                - PLL Settings (optimized for high speed)
                - Voltage Regulator Settings for external voltage regulator
                - EBIU settings for the EZ-Board (Asynchronous and SDR SDRAM memory)
                - Bit Rate handler for Mode 'Boot from UART host (slave mode)'

Restrictions:   - Wait states for async flash interface are not optimized yet for some examples. Default/Save settings are still included

*****************************************************************************/

/****************************************************************************
 Include Section
*****************************************************************************/

#include "init_platform.h"
#include "UART.h"

/*****************************************************************************
 Main Program
*****************************************************************************/

section ("L1_code")

void initcode(ADI_BOOT_DATA* pBS)
{
    u16 uwIMASK_reg = 0;
    u32 udUART_BIT_RATE = 0;
    u16 uwUARTx_MCR = 0;


    /************************************************
    * System Interrupt Wakeup Registers (SIC_IWRx)  *
    * Required for PLL_CTL and VR_CTL configuration *
    * Processor idle and wake up after PLL_LOCKCNT  *
    * expired.Reset = 0xFFFF FFFF                   *
    * disable all other interrupt sources in the    *
    * meantime to disable unwanted wake up events   *
    ************************************************/

    *pSIC_IWR1 = IWR1_DISABLE_ALL;
    *pSIC_IWR0 = IRQ_PLL_WAKEUP;


    #if defined __ACTIVATE_DPM__

        /****************************************
         Mode 'Boot from UART host (slave mode)'
         Save current BIT RATE value
        ****************************************/

        if ( (*pSYSCR & BMODE) == BMODE_UART0HOST ) udUART_BIT_RATE = UartGetBitrate(0);


        /***************************************
         Description:   Dynamic Power Managment
        ***************************************/

        if ( verify_clocks(0,0,0) == NO_ERR ) { full_on(); }
        else { asm("EMUEXCPT;"); while(1); }


        /*******************************************************
         Mode 'Boot from UART host (slave mode)'
         Update UART Divisor latch register UART_DLH & UART_DLL
         accroding to new system clock frequency
        *******************************************************/

        if ( (*pSYSCR & BMODE) == BMODE_UART0HOST ) UartSetBitrate(0, udUART_BIT_RATE);


    #endif /* __ACTIVATE_DPM__ */


    /*****************************************************************************
     Description:   Configure EBIU
    ******************************************************************************/


    /*****************************************************************************
     Description:   Configure and enable Asynchronous memory & Flash Mode
    ******************************************************************************/

    if ( (*pSYSCR & BMODE) == BMODE_FLASH  ) async_mem_en();


    /*****************************************************************************
     Description:   Configure and enable SDC
    ******************************************************************************/

    sdram_en();


    /**************************************************************
     Mode 'Boot from SPI memory (Master mode)'
     modify SPIx_BAUD register to speed up booting from SPI memory
    **************************************************************/

    if ( ((*pSYSCR & BMODE) ==  BMODE_SPI0MEM_INT) || ((*pSYSCR & BMODE) ==  BMODE_SPI0MEM_EXT) )
    {
        pBS->dFlags |= BFLAG_FASTREAD;
        *pSPI_BAUD  = SPI_BAUD_VAL;
        pBS->dClock = SPI_BAUD_VAL; /* required to keep dClock in pBS (-> ADI_BOOT_DATA) consistent */
    }


    /*********************************************
    * Restore System Interrupt Wakeup Registers  *
    *********************************************/

    *pSIC_IWR0 = IWR0_ENABLE_ALL;
    *pSIC_IWR1 = IWR1_ENABLE_ALL;


    /* When Compiler Optimization is active, hardware loops are setup for a while loop */
    /* The Loop Count Registers (LC0/LC1) are not necessarily cleared */
    sysreg_write(reg_LC0,0);
    sysreg_write(reg_LC1,0);
}


/****************************************************************************
 EOF
*****************************************************************************/

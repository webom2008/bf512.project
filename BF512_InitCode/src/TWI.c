/*****************************************************************************
**                                                                          **
**  Name:   TWI                                                             **
**                                                                          **
******************************************************************************

(C) Copyright 2007-2008 - Analog Devices, Inc.  All rights reserved.

Version:        v1.1

Last Modified:  12/15/08 ap-muc

History:        taken from Project Power_On_Self_Test
                for ADSP-BF518F EZ-Board in VisualDSP++ 5.0.5.0 (Update 5)

Tested
Software:       VisualDSP++ 5.0.8.0 (Update 8)

Tested
Hardware:       ADSP-BF518F EZ-Board Rev. 0.1 Silicon Rev. 0.0
                ADSP-BF526 EZ-Board Rev. 0.1 Silicon Rev. 0.0

Supported
Platforms:      ADSP-BF506F EZ-Board
                ADSP-BF518F EZ-Board
                ADSP-BF526 EZ-Board
                ADSP-BF592 EZ-Board

Connections:    ADDS-HPUSB-ICE

Software
Settings:       default

Hardware
Settings:       default

Purpose:        TWI interface functions to
                configure digital potentiometer (DIGIPOT)
                for external voltage regulation

*****************************************************************************/

/*****************************************************************************
 Include Files
******************************************************************************/

#include "init_platform.h"
#include "TWI.h"

/*****************************************************************************
 Name:          twi_reset
 Description:   reset the TWI interface
 Input:         -
 Return:        -
******************************************************************************/

void twi_reset(void)
{
    /* Reset the TWI controller */
    *pTWI_CONTROL = TWI_RESET_VAL;
    ssync();

    /* Clearing all status bits */
    *pTWI_MASTER_STAT = ( BUFWRERR | BUFRDERR | DNAK | ANAK | LOSTARB );
    ssync();

    /* Clearing all pending interrupt flags */
    *pTWI_INT_STAT = ( SINIT | SCOMP | SERR | SOVF | MCOMP | MERR | XMTSERV | RCVSERV );
    ssync();

    /* flush the buffers */
    *pTWI_FIFO_CTL = ( XMTFLUSH | RCVFLUSH );
    ssync();
}


/*****************************************************************************
 Name:          twi_mastermode_write
 Description:   do a TWI write in master mode
 Input:         u16 uwDeviceAddr, u8 *pTwiDataPointer, u16 uwCount, u16 uwTwiLength
 Return:        -
******************************************************************************/

void twi_mastermode_write(u16 uwDeviceAddr, u8 *pTwiDataPointer, u16 uwCount, u16 uwTwiLength)
{
    u8 i, j;

    *pTWI_FIFO_CTL = 0; /* clear the bit manually */
    *pTWI_CONTROL = ( TWI_ENA | PRESCALE_VALUE ); /* PRESCALE = fsclk/10MHz */
    *pTWI_CLKDIV = ( ((CLKDIV_HI) << 8) | (CLKDIV_LO) ); /* CLKDIV = (1/SCL)/(1/10MHz) */
    *pTWI_MASTER_ADDR = uwDeviceAddr; /* target address (7-bits plus the read/write bit) */

    for ( i=0; i<uwCount; i++ )
    {
        /* # of configurations to send */
        *pTWI_XMT_DATA8 = *pTwiDataPointer++; /* pointer to data */
        ssync();

        *pTWI_MASTER_CTL = ( (uwTwiLength<<6) | MEN /*| FAST*/ ); /* start transmission */

        for ( j=0; j<(uwTwiLength-1); j++ )
        {
            /* # of transfers before stop condition */
            while (*pTWI_FIFO_STAT == XMTSTAT) { ssync(); } /* wait to load the next sample into the TX FIFO */

            *pTWI_XMT_DATA8 = *pTwiDataPointer++; /* load the next sample into the TX FIFO */
            ssync();
        }

        while ((*pTWI_INT_STAT & MCOMP) == 0) { ssync(); } /* wait until transmission complete and MCOMP is set */

        *pTWI_INT_STAT = ( XMTSERV | MCOMP ); /* service TWI for next transmission */
    }

    ssync();
}


/*****************************************************************************
 Name:          twi_mastermode_read
 Description:   do a TWI read in master mode
 Input:         u16 uwDeviceAddr, u8 *pTwiDataPointer, u16 uwCount
 Return:        -
******************************************************************************/

void twi_mastermode_read(u16 uwDeviceAddr, u8 *pTwiDataPointer, u16 uwCount)
{
    u8 i;

    *pTWI_FIFO_CTL = 0; /* clear the bit manually */
    *pTWI_CONTROL = ( TWI_ENA | PRESCALE_VALUE ); /* PRESCALE = fsclk/10MHz */
    *pTWI_CLKDIV = ( ((CLKDIV_HI) << 8) | (CLKDIV_LO) ); /* CLKDIV = (1/SCL)/(1/10MHz) */
    *pTWI_MASTER_ADDR = uwDeviceAddr; /* target address (7-bits plus the read/write bit) */

    *pTWI_MASTER_CTL = ( (uwCount<<6) | MEN | MDIR /*| FAST*/ ); /* start transmission */

    /* for each item */
    for ( i=0; i<uwCount; i++ )
    { 
        while (*pTWI_FIFO_STAT == RCV_EMPTY) { ssync(); } /* wait for data to be in FIFO */

        *pTwiDataPointer++ = *pTWI_RCV_DATA8; /* read the data */
        ssync();
    }

    while ((*pTWI_INT_STAT & MCOMP) == 0) { ssync(); } /* wait until transmission complete and MCOMP is set */

    *pTWI_INT_STAT = ( RCVSERV | MCOMP ); /* service TWI for next transmission */

    ssync();
}


/****************************************************************************
 EOF
*****************************************************************************/

/*****************************************************************************
**                                                                          **
**  Name:   VR_DIGIPOT                                                      **
**                                                                          **
******************************************************************************

(C) Copyright 2007-2008 - Analog Devices, Inc.  All rights reserved.

Version:        v1.1

Last Modified:  12/16/08 ap-muc

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

Purpose:        configure digital potentiometer (DIGIPOT)
                for external voltage regulation

*****************************************************************************/

/*****************************************************************************
 Include Files
******************************************************************************/

#include "init_platform.h"
#include "VR_DIGIPOT.h"
#include "TWI.h"

/*****************************************************************************
 Global variables
******************************************************************************/

u8 WriteRDAC[] = { 0x00, RDAC_STEP_VAL };
u8 StoreRDAC[] = { 0xC0 };
u8 ReadRdacPrefix[] = { 0x00 };
u8 ReadEepromPrefix[] = { 0x20 };


/*****************************************************************************
 Name:          vr_digipot_program
 Description:   This programs the digipot on the EZ-Board.
 Input:         -
 Return:        ERROR_CODE
******************************************************************************/

ERROR_CODE vr_digipot_program (void)
{
    u16 uwIMASK_reg = 0;

    twi_reset(); /* reset the TWI interface */

    /* write the RDAC register */
    twi_mastermode_write(DIGIPOT_ADDRESS, WriteRDAC, 1, 2);
    ssync();

    /* store the RDAC value in digipot EEPROM */
    twi_mastermode_write(DIGIPOT_ADDRESS, StoreRDAC, 1, 1);
    ssync();

    return ( vr_digipot_check() );
}


/*****************************************************************************
 Name:          vr_digipot_check
 Description:   This verifies the digipot has been programmed on the EZ-Board.
 Input:         -
 Return:        ERROR_CODE
******************************************************************************/

ERROR_CODE vr_digipot_check (void)
{
    u8 usRdac = 0x00; /* stored RDAC value */
    u8 usEeprom = 0x00; /* stored EEPROM value */

    twi_reset(); /* reset the TWI interface */

    /* read RDAC and EEPROM */
    twi_mastermode_write(DIGIPOT_ADDRESS, ReadRdacPrefix, 1, 1);
    twi_mastermode_read(DIGIPOT_ADDRESS, &usRdac, 1);

    twi_mastermode_write(DIGIPOT_ADDRESS, ReadEepromPrefix, 1, 1);
    twi_mastermode_read(DIGIPOT_ADDRESS, &usEeprom, 1);

    /* both values should match the RDAC value that we program */
    if ( (RDAC_STEP_VAL == usRdac) && (RDAC_STEP_VAL == usEeprom) )
        return NO_ERR;
    else
        return ERROR;
}


/*****************************************************************************
 Name:          vr_digipot_rdac_read
 Description:   This verifies the digipot has been programmed on the EZ-Board.
 Input:         -
 Return:        u8 usRdac
******************************************************************************/

u8 vr_digipot_rdac_read (void)
{
    u8 usRdac = 0x00; /* stored RDAC value */

    twi_reset(); /* reset the TWI interface */

    /* read RDAC */
    twi_mastermode_write(DIGIPOT_ADDRESS, ReadRdacPrefix, 1, 1);
    twi_mastermode_read(DIGIPOT_ADDRESS, &usRdac, 1);

    return usRdac;
}


/****************************************************************************
 EOF
*****************************************************************************/

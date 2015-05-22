/*****************************************************************************
**                                                                          **
**  Name:   NOR_FLASH                                                       **
**                                                                          **
******************************************************************************

(C) Copyright 2007-2008 - Analog Devices, Inc.  All rights reserved.

Version:        v1.02

Last Modified:  12/15/08 ap-muc

History:        v0.0 05/22/07 JW

Tested
Software:       VisualDSP++ 5.0.0.0
                VisualDSP++ 5.0.1.0 (Update 1)
                VisualDSP++ 5.0.2.0 (Update 2)
                VisualDSP++ 5.0.3.0 (Update 3)
                VisualDSP++ 5.0.4.0 (Update 4)
                VisualDSP++ 5.0.5.0 (Update 5)

Tested
Hardware:       ADSP-BF548 EZ-KIT Lite Rev. 1.2 Silicon Rev. 0.0
                ADSP-BF548 EZ-KIT Lite Rev. 1.3 Silicon Rev. 0.1
                ADSP-BF548 EZ-KIT Lite Rev. 1.4 Silicon Rev. 0.2

Supported
Platforms:      ADSP-BF548 EZ-KIT Lite

Connections:    ADDS-HPUSB-ICE

Software
Settings:       default

Hardware
Settings:       default

Purpose:        Generate the Initcode for BF54x to set the NOR flash to
                Burst/Page/Flash mode for further use of the Boot Rom

*****************************************************************************/


/*****************************************************************************
 Include Files
******************************************************************************/

#include "init_platform.h"
#include "NOR_FLASH.h"

/*****************************************************************************
 Prototypes / Functions
******************************************************************************/

section ("program") ERROR_CODE SetupForFlash (void);

section ("program") ERROR_CODE ReadFlash ( u32 udOffset, u16 *pnValue );
section ("program") ERROR_CODE WriteFlash ( u32 udOffset, u16 nValue );

section ("program") ERROR_CODE GetCodes (void);
section ("program") ERROR_CODE ResetFlash (void);

section ("program") ERROR_CODE SetRCR ( u16 uwRCR );
section ("program") ERROR_CODE GetRCR ( u16* uwRCR );

/****************************************************************************
 Global Variables
*****************************************************************************/

/* Manufacture Code and Device Code */
section ("data1") u16 uwManufactureCode = 0;     /* 0x89 = Intel */
section ("data1") u16 uwDeviceCode = 0;          /* 0x891F = PC28F128K3C115 */
section ("data1") u16 uwRCR_Default = 0;


/****************************************************************************
 Functions
*****************************************************************************/


/*****************************************************************************
 Name:          SetupForFlash
 Description:   Perform necessary setup for the processor to talk to the
                flash such as external memory interface registers, etc.
 Input:         
 Return:        ERROR_CODE
******************************************************************************/

ERROR_CODE SetupForFlash(void)
{
    /* set port H MUX to configure PH8-PH13 as 1st Function (MUX = 00) (bits 16-27 == 0) - Address signals A4-A9 */
    *pPORTH_FER = 0x3F00;
    *pPORTH_MUX = ( *pPORTH_MUX ) & ( 0xF000FFFF );
    /* configure PI0-PI15 (JW. PI14) as A10-A21(JW. A24) respectively, and PI15 is the NOR CLK used for burst mode */
    *pPORTI_FER = 0xFFFF;
    *pPORTI_MUX = 0x00000000L;
    
    #if  0     /* ----- #if 0 : If0Label_1 ----- */
        *pPORTJ_FER = 0x1;                      /* configure PJ0 as 1st Function ( NOR WAIT ) */
        *pPORTJ_MUX = 0x0;
        *pPORTJ_DIR_SET = 0x1;
    #endif     /* ----- #if 0 : If0Label_1 ----- */

    return NO_ERR;
}


/*****************************************************************************
 Name:          WriteFlash
 Description:   Write a value to an offset in flash.
 Input:         u32 udOffset, u16 *pnValue
 Return:        ERROR_CODE
******************************************************************************/

ERROR_CODE WriteFlash ( u32 udOffset, u16 nValue )
{
    u16* pFlashAddr = ( u16 * ) ( FLASH_START_ADDR + udOffset );
    *pFlashAddr = ( u16 ) nValue;

    return NO_ERR;
}


/*****************************************************************************
 Name:          ReadFlash
 Description:   Read a value from an offset in flash.
 Input:         u32 udOffset, u16 *pnValue
 Return:        ERROR_CODE
******************************************************************************/

ERROR_CODE ReadFlash ( u32 udOffset, u16 *pnValue )
{
    u16* pFlashAddr = ( u16 * ) ( FLASH_START_ADDR + udOffset );
    *pnValue = *pFlashAddr;

    return NO_ERR;
}


/*****************************************************************************
 Name:          ResetFlash
 Description:   Sends a "reset" command to the flash. This is actually to put 
                the Flash in to Array Read mode - the default mode.
 Input:         -
 Return:        ERROR_CODE
******************************************************************************/

ERROR_CODE ResetFlash(void)
{
    WriteFlash ( 0x0000, 0xFF );    /* send the reset command to the flash */

    return NO_ERR;
}


/*****************************************************************************
 Name:          GetCodes
 Description:   
 Input:         -
 Return:        ERROR_CODE
******************************************************************************/

ERROR_CODE GetCodes(void)
{
    WriteFlash ( 0x0000, 0x90 );                /* send the auto select command to the flash */
    ReadFlash ( 0x0000, &uwManufactureCode );  /* now we can read the codes */
    uwManufactureCode &= 0x00FF;
    ReadFlash ( 0x0002, &uwDeviceCode );
    uwDeviceCode &= 0xFFFF;
    uwRCR_Default = 0xBFCF;                    /* hardwired to facilitate the test , it may be GetRCR( &RCR_Default ); */
    ResetFlash();                               /* we need to issue another command to get the part out of auto select mode so issue a reset which  just puts the device back in read mode */

    return NO_ERR;
}


/*****************************************************************************
 Name:          SetRCR
 Description:   
 Input:         u16* uwRCR
 Return:        ERROR_CODE
******************************************************************************/

ERROR_CODE SetRCR ( u16 uwRCR )
{
    WriteFlash( uwRCR << 1, 0x60 );           /* RCR setup */
    WriteFlash( uwRCR << 1, 0x03 );
    ResetFlash();

    return NO_ERR;
}


/*****************************************************************************
 Name:          GetRCR
 Description:   
 Input:         u16* uwRCR
 Return:        ERROR_CODE
******************************************************************************/

ERROR_CODE GetRCR ( u16* uwRCR )
{
    WriteFlash( 0x0000, 0x90 );                /* send the auto select command to the flash */
    ReadFlash ( 0x000A, uwRCR );               /* now we can read the RCR */
    ResetFlash();

    return NO_ERR;
}


/*****************************************************************************
 Name:          SetToFlashMode
 Description:   Set all Banks to Asynchronous Flash Mode
 Input:         -
 Return:        ERROR_CODE
******************************************************************************/

ERROR_CODE SetToFlashMode(void)
{
    SetupForFlash();
    GetCodes();

    *pEBIU_AMGCTL = 0x0000;                 /* disable the Async Memory */
    /* Set the mode to async Mode (0-async mode, 1-flash mode, 2-page mode, 3-burst mode) */
    *pEBIU_MODE   = ( B0MODE_FLASH | B1MODE_FLASH | B2MODE_FLASH | B3MODE_FLASH );
    *pEBIU_FCTL   = ( BCLK );
    *pEBIU_AMGCTL = ( AMCKEN | AMBEN );     /* enable the Async Memory */
    ssync();

    SetRCR ( uwRCR_Default );
    ResetFlash();

    return NO_ERR;
}


/*****************************************************************************
 Name:          SetToPageMode
 Description:   Set all Banks to Asynchronous Page Mode
 Input:         -
 Return:        ERROR_CODE
******************************************************************************/

ERROR_CODE SetToPageMode(void)
{
    SetupForFlash();
    GetCodes();

    *pEBIU_AMGCTL = 0x0000;                 /* disable the Async Memory */
    /* Set the mode to async Mode (0-async mode, 1-flash mode, 2-page mode, 3-burst mode) */
    *pEBIU_MODE   = ( B0MODE_PAGE | B1MODE_PAGE | B2MODE_PAGE | B3MODE_PAGE );
    *pEBIU_FCTL   = ( PGWS | BCLK );
    *pEBIU_AMGCTL = ( AMCKEN | AMBEN );     /* enable the Async Memory */
    ssync();

    SetRCR ( uwRCR_Default );
    ResetFlash();

    return NO_ERR;
}


/*****************************************************************************
 Name:          SetToBurstMode
 Description:   Set all Banks to Synchronous (Burst) Mode
 Input:         -
 Return:        ERROR_CODE
******************************************************************************/

ERROR_CODE SetToBurstMode(void)
{
    u16 uwRCRtmp;

    SetupForFlash();
    GetCodes();

    *pEBIU_AMGCTL = 0x0000;                 /* disable the Async Memory */
    /* Set the mode to async Mode (0-async mode, 1-flash mode, 2-page mode, 3-burst mode) */
    *pEBIU_MODE   = ( B0MODE_BURST | B1MODE_BURST | B2MODE_BURST | B3MODE_BURST );
    *pEBIU_FCTL   = ( BCLK );
    *pEBIU_AMGCTL = ( AMCKEN | AMBEN );     /* enable the Async Memory */
    ssync();

    uwRCRtmp = uwRCR_Default & 0x05F0;
    uwRCRtmp |= 0x0003;
    uwRCRtmp |= 3 << 11;
    SetRCR ( uwRCRtmp );
    ResetFlash();

    return NO_ERR;
}


/*****************************************************************************
 Name:          SetToAsyncMode
 Description:   Set all Banks to Asynchronous (default) Mode
 Input:         -
 Return:        ERROR_CODE
******************************************************************************/

ERROR_CODE SetToAsyncMode(void)
{
    *pEBIU_AMGCTL = 0x0000;                 /* disable the Async Memory */
    /* Set the mode to async Mode (0-async mode, 1-flash mode, 2-page mode, 3-burst mode) */
    *pEBIU_MODE   = ( B0MODE_ASYNC | B1MODE_ASYNC | B2MODE_ASYNC | B3MODE_ASYNC );
    *pEBIU_FCTL   = ( BCLK );
    *pEBIU_AMGCTL = ( AMCKEN | AMBEN );     /* enable the Async Memory */
    ssync();

    return NO_ERR;
}


/****************************************************************************
 EOF
*****************************************************************************/

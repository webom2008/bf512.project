/*******************************************************************************/
/*                                                                             */
/*   (C) Copyright 2008 - Analog Devices, Inc.  All rights reserved.           */
/*                                                                             */
/*    FILE:     bf50x4mbflash.c                                                */
/*                                                                             */
/*    PURPOSE:  Performs operations specific to the bf50x4mbflash device.  	   */
/*                                                                             */
/*******************************************************************************/


/* includes */
#include <ccblkfn.h>
#include <drivers\flash\util.h>
#include <drivers\flash\Errors.h>
#include <drivers\flash\bf50x4mbflash.h>


/* constants, if you have a custom target you may need to change these */
#if defined ( __ADSPBF506F__) || ( __ADSPBF504F__)		/* this works on both BF506F and BF504F parts */
static char *pBoardDesc = 		"ADSP-BF506F EZ-KIT";	/* description of supported platform */
#define FLASH_START_ADDRESS		0x20000000				/* location of the start of flash */
#else
/* we should not fall in here */
#error *** target board not supported ***
#endif

static char 	*pFlashDesc;
static char 	*pDeviceCompany	=	"Analog Devices, Inc.";

static int		gNumSectors;		/* number of sectors */
static int		g_ManId = 0x0;		/* flash manufacturer id */
static int		g_DevId = 0x0;		/* flash device id */

/* function prototypes */
static ERROR_CODE EraseFlash(unsigned long ulStartAddr);
static ERROR_CODE EraseBlock( int nBlock, unsigned long ulStartAddr );
static ERROR_CODE GetCFIData(unsigned short *pCFIData, unsigned long ulAddr);
static ERROR_CODE GetCodes(int *pnManCode, int *pnDevCode, unsigned long ulStartAddr);
static ERROR_CODE GetSectorNumber( unsigned long ulAddr, int *pnSector );
static ERROR_CODE GetSectorStartEnd( unsigned long *ulStartOff, unsigned long *ulEndOff, int nSector );
static ERROR_CODE PollToggleBit(unsigned long ulOffset, unsigned long timeout);
static ERROR_CODE ReadFlash(unsigned long ulOffset, unsigned short *pusValue );
static ERROR_CODE ResetFlash(unsigned long ulStartAddr);
static ERROR_CODE WriteFlash(unsigned long ulOffset, unsigned short usValue );
static unsigned long GetFlashStartAddress( unsigned long ulAddr);
bool IsStatusReady( unsigned long ulOffset );



ERROR_CODE bf50x4mbflash_Open(void)
{
	ERROR_CODE Result = NO_ERR;


	/* make sure we are connecting to a supported part */
	GetCodes(&g_ManId, &g_DevId, FLASH_START_ADDRESS);
	if ( (FLASH_MANUFACTURER_BF50X4MBFLASH != g_ManId) || (FLASH_DEVICE_BF50X4MBFLASH != g_DevId) )
	{
		return UNKNOWN_COMMAND;
	}

	/* 32 Mbit (2 MB x 16) top boot device */
	gNumSectors = 71;
	pFlashDesc = "BF50x4MBFlash";

	return (Result);
}


ERROR_CODE bf50x4mbflash_Close(void)
{
	return (NO_ERR);
}


ERROR_CODE bf50x4mbflash_Read(unsigned short *pusData,
                    			unsigned long ulStartAddress,
								unsigned int uiCount )
{
	ERROR_CODE Result = NO_ERR;
	unsigned int i = 0;
	unsigned short *pusCurrentData = pusData;
    unsigned long ulCurrentAddress = ulStartAddress;


	for (i = 0; i < uiCount; i++, ulCurrentAddress++, pusCurrentData++)
    {
		Result = ReadFlash( ulCurrentAddress, pusCurrentData );
	}

	return(Result);
}


ERROR_CODE bf50x4mbflash_Write(unsigned short *pusData,
						 		unsigned long ulStartAddress,
						 		unsigned int uiCount )
{
    ERROR_CODE Result = NO_ERR;
    unsigned int i = 0;
    unsigned short *pusCurrentData = pusData;
    unsigned long ulCurrentAddress = ulStartAddress;
    unsigned long ulFlashStartAddress;


    for (i = 0; i < uiCount; i++, ulCurrentAddress++, pusCurrentData++)
    {
		/* get flash start address from absolute address */
		ulFlashStartAddress = GetFlashStartAddress(ulCurrentAddress);

		// send the unlock command to the flash
		Result = WriteFlash( ulFlashStartAddress, 0x60 );
		Result = WriteFlash( ulFlashStartAddress, 0xD0 );

		Result = PollToggleBit(ulFlashStartAddress, PROGRAM_TIMEOUT);

		WriteFlash( ulFlashStartAddress, 0x40 );

		// program our actual value now
		Result = WriteFlash( ulCurrentAddress, *pusCurrentData );

		// make sure the write was successful
		Result = PollToggleBit(ulFlashStartAddress, PROGRAM_TIMEOUT);
	}

	return(Result);
}



ERROR_CODE bf50x4mbflash_Control( unsigned int uiCmd, COMMAND_STRUCT *pCmdStruct )
{

	ERROR_CODE ErrorCode = NO_ERR;

	// switch on the command
	switch ( uiCmd )
	{
		// erase all
		case CNTRL_ERASE_ALL:
			ErrorCode = EraseFlash(pCmdStruct->SEraseAll.ulFlashStartAddr);
			break;

		// erase sector
		case CNTRL_ERASE_SECT:
			ErrorCode = EraseBlock( pCmdStruct->SEraseSect.nSectorNum,
									pCmdStruct->SEraseSect.ulFlashStartAddr );
			break;

		// get manufacturer and device codes
		case CNTRL_GET_CODES:
			ErrorCode = GetCodes((int *)pCmdStruct->SGetCodes.pManCode,
								 (int *)pCmdStruct->SGetCodes.pDevCode,
								 (unsigned long)pCmdStruct->SGetCodes.ulFlashStartAddr);
			break;

		// get flash width
		case CNTRL_GET_FLASH_WIDTH:
			*(int *)pCmdStruct->SGetFlashWidth.pnFlashWidth = _FLASH_WIDTH_16;
			ErrorCode = NO_ERR;
			break;

		// supports CFI?
		case CNTRL_SUPPORTS_CFI:
			*(bool *)pCmdStruct->SSupportsCFI.pbSupportsCFI = true;
			ErrorCode = NO_ERR;
			break;

		// get CFI data
		case CNTRL_GET_CFI_DATA:
			ErrorCode = GetCFIData(	(unsigned short *)pCmdStruct->SGetCFIData.pCFIData,
									pCmdStruct->SGetCFIData.ulFlashStartAddr);
			break;

		case CNTRL_GET_DESC:
			//Filling the contents with data
			pCmdStruct->SGetDesc.pTitle = pBoardDesc;
			pCmdStruct->SGetDesc.pDesc  = pFlashDesc;
			pCmdStruct->SGetDesc.pFlashCompany  = pDeviceCompany;
			break;

		// get sector number based on address
		case CNTRL_GET_SECTNUM:
			ErrorCode = GetSectorNumber( pCmdStruct->SGetSectNum.ulOffset, (int *)pCmdStruct->SGetSectNum.pSectorNum );
			break;

		// get sector number start and end offset
		case CNTRL_GET_SECSTARTEND:
			ErrorCode = GetSectorStartEnd( pCmdStruct->SSectStartEnd.pStartOffset, pCmdStruct->SSectStartEnd.pEndOffset, pCmdStruct->SSectStartEnd.nSectorNum );
			break;

		// get the number of sectors
		case CNTRL_GETNUM_SECTORS:
			pCmdStruct->SGetNumSectors.pnNumSectors[0] = gNumSectors;
			break;

		// reset
		case CNTRL_RESET:
			ErrorCode = ResetFlash(pCmdStruct->SReset.ulFlashStartAddr);
			break;

		// no command or unknown command do nothing
		default:
			// set our error
			ErrorCode = UNKNOWN_COMMAND;
			break;
	}

	// return
	return(ErrorCode);
}


//-----  H e l p e r   F u n c t i o n s	----//

//----------- R e s e t F l a s h  ( ) ----------//
//
//  PURPOSE
//  	Sends a "reset" command to the flash.
//
//	INPUTS
//		unsigned long ulStartAddr - flash start address
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE ResetFlash(unsigned long ulAddr)
{
	unsigned long ulFlashStartAddr;		// flash start address

	// get flash start address from absolute address
	// The ulAddr should ideally be pointing to the flash start
	// address. However we just verify it here again.
	ulFlashStartAddr = GetFlashStartAddress(ulAddr);

	// send the reset command to the flash
	WriteFlash( ulFlashStartAddr, 0xFF );

	// reset should be complete
	return NO_ERR;
}


//----------- E r a s e F l a s h  ( ) ----------//
//
//  PURPOSE
//  	Sends an "erase all" command to the flash.
//
//	INPUTS
//		unsigned long ulStartAddr - flash start address
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE EraseFlash(unsigned long ulAddr)
{
	ERROR_CODE ErrorCode = NO_ERR;	// tells us if there was an error erasing flash
	int nBlock = 0;


	for( nBlock = 0; ( nBlock < (gNumSectors - 1) ) && ( ErrorCode == NO_ERR ); nBlock++ )
		ErrorCode = EraseBlock( nBlock, ulAddr );

	// erase should be complete
	return ErrorCode;
}


//----------- E r a s e B l o c k ( ) ----------//
//
//  PURPOSE
//  	Sends an "erase block" command to the flash.
//
//	INPUTS
//		int nBlock - block to erase
//		unsigned long ulStartAddr - flash start address
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE EraseBlock( int nBlock, unsigned long ulAddr )
{

	ERROR_CODE 	  ErrorCode   = NO_ERR;		//tells us if there was an error erasing flash
 	unsigned long ulSectStart = 0x0;		//stores the sector start offset
 	unsigned long ulSectEnd   = 0x0;		//stores the sector end offset(however we do not use it here)
	unsigned long ulFlashStartAddr;			//flash start address

 	// get flash start address from absolute address
	// The ulAddr should ideally be pointing to the flash start
	// address. However we just verify it here again.
	ulFlashStartAddr = GetFlashStartAddress(ulAddr);

	if ( (nBlock < 0) || (nBlock >= gNumSectors)  )
		return INVALID_BLOCK;

	ErrorCode = GetSectorStartEnd( &ulSectStart, &ulSectEnd, nBlock );

	if( ErrorCode == NO_ERR )
	{
		// unlock this block
		WriteFlash( (ulFlashStartAddr + ulSectStart), 0x60 );
		WriteFlash( (ulFlashStartAddr + ulSectStart), 0xD0 );

		ErrorCode = PollToggleBit((ulFlashStartAddr + ulSectStart), ERASE_TIMEOUT);

		// erase block
		WriteFlash( (ulFlashStartAddr + ulSectStart), 0x20 );
		WriteFlash( (ulFlashStartAddr + ulSectStart), 0xD0 );

		ErrorCode = PollToggleBit((ulFlashStartAddr + ulSectStart), ERASE_TIMEOUT);

		WriteFlash( (ulFlashStartAddr + ulSectStart), 0xFF );
	}

		// block erase should be complete
	return ErrorCode;
}


//////////////////////////////////////////////////////////////
// ERROR_CODE PollToggleBit()
//
// Polls the toggle bit in the flash to see when the operation
// is complete.
//
// Inputs:	unsigned long ulOffset - offset in flash
//
//////////////////////////////////////////////////////////////

ERROR_CODE PollToggleBit(unsigned long ulOffset, unsigned long timeout)
{

	while ( timeout )
    {

        if ( IsStatusReady( ulOffset ) )
        {
            WriteFlash( ulOffset, 0xFF );
            return( NO_ERR );
        }
        timeout--;
    }

    return( POLL_TIMEOUT );

}


//----------- G e t C o d e s ( ) ----------//
//
//  PURPOSE
//  	Sends an "auto select" command to the flash which will allow
//		us to get the manufacturer and device codes.
//
//  INPUTS
//  	int *pnManCode - pointer to manufacture code
//		int *pnDevCode - pointer to device code
//		unsigned long ulStartAddr - flash start address
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE GetCodes(int *pnManCode, int *pnDevCode, unsigned long ulAddr)
{

	unsigned long ulFlashStartAddr;		//flash start address

	// set the flash start address
	ulFlashStartAddr = ulAddr;

 	// send the auto select command to the flash
	WriteFlash( ulFlashStartAddr, 0x90 );

 	// now we can read the codes
	ReadFlash( ulFlashStartAddr + 0x0000,(unsigned short *)pnManCode );
	*pnManCode &= 0x00FF;

	ReadFlash( ulFlashStartAddr + 0x0002, (unsigned short *)pnDevCode );
	*pnDevCode &= 0xFFFF;

	// we need to issue another command to get the part out
	// of auto select mode so issue a reset which just puts
	// the device back in read mode
	ResetFlash(ulAddr);

	// ok
	return NO_ERR;
}


//----------- G e t C F I D a t a ( ) ----------//
//
//  PURPOSE
//  	Sends a CFI query command to the flash which will allow
//		us to get the Common Flash Interface data structure.
//
//  INPUTS
//  	unsigned short *pCFIData - pointer to CFI data structure
//		unsigned long ulAddr - flash start address
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE GetCFIData(unsigned short *pCFIData, unsigned long ulAddr)
{
	unsigned int i = 0;					/* index */
	unsigned long ulFlashStartAddr;		/* flash start address */
	unsigned short *pusData = pCFIData;	/* data ptr */

	// set the flash start address
	ulFlashStartAddr = ulAddr;

 	// send the auto select command to the flash
	WriteFlash( ulFlashStartAddr + 0xaa, 0x98 );

 	// now we can read the data, times 2 because it's a 16-bit device
 	for ( i = 0; i < 0x80*2; i+=2, pusData++ )
 	{
		ReadFlash( ulFlashStartAddr + i,(unsigned short *)pusData );
 	}

	// we need to issue another command to get the part out
	// of CFI query mode so issue a reset which just puts
	// the device back in read mode
	ResetFlash(ulAddr);

	// ok
	return NO_ERR;
}


//----------- G e t S e c t o r N u m b e r ( ) ----------//
//
//  PURPOSE
//  	Gets a sector number based on the offset.
//
//  INPUTS
//  	unsigned long ulAddr - absolute address
//		int 	 *pnSector     - pointer to sector number
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE GetSectorNumber( unsigned long ulAddr, int *pnSector )
{
	int nSector = 0;
	unsigned long ulMask;					//offset mask
	unsigned long ulOffset;					//offset
	unsigned long ulStartOff;
	unsigned long ulEndOff;
	int i = 0;

	ulMask      	  = 0x3fffff;
	ulOffset		  = ulAddr & ulMask;

	for(i = 0; i < gNumSectors; i++)
	{
	    GetSectorStartEnd(&ulStartOff, &ulEndOff, i);
		if ( (ulOffset >= ulStartOff)
			&& (ulOffset <= ulEndOff) )
		{
			nSector = i;
			break;
		}
	}

	// if it is a valid sector, set it
	if ( (nSector >= 0) && (nSector < gNumSectors) )
		*pnSector = nSector;

	// else it is an invalid sector
	else
		return INVALID_SECTOR;

	// ok
	return NO_ERR;
}


//----------- G e t S e c t o r S t a r t E n d ( ) ----------//
//
//  PURPOSE
//  	Gets a sector start and end address based on the sector number.
//
//  INPUTS
//  	unsigned long *ulStartOff - pointer to the start offset
//		unsigned long *ulEndOff - pointer to the end offset
//		int nSector - sector number
//
//	RETURN VALUE
//		ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE GetSectorStartEnd( unsigned long *ulStartOff, unsigned long *ulEndOff, int nSector )
{

	if ( (FLASH_MANUFACTURER_BF50X4MBFLASH == g_ManId) && (FLASH_DEVICE_BF50X4MBFLASH == g_DevId) )
	{
		/* is it a main block?  these are 32K words(64K bytes) each */
		if( ( nSector >= 0 ) && ( nSector <= 62 ) )
		{
			*ulStartOff = nSector * (32*2048);
			*ulEndOff = (*ulStartOff) + (32*2048) - 1;
		}

		/* else is it a parameter block?  these are 4K words each */
		else if( ( nSector >= 63 ) && ( nSector <= 70 ) )
		{
			*ulStartOff = 0x3f0000 + ( (4*2048)*(nSector - 63) );
			*ulEndOff = (*ulStartOff) + (4*2048) - 1;
		}

		/* else invalid sector */
		else
			return INVALID_SECTOR;
	}

	/* else unsupported device */
	else
		return PROCESS_COMMAND_ERR;


	// ok
	return NO_ERR;
}


//----------- G e t F l a s h S t a r t A d d r e s s ( ) ----------//
//
//  PURPOSE
//  	Gets flash start address from an absolute address.
//
//  INPUTS
//  	unsigned long ulAddr - absolute address
//
//	RETURN VALUE
//		unsigned long - Flash start address

unsigned long GetFlashStartAddress( unsigned long ulAddr)
{

	ERROR_CODE 	  ErrorCode = NO_ERR;		//tells us if there was an error erasing flash
	unsigned long ulFlashStartAddr;			//flash start address
	unsigned long ulSectStartAddr;			//sector start address
	unsigned long ulSectEndAddr;			//sector end address
	unsigned long ulMask;					//offset mask

	// get flash start address from absolute address
	GetSectorStartEnd( &ulSectStartAddr, &ulSectEndAddr, 0);
	ulMask      	  = ~(ulSectEndAddr);
	ulFlashStartAddr  =  ulAddr & ulMask;

	return(ulFlashStartAddr);
}


//----------- R e a d F l a s h ( ) ----------//
//
//  PURPOSE
//  	Reads a value from an address in flash.
//
//  INPUTS
// 		unsigned long ulAddr - the address to read from
// 		int pnValue - pointer to store value read from flash
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE ReadFlash( unsigned long ulAddr, unsigned short *pusValue )
{

	// disable interrupts before performing the load or store operation
	// [refer warning: page 6-71 BF533 HRM]
	unsigned int uiSaveInts = cli();

	// set our flash address to where we want to read
	unsigned short *pFlashAddr = (unsigned short *)(ulAddr);

	// read the value
	*pusValue = (unsigned short)*pFlashAddr;

	// Enable Interrupts
    sti(uiSaveInts);

	// ok
	return NO_ERR;
}


//----------- W r i t e F l a s h ( ) ----------//
//
//  PURPOSE
//  	Write a value to an address in flash.
//
//  INPUTS
//	 	unsigned long  ulAddr - address to write to
//		unsigned short nValue - value to write
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE WriteFlash( unsigned long ulAddr, unsigned short usValue )
{

	// disable interrupts before performing the load or store operation
	// [refer warning: page 6-71 BF533 HRM]
	unsigned int uiSaveInts = cli();

	// set the address
	unsigned short *pFlashAddr = (unsigned short *)(ulAddr);

	*pFlashAddr = usValue;

	// Enable Interrupts
    sti(uiSaveInts);

	// ok
	return NO_ERR;
}


//----------- I s S t a t u s R e a d y ( ) ----------//
//
//  PURPOSE
//  	Checks if the status is ready.
//
//  INPUTS
// 		unsigned long ulOffset - the address
//
//	RETURN VALUE
//  	bool - returns TRUE if ready, FALSE otherwise

bool IsStatusReady( unsigned long ulOffset )
{
    unsigned short status = 0;

 	WriteFlash(ulOffset, 0x0070 );

    ReadFlash(ulOffset, &status );

    if( ( status & 0x80 ) == 0x80 )
    {
        WriteFlash(ulOffset, 0x0050 );
    	return true;
    }
    else
    	return false;
}

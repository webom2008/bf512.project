/*******************************************************************************/
/*                                                                             */
/*   (C) Copyright 2008 - Analog Devices, Inc.  All rights reserved.           */
/*                                                                             */
/*                                                                             */
/*    CHANGES:  1.00.0  - initial release    								   */
/*																			   */
/*    PURPOSE:  Performs operations specific to the M58WR032KB flash device    */
/*              while adhering to the adi device drivers model.                */
/*                                                                             */
/*******************************************************************************/

//---- I n c l u d e s  ----//

#include <drivers\flash\util.h>
#include <drivers\flash\adi_m58wr032kb.h>
#include <drivers\flash\Errors.h>


//---- c o n s t a n t   d e f i n i t i o n s -----//
static char   	*pEzKitTitle 	= "ADSP-BF526 EZ-KIT Lite";
static char 	*pFlashDesc		= NULL;
static char 	*pDeviceCompany	= "Numonyx";

static int		gNumSectors;		/* num sectors */
static int		g_ManId = 0x0;		/* flash manufacturer id */
static int		g_DevId = 0x0;		/* flash device id */

#define MAN_CODE_NUMONYX				0x20		/* Numonyx manufacturer flash id */
#define DEV_CODE_M58WR032KB				0x8815		/* M58WR032KB (32Mbit parallel flash) device id */

/* defines for BF526 EZ-KIT, if using a custom target you may need to change these */
#ifdef __ADSPBF526__
#define FLASH_START_ADDRESS				0x20000000	/* location of the start of flash */
#define PROTECTED_SECTOR_START_ADDRESS	0x203f0000	/* location of the protected sector */
#endif


//---- F u n c t i o n   P r o t o t y p e s  ----//
//            (for  Helper Functions)             //

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
static ERROR_CODE LockdownCfgDataSector(void);

// helper functions
static unsigned long GetFlashStartAddress( unsigned long ulAddr);
bool IsStatusReady( unsigned long ulOffset );


// Open a device
static u32 adi_pdd_Open(
	ADI_DEV_MANAGER_HANDLE	ManagerHandle,		// device manager handle
	u32 					DeviceNumber,		// device number
	ADI_DEV_DEVICE_HANDLE 	DeviceHandle,		// device handle
	ADI_DEV_PDD_HANDLE 		*pPDDHandle,		// pointer to PDD handle location
	ADI_DEV_DIRECTION 		Direction,			// data direction
	void					*pEnterCriticalArg,	// enter critical region parameter
	ADI_DMA_MANAGER_HANDLE	DMAHandle,			// handle to the DMA manager
	ADI_DCB_HANDLE			DCBHandle,			// callback handle
	ADI_DCB_CALLBACK_FN		DMCallback			// device manager callback function
);

// Close a device
static u32 adi_pdd_Close(
	ADI_DEV_PDD_HANDLE PDDHandle
);

// Read from a device
static u32 adi_pdd_Read(	ADI_DEV_PDD_HANDLE 	PDDHandle,
	  				ADI_DEV_BUFFER_TYPE BufferType,
 			  		ADI_DEV_BUFFER		*pBuffer);

// Write to a device
static u32 adi_pdd_Write(	ADI_DEV_PDD_HANDLE 	PDDHandle,
 			  		ADI_DEV_BUFFER_TYPE BufferType,
 			  		ADI_DEV_BUFFER		*pBuffer);

// Control the device
static u32 adi_pdd_Control(	ADI_DEV_PDD_HANDLE 	PDDHandle,
 			  			u32					Command,
 			  			void				*pArg);

ADI_DEV_PDD_ENTRY_POINT ADIM58WR032KBEntryPoint =
{
	adi_pdd_Open,
	adi_pdd_Close,
	adi_pdd_Read,
	adi_pdd_Write,
	adi_pdd_Control
};


// ---- P h y s i c a l   D e v i c e   D r i v e  A P I  f u n c t i o n s ----//

//----------- a d i _ p d d _ O p e n ( ) ----------//
//
//  PURPOSE
//  	This function opens the flash device for use.
//
//	INPUTS
//		ManagerHandle - device manager handle
//		DeviceNumber - device number
//		DeviceHandle - device handle
//		PDDHandle - This is the handle used to identify the device
//		Direction - data direction
//		*pEnterCriticalArg - enter critical region parameter
//		DMAHandle - handle to the DMA manager
//		DCBHandle - callback handle
//		DMCallback - device manager callback function
//
// 	RETURN VALUE
//  	Result

u32 adi_pdd_Open(								// Open a device
	ADI_DEV_MANAGER_HANDLE	ManagerHandle,		// device manager handle
	u32 					DeviceNumber,		// device number
	ADI_DEV_DEVICE_HANDLE 	DeviceHandle,		// device handle
	ADI_DEV_PDD_HANDLE 		*pPDDHandle,		// pointer to PDD handle location
	ADI_DEV_DIRECTION 		Direction,			// data direction
	void					*pEnterCriticalArg,	// enter critical region parameter
	ADI_DMA_MANAGER_HANDLE	DMAHandle,			// handle to the DMA manager
	ADI_DCB_HANDLE			DCBHandle,			// callback handle
	ADI_DCB_CALLBACK_FN		DMCallback	)		// device manager callback function
{
	u32 Result = NO_ERR;			// error code returned


	// check for errors if required
#ifdef ADI_M58WR032KB_ERROR_CHECKING_ENABLED
	if (DeviceNumber > 0)								// check the device number
		return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
	if (Direction != ADI_DEV_DIRECTION_BIDIRECTIONAL)	// check the direction
		return (ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED);
#endif

	/* make sure we are connecting to a supported part */
	GetCodes(&g_ManId, &g_DevId, FLASH_START_ADDRESS);
	if ( (MAN_CODE_NUMONYX != g_ManId) || (DEV_CODE_M58WR032KB != g_DevId) )
	{
		return UNKNOWN_COMMAND;
	}

	/* now that we know what we're connected to, update some variables */
	if ( DEV_CODE_M58WR032KB == g_DevId)
	{
		// 32 Mbit device
		gNumSectors = 71;
		pFlashDesc = "Numonyx M58WR032KB";
	}

	/* here we lock-down the cfg data sector which will contain hardware configuration data */
	Result = LockdownCfgDataSector();

	return (Result);
}


//----------- a d i _ p d d _ C l o s e  ( ) ----------//
//
//  PURPOSE
//  	This function closes the flash device.
//
//	INPUTS
//		PDDHandle - This is the handle used to identify the device
//
// 	RETURN VALUE
//  	Result

u32 adi_pdd_Close(ADI_DEV_PDD_HANDLE PDDHandle) // PDD handle
{
	return (NO_ERR);
}


//----------- a d i _ p d d _ R e a d  ( ) ----------//
//
//  PURPOSE
//  	Provides buffers to the flash device for reception
//		of inbound data.
//
//	INPUTS
//		PDDHandle - This is the handle used to identify the device
//		BufferType - This argument identifies the type of buffer: one-
//		  dimentional, two-dimensional or circular
//		*pBuffer - The is the address of the buffer or first buffer in
//		  a chain of buffers
//
// 	RETURN VALUE
//  	Result

u32 adi_pdd_Read(	ADI_DEV_PDD_HANDLE 	PDDHandle,
 			  		ADI_DEV_BUFFER_TYPE BufferType,
 			  		ADI_DEV_BUFFER		*pBuffer)
{

	ADI_DEV_1D_BUFFER *pBuff1D;
	unsigned short    *pusValue;
	unsigned long	  *pulAbsoluteAddr;
	u32		Result;

	// cast our buffer to a 1D buffer
	pBuff1D = (ADI_DEV_1D_BUFFER*)pBuffer;

	// cast our data buffer
	pusValue = (unsigned short *)pBuff1D->Data;

	// cast our offset
	pulAbsoluteAddr = (unsigned long *)pBuff1D->pAdditionalInfo;

	Result = ReadFlash( *pulAbsoluteAddr, pusValue );

	return(Result);
}


//----------- a d i _ p d d _ W r i t e  ( ) ----------//
//
//  PURPOSE
//  	Provides buffers to the flash device for transmission
//		of outbound data.
//
//	INPUTS
//		PDDHandle - This is the handle used to identify the device
//		BufferType - This argument identifies the type of buffer: one-
//		  dimentional, two-dimensional or circular
//		*pBuffer - The is the address of the buffer or first buffer in
//		  a chain of buffers
//
// 	RETURN VALUE
//  	Result

u32 adi_pdd_Write(	ADI_DEV_PDD_HANDLE 	PDDHandle,
 			  	 	ADI_DEV_BUFFER_TYPE BufferType,
 			  		ADI_DEV_BUFFER		*pBuffer)
{
	ADI_DEV_1D_BUFFER *pBuff1D;			// buffer pointer
	short   		  *psValue;			// stores the value to be written to flash
	unsigned long     *pulAbsoluteAddr;	// the absolute address to write
	unsigned long 	  ulFlashStartAddr;	// flash start address
	u32				  Result;			// error code returned

	// cast our buffer to a 1D buffer
	pBuff1D = (ADI_DEV_1D_BUFFER*)pBuffer;

	// cast our data buffer
	psValue = (short *)pBuff1D->Data;

	// cast our offset
	pulAbsoluteAddr = (unsigned long *)pBuff1D->pAdditionalInfo;

	// get flash start address from absolute address
	ulFlashStartAddr = GetFlashStartAddress(*pulAbsoluteAddr);

	// send the unlock command to the flash
	Result = WriteFlash( *pulAbsoluteAddr, 0x60 );
	Result = WriteFlash( *pulAbsoluteAddr, 0xD0 );

	Result = PollToggleBit(*pulAbsoluteAddr, PROGRAM_TIMEOUT);

	WriteFlash( *pulAbsoluteAddr, 0x40 );

	// program our actual value now
	Result = WriteFlash( *pulAbsoluteAddr, *psValue );

	// make sure the write was successful
	Result = PollToggleBit(*pulAbsoluteAddr, PROGRAM_TIMEOUT);

	return(Result);
}


//----------- a d i _ p d d _ C o n t r o l  ( ) ----------//
//
//  PURPOSE
//  	This function sets or detects a configuration parameter
//		for the flash device.
//
//	INPUTS
//		PDDHandle - This is the handle used to identify the device
//		Command - This is the command identifier
//		*pArg - The is the address of command-specific parameter
//
// 	RETURN VALUE
//  	Result

u32 adi_pdd_Control(	ADI_DEV_PDD_HANDLE 	PDDHandle,
 			  			u32					Command,
 			  			void				*pArg)
{

	ERROR_CODE ErrorCode = NO_ERR;

	COMMAND_STRUCT *pCmdStruct = (COMMAND_STRUCT *)pArg;

	// switch on the command
	switch ( Command )
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
			pCmdStruct->SGetDesc.pTitle = pEzKitTitle;
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

		// turn on dataflow (command required of all device drivers)
		case ADI_DEV_CMD_SET_DATAFLOW:
		    ErrorCode = NO_ERR;
		    break;

		// set dataflow method (command required of all device drivers)
		case ADI_DEV_CMD_SET_DATAFLOW_METHOD:
			ErrorCode = NO_ERR;
		    break;

		// get peripheral DMA support (command required of all device drivers)
		case ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT:
		    (*(u32 *)pArg) = FALSE; // no, this device is not supported by peripheral DMA
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
	
	// reset will undo the lockdown, so we must lockdown again here
	LockdownCfgDataSector();

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

	ulMask      	  = 0xFFFFFF;
	ulOffset		  = ulAddr & ulMask;

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

	if ( (MAN_CODE_NUMONYX == g_ManId) && (DEV_CODE_M58WR032KB == g_DevId) )
	{
		/* is it a parameter block?  these are 8KB each */
		if( ( nSector >= 0 ) && ( nSector <= 7 ) )
		{
			*ulStartOff = nSector * (8*1024);
			*ulEndOff = ( (*ulStartOff) + (8*1024) );
			*ulEndOff -= 1;
		}

		/* else is it a main block?  these are 64KB each */
		else if( ( nSector >= 8 ) && ( nSector < gNumSectors ) )
		{
			*ulStartOff = 0x10000 + ( (64*1024)*(nSector - 8) );
			*ulEndOff = ( (*ulStartOff) + (64*1024) );
			*ulEndOff -= 1;
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


//----------- L o c k d o w n C f g D a t a S e c t o r ( ) ----------//
//
//  PURPOSE
//  	Puts the configuration data sector in lockdown.
//
//  INPUTS
// 		none
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE LockdownCfgDataSector()
{
	ERROR_CODE ErrorCode = NO_ERR;
	
	ErrorCode = WriteFlash( PROTECTED_SECTOR_START_ADDRESS, 0x60 );
	ErrorCode = WriteFlash( PROTECTED_SECTOR_START_ADDRESS, 0x2F );
	ErrorCode = PollToggleBit(PROTECTED_SECTOR_START_ADDRESS, PROGRAM_TIMEOUT);

	return ErrorCode;
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
    	return TRUE;
    }
    else
    	return FALSE;
}

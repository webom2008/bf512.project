/*******************************************************************************
*
*   (C) Copyright 2004 - Analog Devices, Inc.  All rights reserved.
*
*    FILE:     a d i _ m 2  9 w 3 2 0 . c ( )
*
*    CHANGES:  1.00.0  - initial release
*			   1.00.1  - updated to work with EB and DB parts
*			   1.00.2  - added a new error to give info about sectors
*			   			 that are not accessible for one reason or another
*
*    PURPOSE:  Performs operations specific to the M29W320 flash device
*              while adhering to the adi device drivers model.
*
*******************************************************************************/

//---- I n c l u d e s  ----//

#include <drivers\flash\util.h>
#include <drivers\flash\adi_m29w320.h>
#include <drivers\flash\Errors.h>


//---- c o n s t a n t   d e f i n i t i o n s -----//
#ifdef __ADSPBF538__
static char   	*pEzKitTitle 	= 	"ADSP-BF538F EZ-KIT Lite";
#elif __ADSPBF537__
static char   	*pEzKitTitle 	= 	"ADSP-BF537 EZ-KIT Lite";
#elif __ADSPBF527__
static char   	*pEzKitTitle 	= 	"ADSP-BF527 EZ-KIT Lite";
#else
// we should not fall in here
#endif

static char 	*pFlashDesc;
static char 	*pDeviceCompany	=	"STMicroelectronics";

static int		gNumSectors;

//---- F u n c t i o n   P r o t o t y p e s  ----//
//            (for  Helper Functions)             //

static ERROR_CODE EraseFlash(unsigned long ulStartAddr);
static ERROR_CODE EraseBlock( int nBlock, unsigned long ulStartAddr );
static ERROR_CODE GetCodes(int *pnManCode, int *pnDevCode, unsigned long ulStartAddr);
static ERROR_CODE GetSectorNumber( unsigned long ulAddr, int *pnSector );
static ERROR_CODE GetSectorStartEnd( unsigned long *ulStartOff, unsigned long *ulEndOff, int nSector );
static ERROR_CODE PollToggleBit(unsigned long ulOffset);
static ERROR_CODE ReadFlash(unsigned long ulOffset, unsigned short *pusValue );
static ERROR_CODE ResetFlash(unsigned long ulStartAddr);
static ERROR_CODE WriteFlash(unsigned long ulOffset, unsigned short usValue );
static unsigned long GetFlashStartAddress( unsigned long ulAddr);


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

ADI_DEV_PDD_ENTRY_POINT ADIM29W320EntryPoint =
{
	adi_pdd_Open,
	adi_pdd_Close,
	adi_pdd_Read,
	adi_pdd_Write,
	adi_pdd_Control
};


// ---- P h y s i c a l   D e v i c e   D r i v e  A P I  f u n c t i o n s ----//

//----------- a d i _ p d d _ C l o s e  ( ) ----------//
//
//  PURPOSE
//  	This function opens the M29W320 flash device for use.
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

	// check for errors if required
#ifdef ADI_M29W320_ERROR_CHECKING_ENABLED
	if (DeviceNumber > 0)								// check the device number
		return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
	if (Direction != ADI_DEV_DIRECTION_BIDIRECTIONAL)	// check the direction
		return (ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED);
#endif

	return (NO_ERR);

}


//----------- a d i _ p d d _ C l o s e  ( ) ----------//
//
//  PURPOSE
//  	This function closes the M29W320 flash device.
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
//  	Provides buffers to the M29W320 flash device for reception
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
//  	Provides buffers to the M29W320 flash device for transmission
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

	// unlock flash
	WriteFlash( ulFlashStartAddr + 0x0AAA, 0xaa );
	WriteFlash( ulFlashStartAddr + 0x0554, 0x55 );
	WriteFlash( ulFlashStartAddr + 0x0AAA, 0xa0 );

	// program our actual value now
	Result = WriteFlash( *pulAbsoluteAddr, *psValue );

	// make sure the write was successful
	Result = PollToggleBit(*pulAbsoluteAddr);

	return(Result);
}


//----------- a d i _ p d d _ C o n t r o l  ( ) ----------//
//
//  PURPOSE
//  	This function sets or detects a configuration parameter
//		for the M29W320 flash device.
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
			ErrorCode = EraseBlock( pCmdStruct->SEraseSect.nSectorNum, pCmdStruct->SEraseSect.ulFlashStartAddr );
			break;

		// get manufacturer and device codes
		case CNTRL_GET_CODES:
			ErrorCode = GetCodes((int *)pCmdStruct->SGetCodes.pManCode, (int *)pCmdStruct->SGetCodes.pDevCode, (unsigned long)pCmdStruct->SGetCodes.ulFlashStartAddr);
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

		case ADI_DEV_CMD_SET_DATAFLOW_METHOD:
		    // Do nothing & simply return back for these commands
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
	// send the reset command to the flash
	WriteFlash( ulAddr + 0x0554, 0xf0 );

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
	int nBlock = 0;					// index for each block to erase

 #ifdef LOCK_MAC_ADDR_SECTOR

	// do an erase block here because the MAC address is contained
	// at address 0x201F8000(0x203F0000 in a memory window) in FLASH
	for( nBlock = 0; ( nBlock < (gNumSectors - 1) ) && ( ErrorCode == NO_ERR ); nBlock++ )
		ErrorCode = EraseBlock( nBlock, ulAddr );

 #else

	// erase contents of Flash
	WriteFlash( ulAddr + 0x0AAA, 0xaa );
	WriteFlash( ulAddr + 0x0554, 0x55 );
	WriteFlash( ulAddr + 0x0AAA, 0x80 );
	WriteFlash( ulAddr + 0x0AAA, 0xaa );
	WriteFlash( ulAddr + 0x0554, 0x55 );
	WriteFlash( ulAddr + 0x0AAA, 0x10 );

	// poll until the command has completed
	ErrorCode = PollToggleBit(ulAddr + 0x0000);

 #endif

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

#ifdef LOCK_MAC_ADDR_SECTOR

	if ( (nBlock < 0) || (nBlock > gNumSectors)  )
		return INVALID_BLOCK;

	// we do not allow an erase of the last sector because the
	// MAC address is stored at this location
	if ( nBlock == (gNumSectors - 1)  )
		return NO_ACCESS_SECTOR;

#else

	if ( (nBlock < 0) || (nBlock > gNumSectors)  )
		return INVALID_BLOCK;

#endif

	// Get the sector start offset
	// we get the end offset too however we do not actually use it for Erase sector
	GetSectorStartEnd( &ulSectStart, &ulSectEnd, nBlock );

	// send the erase block command to the flash
	WriteFlash( (ulAddr + 0x0AAA), 0xaa );
	WriteFlash( (ulAddr + 0x0554), 0x55 );
	WriteFlash( (ulAddr + 0x0AAA), 0x80 );
	WriteFlash( (ulAddr + 0x0AAA), 0xaa );
	WriteFlash( (ulAddr + 0x0554), 0x55 );

	// the last write has to be at an address in the block
	WriteFlash( (ulAddr + ulSectStart), 0x30 );

	// poll until the command has completed
	ErrorCode = PollToggleBit(ulAddr + ulSectStart);

 	// block erase should be complete
	return ErrorCode;
}


//----------- P o l l T o g g l e B i t ( ) ----------//
//
//  PURPOSE
//  	Polls the toggle bit in the flash to see when the operation
//		is complete.
//
//	INPUTS
//	unsigned long ulAddr - address in flash
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise


ERROR_CODE PollToggleBit(unsigned long ulAddr)
{
	ERROR_CODE ErrorCode = NO_ERR;	// flag to indicate error
	unsigned short sVal1;
	unsigned short sVal2;

	// read flash 1 time
	ReadFlash( ulAddr, &sVal1 );

	while( ErrorCode == NO_ERR )
	{
		// read the value 2 times
		ReadFlash( ulAddr, &sVal1 );
		ReadFlash( ulAddr, &sVal2 );

		// XOR to see if any bits are different
		sVal1 ^= sVal2;

		// see if we are toggling
		if( !(sVal1 & 0x40) )
			break;

		// check error bit
		if( !(sVal2 & 0x20) )
			continue;
		else
		{
			// read the value 2 times
			ReadFlash( ulAddr, &sVal1 );
			ReadFlash( ulAddr, &sVal2 );

			// XOR to see if any bits are different
			sVal1 ^= sVal2;

			// see if we are toggling
			if( !(sVal1 & 0x40) )
				break;
			else
			{
				ErrorCode = POLL_TIMEOUT;
				ResetFlash(ulAddr);
			}
		}
	}

	// we can return
	return ErrorCode;
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
 	// send the auto select command to the flash
	WriteFlash( ulAddr + 0x0AAA, 0xaa );
	WriteFlash( ulAddr + 0x0554, 0x55 );
	WriteFlash( ulAddr + 0x0AAA, 0x90 );

 	// now we can read the codes
	ReadFlash( ulAddr + 0x0400,(unsigned short *)pnManCode );
	*pnManCode &= 0x00FF;

	ReadFlash( ulAddr + 0x0402, (unsigned short *)pnDevCode );
	*pnDevCode &= 0xFFFF;

	// if it is the M29W320DB
	if( *pnDevCode == 0x22CB )
	{
	    gNumSectors = 67;
	  	pFlashDesc =	"STMicro. M29W320DB";
	}
	else if( *pnDevCode == 0x2257 )
	{
	    gNumSectors = 71;
		pFlashDesc =	"STMicro. M29W320EB";
	}
	else
		return PROCESS_COMMAND_ERR;

	// we need to issue another command to get the part out
	// of auto select mode so issue a reset which just puts
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
	int i;
	int error_code = 1;
	unsigned long ulMask;					//offset mask
	unsigned long ulOffset;					//offset
	unsigned long ulStartOff;
	unsigned long ulEndOff;

	ulMask      	  = 0x3fffff;
	ulOffset		  = ulAddr & ulMask;

	for(i = 0; i < gNumSectors; i++)
	{
	    GetSectorStartEnd(&ulStartOff, &ulEndOff, i);
		if ( (ulOffset >= ulStartOff)
			&& (ulOffset <= ulEndOff) )
		{
			error_code = 0;
			nSector = i;
			break;
		}
	}

	// if it is a valid sector, set it
	if (error_code == 0)
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
	long lSectorSize = 0;
	long lMainStart = 0x10000;

	// The blocks in the flash memory are asymmetrically arranged
	// Thus we use block sizes to determine the block addresses
	unsigned long  ulBootBlkSize 	= 0x4000;	//size of Boot Block
	unsigned long  ulParamBlkSize	= 0x2000;	//size of Parameter Block
	unsigned long  ulSmMainBlkSize 	= 0x8000;	//size of Small Main Block
	unsigned long  ulMainBlkSize	= 0x10000;	//size of Main Block
	int nNumEBParamBlocks = 8;

	// if it is the M29320DB
	if( gNumSectors == 67 )
	{

		// M29W320DB Bottom Boot Block Addresses (x8)
		// Boot Block-used for small initilization code
		if( nSector == 0 )
		{
			*ulStartOff = 0x0000;
			*ulEndOff = ( (*ulStartOff) + (ulBootBlkSize ) );
		}
		else if( nSector == 1 ) //Parameter Block
		{
			*ulStartOff = 0x4000;
			*ulEndOff = ( (*ulStartOff) + (ulParamBlkSize) );
		}
		else if( nSector == 2 ) //Parameter Block
		{
			*ulStartOff = 0x6000;
			*ulEndOff = ( (*ulStartOff) + (ulParamBlkSize) );
		}
		else if( nSector == 3 ) //Small Main Block-where applications may be stored
		{
			*ulStartOff = 0x8000;
			*ulEndOff = ( (*ulStartOff) + (ulSmMainBlkSize) );
		}
		else if( ( nSector >= 4 ) && ( nSector < gNumSectors ) ) // 64 Main Blocks
		{
			*ulStartOff = lMainStart + ( ( nSector - 4 ) * ulMainBlkSize );
			*ulEndOff = ( (*ulStartOff) + ulMainBlkSize );
		}
		// no such sector
		else
			return INVALID_SECTOR;
	}
	// M29W320EB
	else
	{
		if( (nSector >= 0) && (nSector < nNumEBParamBlocks) )
		{
			*ulStartOff = nSector * ulParamBlkSize;
			*ulEndOff = ( (*ulStartOff) + (ulParamBlkSize ) );
		}
		else if( ( nSector >= nNumEBParamBlocks ) && ( nSector < gNumSectors ) ) // 64 Main Blocks
		{
			*ulStartOff = lMainStart + ( ( nSector - nNumEBParamBlocks ) * ulMainBlkSize );
			*ulEndOff = ( (*ulStartOff) + ulMainBlkSize );
		}
		// no such sector
		else
			return INVALID_SECTOR;
	}

	*ulEndOff -= 1;

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

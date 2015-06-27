/*******************************************************************************/
/*                                                                             */
/*   (C) Copyright 2008 - Analog Devices, Inc.  All rights reserved.           */
/*                                                                             */
/*    FILE:     adi_sst25wf040.c                                               */
/*																			   */
/*    PURPOSE:  Performs operations specific to the SST25WF040 flash           */
/*              device while adhering to the adi device drivers model.         */
/*                                                                             */
/*******************************************************************************/

/* includes */
#ifdef __ADSPBF526__
	#include <cdefBF526.h>
#endif
#include <ccblkfn.h>
#include <stdio.h>
#include <drivers\flash\util.h>
#include <drivers\flash\adi_sst25wf040.h>
#include <drivers\flash\Errors.h>

/* constants */
static char 	*pFlashDesc = "SST25WF0x0";
static char 	*pDeviceCompany	= "SST, Inc.";
static int		gNumSectors;
static int		g_ManId = 0x0;
static int		g_DevId = 0x0;

#define MAN_CODE_SST	0xbf		/* SST, Inc. */
#define DEV_CODE_040	0x04		/* SST25WF040 (4Mbit SPI flash) */
#define DEV_CODE_020	0x03		/* SST25WF020 (2Mbit SPI flash) */
#define DEV_CODE_010	0x02		/* SST25WF010 (1Mbit SPI flash) */
#define DEV_CODE_512	0x01		/* SST25WF512 (512Kbit SPI flash) */

#define START_ADDRESS_020	0x0
#define END_ADDRESS_020 	0x0001ffff	/* 128 KB */
#define START_ADDRESS_040 	0x0
#define END_ADDRESS_040		0x0007ffff	/* 512 KB */

#undef TIMEOUT
#undef DELAY

/* flash commands */
#define SPI_WREN            (0x06)  /* Set Write Enable Latch */
#define SPI_WRDI            (0x04)  /* Reset Write Enable Latch */
#define SPI_RDID            (0x9F)  /* Read Identification */
#define SPI_RDSR            (0x05)  /* Read Status Register */
#define SPI_WRSR            (0x01)  /* Write Status Register */
#define SPI_READ            (0x03)  /* Read data from memory */
#define SPI_FAST_READ       (0x0B)  /* Read data from memory */
#define SPI_BYTE_PROGRAM    (0x02)  /* Program Data into memory */
#define SPI_ERASE_4KB       (0x20)  /* Erase one sector in memory */
#define SPI_ERASE_32KB      (0x52)  /* Erase one 32 KB block */
#define SPI_ERASE_64KB      (0xD8)  /* Erase one 64 KB block */
#define SPI_CHIP_ERASE      (0xC7)  /* Erase all memory */

/* bit masks */
#define WIP                  (0x1)	/* Check the write in progress bit of the SPI status register */
#define WEL                  (0x2)	/* Check the write enable bit of the SPI status register */

#define SPI_PAGE_SIZE		(528)
#define SPI_SECTORS		    (512)
#define SPI_SECTOR_SIZE		(4224)
#define SPI_SECTOR_DIFF		(3968)
#define PAGE_BITS			(10)
#define PAGE_SIZE_DIFF		(496)

#define DELAY				300
#define TIMEOUT        35000*64

char SPI_Page_Buffer[SPI_PAGE_SIZE];
int SPI_Page_Index = 0;
char SPI_Status;


/* function prototypes */

static ERROR_CODE SetupForFlash();
static ERROR_CODE GetCodes();
static ERROR_CODE Wait_For_nStatus(void);
ERROR_CODE Wait_For_Status( char Statusbit );
static ERROR_CODE Wait_For_WEL(void);
char ReadStatusRegister(void);
void Wait_For_SPIF(void);
extern void SetupSPI( const int spi_setting );
extern void SPI_OFF(void);
void SendSingleCommand( const int iCommand );
unsigned long DataFlashAddress (unsigned long address);

static ERROR_CODE EraseFlash( void );
static ERROR_CODE EraseBlock( int nBlock, unsigned long ulStartAddr );
static ERROR_CODE GetCodes( int *pnManCode, int *pnDevCode );
static ERROR_CODE GetSectorNumber( unsigned long ulAddr, int *pnSector );
static ERROR_CODE GetSectorStartEnd( unsigned long *ulStartOff, unsigned long *ulEndOff, int nSector );
static ERROR_CODE PollToggleBit( void );
static ERROR_CODE ReadFlash( unsigned short *pusValue );
static ERROR_CODE ResetFlash( void );
static ERROR_CODE WriteFlash( unsigned short usValue );


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

ADI_DEV_PDD_ENTRY_POINT ADISST25WF040EntryPoint =
{
	adi_pdd_Open,
	adi_pdd_Close,
	adi_pdd_Read,
	adi_pdd_Write,
	adi_pdd_Control
};


// ---- P h y s i c a l   D e v i c e   D r i v e  A P I  f u n c t i o n s ----//

//----------- a d i _ p d d _ O p e n  ( ) ----------//
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

	// check for errors if required
#ifdef ADI_SST25WF040_ERROR_CHECKING_ENABLED
	if (DeviceNumber > 0)								// check the device number
		return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
	if (Direction != ADI_DEV_DIRECTION_BIDIRECTIONAL)	// check the direction
		return (ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED);
#endif


#ifdef __ADSPBF526__
	// enable writes
	
	char status_register = 0, status_register2 = 0;
	status_register = ReadStatusRegister();
	status_register &= 0xe3;	// clear BP0 and BP1 to allow writes
	
	SendSingleCommand(SPI_WREN);
	
	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) ); // Turn on the SPI

	//send instruction to read status register
	WriteFlash( SPI_WRSR );

	// write the status register
	WriteFlash( status_register );

	SPI_OFF();		// Turn off the SPI
	
	status_register2 = ReadStatusRegister();

#endif

	/* make sure we are connecting to a supported part */
	GetCodes(&g_ManId, &g_DevId);
	if ( (MAN_CODE_SST != g_ManId) || ((DEV_CODE_020 != g_DevId) && (DEV_CODE_040 != g_DevId)) )
	{
		return UNKNOWN_COMMAND;	
	}
	
	/* now that we know what we're connected to, update some variables */
	if ( DEV_CODE_020 == g_DevId)
	{
		gNumSectors = 32;		// thirty-two 4KB sectors
		pFlashDesc[8] = '2';	// change 'x' to '2'
	}
	else if ( DEV_CODE_040 == g_DevId)
	{
		gNumSectors = 128;		// one-hundred-twenty-eight 4KB sectors
		pFlashDesc[8] = '4';	// change 'x' to '4'
	}
	
	return (NO_ERR);

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

	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

	
	int n;

	for(n=0; n<1000; n++)
	{
		asm("nop;");
	}
	
	
	//send the bulk erase command to the flash
	WriteFlash( SPI_READ );
	WriteFlash( *pulAbsoluteAddr >> 16);
	WriteFlash( *pulAbsoluteAddr >> 8);
	WriteFlash( *pulAbsoluteAddr );

	// read our actual value now
	Result = ReadFlash( pusValue );

	SPI_OFF();


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
	ulFlashStartAddr = *pulAbsoluteAddr;	
	int n;

	for(n=0; n<1000; n++)
	{
		asm("nop;");
	}
	
	
	// First, a Write Enable Command must be sent to the SPI.
	SendSingleCommand(SPI_WREN);

	// Second, the SPI Status Register will be tested whether the
	//         Write Enable Bit has been set.
	Result = Wait_For_WEL();

	if( POLL_TIMEOUT == Result )
	{
		/* if we timed out, try it again */
		for (n = 0; n < 3; n++)
		{
			// First, a Write Enable Command must be sent to the SPI.
			SendSingleCommand(SPI_WREN);
			
			// Second, the SPI Status Register will be tested whether the
			//         Write Enable Bit has been set.
			Result = Wait_For_WEL();
			
			if ( POLL_TIMEOUT != Result )
				break;
		}
	}
	
	if( POLL_TIMEOUT == Result )
		return Result;
	else
	{
		SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

		//send the bulk erase command to the flash
		WriteFlash( SPI_BYTE_PROGRAM );
		WriteFlash( ulFlashStartAddr >> 16);
		WriteFlash( ulFlashStartAddr >> 8);
		WriteFlash( ulFlashStartAddr );

		// program our actual value now
		Result = WriteFlash( *psValue );

		SPI_OFF();

		return(Result);
	}


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
			ErrorCode = EraseFlash();
			break;

		// erase sector
		case CNTRL_ERASE_SECT:
			ErrorCode = EraseBlock( pCmdStruct->SEraseSect.nSectorNum, pCmdStruct->SEraseSect.ulFlashStartAddr );
			break;

		// get manufacturer and device codes
		case CNTRL_GET_CODES:
			*(int *)pCmdStruct->SGetCodes.pManCode = g_ManId;
			*(int *)pCmdStruct->SGetCodes.pDevCode = g_DevId;
			break;

		// get flash width
		case CNTRL_GET_FLASH_WIDTH:
			*(int *)pCmdStruct->SGetFlashWidth.pnFlashWidth = _FLASH_WIDTH_8;
			ErrorCode = NO_ERR;
			break;
			
		// supports CFI?
		case CNTRL_SUPPORTS_CFI:
			*(bool *)pCmdStruct->SSupportsCFI.pbSupportsCFI = false;
			ErrorCode = NO_ERR;
			break;
			
		case CNTRL_GET_DESC:
			//Filling the contents with data
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
			ErrorCode = ResetFlash();
			break;

		// turn on dataflow (command required of all device drivers)
		case ADI_DEV_CMD_SET_DATAFLOW:
		    ErrorCode = NO_ERR;
		    break;

		case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):
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
//		none
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE ResetFlash(void)
{


//	char status_register = 0;
	ERROR_CODE ErrorCode = NO_ERR;

	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

	//send the bulk erase command to the flash
	WriteFlash( SPI_WRDI );

	SPI_OFF();

	// poll until the command has completed
	ErrorCode = PollToggleBit() ;


	// reset should be complete
	return ErrorCode;
}


//----------- E r a s e F l a s h  ( ) ----------//
//
//  PURPOSE
//  	Sends an "erase all" command to the flash.
//
//	INPUTS
//		none
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE EraseFlash(void)
{
	ERROR_CODE ErrorCode = NO_ERR;	// tells us if there was an error erasing flash
	int nBlock = 0, n;					// index for each block to erase

	//A write enable instruction must previously have been executed
	SendSingleCommand(SPI_WREN);

	//The status register will be polled to check the write enable latch "WREN"
	ErrorCode = Wait_For_WEL();

	if( POLL_TIMEOUT == ErrorCode )
	{
		return ErrorCode;
	}
	else
	{
	    //The bulk erase instruction will erase the whole flash
		SendSingleCommand(SPI_CHIP_ERASE);

		// Erasing the whole flash will take time, so the following bit must be polled.
		//The status register will be polled to check the write in progress bit "WIP"
		ErrorCode = Wait_For_Status(WIP);

		printf("Error Code: %d", ErrorCode);


	}

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
	unsigned long ShiftValue;

	// get flash start address from absolute address
	// The ulAddr should ideally be pointing to the flash start
	// address. However we just verify it here again.
	ulFlashStartAddr = ulAddr;

	// Get the sector start offset
	// we get the end offset too however we do not actually use it for Erase sector
	GetSectorStartEnd( &ulSectStart, &ulSectEnd, nBlock );


	// Poll the status register to check the Write in Progress bit
	// Sector erase takes time
//	ErrorCode = Wait_For_Status(WIP);

	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

	// send the write enable instruction
	WriteFlash( SPI_WREN );

	SPI_OFF();

	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

	//send the erase block command to the flash
	WriteFlash( SPI_ERASE_4KB );
	WriteFlash( (ulFlashStartAddr + ulSectStart) >> 16);
	WriteFlash( (ulFlashStartAddr + ulSectStart) >> 8);
	WriteFlash( ulFlashStartAddr + ulSectStart );

	// Poll the status register to check the Write in Progress bit
	// Sector erase takes time
	ErrorCode = Wait_For_Status(WIP);

	SPI_OFF();

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


ERROR_CODE PollToggleBit(void)
{
	ERROR_CODE ErrorCode = NO_ERR;	// flag to indicate error
	char status_register = 0;
	int n, i;

	for(i = 0; i < 500; i++)
	{
		status_register = ReadStatusRegister();
		if( (status_register & WEL) )
		{
			ErrorCode = NO_ERR;

		}
		ErrorCode = POLL_TIMEOUT;	// Time out error
	};

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
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE GetCodes(int *pnManCode, int *pnDevCode)
{
	// open the SPI, Deasserting CS
	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

	// write the OpCode and Write address, 4 bytes.
	WriteFlash( SPI_RDID );

	// now we can read the codes
	ReadFlash( (unsigned short *)pnManCode );
	*pnManCode &= 0x00FF;

	// write the OpCode and Write address, 4 bytes.
	WriteFlash( SPI_RDID );

	ReadFlash( (unsigned short *)pnDevCode );
	*pnDevCode &= 0x00FF;

	SPI_OFF();
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
	
	if ( MAN_CODE_SST == g_ManId )
	{
		if ( DEV_CODE_020 == g_DevId )
		{
			if ( ulAddr <= END_ADDRESS_020 )
			{
				nSector = (int)(ulAddr / (4*1024));
				*pnSector = nSector;
			}
			else
			{
				return INVALID_SECTOR;
			}
		}
		
		else if ( DEV_CODE_040 == g_DevId )	
		{
			if ( ulAddr <= END_ADDRESS_040 )
			{
				nSector = (int)(ulAddr / (4*1024));
				*pnSector = nSector;
			}
			else
			{
				return INVALID_SECTOR;
			}
		}		
	}

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
	// if it's a valid sector calculate addresses
	if( ( nSector >= 0 ) && ( nSector < gNumSectors ) )
	{
		*ulStartOff = nSector * (4*1024);				// 4KB sectors
		*ulEndOff = ( (*ulStartOff) + (4*1024) ) - 1;
	}
	else
		return INVALID_SECTOR;

	// ok
	return NO_ERR;
}


//----------- R e a d F l a s h ( ) ----------//
//
//  PURPOSE
//  	Reads a value from an address in flash.
//
//  INPUTS
// 		int pnValue - pointer to store value read from flash
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE ReadFlash( unsigned short *pusValue )
{

	// disable interrupts before performing the load or store operation
	unsigned long  ulDummyRead, ulDummyWrite=0;
#ifdef __ADSPBF527__
	ulDummyRead  = *pSPI_RDBR;
	Wait_For_SPIF();

	*pSPI_TDBR = ulDummyWrite;
	Wait_For_SPIF();
	*pusValue =  *pSPI_RDBR;
#elif __ADSPBF526__
	ulDummyRead  = *pSPI_RDBR;
	Wait_For_SPIF();

	*pSPI_TDBR = ulDummyWrite;
	Wait_For_SPIF();
	*pusValue =  *pSPI_RDBR;
#elif __ADSPBF548__
        ulDummyRead  = *pSPI0_RDBR;
	Wait_For_SPIF();

	*pSPI0_TDBR = ulDummyWrite;
	Wait_For_SPIF();
	*pusValue =  *pSPI0_RDBR;
#endif
	// ok
	return NO_ERR;
}


//----------- W r i t e F l a s h ( ) ----------//
//
//  PURPOSE
//  	Write a value to an address in flash.
//
//  INPUTS
//		unsigned short nValue - value to write
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE WriteFlash( unsigned short usValue )
{
#ifdef __ADSPBF527__
        *pSPI_TDBR = usValue;
#elif __ADSPBF526__
        *pSPI_TDBR = usValue;
#elif __ADSPBF548__
	*pSPI0_TDBR = usValue;
#endif
	Wait_For_SPIF();

	// ok
	return NO_ERR;
}


//////////////////////////////////////////////////////////////
// int ReadStatusRegister()
//
// Returns the 8-bit value of the status register.
// Inputs - none
// returns- second location of status_register[2],
//         first location is garbage.
// Core sends the command
//
//////////////////////////////////////////////////////////////
char ReadStatusRegister(void)
{

   	char status_register = 0;
   	unsigned short usStatus;
	int n;
    unsigned long ulFlashStartAddr;		//flash start address

	// get flash start address from absolute address
	// The ulAddr should ideally be pointing to the flash start
	// address. However we just verify it here again.
	ulFlashStartAddr = 0x0;

	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) ); // Turn on the SPI

	//send instruction to read status register
	WriteFlash( SPI_RDSR );

	// receive the status register
	ReadFlash( &usStatus );

	SPI_OFF();		// Turn off the SPI

	return usStatus;
}

//////////////////////////////////////////////////////////////
// void Wait_For_SPIF()
//
// Polls the SPIF (SPI single word transfer complete) bit
// of SPISTAT until the transfer is complete.
// Inputs - none
// returns- none
//
//////////////////////////////////////////////////////////////
void Wait_For_SPIF(void)
{
	int n;

	for(n=0; n<DELAY; n++)
	{
		asm("nop;");
	}

	while(1)
	{
#ifdef __ADSPBF527__
        unsigned short iTest = *pSPI_STAT;
#elif __ADSPBF526__
	unsigned short iTest = *pSPI_STAT;
#elif __ADSPBF548__
	unsigned short iTest = *pSPI0_STAT;
#endif
		if( (iTest & SPIF) )
		{
			break;
		}
	}
}


//////////////////////////////////////////////////////////////
// Wait_For_WEL()
//
// Polls the WEL (Write Enable Latch) bit of the Flash's status
// register.
// Inputs - none
// returns- none
//
//////////////////////////////////////////////////////////////

ERROR_CODE Wait_For_WEL(void)
{
	int n, i;
	char status_register = 0;
	ERROR_CODE ErrorCode = NO_ERR;	// tells us if there was an error erasing flash

		for(i = 0; i < 35; i++)
		{
			status_register = ReadStatusRegister();
			if( (status_register & WEL) )
			{
				ErrorCode = NO_ERR;	// tells us if there was an error erasing flash
				break;
			}

			for(n=0; n<DELAY; n++)
				asm("nop;");
			ErrorCode = POLL_TIMEOUT;	// Time out error
		};


	return ErrorCode;
}
//////////////////////////////////////////////////////////////
// Wait_For_Status()
//
// Polls the Status Register of the Flash's status
// register until the Flash is finished with its access. Accesses
// that are affected by a latency are Page_Program, Sector_Erase,
// and Block_Erase.
// Inputs - Statusbit
// returns- none
//
//////////////////////////////////////////////////////////////

ERROR_CODE Wait_For_Status( char Statusbit )
{
	int n, i;
	char status_register = 0xFF;
	ERROR_CODE ErrorCode = NO_ERR;	// tells us if there was an error erasing flash

		for(i = 0; i < TIMEOUT; i++)
		{
			status_register = ReadStatusRegister();
			if( !(status_register & Statusbit) )
			{
				ErrorCode = NO_ERR;	// tells us if there was an error erasing flash
				break;
			}

			for(n=0; n<DELAY; n++)
				asm("nop;");
			ErrorCode = POLL_TIMEOUT;	// Time out error
		};


	return ErrorCode;

}

//////////////////////////////////////////////////////////////
// SendSingleCommand()
//
// Sends a single command to the SPI flash
// inputs - the 8-bit command to send
// returns- none
//
//////////////////////////////////////////////////////////////
void SendSingleCommand( const int iCommand )
{
	int n;

	//turns on the SPI in single write mode
	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

	//sends the actual command to the SPI TX register
#ifdef __ADSPBF527__
	*pSPI_TDBR = iCommand;
#elif __ADSPBF526__
	*pSPI_TDBR = iCommand;
#elif __ADSPBF548__
	*pSPI0_TDBR = iCommand;
#endif


	//The SPI status register will be polled to check the SPIF bit
	Wait_For_SPIF();

	//The SPI will be turned off
	SPI_OFF();

	//Pause before continuing
	for(n=0; n<DELAY; n++)
	{
		asm("nop;");
	}
}

//////////////////////////////////////////////////////////////
// Sets up the SPI for mode specified in spi_setting
// Inputs - spi_setting
// returns- none
//////////////////////////////////////////////////////////////
void SetupSPI( const int spi_setting )
{
    int i;

#ifdef __ADSPBF527__
  	*pPORTG_FER   |= 0x1C;
	*pPORTG_MUX   &= 0xFC;
  	*pPORTGIO_DIR |= PG1;
   	*pPORTGIO_SET = PG1;

   	for(i=0; i<DELAY; i++)
	{
		asm("nop;");
	}

	*pPORTGIO_CLEAR = PG1;

	*pSPI_BAUD = BAUD_RATE_DIVISOR;
	*pSPI_CTL = spi_setting;
#elif __ADSPBF526__
  	*pPORTG_FER   |= 0x1C;
	*pPORTG_MUX   = 0x0;
  	*pPORTGIO_DIR |= PG1;
   	*pPORTGIO_SET = PG1;

   	for(i=0; i<DELAY; i++)
	{
		asm("nop;");
	}

	*pPORTGIO_CLEAR = PG1;

	*pSPI_BAUD = 4;//BAUD_RATE_DIVISOR;
	*pSPI_CTL = spi_setting;
#elif __ADSPBF548__
  	*pPORTE_FER = 0x0007;
   	*pPORTE_DIR_SET = PE4;
   	*pPORTE_SET = PE4;

   	for(i=0; i<DELAY; i++)
	{
		asm("nop;");
	}

	*pPORTE_CLEAR = PE4;

	*pSPI0_BAUD = BAUD_RATE_DIVISOR;
	*pSPI0_CTL = spi_setting;
#endif


}

//////////////////////////////////////////////////////////////
// Turns off the SPI
// Inputs - none
// returns- none
//
//////////////////////////////////////////////////////////////

void SPI_OFF(void)
{
	int i;

	//de-asserts the PF10 dedicated as SPI select
#ifdef __ADSPBF527__
	*pSPI_CTL = 0x0400;	// disable SPI
	*pSPI_BAUD = 0;
	*pPORTGIO_SET = PG1;
#elif __ADSPBF526__
	*pSPI_CTL = 0x0400;	// disable SPI
	*pSPI_BAUD = 0;
	*pPORTGIO_SET = PG1;
	
	*pPORTG_FER   &= ~0x1C;
  	*pPORTGIO_DIR &= ~PG1;
	
#elif __ADSPBF548__
	*pSPI0_CTL = 0x0400;	// disable SPI
	*pSPI0_BAUD = 0;
	*pPORTE_SET |= PE4;
#endif

	for(i=0; i<DELAY; i++)
	{
		asm("nop;");
	}
}



//////////////////////////////////////////////////////////////
// Wait_For_nStatus()
//
// Polls the WEL (Write Enable Latch) bit of the Flash's status
// register.
// Inputs - none
// returns- none
//
//////////////////////////////////////////////////////////////
ERROR_CODE Wait_For_nStatus(void)
{
	int n, i;
	char status_register = 0;
	ERROR_CODE ErrorCode = NO_ERR;	// tells us if there was an error erasing flash

		for(i = 0; i < 500; i++)
		{
			status_register = ReadStatusRegister();
			if( (status_register & WEL) )
			{
				ErrorCode = NO_ERR;
				return ErrorCode;
			}
			ErrorCode = POLL_TIMEOUT;	// Time out error
		};

	return ErrorCode;
}

/////////////////////////////////////////////////////////////////
// unsigned long DataFlashAddress()
//
// Translates a binary address into the appropriate dataflash address.
//
// Inputs: unsigned long address - The binary address supplied
/////////////////////////////////////////////////////////////////
unsigned long DataFlashAddress (unsigned long address)
{
	// Determine the page that it is on
	unsigned long pageAddress = address / SPI_PAGE_SIZE;

	// Determine the byte offset within the page
	unsigned long bitAddress = address % SPI_PAGE_SIZE;

	// Shift the page address the correct number of bits
	pageAddress = pageAddress << PAGE_BITS;

	// Return the combined Page and Byte offset address
	return (pageAddress | bitAddress);
}


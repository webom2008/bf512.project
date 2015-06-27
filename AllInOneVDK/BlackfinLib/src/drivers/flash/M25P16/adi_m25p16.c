/*******************************************************************************/
/*                                                                             */
/*   (C) Copyright 2004 - Analog Devices, Inc.  All rights reserved.           */
/*                                                                             */
/*    FILE:     a d i _ m 2 5 p 1 6 . c ( )                                    */
/*                                                                             */
/*    CHANGES:  1.00.0  - initial release    								   */
/*																			   */
/*    PURPOSE:  Performs operations specific to the M25P16 flash device        */
/*              while adhering to the adi device drivers model.                */
/*                                                                             */
/*******************************************************************************/

//---- I n c l u d e s  ----//
#ifdef __ADSPBF548__
	#include <cdefBF548.h>
#elif __ADSPBF527__
	#include <cdefBF527.h>
#endif
#include <ccblkfn.h>
#include <stdio.h>
#include <drivers\flash\util.h>
#include <drivers\flash\adi_m25p16.h>
#include <drivers\flash\Errors.h>

#define NUM_SECTORS 	32			// number of sectors in the flash device

//---- c o n s t a n t   d e f i n i t i o n s -----//

static char 	*pFlashDesc = "STMicro. M25P16";
static char 	*pDeviceCompany	=	"STMicroelectronics";

static int		gNumSectors = NUM_SECTORS;

#undef TIMEOUT
#undef DELAY

//Flash commands
#define SPI_WREN            (0x06)  //Set Write Enable Latch
#define SPI_WRDI            (0x04)  //Reset Write Enable Latch
#define SPI_RDID            (0x9F)  //Read Identification
#define SPI_RDSR            (0x05)  //Read Status Register
#define SPI_WRSR            (0x01)  //Write Status Register
#define SPI_READ            (0x03)  //Read data from memory
#define SPI_FAST_READ       (0x0B)  //Read data from memory
#define SPI_PP              (0x02)  //Program Data into memory
#define SPI_SE              (0xD8)  //Erase one sector in memory
#define SPI_BE              (0xC7)  //Erase all memory
#define WIP                  (0x1)	//Check the write in progress bit of the SPI status register
#define WEL                  (0x2)	//Check the write enable bit of the SPI status register

#define SPI_PAGE_SIZE		(528)
#define SPI_SECTORS		    (512)
#define SPI_SECTOR_SIZE		(4224)
#define SPI_SECTOR_DIFF		(3968)
#define PAGE_BITS			(10)
#define PAGE_SIZE_DIFF		(496)

#define DELAY				300
#define TIMEOUT        35000*64

char			SPI_Page_Buffer[SPI_PAGE_SIZE];
int 			SPI_Page_Index = 0;
char            SPI_Status;


//---- F u n c t i o n   P r o t o t y p e s  ----//
//            (for  Helper Functions)             //

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

static ERROR_CODE EraseFlash(unsigned long ulStartAddr);
static ERROR_CODE EraseBlock( int nBlock, unsigned long ulStartAddr );
static ERROR_CODE GetCodes(int *pnManCode, int *pnDevCode, unsigned long ulStartAddr);
static ERROR_CODE GetSectorNumber( unsigned long ulAddr, int *pnSector );
static ERROR_CODE GetSectorStartEnd( unsigned long *ulStartOff, unsigned long *ulEndOff, int nSector );
static ERROR_CODE PollToggleBit(void);
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

ADI_DEV_PDD_ENTRY_POINT ADIM25P16EntryPoint =
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
//  	This function opens the M25P16 flash device for use.
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
#ifdef ADI_M25P16_ERROR_CHECKING_ENABLED
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
//  	This function closes the M25P16 flash device.
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
//  	Provides buffers to the M25P16 flash device for reception
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

	//send the bulk erase command to the flash
	WriteFlash( *pulAbsoluteAddr , SPI_READ );
	WriteFlash( *pulAbsoluteAddr , (*pulAbsoluteAddr ) >> 16);
	WriteFlash( *pulAbsoluteAddr , (*pulAbsoluteAddr ) >> 8);
	WriteFlash( *pulAbsoluteAddr ,  *pulAbsoluteAddr );

	// read our actual value now
	Result = ReadFlash( *pulAbsoluteAddr, pusValue );

	SPI_OFF();


	return(Result);
}


//----------- a d i _ p d d _ W r i t e  ( ) ----------//
//
//  PURPOSE
//  	Provides buffers to the M25P16 flash device for transmission
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
	u32				  Result;			// error code returned

	// cast our buffer to a 1D buffer
	pBuff1D = (ADI_DEV_1D_BUFFER*)pBuffer;

	// cast our data buffer
	psValue = (short *)pBuff1D->Data;

	// cast our offset
	pulAbsoluteAddr = (unsigned long *)pBuff1D->pAdditionalInfo;

	// First, a Write Enable Command must be sent to the SPI.
	SendSingleCommand(SPI_WREN);

	// Second, the SPI Status Register will be tested whether the
	//         Write Enable Bit has been set.
	Result = Wait_For_WEL();

	if( POLL_TIMEOUT == Result )
		return Result;
	else
	{
		SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

		//send the bulk erase command to the flash
		WriteFlash( *pulAbsoluteAddr , SPI_PP );
		WriteFlash( *pulAbsoluteAddr , (*pulAbsoluteAddr ) >> 16);
		WriteFlash( *pulAbsoluteAddr , (*pulAbsoluteAddr ) >> 8);
		WriteFlash( *pulAbsoluteAddr ,  *pulAbsoluteAddr );

		// program our actual value now
		Result = WriteFlash( *pulAbsoluteAddr, *psValue );

		SPI_OFF();

		return(Result);
	}


}


//----------- a d i _ p d d _ C o n t r o l  ( ) ----------//
//
//  PURPOSE
//  	This function sets or detects a configuration parameter
//		for the M25P16 flash device.
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
//		unsigned long ulStartAddr - flash start address
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE ResetFlash(unsigned long ulAddr)
{


//	char status_register = 0;
	ERROR_CODE ErrorCode = NO_ERR;

	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

	//send the bulk erase command to the flash
	WriteFlash( NULL , SPI_WRDI );

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
//		unsigned long ulStartAddr - flash start address
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE EraseFlash(unsigned long ulAddr)
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
		SendSingleCommand(SPI_BE);

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
	unsigned long ShiftValue;

	// Get the sector start offset
	// we get the end offset too however we do not actually use it for Erase sector
	GetSectorStartEnd( &ulSectStart, &ulSectEnd, nBlock );

	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

	// send the write enable instruction
	WriteFlash( (ulAddr + ulSectStart), SPI_WREN );

	SPI_OFF();

	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

	//send the erase block command to the flash
	WriteFlash( ulAddr , SPI_SE );
	WriteFlash( ulAddr , (ulAddr + ulSectStart) >> 16);
	WriteFlash( ulAddr , (ulAddr + ulSectStart) >> 8);
	WriteFlash( ulAddr ,  ulAddr + ulSectStart );

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
//		unsigned long ulStartAddr - flash start address
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE GetCodes(int *pnManCode, int *pnDevCode, unsigned long ulAddr)
{
	//Open the SPI, Deasserting CS
	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

	//Write the OpCode and Write address, 4 bytes.
	WriteFlash( ulAddr , SPI_RDID );

	// now we can read the codes
	ReadFlash( ulAddr + 0x0000, (unsigned short *)pnManCode );
	*pnManCode &= 0x00FF;

	//Write the OpCode and Write address, 4 bytes.
	WriteFlash( ulAddr , SPI_RDID );

	ReadFlash( ulAddr + 0x0002, (unsigned short *)pnDevCode );
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
	int i;
	int error_code = 1;
	unsigned long ulMask;					//offset mask
	unsigned long ulOffset;					//offset
	unsigned long ulStartOff;
	unsigned long ulEndOff;

	ulMask      	  = 0x7ffffff;
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
	unsigned long ulSectorSize = 0x10000;

	if( ( nSector >= 0 ) && ( nSector < gNumSectors ) ) // 32 sectors
		{
			*ulStartOff = nSector * ulSectorSize;
			*ulEndOff = ( (*ulStartOff) + ulSectorSize - 1 );
		}
	else
		return INVALID_SECTOR;


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

	ulFlashStartAddr  =  0;

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
	unsigned long  ulDummyRead, ulDummyWrite=0;
#ifdef __ADSPBF527__
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
//	 	unsigned long  ulAddr - address to write to
//		unsigned short nValue - value to write
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

ERROR_CODE WriteFlash( unsigned long ulAddr, unsigned short usValue )
{

#ifdef __ADSPBF527__
        *pSPI_TDBR = usValue;
#elif __ADSPBF548__
	*pSPI0_TDBR = usValue;
#endif

	Wait_For_SPIF();

	// ok
	return NO_ERR;

}


//////////////////////////////////////////////////////////////
// int ReadStatusRegister(void)
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

	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) ); // Turn on the SPI

	//send instruction to read status register
	WriteFlash( 0 , SPI_RDSR );

	// receive the status register
	ReadFlash( 0, &usStatus );

	SPI_OFF();		// Turn off the SPI

	return usStatus;
}

//////////////////////////////////////////////////////////////
// void Wait_For_SPIF(void)
//
// Polls the SPIF (SPI single word transfer complete) bit
// of SPISTAT until the transfer is complete.
// Inputs - none
// returns- none
//
//////////////////////////////////////////////////////////////
void Wait_For_SPIF(void)
{
	volatile int n;

	for(n=0; n<DELAY; n++)
	{
		asm("nop;");
	}

	while(1)
	{
#ifdef __ADSPBF527__
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
// Wait_For_WEL(void)
//
// Polls the WEL (Write Enable Latch) bit of the Flash's status
// register.
// Inputs - none
// returns- none
//
//////////////////////////////////////////////////////////////

ERROR_CODE Wait_For_WEL(void)
{
	volatile int n, i;
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
	}


	return ErrorCode;
}
//////////////////////////////////////////////////////////////
// Wait_For_Status(void)
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
	volatile int n, i;
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
	}


	return ErrorCode;

}

//////////////////////////////////////////////////////////////
// void SendSingleCommand( const int iCommand )
//
// Sends a single command to the SPI flash
// inputs - the 8-bit command to send
// returns- none
//
//////////////////////////////////////////////////////////////
void SendSingleCommand( const int iCommand )
{
	volatile int n;

	//turns on the SPI in single write mode
	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

	//sends the actual command to the SPI TX register
#ifdef __ADSPBF527__

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
    volatile int i;

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
	volatile int i;

	//de-asserts the PF10 dedicated as SPI select
#ifdef __ADSPBF527__
	*pSPI_CTL = 0x0400;	// disable SPI
	*pSPI_BAUD = 0;

	*pPORTGIO_SET = PG1;
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
// Wait_For_nStatus(void)
//
// Polls the WEL (Write Enable Latch) bit of the Flash's status
// register.
// Inputs - none
// returns- none
//
//////////////////////////////////////////////////////////////
ERROR_CODE Wait_For_nStatus(void)
{
	volatile int i;
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
	}

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


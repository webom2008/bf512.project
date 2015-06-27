/*******************************************************************************/
/*                                                                             */
/*   (C) Copyright 2008 - Analog Devices, Inc.  All rights reserved.           */
/*                                                                             */
/*    FILE:     sst25wf040.c                                                   */
/*																			   */
/*    PURPOSE:  Performs operations specific to the SST25WF040 flash           */
/*              device.                                                        */
/*                                                                             */
/*******************************************************************************/

/********************************************************************************************
* SST25WF040 architecture - 4 Mbit / 512 KB
*
* - uniform one-hundred-twenty-eight 4 KB sectors, but flexible erase capability with 32 KB and 64 KB overlays
*
* - addresses are offsets into the SPI flash, not part of the Blackfin memory space
* - 4KB, 32KB, and 64KB columns are the overlay numbers in order to perform erases of that size
*   using the size-specific erase commands
*
+===+====+====+======+============+============+ +===+====+====+======+============+============+
|4KB|32KB|64KB| SIZE |    START   |     END    | |4KB|32KB|64KB| SIZE |    START   |     END    |
+===+====+====+======+============+============+ +===+====+====+======+============+============+
| 0 |  0 |  0 |  4KB | 0x00000000 | 0x00000fff | |32 |  4 |  2 |  4KB | 0x00020000 | 0x00020fff |
| 1 |  0 |  0 |  4KB | 0x00001000 | 0x00001fff | |33 |  4 |  2 |  4KB | 0x00021000 | 0x00021fff |
| 2 |  0 |  0 |  4KB | 0x00002000 | 0x00002fff | |34 |  4 |  2 |  4KB | 0x00022000 | 0x00022fff |
| 3 |  0 |  0 |  4KB | 0x00003000 | 0x00003fff | |35 |  4 |  2 |  4KB | 0x00023000 | 0x00023fff |
| 4 |  0 |  0 |  4KB | 0x00004000 | 0x00004fff | |36 |  4 |  2 |  4KB | 0x00024000 | 0x00024fff |
| 5 |  0 |  0 |  4KB | 0x00005000 | 0x00005fff | |37 |  4 |  2 |  4KB | 0x00025000 | 0x00025fff |
| 6 |  0 |  0 |  4KB | 0x00006000 | 0x00006fff | |38 |  4 |  2 |  4KB | 0x00026000 | 0x00026fff |
| 7 |  0 |  0 |  4KB | 0x00007000 | 0x00007fff | |39 |  4 |  2 |  4KB | 0x00027000 | 0x00027fff |
+---+----+----+------+------------+------------+ +---+----+----+------+------------+------------+
| 8 |  1 |  0 |  4KB | 0x00008000 | 0x00008fff | |40 |  5 |  2 |  4KB | 0x00028000 | 0x00028fff |
| 9 |  1 |  0 |  4KB | 0x00009000 | 0x00009fff | |41 |  5 |  2 |  4KB | 0x00029000 | 0x00029fff |
|10 |  1 |  0 |  4KB | 0x0000a000 | 0x0000afff | |42 |  5 |  2 |  4KB | 0x0002a000 | 0x0002afff |
|11 |  1 |  0 |  4KB | 0x0000b000 | 0x0000bfff | |43 |  5 |  2 |  4KB | 0x0002b000 | 0x0002bfff |
|12 |  1 |  0 |  4KB | 0x0000c000 | 0x0000cfff | |44 |  5 |  2 |  4KB | 0x0002c000 | 0x0002cfff |
|13 |  1 |  0 |  4KB | 0x0000d000 | 0x0000dfff | |45 |  5 |  2 |  4KB | 0x0002d000 | 0x0002dfff |
|14 |  1 |  0 |  4KB | 0x0000e000 | 0x0000efff | |46 |  5 |  2 |  4KB | 0x0002e000 | 0x0002efff |
|15 |  1 |  0 |  4KB | 0x0000f000 | 0x0000ffff | |47 |  5 |  2 |  4KB | 0x0002f000 | 0x0002ffff |
+---+----+----+------+------------+------------+ +---+----+----+------+------------+------------+
|16 |  2 |  1 |  4KB | 0x00010000 | 0x00010fff | |48 |  6 |  3 |  4KB | 0x00030000 | 0x00030fff |
|17 |  2 |  1 |  4KB | 0x00011000 | 0x00011fff | |49 |  6 |  3 |  4KB | 0x00031000 | 0x00030fff |
|18 |  2 |  1 |  4KB | 0x00012000 | 0x00012fff | |50 |  6 |  3 |  4KB | 0x00032000 | 0x00030fff |
|19 |  2 |  1 |  4KB | 0x00013000 | 0x00013fff | |51 |  6 |  3 |  4KB | 0x00033000 | 0x00030fff |
|20 |  2 |  1 |  4KB | 0x00014000 | 0x00014fff | |52 |  6 |  3 |  4KB | 0x00034000 | 0x00030fff |
|21 |  2 |  1 |  4KB | 0x00015000 | 0x00015fff | |53 |  6 |  3 |  4KB | 0x00035000 | 0x00030fff |
|22 |  2 |  1 |  4KB | 0x00016000 | 0x00016fff | |54 |  6 |  3 |  4KB | 0x00036000 | 0x00030fff |
|23 |  2 |  1 |  4KB | 0x00017000 | 0x00017fff | |55 |  6 |  3 |  4KB | 0x00037000 | 0x00030fff |
+---+----+----+------+------------+------------+ +---+----+----+------+------------+------------+
|24 |  3 |  1 |  4KB | 0x00018000 | 0x00018fff | |56 |  7 |  3 |  4KB | 0x00038000 | 0x00038fff |
|25 |  3 |  1 |  4KB | 0x00019000 | 0x00019fff | |57 |  7 |  3 |  4KB | 0x00039000 | 0x00039fff |
|26 |  3 |  1 |  4KB | 0x0001a000 | 0x0001afff | |58 |  7 |  3 |  4KB | 0x0003a000 | 0x0003afff |
|27 |  3 |  1 |  4KB | 0x0001b000 | 0x0001bfff | |59 |  7 |  3 |  4KB | 0x0003b000 | 0x0003bfff |
|28 |  3 |  1 |  4KB | 0x0001c000 | 0x0001cfff | |60 |  7 |  3 |  4KB | 0x0003c000 | 0x0003cfff |
|29 |  3 |  1 |  4KB | 0x0001d000 | 0x0001dfff | |61 |  7 |  3 |  4KB | 0x0003d000 | 0x0003dfff |
|30 |  3 |  1 |  4KB | 0x0001e000 | 0x0001efff | |62 |  7 |  3 |  4KB | 0x0003e000 | 0x0003efff |
|31 |  3 |  1 |  4KB | 0x0001f000 | 0x0001ffff | |63 |  7 |  3 |  4KB | 0x0003f000 | 0x0003ffff |
+---+----+----+------+------------+------------+ +---+----+----+------+------------+------------+
*/

/* includes */
#ifdef __ADSPBF526__
	#include <cdefBF526.h>
#elif __ADSPBF518__
	#include <cdefBF518.h>
#endif
#include <ccblkfn.h>
#include <stdio.h>
#include <drivers\flash\util.h>
#include <drivers\flash\Errors.h>
#include <drivers\flash\sst25wf040.h>

/* constants */
static char 	*pFlashDesc = "SST25WF0x0";
static char 	*pDeviceCompany	= "SST, Inc.";
static int		gNumSectors;
static int		g_ManId = 0x0;
static int		g_DevId = 0x0;

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


/* function prototypes */
static ERROR_CODE Wait_For_nStatus(void);
static ERROR_CODE Wait_For_Status( char Statusbit );
static ERROR_CODE Wait_For_WEL(void);
static char ReadStatusRegister(void);
static void Wait_For_SPIF(void);
static void SetupSPI( const int spi_setting );
static void SPI_OFF(void);
static void SendSingleCommand( const int iCommand );
static unsigned long DataFlashAddress (unsigned long address);

static ERROR_CODE EraseFlash(void);
static ERROR_CODE EraseBlock( int nBlock, unsigned long ulStartAddr );
static ERROR_CODE GetCodes( int *pnManCode, int *pnDevCode );
static ERROR_CODE GetSectorNumber( unsigned long ulAddr, int *pnSector );
static ERROR_CODE GetSectorStartEnd( unsigned long *ulStartOff, unsigned long *ulEndOff, int nSector );
static ERROR_CODE PollToggleBit(void);
static ERROR_CODE ReadFlash( unsigned short *pusValue );
static ERROR_CODE ResetFlash(void);
static ERROR_CODE WriteFlash( unsigned short usValue );


//////////////////////////////////////////////////////////////
// sst25wf040_Open()
//
//  Opens the driver.
//
//	INPUTS
//		none
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise
//
/////////////////////////////////////////////////////////////////

ERROR_CODE sst25wf040_Open(void)
{
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


//////////////////////////////////////////////////////////////
// sst25wf040_Close()
//
//  Closes the driver.
//
//	INPUTS
//		none
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise
//
/////////////////////////////////////////////////////////////////

ERROR_CODE sst25wf040_Close(void)
{
	return (NO_ERR);
}


//////////////////////////////////////////////////////////////
// sst25wf040_Read()
//
//  Reads data from the flash memory.
//
//	INPUTS
//		unsigned short *pusData - data buffer to recieve, note this driver
//			uses unsigned short values, but only the lower byte of
//			each element is read into, the upper byte is ignored
//		unsigned long ulStartAddress - starting address
//		unsigned int uiCount - element count
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise
//
/////////////////////////////////////////////////////////////////

ERROR_CODE sst25wf040_Read(	unsigned short *pusData,
                    		unsigned long ulStartAddress,
							unsigned int uiCount )
{
	ERROR_CODE Result = NO_ERR;
	unsigned int i = 0;
	unsigned short *pusCurrentData = pusData;
	unsigned long ulCurrentAddress = ulStartAddress;


	for (i = 0; i < uiCount; i++, ulCurrentAddress++, pusCurrentData++)
	{
		SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

		volatile int n;
		for(n=0; n<1000; n++)
		{
			asm("nop;");
		}

		/* send the bulk erase command to the flash */
		WriteFlash( SPI_READ );
		WriteFlash( ulCurrentAddress >> 16);
		WriteFlash( ulCurrentAddress >> 8);
		WriteFlash( ulCurrentAddress );

		/* read the data now */
		Result = ReadFlash( pusCurrentData );

		SPI_OFF();
	}

	return(Result);
}


//////////////////////////////////////////////////////////////
// sst25wf040_Write()
//
//  Writes data to the flash memory.
//
//	INPUTS
//		unsigned short *pusData - data to be written, note this driver
//			receives unsigned short values, but only the lower byte of
//			each element is written, the upper byte is ignored
//		unsigned long ulStartAddress - starting address
//		unsigned int uiCount - element count
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise
//
/////////////////////////////////////////////////////////////////

ERROR_CODE sst25wf040_Write(unsigned short *pusData,
							unsigned long ulStartAddress,
							unsigned int uiCount )
{
    ERROR_CODE Result = NO_ERR;
	unsigned int i = 0;
	unsigned short *pusCurrentData = pusData;
	unsigned long ulCurrentAddress = ulStartAddress;


	for (i = 0; i < uiCount; i++, ulCurrentAddress++, pusCurrentData++)
	{
		int n;
		for(n=0; n<1000; n++)
		{
			asm("nop;");
		}

		/* first, a "Write Enable" command must be sent to the SPI */
		SendSingleCommand(SPI_WREN);

		/* second, the SPI Status Register will be tested whether the
			Write Enable Bit has been set */
		Result = Wait_For_WEL();

		if( POLL_TIMEOUT == Result )
		{
			/* if we timed out, try it again */
			for (n = 0; n < 3; n++)
			{
				/* first, a "Write Enable" command must be sent to the SPI */
				SendSingleCommand(SPI_WREN);

				/* second, the SPI Status Register will be tested whether the
					Write Enable Bit has been set */
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

			/* send the bulk erase command to the flash */
			WriteFlash( SPI_BYTE_PROGRAM );
			WriteFlash( ulCurrentAddress >> 16);
			WriteFlash( ulCurrentAddress >> 8);
			WriteFlash( ulCurrentAddress );

			/* program our actual value now */
			Result = WriteFlash( *pusCurrentData );

			SPI_OFF();
		}
	}

    return(Result);
}


//////////////////////////////////////////////////////////////
// sst25wf040_Control()
//
//  Handles commands for this driver.
//
//	INPUTS
//		unsigned int uiCmd - command index
//		COMMAND_STRUCT *pCmdStruct - command data
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise
//
/////////////////////////////////////////////////////////////////

ERROR_CODE sst25wf040_Control(	unsigned int uiCmd, COMMAND_STRUCT *pCmdStruct )
{
	ERROR_CODE ErrorCode = NO_ERR;


	// switch on the command
	switch ( uiCmd )
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

		// no command or unknown command do nothing
		default:
			// set our error
			ErrorCode = UNKNOWN_COMMAND;
			break;
	}

	// return
	return(ErrorCode);
}


//////////////////////////////////////////////////////////////
// ResetFlash()
//
// 	Sends a "reset" command to the flash.
//
//	INPUTS
//		none
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise
//
/////////////////////////////////////////////////////////////////

static ERROR_CODE ResetFlash(void)
{
	ERROR_CODE ErrorCode = NO_ERR;

	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

	//send the bulk erase command to the flash
	WriteFlash( SPI_WRDI );

	SPI_OFF();

	// poll until the command has completed
	ErrorCode = PollToggleBit();

	// reset should be complete
	return ErrorCode;
}


//////////////////////////////////////////////////////////////
// EraseFlash()
//
//  Sends an "erase all" command to the flash.
//
//	INPUTS
//		none
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise
//
/////////////////////////////////////////////////////////////////

static ERROR_CODE EraseFlash(void)
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
		// The status register will be polled to check the write in progress bit "WIP"
		ErrorCode = Wait_For_Status(WIP);

		printf("Error Code: %d", ErrorCode);
	}

	// erase should be complete
	return ErrorCode;
}


//////////////////////////////////////////////////////////////
// EraseBlock()
//
// 	Sends an "erase block" command to the flash.
//
//	INPUTS
//		int nBlock - block to erase
//		unsigned long ulStartAddr - flash start address
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise
//
/////////////////////////////////////////////////////////////////

static ERROR_CODE EraseBlock( int nBlock, unsigned long ulAddr )
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
	WriteFlash( SPI_WREN );

	SPI_OFF();

	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

	//send the erase block command to the flash
	WriteFlash( SPI_ERASE_4KB );
	WriteFlash( (ulAddr + ulSectStart) >> 16);
	WriteFlash( (ulAddr + ulSectStart) >> 8);
	WriteFlash( ulAddr + ulSectStart );

	// Poll the status register to check the Write in Progress bit
	// Sector erase takes time
	ErrorCode = Wait_For_Status(WIP);

	SPI_OFF();


 	// block erase should be complete
	return ErrorCode;
}


//////////////////////////////////////////////////////////////
// PollToggleBit()
//
//  Polls the toggle bit in the flash to see when the operation
//	is complete.
//
//	INPUTS
//		none
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise
//
/////////////////////////////////////////////////////////////////

static ERROR_CODE PollToggleBit(void)
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


//////////////////////////////////////////////////////////////
// GetCodes()
//
//  Sends an "auto select" command to the flash which will allow
//	us to get the manufacturer and device codes.
//
//  INPUTS
//  	int *pnManCode - pointer to manufacture code
//		int *pnDevCode - pointer to device code
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise
//
/////////////////////////////////////////////////////////////////

static ERROR_CODE GetCodes(int *pnManCode, int *pnDevCode )
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

//////////////////////////////////////////////////////////////
// GetSectorNumber()
//
// 	Gets a sector number based on the offset.
//
//  INPUTS
//  	unsigned long ulAddr - absolute address
//		int 	 *pnSector     - pointer to sector number
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise
//
/////////////////////////////////////////////////////////////////

static ERROR_CODE GetSectorNumber( unsigned long ulAddr, int *pnSector )
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


//////////////////////////////////////////////////////////////
//  GetSectorStartEnd()
//
// 	Gets a sector start and end address based on the sector number.
//
//  INPUTS
//  	unsigned long *ulStartOff - pointer to the start offset
//		unsigned long *ulEndOff - pointer to the end offset
//		int nSector - sector number
//
//	RETURN VALUE
//		ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise
//
/////////////////////////////////////////////////////////////////

static ERROR_CODE GetSectorStartEnd( unsigned long *ulStartOff, unsigned long *ulEndOff, int nSector )
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


//////////////////////////////////////////////////////////////
// ReadFlash()
//
// 	Reads a value from an address in flash.
//
//  INPUTS
// 		int pnValue - pointer to store value read from flash
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise
//
/////////////////////////////////////////////////////////////////

static ERROR_CODE ReadFlash( unsigned short *pusValue )
{
	/* disable interrupts before performing the load or store operation */
	unsigned long  ulDummyRead, ulDummyWrite = 0;

	ulDummyRead  = *pSPI_RDBR;
	Wait_For_SPIF();

	*pSPI_TDBR = ulDummyWrite;
	Wait_For_SPIF();
	*pusValue =  *pSPI_RDBR;

	/* ok */
	return NO_ERR;
}




//////////////////////////////////////////////////////////////
// WriteFlash()
//
// 	Write a value to an address in flash.
//
//  INPUTS
//		unsigned short nValue - value to write
//
//	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise
//
//////////////////////////////////////////////////////////////

static ERROR_CODE WriteFlash( unsigned short usValue )
{
    /* set the data */
	*pSPI_TDBR = usValue;

	Wait_For_SPIF();

	/* ok */
	return NO_ERR;
}


//////////////////////////////////////////////////////////////
// ReadStatusRegister()
//
// Returns the 8-bit value of the status register.
// Inputs - none
// returns- second location of status_register[2],
//         first location is garbage.
// Core sends the command
//
//////////////////////////////////////////////////////////////

static char ReadStatusRegister(void)
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
// void Wait_For_SPIF(void)
//
// Polls the SPIF (SPI single word transfer complete) bit
// of SPISTAT until the transfer is complete.
// Inputs - none
// returns- none
//
//////////////////////////////////////////////////////////////
static void Wait_For_SPIF(void)
{
	int n;

	for(n=0; n<DELAY; n++)
	{
		asm("nop;");
	}

	while(1)
	{
        unsigned short iTest = *pSPI_STAT;
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

static ERROR_CODE Wait_For_WEL(void)
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

static ERROR_CODE Wait_For_Status( char Statusbit )
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

static void SendSingleCommand( const int iCommand )
{
	int n;

	//turns on the SPI in single write mode
	SetupSPI( (COMMON_SPI_SETTINGS|TIMOD01) );

	//sends the actual command to the SPI TX register
	*pSPI_TDBR = iCommand;

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
// SetupSPI()
//
// Sets up the SPI for mode specified in spi_setting
// Inputs - spi_setting
// returns- none
//////////////////////////////////////////////////////////////

static void SetupSPI( const int spi_setting )
{
    int i;

#ifdef __ADSPBF518__

	/* 	PG12 - SPI0_SCK
		PG13 - SPI0_MISO
		PG14 - SPI0_MOSI */

	*pPORTG_FER |= (PG12 | PG13 | PG14);	/* set PG12, PG13, and PG14 */
	*pPORTG_MUX &= 0x3fff;					/* set for 1st function (00) */
  	
	*pPORTH_FER &= ~(PH8);						 
	*pPORTHIO_DIR |= PH8;
   	*pPORTHIO_SET = PH8;

   	for(i=0; i<DELAY; i++)
	{
		asm("nop;");
	}

	*pPORTHIO_CLEAR = PH8;					/* chip enable for internal SPI */

	*pSPI_BAUD = 6;							/* baud rate divisor */
	*pSPI_CTL = spi_setting;

#elif __ADSPBF526__
  	*pPORTG_FER   |= 0x1C;
	*pPORTG_MUX   = 0x00;
  	*pPORTGIO_DIR |= PG1;
   	*pPORTGIO_SET = PG1;

   	for(i=0; i<DELAY; i++)
	{
		asm("nop;");
	}

	*pPORTGIO_CLEAR = PG1;


	*pSPI_BAUD = 4;							/* baud rate divisor */
	*pSPI_CTL = spi_setting;

#endif


}

//////////////////////////////////////////////////////////////
// SPI_OFF()
//
// Turns off the SPI
// Inputs - none
// returns- none
//
//////////////////////////////////////////////////////////////

static void SPI_OFF(void)
{
	int i;

	//de-asserts the PF10 dedicated as SPI select
#ifdef __ADSPBF518__
	*pSPI_CTL = 0x0000;	// disable SPI
	*pSPI_BAUD = 0;
	*pPORTHIO_SET = PH8;
#elif __ADSPBF526__
	*pSPI_CTL = 0x0400;	// disable SPI
	*pSPI_BAUD = 0;
	*pPORTGIO_SET = PG1;
	*pPORTG_FER   &= ~0x1C;
	*pPORTGIO_DIR &= ~PG1;
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

static ERROR_CODE Wait_For_nStatus(void)
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
// DataFlashAddress()
//
// Translates a binary address into the appropriate dataflash address.
//
// Inputs: unsigned long address - The binary address supplied
/////////////////////////////////////////////////////////////////

static unsigned long DataFlashAddress (unsigned long address)
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


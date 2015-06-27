/*******************************************************************************/
/*                                                                             */
/*   (C) Copyright 2004 - Analog Devices, Inc.  All rights reserved.           */
/*                                                                             */
/*    FILE:     a d i _ m 2  9 w 3 2 0 . c ( )                                 */
/*                                                                             */
/*    CHANGES:  1.00.0  - initial release    								   */
/*																			   */
/*    PURPOSE:  Performs operations specific to the Spartan-3				   */
/*              XC3S1000-4FG456C fpga device while adhering to the			   */
/*				adi device drivers model.									   */
/*                                                                             */
/*******************************************************************************/


#include <drivers\fpga\adi_spartan3.h>
#include <drivers\fpga\util.h>
#include <drivers\flash\Errors.h>


#ifdef __ADSPBF527__
	#include <cdefBF527.h>
	char   	*pEzKitTitle 	= 	"ADSP-BF527 EZ-KIT Lite";

#elif __ADSPBF533__
	#include <cdefBF533.h>
	char   	*pEzKitTitle 	= 	"ADSP-BF533 EZ-KIT Lite";

#elif __ADSPBF537__
	#include <cdefBF537.h>
	char   	*pEzKitTitle 	= 	"ADSP-BF537 EZ-KIT Lite";

#elif (__ADSPBF548__) || (__ADSPBF549__)
	#include <cdefBF548.h>
	#include <defbf548.h>
	char   	*pEzKitTitle 	= 	"ADSP-BF548 EZ-KIT Lite";

#elif __ADSPBF561__
	#include <cdefBF561.h>
	char   	*pEzKitTitle 	= 	"ADSP-BF561 EZ-KIT Lite";

#endif


#define NUM_SECTORS 	1					// number of sectors in the flash device

//---- c o n s t a n t   d e f i n i t i o n s -----//

char 	*pDesc			=	"Spartan-3 XC3S1000-4FG456C";
char 	*pCompany		=	"Xilinx";


/*********************************************************************

Data Structures

*********************************************************************/

typedef struct {								// Spartan device structure
//  Init Ports
	volatile u16            *SPRTNProgDIR;		// Spartan Program DIR register
	volatile u16            *SPRTNProgINEN;		// Spartan Program INEN register
	volatile u16			*SPRTNDataDIR;		// Spartan Init DIR register
	volatile u16			*SPRTNDataINEN;		// Spartan Init INEN register
	volatile u16            *SPRTNInitDIR;		// Spartan Done DIR register
	volatile u16            *SPRTNInitINEN;		// Spartan Done INEN register
	volatile u16			*SPRTNDoneDIR;		// Spartan data DIR register
	volatile u16			*SPRTNDoneINEN;		// Spartan data INEN register

//	Command Ports
	volatile u16            *SPRTNSetPROG;	 	// Spartan set port for program register
	volatile u16            *SPRTNClearPROG;	// Spartan clear port register
	volatile u16            *SPRTNSetDATA;	 	// Spartan set port for data register
	volatile u16            *SPRTNClearDATA;	// Spartan clear port register
	volatile u16            *SPRTNSetCLK;		// Spartan set port with clock
	volatile u16            *SPRTNClearCLK;	 	// Spartan clear port register
	volatile u16			*SPRTNPortINIT; 	// Spartan init port

//  Commands
	u32                     PROG;          		// Function:PROGRAM
	u32                     INIT;          		// Function:INIT
	u32                     CLK;          		// Function:CLOCK
	u32                     DIN;          		// Function:DATA IN
	u32                     DONE;          		// Function:DONE

} ADI_SPARTAN;

/*********************************************************************

Device specific data

*********************************************************************/

/***************************
	Edinburgh
****************************/

#if defined(__ADSPBF533__)				// settings for Edinburgh class devices

//SPORT1 pins DT1PRI, TFS1, TSCLK are used for DIN, INIT and CCLK respectively
static ADI_SPARTAN Device[] = {			// Actual Spartan devices
	{									// device 0
											
		(volatile u16 *)pFIO_DIR,		// Spartan Program DIR register
		(volatile u16 *)pFIO_INEN,  	// Spartan Program INEN register
	    				NULL,   		// Spartan Data DIR register
	    				NULL,  			// Spartan Data INEN register
						NULL,			// Spartan Init DIR register
						NULL,  			// Spartan Init INEN register
	    (volatile u16 *)pFIO_DIR,   	// Spartan Done DIR register
	    (volatile u16 *)pFIO_INEN,  	// Spartan Done INEN register

   	    (volatile u16 *)pFIO_FLAG_S,	// Spartan set port for program register
		(volatile u16 *)pFIO_FLAG_C,	// Spartan clear port register
	    				NULL,			// Spartan set port for data register
						NULL,			// Spartan clear port register
						NULL,			// Spartan set port with clock
						NULL,			// Spartan clear port register
	    (volatile u16 *)pFIO_FLAG_D,   	// Spartan init port

	    PF2,                            // Function:PROGRAM
		0,								// Function:INIT
		0,								// Function:CLOCK
        0,                         		// Function:DATA IN
		PF3,							// Function:DONE

	},
};

#endif

#if defined(__ADSPBF537__)				// settings for Edinburgh class devices

static ADI_SPARTAN Device[] = {			// Actual Spartan devices
	{									// device 0
		
		(volatile u16 *)pPORTFIO_DIR,	// Spartan Program DIR register
		(volatile u16 *)pPORTFIO_INEN,  // Spartan Program INEN register
	    (volatile u16 *)pPORTGIO_DIR,   // Spartan Data DIR register
	    (volatile u16 *)pPORTGIO_INEN,  // Spartan Data INEN register
		(volatile u16 *)pPORTGIO_DIR,	// Spartan Init DIR register
		(volatile u16 *)pPORTGIO_INEN,  // Spartan Init INEN register
	    (volatile u16 *)pPORTFIO_DIR,   // Spartan Done DIR register
	    (volatile u16 *)pPORTFIO_INEN,  // Spartan Done INEN register

   	    (volatile u16 *)pPORTFIO_SET,	// Spartan set port for program register
		(volatile u16 *)pPORTFIO_CLEAR,	// Spartan clear port register
	    (volatile u16 *)pPORTGIO_SET,	// Spartan set port for data register
		(volatile u16 *)pPORTGIO_CLEAR,	// Spartan clear port register
		(volatile u16 *)pPORTGIO_SET,	// Spartan set port with clock
		(volatile u16 *)pPORTGIO_CLEAR,	// Spartan clear port register
	    (volatile u16 *)pPORTGIO,   	// Spartan init port

	    PF4,                            // Function:PROGRAM
		PG14,							// Function:INIT
		PG13,							// Function:CLOCK
        PG15,                           // Function:DATA IN
		PF14,							// Function:DONE

	},
};

#endif


#if defined(__ADSPBF561__)				// settings for Edinburgh class devices

static ADI_SPARTAN Device[] = {			// Actual Spartan devices
	{									// device 0
		(volatile u16 *)pFIO0_DIR,		// Spartan Program DIR register
		(volatile u16 *)pFIO0_INEN, 	// Spartan Program INEN register
	    (volatile u16 *)pFIO1_DIR,   	// Spartan Data DIR register
	    (volatile u16 *)pFIO1_INEN,  	// Spartan Data INEN register
		(volatile u16 *)pFIO1_DIR,		// Spartan Init DIR register
		(volatile u16 *)pFIO1_INEN,  	// Spartan Init INEN register
	    (volatile u16 *)pFIO0_DIR,   	// Spartan Done DIR register		
	    (volatile u16 *)pFIO0_INEN,  	// Spartan Done INEN register		

   	    (volatile u16 *)pFIO0_FLAG_S ,	// Spartan set port for program register
		(volatile u16 *)pFIO0_FLAG_C ,	// Spartan clear port register
	    (volatile u16 *)pFIO1_FLAG_S,	// Spartan set port for data register
		(volatile u16 *)pFIO1_FLAG_C,	// Spartan clear port register
		(volatile u16 *)pFIO1_FLAG_S,	// Spartan set port with clock
		(volatile u16 *)pFIO1_FLAG_C,	// Spartan clear port register
	    (volatile u16 *)pFIO1_FLAG_D ,  // Spartan init port

	    PF2,                            // Function:PROGRAM
		PF5,							// Function:INIT
		PF15,							// Function:CLOCK
        PF7,                            // Function:DATA IN
		PF3,							// Function:DONE

	},
};

#endif


/***************************
	Kookaburra
****************************/

#if defined(__ADSPBF527__)				// settings for Edinburgh class devices

static ADI_SPARTAN Device[] = {			// Actual Spartan devices
	{									// device 0
		(volatile u16 *)pPORTFIO_DIR,	// Spartan Program DIR register
		(volatile u16 *)pPORTFIO_INEN,  // Spartan Program INEN register
		(volatile u16 *)pPORTFIO_DIR,   // Spartan data DIR register
	    (volatile u16 *)pPORTFIO_INEN,  // Spartan data INEN register
		(volatile u16 *)pPORTFIO_DIR,	// Spartan Init DIR register
		(volatile u16 *)pPORTFIO_INEN,  // Spartan Init INEN register
	    (volatile u16 *)pPORTFIO_DIR,   // Spartan Done DIR register
	    (volatile u16 *)pPORTFIO_INEN,  // Spartan Done INEN register

   	    (volatile u16 *)pPORTFIO_SET,	// Spartan set port for program register
		(volatile u16 *)pPORTFIO_CLEAR,	// Spartan clear port register
	    (volatile u16 *)pPORTFIO_SET,	// Spartan set port for data register
		(volatile u16 *)pPORTFIO_CLEAR,	// Spartan clear port register
	    (volatile u16 *)pPORTFIO_SET,	// Spartan set port with clock
		(volatile u16 *)pPORTFIO_CLEAR,	// Spartan clear port register
	    (volatile u16 *)pPORTFIO,   	// Spartan init port

	    PF15,                           // Function:PROGRAM
		PF11,							// Function:INIT
		PF13,							// Function:CLOCK
        PF12,                           // Function:DATA IN
		PF14,							// Function:DONE

	},
};

#endif //BF527



/***************************

	Moab

****************************/

#if defined(__ADSPBF548__) || defined(__ADSPBF549__)  // settings for BF548 and BF549

static ADI_SPARTAN Device[] = {			// Actual Spartan devices
	{									// device 0
		(volatile u16 *)pPORTE_DIR_SET,	// Spartan Program DIR register
		(volatile u16 *)pPORTE_INEN,	// Spartan Program INEN register
		(volatile u16 *)pPORTA_DIR_SET, // Spartan Data DIR register
	    (volatile u16 *)pPORTA_INEN,  	// Spartan Data INEN register
	    (volatile u16 *)pPORTA_DIR_SET, // Spartan Init DIR register
	    (volatile u16 *)pPORTA_INEN,  	// Spartan Init INEN register
	    (volatile u16 *)pPORTG_DIR_SET, // Spartan Done DIR register
	    (volatile u16 *)pPORTG_INEN,  	// Spartan Done INEN register

	    (volatile u16 *)pPORTE_SET,		// Spartan set port for program register
		(volatile u16 *)pPORTE_CLEAR,	// Spartan clear port register
	    (volatile u16 *)pPORTA_SET,		// Spartan set port for data register
		(volatile u16 *)pPORTA_CLEAR,	// Spartan clear port register
	    (volatile u16 *)pPORTA_SET,		// Spartan set port with clock
		(volatile u16 *)pPORTA_CLEAR,	// Spartan clear port register
	    (volatile u16 *)pPORTA,   		// Spartan init port

	    PE6,                            // Function:PROGRAM
		PA8,							// Function:INIT
		PA11,							// Function:CLOCK
        PA10,                           // Function:DATA IN
		PG3,							// Function:DONE

	},
};

#endif  // BF548, BF549


ADI_SPARTAN 		*pDevice;			// pointer to the device we're working on


//---- F u n c t i o n   P r o t o t y p e s  ----//
//            (for  Helper Functions)             //


static u32 adi_pdd_Open(						// Open a device
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


static u32 adi_pdd_Close(			// Closes a device
	ADI_DEV_PDD_HANDLE PDDHandle	// PDD handle
);

static u32 adi_pdd_Read(	ADI_DEV_PDD_HANDLE 	PDDHandle,
	  				ADI_DEV_BUFFER_TYPE BufferType,
 			  		ADI_DEV_BUFFER		*pBuffer);

static u32 adi_pdd_Write(	ADI_DEV_PDD_HANDLE 	PDDHandle,
 			  		ADI_DEV_BUFFER_TYPE BufferType,
 			  		ADI_DEV_BUFFER		*pBuffer);


static u32 adi_pdd_Control(	ADI_DEV_PDD_HANDLE 	PDDHandle,
 			  			u32					Command,
 			  			void				*pArg);

static void EraseFPGA(    // reports and clears errors
	ADI_SPARTAN				*pDevice,       // device we're working on
	u16                     StatusReg       // status register
);


ADI_DEV_PDD_ENTRY_POINT ADISPARTAN3EntryPoint =
{
	adi_pdd_Open,
	adi_pdd_Close,
	adi_pdd_Read,
	adi_pdd_Write,
	adi_pdd_Control
};

ERROR_CODE GetSectorStartEnd( unsigned long *ulStartOff, unsigned long *ulEndOff, int nSector );
ERROR_CODE GetCodes(int *pnManCode, int *pnDevCode, unsigned long ulAddr);


// ---- P h y s i c a l   D e v i c e   D r i v e  A P I  f u n c t i o n s ----//

//----------- a d i _ p d d _ O p e n  ( ) ----------//
//
//  PURPOSE
//  	This function opens the Spartan3 fpga device for use.
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
	int i = 0;
	u32 			Result;		// return value
	ADI_SPARTAN 		*pDevice;				// pointer to the device we're working on

	 // assume we'll be successful
    Result = ADI_DEV_RESULT_SUCCESS;

	// avoid casts
	pDevice = &Device[DeviceNumber];


	// check for errors if required
#ifdef ADI_SPARTAN3_ERROR_CHECKING_ENABLED
	if (DeviceNumber > 0)								// check the device number
		return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
	if (Direction != ADI_DEV_DIRECTION_BIDIRECTIONAL)	// check the direction
		return (ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED);
#endif

	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);

	// initialize the Erase & Write Posts
	*pDevice->SPRTNSetPROG 		= 0;		// Spartan set port for program register
	*pDevice->SPRTNClearPROG 	= 0;		// Spartan clear port register
	
#ifndef __ADSPBF533__	
	*pDevice->SPRTNSetDATA 		= 0;		// Spartan set port for data register
	*pDevice->SPRTNClearDATA 	= 0;		// Spartan clear port register
	*pDevice->SPRTNSetCLK 		= 0;		// Spartan set port with clock
	*pDevice->SPRTNClearCLK 	= 0;		// Spartan clear port register
	*pDevice->SPRTNPortINIT 	= 0;		// Spartan init port
#endif

	//Initialize the necessary Pins
    *pDevice->SPRTNProgDIR  |= pDevice->PROG;					// Enable PROG As Output
	*pDevice->SPRTNProgINEN &= ~pDevice->PROG;					// make sure it is not an input

#ifndef __ADSPBF533__					
	*pDevice->SPRTNDataDIR  |= (pDevice->CLK | pDevice->DIN);	// Enable CLK and DIN As Output
	*pDevice->SPRTNDataINEN	&= (~pDevice->CLK | ~pDevice->DIN); // make sure they are not inputs

	*pDevice->SPRTNInitINEN |= pDevice->INIT;					// make sure it is an input
	*pDevice->SPRTNInitDIR  &= ~pDevice->INIT;					// Enable INIT as input
#endif

#ifdef __ADSPBF533__
	// Initialize SPORT1
	// internal clock, external FS, falling edge clock, and LSB
	*pSPORT1_TCR1 = TFSR | ITCLK | TCKFE | TLSBIT;
	
	// SLEN = 7 i.e. Serial Word Length = SLEN + 1
	*pSPORT1_TCR2 = 0x7;
	
	// divisor of 1, SPORTx_TCLK frequency = (SCLK frequency)/(2 x (SPORTx_TCLKDIV + 1)) 
	*pSPORT1_TCLKDIV = 0x1;
	
	ssync();
	
	// Configure DMA6
	// 8-bit transfers, stop mode
	*pDMA4_CONFIG = WDSIZE_8 | DMA2D;
	ssync();
	
#endif

	*pDevice->SPRTNDoneINEN |= pDevice->DONE;					// make sure it is an input
	*pDevice->SPRTNDoneDIR  &= ~pDevice->DONE;					// Enable DONE as input


	// save the physical device handle in the client supplied location
	*pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;

	return (NO_ERR);

}


//----------- a d i _ p d d _ C l o s e  ( ) ----------//
//
//  PURPOSE
//  	This function closes the Spartan3 fpga device.
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
//  	Provides buffers to the Spartan3 fpga device for reception
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

	// cannot read the fpga
	return ( UNKNOWN_COMMAND );
}


//----------- a d i _ p d d _ W r i t e  ( ) ----------//
//
//  PURPOSE
//  	Provides buffers to the Spartan3 fpga device for transmission
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
	ADI_DEV_1D_BUFFER 	*pBuff1D;			// buffer pointer
	char   		  	  	*pcBuff;			// stores the value to be written to device
	int				  	i = 0, j = 0;
	unsigned long     	*pulOffset;	// the absolute address to write
	u32				  	ulError = NO_ERR;
	u16 				Result=0;		// return value
	ADI_SPARTAN 		*pDevice;				// pointer to the device we're working on


	// avoid casts
	pDevice = (ADI_SPARTAN *)PDDHandle;


	// cast our buffer to a 1D buffer
	pBuff1D = (ADI_DEV_1D_BUFFER*)pBuffer;

	// cast our data buffer
	pcBuff = (char *)pBuff1D->Data;

	// cast our offset
	pulOffset = (unsigned long *)pBuff1D->pAdditionalInfo;

	// This should only be the first write
	// which is where we want to erase the FPGA
	if( *pulOffset == 0x0 )
		EraseFPGA(pDevice, Result);

#ifndef __ADSPBF533__
	// set clock low
	*pDevice->SPRTNClearCLK = pDevice->CLK;

	for( i = 0; i < pBuff1D->ElementCount; i++)
	{
		for( j = 0; j < 8; j++ )
		{
			*pDevice->SPRTNSetCLK = pDevice->CLK;		// clock low

			if(( (pcBuff[i] >> j) & 0x1 ))
				*pDevice->SPRTNSetDATA = pDevice->DIN;	// set bit
			else
				*pDevice->SPRTNClearDATA = pDevice->DIN;// clear bit

			*pDevice->SPRTNClearCLK = pDevice->CLK;		// clock high
		}

	}

	*pDevice->SPRTNClearCLK = pDevice->CLK;		// clock low
#endif

#ifdef __ADSPBF533__
	
	// Program low to start then back high
	*pDevice->SPRTNClearPROG = pDevice->PROG;
	
	// disable DMAs
	*pDMA4_CONFIG	= (*pDMA4_CONFIG & ~DMAEN);
	ssync();
	
	// Start address of data buffer
	*pDMA4_START_ADDR = pcBuff;
	
	// DMA loop count( count is always fixed so divide it equally )
	*pDMA4_X_COUNT = pBuff1D->ElementCount / 12;
	
	// DMA loop address increment
	*pDMA4_X_MODIFY = 1;
	
	// DMA outer loop count( gives us equal size inner loops )
	*pDMA4_Y_COUNT = 12;
	
	// DMA out loop modify
	*pDMA4_Y_MODIFY = 1;
	
	// enable DMA
	*pDMA4_CONFIG	= (*pDMA4_CONFIG | DMAEN);
	ssync();
	
	// enable Sport1 TX
	*pSPORT1_TCR1 	= (*pSPORT1_TCR1 | TSPEN);
	ssync();
	
	// program high to complete the erase and start frame sync
	*pDevice->SPRTNSetPROG = pDevice->PROG;
	
	// wait for the DMA to complete
	while( (*pDMA4_IRQ_STATUS & 0x8) == 0x8 ) {};
	
	// make sure the sport has transmitted everything
	while( (*pSPORT1_STAT & 0x40) == 0x0 ) {};
	
	i = 0;
	while(i++ < 0xFFFF)
	{
		asm("nop;");
	}
	
	// disable Sport1 TX
	*pSPORT1_TCR1 	= (*pSPORT1_TCR1 & ~TSPEN);
	ssync();
	
	// disable DMAs
	*pDMA4_CONFIG	= (*pDMA4_CONFIG & ~DMAEN);
	ssync();
#endif

	return NO_ERR;
}


//----------- a d i _ p d d _ C o n t r o l  ( ) ----------//
//
//  PURPOSE
//  	This function sets or detects a configuration parameter
//		for the Spartan3 fpga device.
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
	pDevice = (ADI_SPARTAN *)PDDHandle;
	u16	Result=0;										// return value

	COMMAND_STRUCT *pCmdStruct = (COMMAND_STRUCT *)pArg;

	// switch on the command
	switch ( Command )
	{

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
		    
		// get the number of sectors
		case ADI_FPGA_CMD_GETNUM_SECTORS:
			pCmdStruct->SGetNumSectors.pnNumSectors[0] = NUM_SECTORS;
			break;
			
		// get sector number start and end offset
		case ADI_FPGA_CMD_GET_SECSTARTEND:
			ErrorCode = GetSectorStartEnd( pCmdStruct->SSectStartEnd.pStartOffset, pCmdStruct->SSectStartEnd.pEndOffset, pCmdStruct->SSectStartEnd.nSectorNum );
			break;
			
		// get manufacturer and device codes
		case ADI_FPGA_CMD_GET_CODES:
			ErrorCode = GetCodes((int *)pCmdStruct->SGetCodes.pManCode, (int *)pCmdStruct->SGetCodes.pDevCode, (unsigned long)pCmdStruct->SGetCodes.ulFlashStartAddr);
			break;

		case ADI_FPGA_CMD_GET_DESC:
			//Filling the contents with data
			pCmdStruct->SGetDesc.pTitle = pEzKitTitle;
			pCmdStruct->SGetDesc.pDesc  = pDesc;
			pCmdStruct->SGetDesc.pFlashCompany  = pCompany;
			break;

		// erase all
		case ADI_FPGA_CMD_ERASE_ALL:
			EraseFPGA(pDevice, Result);
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


//----------- E r a s e F P G A ( ) ----------//
//
//  PURPOSE
//  	Erases the Fpga
//
// 	RETURN VALUE
//  	ERROR_CODE - value if any error occurs
//  	NO_ERR     - otherwise

static void EraseFPGA(
	ADI_SPARTAN	   *pDevice,       // device we're working on
	u16        StatusReg       // status register
) {

	int i = 0;
	volatile int nInitBit = 0;

	// Program low to start then back high
	*pDevice->SPRTNClearPROG = pDevice->PROG;

	while( i++ < 0xFF0 )
	{
		asm("nop;");
	}

	*pDevice->SPRTNSetPROG = pDevice->PROG;

#ifndef __ADSPBF533__	
	// check init pin to see if it is high
	// to continue programming
	nInitBit = *pDevice->SPRTNPortINIT;

	while( !(nInitBit & pDevice->INIT) )
	{
		nInitBit = *pDevice->SPRTNPortINIT;
	}
#endif
	// erase should be complete
	StatusReg = 0;
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
	// the spartan does not really have sectors so report back
	// the largest size we can
	if( nSector == 0 )
	{
		*ulStartOff = 0x0;
		*ulEndOff = 0xFFFFFFFF;
	}
	// no such sector
	else
		return INVALID_SECTOR;


	// ok
	return NO_ERR;
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
	// The Spartan3 does not have a man code and device code
	*pnManCode = 0x0;
	*pnDevCode = 0x0;

	// ok
	return NO_ERR;
}

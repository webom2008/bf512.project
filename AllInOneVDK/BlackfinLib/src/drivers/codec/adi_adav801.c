/*****************************************************************************

Copyright (c) 2005 Analog Devices.	All	Rights Reserved.

This software is proprietary and confidential.	By using this software you agree
to the terms of	the	associated Analog Devices License Agreement.  
******************************************************************************

$File: adi_adav801.c $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Project:	ADAV801	device driver -	Audio Codec	for	Recordable DVD
Title:		ADAV801	driver source code
Author(s):	bmk
Revised	by:	bmk

Description:
			This is	the	driver source code for the ADAV801 Audio Codec 
			for	Recordable DVD driver. It is layered on	top	of the SPI 
			and	SPORT device drivers, which	are	configured for the specific	
			use	of the ADAV801 peripheral. The driver uses Device Access 
			Service	files to access	its	internal registers

References:
******************************************************************************

Modification History:
====================
$Log: adi_adav801.c,v $
Revision 1.6  2007/03/02 21:26:43  bgiese
snapshot from VDSP 4.5

Revision 1.7  2006/11/17 03:59:42  bmk
Added workaround to address BF Audio extender - SPI hardware anomaly

Revision 1.6  2006/09/11 05:43:43  bmk
Fixed adi_dev_Close bug


******************************************************************************

Include	files

*****************************************************************************/

#include <services/services.h>							// system service includes
#include <drivers/adi_dev.h>							// device manager includes
#include <drivers/sport/adi_sport.h>					// SPORT driver	includes
#include <drivers/deviceaccess/adi_device_access.h>		// adi_device_access driver	includes
#include <drivers/codec/adi_adav801.h>					// ADAV801 driver includes
 
/*********************************************************************

Enumerations and defines

*********************************************************************/

// number of ADAV801 devices in	the	system
#define	ADI_ADAV801_NUM_DEVICES	(sizeof(Device)/sizeof(ADI_ADAV801)) 
// Address of the last register	in ADAV801
#define	ADAV801_FINAL_REG_ADDRESS	  0x7D	  

// SPORT word length
#define	SLEN_24	0x0017

// Structure to	contain	data for an	instance of	the	ADAV801	device driver
typedef	struct {
	ADI_DEV_MANAGER_HANDLE		ManagerHandle;		// Manager Handle		
	ADI_DEV_DEVICE_HANDLE		DMHandle;			// Handle of Device	Manager	instance
	ADI_DEV_PDD_HANDLE			sportHandle;		// Handle to the underlying	SPORT device driver
	u8							sportDeviceNumber;	// SPORT Device	number used	for	ADAV801	audio dataflow
	u8							sport_rxlen;		// Receive data	length for SPORT
	u8							sport_txlen;		// Transmit	data length	for	SPORT
	u8							adav801_spi_cs;		// SPI Chipselect for blackfin (SPI_FLG	location to	select ADAV801)
	ADI_DMA_MANAGER_HANDLE		DMAHandle;			// handle to the DMA manager
	ADI_DCB_HANDLE				DCBHandle;			// callback	handle	  
	ADI_DCB_CALLBACK_FN			DMCallback;			// the callback	function supplied by the Device	Manager
	ADI_DEV_DIRECTION			sportDirection;		// SPORT data direction
	u8							InUseFlag;			// Device in use flag
	u8							DataflowStatus;		// Dataflow status flag (Data flow ON/OFF)
	u32							DataflowMethod;		// Dataflow method
}	ADI_ADAV801;

// The initial values for the device instance
static ADI_ADAV801 Device[]	= {
	{
		NULL,
		NULL,
		NULL,
		0,				// SPORT 0 as default
		SLEN_24,		// by default as 24	bit	data (SPORT	Rx data	length)
		SLEN_24,		// by default as 24	bit	data (SPORT	Tx data	length)
		0,				// no SPI CS by	default
		NULL,
		NULL,
		NULL,
		ADI_DEV_DIRECTION_BIDIRECTIONAL,	// SPORT data direction
		FALSE,				// device not in use
		FALSE,				// dataflow off
		ADI_DEV_MODE_UNDEFINED,	// dataflow method is not defined by default
	},
};

/*********************************************************************

Static functions

*********************************************************************/

static u32 adi_pdd_Open(							// Open	a device
	ADI_DEV_MANAGER_HANDLE	ManagerHandle,			// device manager handle
	u32						DeviceNumber,			// device number
	ADI_DEV_DEVICE_HANDLE	DeviceHandle,			// device handle
	ADI_DEV_PDD_HANDLE		*pPDDHandle,			// pointer to PDD handle location 
	ADI_DEV_DIRECTION		Direction,				// data	direction
	void					*pCriticalRegionArg,	// critical	region imask storage location
	ADI_DMA_MANAGER_HANDLE	DMAHandle,				// handle to the DMA manager
	ADI_DCB_HANDLE			DCBHandle,				// callback	handle
	ADI_DCB_CALLBACK_FN		DMCallback				// device manager callback function
);

static u32 adi_pdd_Close(							// Closes a	device
	ADI_DEV_PDD_HANDLE		PDDHandle				// PDD handle
);

static u32 adi_pdd_Read(							// Reads data or queues	an inbound buffer to a device
	ADI_DEV_PDD_HANDLE		PDDHandle,				// PDD handle
	ADI_DEV_BUFFER_TYPE		BufferType,				// buffer type
	ADI_DEV_BUFFER			*pBuffer				// pointer to buffer
);
	
static u32 adi_pdd_Write(							// Writes data or queues an	outbound buffer	to a device
	ADI_DEV_PDD_HANDLE		PDDHandle,				// PDD handle
	ADI_DEV_BUFFER_TYPE		BufferType,				// buffer type
	ADI_DEV_BUFFER			*pBuffer				// pointer to buffer
);
	
static u32 adi_pdd_SequentialIO(					
	ADI_DEV_PDD_HANDLE		PDDHandle,				// PDD handle
	ADI_DEV_BUFFER_TYPE		BufferType,				// buffer type
	ADI_DEV_BUFFER			*pBuffer				// pointer to buffer
);

static u32 adi_pdd_Control(							// Sets	or senses a	device specific	parameter
	ADI_DEV_PDD_HANDLE		PDDHandle,				// PDD handle
	u32						Command,				// command ID
	void					*Value					// command specific	value
);

static u32 SPORT_Open( 
	ADI_DEV_PDD_HANDLE		PDDHandle				// Physical	Device Driver Handle
);

static u32 SPORT_Close(	
	ADI_DEV_PDD_HANDLE		PDDHandle				// Physical	Device Driver Handle
);

// the callback	function passed	to the SPORT driver
static void	sportCallbackFunction(
	void*	DeviceHandle, 
	u32		Event, 
	void*	pArg
);

/*********************************************************************

Debug Mode functions (debug	build only)

*********************************************************************/
#if	defined(ADI_DEV_DEBUG)

// Validate	PDD	handle
static int ValidatePDDHandle(
	ADI_DEV_PDD_HANDLE		PDDHandle
);

#endif

/*********************************************************************

Global Registers

*********************************************************************/

/*********************************************************************
Table for Register Field Error check and Register field	access
Table structure	-	'Count'	of Register	addresses containing individual	fields
					Register address containing	individual fields, 
					Register field locations in	the	corresponding register,	
					Reserved bit locations in the corresponding	register
Register field location	- Bit indicating start of new field	in a register will be 1
Reserved bit locations will	be 1
*********************************************************************/
// ADAV801 register	address	to perform Register	Field Error	check and Register field access
static u16 adav801RegAddr1[]	= {	ADAV801_SRC_CLK_CTRL, ADAV801_SPDIF_CTRL, ADAV801_PBK_CTRL,	
									ADAV801_AUXIN_PORT,	ADAV801_REC_CTRL, ADAV801_AUXOUT_PORT, 
									ADAV801_GDLY_MUTE, ADAV801_RX_CON1,	ADAV801_RX_CON2, 
									ADAV801_RXBUF_CON, ADAV801_TX_CTRL,	ADAV801_TXBUF_CON, 
									ADAV801_CSSBUF_TX, ADAV801_AUTO_BUF, ADAV801_RX_ERR_MASK, 
									ADAV801_SRC_ERR_MASK, ADAV801_INT_STAT_MASK, ADAV801_MUTE_DEEMP, 
									ADAV801_DP_CTRL1, ADAV801_DP_CTRL2,	ADAV801_DAC_CTRL1, 
									ADAV801_DAC_CTRL2, ADAV801_DAC_CTRL3, ADAV801_DAC_CTRL4, 
									ADAV801_ADC_CTRL1, ADAV801_ADC_CTRL2, ADAV801_PLL_CTRL1, 
									ADAV801_PLL_CTRL2, ADAV801_ICLK_CTRL1, ADAV801_ICLK_CTRL2, 
									ADAV801_PLL_CLK_SRC, ADAV801_PLLOUT_ENBL, ADAV801_ALC_CTRL1, 
									ADAV801_ALC_CTRL2	};

// Register	Field start	locations corresponding	to the entries in adav801RegAddr1 (Reserved	Bit	locations marked as	1)
static u16 adav801RegField[]	= {	0x55, 0xFF,	0xE9, 
									0xE9, 0xD5,	0xD5, 
									0x81, 0xB5,	0xDF, 
									0xFB, 0xCB,	0x1B, 
									0xFF, 0xF1,	0xFF,
									0xFF, 0xFF,	0xFB, 
									0x49, 0xC9,	0xD7, 
									0xD5, 0xFF,	0xDF, 
									0xFF, 0xFD,	0x7F, 
									0x77, 0x25,	0xEB,
									0xFF, 0xFF,	0x57, 
									0xAB	};

static ADI_DEVICE_ACCESS_REGISTER_FIELD	RegisterField[]	= {
	sizeof(adav801RegAddr1)/2,	// 'Count' of Register addresses containing	individual fields
	adav801RegAddr1,			// array of	ADAV801	register addresses containing individual fields
	adav801RegField,			// array of	register field locations in	the	corresponding registers
};

/*********************************************************************
Table to configure the reserved	bits in	the	device to its recommended values
Table structure	-  'Count' of Register addresses containing	Reserved Locations
	 Register address containing Reserved Locations
	 Recommended value for the Reserved	Locations
*********************************************************************/

// No specific values are recommended for reserved bits	in ADAV801 (Passed as NULL)

// Reserved	Bit	locations corresponding	to the entries in adav801RegAddr1
static u16 adav801ReservedBits[]	=	{	0x00, 0xFE,	0xE0, 
											0xE0, 0xC0,	0xC0, 
											0x00, 0x00,	0x0C, 
											0xC0, 0x80,	0x00, 
											0xCC, 0x80,	0x00,
											0xF8, 0x08,	0xD9, 
											0x00, 0xC0,	0x00, 
											0xC0, 0xF8,	0x8F, 
											0x00, 0xEC,	0x00, 
											0x00, 0x00,	0xE0,
											0x3F, 0xC8,	0x00, 
											0x80	};
									
static ADI_DEVICE_ACCESS_RESERVED_VALUES ReservedValues[] =	{
	sizeof(adav801RegAddr1)/2,	// 'Count' of Register addresses containing	Reserved Locations
	adav801RegAddr1,			// array of	ADAV801	register addresses containing individual fields
	adav801ReservedBits,		// array of	reserved bit locations in the corresponding	register	
	NULL,						// array of	Recommended	values for the Reserved	Bit	locations
};

/*********************************************************************
Table for ADAV801 Register Error check for invalid & read-only register(s) access
Structure -	'Count'	of invalid Register	addresses in ADAV801
			Invalid	Register addresses in ADAV801
			'Count'	of Read-only Register addresses	in ADAV801
			Read-only Register addresses in	ADAV801
*********************************************************************/

// array of	read-only registers	addresses in ADAV801.
static u16 adav801ReadOnlyRegs[]	= {	ADAV801_SRR_MSB, ADAV801_SRR_LSB, ADAV801_PBL_C_MSB, 
										ADAV801_PBL_C_LSB, ADAV801_PBL_D_MSB, ADAV801_PBL_D_LSB, 
										ADAV801_RX_ERR,	ADAV801_SRC_ERR, ADAV801_INT_STAT, 
										ADAV801_NONAUDIO_PBL, ADAV801_QCRC_ERR_STATUS	};
// array of	invalid	registers addresses	in ADAV801.
static u16 adav801InvalidRegs[]		= {	0x01,0x02,0x5F,
										0x60,0x61,0x79 };

static ADI_DEVICE_ACCESS_VALIDATE_REGISTER	ValidateRegister[] = {
	sizeof(adav801InvalidRegs)/2, // 'Count' of	Invalid	Register addresses in ADAV801	
	adav801InvalidRegs,			// array of	Invalid	Register addresses in ADAV801
	sizeof(adav801ReadOnlyRegs)/2,// 'Count' of	Read-only Register addresses in	ADAV801
	adav801ReadOnlyRegs,		// pointer to array	of Read-only Register addresses	in ADAV801
};

/*********************************************************************
*
*	Function:		SPORT_Open
*
*	Description:	Opens the SPORT	device for ADAV801 audio dataflow
*
*********************************************************************/
static u32 SPORT_Open( 
	ADI_DEV_PDD_HANDLE PDDHandle	// Physical	Device Driver Handle
) {
	
	//	Pointer	to ADAV801 device driver instance
	ADI_ADAV801	 *pADAV801 = (ADI_ADAV801 *)PDDHandle;
	// default return code
	u32	Result = ADI_DEV_RESULT_SUCCESS;

	// Open	the	SPORT driver
	Result = adi_dev_Open( 
		pADAV801->ManagerHandle,		// device manager handle
		&ADISPORTEntryPoint,			// SPORT Entry point
		pADAV801->sportDeviceNumber,	// SPORT device	number
		pADAV801,						// client handle - passed to internal callback function
		&pADAV801->sportHandle,			// pointer to DM handle	(for SPORT driver) location
		pADAV801->sportDirection,		// SPORT data direcion
		pADAV801->DMAHandle,			// handle to the DMA manager
		pADAV801->DCBHandle,			// handle to the callback manager
		sportCallbackFunction			// internal	callback function
	);

// return with appropriate code	if SPORT driver	fails to open
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

// SPORT Configuration (by default)
// Active Low, External	Frame sync,	MSB	first, External	CLK
// Stereo frame	sync enable, Secondary side	enabled, 24-bit	data

ADI_DEV_CMD_VALUE_PAIR SPORT_Config[] =	{
	{ ADI_SPORT_CMD_SET_TCR1,		(void *)(TFSR |	LTFS | TCKFE)						},
	{ ADI_SPORT_CMD_SET_TCR2,		(void *)(pADAV801->sport_txlen|	TXSE| TSFSE	)		},
	{ ADI_SPORT_CMD_SET_RCR1,		(void *)(RFSR |	LRFS | RCKFE)						},
	{ ADI_SPORT_CMD_SET_RCR2,		(void *)(pADAV801->sport_rxlen|	RXSE | RSFSE)		},
	{ ADI_DEV_CMD_END,				(void *)NULL										},
};

	// Configure SPORT device
	Result = adi_dev_Control( pADAV801->sportHandle, ADI_DEV_CMD_TABLE,	(void*)SPORT_Config	);
	
	return (Result);
}

/*********************************************************************
*
*	Function:		SPORT_Close
*
*	Description:	Closes the SPORT device	used for ADAV801 audio dataflow
*
*********************************************************************/
static u32 SPORT_Close(	
	ADI_DEV_PDD_HANDLE PDDHandle	// Physical	Device Driver Handle
) {

	//	Pointer	to ADAV801 device driver instance	
	ADI_ADAV801	 *pADAV801 = (ADI_ADAV801 *)PDDHandle;
	u32	Result = ADI_DEV_RESULT_SUCCESS;	// default return code

	// Check if	any	SPORT device is	open
	// if so, close	the	present	SPORT device in	use
	if (pADAV801->sportHandle != NULL)
	{
		// close the present SPORT device
		if ((Result	= adi_dev_Close(pADAV801->sportHandle))!= ADI_DEV_RESULT_SUCCESS)
			return (Result);
			// Mark	SPORT Handle as	NULL indicating	SPORT device is	closed
			pADAV801->sportHandle =	NULL;					
	}
	
	return (Result);
}

/*********************************************************************
*
* Function:	 adi_pdd_Open
*
* Description: Opens the ADAV801 device	for	use
*
*********************************************************************/
static u32 adi_pdd_Open(							// Open	a device
	ADI_DEV_MANAGER_HANDLE	ManagerHandle,			// device manager handle
	u32						DeviceNumber,			// device number
	ADI_DEV_DEVICE_HANDLE	DMHandle,				// device handle
	ADI_DEV_PDD_HANDLE		*pPDDHandle,			// pointer to PDD handle location 
	ADI_DEV_DIRECTION		Direction,				// data	direction
	void					*pEnterCriticalArg,		// enter critical region parameter
	ADI_DMA_MANAGER_HANDLE	DMAHandle,				// handle to the DMA manager
	ADI_DCB_HANDLE			DCBHandle,				// callback	handle
	ADI_DCB_CALLBACK_FN		DMCallback				// device manager callback function
) {
 
	u32	Result = ADI_DEV_RESULT_SUCCESS;	// default return code
	ADI_ADAV801	 *pADAV801;		// pointer to the ADAV801 device we're working on
	void *pExitCriticalArg;		// exit	critical region	parameter
  
	// Check for a valid device	number
#ifdef ADI_DEV_DEBUG
	if (DeviceNumber >=	ADI_ADAV801_NUM_DEVICES) return	(ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
#endif

	// assign the pointer to the device	instance
	pADAV801 = &Device[DeviceNumber];
	// store the Manager handle
	pADAV801->ManagerHandle	= ManagerHandle;
	// store the Device	Manager	handle
	pADAV801->DMHandle = DMHandle;
	// store the DMA Manager handle
	pADAV801->DMAHandle	= DMAHandle;
	// store the Deffered Callback Manager handle
	pADAV801->DCBHandle	= DCBHandle;
	// callback	function
	pADAV801->DMCallback = DMCallback;
	// direction
	pADAV801->sportDirection = Direction;

	// insure the device the client	wants is available	  
	Result = ADI_DEV_RESULT_DEVICE_IN_USE;	  
	
	// Check that this device instance is not already in use. 
	// If not, assign flag to indicate that	it is now.
	pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
	if (pADAV801->InUseFlag == FALSE) 
	{
		pADAV801->InUseFlag = TRUE;
		Result = ADI_DEV_RESULT_SUCCESS;
	}
	adi_int_ExitCriticalRegion(pExitCriticalArg);
	
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);	
#endif

	// save	the	physical device	handle in the client supplied location
	*pPDDHandle	= (ADI_DEV_PDD_HANDLE *)pADAV801;
	
	// return after	successful completion
	return(Result);
}

/*********************************************************************
*
* Function:	 adi_pdd_Close
*
* Description: Closes down a SPI & SPORT device	
*
*********************************************************************/

static u32 adi_pdd_Close(							// Closes a	device
	ADI_DEV_PDD_HANDLE		PDDHandle				// PDD handle
)	{

	u32	Result;	 //	return value
	//	Pointer	to ADAV801 device driver instance	
	ADI_ADAV801	 *pADAV801 = (ADI_ADAV801 *)PDDHandle;

 //	check for errors if	required
#ifdef ADI_DEV_DEBUG
	if ((Result	= ValidatePDDHandle(PDDHandle))	!= ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Close the present SPORT device being	used
	Result = SPORT_Close(PDDHandle);

	// mark	the	device as closed
	pADAV801->InUseFlag = FALSE;

	return(Result);
}

/*********************************************************************
*
* Function:	 adi_pdd_Read
*
* Description: Pass buffer to SPORT
*
*********************************************************************/

static u32 adi_pdd_Read(							// Reads data or queues	an inbound buffer to a device
	ADI_DEV_PDD_HANDLE		PDDHandle,				// PDD handle
	ADI_DEV_BUFFER_TYPE		BufferType,				// buffer type
	ADI_DEV_BUFFER			*pBuffer				// pointer to buffer
) {
	//	Pointer	to ADAV801 device driver instance	
	ADI_ADAV801	 *pADAV801 = (ADI_ADAV801 *)PDDHandle;	

#ifdef ADI_DEV_DEBUG
	// check for errors	if required
	u32	Result = ADI_DEV_RESULT_SUCCESS;
 	if	((Result = ValidatePDDHandle(PDDHandle)) !=	ADI_DEV_RESULT_SUCCESS)	return (Result);
#endif
	
	// pass	read operation to SPORT
	return(adi_dev_Read(pADAV801->sportHandle,BufferType,pBuffer));

}

/*********************************************************************
*
* Function:	 adi_pdd_Write
*
* Description: Pass buffer to SPORT
*
*********************************************************************/
 
static u32 adi_pdd_Write(							// Writes data or queues an	outbound buffer	to a device
	ADI_DEV_PDD_HANDLE		PDDHandle,				// PDD handle
	ADI_DEV_BUFFER_TYPE		BufferType,				// buffer type
	ADI_DEV_BUFFER			*pBuffer				// pointer to buffer
) {

	//	Pointer	to ADAV801 device driver instance	
	ADI_ADAV801	 *pADAV801 = (ADI_ADAV801 *)PDDHandle;
	
#ifdef ADI_DEV_DEBUG
	// check for errors	if required
	u32	Result = ADI_DEV_RESULT_SUCCESS;
	if ((Result	= ValidatePDDHandle(PDDHandle))	!= ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// pass	write operation	to SPORT
	return(adi_dev_Write(pADAV801->sportHandle,BufferType,pBuffer));
 
}

/*********************************************************************
*
* Function:	 adi_pdd_SequentialIO
*
* Description: Function	not	supported by this driver
*
*********************************************************************/

static u32 adi_pdd_SequentialIO(					
	ADI_DEV_PDD_HANDLE		PDDHandle,				// PDD handle
	ADI_DEV_BUFFER_TYPE		BufferType,				// buffer type
	ADI_DEV_BUFFER			*pBuffer				// pointer to buffer
) {

	// Function	not	supported
	return(ADI_ADAV801_RESULT_CMD_NOT_SUPPORTED);
}

/*********************************************************************
*
* Function:	 adi_pdd_Control
*
* Description: ADAV801 command implementation
*
*********************************************************************/
	
static u32 adi_pdd_Control(							// Sets	or senses a	device specific	parameter
	ADI_DEV_PDD_HANDLE		PDDHandle,				// PDD handle
	u32						Command,				// command ID
	void					*Value					// command specific	value
) {

	ADI_ADAV801	   *pADAV801; // pointer to	the	device we're working on
	u32				Result;	  // return	value
	u32				u32Value;  // u32 type to avoid	casts/warnings etc.
	u8				u8Value;	// u8 type to avoid	casts/warnings etc.	 
	
	// avoid casts
	pADAV801 = (ADI_ADAV801	*)PDDHandle; //	Pointer	to ADAV801 device driver instance
	// assign 8, 16	and	32 bit values for the Value	argument
	u32Value = (u32) Value;
	u8Value	 = (u8)	u32Value;

    // Location to hold SPI dummy read
    ADI_DEV_ACCESS_REGISTER		DummyRead; 	
	// Structure passed	to device access service
	ADI_DEVICE_ACCESS_REGISTERS		access_device;
	// Structure passed	to select Device type and access type
	ADI_DEVICE_ACCESS_SELECT	SelectSPIAccess[] =	{
		pADAV801->adav801_spi_cs,			// SPI Chip-select
		ADI_DEVICE_ACCESS_LENGTH0,			// No Global address for ADAV801
		ADI_DEVICE_ACCESS_LENGTH1,			// ADAV801 register	address	length (1 byte)
		ADI_DEVICE_ACCESS_LENGTH1,			// ADAV801 register	data length	(1 byte)
		ADI_DEVICE_ACCESS_TYPE_SPI,			// Select SPI access
	};
  
	// check for errors	if required
#ifdef ADI_DEV_DEBUG
	if ((Result	= ValidatePDDHandle(PDDHandle))	!= ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
	
	// assume we're	going to be	successful
	Result = ADI_DEV_RESULT_SUCCESS;

	switch (Command)	
	{

		// CASE	(control dataflow)
		case (ADI_DEV_CMD_SET_DATAFLOW):
		
     		// check if the SPORT device is already open
     		if (pADAV801->sportHandle != NULL)
     		{
     			// if so, Enable/Disable the SPORT device used by ADAV810
     			Result = adi_dev_Control( pADAV801->sportHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)u8Value );
			}
			// else check if the client is trying to enable dataflow without opening the SPORT
			else if (u8Value == TRUE)
			{
				// can not pass this value to SPORT as no valid SPORT handle is available
				// which also means the client hasn't defined the dataflow method yet, return error.
 				return (ADI_DEV_RESULT_DATAFLOW_UNDEFINED);
 			}
 			
         	pADAV801->DataflowStatus = u8Value;	// Save the dataflow status
         		
		break;

		// CASE	(query for processor DMA support)
		case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
  
			// ADAV801 doesn't support DMA,	but	supports indirectly	thru SPORT operation
			*((u32 *)Value)	= FALSE;
   
			break;

		// CASE	(Set Dataflow method - applies only	for	SPORT)
		case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):

			// Check if sport device to used by ADAV801 is already open
			if (pADAV801->sportHandle == NULL)
				// if not, try to open the SPORT device corresponding to sportDeviceNumber for ADAV801 dataflow
				Result = SPORT_Open(PDDHandle);

// on occurance	of error, return the error code
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

			// Pass	the	dataflow method	to the SPORT device allocated to ADAV801
			Result = adi_dev_Control( pADAV801->sportHandle, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void*)u32Value );
			
		break;

		// CASE	(Read a	specific register from the ADAV801)
		case(ADI_DEV_CMD_REGISTER_READ):
		// CASE	(Configure a specific register in the ADAV801)
		case(ADI_DEV_CMD_REGISTER_WRITE):
		// CASE	(Read a	specific field from	a single device	register)
		case(ADI_DEV_CMD_REGISTER_FIELD_READ):
		// CASE	(Write to a	specific field in a	single device register)
		case(ADI_DEV_CMD_REGISTER_FIELD_WRITE):			
		// CASE	(Read block	of ADAV801 registers starting from first given address)
		case(ADI_DEV_CMD_REGISTER_BLOCK_READ):	
		// CASE	(Write to a	block of ADAV801 registers starting	from first given address)
		case(ADI_DEV_CMD_REGISTER_BLOCK_WRITE):		
		// CASE	(Read a	table of selective registers in	ADAV801)
		case(ADI_DEV_CMD_REGISTER_TABLE_READ):
		// CASE	(Write to a	table of selective registers in	ADAV801)
		case(ADI_DEV_CMD_REGISTER_TABLE_WRITE):		
		// CASE	(Read a	table of selective register(s) field(s)	in ADAV801)
		case(ADI_DEV_CMD_REGISTER_FIELD_TABLE_READ):
		// CASE	(Write to a	table of selective register(s) field(s)	in ADAV801)
		case(ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE):	  

			access_device.ManagerHandle		= pADAV801->ManagerHandle;		// device manager handle
			access_device.ClientHandle		= NULL;							// client handle - passed to the internal 'Device' specific	function
			access_device.DeviceNumber		= 0;							// SPI device number
			access_device.DeviceAddress		= 0x00;							// SPI address of ADAV801 Device (Don't	care as	no specific	SPI	address	for	ADAV801)
			access_device.DCBHandle			= pADAV801->DCBHandle;			// handle to the callback manager
			access_device.DeviceFunction	= NULL;							// Function	specific to	ADAV801	driver passed to the 'Device' access service
			access_device.Command			= Command;						// command ID
			access_device.Value				= Value;						// command specific	value
			access_device.FinalRegAddr		= ADAV801_FINAL_REG_ADDRESS;	// Address of the last register	in ADAV801		
			access_device.RegisterField		= RegisterField;				// table for ADAV801 Register Field	Error check	and	Register field access
			access_device.ReservedValues	= ReservedValues;				// table to	configure reserved bits	in ADAV801 to recommended values
			access_device.ValidateRegister	= ValidateRegister;				// table containing	reserved and read-only registers in	ADAV801
			access_device.ConfigTable		= NULL;							// SPI configuration table
			access_device.SelectAccess		= SelectSPIAccess;				// Device Access type
			access_device.pAdditionalinfo	= (void	*)NULL;					// No Additional info

			/* Workaround for Blackfin Audio Extender rev 1.2 hardware anamoly
			Access to first register in a given register access table */
			DummyRead.Address 				= ADAV801_SRC_CLK_CTRL;			// ADAV801 register to be read
     		access_device.Command   		= ADI_DEV_CMD_REGISTER_READ;  	// Read a single register
     		access_device.Value    			= &DummyRead;       			// Location to hold the dummy read value

			Result = adi_device_access (&access_device);
			     		     					
     		access_device.Command   		= Command;                  	// command ID
     		access_device.Value    			= Value;                    	// command specific value						
			
			Result = adi_device_access (&access_device);
			
			break;

/************************
SPI	related	commands
************************/
				
		// CASE	(Set ADAV801 SPI Chipselect)
		case ADI_ADAV801_CMD_SET_SPI_CS:
  
			pADAV801->adav801_spi_cs = u8Value;
		 
			break;
   
		// CASE	(Get ADAV801 SPI Chipselect	value)
		case ADI_ADAV801_CMD_GET_SPI_CS:
   
			*((u8 *)Value) = pADAV801->adav801_spi_cs;

			break;

/************************
SPORT related commands
************************/

		// CASE	(Set SPORT Device Number that will be used for audio dataflow between Blackfin and ADAV801)
		case (ADI_ADAV801_CMD_SET_SPORT_DEVICE_NUMBER):
	   
			// Close the present SPORT device being	used
			Result = SPORT_Close(PDDHandle);
			
			// Update the SPORT	device number
			pADAV801->sportDeviceNumber	= u8Value;

			// Application program should open the new SPORT device, set its data flow method,
			// load	the	buffer(s) for the new SPORT	device & enable	the	dataflow
	
			break;

		// CASE	(Set SPORT word	length)
		// if ADAV801 is configured	for	right-justified	data, SPORT	wordlength should be configured	to it as well.
		// default value of	24 bits	can be	used when operated in other	modes  
		case ADI_ADAV801_CMD_SET_SPORT_TX_WLEN:
		case ADI_ADAV801_CMD_SET_SPORT_RX_WLEN:

			// Check if	the	Word length	is legal
			if ((u8Value < 2) || (u8Value >	31))
				return(ADI_ADAV801_RESULT_SPORT_WLEN_ILLEGAL);
				  
			// if dataflow is on, switch dataflow off before updating SPORT	word length
			if ((pADAV801->DataflowStatus == TRUE) && (pADAV801->sportHandle != NULL))
				Result	= adi_dev_Control( pADAV801->sportHandle, ADI_DEV_CMD_SET_DATAFLOW,	(void*)FALSE );
			
// on occurance	of error, return the error code
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

			if (Command	== ADI_ADAV801_CMD_SET_SPORT_TX_WLEN)
			{
				pADAV801->sport_txlen = u8Value;	   // load the new tx word length
				// Check if	any	SPORT device is	already	open
				// if so, configure	SPORT TX Word Length
				if (pADAV801->sportHandle != NULL)
					Result = adi_dev_Control( pADAV801->sportHandle, ADI_SPORT_CMD_SET_TX_WORD_LENGTH, (void*)u8Value );
			}
			else
			{
				pADAV801->sport_rxlen =	u8Value;	// load	the	new	rx word	length
				// Check if	any	SPORT device is	already	open
				// if so, configure	SPORT TX Word Length
				if (pADAV801->sportHandle != NULL)
					Result = adi_dev_Control( pADAV801->sportHandle, ADI_SPORT_CMD_SET_RX_WORD_LENGTH, (void*)u8Value );
			}

// on occurance	of error, return the error code
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

			// if dataflow was previously on, switch the dataflow on again
			if ((pADAV801->DataflowStatus == TRUE) && (pADAV801->sportHandle != NULL))
				Result = adi_dev_Control( pADAV801->sportHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)TRUE );  
   
			break;

		// CASE	(OPEN/CLOSE	SPORT Device to	be used	for	audio dataflow between Blackfin	and	ADAV801)
		case (ADI_ADAV801_CMD_SET_SPORT_STATUS):
		
			// check the SPORT mode
			if ((ADI_ADAV801_SET_SPORT_STATUS) Value == ADI_ADAV801_SPORT_OPEN)
			{
				// check if the SPORT device is already open
				if (pADAV801->sportHandle == NULL)
					// if not, OPEN	Sport Device, else do nothing
					Result = SPORT_Open(PDDHandle);
			}			
			else	// this should be to close the SPORT device
				// Close any SPORT device presently being used by ADAV801
				Result = SPORT_Close(PDDHandle);
			
			break;
		   
		default:
   
			if (pADAV801->sportHandle) // if SPORT device is already opened
				// pass	the	unknown	command	to SPORT
				Result = adi_dev_Control( pADAV801->sportHandle, Command, Value	);
			else
				// return error	indicating as command not valid
				Result = ADI_ADAV801_RESULT_CMD_NOT_SUPPORTED;
   
			break;
	}
 
	return(Result);
}

/*********************************************************************

 Function:	sportCallbackFunction

 Description: callback from	the	SPORT Driver

*********************************************************************/

static void	sportCallbackFunction (
	void*	DeviceHandle, 
	u32		Event, 
	void*	pArg
){
	ADI_ADAV801	*pADAV801 =	(ADI_ADAV801 *)DeviceHandle;

#ifdef ADI_DEV_DEBUG
	if (ValidatePDDHandle(DeviceHandle)	!= ADI_DEV_RESULT_SUCCESS) {   // verify the DM	handle
		return;
	}
#endif

	// simply pass the callback	along to the Device	Manager	Callback without interpretation
	(pADAV801->DMCallback)(pADAV801->DMHandle,Event,pArg);

}

// Debug build only
#ifdef ADI_DEV_DEBUG

/*********************************************************************

 Function:	ValidatePDDHandle

 Description: Validates	a PDD handle

*********************************************************************/
static int ValidatePDDHandle(
	ADI_DEV_PDD_HANDLE	PDDHandle
) {
	unsigned int i;
	for	(i = 0;	i <	ADI_ADAV801_NUM_DEVICES; i++) 
	{
		if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i])
			return (ADI_DEV_RESULT_SUCCESS);
	}
	return (ADI_DEV_RESULT_BAD_PDD_HANDLE);
}

#endif
/*********************************************************************

Entry point	for	device manager

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT	ADIADAV801EntryPoint = {
	adi_pdd_Open,
	adi_pdd_Close,
	adi_pdd_Read,
	adi_pdd_Write,
	adi_pdd_Control,
	adi_pdd_SequentialIO
};



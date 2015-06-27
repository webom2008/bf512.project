/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_ad1854.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			This is the driver source code for the AD1854 DAC peripheral. It is layered
			on top of the SPORT device driver, which is configured for the specific use of
			the AD1854 peripheral.
			
*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/

#define ADI_AD1854_MODULE

#include <services/services.h>			// system service includes
#include <drivers/adi_dev.h>			// device manager includes
#include <drivers/sport/adi_sport.h>    // sport driver includes
#include <drivers/dac/adi_ad1854.h>		// AD1854 driver includes

/*********************************************************************

Enumerations and defines

*********************************************************************/

#define ADI_AD1854_NUM_DEVICES	(sizeof(Device)/sizeof(ADI_AD1854))	// number of AD1854 devices in the system

static ADI_DEV_DEVICE_HANDLE SPORTDeviceHandle;
// Structure to contain data for an instance of the AD7476 device driver
#pragma pack(4)
typedef struct {
	ADI_DEV_DEVICE_HANDLE		DMHandle;			// Handle of Device Manager instance
	ADI_DEV_PDD_HANDLE			sportHandle;			// Handle to the underlying SPORT device driver
	u32							InUseFlag;			// flag to indicate whether an instance is in use or not
	//ADI_AD1854_OPERATION_MODE	CurMode;			// the Current mode of operation of the AD1854 device
	ADI_DCB_CALLBACK_FN			DMCallback;			// the callback function supplied by the Device Manager
	u32 						tquiet_factor;		// time between end of conversion and next sample as % of 
													// conversion time
} ADI_AD1854;
#pragma pack()
	
// The initial values for the device instance
static ADI_AD1854 Device[] = {
	{
		NULL,
		NULL,
		0,
//		ADI_AD1854_MODE_NORMAL,
		NULL,
		1062,
	},
};

// The prototpe functions for the AD1854 device driver

static u32 adi_pdd_Open(					// Open the AD1854 device
	ADI_DEV_MANAGER_HANDLE	ManagerHandle,			// device manager handle
	u32 					DeviceNumber,			// device number
	ADI_DEV_DEVICE_HANDLE 	DMHandle,				// device handle
	ADI_DEV_PDD_HANDLE 		*pPDDHandle,			// pointer to PDD handle location 
	ADI_DEV_DIRECTION 		Direction,				// data direction
	void					*pCriticalRegionArg,	// critical region imask storage location
	ADI_DMA_MANAGER_HANDLE	DMAHandleNULL,				// handle to the DMA manager
	ADI_DCB_HANDLE			DCBHandle,				// callback handle
	ADI_DCB_CALLBACK_FN		DMCallback				// device manager callback function
);

static u32 adi_pdd_Close(		// Closes the AD1854 device
	ADI_DEV_PDD_HANDLE PDDHandle	// PDD handle
);

static u32 adi_pdd_Read(		// Reads data or queues an inbound buffer to a device
	ADI_DEV_PDD_HANDLE PDDHandle,	// PDD handle
	ADI_DEV_BUFFER_TYPE	BufferType,	// buffer type
	ADI_DEV_BUFFER *pBuffer			// pointer to buffer
);
	
static u32 adi_pdd_Write(		// Writes data or queues an outbound buffer to a device
	ADI_DEV_PDD_HANDLE PDDHandle,	// PDD handle
	ADI_DEV_BUFFER_TYPE	BufferType,	// buffer type
	ADI_DEV_BUFFER *pBuffer			// pointer to buffer
);
	
static u32 adi_pdd_Control(		// Sets or senses a device specific parameter
	ADI_DEV_PDD_HANDLE PDDHandle,	// PDD handle
	u32 Command,					// command ID
	void *Value						// command specific value
);

static ADI_INT_HANDLER(ErrorHandler);		// interrupt handler for errors

// the callback function passed to the SPORT driver
static void sportCallbackFunction(void* DeviceHandle, u32 Event, void* pArg);
	

// Validation function (debug build only)	
#if defined(ADI_DEV_DEBUG)
static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle);
#endif


/*********************************************************************

Entry point for device manager

*********************************************************************/


ADI_DEV_PDD_ENTRY_POINT ADI_AD1854_EntryPoint = {
	adi_pdd_Open,
	adi_pdd_Close,
	adi_pdd_Read,
	adi_pdd_Write,
	adi_pdd_Control
};


/*********************************************************************

Entry point table for SPORT Device Driver

*********************************************************************/
extern ADI_DEV_PDD_ENTRY_POINT ADISPORTEntryPoint;


/*********************************************************************
*
*	Function:		adi_pdd_Open
*
*	Description:	Opens the AD1854 device for use
*
*********************************************************************/


static u32 adi_pdd_Open(
	ADI_DEV_MANAGER_HANDLE	ManagerHandle,			// device manager handle
	u32 					DeviceNumber,			// device number
	ADI_DEV_DEVICE_HANDLE	DMHandle,				// device handle
	ADI_DEV_PDD_HANDLE 		*pPDDHandle,			// pointer to PDD handle location 
	ADI_DEV_DIRECTION 		Direction,				// data direction
	void					*pEnterCriticalArg,		// enter critical region parameter
	ADI_DMA_MANAGER_HANDLE	DMAHandle,				// handle to the DMA manager
	ADI_DCB_HANDLE			DCBHandle,				// callback handle
	ADI_DCB_CALLBACK_FN		DMCallback				// client callback function
) {

	// default return code
	u32 Result = ADI_DEV_RESULT_SUCCESS;
	ADI_AD1854 *pAD1854;		// pointer to the AD1854 device we're working on
	void *pExitCriticalArg;		// exit critical region parameter
	u32 ResponseCount;	
	
	// Check for a valid device number and that we're only doing input
#ifdef ADI_DEV_DEBUG
	if (DeviceNumber >= ADI_AD1854_NUM_DEVICES) return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
	if (Direction != ADI_DEV_DIRECTION_OUTBOUND) return (ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED);
#endif
	
	// assign the pointer to the device instance
	pAD1854 = &Device[DeviceNumber];
	// and store the Device Manager handle 
	pAD1854->DMHandle = DMHandle;
	// and callback function
	pAD1854->DMCallback = DMCallback;	
		
	pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
	if (pAD1854->InUseFlag == FALSE) {
		pAD1854->InUseFlag = TRUE;
		Result = ADI_DEV_RESULT_SUCCESS;
	}
	adi_int_ExitCriticalRegion(pExitCriticalArg);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Open SPORT0 for communication
	Result = adi_dev_Open(ManagerHandle, &ADISPORTEntryPoint, 0, pAD1854, &(pAD1854->sportHandle), ADI_DEV_DIRECTION_OUTBOUND, DMAHandle, DCBHandle, sportCallbackFunction);

	// return with appropriate code if SPORT driver fails to open
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
    
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// save the physical device handle in the client supplied location
	*pPDDHandle = (ADI_DEV_PDD_HANDLE *)pAD1854;
	
	//Reset_DAC();

	// return after successful completion 
	return(Result);

}
		
		
/*********************************************************************
*
*	Function:		adi_pdd_Close
*
*	Description:	Closes down a SPORT device
*
*********************************************************************/


static u32 adi_pdd_Close(		// Closes a device
	ADI_DEV_PDD_HANDLE PDDHandle	// PDD handle
) {
	u32 Result;		// return value
	ADI_AD1854 *pAD1854 = (ADI_AD1854 *)PDDHandle;

	// check for errors if required
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
	// close SPORT driver
	Result = adi_dev_Close(pAD1854->sportHandle);

#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// mark the device as closed
	pAD1854->InUseFlag = FALSE;
	
	return(Result);
}
	

/*********************************************************************
*
*	Function:		adi_pdd_Read
*
*	Description:	Never called as SPORT uses DMA
*
*********************************************************************/


static u32 adi_pdd_Read(		// Reads data or queues an inbound buffer to a device
	ADI_DEV_PDD_HANDLE 	PDDHandle,	// PDD handle
	ADI_DEV_BUFFER_TYPE	BufferType,	// buffer type
	ADI_DEV_BUFFER 		*pBuffer	// pointer to buffer
){
	// this device only inputs data so we should never get here
	return(ADI_DEV_RESULT_FAILED);
}
	
/*********************************************************************
*
*	Function:		adi_pdd_Write
*
*	Description:	Never called as SPORT uses DMA
*
*********************************************************************/


static u32 adi_pdd_Write(		// Writes data or queues an outbound buffer to a device
	ADI_DEV_PDD_HANDLE 	PDDHandle,	// PDD handle
	ADI_DEV_BUFFER_TYPE	BufferType,	// buffer type
	ADI_DEV_BUFFER 		*pBuffer	// pointer to buffer
){
	
// check for errors if required
	u32 Result = ADI_DEV_RESULT_SUCCESS;
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
	// Simply pass the request on
	ADI_AD1854 *pAD1854 = (ADI_AD1854 *)PDDHandle;
	Result = adi_dev_Write(pAD1854->sportHandle,BufferType,pBuffer);

	return(Result);

		
}


/*********************************************************************
*
*	Function:		adi_pdd_Control
*
*	Description:	Configures the SPORT device
*
*********************************************************************/

static u32 adi_pdd_Control(		// Sets or senses a device specific parameter
	ADI_DEV_PDD_HANDLE 	PDDHandle,	// PDD handle
	u32 				Command,	// command ID
	void 				*Value		// command specific value
) {
	
	ADI_AD1854 				*pAD1854;		// pointer to the device we're working on
	u32 						tmp;			// temporary storage
	u32 						Result;			// return value
	u32							u32Value;		// u32 type to avoid casts/warnings etc.
	u16							u16Value;		// u16 type to avoid casts/warnings etc.
	
	// avoid casts
	pAD1854 = (ADI_AD1854 *)PDDHandle; // Pointer to AD1854 device driver instance
    // assign 16 and 32 bit values for the Value argument
	u32Value = (u32)Value;
    u16Value = ((u16)((u32)Value));
	
	// check for errors if required
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// assume we're going to be successful
	Result = ADI_DEV_RESULT_SUCCESS;	
	
	// CASEOF (Command ID)
	switch (Command) 
	{

		// CASE (query for processor DMA support)
		case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
			// no, we do support it at this level (it is delegated down to SPORT driver
			*((u32 *)Value) = FALSE;
			break;
		case (ADI_AD1854_CMD_SET_I2S_MODE): //I2S Active Low Frame Sync
			Result = adi_dev_Control(pAD1854->sportHandle, ADI_SPORT_CMD_SET_TCR1, (void *)(TFSR | LTFS | TCKFE));
			Result |= adi_dev_Control(pAD1854->sportHandle, ADI_SPORT_CMD_SET_TCR2, (void *)(SLEN_24 | TSFSE));
			break;		
		// DEFAULT 
		default:
		
			// pass anything we don't specifically handle to the SPORT driver
			return adi_dev_Control(pAD1854->sportHandle,Command,Value);			
		
	// ENDCASE
	}

	return(Result);
}

/*********************************************************************

	Function:		sportCallbackFunction

	Description:	Fields the callback from the SPORT Driver

*********************************************************************/

static void sportCallbackFunction(void* DeviceHandle, u32 Event, void* pArg)
{
	ADI_AD1854 *pAD1854 = (ADI_AD1854 *)DeviceHandle;
#if defined(ADI_DEV_DEBUG)
	if (ValidatePDDHandle(DeviceHandle) != ADI_DEV_RESULT_SUCCESS) {  	// verify the DM handle
		return;
	}
#endif
	// simply pass the callback along to the Device Manager Callback without interpretation
	(pAD1854->DMCallback)(pAD1854->DMHandle,Event,pArg);
}

#if defined(ADI_DEV_DEBUG)

/*********************************************************************

	Function:		ValidatePDDHandle

	Description:	Validates a PDD handle

*********************************************************************/

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle) {
	unsigned int i;
	for (i = 0; i < ADI_AD1854_NUM_DEVICES; i++) {
		if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i]) {
			return (ADI_DEV_RESULT_SUCCESS);
		}
	}
	return (ADI_DEV_RESULT_BAD_PDD_HANDLE);
	}


#endif



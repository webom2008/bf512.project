/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_ad1871.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			This is the driver source code for the AD1871 ADC peripheral. It is layered
			on top of the SPORT device driver, which is configured for the specific use of
			the AD1871 peripheral.
			
*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/

#define ADI_AD1871_MODULE

#include <services/services.h>			// system service includes
#include <drivers/adi_dev.h>			// device manager includes
#include <drivers/sport/adi_sport.h>	// sport driver includes
#include <drivers/adc/adi_ad1871.h> 	// AD1871 driver includes

/*********************************************************************

Enumerations and defines

*********************************************************************/

#define ADI_AD1871_NUM_DEVICES	(sizeof(Device)/sizeof(ADI_AD1871))	// number of AD1871 devices in the system

static ADI_DEV_DEVICE_HANDLE SPORTDeviceHandle;
// Structure to contain data for an instance of the AD7476 device driver
#pragma pack(4)
typedef struct {
	ADI_DEV_DEVICE_HANDLE		DMHandle;			// Handle of Device Manager instance
	ADI_DEV_PDD_HANDLE			sportHandle;			// Handle to the underlying SPORT device driver
	u32							InUseFlag;			// flag to indicate whether an instance is in use or not
	//ADI_AD1871_OPERATION_MODE	CurMode;			// the Current mode of operation of the AD1871 device
	ADI_DCB_CALLBACK_FN			DMCallback;			// the callback function supplied by the Device Manager
	u32 						tquiet_factor;		// time between end of conversion and next sample as % of 
													// conversion time
} ADI_AD1871;
#pragma pack()
	
// The initial values for the device instance
static ADI_AD1871 Device[] = {
	{
		NULL,
		NULL,
		0,
//		ADI_AD1871_MODE_NORMAL,
		NULL,
		1062,
	},
};

// The prototpe functions for the AD1871 device driver

static u32 adi_pdd_Open(					// Open the AD1871 device
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

static u32 adi_pdd_Close(		// Closes the AD1871 device
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


ADI_DEV_PDD_ENTRY_POINT ADI_AD1871_EntryPoint = {
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
*	Description:	Opens the AD1871 device for use
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
	ADI_AD1871 *pAD1871;		// pointer to the AD1871 device we're working on
	void *pExitCriticalArg;		// exit critical region parameter
	u32 ResponseCount;	
	
	// Check for a valid device number and that we're only doing input
#ifdef ADI_DEV_DEBUG
	if (DeviceNumber >= ADI_AD1871_NUM_DEVICES) return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
	if (Direction != ADI_DEV_DIRECTION_INBOUND) return (ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED);
#endif
	
	// assign the pointer to the device instance
	pAD1871 = &Device[DeviceNumber];
	// and store the Device Manager handle 
	pAD1871->DMHandle = DMHandle;
	// and callback function
	pAD1871->DMCallback = DMCallback;
	// Assign the quiet period factor, to enable us to more accurately 
	// modify the requested sample rate to what is achievable. This value is the 
	// (1+e)*1000, where e = (tsample - tconvert)/tconvert.
	// (see description under case ADI_AD1871_CMD_SET_QUIET_PERIOD_FACTOR in the 
	// adi_pdd_Control function for more details). 
	// The default value is based on experimental evidence using two ADSP-BF537 
	// EZ-Kits.
	pAD1871->tquiet_factor = 1062;
	// Check that this device instance is not already in use. If not, 
	// assign flag to indicate that it is now.
	// the device starts in powerup mode
	//pAD1871->CurMode = ADI_AD1871_MODE_NORMAL;
	

	pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
	if (pAD1871->InUseFlag == FALSE) {
		pAD1871->InUseFlag = TRUE;
		Result = ADI_DEV_RESULT_SUCCESS;
	}
	adi_int_ExitCriticalRegion(pExitCriticalArg);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif		
	// Open SPORT0 for communication
	Result = adi_dev_Open(ManagerHandle, &ADISPORTEntryPoint, 0, pAD1871, &(pAD1871->sportHandle), ADI_DEV_DIRECTION_INBOUND, DMAHandle, DCBHandle, sportCallbackFunction);
	// return with appropriate code if SPORT driver fails to open
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif	    
    ADI_DEV_CMD_VALUE_PAIR *pCommandPair;
    
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// save the physical device handle in the client supplied location
	*pPDDHandle = (ADI_DEV_PDD_HANDLE *)pAD1871;
	
	//Reset_ADC();

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
	ADI_AD1871 *pAD1871 = (ADI_AD1871 *)PDDHandle;

	// check for errors if required
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// close SPORT driver
	Result = adi_dev_Close(pAD1871->sportHandle);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// mark the device as closed
	pAD1871->InUseFlag = FALSE;
	
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
	// check for errors if required
	u32 Result = ADI_DEV_RESULT_SUCCESS;
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
	// Simply pass the request on
	ADI_AD1871 *pAD1871 = (ADI_AD1871 *)PDDHandle;
	Result = adi_dev_Read(pAD1871->sportHandle,BufferType,pBuffer);

	return(Result);

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
	
	// this device only inputs data so we should never get here
	return(ADI_DEV_RESULT_FAILED);		
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
	
	ADI_AD1871 				*pAD1871;		// pointer to the device we're working on
	u32 						tmp;			// temporary storage
	u32 						Result;			// return value
	u32							u32Value;		// u32 type to avoid casts/warnings etc.
	u16							u16Value;		// u16 type to avoid casts/warnings etc.
	// avoid casts
	pAD1871 = (ADI_AD1871 *)PDDHandle; // Pointer to AD1871 device driver instance
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
		case (ADI_AD1871_CMD_SET_I2S_MODE):
			Result = adi_dev_Control(pAD1871->sportHandle, ADI_SPORT_CMD_SET_RCR1, (void *)(RFSR | LRFS | RCKFE));
	       	Result = adi_dev_Control(pAD1871->sportHandle, ADI_SPORT_CMD_SET_RCR2, (void *)(SLEN_24 | RSFSE));
			break;
		// DEFAULT 
		default:
		
			// pass anything we don't specifically handle to the SPORT driver
			return adi_dev_Control(pAD1871->sportHandle,Command,Value);		
		
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
	ADI_AD1871 *pAD1871 = (ADI_AD1871 *)DeviceHandle;
#if defined(ADI_DEV_DEBUG)
	if (ValidatePDDHandle(DeviceHandle) != ADI_DEV_RESULT_SUCCESS) {  	// verify the DM handle
		return;
	}
#endif
	// simply pass the callback along to the Device Manager Callback without interpretation
	(pAD1871->DMCallback)(pAD1871->DMHandle,Event,pArg);
}

#if defined(ADI_DEV_DEBUG)

/*********************************************************************

	Function:		ValidatePDDHandle

	Description:	Validates a PDD handle

*********************************************************************/

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle) {
	unsigned int i;
	for (i = 0; i < ADI_AD1871_NUM_DEVICES; i++) {
		if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i]) {
			return (ADI_DEV_RESULT_SUCCESS);
		}
	}
	return (ADI_DEV_RESULT_BAD_PDD_HANDLE);
	}


#endif



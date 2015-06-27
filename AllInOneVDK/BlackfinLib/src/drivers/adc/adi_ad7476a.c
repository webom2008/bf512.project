/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_ad7476a.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			This is the driver source code for the AD7476A ADC peripheral. It is layered
			on top of the SPI device driver, which is configured for the specific use of
			the AD7476A peripheral.
			
*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/

#define ADI_AD7476A_MODULE

#include <services/services.h>			// system service includes

#ifndef __ADSP_BRAEMAR__
#error "AD7476A driver is not available for the current processor"
#endif

#include <drivers/adi_dev.h>			// device manager includes
#include <drivers/spi/adi_spi.h>       	// spi driver includes
#include <drivers/adc/adi_ad7476a.h>	// AD7476A driver includes

/*********************************************************************

Enumerations and defines

*********************************************************************/

#define ADI_AD7476A_NUM_DEVICES	(sizeof(Device)/sizeof(ADI_AD7476A))	// number of AD7476A devices in the system

// Structure to contain data for an instance of the AD7476 device driver
#pragma pack(4)
typedef struct {
	ADI_DEV_DEVICE_HANDLE		DMHandle;			// Handle of Device Manager instance
	ADI_DEV_PDD_HANDLE			spiHandle;			// Handle to the underlying SPI device driver
	u32							InUseFlag;			// flag to indicate whether an instance is in use or not
	//ADI_AD7476A_OPERATION_MODE	CurMode;			// the Current mode of operation of the AD7476A device
	ADI_DCB_CALLBACK_FN			DMCallback;			// the callback function supplied by the Device Manager
	u32 						tquiet_factor;		// time between end of conversion and next sample as % of 
													// conversion time
} ADI_AD7476A;
#pragma pack()
	
// The initial values for the device instance
static ADI_AD7476A Device[] = {
	{
		NULL,
		NULL,
		0,
//		ADI_AD7476A_MODE_NORMAL,
		NULL,
		1062,
	},
};

// The prototpe functions for the AD7476A device driver

static u32 adi_pdd_Open(					// Open the AD7476A device
	ADI_DEV_MANAGER_HANDLE	ManagerHandle,			// device manager handle
	u32 					DeviceNumber,			// device number
	ADI_DEV_DEVICE_HANDLE 	DMHandle,				// device handle
	ADI_DEV_PDD_HANDLE 		*pPDDHandle,			// pointer to PDD handle location 
	ADI_DEV_DIRECTION 		Direction,				// data direction
	void					*pCriticalRegionArg,	// critical region imask storage location
	ADI_DMA_MANAGER_HANDLE	DMAHandle,				// handle to the DMA manager
	ADI_DCB_HANDLE			DCBHandle,				// callback handle
	ADI_DCB_CALLBACK_FN		DMCallback				// device manager callback function
);

static u32 adi_pdd_Close(		// Closes the AD7476A device
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

// the callback function passed to the SPI driver
static void spiCallbackFunction(void* DeviceHandle, u32 Event, void* pArg);
	

// Validation function (debug build only)	
#if defined(ADI_DEV_DEBUG)
static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle);
#endif


/*********************************************************************

Entry point for device manager

*********************************************************************/


ADI_DEV_PDD_ENTRY_POINT ADI_AD7476A_EntryPoint = {
	adi_pdd_Open,
	adi_pdd_Close,
	adi_pdd_Read,
	adi_pdd_Write,
	adi_pdd_Control
};


/*********************************************************************

Entry point table for SPI Device Driver

*********************************************************************/
extern ADI_DEV_PDD_ENTRY_POINT ADISPIEntryPoint;

/*********************************************************************
*
*	Function:		adi_pdd_Open
*
*	Description:	Opens the AD7476A device for use
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
	ADI_AD7476A *pAD7476A;		// pointer to the AD7476A device we're working on
	void *pExitCriticalArg;		// exit critical region parameter
	
	// Check for a valid device number and that we're only doing input
#ifdef ADI_DEV_DEBUG
	if (DeviceNumber >= ADI_AD7476A_NUM_DEVICES) return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
	if (Direction != ADI_DEV_DIRECTION_INBOUND) return (ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED);
#endif

	// assign the pointer to the device instance
	pAD7476A = &Device[DeviceNumber];
	// and store the Device Manager handle 
	pAD7476A->DMHandle = DMHandle;
	// and callback function
	pAD7476A->DMCallback = DMCallback;
	// Assign the quiet period factor, to enable us to more accurately 
	// modify the requested sample rate to what is achievable. This value is the 
	// (1+e)*1000, where e = (tsample - tconvert)/tconvert.
	// (see description under case ADI_AD7476A_CMD_SET_QUIET_PERIOD_FACTOR in the 
	// adi_pdd_Control function for more details). 
	// The default value is based on experimental evidence using two ADSP-BF537 
	// EZ-Kits.
	pAD7476A->tquiet_factor = 1062;
	// Check that this device instance is not already in use. If not, 
	// assign flag to indicate that it is now.
	// the device starts in powerup mode
	//pAD7476A->CurMode = ADI_AD7476A_MODE_NORMAL;
	pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
	if (pAD7476A->InUseFlag == FALSE) {
		pAD7476A->InUseFlag = TRUE;
		Result = ADI_DEV_RESULT_SUCCESS;
	}
	adi_int_ExitCriticalRegion(pExitCriticalArg);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Open the SPI driver 
	Result = adi_dev_Open(
					ManagerHandle,			// device manager handle
					&ADISPIDMAEntryPoint,	// dma-driven SPI driver entry point
					0,						// device number
					pAD7476A,				// client handle - passed to internal callback function
					&pAD7476A->spiHandle,	// pointer to DM handle (for SPI driver) location 
					Direction,				// data direction as input
					DMAHandle,				// handle to the DMA manager
					DCBHandle,				// handle to the callback manager
					spiCallbackFunction		// internal callback function 
	);
	// return with appropriate code if SPI driver fails to open
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Configure the SPI Control register (SPI_CTL) for the 
	// appropriate values for the AD7476A device
	ADI_DEV_CMD_VALUE_PAIR SPI_config[] = {
		{ ADI_SPI_CMD_SET_TRANSFER_INIT_MODE, (void*)2 },	// DMA read
		{ ADI_SPI_CMD_SET_GET_MORE_DATA, (void*)0 },		// discard incoming data if SPI_RDBR register is full
		{ ADI_SPI_CMD_SET_PSSE, (void*)0 },					// free up PF0
		{ ADI_SPI_CMD_SET_MISO, (void*)1 },					// allow data in
		{ ADI_SPI_CMD_SET_WORD_SIZE, (void*)16 },			// Data word size 16 Bit (we need 16bits
															// since the AD7476A device generates 12 bits data with 4 leading zeros)
		{ ADI_SPI_CMD_SET_LSB_FIRST, (void*)0 },			// Most Significant Bit transmitted first
		{ ADI_SPI_CMD_SET_CLOCK_PHASE, (void*)0 },			// Clock phase field = 0 is required for DMA
		{ ADI_SPI_CMD_SET_CLOCK_POLARITY, (void*)1 },		// Clock polarity - sample on falling edge of serial clock
		{ ADI_SPI_CMD_SET_MASTER, (void*)1 },				// The SPI port is the master
		{ ADI_SPI_CMD_SET_EXCLUSIVE_ACCESS, (void*) TRUE},	// get us exclusive access to the SPI driver
		{ ADI_DEV_CMD_END, (void*)0 }
	};
	Result = adi_dev_Control( pAD7476A->spiHandle, ADI_DEV_CMD_TABLE, (void*)SPI_config );
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// save the physical device handle in the client supplied location
	*pPDDHandle = (ADI_DEV_PDD_HANDLE *)pAD7476A;

	// return after successful completion 
	return(Result);
}
		
		
/*********************************************************************
*
*	Function:		adi_pdd_Close
*
*	Description:	Closes down a SPI device
*
*********************************************************************/


static u32 adi_pdd_Close(		// Closes a device
	ADI_DEV_PDD_HANDLE PDDHandle	// PDD handle
) {
	u32 Result;		// return value
	ADI_AD7476A *pAD7476A = (ADI_AD7476A *)PDDHandle;

	// check for errors if required
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// close SPI driver
	Result = adi_dev_Close(pAD7476A->spiHandle);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// mark the device as closed
	pAD7476A->InUseFlag = FALSE;
	
	return(Result);
}
	

/*********************************************************************
*
*	Function:		adi_pdd_Read
*
*	Description:	Never called as SPI uses DMA
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
	ADI_AD7476A *pAD7476A = (ADI_AD7476A *)PDDHandle;
	Result = adi_dev_Read(pAD7476A->spiHandle,BufferType,pBuffer);

	return(Result);
}
	
/*********************************************************************
*
*	Function:		adi_pdd_Write
*
*	Description:	Never called as SPI uses DMA
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
*	Description:	Configures the SPI device
*
*********************************************************************/

static u32 adi_pdd_Control(		// Sets or senses a device specific parameter
	ADI_DEV_PDD_HANDLE 	PDDHandle,	// PDD handle
	u32 				Command,	// command ID
	void 				*Value		// command specific value
) {
	
	ADI_AD7476A 				*pAD7476A;		// pointer to the device we're working on
	u32 						tmp;			// temporary storage
	u32 						Result;			// return value
	u32							u32Value;		// u32 type to avoid casts/warnings etc.
	u16							u16Value;		// u16 type to avoid casts/warnings etc.
	
	// avoid casts
	pAD7476A = (ADI_AD7476A *)PDDHandle; // Pointer to AD7476A device driver instance
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
			// no, we do support it at this level (it is delegated down to SPI driver
			*((u32 *)Value) = FALSE;
			break;

		// CASE (Set the sample rate of the AD7476A device)
		case (ADI_AD7476A_CMD_SET_SAMPLE_RATE):
			{
				// note that for this option the user passes in the address containing
				// the requested sampling rate value, which is replaced by the achievable
				// value - due to the granularity of the SPI_BAUD register.
				u32 fsample = *(u32*)Value;
				
				// Obtain the System Clock freqency, fsclk, from  the
				// power management module of the System Services library. This is returned
				// as a MHz value. If the Power management module is not initialized, the 
				// reset value is used.
				u32 fcclk,fsclk,fvco;
				if (adi_pwr_GetFreq(&fcclk,&fsclk,&fvco)!=ADI_PWR_RESULT_SUCCESS)
					fsclk = 50000000;
				
				// Calculate the BAUD rate.
				// The calculation is split into two parts to enable integer arithmetic to be
				// used. NB. fsclk ~ O(1E8), pAD7476A->tquiet_factor # O(1000), so that x will 
				// be at least 0(1E4).
				u32 y = 32*pAD7476A->tquiet_factor;
				u32 x = fsclk/y;
				// remainder (is significant for calculation of achievable sample rate)
				u32 yy = fsclk - x*y;
				// We can only take the higher Baud value (i.e. if the floating point 
				// value is 3.5 then we must use 4 as using 3 would result in a higher 
				// sampling rate than requested).
				u32 spi_baud = (x*1000/fsample) + 1;
				
				// and configure the SPI_BAUD register in the SPI device driver
				Result = adi_dev_Control(pAD7476A->spiHandle, ADI_SPI_CMD_SET_BAUD_REG, (void*)spi_baud);

				// Now calculate the revised value which is returned to the user to facilitate the
				// the interpretation of the sampled data. 
				// For example, it is the value to be used in the Sample 
				// Rate box of the Data Processing tab of the Plot Settings dialog to enable
				// the '2D FFT magnitude' Data Process to correctly identify the frequency
				// content in the sampled data.
				fsample = x*1000/spi_baud;
				fsample += (yy*1000/(y*spi_baud)); 
				*(u32*)Value = fsample;

			}
			break;

		// CASE (Set mode of operation)
		case (ADI_AD7476A_CMD_SET_OPERATION_MODE):
			
			// IF (powering down the device)
			//if (u32Value==ADI_AD7476A_MODE_POWERDOWN && pAD7476A->CurMode==ADI_AD7476A_MODE_NORMAL)
			if (u32Value == ADI_AD7476A_MODE_POWERDOWN) {
				
				// pause SPI dataflow
				if (Result = adi_dev_Control(pAD7476A->spiHandle, ADI_SPI_CMD_PAUSE_DATAFLOW, (void*)TRUE)) break;
				
				// execute a dummy 8 bit read to force the part into powerdown
				if (Result = adi_dev_Control(pAD7476A->spiHandle, ADI_SPI_CMD_EXECUTE_DUMMY_READ, (void*)8)) break;
				
			// ELSE 
			//} else if (u32Value==ADI_AD7476A_MODE_NORMAL && pAD7476A->CurMode==ADI_AD7476A_MODE_POWERDOWN)  {
			} else {
				
				// execute a dummy 16 bit read to force the part to power up
				if (Result = adi_dev_Control(pAD7476A->spiHandle, ADI_SPI_CMD_EXECUTE_DUMMY_READ, (void*)16)) break;
				
				// resume SPI dataflow
				if (Result = adi_dev_Control(pAD7476A->spiHandle, ADI_SPI_CMD_PAUSE_DATAFLOW, (void*)FALSE)) break;
				
			// ENDIF
			}
			break;

		// CASE (declares the slave select line we're connected to)
		case (ADI_AD7476A_CMD_DECLARE_SLAVE_SELECT):

			// tell the SPI driver to enable our slave select line		
			// note that because we use DMA and therefore CPHA = 0, the hardware
			// automatically drives the slave select line so we don't have to set the 
			// FLGx bits of the SPI flag register, only enable the proper slave select
			Result = adi_dev_Control(pAD7476A->spiHandle, ADI_SPI_CMD_ENABLE_SLAVE_SELECT, Value);
			break;
		
		// CASE (replace the default quiet period factor)
		case (ADI_AD7476A_CMD_SET_QUIET_PERIOD_FACTOR):

			// Should the build-in quiet period factor not be sufficient, the user
			// can change it. The requested value should conform to the form
			// (1+e)*1000,  where e = (tsample - tconvert)/tconvert, ie. the proportion 
			// of time after the conversion is complete and the next falling edge of 
			// /cs line. 
			// An integer value is used to avoid pulling in floating point library. This 
			// limits the value of e to 3 decimal places, eg: 0.062 in the default case
			pAD7476A->tquiet_factor = u32Value;

		// DEFAULT 
		default:
		
			// pass anything we don't specifically handle to the SPI driver
			Result = adi_dev_Control(pAD7476A->spiHandle,Command,Value);
			break;
		
	// ENDCASE
	}
		
	// return
	return(Result);
}

/*********************************************************************

	Function:		spiCallbackFunction

	Description:	Fields the callback from the SPI Driver

*********************************************************************/

static void spiCallbackFunction(void* DeviceHandle, u32 Event, void* pArg)
{
	ADI_AD7476A *pAD7476A = (ADI_AD7476A *)DeviceHandle;
#if defined(ADI_DEV_DEBUG)
	if (ValidatePDDHandle(DeviceHandle) != ADI_DEV_RESULT_SUCCESS) {  	// verify the DM handle
		return;
	}
#endif
	// simply pass the callback along to the Device Manager Callback without interpretation
	(pAD7476A->DMCallback)(pAD7476A->DMHandle,Event,pArg);
}

#if defined(ADI_DEV_DEBUG)

/*********************************************************************

	Function:		ValidatePDDHandle

	Description:	Validates a PDD handle

*********************************************************************/

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle) {
	unsigned int i;
	for (i = 0; i < ADI_AD7476A_NUM_DEVICES; i++) {
		if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i]) {
			return (ADI_DEV_RESULT_SUCCESS);
		}
	}
	return (ADI_DEV_RESULT_BAD_PDD_HANDLE);
	}


#endif



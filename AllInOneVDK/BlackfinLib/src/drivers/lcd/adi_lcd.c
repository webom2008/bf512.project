/*********************************************************************************

Copyright(c) 2006 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.


Description:
			This is the driver source code for the LCD panel. It is layered
			on top of the PPI device driver, which is configured for the specific use of
			the LCD module.

Note:	This driver supports NEC-NL6448BC35-54 and Sharp LQ10D368
*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/
#include <services/services.h>							// system services
#include <drivers/adi_dev.h>							// device driver API
#include <drivers/ppi/adi_ppi.h>       					// PPI driver includes
#include <drivers/lcd/adi_lcd.h>			// LCD driver includes

/*********************************************************************

Check to insure that one of the NEC or Sharp LCD is built but not the LCD driver by itself.

*********************************************************************/
#define ADI_SHARP_LQ10D368



#ifndef ADI_NEC_NL6448BC33_54
#ifndef ADI_SHARP_LQ10D368
#error "*** ERROR: Must build either adi_nl6448bc33_53 or adi_lq10d368.c not adi_lcd.c ***"
#endif
#endif




/*********************************************************************

Enumerations and defines

*********************************************************************/

#define ADI_LCD_NUM_DEVICES	(sizeof(Device)/sizeof(ADI_LCD))	// number of LCD devices in the system


// Structure to contain data for an instance of the LCD device driver
#pragma pack(4)
typedef struct {
    ADI_DEV_MANAGER_HANDLE      ManagerHandle;  // Manager Handle
    ADI_DEV_DEVICE_HANDLE       DMHandle;       // Handle of Device Manager instance
    u32                     	InUseFlag;      // flag to indicate whether an instance is in use or not
    ADI_DMA_MANAGER_HANDLE      DMAHandle;      // handle to the DMA manager
    ADI_DCB_HANDLE              DCBHandle;      // callback handle
    ADI_DCB_CALLBACK_FN         DMCallback;     // the callback function supplied by the Device Manager
    ADI_DEV_PDD_HANDLE          ppiHandle;          // Handle to the underlying PPI device driver
    u32 			PPINumber;	// PPI hardware number
} ADI_LCD;
#pragma pack()


/*********************************************************************

Device specific data

*********************************************************************/

/* Edinburgh *******************************************************/
#if defined(__ADSP_EDINBURGH__) 	// settings for Edinburgh class devices

// The initial values for the device instance
static ADI_LCD Device[] = {
		{ NULL,NULL,false,NULL,NULL,NULL,NULL,0 },
};
#endif



/* Braemar *********************************************************/
#if defined(__ADSP_BRAEMAR__)  		// settings for Braemar class devices
static ADI_LCD Device[] = {
		{ NULL,NULL,false,NULL,NULL,NULL,NULL,0 },
};
#endif



/* Teton-Lite ******************************************************/
#if defined(__ADSP_TETON__)			// settings for Teton class devices
static ADI_LCD Device[] = {
		{ NULL,NULL,false,NULL,NULL,NULL,NULL,0 },	// device 0
		{ NULL,NULL,false,NULL,NULL,NULL,NULL,1 },	// device 1
};
#endif


// The prototpe functions for the LCD device driver

static u32 adi_pdd_Open(					// Open the LCD device
	ADI_DEV_MANAGER_HANDLE	ManagerHandle,			// device manager handle
	u32 			DeviceNumber,			// device number
	ADI_DEV_DEVICE_HANDLE 	DMHandle,				// device handle
	ADI_DEV_PDD_HANDLE 	*pPDDHandle,			// pointer to PDD handle location
	ADI_DEV_DIRECTION 	Direction,				// data direction
	void			*pCriticalRegionArg,	// critical region imask storage location
	ADI_DMA_MANAGER_HANDLE	DMAHandleNULL,				// handle to the DMA manager
	ADI_DCB_HANDLE		DCBHandle,				// callback handle
	ADI_DCB_CALLBACK_FN	DMCallback				// device manager callback function
);

static u32 adi_pdd_Close(		// Closes the AD1854 device
	ADI_DEV_PDD_HANDLE PDDHandle	// PDD handle
);

static u32 adi_pdd_SeqIO(		// Sequential IO buffer to a device
	ADI_DEV_PDD_HANDLE PDDHandle,	// PDD handle
	ADI_DEV_BUFFER_TYPE	BufferType,	// buffer type
	ADI_DEV_BUFFER *pBuffer			// pointer to buffer
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

// the callback function passed to the PPI driver
static void ppiCallbackFunction(void* DeviceHandle, u32 Event, void* pArg);

static u32 PPI_Open(
    ADI_DEV_PDD_HANDLE PDDHandle    // Physical Device Driver Handle
);


// Validation function (debug build only)
#if defined(ADI_DEV_DEBUG)
static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle);
#endif


/*********************************************************************

Entry point for device manager

*********************************************************************/

#if defined ADI_NEC_NL6448BC33_54
ADI_DEV_PDD_ENTRY_POINT ADI_NL6448BC3354_EntryPoint = {
#endif
#if defined ADI_SHARP_LQ10D368
ADI_DEV_PDD_ENTRY_POINT ADI_LQ10D368_EntryPoint = {
#endif

	adi_pdd_Open,
	adi_pdd_Close,
	adi_pdd_Read,
	adi_pdd_Write,
	adi_pdd_Control,
	adi_pdd_SeqIO

};



/*********************************************************************
*
*	Function:		adi_pdd_Open
*
*	Description:	Opens the ADI_LCD device for use
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
	ADI_LCD *pLCD; // pointer to the ADI_LCD device we're working on
	void *pExitCriticalArg;		// exit critical region parameter

	// Check for a valid device number and that we're only doing output
#ifdef ADI_DEV_DEBUG
	if (DeviceNumber >= ADI_LCD_NUM_DEVICES) return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
	if (Direction != ADI_DEV_DIRECTION_OUTBOUND) return (ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED);
#endif

	// assign the pointer to the device instance
	pLCD = &Device[DeviceNumber];
    	// and store the Manager handle
    	pLCD->ManagerHandle = ManagerHandle;
    	// and store the Device Manager handle
    	pLCD->DMHandle = DMHandle;
    	// and store the DMA Manager handle
    	pLCD->DMAHandle = DMAHandle;
    	// and store the DCallback Manager handle
    	pLCD->DCBHandle = DCBHandle;
    	// and callback function
    	pLCD->DMCallback = DMCallback;

	// insure the device the client wants is available
    	Result = ADI_DEV_RESULT_DEVICE_IN_USE;
    	// If not, assign flag to indicate that it is now.
	pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
	if (pLCD->InUseFlag == FALSE) {
		pLCD->InUseFlag = TRUE;
		Result = ADI_DEV_RESULT_SUCCESS;
	}
	adi_int_ExitCriticalRegion(pExitCriticalArg);

#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// save the physical device handle in the client supplied location
	*pPDDHandle = (ADI_DEV_PDD_HANDLE *)pLCD;

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
	ADI_LCD *pLCD = (ADI_LCD *)PDDHandle;

	// check for errors if required
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
	// close PPI driver
	Result = adi_dev_Close(pLCD->ppiHandle);

#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// mark the device as closed
	pLCD->InUseFlag = FALSE;
	pLCD->ManagerHandle = NULL;
    	pLCD->DMHandle = NULL;
    	pLCD->DMAHandle = NULL;
    	pLCD->DCBHandle = NULL;
    	pLCD->DMCallback = NULL;
    	pLCD->ppiHandle = NULL;

	return(Result);
}

/*********************************************************************
*
*	Function:		adi_pdd_SeqIO
*
*	Description:	Never called as device only outputs data
*
*********************************************************************/


static u32 adi_pdd_SeqIO(		// Reads data or queues an inbound buffer to a device
	ADI_DEV_PDD_HANDLE 	PDDHandle,	// PDD handle
	ADI_DEV_BUFFER_TYPE	BufferType,	// buffer type
	ADI_DEV_BUFFER 		*pBuffer	// pointer to buffer
){
	// this device only outputs data so we should never get here
	return ADI_DEV_RESULT_NOT_SUPPORTED;

}


/*********************************************************************
*
*	Function:		adi_pdd_Read
*
*	Description:	Never called as device only outputs data
*
*********************************************************************/


static u32 adi_pdd_Read(		// Reads data or queues an inbound buffer to a device
	ADI_DEV_PDD_HANDLE 	PDDHandle,	// PDD handle
	ADI_DEV_BUFFER_TYPE	BufferType,	// buffer type
	ADI_DEV_BUFFER 		*pBuffer	// pointer to buffer
){
	// this device only outputs data so we should never get here
	return ADI_DEV_RESULT_NOT_SUPPORTED;

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
	ADI_LCD *pLCD = (ADI_LCD *)PDDHandle;
	Result = adi_dev_Write(pLCD->ppiHandle,BufferType,pBuffer);

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

	ADI_LCD 				*pLCD;		// pointer to the device we're working on
	u32 						tmp;			// temporary storage
	u32 						Result;			// return value
	u32							u32Value;		// u32 type to avoid casts/warnings etc.
	u16							u16Value;		// u16 type to avoid casts/warnings etc.

	// avoid casts
	pLCD = (ADI_LCD *)PDDHandle; // Pointer to device instance
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
			// no, we do support it at this level (it is delegated down to PPI driver
			*((u32 *)Value) = FALSE;
		break;


	    // CASE (Set PPI Device Number that will be used and open the PPI device)
#if defined ADI_NEC_NL6448BC33_54
    	case (ADI_NL6448BC3354_CMD_OPEN_PPI):
#else
    	case (ADI_LQ10D368_CMD_OPEN_PPI):
#endif

// Check PPI device number
#if defined(__ADSP_TETON__)     // two PPI ports available in BF561
        	if (u32Value > 1)
#if defined ADI_NEC_NL6448BC33_54
		return ( ADI_NL6448BC3354_RESULT_BAD_PPI_DEVICE);
#else
		return ( ADI_LQ10D368_RESULT_BAD_PPI_DEVICE);
#endif
#elif defined(__ADSP_BRAEMAR__) || defined (__ADSP_EDINBURGH__)     // Only one PPI port available in BF533 & BF537
            if (u32Value > 0)
#if defined ADI_NEC_NL6448BC33_54
		return ( ADI_NL6448BC3354_RESULT_BAD_PPI_DEVICE);
#else
		return ( ADI_LQ10D368_RESULT_BAD_PPI_DEVICE);
#endif
#endif
        	// Update the PPI device number
        	pLCD->PPINumber = u32Value;

        	// open PPI device if it is not opened.
    		if (pLCD->ppiHandle) return ADI_DEV_RESULT_SUCCESS;
    		else {
    		 Result = PPI_Open(PDDHandle);
    		}


		break;


		// CASE (query for LCD buffer padding)
#if defined ADI_NEC_NL6448BC33_54
		case (ADI_NL6448BC3354_CMD_GET_TOP_PADDING):
		*((u32 *)Value) = 33;
#else
		case (ADI_LQ10D368_CMD_GET_TOP_PADDING):
		*((u32 *)Value) = 34;
#endif
		break;


		// DEFAULT
		default:
			// pass anything we don't specifically handle to the PPI driver
			return adi_dev_Control(pLCD->ppiHandle,Command,Value);

	// ENDCASE
	}

	return(Result);
}

/*********************************************************************

	Function:		ppiCallbackFunction

	Description:	Fields the callback from the PPI Driver

*********************************************************************/

static void ppiCallbackFunction(void* DeviceHandle, u32 Event, void* pArg)
{
	ADI_LCD *pLCD = (ADI_LCD *)DeviceHandle;
#if defined(ADI_DEV_DEBUG)
	if (ValidatePDDHandle(DeviceHandle) != ADI_DEV_RESULT_SUCCESS) {  	// verify the DM handle
		return;
	}
#endif
	// simply pass the callback along to the Device Manager Callback without interpretation
	(pLCD->DMCallback)(pLCD->DMHandle,Event,pArg);
}

/*********************************************************************
*
*   Function:       PPI_Open
*
*   Description:    Opens the ppi device for NL6448BC33-54 LCD dataflow
*
*********************************************************************/
static u32 PPI_Open(
    ADI_DEV_PDD_HANDLE PDDHandle    // Physical Device Driver Handle
) {


    ADI_LCD *pLCD;	// pointer to the device we're working on
    pLCD = (ADI_LCD *)PDDHandle; // Pointer to device driver instance

    // default return code
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    // storage for PPI frame sync timer values
    ADI_PPI_FS_TMR FsTmrBuf = {0};  // initialize - sets all fields to zero

    // Configure the PPI registers for LCD
	ADI_DEV_CMD_VALUE_PAIR PPI_Cfg[] = {
#if defined (__ADSPBF561__)
        // by default, PPI is configured as output & DMA32 enabled
	{ ADI_PPI_CMD_SET_CONTROL_REG,	 (void *)0xB91E	},// tx-mode,2or3 syncs,16bits and FS_INVERT = 1
#else	// for BF533 & BF537
    	{ ADI_PPI_CMD_SET_CONTROL_REG,	 (void *)0xB81E	},// tx-mode,2or3 syncs,16bits and FS_INVERT = 1
#endif
#if defined ADI_NEC_NL6448BC33_54
	{ ADI_PPI_CMD_SET_DELAY_COUNT_REG, 	(void *)143 },// clk delay after frame sync
#else // ADI_SHARP_LQ10D368
#if defined (__ADSPBF537__)
    	{ ADI_PPI_CMD_SET_DELAY_COUNT_REG, 	(void *)199 },//BF537 clk delay after frame sync
#else
    	{ ADI_PPI_CMD_SET_DELAY_COUNT_REG, 	(void *)199 },//BF533 or BF561 clk delay after frame sync
#endif
#endif

		{ ADI_PPI_CMD_SET_TRANSFER_COUNT_REG,	(void *)639 	},// 640 active pixels
		{ ADI_DEV_CMD_END, 			(void *)0 	}
    	};



    // Open the PPI driver
    Result = adi_dev_Open(
        pLCD->ManagerHandle,	// device manager handle
        &ADIPPIEntryPoint,          // ppi Entry point
        pLCD->PPINumber,  // ppi device number
        pLCD,                   // client handle - passed to internal callback function
        &pLCD->ppiHandle,       // pointer to DM handle (for PPI driver) location
        ADI_DEV_DIRECTION_OUTBOUND, // PPI used only to receive video data
        pLCD->DMAHandle,        // handle to the DMA manager
        pLCD->DCBHandle,        // handle to the callback manager
        ppiCallbackFunction         // internal callback function
    );

    // return with appropriate code if PPI driver fails to open
#ifdef ADI_DEV_DEBUG
    if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
	// set the PPI configuration
	Result = adi_dev_Control( pLCD->ppiHandle, ADI_DEV_CMD_TABLE, (void*)PPI_Cfg );
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// set the frame sync configuration.
	FsTmrBuf.pulse_hi = 0;//positive action pulse
	FsTmrBuf.emu_run = 1;// timer counter runs during emulation

	// timer 0 configuration (Hsync)
#if defined ADI_NEC_NL6448BC33_54
	FsTmrBuf.period = 800; // timer0 period (800clk = 1H)
#else // ADI_SHARP_LQ10D368
	FsTmrBuf.period = 900; // timer0 period (900clk)
#endif
	FsTmrBuf.width = 96; // timer0 width (96clk)
	Result = adi_dev_Control( pLCD->ppiHandle, ADI_PPI_CMD_SET_TIMER_FRAME_SYNC_1, (void*)&FsTmrBuf );

#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
	// timer 1 configuration (Vsync)
#if defined ADI_NEC_NL6448BC33_54
	FsTmrBuf.period = 0x668A0; // timer1 period (525H = 525*800clk)
#else // ADI_SHARP_LQ10D368
	FsTmrBuf.period = 0x735B4; // timer1 period (525*900clk)
#endif
	FsTmrBuf.width = 1600; // timer1 width (2H = 2*800clk)
	Result = adi_dev_Control( pLCD->ppiHandle, ADI_PPI_CMD_SET_TIMER_FRAME_SYNC_2, (void*)&FsTmrBuf );

#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif


    return (Result);
}


#if defined(ADI_DEV_DEBUG)

/*********************************************************************

	Function:		ValidatePDDHandle

	Description:	Validates a PDD handle

*********************************************************************/

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle) {
	unsigned int i;
	for (i = 0; i < ADI_LCD_NUM_DEVICES; i++) {
		if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i]) {
			return (ADI_DEV_RESULT_SUCCESS);
		}
	}
	return (ADI_DEV_RESULT_BAD_PDD_HANDLE);
	}


#endif



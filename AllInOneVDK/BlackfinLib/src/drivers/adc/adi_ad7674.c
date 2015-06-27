/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_ad7674.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			This is the driver source code for the AD7674 ADC peripheral. It is layered
			on top of the PPI device driver, which is configured for the specific use of
			the AD7674 peripheral.

*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/

#define ADI_AD7674_MODULE

#include <services/services.h>		// system service includes

#ifndef __ADSP_BRAEMAR__
#error "AD7674 driver is not available for the current processor"
#endif

#include <drivers/adi_dev.h>			// device manager includes
#include <drivers/ppi/adi_ppi.h>       	// spi driver includes
#include <drivers/adc/adi_ad7674.h> 	// AD7674 driver includes

/*********************************************************************

Enumerations and defines

*********************************************************************/

#define ADI_AD7674_NUM_DEVICES	(sizeof(Device)/sizeof(ADI_AD7674))	// number of AD7674 devices in the system

// Structure to contain data for an instance of the AD7476 device driver
#pragma pack(4)
typedef struct {
	ADI_DEV_DEVICE_HANDLE		DMHandle;					// Handle of Device Manager instance
	ADI_DEV_PDD_HANDLE			ppiHandle;				    // Handle to the underlying PPI device driver
	u32							InUseFlag;					// flag to indicate whether an instance is in use or not
	ADI_DCB_CALLBACK_FN			DMCallback;					// the callback function supplied by the Device Manager
	ADI_AD7674_PIN_DEF			pins;						// Pins used to select the user configuratable GPIO pins for ADC control
	u32							cmode;						// Conversion mode (IMPUSLE|NORMAL|WARP)
	u32							fsclk;						// SCLK frequency (Hz)
	u32							fpclk;						// PPI_CLK frequency (Hz)
	u32							fsample;					// Sample rate (Hz)
	u16							portx_fer;					// Original PORTx_FER settings (whatever port we have chosen - see pins structure)
	u16							port_mux;					// Original PORT_MUX settings
	u32							a0_continuous;				// determines whether A0 is continuous or pulse set to PPI_DELAY + 1
	u32							MSBFirst;					// sampling order
} ADI_AD7674;
#pragma pack()

// The data structure instances for each device (SPORT0, SPORT1)
static ADI_AD7674 Device[1];

// The prototpe functions for the AD7674 device driver

static u32 adi_pdd_Open(					// Open the AD7674 device
	ADI_DEV_MANAGER_HANDLE	ManagerHandle,			// device manager handle
	u32 					DeviceNumber,			// device number
	ADI_DEV_DEVICE_HANDLE 	DMHandle,				// device handle
	ADI_DEV_PDD_HANDLE 		*DeviceHandle,			// pointer to PDD handle location
	ADI_DEV_DIRECTION 		Direction,				// data direction
	void					*pCriticalRegionArg,	// critical region imask storage location
	ADI_DMA_MANAGER_HANDLE	DMAHandle,				// handle to the DMA manager
	ADI_DCB_HANDLE			DCBHandle,				// callback handle
	ADI_DCB_CALLBACK_FN		DMCallback				// device manager callback function
);

static u32 adi_pdd_Close(		// Closes the AD7674 device
	ADI_DEV_PDD_HANDLE DeviceHandle	// PDD handle
);

static u32 adi_pdd_Read(		// Reads data or queues an inbound buffer to a device
	ADI_DEV_PDD_HANDLE DeviceHandle,	// PDD handle
	ADI_DEV_BUFFER_TYPE	BufferType,	// buffer type
	ADI_DEV_BUFFER *pBuffer			// pointer to buffer
);

static u32 adi_pdd_Write(		// Writes data or queues an outbound buffer to a device
	ADI_DEV_PDD_HANDLE DeviceHandle,	// PDD handle
	ADI_DEV_BUFFER_TYPE	BufferType,	// buffer type
	ADI_DEV_BUFFER *pBuffer			// pointer to buffer
);

static u32 adi_pdd_Control(		// Sets or senses a device specific parameter
	ADI_DEV_PDD_HANDLE DeviceHandle,	// PDD handle
	u32 Command,					// command ID
	void *Value						// command specific value
);

static ADI_INT_HANDLER(ErrorHandler);		// interrupt handler for errors

// the callback function passed to the PPI driver
static void ppiCallbackFunction(void* DeviceHandle, u32 Event, void* pArg);


// Validation function (debug build only)
#if defined(ADI_DEV_DEBUG)
static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE DeviceHandle);
#endif

///////////////////////////////////////////////////////////////////////
// Convenience pointers to MMR locations for PORT registers
static volatile u16 *pADI_PORTx_FER = (volatile u16 *)(0xFFC03200);
// pADI_PORTx_FER[0] -- Port F
// pADI_PORTx_FER[2] -- Port G
// pADI_PORTx_FER[4] -- Port H

static volatile u16 *pADI_PORTFIO_CLEAR = (volatile u16 *)(0xFFC00704);
static volatile u16 *pADI_PORTFIO_SET = (volatile u16 *)(0xFFC00708);
static volatile u16 *pADI_PORTFIO_DIR = (volatile u16 *)(0xFFC00730);

static volatile u16 *pADI_PORTGIO_CLEAR = (volatile u16 *)(0xFFC01504);
static volatile u16 *pADI_PORTGIO_SET = (volatile u16 *)(0xFFC01508);
static volatile u16 *pADI_PORTGIO_DIR = (volatile u16 *)(0xFFC01530);

static volatile u16 *pADI_PORTHIO_CLEAR = (volatile u16 *)(0xFFC01704);
static volatile u16 *pADI_PORTHIO_SET = (volatile u16 *)(0xFFC01708);
static volatile u16 *pADI_PORTHIO_DIR = (volatile u16 *)(0xFFC01730);
//

///////////////////////////////////////////////////////////////////////
// static functions
static void SetSampleRate(ADI_DEV_PDD_HANDLE DeviceHandle, u32 *fsample);
static void CalcSamplingParameters(ADI_AD7674 *pAD7674, u32 *fsample, u32 *pclk_div, u32 *fs1div, u32 *ppi_delay);
static void ConfigureFlagPins(ADI_DEV_PDD_HANDLE DeviceHandle);
static void ResetFlagPins(ADI_DEV_PDD_HANDLE DeviceHandle);
static void ConfigurePPICLK(ADI_DEV_PDD_HANDLE DeviceHandle);
static void ResetPPICLK(ADI_DEV_PDD_HANDLE DeviceHandle);
static void SetConversionMode(ADI_DEV_PDD_HANDLE DeviceHandle, u32 mode);
static void ConfigureTimers(ADI_AD7674 *pAD7674, u32 pclk_div, u32 fs1_div, u32 ppi_delay);
static void EnablePPI_CLK(ADI_DEV_PDD_HANDLE DeviceHandle, u32 flag);
static void DisableTimers(ADI_DEV_PDD_HANDLE DeviceHandle);

// Max time to allow for BUSY period
static u32 ppi_delay_max[3] = { // nanosecs
//   t4  + t3
	1500 + 35,  // impulse
	1250 + 35,  // normal
	1000 + 35,  // warp
};

// Sample rates for different conversion modes
#define WARP_MAX_THROUGHPUT    800000
#define NORMAL_MAX_THROUGHPUT  666000
#define IMPULSE_MAX_THROUGHPUT 570000

/*********************************************************************

Entry point for device manager

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT ADI_AD7674_EntryPoint = {
	adi_pdd_Open,
	adi_pdd_Close,
	adi_pdd_Read,
	adi_pdd_Write,
	adi_pdd_Control
};


/*********************************************************************

Entry point table for PPI Device Driver

*********************************************************************/
extern ADI_DEV_PDD_ENTRY_POINT ADIPPIEntryPoint;

/*********************************************************************
*
*	Function:		adi_pdd_Open
*
*	Description:	Opens the AD7674 device for use
*
*********************************************************************/


static u32 adi_pdd_Open(
	ADI_DEV_MANAGER_HANDLE	ManagerHandle,			// device manager handle
	u32 					DeviceNumber,			// device number
	ADI_DEV_DEVICE_HANDLE	DMHandle,				// device handle
	ADI_DEV_PDD_HANDLE 		*DeviceHandle,			// pointer to PDD handle location
	ADI_DEV_DIRECTION 		Direction,				// data direction
	void					*pEnterCriticalArg,		// enter critical region parameter
	ADI_DMA_MANAGER_HANDLE	DMAHandle,				// handle to the DMA manager
	ADI_DCB_HANDLE			DCBHandle,				// callback handle
	ADI_DCB_CALLBACK_FN		DMCallback				// client callback function
) {

	// default return code
	u32 Result = ADI_DEV_RESULT_SUCCESS;
	ADI_AD7674 *pAD7674;		// pointer to the AD7674 device we're working on
	void *pExitCriticalArg;		// exit critical region parameter

	// Check for a valid device number and that we're only doing input
#ifdef ADI_DEV_DEBUG
	if (DeviceNumber >= ADI_AD7674_NUM_DEVICES) return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
	if (Direction != ADI_DEV_DIRECTION_INBOUND) return (ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED);
#endif

	// assign the pointer to the device instance
	pAD7674 = &Device[DeviceNumber];
	// and store the Device Manager handle
	pAD7674->DMHandle = DMHandle;
	// and callback function
	pAD7674->DMCallback = DMCallback;
	// assign in-use flag
	pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
	if (pAD7674->InUseFlag == FALSE) {
		pAD7674->InUseFlag = TRUE;
		Result = ADI_DEV_RESULT_SUCCESS;
	}
	adi_int_ExitCriticalRegion(pExitCriticalArg);

#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Open the PPI driver
	Result = adi_dev_Open(
					ManagerHandle,			// device manager handle
					&ADIPPIEntryPoint,		// PPI Entry point
					DeviceNumber,			// device number
					pAD7674,				// client handle - passed to internal callback function
					&pAD7674->ppiHandle,	// pointer to DM handle (for PPI driver) location
					Direction,				// data direction as input
					DMAHandle,				// handle to the DMA manager
					DCBHandle,				// handle to the callback manager
					ppiCallbackFunction	// internal callback function
	);
	// return with appropriate code if PPI driver fails to open
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Configure the PPI for the appropriate values for the AD7674 device
	// We are configuring the PPI to use 2 internal frame syncs:
	//    PPI_FS1 => _CNVST  on PF9
	//    PPI_FS2 => A0 on PF8 (which will timed to go low 1 PPI_CLK cycle after BUSY is deasserted)
	// In this way we will sample two 16 bit words containing the 18 bits required.

	ADI_PPI_CONTROL_REG ppi_control;

	// Configure the PPI_CONTROL register
	ppi_control.port_en  = 0;	// Disable until ready
	ppi_control.port_dir = 0;	// Receive mode
	ppi_control.xfr_type = 3;	// Non ITU-R 656 mode
	ppi_control.port_cfg = 1;	// two or three internal frame syncs
	ppi_control.fld_sel  = 0;	// ????
	ppi_control.skip_en  = 0;	// Not required
	ppi_control.skip_eo  = 0;	// Not required
	ppi_control.pack_en  = 0;	// Not required
	ppi_control.dlen	 = 7;	// 16 Bits data length
	ppi_control.polc	 = 0;	// Do not invert PPI_CLK
	ppi_control.pols     = 0;	// Do not invert PPI_FS1 & PPI_FS2
	//ppi_control.pols     = 1;	// invert PPI_FS1 & PPI_FS2

	// set PPI_COUNT to 1 to sample 2 16bit words.
	u16 ppi_count = 1;
	u16 ppi_frame = 1;

	ADI_DEV_CMD_VALUE_PAIR PPI_config[] = {
		{ ADI_PPI_CMD_SET_CONTROL_REG, 			(void*)(*(u16*)&ppi_control)	},
		{ ADI_PPI_CMD_SET_TRANSFER_COUNT_REG, 	(void*)(*(u16*)&ppi_count)		},
		{ ADI_PPI_CMD_SET_LINES_PER_FRAME_REG, 	(void*)(*(u16*)&ppi_frame)		},
		{ ADI_DEV_CMD_END, 0 }
	};
	// and pass the values to the PPI driver
	Result = adi_dev_Control( pAD7674->ppiHandle, ADI_DEV_CMD_TABLE, (void*)PPI_config );
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// set default data word order to MSB,LSB
	pAD7674->MSBFirst = TRUE;

	// set default PPI_CLK frequency at 40 MHz
	pAD7674->fpclk = 40000000;

	// default action for A0 signal (PPI_FS2) is to switch value at one PPI_CLK cycle
	// after the delay
	pAD7674->a0_continuous = FALSE;

	// Obtain the System Clock freqency, fsclk, from  the
	// power management module of the System Services library. This is returned
	// as a MHz value. If the Power management module is not initialized, the
	// reset value is used.
	u32 fcclk,fvco;
	if (adi_pwr_GetFreq(&fcclk,&pAD7674->fsclk,&fvco)!=ADI_PWR_RESULT_SUCCESS)
		pAD7674->fsclk = 50000000;

	// save the physical device handle in the client supplied location
	*DeviceHandle = (ADI_DEV_PDD_HANDLE *)pAD7674;

	// Configure default pin arrangements
	pAD7674->pins.PortIdent = ADI_AD7674_PINS_PORT_F;		// They will all be from Port F
	// TMR2 will generate PPI_CLK on PF7. This will need to be tied (by h/w) to the PPI_CLK input on PF15
	pAD7674->pins.PPI_CLK_TIMER_pin = 7;

	pAD7674->pins.CS_pin = 10;								// _CS will be mapped to PF10
	pAD7674->pins.WARP_pin = 11;							// IMPULSE pin is PF11
	pAD7674->pins.IMPULSE_pin = 12;							// WARP pin is PF12

	// Use WARP as default conversion mode
	adi_pdd_Control(*DeviceHandle,ADI_AD7674_CMD_SET_CONVERSION_MODE,(void*)ADI_AD7674_WARP_MODE);

	// return after successful completion
	return(Result);
}

/*********************************************************************
*
*	Function:		adi_pdd_Close
*
*	Description:	Closes down a PPI device
*
*********************************************************************/

static u32 adi_pdd_Close(		// Closes a device
	ADI_DEV_PDD_HANDLE DeviceHandle	// PDD handle
) {
	u32 Result;		// return value
	ADI_AD7674 *pAD7674 = (ADI_AD7674 *)DeviceHandle;

	// check for errors if required
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(DeviceHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Disable dataflow if not already done so
	adi_pdd_Control(DeviceHandle,ADI_DEV_CMD_SET_DATAFLOW, (void*)FALSE);

	// close PPI driver
	Result = adi_dev_Close(pAD7674->ppiHandle);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// mark the device as closed
	pAD7674->InUseFlag = FALSE;

	return(Result);
}


/*********************************************************************
*
*	Function:		adi_pdd_Read
*
*	Description:	Never called as PPI uses DMA
*
*********************************************************************/


static u32 adi_pdd_Read(		// Reads data or queues an inbound buffer to a device
	ADI_DEV_PDD_HANDLE 	DeviceHandle,	// PDD handle
	ADI_DEV_BUFFER_TYPE	BufferType,	// buffer type
	ADI_DEV_BUFFER 		*pBuffer	// pointer to buffer
){
	// check for errors if required
	u32 Result = ADI_DEV_RESULT_SUCCESS;
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(DeviceHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
	// Simply pass the request on
	ADI_AD7674 *pAD7674 = (ADI_AD7674 *)DeviceHandle;
	Result = adi_dev_Read(pAD7674->ppiHandle,BufferType,pBuffer);

	return(Result);
}

/*********************************************************************
*
*	Function:		adi_pdd_Write
*
*	Description:	Never called as PPI uses DMA
*
*********************************************************************/


static u32 adi_pdd_Write(		// Writes data or queues an outbound buffer to a device
	ADI_DEV_PDD_HANDLE 	DeviceHandle,	// PDD handle
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
*	Description:	Configures the PPI device
*
*********************************************************************/

static u32 adi_pdd_Control(		// Sets or senses a device specific parameter
	ADI_DEV_PDD_HANDLE 	DeviceHandle,	// PDD handle
	u32 				Command,	// command ID
	void 				*Value		// command specific value
) {

	ADI_AD7674 				*pAD7674;		// pointer to the device we're working on
	u32 					tmp;			// temporary storage
	u32 					Result;			// return value
	u32						u32Value;		// u32 type to avoid casts/warnings etc.
	u16						u16Value;		// u16 type to avoid casts/warnings etc.
	u16						sample_rate_set = 0;

	// avoid casts
	pAD7674 = (ADI_AD7674 *)DeviceHandle; // Pointer to AD7674 device driver instance
    // assign 16 and 32 bit values for the Value argument
	u32Value = (u32)Value;
    u16Value = ((u16)((u32)Value));

	// check for errors if required
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(DeviceHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
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

		// CASE (Set the sample rate of the AD7674 device)
		case (ADI_AD7674_CMD_SET_SAMPLE_RATE):
			{
				// note that for this option the user passes in the address containing
				// the requested sampling rate value, which is replaced by the achievable
				// value.
				u32 fsample = *(u32*)Value;
				// set appropriate conversion mode

				pAD7674->fsample = fsample;

				if (pAD7674->fsample <= IMPULSE_MAX_THROUGHPUT)		// IMPULSE
					pAD7674->cmode = ADI_AD7674_IMPULSE_MODE;
				else if (pAD7674->fsample <= NORMAL_MAX_THROUGHPUT) // NORMAL
					pAD7674->cmode = ADI_AD7674_NORMAL_MODE;
				else if (pAD7674->fsample <= WARP_MAX_THROUGHPUT)	// WARP
					pAD7674->cmode = ADI_AD7674_WARP_MODE;
				else
					return ADI_AD7674_RESULT_INVALID_SAMPLING_RATE;

				// Calculate and true sampling rate and configure the timers
				SetSampleRate(DeviceHandle, &pAD7674->fsample);

				// Pass back the modified sample rate
				*(u32*)Value = pAD7674->fsample;

			}
			break;

		// CASE (Set the PPI_CLK frequency)
		case (ADI_AD7674_CMD_SET_PPI_CLK):
			pAD7674->fpclk = u32Value;
			break;

		// CASE (Set the A0 toggle mode)
		case (ADI_AD7674_CMD_SET_A0_CONTINUOUS):
			pAD7674->a0_continuous = (u32Value ? 1 : 0);
			break;

		// CASE (Get the sample rate of the AD7674 device)
		case (ADI_AD7674_CMD_GET_SAMPLE_RATE):
			*(u32*)Value = pAD7674->fsample;
			break;

		// CASE (Chose word order)
		case (ADI_AD7674_CMD_SET_MSB_FIRST):
			pAD7674->MSBFirst = u32Value;
			break;

		// CASE (Set ADC pins)
		case ADI_AD7674_CMD_SET_PINS:
			{
			ADI_AD7674_PIN_DEF *pin_def = (ADI_AD7674_PIN_DEF *)Value;
#ifdef ADI_DEV_DEBUG
			// Check port value is in valid range
			if (pin_def->PortIdent > ADI_AD7674_PINS_PORT_H)
				return ADI_AD7674_RESULT_INVALID_PORT_SELECTION;

			// Check pin numbers are in valid range
			u16 ipin,*pin=&pin_def->PPI_CLK_TIMER_pin;
			for (ipin=0; ipin<ADI_AD7674_NUM_PINS; ipin++)
			{
				if (pin[ipin] > 15)
					return ADI_AD7674_RESULT_INVALID_PIN_SELECTION;
			}
#endif
			// save definition in device instance structure
			pAD7674->pins = *pin_def;
			}
			break;

		// CASE (Set mode of conversion )
		case (ADI_AD7674_CMD_SET_CONVERSION_MODE):
#ifdef ADI_DEV_DEBUG
			// Check selection value is in valid range
			if ( u32Value > ADI_AD7674_WARP_MODE)
				return ADI_AD7674_RESULT_INVALID_CONVERSION_MODE;
#endif
			SetConversionMode(DeviceHandle,u32Value);
			break;

		// CASE (Set data flow)
		case (ADI_DEV_CMD_SET_DATAFLOW):
			if (u32Value) // enable data flow
			{
				// Configure flag pins
				ConfigureFlagPins(DeviceHandle);
				// Enable PPI_CLK
				EnablePPI_CLK(DeviceHandle, TRUE);
				// Enable PPI
				Result = adi_dev_Control(pAD7674->ppiHandle,Command,Value);
			}
			else  // disable data flow
			{
				// Stop PPI
				Result = adi_dev_Control(pAD7674->ppiHandle,Command,Value);
				// Stop PPI_CLK
				EnablePPI_CLK(DeviceHandle, FALSE);
				// and reset the flag pins
				ResetFlagPins(DeviceHandle);
			}
			break;

		// DEFAULT
		default:

			// pass anything we don't specifically handle to the PPI driver
			Result = adi_dev_Control(pAD7674->ppiHandle,Command,Value);
			break;

	// ENDCASE
	}

	// return
	return(Result);
}

/*********************************************************************
	Function:		SetSampleRate
	Description:	Fully Powers down the converter by switching to 8bit words
*********************************************************************/
static void SetSampleRate(ADI_DEV_PDD_HANDLE DeviceHandle, u32 *fsample)
{
	ADI_AD7674 *pAD7674 = (ADI_AD7674 *)DeviceHandle;
	u32 fcclk,fsclk,fvco,fs1_div,ppi_delay,pclk_div;

	// Calculate the optimal divider values and update the sampling rate
	CalcSamplingParameters(
				pAD7674,
				fsample,
				&pclk_div,
				&fs1_div,
				&ppi_delay
				);

	// and finally, set the timers accordingly
	ConfigureTimers(DeviceHandle,pclk_div,fs1_div,ppi_delay);
}

/*********************************************************************
	Function:		ppiCallbackFunction
	Description:	Fields the callback from the PPI Driver
*********************************************************************/

static void ppiCallbackFunction(void* DeviceHandle, u32 Event, void* pArg)
{
	ADI_AD7674 *pAD7674 = (ADI_AD7674 *)DeviceHandle;
	ADI_DEV_BUFFER *pBuffer = (ADI_DEV_BUFFER*)pArg;
#if defined(ADI_DEV_DEBUG)
	if (ValidatePDDHandle(DeviceHandle) != ADI_DEV_RESULT_SUCCESS) {  	// verify the DM handle
		return;
	}
#endif
	// Pass the callback along to the Device Manager Callback
	(pAD7674->DMCallback)(pAD7674->DMHandle,Event,pArg);
}

/*********************************************************************
	Function:		CalcSamplingParameters
	Description:	For a given fsclk and requested sample frequency (both in Hz),
	                the routine calculates the optimal SPORTx_RCLKDIV & SPORTx_fs1div
					values and adjusts the sampling rate accordingly.
					The formulae are such as to use integer arithmetic only.
*********************************************************************/
#define ten2powr6 1000000

static void CalcSamplingParameters(ADI_AD7674 *pAD7674, u32 *fsample, u32 *pclk_div, u32 *fs1_div, u32 *ppi_delay)
{
     u32 fsamp = *fsample; // for convenience
	 u32 fsclk = pAD7674->fsclk;

	 // min time we need to allow for the busy signal
	 u32 tbusy = ppi_delay_max[pAD7674->cmode];  // nanosec

	 // PPI_CLK divider
	 *pclk_div = fsclk/pAD7674->fpclk;

	 // adjust PPI_CLK frequency
	 pAD7674->fpclk = fsclk/(*pclk_div);

	 // PPI_DELAY value - covers busy period
	 *ppi_delay = tbusy*(pAD7674->fpclk/ten2powr6)/1000+1;

	 // Frame Sync divider
	 *fs1_div = pAD7674->fpclk/fsamp;

	 // revised sample rate
	 *fsample = pAD7674->fpclk/(*fs1_div);
}

/*********************************************************************
	Function:		ClearFlagPins
	Description:	Clears the GPIO pins on the given port as per the
					mask argument.
*********************************************************************/
static void ClearFlagPins(u32 port, u16 pin_mask)
{
	switch (port)
	{
		// CASE (PORT F)
		case ADI_AD7674_PINS_PORT_F:
			*pADI_PORTFIO_CLEAR = pin_mask;
			break;

		// CASE (PORT G)
		case ADI_AD7674_PINS_PORT_G:
			*pADI_PORTGIO_CLEAR = pin_mask;
			break;

		// CASE (PORT H)
		case ADI_AD7674_PINS_PORT_H:
			*pADI_PORTHIO_CLEAR = pin_mask;
			break;
	}
}

/*********************************************************************
	Function:		SetFlagPins
	Description:	Sets the GPIO pins on the given port as per the
					mask argument.
*********************************************************************/
static void SetFlagPins(u32 port, u16 pin_mask)
{
	switch (port)
	{
		// CASE (PORT F)
		case ADI_AD7674_PINS_PORT_F:
			*pADI_PORTFIO_SET = pin_mask;
			break;

		// CASE (PORT G)
		case ADI_AD7674_PINS_PORT_G:
			*pADI_PORTGIO_SET = pin_mask;
			break;

		// CASE (PORT H)
		case ADI_AD7674_PINS_PORT_H:
			*pADI_PORTHIO_SET = pin_mask;
			break;
	}
}

/*********************************************************************
	Function:		ConfigureFlagPins
	Description:	Clears the relevant PORTx_FER bits to acquire the
					pins for GPIO use, and to set the direction to output.
*********************************************************************/
static void ConfigureFlagPins(ADI_DEV_PDD_HANDLE DeviceHandle)
{
	// Assign pointer to Device Instance
	ADI_AD7674 *pAD7674 = (ADI_AD7674 *)DeviceHandle;

	// and determine the port required
	u16 port = pAD7674->pins.PortIdent;

	// Set the mask for the required pins
	//u16 pin_mask =  ( 1<<pAD7674->PPI_CLK_TIMER_pin );
	u16 pin_mask =  0;
	u16 ipin,*pin = &pAD7674->pins.PPI_CLK_TIMER_pin;
	for (ipin=0;ipin<ADI_AD7674_NUM_PINS;ipin++)
		pin_mask |= ( 1<<pin[ipin] );

	// Save original values
	pAD7674->portx_fer = pADI_PORTx_FER[2*port];
	pAD7674->port_mux = *pPORT_MUX;

	// Clear pins to enable GPIO use

	pADI_PORTx_FER[2*port] &= ~pin_mask;
#if (__SILICON_REVISION__==0xffff) || (__SILICON_REVISION__<0x0001)
	ssync();
	pADI_PORTx_FER[2*port] &= ~pin_mask;
	ssync();
#endif

	// And set direction to output
	switch (port)
	{
		// CASE (PORT F)
		case ADI_AD7674_PINS_PORT_F:
			*pADI_PORTFIO_DIR |= pin_mask;
			break;

		// CASE (PORT G)
		case ADI_AD7674_PINS_PORT_G:
			*pADI_PORTGIO_DIR |= pin_mask;
			break;

		// CASE (PORT H)
		case ADI_AD7674_PINS_PORT_H:
			*pADI_PORTHIO_DIR |= pin_mask;
			break;
	}

	// Set Conversion mode
	pin_mask = (1<<pAD7674->pins.WARP_pin) | (1<<pAD7674->pins.IMPULSE_pin);
	// clear required flag pins
	ClearFlagPins(port,pin_mask);

	// and set where required
	u16 pin_value_mask = 0;

	switch(pAD7674->cmode) {
		case ADI_AD7674_NORMAL_MODE:
			break;
		case ADI_AD7674_IMPULSE_MODE:
			pin_value_mask = ( 1<<pAD7674->pins.IMPULSE_pin );
			SetFlagPins(port,pin_value_mask);
			break;
		case ADI_AD7674_WARP_MODE:
			pin_value_mask = ( 1<<pAD7674->pins.WARP_pin );
			SetFlagPins(port,pin_value_mask);
			break;
	}

	// Assert /CS pin
	ClearFlagPins(port,( 1<<pAD7674->pins.CS_pin ));

	// Configure PPI_CLK pin
	ConfigurePPICLK(DeviceHandle);
}

/*********************************************************************
	Function:		ResetFlagPins
	Description:	Clears the relevant PORTx_FER bits to acquire the
					pins for GPIO use, and to set the direction to output.
*********************************************************************/
static void ResetFlagPins(ADI_DEV_PDD_HANDLE DeviceHandle)
{
	// Assign pointer to Device Instance
	ADI_AD7674 *pAD7674 = (ADI_AD7674 *)DeviceHandle;

	// and determine the port required
	u16 port = pAD7674->pins.PortIdent;

	// Set the mask for the required pins
	//u16 pin_mask =  ( 1<<pAD7674->PPI_CLK_TIMER_pin );
	u16 pin_mask =  0;
	u16 ipin,*pin = &pAD7674->pins.PPI_CLK_TIMER_pin;
	for (ipin=0;ipin<ADI_AD7674_NUM_PINS;ipin++)
		pin_mask |= ( 1<<pin[ipin] );

	// Deassert /CS pin
	SetFlagPins(port,( 1<<pAD7674->pins.CS_pin ));

	pADI_PORTx_FER[2*port] = pAD7674->portx_fer;
#if (__SILICON_REVISION__==0xffff) || (__SILICON_REVISION__<0x0001)
	ssync();
	pADI_PORTx_FER[2*port] = pAD7674->portx_fer;
	ssync();
#endif
	 *pPORT_MUX = pAD7674->port_mux;
#if (__SILICON_REVISION__==0xffff) || (__SILICON_REVISION__<0x0001)
	ssync();
	 *pPORT_MUX = pAD7674->port_mux;
	ssync();
#endif

	// And set direction to output
	switch (port)
	{
		// CASE (PORT F)
		case ADI_AD7674_PINS_PORT_F:
			*pADI_PORTFIO_DIR &= ~pin_mask;
			break;

		// CASE (PORT G)
		case ADI_AD7674_PINS_PORT_G:
			*pADI_PORTGIO_DIR &= ~pin_mask;
			break;

		// CASE (PORT H)
		case ADI_AD7674_PINS_PORT_H:
			*pADI_PORTHIO_DIR &= ~pin_mask;
			break;
	}

	// Reset PPI_CLK pin
	ResetPPICLK(DeviceHandle);
	// Restore original values

	// Clear Conversion mode
	pin_mask = (1<<pAD7674->pins.WARP_pin) | (1<<pAD7674->pins.IMPULSE_pin);
	// clear required flag pins
	ClearFlagPins(port,pin_mask);

	// and set where required
	u16 pin_value_mask = 0;

	switch(pAD7674->cmode) {
		case ADI_AD7674_NORMAL_MODE:
			break;
		case ADI_AD7674_IMPULSE_MODE:
			pin_value_mask = ( 1<<pAD7674->pins.IMPULSE_pin );
			ClearFlagPins(port,pin_value_mask);
			break;
		case ADI_AD7674_WARP_MODE:
			pin_value_mask = ( 1<<pAD7674->pins.WARP_pin );
			ClearFlagPins(port,pin_value_mask);
			break;
	}
}


/*********************************************************************
		TIMER CONFIGURATION
*********************************************************************/

struct timer_regs {
	volatile u16 *pConfig;
	volatile u32 *pCounter;
	volatile u32 *pPeriod;
	volatile u32 *pWidth;
};

static struct timer_regs timers[8] = {
	{ (volatile u16 *)TIMER0_CONFIG, (volatile u32 *)TIMER0_COUNTER, (volatile u32 *)TIMER0_PERIOD, (volatile u32 *)TIMER0_WIDTH},
	{ (volatile u16 *)TIMER1_CONFIG, (volatile u32 *)TIMER1_COUNTER, (volatile u32 *)TIMER1_PERIOD, (volatile u32 *)TIMER1_WIDTH},
	{ (volatile u16 *)TIMER2_CONFIG, (volatile u32 *)TIMER2_COUNTER, (volatile u32 *)TIMER2_PERIOD, (volatile u32 *)TIMER2_WIDTH},
	{ (volatile u16 *)TIMER3_CONFIG, (volatile u32 *)TIMER3_COUNTER, (volatile u32 *)TIMER3_PERIOD, (volatile u32 *)TIMER3_WIDTH},
	{ (volatile u16 *)TIMER4_CONFIG, (volatile u32 *)TIMER4_COUNTER, (volatile u32 *)TIMER4_PERIOD, (volatile u32 *)TIMER4_WIDTH},
	{ (volatile u16 *)TIMER5_CONFIG, (volatile u32 *)TIMER5_COUNTER, (volatile u32 *)TIMER5_PERIOD, (volatile u32 *)TIMER5_WIDTH},
	{ (volatile u16 *)TIMER6_CONFIG, (volatile u32 *)TIMER6_COUNTER, (volatile u32 *)TIMER6_PERIOD, (volatile u32 *)TIMER6_WIDTH},
	{ (volatile u16 *)TIMER7_CONFIG, (volatile u32 *)TIMER7_COUNTER, (volatile u32 *)TIMER7_PERIOD, (volatile u32 *)TIMER7_WIDTH},
};

#define TIMER_NUM(F)  ( 9 - (F) )
/*********************************************************************
	Function:		ConfigureTimers
	Description:	Set the timers for PPI_CLK, PPI_FS1, PPI_FS2 and set PPI_DELAY register
*********************************************************************/
static void ConfigureTimers(ADI_AD7674 *pAD7674, u32 pclk_div, u32 fs1_div, u32 ppi_delay)
{
	// Assign pointer to Device Instance
	u32 timerID = TIMER_NUM(pAD7674->pins.PPI_CLK_TIMER_pin);
	struct timer_regs *ppi_clk_timer = &timers[timerID];

	// Configure Timer to generate PPI_CLK signal
	*ppi_clk_timer->pConfig = PWM_OUT | PULSE_HI | PERIOD_CNT | EMU_RUN;
	*ppi_clk_timer->pPeriod = pclk_div;
	*ppi_clk_timer->pWidth = pclk_div>>1;

	// Configure PPI_FS1 Timer to generate CNVSTb signal - uses PPI_CLK as basis
	ADI_PPI_FS_TMR ppi_fs_data = {0};  // initialize - sets all fields to zero
	ppi_fs_data.period = fs1_div;
	ppi_fs_data.width = 1;
	ppi_fs_data.pulse_hi = 0;
	ppi_fs_data.emu_run = 1;
	adi_dev_Control(pAD7674->ppiHandle, ADI_PPI_CMD_SET_TIMER_FRAME_SYNC_1, (void*)&ppi_fs_data);

	// Configure PPI_FS2 Timer to generate A0 signal.
	// This needs to switch from one PPI_CLK cycle after the busy period.
	// the user choses from a continuous A0 signal to one with pulse of duration PPI_DELAY + 1
	if (pAD7674->a0_continuous)
	{
		ppi_fs_data.period = 2;   // same frequency as PPI_FS1
		ppi_fs_data.width = 1; // but pulse width equal to busy time + 1 cycle
	}
	else
	{
		ppi_fs_data.period = fs1_div;   // same frequency as PPI_FS1
		ppi_fs_data.width = ppi_delay + 1; // but pulse width equal to ppi_delay + 1 cycle
	}

	// The user choses whether to puls high or low depending on whether they want
	// MSB or LSB first.
	ppi_fs_data.pulse_hi = (pAD7674->MSBFirst ? 1 : 0);
	// and assume  we want clock to run when halded in emulator mode
	ppi_fs_data.emu_run = 1;
	// now tell the PPI driver to configure accordingly
	adi_dev_Control(pAD7674->ppiHandle, ADI_PPI_CMD_SET_TIMER_FRAME_SYNC_2, (void*)&ppi_fs_data);

	// and set the PPI_DELAY register
	adi_dev_Control(pAD7674->ppiHandle,ADI_PPI_CMD_SET_DELAY_COUNT_REG,(void*)ppi_delay);
}

/*********************************************************************
	Function:		EnablePPI_CLK
	Description:	Enables/Disables the timer for PPI_CLK
*********************************************************************/
static void EnablePPI_CLK(ADI_DEV_PDD_HANDLE DeviceHandle, u32 flag)
{
	// Assign pointer to Device Instance
	ADI_AD7674 *pAD7674 = (ADI_AD7674 *)DeviceHandle;
	u32 timerID = TIMER_NUM(pAD7674->pins.PPI_CLK_TIMER_pin);

	if (flag) {
		*pTIMER_ENABLE |= (1<<timerID);
	} else {
		*pTIMER_DISABLE |= (1<<timerID);
	}
}

/*********************************************************************
	Function:		ConfigurePPICLK
	Description:	Configures the required GPIO pins for PPI_CLK input
*********************************************************************/
static void ConfigurePPICLK(ADI_DEV_PDD_HANDLE DeviceHandle)
{
	// Assign pointer to Device Instance
	ADI_AD7674 *pAD7674 = (ADI_AD7674 *)DeviceHandle;

	// and determine the port required
	u16 port = pAD7674->pins.PortIdent;

	// Set the required PF pin to TIMER use & output
	//u16 pin_mask =  ( 1<<pAD7674->pins.PPI_CLK_TIMER_pin );
	//*pADI_PORTFIO_DIR &= ~pin_mask;
	// Set the PPI_CLK Timer pin to peripheral mode in the PORTF_FER register
	// (IMPORTANT, this is the opposite sense to what's been done in ConfigureFlagPins:
	// here, we set the timer pin for timer use not GPIO).
	u16 portf_fer = *pPORTF_FER;
	portf_fer |= (1<<pAD7674->pins.PPI_CLK_TIMER_pin);
	*pPORTF_FER = portf_fer;
#if (__SILICON_REVISION__==0xffff) || (__SILICON_REVISION__<0x0001)
	ssync();
	*pPORTF_FER = portf_fer;
	ssync();
#endif

	// Set the PORT_MUX register as required
	u16 port_mux = *pPORT_MUX;

	switch(pAD7674->pins.PPI_CLK_TIMER_pin)
	{
		case 2:
		case 3:
			port_mux |= PFTE_TIMER;
			break;
		case 4:
			port_mux &= ~(PFS6E); // Clear to set TMR5
			break;
		case 5:
			port_mux &= ~(PFS5E); // Clear to set TMR4
			break;
		case 6:
			port_mux &= ~(PFS5E); // Clear to set TMR3
			break;
		case 7:
			port_mux &= ~(PFFE); // Clear to set TMR2
			break;
		default:
			break;
	}
	if (pAD7674->pins.PPI_CLK_TIMER_pin!=8)
	{
	*pPORT_MUX = port_mux;
#if (__SILICON_REVISION__==0xffff) || (__SILICON_REVISION__<0x0001)
	ssync();
	*pPORT_MUX = port_mux;
	ssync();
#endif
	}
}
/*********************************************************************
	Function:		ResetPPICLK
	Description:	Configures the required GPIO pins for PPI_CLK input
*********************************************************************/
static void ResetPPICLK(ADI_DEV_PDD_HANDLE DeviceHandle)
{
	// Assign pointer to Device Instance
	ADI_AD7674 *pAD7674 = (ADI_AD7674 *)DeviceHandle;

	// and determine the port required
	u16 port = pAD7674->pins.PortIdent;

	// Set the required PF pin to TIMER use & output
	u16 pin_mask =  ( 1<<pAD7674->pins.PPI_CLK_TIMER_pin );
	*pADI_PORTFIO_DIR |= pin_mask;

	// Set the PPI_CLK Timer pin to peripheral mode in the PORTF_FER register
	u16 portf_fer = *pPORTF_FER;
	portf_fer &= ~(1<<pAD7674->pins.PPI_CLK_TIMER_pin);
	*pPORTF_FER = portf_fer;
#if (__SILICON_REVISION__==0xffff) || (__SILICON_REVISION__<0x0001)
	ssync();
	*pPORTF_FER = portf_fer;
	ssync();
#endif

	// Reset the PORT_MUX register as required
	u16 port_mux = *pPORT_MUX;

	switch(pAD7674->pins.PPI_CLK_TIMER_pin)
	{
		case 2:
		case 3:
			port_mux &= ~PFTE_TIMER;
			break;
		case 4:
			port_mux |= (PFS6E); // Set to clear TMR5
			break;
		case 5:
			port_mux |= (PFS5E); // Set to clear  TMR4
			break;
		case 6:
			port_mux |= (PFS5E); // Set to clear TMR3
			break;
		case 7:
			port_mux |= (PFFE); // Set to clear  TMR2
			break;
		default:
			break;
	}
	if (pAD7674->pins.PPI_CLK_TIMER_pin!=8)
	{
	*pPORT_MUX = port_mux;
#if (__SILICON_REVISION__==0xffff) || (__SILICON_REVISION__<0x0001)
	ssync();
	*pPORT_MUX = port_mux;
	ssync();
#endif
	}
}

static void SetConversionMode(ADI_DEV_PDD_HANDLE DeviceHandle, u32 mode)
{
	// Assign pointer to Device Instance
	ADI_AD7674 *pAD7674 = (ADI_AD7674 *)DeviceHandle;

	// Assign value in instance data
	pAD7674->cmode = mode;

	// Assign nominal sampling rate
	switch(mode) {
		case ADI_AD7674_NORMAL_MODE:
			pAD7674->fsample = NORMAL_MAX_THROUGHPUT;
			break;
		case ADI_AD7674_IMPULSE_MODE:
			pAD7674->fsample = IMPULSE_MAX_THROUGHPUT;
			break;
		case ADI_AD7674_WARP_MODE:
			pAD7674->fsample = WARP_MAX_THROUGHPUT;
			break;
	}

	// Calculate and true sampling rate and configure the timers
	SetSampleRate(DeviceHandle, &pAD7674->fsample);

}

#if defined(ADI_DEV_DEBUG)

/*********************************************************************
	Function:		ValidatePDDHandle
	Description:	Validates a PDD handle
*********************************************************************/

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE DeviceHandle) {
	unsigned int i;
	for (i = 0; i < ADI_AD7674_NUM_DEVICES; i++) {
		if (DeviceHandle == (ADI_DEV_PDD_HANDLE)&Device[i]) {
			return (ADI_DEV_RESULT_SUCCESS);
		}
	}
	return (ADI_DEV_RESULT_BAD_PDD_HANDLE);
	}

#endif


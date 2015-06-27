/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_ad7266.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			This is the driver source code for the AD7266 ADC peripheral. It is layered
			on top of the SPORT device driver, which is configured for the specific use of
			the AD7266 peripheral.
			
*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/

#define ADI_AD7266_MODULE

#include <services/services.h>		// system service includes

#ifndef __ADSP_BRAEMAR__
#error "AD7266 driver is not available for the current processor"
#endif

#include <drivers/adi_dev.h>			// device manager includes
#include <drivers/sport/adi_sport.h>	// sport driver includes
#include <drivers/adc/adi_ad7266.h> 	// AD7266 driver includes

/*********************************************************************

Enumerations and defines

*********************************************************************/

#define ADI_AD7266_NUM_DEVICES	(sizeof(Device)/sizeof(ADI_AD7266))	// number of AD7266 devices in the system

// Pin Definition array index Macros for clarity
#define A0_PIN 0
#define A1_PIN 1
#define A2_PIN 2

// For clarity
#define ENABLE 1
#define DISABLE 0

// Structure to contain data for an instance of the AD7476 device driver
#pragma pack(4)
typedef struct {
	ADI_DEV_DEVICE_HANDLE		DMHandle;					// Handle of Device Manager instance
	ADI_DEV_PDD_HANDLE			sportHandle;				// Handle to the underlying SPORT device driver
	u32							InUseFlag;					// flag to indicate whether an instance is in use or not
	ADI_DCB_CALLBACK_FN			DMCallback;					// the callback function supplied by the Device Manager
	ADI_FLAG_ID					ChannelSelectPins[3];		// Pins used to select the input channels
	u32							ChannelSelectValue;			// Value to be assigned to flag pins
	u32							DataFlowEnabled;			// Flag to indicate whether data flow is enabled
#if defined (__ADSP_BRAEMAR__)
	u32							portGPIOSelect[3];			// array of directives to enable Channel select pins for GPIO
#endif
	u32							UseOptimalSCLK;				// determines whether the SCLK freq can be varied to gain optimal throughput
	u16							sport_rcr2;					// temporary measure to cache SPORT_RCR2 register
	u32							ncclk;						// number of complete CCLK cycles per RSCLK cycle
	u32							fsclk;						// SCLK frequency (MHz)
	u16							portx_fer;					// Original PORTx_FER settings (whatever port we have chosen - see pins structure)
	u16							port_mux;					// Original PORT_MUX settings
} ADI_AD7266;
#pragma pack()
	
// The data structure instances for each device (SPORT0, SPORT1)
static ADI_AD7266 Device[2];

// The prototpe functions for the AD7266 device driver

static u32 adi_pdd_Open(					// Open the AD7266 device
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

static u32 adi_pdd_Close(		// Closes the AD7266 device
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

// the callback function passed to the SPORT driver
static void sportCallbackFunction(void* DeviceHandle, u32 Event, void* pArg);
	

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

#if 0
static volatile u16 *pADI_PORTFIO_CLEAR = (volatile u16 *)(0xFFC00704);
static volatile u16 *pADI_PORTFIO_SET = (volatile u16 *)(0xFFC00708);
static volatile u16 *pADI_PORTFIO_DIR = (volatile u16 *)(0xFFC00730);

static volatile u16 *pADI_PORTGIO_CLEAR = (volatile u16 *)(0xFFC01504);
static volatile u16 *pADI_PORTGIO_SET = (volatile u16 *)(0xFFC01508);
static volatile u16 *pADI_PORTGIO_DIR = (volatile u16 *)(0xFFC01530);

static volatile u16 *pADI_PORTHIO_CLEAR = (volatile u16 *)(0xFFC01704);
static volatile u16 *pADI_PORTHIO_SET = (volatile u16 *)(0xFFC01708);
static volatile u16 *pADI_PORTHIO_DIR = (volatile u16 *)(0xFFC01730);
#endif
//

///////////////////////////////////////////////////////////////////////
// static functions
static void CalcSamplingParameters(u32 fsclk, u32 *fsample, u32 *rclkdiv, u32 *rfsdiv, u32 opt_sclk);
static void ConfigureChannelSelection(ADI_DEV_PDD_HANDLE DeviceHandle, u16 selection);
static void ConfigureFlagPins(ADI_DEV_PDD_HANDLE DeviceHandle, u32 enable);
static u32 partial_power_down(ADI_AD7266 *pAD7266);
static u32 power_down(ADI_AD7266 *pAD7266);
static u32 power_up(ADI_AD7266 *pAD7266);

// the sampling word length minus 0ne
#define SLEN_VALUE 13

/*********************************************************************

Entry point for device manager

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT ADI_AD7266_EntryPoint = {
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
*	Description:	Opens the AD7266 device for use
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
	ADI_AD7266 *pAD7266;		// pointer to the AD7266 device we're working on
	void *pExitCriticalArg;		// exit critical region parameter
	
	// Check for a valid device number and that we're only doing input
#ifdef ADI_DEV_DEBUG
	if (DeviceNumber >= ADI_AD7266_NUM_DEVICES) return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
	if (Direction != ADI_DEV_DIRECTION_INBOUND) return (ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED);
#endif

	// assign the pointer to the device instance
	pAD7266 = &Device[DeviceNumber];
	// and store the Device Manager handle 
	pAD7266->DMHandle = DMHandle;
	// and callback function
	pAD7266->DMCallback = DMCallback;
	// assign in-use flag
	pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
	if (pAD7266->InUseFlag == FALSE) {
		pAD7266->InUseFlag = TRUE;
		Result = ADI_DEV_RESULT_SUCCESS;
	}
	adi_int_ExitCriticalRegion(pExitCriticalArg);
	// assign optimal SCLK flag (defaults to no-variation)
	pAD7266->UseOptimalSCLK = 0;

#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Open the SPORT driver 
	Result = adi_dev_Open(
					ManagerHandle,			// device manager handle
					&ADISPORTEntryPoint,	// SPORT Entry point
					DeviceNumber,			// device number
					pAD7266,				// client handle - passed to internal callback function
					&pAD7266->sportHandle,	// pointer to DM handle (for SPORT driver) location 
					Direction,				// data direction as input
					DMAHandle,				// handle to the DMA manager
					DCBHandle,				// handle to the callback manager
					sportCallbackFunction	// internal callback function 
	);
	// return with appropriate code if SPORT driver fails to open
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Configure the SPORT for the appropriate values for the AD7266 device
	ADI_SPORT_RX_CONFIG1_REG sport_rcr1;
	ADI_SPORT_RX_CONFIG2_REG sport_rcr2;

	// Configure the SPORT driver to alternately sample from DoutA and DoutB
	// by using a frame sync signal that asserts (drive low) the /cs (chip select pin) 
	// at the required interval (set up via a call to the Control function).
	// The AD7266 device requires a data length of 14 bits and generates data 
	// MSB first.

	// Configure the SPORTx_RCR1 register
	sport_rcr1.irclk = 1;	// internal rx clock
	sport_rcr1.rdtype = 0;	// zero fill
	sport_rcr1.rlsbit = 0;	// MSB first
	sport_rcr1.irfs = 1;	// internal RX frame sync (RFS) select
	sport_rcr1.rfsr = 1;	// requires RFS for every data word
	sport_rcr1.lrfs = 1;	// Active low RFS 
	sport_rcr1.larfs = 1;	// late RFS
	sport_rcr1.rckfe = 1;	// Drive convertor on falling edge of RSCLK, 
							// and sample data (into SPORT) on rising edge

	// Configure the SPORTx_RCR2 register
	sport_rcr2.slen = 13;	// word length is 14 bits (two leading zeros)
	sport_rcr2.rxse = 0;	// Enable secondary (selectable via Control ?)
	sport_rcr2.rsfse = 0;	// Normal mode
	sport_rcr2.rrfst = 0;	// default left stereo channel first

	// cache SPORT_RC2 contents
	pAD7266->sport_rcr2 = *(u16*)&sport_rcr2;
 	
	// and pass the values to the SPORT driver
	ADI_DEV_CMD_VALUE_PAIR SPORT_config[] = {
		{ ADI_SPORT_CMD_SET_RCR1, 		(void*)(*(u16*)&sport_rcr1)	},	
		{ ADI_SPORT_CMD_SET_RCR2, 		(void*)(*(u16*)&sport_rcr2)	},	
		// SPORTx TX not required. So Clear to ensure it's not used.
		{ ADI_SPORT_CMD_SET_TCR1,		(void*)0					},	
		{ ADI_SPORT_CMD_SET_TCR2,		(void*)0					},	
		{ ADI_DEV_CMD_END, 0 }
	};
	Result = adi_dev_Control( pAD7266->sportHandle, ADI_DEV_CMD_TABLE, (void*)SPORT_config );
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Default Flag ID's for Channel select pins
	pAD7266->ChannelSelectPins[A0_PIN] = ADI_FLAG_PG4;
	pAD7266->ChannelSelectPins[A1_PIN] = ADI_FLAG_PG5;
	pAD7266->ChannelSelectPins[A2_PIN] = ADI_FLAG_PG6;

#if defined (__ADSP_BRAEMAR__)
	// Assign Default Directives for enabling the pins for GPIO
	pAD7266->portGPIOSelect[A0_PIN] = ADI_PORTS_PIN_ENUM_VALUE(ADI_FLAG_PG4,0,0,0);
	pAD7266->portGPIOSelect[A1_PIN] = ADI_PORTS_PIN_ENUM_VALUE(ADI_FLAG_PG5,0,0,0);
	pAD7266->portGPIOSelect[A2_PIN] = ADI_PORTS_PIN_ENUM_VALUE(ADI_FLAG_PG6,0,0,0);
#endif

	// save the physical device handle in the client supplied location
	*pPDDHandle = (ADI_DEV_PDD_HANDLE *)pAD7266;

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
	ADI_DEV_PDD_HANDLE DeviceHandle	// PDD handle
) {
	u32 Result;		// return value
	ADI_AD7266 *pAD7266 = (ADI_AD7266 *)DeviceHandle;

	// check for errors if required
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(DeviceHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
	// Disable ports and DMA etc
	if (pAD7266->DataFlowEnabled)
		adi_pdd_Control(DeviceHandle, ADI_DEV_CMD_SET_DATAFLOW, DISABLE);
		
	// close SPORT driver
	Result = adi_dev_Close(pAD7266->sportHandle);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// mark the device as closed
	pAD7266->InUseFlag = FALSE;
	
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
	ADI_AD7266 *pAD7266 = (ADI_AD7266 *)DeviceHandle;
	Result = adi_dev_Read(pAD7266->sportHandle,BufferType,pBuffer);

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
*	Description:	Configures the SPORT device
*
*********************************************************************/

static u32 adi_pdd_Control(		// Sets or senses a device specific parameter
	ADI_DEV_PDD_HANDLE 	DeviceHandle,	// PDD handle
	u32 				Command,	// command ID
	void 				*Value		// command specific value
) {
	
	ADI_AD7266 				*pAD7266;		// pointer to the device we're working on
	u32 					tmp;			// temporary storage
	u32 					Result;			// return value
	u32						u32Value;		// u32 type to avoid casts/warnings etc.
	u16						u16Value;		// u16 type to avoid casts/warnings etc.
	
	// avoid casts
	pAD7266 = (ADI_AD7266 *)DeviceHandle; // Pointer to AD7266 device driver instance
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
			// no, we do support it at this level (it is delegated down to SPORT driver
			*((u32 *)Value) = FALSE;
			break;

		// CASE (Set the sample rate of the AD7266 device)
		case (ADI_AD7266_CMD_SET_SAMPLE_RATE):
			{
				u32 fcclk,fsclk,fvco,rclkdiv,rfsdiv;
				// note that for this option the user passes in the address containing
				// the requested sampling rate value, which is replaced by the achievable
				// value.
				u32 fsample = *(u32*)Value;
				
				// Obtain the System Clock freqency, fsclk, from  the
				// power management module of the System Services library. This is returned
				// as a MHz value. If the Power management module is not initialized, the 
				// reset value is used.
				if (adi_pwr_GetFreq(&fcclk,&fsclk,&fvco)!=ADI_PWR_RESULT_SUCCESS)
					fsclk = 50000000;

				// Calculate the optimal divider values and update the sampling rate
				CalcSamplingParameters(fsclk, &fsample, &rclkdiv, &rfsdiv, pAD7266->UseOptimalSCLK);
				*(u32*)Value = fsample;

				// calculate number of CCLK cycles in a RSCLK cycle
				u32 ssel = *pPLL_DIV & 0x000F;
				pAD7266->ncclk = 2*(rclkdiv+1)*ssel;

				// assign SPORT clock divider registers
				ADI_DEV_CMD_VALUE_PAIR SPORT_config[] = {
					{ ADI_SPORT_CMD_SET_RCLKDIV, (void*)rclkdiv },	// SPORTx_RCLKDIV
					{ ADI_SPORT_CMD_SET_RFSDIV, (void*)rfsdiv },	// SPORTx_RFSDIV
					{ ADI_DEV_CMD_END, (void*)0 }
				};
				Result = adi_dev_Control( pAD7266->sportHandle, ADI_DEV_CMD_TABLE, (void*)SPORT_config );
#ifdef ADI_DEV_DEBUG
				if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
			}
			break;

		// CASE (Enable the SCLK freq to be varied to gain optimal throughput)
		case (ADI_AD7266_CMD_USE_OPTIMAL_SCLK):
			pAD7266->UseOptimalSCLK = u32Value;
			break;

		// CASE (Enable data to be captured from DoutB on the AD7266)
		case (ADI_AD7266_CMD_SET_DUAL_MODE):

			// Set Secondary enable (underlying SPORT driver will handle PF set up on flow enable)
			Result = adi_dev_Control(pAD7266->sportHandle,ADI_SPORT_CMD_SET_RX_SECONDARY_ENABLE,(void*)TRUE);
#ifdef ADI_DEV_DEBUG
			if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
			break;

		// CASE (Set Channel select flag pins)
		case ADI_AD7266_CMD_SET_CHANNEL_SELECT_PINS:
			{
			ADI_FLAG_ID *pin_def = (ADI_FLAG_ID *)Value;
#ifdef ADI_DEV_DEBUG
			// Check pin numbers are in valid range
			if ( ADI_FLAG_GET_BIT(pin_def[A0_PIN] ) > 15 || ((ADI_FLAG_GET_OFFSET(pin_def[A0_PIN])&0x00001800)>>11) >2 )
					return ADI_AD7266_RESULT_INVALID_PIN_SELECTION;
			if ( ADI_FLAG_GET_BIT(pin_def[A1_PIN] ) > 15 || ((ADI_FLAG_GET_OFFSET(pin_def[A1_PIN])&0x00001800)>>11) >2)
					return ADI_AD7266_RESULT_INVALID_PIN_SELECTION;
			if ( ADI_FLAG_GET_BIT(pin_def[A2_PIN] ) > 15 || ((ADI_FLAG_GET_OFFSET(pin_def[A2_PIN])&0x00001800)>>11) >2)
					return ADI_AD7266_RESULT_INVALID_PIN_SELECTION;
#endif
			// Form Flag ID's for requested pins
			pAD7266->ChannelSelectPins[A0_PIN] = pin_def[A0_PIN];
			pAD7266->ChannelSelectPins[A1_PIN] = pin_def[A1_PIN];
			pAD7266->ChannelSelectPins[A2_PIN] = pin_def[A2_PIN];
			
#if defined (__ADSP_BRAEMAR__)
			// Assign Directives for enabling the pins for GPIO
			pAD7266->portGPIOSelect[A0_PIN] = ADI_PORTS_PIN_ENUM_VALUE(pin_def[A0_PIN],0,0,0);
			pAD7266->portGPIOSelect[A1_PIN] = ADI_PORTS_PIN_ENUM_VALUE(pin_def[A1_PIN],0,0,0);
			pAD7266->portGPIOSelect[A2_PIN] = ADI_PORTS_PIN_ENUM_VALUE(pin_def[A2_PIN],0,0,0);
#endif
			
			}
			break;

		// CASE (Select channel to read from)
		case ADI_AD7266_CMD_SELECT_CHANNELS:
#ifdef ADI_DEV_DEBUG
			// Check selection value is in valid range
			if ( u16Value > ADI_AD7266_CHANNEL_VA6_VB6)
				return ADI_AD7266_RESULT_INVALID_CHANNEL_SELECTION;
#endif
			// Store channel select value - applied when data flow is enabled
			pAD7266->ChannelSelectValue = u32Value;
			break;

		// CASE (Set mode of operation)
		case (ADI_AD7266_CMD_SET_OPERATION_MODE):
			
#ifdef ADI_DEV_DEBUG
			// Check selection value is in valid range
			if ( u32Value > ADI_AD7266_MODE_NORMAL)
				return ADI_AD7266_RESULT_INVALID_OPERATION_MODE;
#endif
			// IF (powering down the device)
			if (u32Value == ADI_AD7266_MODE_POWERDOWN) {
				
				if (Result = power_down(pAD7266)) break;

			// ELSE IF (partially powering down the device)
			} else if (u32Value == ADI_AD7266_MODE_PARTIAL_POWERDOWN) {

				if (Result = partial_power_down(pAD7266)) break;

			// ELSE IF (powering up the device)
			} else if (u32Value == ADI_AD7266_MODE_NORMAL){
				
				if (Result = power_up(pAD7266)) break;
				
			// ENDIF
			}
			break;
		// CASE (Enable/Disable data flow)
		case ADI_DEV_CMD_SET_DATAFLOW:
			pAD7266->DataFlowEnabled = u32Value;
			if (u32Value) // enable data flow
			{
				// Configure flag pins 
				ConfigureFlagPins(DeviceHandle,ENABLE);
				// Enable SPORT
				Result = adi_dev_Control(pAD7266->sportHandle,Command,Value);
			}
			else  // disable data flow
			{
				// Stop SPORT
				Result = adi_dev_Control(pAD7266->sportHandle,Command,Value);
				// and reset the flag pins
				ConfigureFlagPins(DeviceHandle,DISABLE);
			}
			break;
		
		// DEFAULT 
		default:
		
			// pass anything we don't specifically handle to the SPORT driver
			Result = adi_dev_Control(pAD7266->sportHandle,Command,Value);
			break;
		
	// ENDCASE
	}
		
	// return
	return(Result);
}

#define RSCLK_CYCLES_DELAY 32
#define CALL_OVERHEAD 130

#ifndef ADI_DEV_DEBUG
#pragma optimize_off
#endif
static void pwr_updown_delay(int delay)
{
	asm(	
		"P0 = %0;"
		"LSETUP(pwr_updown_loop_begin,pwr_updown_loop_end) LC0 = P0;"
		"pwr_updown_loop_begin:"
		"pwr_updown_loop_end: nop;"
		 : : "d"(delay)
		 );
}

#ifndef ADI_DEV_DEBUG
#pragma optimize_for_speed
#endif
/*********************************************************************
	Function:		power_down
	Description:	Fully Powers down the converter by switching to 8bit words
*********************************************************************/
static u32 power_down(ADI_AD7266 *pAD7266)
{
	u32 Result; 

	// delay interval is calculated as 'RSCLK_CYCLES_DELAY' * number of Core clock cycles per RSCLK cycle
	// less an overhead value estimated for the cycles between the instruction that initiates the power up 
	// and the zero overhead delay loop.
	int delay = RSCLK_CYCLES_DELAY*pAD7266->ncclk - CALL_OVERHEAD;
	if (delay<0) delay = 0;

	// Suspend SPORT driver
	if (Result = adi_dev_Control( pAD7266->sportHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)FALSE )) 
		return Result;
				
	// Change the length of the words to read to 8 bits by changing SLEN in the SPORT_RCR2 register
	Result = adi_dev_Control(pAD7266->sportHandle,ADI_SPORT_CMD_SET_RX_WORD_LENGTH,(void*)7);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Resume SPORT driver for changes to take effect
	if (Result = adi_dev_Control( pAD7266->sportHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)TRUE )) 
		return Result;
	
	// wait for at least 32 RSCLK cycles to allow time for powerdown to occur
	pwr_updown_delay(delay);
	
	// and Suspend SPORT driver
	if (Result = adi_dev_Control( pAD7266->sportHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)FALSE )) 
		return Result;
		
	return ADI_DEV_RESULT_SUCCESS;
}

/*********************************************************************
	Function:		partial_power_down
	Description:	partially Powers down the converter by switching to 8bit words
*********************************************************************/
static u32 partial_power_down(ADI_AD7266 *pAD7266)
{
	u32 Result; 
	u16 portg_fer = *pPORTG_FER;
	u16 portg_fer_save = portg_fer;
	// delay interval is calculated as 12 RCLK cycles * number of Core clock cycles per RSCLK cycle
	// less an overhead value estimated for the cycles between the instruction that initiates the power up 
	// and the zero overhead delay loop.
	int delay = 14*pAD7266->ncclk - CALL_OVERHEAD;
	if (delay<0) delay = 0;

	// Suspend SPORT driver
	if (Result = adi_dev_Control( pAD7266->sportHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)FALSE )) 
		return Result;

	// Change the length of the words to read to 8 bits by changing SLEN in the SPORT_RCR2 register
	Result = adi_dev_Control(pAD7266->sportHandle,ADI_SPORT_CMD_SET_RX_WORD_LENGTH,(void*)7);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Resume SPORT driver for changes to take effect
	if (Result = adi_dev_Control( pAD7266->sportHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)TRUE )) 
		return Result;
	
	// wait for at least 32 RSCLK cycles to allow time for powerdown to occur
	pwr_updown_delay(delay);
	
	// and Suspend SPORT driver
	if (Result = adi_dev_Control( pAD7266->sportHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)FALSE )) 
		return Result;
		
	return ADI_DEV_RESULT_SUCCESS;
}

/*********************************************************************
	Function:		power_up
	Description:	Powers up the converter by switching back to 14bit words
*********************************************************************/

static u32 power_up(ADI_AD7266 *pAD7266)
{
	u32 Result;
	// delay interval is calculated as 'RSCLK_CYCLES_DELAY' * number of Core clock cycles per RSCLK cycle
	// less an overhead value estimated for the cycles between the instruction that initiates the power up 
	// and the zero overhead delay loop.
	int delay = RSCLK_CYCLES_DELAY*pAD7266->ncclk - CALL_OVERHEAD;
	if (delay<0) delay = 0;

	// Change the length of the words to read back to 14 bits by changing SLEN in the SPORT_RCR2 register
	((ADI_SPORT_RX_CONFIG2_REG*)&pAD7266->sport_rcr2)->slen=SLEN_VALUE;
	Result = adi_dev_Control(pAD7266->sportHandle,ADI_SPORT_CMD_SET_RX_WORD_LENGTH,(void*)SLEN_VALUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Resume SPORT driver for changes to take effect
	if (Result = adi_dev_Control( pAD7266->sportHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)TRUE )) 
		return Result;

	// wait for at least 32 RSCLK cycles to allow time for powerdown to occur
	pwr_updown_delay(delay);

	return ADI_DEV_RESULT_SUCCESS;
}

/*********************************************************************
	Function:		sportCallbackFunction
	Description:	Fields the callback from the SPORT Driver
*********************************************************************/

static void sportCallbackFunction(void* DeviceHandle, u32 Event, void* pArg)
{
	ADI_AD7266 *pAD7266 = (ADI_AD7266 *)DeviceHandle;
#if defined(ADI_DEV_DEBUG)
	if (ValidatePDDHandle(DeviceHandle) != ADI_DEV_RESULT_SUCCESS) {  	// verify the DM handle
		return;
	}
#endif
	// simply pass the callback along to the Device Manager Callback without interpretation
	(pAD7266->DMCallback)(pAD7266->DMHandle,Event,pArg);
}


#if defined(ADI_DEV_DEBUG)

/*********************************************************************
	Function:		ValidatePDDHandle
	Description:	Validates a PDD handle
*********************************************************************/

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE DeviceHandle) {
	unsigned int i;
	for (i = 0; i < ADI_AD7266_NUM_DEVICES; i++) {
		if (DeviceHandle == (ADI_DEV_PDD_HANDLE)&Device[i]) {
			return (ADI_DEV_RESULT_SUCCESS);
		}
	}
	return (ADI_DEV_RESULT_BAD_PDD_HANDLE);
	}

#endif

/*********************************************************************
	Function:		CalcSamplingParameters
	Description:	For a given fsclk and requested sample frequency (both in Hz),
	                the routine calculates the optimal SPORTx_RCLKDIV & SPORTx_RFSDIV
					values and adjusts the sampling rate accordingly.
					The formulae are such as to use integer arithmetic only.
*********************************************************************/
#define ten2powr8 100000000
#define tepsmin  7   // min quiet period, teps = t2 + t8 + tquiet ~70ns
//#define tepsmin  10   // min quiet period, teps = t2 + t8 + tquiet ~70ns

static void CalcSamplingParameters(u32 fsclk, u32 *fsample, u32 *rclkdiv, u32 *rfsdiv, u32 opt_sclk)
{
     u32 fsamp = *fsample; // for convenience
	 u32 fsclk_opt;

	 // calculate the SPORTx_RFSDIV value from the requested sampling rate
	 // and taking into account the minimum total 'quiet' time between last 
	 // sampled bit and the next falling edge of the /CS signal.
     u32 nrfs = (tepsmin*fsamp+SLEN_VALUE*ten2powr8)/(ten2powr8 - tepsmin*fsamp) + 1;

	 // Now we estimate the SPORTx_RCLKDIV value 
     u32 nrclk = fsclk/(2*fsamp*(nrfs+1));
	 // ensuring its positive
     if (nrclk>0) 
     	nrclk--;

	 // If the optional optimal SCLK frequency option has been requested, adjust the
	 // SCLK frequency to maximize throughput
	 if (opt_sclk)
	 {
		 u32 fcclk, fvco;
		 // calculate optimal SCLK frequency for exactly the requested sampling rate
		 // (value in MHz)
		 fsclk_opt = 2*fsamp*(nrfs+1)*(nrclk+1);
		 // and suggest this value to the Power Management module
		 adi_pwr_SetFreq(0,fsclk_opt,ADI_PWR_DF_NONE);
		 // Now find out what it actually set 
		 adi_pwr_GetFreq(&fcclk,&fsclk_opt,&fvco);
	 }
	 else
		 // otherwise use as is
		 fsclk_opt = fsclk;

	 // ensure the actual sampling rate is no greater than that
	 // requested.
     if (fsclk_opt > fsamp*2*(nrfs+1)*(nrclk+1))
        nrclk++;
        
     // finally, assign return values
     *fsample = fsclk_opt/(2*(nrfs+1)*(nrclk+1));
     *rfsdiv = nrfs;
     *rclkdiv = nrclk;
}

/*********************************************************************
	Function:		ConfigureFlagPins
	Description:	Clears the relevant PORTx_FER bits to acquire the
					pins for GPIO use, and to set the direction to output.
*********************************************************************/

static void ConfigureFlagPins(ADI_DEV_PDD_HANDLE DeviceHandle, u32 Enable)
{
	// Assign pointer to Device Instance
	ADI_AD7266 *pAD7266 = (ADI_AD7266 *)DeviceHandle;
	u32 i, val;

	if (Enable) 
	{
	// Open the flag pins
		adi_flag_SetDirection(
						pAD7266->ChannelSelectPins[A0_PIN], 				// A0
						ADI_FLAG_DIRECTION_OUTPUT
						);
		adi_flag_SetDirection(
						pAD7266->ChannelSelectPins[A1_PIN], 				// A0
						ADI_FLAG_DIRECTION_OUTPUT
						);
		adi_flag_SetDirection(
						pAD7266->ChannelSelectPins[A2_PIN], 				// A0
						ADI_FLAG_DIRECTION_OUTPUT
						);
		
#if defined (__ADSP_BRAEMAR__)
		// Enable pins for GPIO
		adi_ports_EnableGPIO(pAD7266->portGPIOSelect, 3, ENABLE);
#endif

		// And set the value
		val = pAD7266->ChannelSelectValue;
		for (i=0; i<3; i++, val>>=1) {
			if (val&0x1) adi_flag_Set(pAD7266->ChannelSelectPins[i]);
			else adi_flag_Clear(pAD7266->ChannelSelectPins[i]);
		}

	} 
}

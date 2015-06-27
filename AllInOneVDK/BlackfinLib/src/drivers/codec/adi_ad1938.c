/*********************************************************************************

Copyright(c) 2005 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.


Description:
			This is the driver source code for the AD1938 audio codec. It is layered
			on top of the SPORT and SPI device drivers, which are configured for
			the specific use of the AD1938 peripheral.

*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/

#include <services/services.h>			// system service includes
#include <drivers/adi_dev.h>			// device manager includes
#include <drivers/spi/adi_spi.h>       	// spi driver includes
#include <drivers/sport/adi_sport.h>    // sport driver includes
#include <drivers/codec/adi_ad1938.h>	// AD1938 driver includes

/*********************************************************************

Enumerations and defines

*********************************************************************/
//AD1938 global address
#define AD1938GLOBALADDR 0x04;

// SPORT word length
#define SLEN_24	0x0017

// SPORT word length
#define SLEN_32	0x001f




//Buffer for sending codec register values to codec
static ADI_DEV_1D_BUFFER 	SPI_1D_BUF_OUT;

//Buffer for reading codec register values
static ADI_DEV_1D_BUFFER 	SPI_1D_BUF_IN;


//Flags completion of wait loop for dma out to codec
static volatile int spiSemaphore = 0;

static enum {
	ADI_AD1938_OPERATION_MODE_NONE,
	ADI_AD1938_OPERATION_MODE_I2S,    //I2S settings for SPT and Codec
	ADI_AD1938_OPERATION_MODE_TDM,    //TDM settings for SPT and Codec
	ADI_AD1938_OPERATION_MODE_AUX,    //AUX settings for SPT and Codec
	ADI_AD1938_OPERATION_MODE_DUALLINE_TDM  // Dual LineTDM settings for SPT and Codec
} ADI_AD1938_OPERATION_MODE;

// Configure the SPI Control register (SPI_CTL) for the
// appropriate values to access the AD1938 device
static ADI_DEV_CMD_VALUE_PAIR SPI_Cfg[] = {
	{ ADI_DEV_CMD_SET_DATAFLOW_METHOD,	(void *)ADI_DEV_MODE_CHAINED},
	{ ADI_SPI_CMD_SET_BAUD_REG, 	    (void *)0x7ff				},
	{ ADI_SPI_CMD_SET_MASTER, 			(void *)TRUE 				},//Master mode
	{ ADI_SPI_CMD_SET_WORD_SIZE, 		(void *)8					},// 8bits transfer
	{ ADI_SPI_CMD_SET_CLOCK_POLARITY,	(void *)1					},//active low SCK
	{ ADI_SPI_CMD_SET_CLOCK_PHASE,		(void *)1					},//sw chip select
	{ ADI_DEV_CMD_END, 					(void *)0 					}
};

#define ADI_AD1938_NUM_DEVICES	(sizeof(Device)/sizeof(ADI_AD1938))	// number of AD1938 devices in the system
/********************************************************************************************	
 Data Structures to manage the device
*******************************************************************************************/
typedef struct {
	ADI_DEV_PDD_HANDLE			spiHandle;	// Handle to the underlying SPI device driver
	u32					spiOpenFlag;	// flag to indicate whether spi driver is opened or not
	u32   					spiSlaveSelect; // Active SPI slave select
} ADI_AD1938_SPI;

typedef struct {
	ADI_DEV_PDD_HANDLE			sportHandle;		// Handle to the underlying SPORT device driver
	u32					sportOpenFlag;		// flag to indicate whether sport driver is opened or not
	u32   					activeSportDevice;  // Active SPORT device
} ADI_AD1938_SPORT;

typedef struct {
	ADI_DEV_MANAGER_HANDLE		ManagerHandle;		// Manager Handle
	ADI_DEV_DEVICE_HANDLE		DMHandle;			// Handle of Device Manager instance
	ADI_DMA_MANAGER_HANDLE		DMAHandle;			// handle to the DMA manager
	ADI_DCB_HANDLE				DCBHandle;			// callback handle
	u32							InUseFlag;			// flag to indicate whether an instance is in use or not
	ADI_DCB_CALLBACK_FN			DMCallback;			// the callback function supplied by the Device Manager
	u32   						OperationMode;      // I2S or TDM mode
	ADI_DEV_DIRECTION	        Direction;			// direction		
	
	ADI_AD1938_SPI			AD1938Spi;
	ADI_AD1938_SPORT		AD1938Sport;
	// Cache the regs sent to the codec
	u8	PllClockControl0;
 	u8	PllClockControl1;
 	u8	DacControl0;
 	u8	DacControl1;
 	u8	DacControl2;
 	u8	DacIndChannelMutes;
	u8	Dac1LVolumeControl;
 	u8	Dac1RVolumeControl;
 	u8	Dac2LVolumeControl;
 	u8	Dac2RVolumeControl;
 	u8	Dac3LVolumeControl;
	u8	Dac3RVolumeControl;
	u8	Dac4LVolumeControl;
 	u8	Dac4RVolumeControl;
 	u8	AdcControl0;
 	u8	AdcControl1;
 	u8	AdcControl2;
	
} ADI_AD1938;


// The initial values for the device instance
static ADI_AD1938 Device[] = {
	{
		NULL,
		NULL,
		NULL,
		NULL,
		false,
		NULL,
		ADI_AD1938_OPERATION_MODE_NONE,
        ADI_DEV_DIRECTION_UNDEFINED,               	// direction	    
		
		{
		  NULL,
		false,// SPI is not opened
		0,// default SPI slave select
		},
		{
		NULL,
		false,// SPORT is not opened
		0,// default active SPORT device
	},
	// Cache the regs sent to the codec
		0x01,//PllClockControl0
		0x00,//PllClockControl1
		0x01,//DacControl0
		0x00,//DacControl1
		0x00,//DacControl2
		0x00,//DacIndChannelMutes
		0x00,//Dac1LVolumeControl
		0x00,//Dac1RVolumeControl
		0x00,//Dac2LVolumeControl
		0x00,//Dac2RVolumeControl
		0x00,//Dac3LVolumeControl
		0x00,//Dac3RVolumeControl
		0x00,//Dac4LVolumeControl
		0x00,//Dac4RVolumeControl
		0x01,//AdcControl0
		0x00,//AdcControl1
		0x00,//AdcControl2
		
	},
};

/*********************************************************************

Static functions

*********************************************************************/
// the callback function passed to the SPI driver
static void spiCallbackFunction(void* DeviceHandle, u32 Event, void* pArg);

// the callback function passed to the SPORT driver
static void sportCallbackFunction(void* DeviceHandle, u32 Event, void* pArg);


// Validation function (debug build only)
#if defined(ADI_DEV_DEBUG)
static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle);
static int ValidateArg(u32 Value, u32 MaxValue);

#endif

/*********************************************************************
*
*	Function:		GetSet_AD1938_Reg
*
*	Description:	Read or Write AD1938 registers via SPI
*
*********************************************************************/
static u32 GetSet_AD1938_Reg(ADI_DEV_PDD_HANDLE PDDHandle, u8 regs_addr, u8 *pregs_data, 
							u8 rnwFlag)
{
	ADI_AD1938_CONTROL_PACKET spiPacket;
	ADI_AD1938_CONTROL_PACKET spiRdPacket;
	ADI_AD1938 *pAD1938 ;
	
	// default return code
	u32 Result = ADI_DEV_RESULT_SUCCESS;

	pAD1938 = (ADI_AD1938 *)PDDHandle;
	
	// form a SPI-packet
	spiPacket.registerdata = *pregs_data;
	spiPacket.registeraddress = regs_addr;
	spiPacket.globaladd_rnw = AD1938GLOBALADDR;
	spiPacket.globaladd_rnw = spiPacket.globaladd_rnw << 1;
	
	if (rnwFlag)// read request
	{
		spiPacket.globaladd_rnw |= 0x01;
	}
	
		
	SPI_1D_BUF_OUT.Data = &spiPacket;
	SPI_1D_BUF_OUT.ElementCount = sizeof(spiPacket);
	SPI_1D_BUF_OUT.ElementWidth = 1;
	SPI_1D_BUF_OUT.CallbackParameter = NULL;	
	SPI_1D_BUF_OUT.ProcessedFlag = FALSE;
	SPI_1D_BUF_OUT.pNext = NULL;

	// Submit the output buffer to the SPI device driver
	Result = adi_dev_Write(pAD1938->AD1938Spi.spiHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&SPI_1D_BUF_OUT);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

		spiRdPacket.registerdata = 0;

		SPI_1D_BUF_IN.Data = &spiRdPacket;
		SPI_1D_BUF_IN.ElementCount = sizeof(spiRdPacket);
		SPI_1D_BUF_IN.ElementWidth = 1;
		SPI_1D_BUF_IN.CallbackParameter = &SPI_1D_BUF_IN;
		SPI_1D_BUF_IN.ProcessedFlag = FALSE;	
		SPI_1D_BUF_IN.pNext = NULL;

		Result = adi_dev_Read(pAD1938->AD1938Spi.spiHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&SPI_1D_BUF_IN);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// select the device
	Result = adi_dev_Control(pAD1938->AD1938Spi.spiHandle, ADI_SPI_CMD_SELECT_SLAVE, (void *)pAD1938->AD1938Spi.spiSlaveSelect );
	
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
	
	// Enable data flow
	Result = adi_dev_Control(pAD1938->AD1938Spi.spiHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	while (!spiSemaphore)
	{
		ssync();
	}
	spiSemaphore = 0;
	
	// deselect the device
	Result = adi_dev_Control(pAD1938->AD1938Spi.spiHandle, ADI_SPI_CMD_DESELECT_SLAVE, (void *)pAD1938->AD1938Spi.spiSlaveSelect );
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// disable data flow
	Result = adi_dev_Control(pAD1938->AD1938Spi.spiHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
	
	// if read request, update preg_data content
	if (rnwFlag)
	{
		*pregs_data = ((ADI_AD1938_CONTROL_PACKET *)SPI_1D_BUF_IN.Data)->registerdata;	
	}

	return (Result);
}


/*********************************************************************
*
*	Function:		do_AD1938_spi_Open
*
*	Description:	Opens the spi device for use in configuring the
*                   AD1938
*
*********************************************************************/
static u32 SPI_Open( ADI_DEV_PDD_HANDLE PDDHandle ,u32 DeviceNumber )
{
	ADI_AD1938 	*pAD1938;	// pointer to the device we're working on

	// default return code
	u32 Result = ADI_DEV_RESULT_SUCCESS;
	
	// avoid casts
	pAD1938 = (ADI_AD1938 *)PDDHandle; // Pointer to AD1938 device driver instance


	// Open the SPI driver
	Result = adi_dev_Open(
					pAD1938->ManagerHandle,	// device manager handle
					&ADISPIIntEntryPoint,		// SPI Entry point
					DeviceNumber,				// device number
					pAD1938,			// client handle - passed to internal callback function
					&pAD1938->AD1938Spi.spiHandle,	// pointer to DM handle (for SPI driver) location
					ADI_DEV_DIRECTION_BIDIRECTIONAL,
					pAD1938->DMAHandle,		// handle to the DMA manager
					pAD1938->DCBHandle,		// handle to the callback manager
					spiCallbackFunction		// internal callback function
	);
	// return with appropriate code if SPI driver fails to open
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (ADI_AD1938_RESULT_OPEN_SPI_FAILED);
#endif
	pAD1938->AD1938Spi.spiOpenFlag = 1;
	
	// set the SPI configuration
	Result = adi_dev_Control( pAD1938->AD1938Spi.spiHandle, ADI_DEV_CMD_TABLE, (void*)SPI_Cfg );
	
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (ADI_AD1938_RESULT_SPI_CONFIG_FAILED);
#endif

	return (Result);
}

/*********************************************************************
*
*	Function:		do_AD1938_sport_Open
*
*	Description:	Opens the sport device for use with the
*                   AD1938 in I2S or TDM modes
*
*********************************************************************/
static u32 SPT_Open( ADI_DEV_PDD_HANDLE PDDHandle ,u32 DeviceNumber ) 
{
	ADI_AD1938 	*pAD1938;	// pointer to the device we're working on
	// default return code
	u32 Result = ADI_DEV_RESULT_SUCCESS;
	// avoid casts
	pAD1938 = (ADI_AD1938 *)PDDHandle; // Pointer to AD1938 device driver instance

	// Open the SPT driver
	Result = adi_dev_Open(
					pAD1938->ManagerHandle,// device manager handle
					&ADISPORTEntryPoint,	// SPORT Entry point
					DeviceNumber,			// device number
					pAD1938,				// client handle - passed to internal callback function
					&pAD1938->AD1938Sport.sportHandle,	// pointer to DM handle (for SPORT driver) location
					pAD1938->Direction,
					pAD1938->DMAHandle,	// handle to the DMA manager
					pAD1938->DCBHandle,	// handle to the callback manager
					sportCallbackFunction	// internal callback function
	);
	
	// return with appropriate code if SPI driver fails to open
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (ADI_AD1938_RESULT_OPEN_SPORT_FAILED);
#endif

	pAD1938->AD1938Sport.sportOpenFlag = true; 

	return (Result);
}

/*********************************************************************
*
*	Function:		adi_pdd_Open
*
*	Description:	Opens the AD1938 device for use
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
	ADI_AD1938 *pAD1938;		// pointer to the AD1938 device we're working on
	void *pExitCriticalArg;		// exit critical region parameter
	

	// Check for a valid device number 
#ifdef ADI_DEV_DEBUG
	if (DeviceNumber >= ADI_AD1938_NUM_DEVICES) return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
#endif

	// assign the pointer to the device instance
	pAD1938 = &Device[DeviceNumber];
	// and store the Manager handle
	pAD1938->ManagerHandle = ManagerHandle;
	// and store the Device Manager handle
	pAD1938->DMHandle = DMHandle;
	// and store the DMA Manager handle
	pAD1938->DMAHandle = DMAHandle;
	// and store the DCallback Manager handle
	pAD1938->DCBHandle = DCBHandle;
	// and store callback function
	pAD1938->DMCallback = DMCallback;
	// and store the direction
	pAD1938->Direction = Direction;

	// protect this region
	pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
	
	// Check that this device instance is not already in use. If not,
	// assign flag to indicate that it is now.
	if (pAD1938->InUseFlag == FALSE) {
		pAD1938->InUseFlag = TRUE;
		Result = ADI_DEV_RESULT_SUCCESS;
	}
	
	// unprotect 
	adi_int_ExitCriticalRegion(pExitCriticalArg);
	
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif


	// save the physical device handle in the client supplied location
	*pPDDHandle = (ADI_DEV_PDD_HANDLE *)pAD1938;
	
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
	ADI_AD1938 *pAD1938 = (ADI_AD1938 *)PDDHandle;

	// check for errors if required
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// close SPI driver
	Result = adi_dev_Close(pAD1938->AD1938Spi.spiHandle);
	pAD1938->AD1938Spi.spiOpenFlag = false;
	
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// close SPORT driver
	Result = adi_dev_Close(pAD1938->AD1938Sport.sportHandle);
	pAD1938->AD1938Sport.sportOpenFlag = false;
	
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// mark the device as closed
	pAD1938->InUseFlag = FALSE;

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
	ADI_AD1938 *pAD1938 = (ADI_AD1938 *)PDDHandle;
	Result = adi_dev_Read(pAD1938->AD1938Sport.sportHandle,BufferType,pBuffer);
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
	// check for errors if required
	u32 Result = ADI_DEV_RESULT_SUCCESS;
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Simply pass the request on
	ADI_AD1938 *pAD1938 = (ADI_AD1938 *)PDDHandle;
	Result = adi_dev_Write(pAD1938->AD1938Sport.sportHandle,BufferType,pBuffer);
	return(Result);
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

	ADI_AD1938 			*pAD1938;	// pointer to the device we're working on
	ADI_AD1938_PLL_CONTROL_REGISTER0	*pPllContReg0;// pointer to PLL control register-0
	ADI_AD1938_PLL_CONTROL_REGISTER1	*pPllContReg1;// pointer to PLL control register-1
	ADI_AD1938_DAC_CONTROL_REGISTER0	*pDacContReg0;// pointer to DAC control register-0
	ADI_AD1938_DAC_CONTROL_REGISTER1	*pDacContReg1;// pointer to DAC control register-1
	ADI_AD1938_DAC_CONTROL_REGISTER2	*pDacContReg2;// pointer to DAC control register-2
	ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER	*pDacMutesReg;// pointer to DAC Ind.Channel mutes
																//register
	ADI_AD1938_ADC_CONTROL_REGISTER0	*pAdcContReg0;// pointer to ADC control register-0
	ADI_AD1938_ADC_CONTROL_REGISTER1	*pAdcContReg1;// pointer to ADC control register-1
	ADI_AD1938_ADC_CONTROL_REGISTER2	*pAdcContReg2;// pointer to ADC control register-2
	

	u32 			Result;		// return value
	u32				u32Value;	// u32 type to avoid casts/warnings etc.
	u16				u16Value;	// u16 type to avoid casts/warnings etc.
	u8				u8Value;	// u8 type to avoid casts/warnings etc.
    u8              *pu8Value; // pointer for get set 8 bit data
	// avoid casts
	pAD1938 = (ADI_AD1938 *)PDDHandle; // Pointer to AD1938 device driver instance
    // assign 16 and 32 bit values for the Value argument
	u32Value = (u32)Value;
    	u16Value = ((u16)((u32)Value));
    	u8Value = ((u8)(u16Value));
		pu8Value = &u8Value;	

	// check for errors if required
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
	
	// if SPI and SPORT are not opened
	if ((pAD1938->AD1938Spi.spiOpenFlag == false) || (pAD1938->AD1938Sport.sportOpenFlag == false))
	{
		if ( (Command == ADI_AD1938_CMD_OPEN_SPI) || (Command == ADI_AD1938_CMD_OPEN_SPORT)
		|| (Command == ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT))
		{
			// accept these commands only
		}
		else
		return(ADI_AD1938_RESULT_SPI_SPORT_NOT_OPENED); 
	}
	
	// assume we're going to be successful
	Result = ADI_DEV_RESULT_SUCCESS;

	switch (Command)
	{
		// CASE (query for DMA support)		
		case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
		// no, we do not support it at this level (it is delegated down to SPORT driver)
			*((u32 *)Value) = FALSE;
			break;

			
		case (ADI_AD1938_CMD_OPEN_SPI):
#if defined(ADI_DEV_DEBUG)
			if ((u32Value > 0) ||(pAD1938->AD1938Spi.spiOpenFlag == TRUE))
			return(ADI_AD1938_RESULT_BAD_SPI_DEVICE);
			
#endif
		if ((Result = SPI_Open(PDDHandle,u32Value)) != ADI_DEV_RESULT_SUCCESS) return (Result);

			break;
						
		case (ADI_AD1938_CMD_OPEN_SPORT):
#if defined(ADI_DEV_DEBUG)
			if ((u32Value > 1) || (pAD1938->AD1938Sport.sportOpenFlag == TRUE))
			return(ADI_AD1938_RESULT_BAD_SPORT_DEVICE);
#endif
		if ((Result = SPT_Open(PDDHandle,u32Value)) != ADI_DEV_RESULT_SUCCESS) return (Result);

			break;			
			
		case (ADI_AD1938_CMD_SET_OPERATION_MODE_I2S):
		{
			pAD1938->OperationMode = ADI_AD1938_OPERATION_MODE_I2S;
			ADI_DEV_CMD_VALUE_PAIR I2S_Cfg[] = 
			{
				{ ADI_SPORT_CMD_SET_TCR1, 	    			(void *)(TFSR | LTFS | TCKFE)   },
				{ ADI_SPORT_CMD_SET_TCR2,       			(void *)(SLEN_24 | TSFSE) 		},
				{ ADI_SPORT_CMD_SET_RCR1, 	    			(void *)(RFSR | LRFS | RCKFE)   },
				{ ADI_SPORT_CMD_SET_RCR2,	    			(void *)(SLEN_24| RSFSE)        },
				{ ADI_DEV_CMD_END, 							(void*)0 						},
			};
			Result = adi_dev_Control(pAD1938->AD1938Sport.sportHandle, ADI_DEV_CMD_TABLE, I2S_Cfg);
			break;
		}
			
		case (ADI_AD1938_CMD_SET_OPERATION_MODE_TDM):
		{
			pAD1938->OperationMode = ADI_AD1938_OPERATION_MODE_TDM;
			ADI_DEV_CMD_VALUE_PAIR TDM_Cfg[] = 
			{
				{ ADI_SPORT_CMD_SET_TCR1, 	    	(void *)(TFSR)   	},// requires TFS for every data word
				{ ADI_SPORT_CMD_SET_TCR2,       	(void *)(SLEN_32 | TXSE) 	},// word length 32bit, secondary enable
				{ ADI_SPORT_CMD_SET_RCR1, 	    	(void *)(RFSR)   	},
				{ ADI_SPORT_CMD_SET_RCR2,	    	(void *)(SLEN_32 | RXSE)   },// word length 32, secondary enable
				{ ADI_SPORT_CMD_SET_MCMC1,	    	(void *)(0x0000)   	},// 8 active channels starts from window 0
				{ ADI_SPORT_CMD_SET_MCMC2,	   		(void *)(0x101c)   	},//multichannel,DMA tx/rx packing, frame delay=1
				{ ADI_SPORT_CMD_SET_MTCS0,	    	(void *)(0x000000FF)},// 8 transmit channels enable
				{ ADI_SPORT_CMD_SET_MRCS0,	    	(void *)(0x000000FF)},// 8 receive channels enable
				{ ADI_DEV_CMD_END, 					(void*)0 			},
			};
			Result = adi_dev_Control(pAD1938->AD1938Sport.sportHandle, ADI_DEV_CMD_TABLE, TDM_Cfg);
			break;
		}

			
			
			
		case ADI_AD1938_CMD_SET_SPI_SLAVE_SELECT:
#if defined(ADI_DEV_DEBUG)
			// validate the slave number
			if ((u32Value == 0) || (u32Value > 7)) return(ADI_AD1938_RESULT_BAD_SPI_SLAVE_NUMBER);
#endif
			pAD1938->AD1938Spi.spiSlaveSelect = u32Value;
			
			//set the active SPI chip select line
			Result = adi_dev_Control( pAD1938->AD1938Spi.spiHandle, ADI_SPI_CMD_ENABLE_SLAVE_SELECT, 
			(void*)pAD1938->AD1938Spi.spiSlaveSelect );
			
			break;
			
			case ADI_AD1938_CMD_GET_SPI_SLAVE_SELECT:
			*((u32 *)Value) = pAD1938->AD1938Spi.spiSlaveSelect;
						
			break;

			
/**************************************
* write register commands
***************************************/			
		case ADI_AD1938_CMD_SET_PLL_CLOCK_CONTROL_0:
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_0, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->PllClockControl0 = u8Value;
						
			break;
			
		case ADI_AD1938_CMD_SET_PLL_CLOCK_CONTROL_1:// PLL_CLOCK_CONTROL_1 register
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_1, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->PllClockControl1 = u8Value;						
			break;

		
		case ADI_AD1938_CMD_SET_DAC_CONTROL_0:// DAC_CONTROL_0 register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_0, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->DacControl0 = u8Value;						
			break;
			
		case ADI_AD1938_CMD_SET_DAC_CONTROL_1:// DAC_CONTROL_1 register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->DacControl1 = u8Value;						
			break;
			
		case ADI_AD1938_CMD_SET_DAC_CONTROL_2://DAC_CONTROL_2 register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_2, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->DacControl2 = u8Value;						
			break;
			
		case ADI_AD1938_CMD_SET_DAC_CHANNEL_MUTES://DAC_IND_CHANNEL_MUTES register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->DacIndChannelMutes = u8Value;						
			break;
			
			
		case ADI_AD1938_CMD_SET_DAC_1LVOLUME_CONTROL:// DAC_1LVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_1LVOLUME_CONTROL, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->Dac1LVolumeControl = u8Value;						
			break;
			
					
		case ADI_AD1938_CMD_SET_DAC_1RVOLUME_CONTROL:// DAC_1RVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_1RVOLUME_CONTROL, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->Dac1RVolumeControl = u8Value;						
			break;
			
		case ADI_AD1938_CMD_SET_DAC_2LVOLUME_CONTROL:// DAC_2LVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_2LVOLUME_CONTROL, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->Dac2LVolumeControl = u8Value;						
			break;
			
					
		case ADI_AD1938_CMD_SET_DAC_2RVOLUME_CONTROL:// DAC_2RVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_2RVOLUME_CONTROL, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->Dac2RVolumeControl = u8Value;						
			break;			
			
			case ADI_AD1938_CMD_SET_DAC_3LVOLUME_CONTROL:// DAC_3LVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_3LVOLUME_CONTROL, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->Dac3LVolumeControl = u8Value;						
			break;
			
					
		case ADI_AD1938_CMD_SET_DAC_3RVOLUME_CONTROL:// DAC_3RVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_3RVOLUME_CONTROL, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->Dac3RVolumeControl = u8Value;						
			break;		
			
		
		case ADI_AD1938_CMD_SET_DAC_4LVOLUME_CONTROL:// DAC_4LVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_4LVOLUME_CONTROL, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->Dac4LVolumeControl = u8Value;						
			break;
			
					
		case ADI_AD1938_CMD_SET_DAC_4RVOLUME_CONTROL:// DAC_4RVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_4RVOLUME_CONTROL, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->Dac4RVolumeControl = u8Value;						
			break;				
			
			
		case ADI_AD1938_CMD_SET_ADC_CONTROL_0://ADC_CONTROL_0 register
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->AdcControl0 = u8Value;						
			break;
			
		case ADI_AD1938_CMD_SET_ADC_CONTROL_1:// ADC_CONTROL_1 register
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_1, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->AdcControl1 = u8Value;						
			break;
			
		case ADI_AD1938_CMD_SET_ADC_CONTROL_2://ADC_CONTROL_2 register
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2, pu8Value, FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_WRITE_REGISTER_FAILED);
#endif
			// update the device register cache
			pAD1938->AdcControl2 = u8Value;						
			break;
			
			

			
			
/**************************************
* read register commands
***************************************/			
			
		case ADI_AD1938_CMD_GET_PLL_CLOCK_CONTROL_0:
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_0, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
			break;
			
		case ADI_AD1938_CMD_GET_PLL_CLOCK_CONTROL_1:// - AD1938 PLL_CLOCK_CONTROL_1 register 
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_1, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
			break;

		
		case ADI_AD1938_CMD_GET_DAC_CONTROL_0:				// - AD1938 DAC_CONTROL_0 register 
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_0, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
			break;
			
		case ADI_AD1938_CMD_GET_DAC_CONTROL_1:				// - AD1938 DAC_CONTROL_1 register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
			break;
			
		case ADI_AD1938_CMD_GET_DAC_CONTROL_2:				// - AD1938 DAC_CONTROL_2 register 	
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_2, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
			break;
			
		case ADI_AD1938_CMD_GET_DAC_CHANNEL_MUTES:		// - AD1938 DAC_IND_CHANNEL_MUTES register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
			break;
			
			
		case ADI_AD1938_CMD_GET_DAC_1LVOLUME_CONTROL:// DAC_1LVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_1LVOLUME_CONTROL, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
			break;
			
					
		case ADI_AD1938_CMD_GET_DAC_1RVOLUME_CONTROL:// DAC_1RVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_1RVOLUME_CONTROL, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
			break;
			
		case ADI_AD1938_CMD_GET_DAC_2LVOLUME_CONTROL:// DAC_2LVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_2LVOLUME_CONTROL, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
			break;
			
					
		case ADI_AD1938_CMD_GET_DAC_2RVOLUME_CONTROL:// DAC_2RVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_2RVOLUME_CONTROL, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
		break;			
			
		case ADI_AD1938_CMD_GET_DAC_3LVOLUME_CONTROL:// DAC_3LVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_3LVOLUME_CONTROL, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
		break;
			
					
		case ADI_AD1938_CMD_GET_DAC_3RVOLUME_CONTROL:// DAC_3RVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_3RVOLUME_CONTROL, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
		break;		
			
		
		case ADI_AD1938_CMD_GET_DAC_4LVOLUME_CONTROL:// DAC_4LVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_4LVOLUME_CONTROL, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
		break;
			
					
		case ADI_AD1938_CMD_GET_DAC_4RVOLUME_CONTROL:// Read DAC_4RVOLUME_CONTROL register
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_4RVOLUME_CONTROL, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
		break;				
			
		
		case ADI_AD1938_CMD_GET_ADC_CONTROL_0:		// - AD1938 ADC_CONTROL_0 register
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
		break;
			
		case ADI_AD1938_CMD_GET_ADC_CONTROL_1:		// - AD1938 ADC_CONTROL_1 register 
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_1, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
		break;
						
		case ADI_AD1938_CMD_GET_ADC_CONTROL_2:		// - AD1938 ADC_CONTROL_2 register 
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2, (u8 *)Value, TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) 
	return (ADI_AD1938_RESULT_READ_REGISTER_FAILED);
#endif
		break;
			
			
/**************************************
* PLL Control-0 set/get commands
***************************************/			
		case ADI_AD1938_CMD_SET_PLL_MODE:		// 0=normal operation	1=power down
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pPllContReg0 = (ADI_AD1938_PLL_CONTROL_REGISTER0 *)&pAD1938->PllClockControl0;
			pPllContReg0->pllpowerdown = u32Value;
			u8Value = pAD1938->PllClockControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_0, &u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_PLL_MODE:
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_0, (u8 *)Value, TRUE);
			pPllContReg0 = (ADI_AD1938_PLL_CONTROL_REGISTER0 *)(u8 *)Value;
			*((u32 *)Value) = pPllContReg0->pllpowerdown;
		break;
					
		case ADI_AD1938_CMD_SET_PLL_MCLK:// 00=input 256(x44.1 or 48kHz) 01=input 384(x44.1 or 48kHz)
										// 10=input 512(x44.1 or 48kHz) 11=input 768(x44.1 or 48kHz)
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,3) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pPllContReg0 = (ADI_AD1938_PLL_CONTROL_REGISTER0 *)&pAD1938->PllClockControl0;		
			pPllContReg0->mclkpinfunction  = u32Value;
			u8Value = pAD1938->PllClockControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_0,&u8Value, FALSE);
		
		break;
		
		case ADI_AD1938_CMD_GET_PLL_MCLK:
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_0,(u8 *)Value, TRUE);
			pPllContReg0 = (ADI_AD1938_PLL_CONTROL_REGISTER0 *)(u8 *)Value;		
			*((u32 *)Value) = pPllContReg0->mclkpinfunction;
		break;		
			
		case ADI_AD1938_CMD_SET_PLL_MCLK_O:// 00=XTAL Osc enable 01=256xfs VCO output
										// 10=512xfs VCO output 11=off
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,3) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pPllContReg0 = (ADI_AD1938_PLL_CONTROL_REGISTER0 *)&pAD1938->PllClockControl0;
			pPllContReg0->mclk_Opin = u32Value;
			u8Value = pAD1938->PllClockControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_0,&u8Value, FALSE);
		break; 
		
		case ADI_AD1938_CMD_GET_PLL_MCLK_O:
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_0,(u8 *)Value, TRUE);
			pPllContReg0 = (ADI_AD1938_PLL_CONTROL_REGISTER0 *)(u8 *)Value;		
			*((u32 *)Value) = pPllContReg0->mclk_Opin;
		break;		
		
		case ADI_AD1938_CMD_SET_PLL_INPUT:// 00=MCLK 01=DLRCLK 10=ALRCLK 11=reserved
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,2) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pPllContReg0 = (ADI_AD1938_PLL_CONTROL_REGISTER0 *)&pAD1938->PllClockControl0;
			pPllContReg0->pllinput = u32Value;
			u8Value = pAD1938->PllClockControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_0,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_PLL_INPUT:
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_0,(u8 *)Value, TRUE);
			pPllContReg0 = (ADI_AD1938_PLL_CONTROL_REGISTER0 *)(u8 *)Value;		
			*((u32 *)Value) = pPllContReg0->pllinput;
		break;		
			
				
		case ADI_AD1938_CMD_SET_ADC_DAC_ACTIVE:	// 0=ADC&DAC idle	1=ADC&DAC active
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pPllContReg0 = (ADI_AD1938_PLL_CONTROL_REGISTER0 *)&pAD1938->PllClockControl0;
			pPllContReg0->internalmclkenable = u32Value;
			u8Value = pAD1938->PllClockControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_0,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_DAC_ACTIVE:
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_0,(u8 *)Value, TRUE);
			pPllContReg0 = (ADI_AD1938_PLL_CONTROL_REGISTER0 *)(u8 *)Value;		
			*((u32 *)Value) = pPllContReg0->internalmclkenable;

		break;
		
		
/**************************************
* PLL Control-1 set/get commands
***************************************/			
		
		case ADI_AD1938_CMD_SET_DAC_CLOCK_SOURCE:// 0=PLL clock	1=MCLK Select DAC clock source
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pPllContReg1 = (ADI_AD1938_PLL_CONTROL_REGISTER1 *)&pAD1938->PllClockControl1;
			pPllContReg1->dacclocksource = u32Value;
			u8Value = pAD1938->PllClockControl1;
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_1,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_CLOCK_SOURCE:
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_1,(u8 *)Value, TRUE);
			pPllContReg1 = (ADI_AD1938_PLL_CONTROL_REGISTER1 *)(u8 *)Value;		
			*((u32 *)Value) = pPllContReg1->dacclocksource;

		break;
		
		case ADI_AD1938_CMD_SET_ADC_CLOCK_SOURCE:// 0=PLL clock	1=MCLK Select ADC clock source
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pPllContReg1 = (ADI_AD1938_PLL_CONTROL_REGISTER1 *)&pAD1938->PllClockControl1;
			pPllContReg1->adcclocksource = u32Value;
			u8Value = pAD1938->PllClockControl1;
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_1,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_CLOCK_SOURCE:
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_1,(u8 *)Value, TRUE);
			pPllContReg1 = (ADI_AD1938_PLL_CONTROL_REGISTER1 *)(u8 *)Value;		
			*((u32 *)Value) = pPllContReg1->adcclocksource;
		
		break;
		
		
		case ADI_AD1938_CMD_SET_VOLTAGE_REFERENCE:// 0=enable	1=disable ;On-chip Volt Reference
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pPllContReg1 = (ADI_AD1938_PLL_CONTROL_REGISTER1 *)&pAD1938->PllClockControl1;
			pPllContReg1->onchipvoltref = u32Value;
			u8Value = pAD1938->PllClockControl1;
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_1,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_VOLTAGE_REFERENCE:
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_1,(u8 *)Value, TRUE);
			pPllContReg1 = (ADI_AD1938_PLL_CONTROL_REGISTER1 *)(u8 *)Value;		
			*((u32 *)Value) = pPllContReg1->onchipvoltref;
		
		break;
		
		case ADI_AD1938_CMD_GET_PLL_LOCK_STATUS:// Read-only: PLL lock indicator,0=not locked	1=locked
			Result = GetSet_AD1938_Reg(PDDHandle, PLL_CLOCK_CONTROL_1, &u8Value, TRUE);
			u8Value = u8Value >> 3;
			*(u8 *)Value = u8Value;
		
		break;
		
/**************************************
* DAC Control Register-0 set commands
***************************************/			

		case ADI_AD1938_CMD_SET_DAC_MODE:		// 0=normal operation	1=power down
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg0 = (ADI_AD1938_DAC_CONTROL_REGISTER0 *)&pAD1938->DacControl0;
			pDacContReg0->powerdown = u32Value;
			u8Value = pAD1938->DacControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_0,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_MODE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_0,(u8 *)Value, TRUE);
			pDacContReg0 = (ADI_AD1938_DAC_CONTROL_REGISTER0 *)(u8 *)Value;		
			*((u32 *)Value) = pDacContReg0->powerdown;
		
		break;
		
		case ADI_AD1938_CMD_SET_DAC_SAMPLE_RATE:	// 00=32/44.1/48kHz	01=64/88.2/96kHz
										// 10=128/176.4/192kHz 11= reserved
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,2) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg0 = (ADI_AD1938_DAC_CONTROL_REGISTER0 *)&pAD1938->DacControl0;
			pDacContReg0->samplerate  = u32Value;
			u8Value = pAD1938->DacControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_0,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_SAMPLE_RATE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_0,(u8 *)Value, TRUE);
			pDacContReg0 = (ADI_AD1938_DAC_CONTROL_REGISTER0 *)(u8 *)Value;		
			*((u32 *)Value) = pDacContReg0->samplerate;
		break;
										
		case ADI_AD1938_CMD_SET_DAC_SDATA_DELAY:	// 000=1	001=0	010=8	011=12	100=16 (BCLK periods)	
										// 101=reserved 110=reserved 111=reserved
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,4) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg0 = (ADI_AD1938_DAC_CONTROL_REGISTER0 *)&pAD1938->DacControl0;
			pDacContReg0->sdatadelay = u32Value;
			u8Value = pAD1938->DacControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_0,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_SDATA_DELAY:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_0,(u8 *)Value, TRUE);
			pDacContReg0 = (ADI_AD1938_DAC_CONTROL_REGISTER0 *)(u8 *)Value;		
			*((u32 *)Value) = pDacContReg0->sdatadelay;
		break;
											
		case ADI_AD1938_CMD_SET_DAC_SERIAL_FORMAT:// 00=stereo	01=TDM  10=AUX  11=dual-line TDM
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,3) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg0 = (ADI_AD1938_DAC_CONTROL_REGISTER0 *)&pAD1938->DacControl0;
			pDacContReg0->serialformat= u32Value;
			u8Value = pAD1938->DacControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_0,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_SERIAL_FORMAT:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_0,(u8 *)Value, TRUE);
			pDacContReg0 = (ADI_AD1938_DAC_CONTROL_REGISTER0 *)(u8 *)Value;		
			*((u32 *)Value) = pDacContReg0->serialformat;
		break;
		
/**************************************
* DAC Control Register-1 set commands
***************************************/			
		case ADI_AD1938_CMD_SET_DAC_BCLK_ACTIVE_EDGE:// 0=latch in mid cycle	1=latch in at end of cycle
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg1 = (ADI_AD1938_DAC_CONTROL_REGISTER1 *)&pAD1938->DacControl1;
			pDacContReg1->bclkactiveedge = u32Value;
			u8Value = pAD1938->DacControl1;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_BCLK_ACTIVE_EDGE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1,(u8 *)Value, TRUE);
			pDacContReg1 = (ADI_AD1938_DAC_CONTROL_REGISTER1 *)(u8 *)Value;
			*((u32 *)Value) = pDacContReg1->bclkactiveedge;
		break;

		case ADI_AD1938_CMD_SET_DAC_NUMBER_OF_CHANNEL://BCLKs per frame 00=64(2chan)	01=128(4chan)	10=256(8chan)	11=512(16chan)
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,3) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg1 = (ADI_AD1938_DAC_CONTROL_REGISTER1 *)&pAD1938->DacControl1;
			pDacContReg1->bclkperframe= u32Value;
			u8Value = pAD1938->DacControl1;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_NUMBER_OF_CHANNEL:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1,(u8 *)Value, TRUE);
			pDacContReg1 = (ADI_AD1938_DAC_CONTROL_REGISTER1 *)(u8 *)Value;
			*((u32 *)Value) =pDacContReg1->bclkperframe;
		break;
		
		case ADI_AD1938_CMD_SET_DAC_LRCLK_POLARITY:// 0=left low	1=left high
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg1 = (ADI_AD1938_DAC_CONTROL_REGISTER1 *)&pAD1938->DacControl1;
			pDacContReg1->lrclkpolarity = u32Value;
			u8Value = pAD1938->DacControl1;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_LRCLK_POLARITY:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1,(u8 *)Value, TRUE);
			pDacContReg1 = (ADI_AD1938_DAC_CONTROL_REGISTER1 *)(u8 *)Value;
			*((u32 *)Value) = pDacContReg1->lrclkpolarity;
		break;
		
		case ADI_AD1938_CMD_SET_DAC_LRCLK_MASTER_SLAVE:// 0=LRCLK slave	1=LRCLK master
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg1 = (ADI_AD1938_DAC_CONTROL_REGISTER1 *)&pAD1938->DacControl1;
			pDacContReg1->lrclkmasterslave = u32Value;
			u8Value = pAD1938->DacControl1;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_LRCLK_MASTER_SLAVE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1,(u8 *)Value, TRUE);
			pDacContReg1 = (ADI_AD1938_DAC_CONTROL_REGISTER1 *)(u8 *)Value;
			*((u32 *)Value) = pDacContReg1->lrclkmasterslave;
		break;
		
		case ADI_AD1938_CMD_SET_DAC_BCLK_MASTER_SLAVE:// 0=BCLK slave	1=BCLK master
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg1 = (ADI_AD1938_DAC_CONTROL_REGISTER1 *)&pAD1938->DacControl1;
			pDacContReg1->bclkmasterslave= u32Value;
			u8Value = pAD1938->DacControl1;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_BCLK_MASTER_SLAVE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1,(u8 *)Value, TRUE);
			pDacContReg1 = (ADI_AD1938_DAC_CONTROL_REGISTER1 *)(u8 *)Value;
			*((u32 *)Value) = pDacContReg1->bclkmasterslave;
		break;
		
		case ADI_AD1938_CMD_SET_DAC_BCLK_SOURCE:// 0=DBCLK pin	1=internally generated
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg1 = (ADI_AD1938_DAC_CONTROL_REGISTER1 *)&pAD1938->DacControl1;
			pDacContReg1->bclksource= u32Value;
			u8Value = pAD1938->DacControl1;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_BCLK_SOURCE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1,(u8 *)Value, TRUE);
			pDacContReg1 = (ADI_AD1938_DAC_CONTROL_REGISTER1 *)(u8 *)Value;
			*((u32 *)Value) = pDacContReg1->bclksource;
		break;
		
		case ADI_AD1938_CMD_SET_DAC_BCLK_POLARITY:// 0=normal	1=inverted
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg1 = (ADI_AD1938_DAC_CONTROL_REGISTER1 *)&pAD1938->DacControl1;
			pDacContReg1->bclkpolarity = u32Value;
			u8Value = pAD1938->DacControl1;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_BCLK_POLARITY:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_1,(u8 *)Value, TRUE);
			pDacContReg1 = (ADI_AD1938_DAC_CONTROL_REGISTER1 *)(u8 *)Value;
			*((u32 *)Value) = pDacContReg1->bclkpolarity;
		break;
			
/**************************************
* DAC Control Register-2 set commands
***************************************/			
		case ADI_AD1938_CMD_SET_DAC_MASTER_MUTE:// 0=unmute	1=mute
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg2 = (ADI_AD1938_DAC_CONTROL_REGISTER2 *)&pAD1938->DacControl2;
			pDacContReg2->mastermute = u32Value;
			u8Value = pAD1938->DacControl2;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_2,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_MASTER_MUTE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_2,(u8 *)Value, TRUE);
			pDacContReg2 = (ADI_AD1938_DAC_CONTROL_REGISTER2 *)(u8 *)Value;
			*((u32 *)Value) = pDacContReg2->mastermute;
		break;
		
		case ADI_AD1938_CMD_SET_DAC_DEEMPHASIS_MODE:// 00=flat	01=48kHz curve	10=44.1kHz curve	11=32kHz curve
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,3) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg2 = (ADI_AD1938_DAC_CONTROL_REGISTER2 *)&pAD1938->DacControl2;
			pDacContReg2->deemphasis = u32Value;
			u8Value = pAD1938->DacControl2;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_2,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_DEEMPHASIS_MODE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_2,(u8 *)Value, TRUE);
			pDacContReg2 = (ADI_AD1938_DAC_CONTROL_REGISTER2 *)(u8 *)Value;
			*((u32 *)Value) = pDacContReg2->deemphasis;
		break;
		
		case ADI_AD1938_CMD_SET_DAC_WORD_WIDTH:// 00=24	01=20	10=reserved	 11=16
#ifdef ADI_DEV_DEBUG
			if (u32Value == 2 || u32Value > 3)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg2 = (ADI_AD1938_DAC_CONTROL_REGISTER2 *)&pAD1938->DacControl2;
			pDacContReg2->wordwidth = u32Value;
			u8Value = pAD1938->DacControl2;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_2,&u8Value, FALSE);
		break;		case ADI_AD1938_CMD_GET_DAC_WORD_WIDTH:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_2,(u8 *)Value, TRUE);
			pDacContReg2 = (ADI_AD1938_DAC_CONTROL_REGISTER2 *)(u8 *)Value;
			*((u32 *)Value) = pDacContReg2->wordwidth;
		break;
		
		case ADI_AD1938_CMD_SET_DAC_OUTPUT_POLARITY:// 0=non-inverted	1=inverted
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacContReg2 = (ADI_AD1938_DAC_CONTROL_REGISTER2 *)&pAD1938->DacControl2;
			pDacContReg2->dacoutputpolarity = u32Value;
			u8Value = pAD1938->DacControl2;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_2,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_OUTPUT_POLARITY:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_CONTROL_2,(u8 *)Value, TRUE);
			pDacContReg2 = (ADI_AD1938_DAC_CONTROL_REGISTER2 *)(u8 *)Value;
			*((u32 *)Value) = pDacContReg2->dacoutputpolarity;
		break;
		
/****************************************************
* DAC Individual Channel Mutes Register set commands
*****************************************************/			
		case ADI_AD1938_CMD_SET_DAC_1LEFT_MUTE:// 0=unmute	1=mute
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)&pAD1938->DacIndChannelMutes;
			pDacMutesReg->dac1leftmute = u32Value;
			u8Value = pAD1938->DacIndChannelMutes;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_1LEFT_MUTE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,(u8 *)Value, TRUE);
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)(u8 *)Value;
			*((u32 *)Value) = pDacMutesReg->dac1leftmute;
		break;
		
		case ADI_AD1938_CMD_SET_DAC_1RIGHT_MUTE:// 0=unmute	1=mute
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)&pAD1938->DacIndChannelMutes;
			pDacMutesReg->dac1rightmute = u32Value;
			u8Value = pAD1938->DacIndChannelMutes;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_1RIGHT_MUTE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,(u8 *)Value, TRUE);
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)(u8 *)Value;
			*((u32 *)Value) = pDacMutesReg->dac1rightmute;
	
		break;
		
		case ADI_AD1938_CMD_SET_DAC_2LEFT_MUTE:// 0=unmute	1=mute
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)&pAD1938->DacIndChannelMutes;
			pDacMutesReg->dac2leftmute = u32Value;
			u8Value = pAD1938->DacIndChannelMutes;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,&u8Value, FALSE);
		break;

		case ADI_AD1938_CMD_GET_DAC_2LEFT_MUTE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,(u8 *)Value, TRUE);
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)(u8 *)Value;
			*((u32 *)Value) = pDacMutesReg->dac2leftmute;

		break;
		
		case ADI_AD1938_CMD_SET_DAC_2RIGHT_MUTE:// 0=unmute	1=mute
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)&pAD1938->DacIndChannelMutes;
			pDacMutesReg->dac2rightmute = u32Value;
			u8Value = pAD1938->DacIndChannelMutes;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_2RIGHT_MUTE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,(u8 *)Value, TRUE);
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)(u8 *)Value;
			*((u32 *)Value) = pDacMutesReg->dac2rightmute;
		break;

		case ADI_AD1938_CMD_SET_DAC_3LEFT_MUTE:// 0=unmute	1=mute
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)&pAD1938->DacIndChannelMutes;
			pDacMutesReg->dac3leftmute = u32Value;
			u8Value = pAD1938->DacIndChannelMutes;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_3LEFT_MUTE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,(u8 *)Value, TRUE);
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)(u8 *)Value;
			*((u32 *)Value) = pDacMutesReg->dac3leftmute;
		break;
		
		case ADI_AD1938_CMD_SET_DAC_3RIGHT_MUTE:// 0=unmute	1=mute
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)&pAD1938->DacIndChannelMutes;
			pDacMutesReg->dac3rightmute = u32Value;
			u8Value = pAD1938->DacIndChannelMutes;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_3RIGHT_MUTE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,(u8 *)Value, TRUE);
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)(u8 *)Value;
			*((u32 *)Value) = pDacMutesReg->dac3rightmute;
		
		break;
		
		case ADI_AD1938_CMD_SET_DAC_4LEFT_MUTE:// 0=unmute	1=mute
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)&pAD1938->DacIndChannelMutes;
			pDacMutesReg->dac4leftmute = u32Value;
			u8Value = pAD1938->DacIndChannelMutes;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_4LEFT_MUTE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,(u8 *)Value, TRUE);
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)(u8 *)Value;
			*((u32 *)Value) = pDacMutesReg->dac4leftmute;
		break;
		
		case ADI_AD1938_CMD_SET_DAC_4RIGHT_MUTE:// 0=unmute	1=mute
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)&pAD1938->DacIndChannelMutes;
			pDacMutesReg->dac4rightmute = u32Value;
			u8Value = pAD1938->DacIndChannelMutes;
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_DAC_4RIGHT_MUTE:
			Result = GetSet_AD1938_Reg(PDDHandle, DAC_IND_CHANNEL_MUTES,(u8 *)Value, TRUE);
			pDacMutesReg = (ADI_AD1938_DAC_CHANNEL_MUTES_REGISTER *)(u8 *)Value;
			*((u32 *)Value) = pDacMutesReg->dac4rightmute;
		break;
		

		
/**************************************
* ADC Control Register-0 set commands
***************************************/			
		case ADI_AD1938_CMD_SET_ADC_MODE:		// 0=normal operation	1=power down
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg0 = (ADI_AD1938_ADC_CONTROL_REGISTER0 *)&pAD1938->AdcControl0;
			pAdcContReg0->powerdown = u32Value;
			u8Value = pAD1938->AdcControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_MODE:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0,(u8 *)Value, TRUE);
			pAdcContReg0 = (ADI_AD1938_ADC_CONTROL_REGISTER0 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg0->powerdown;
		break;
		
		case ADI_AD1938_CMD_SET_ADC_HP_FILTER:	// 0=highpass filter off	1=highpass filter on
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg0 = (ADI_AD1938_ADC_CONTROL_REGISTER0 *)&pAD1938->AdcControl0;
			pAdcContReg0->highpassfilter  = u32Value;
			u8Value = pAD1938->AdcControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_HP_FILTER:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0,(u8 *)Value, TRUE);
			pAdcContReg0 = (ADI_AD1938_ADC_CONTROL_REGISTER0 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg0->highpassfilter;
		break;
		
		case ADI_AD1938_CMD_SET_ADC_1L_MUTE:		// 0=unmute	1=mute
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg0 = (ADI_AD1938_ADC_CONTROL_REGISTER0 *)&pAD1938->AdcControl0;
			pAdcContReg0->adc1lmute = u32Value;
			u8Value = pAD1938->AdcControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_1L_MUTE:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0,(u8 *)Value, TRUE);
			pAdcContReg0 = (ADI_AD1938_ADC_CONTROL_REGISTER0 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg0->adc1lmute;
		break;
		
		case ADI_AD1938_CMD_SET_ADC_1R_MUTE:		// 0=unmute	1=mute
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg0 = (ADI_AD1938_ADC_CONTROL_REGISTER0 *)&pAD1938->AdcControl0;
			pAdcContReg0->adc1rmute = u32Value;
			u8Value = pAD1938->AdcControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_1R_MUTE:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0,(u8 *)Value, TRUE);
			pAdcContReg0 = (ADI_AD1938_ADC_CONTROL_REGISTER0 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg0->adc1rmute;
		break;
		
		case ADI_AD1938_CMD_SET_ADC_2L_MUTE:		// 0=unmute	1=mute
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg0 = (ADI_AD1938_ADC_CONTROL_REGISTER0 *)&pAD1938->AdcControl0;
			pAdcContReg0->adc2lmute = u32Value;
			u8Value = pAD1938->AdcControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_2L_MUTE:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0,(u8 *)Value, TRUE);
			pAdcContReg0 = (ADI_AD1938_ADC_CONTROL_REGISTER0 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg0->adc2lmute;
		break;
		
		case ADI_AD1938_CMD_SET_ADC_2R_MUTE:		// 0=unmute	1=mute
#ifdef ADI_DEV_DEBUG
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg0 = (ADI_AD1938_ADC_CONTROL_REGISTER0 *)&pAD1938->AdcControl0;
			pAdcContReg0->adc2rmute = u32Value;
			u8Value = pAD1938->AdcControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_2R_MUTE:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0,(u8 *)Value, TRUE);
			pAdcContReg0 = (ADI_AD1938_ADC_CONTROL_REGISTER0 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg0->adc2rmute;
		
		break;
		
		case ADI_AD1938_CMD_SET_ADC_SAMPLE_RATE:// 00=32/44.1/48		01=64/88.2/96
												// 10=128/176.4/192	 11=reserved
#ifdef ADI_DEV_DEBUG							
			if (ValidateArg(u32Value,2) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg0 = (ADI_AD1938_ADC_CONTROL_REGISTER0 *)&pAD1938->AdcControl0;
			pAdcContReg0->outputsamplerate = u32Value;
			u8Value = pAD1938->AdcControl0;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_SAMPLE_RATE:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_0,(u8 *)Value, TRUE);
			pAdcContReg0 = (ADI_AD1938_ADC_CONTROL_REGISTER0 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg0->outputsamplerate;
		break;
		
												
/**************************************
* ADC Control Register-1 set commands
***************************************/			
		case ADI_AD1938_CMD_SET_ADC_WORD_WIDTH:// 00=24	01=20	10=reserved		11=16
#ifdef ADI_DEV_DEBUG
			if (u32Value == 2 || u32Value > 3)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg1 = (ADI_AD1938_ADC_CONTROL_REGISTER1 *)&pAD1938->AdcControl1;
			pAdcContReg1->wordwidth = u32Value;
			u8Value = pAD1938->AdcControl1;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_1,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_WORD_WIDTH:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_1,(u8 *)Value, TRUE);
			pAdcContReg1 = (ADI_AD1938_ADC_CONTROL_REGISTER1 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg1->wordwidth;
		break;
		
		case ADI_AD1938_CMD_SET_ADC_SDATA_DELAY:	// 000=1	001=0	010=8	011=12	100=16 (BCLK periods)	
													// 101=reserved 110=reserved 111=reserved
#ifdef ADI_DEV_DEBUG							
			if (ValidateArg(u32Value,4) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg1 = (ADI_AD1938_ADC_CONTROL_REGISTER1 *)&pAD1938->AdcControl1;
			pAdcContReg1->sdatadelay = u32Value;
			u8Value = pAD1938->AdcControl1;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_1,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_SDATA_DELAY:													
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_1,(u8 *)Value, TRUE);
			pAdcContReg1 = (ADI_AD1938_ADC_CONTROL_REGISTER1 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg1->sdatadelay;												
		break;
		
		case ADI_AD1938_CMD_SET_ADC_SERIAL_FORMAT:// 00=stereo	01=TDM  10=AUX  11=dual-line TDM
#ifdef ADI_DEV_DEBUG							
			if (ValidateArg(u32Value,3) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg1 = (ADI_AD1938_ADC_CONTROL_REGISTER1 *)&pAD1938->AdcControl1;
			pAdcContReg1->serialformat = u32Value;
			u8Value = pAD1938->AdcControl1;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_1,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_SERIAL_FORMAT:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_1,(u8 *)Value, TRUE);
			pAdcContReg1 = (ADI_AD1938_ADC_CONTROL_REGISTER1 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg1->serialformat;
		break;
		
		case ADI_AD1938_CMD_SET_ADC_BCLK_ACTIVE_EDGE:// 0=latch in mid cycle	1=latch in at end of cycle
#ifdef ADI_DEV_DEBUG							
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg1 = (ADI_AD1938_ADC_CONTROL_REGISTER1 *)&pAD1938->AdcControl1;
			pAdcContReg1->bclkactiveedge = u32Value;
			u8Value = pAD1938->AdcControl1;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_1,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_BCLK_ACTIVE_EDGE:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_1,(u8 *)Value, TRUE);
			pAdcContReg1 = (ADI_AD1938_ADC_CONTROL_REGISTER1 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg1->bclkactiveedge;
		break;
		

/**************************************
* ADC Control Register-2 set commands
***************************************/			
		case ADI_AD1938_CMD_SET_ADC_LRCLK_FORMAT:// 0=50/50(allows 32/24/20/16 BCLK/channel)
												// 1=pulse(32 BCLK/channel)
#ifdef ADI_DEV_DEBUG							
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg2 = (ADI_AD1938_ADC_CONTROL_REGISTER2 *)&pAD1938->AdcControl2;
			pAdcContReg2->lrclkformat = u32Value;
			u8Value = pAD1938->AdcControl2;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_LRCLK_FORMAT:										
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2,(u8 *)Value, TRUE);
			pAdcContReg2 = (ADI_AD1938_ADC_CONTROL_REGISTER2 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg2->lrclkformat;
		break;
		
		case ADI_AD1938_CMD_SET_ADC_BCLK_POLARITY:// 0=drive out on falling edge
										 // 1=drive out on rising edge
#ifdef ADI_DEV_DEBUG							
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg2 = (ADI_AD1938_ADC_CONTROL_REGISTER2 *)&pAD1938->AdcControl2;
			pAdcContReg2->bclkpolarity = u32Value;
			u8Value = pAD1938->AdcControl2;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_BCLK_POLARITY:										 
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2,(u8 *)Value, TRUE);
			pAdcContReg2 = (ADI_AD1938_ADC_CONTROL_REGISTER2 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg2->bclkpolarity ;
		break;
		
		case ADI_AD1938_CMD_SET_ADC_LRCLK_POLARITY:// 0=left low	1=left high
#ifdef ADI_DEV_DEBUG							
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg2 = (ADI_AD1938_ADC_CONTROL_REGISTER2 *)&pAD1938->AdcControl2;
			pAdcContReg2->lrclkpolarity = u32Value;
			u8Value = pAD1938->AdcControl2;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_LRCLK_POLARITY:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2,(u8 *)Value, TRUE);
			pAdcContReg2 = (ADI_AD1938_ADC_CONTROL_REGISTER2 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg2->lrclkpolarity;
		break;
		
		case ADI_AD1938_CMD_SET_ADC_LRCLK_MASTER_SLAVE:// 0=LRCLK slave	1=LRCLK master
#ifdef ADI_DEV_DEBUG							
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
		 	pAdcContReg2 = (ADI_AD1938_ADC_CONTROL_REGISTER2 *)&pAD1938->AdcControl2;
			pAdcContReg2->lrclkmasterslave = u32Value;
			u8Value = pAD1938->AdcControl2;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_LRCLK_MASTER_SLAVE:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2,(u8 *)Value, TRUE);
			pAdcContReg2 = (ADI_AD1938_ADC_CONTROL_REGISTER2 *)(u8 *)Value;
		 	*((u32 *)Value) = pAdcContReg2->lrclkmasterslave;
		break;
		
		
		case ADI_AD1938_CMD_SET_ADC_BCLKS_FRAME:// 00=64	01=128	10=256	11=512
#ifdef ADI_DEV_DEBUG							
			if (ValidateArg(u32Value,3) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg2 = (ADI_AD1938_ADC_CONTROL_REGISTER2 *)&pAD1938->AdcControl2;
			pAdcContReg2->bclkperframe = u32Value;
			u8Value = pAD1938->AdcControl2;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_BCLKS_FRAME:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2,(u8 *)Value, TRUE);
			pAdcContReg2 = (ADI_AD1938_ADC_CONTROL_REGISTER2 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg2->bclkperframe;
		break;
		
		case ADI_AD1938_CMD_SET_ADC_BCLK_MASTER_SLAVE:// 0=BCLK slave	1=BCLK master
#ifdef ADI_DEV_DEBUG							
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg2 = (ADI_AD1938_ADC_CONTROL_REGISTER2 *)&pAD1938->AdcControl2;
			pAdcContReg2->bclkmasterslave = u32Value;
			u8Value = pAD1938->AdcControl2;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_BCLK_MASTER_SLAVE:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2,(u8 *)Value, TRUE);
			pAdcContReg2 = (ADI_AD1938_ADC_CONTROL_REGISTER2 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg2->bclkmasterslave;
		break;
		
		case ADI_AD1938_CMD_SET_ADC_BCLK_SOURCE:// 0=ABCLK pin	1=internally generated
#ifdef ADI_DEV_DEBUG							
			if (ValidateArg(u32Value,1) != 0)
			return (ADI_AD1938_RESULT_BAD_VALUE);
#endif
			pAdcContReg2 = (ADI_AD1938_ADC_CONTROL_REGISTER2 *)&pAD1938->AdcControl2;
			pAdcContReg2->bclksource = u32Value;
			u8Value = pAD1938->AdcControl2;
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2,&u8Value, FALSE);
		break;
		
		case ADI_AD1938_CMD_GET_ADC_BCLK_SOURCE:
			Result = GetSet_AD1938_Reg(PDDHandle, ADC_CONTROL_2,(u8 *)Value, TRUE);
			pAdcContReg2 = (ADI_AD1938_ADC_CONTROL_REGISTER2 *)(u8 *)Value;
			*((u32 *)Value) = pAdcContReg2->bclksource;
		break;
		
	
		default:
			// pass anything we don't specifically handle to the SPT driver
			Result = adi_dev_Control(pAD1938->AD1938Sport.sportHandle, Command, Value);
			
		break;
	}

	return(Result);
}

/*********************************************************************

	Function:		spiCallbackFunction

	Description:	Fields the callback from the SPI Driver

*********************************************************************/

static void spiCallbackFunction(void* DeviceHandle, u32 Event, void* pArg)
{
	ADI_AD1938 *pAD1938 = (ADI_AD1938 *)DeviceHandle;
#if defined(ADI_DEV_DEBUG)
	if (ValidatePDDHandle(DeviceHandle) != ADI_DEV_RESULT_SUCCESS) {  	// verify the DM handle
		return;
	}
#endif

	switch (Event) {
		
		// identify which buffer is generating the callback
		case ADI_SPI_EVENT_WRITE_BUFFER_PROCESSED:
		break;
			
		case ADI_SPI_EVENT_READ_BUFFER_PROCESSED:
		// drive slave select line to high
		adi_dev_Control(pAD1938->AD1938Spi.spiHandle, ADI_SPI_CMD_SELECT_SLAVE, (void *)pAD1938->AD1938Spi.spiSlaveSelect );

		spiSemaphore = 1;
	
		break;

		case ADI_SPI_EVENT_TRANSMISSION_ERROR:
		case ADI_SPI_EVENT_RECEIVE_ERROR:
		break;	
			
	// ENDCASE
	}

	// simply pass the callback along to the Device Manager Callback without interpretation
	(pAD1938->DMCallback)(pAD1938->DMHandle,Event,pArg);

	
}

/*********************************************************************

	Function:		sportCallbackFunction

	Description:	Fields the callback from the SPORT Driver

*********************************************************************/

static void sportCallbackFunction(void* DeviceHandle, u32 Event, void* pArg)
{
	ADI_AD1938 *pAD1938 = (ADI_AD1938 *)DeviceHandle;
#if defined(ADI_DEV_DEBUG)
	if (ValidatePDDHandle(DeviceHandle) != ADI_DEV_RESULT_SUCCESS) {  	// verify the DM handle
		return;
	}
#endif
	// simply pass the callback along to the Device Manager Callback without interpretation
	(pAD1938->DMCallback)(pAD1938->DMHandle,Event,pArg);
}

#if defined(ADI_DEV_DEBUG)

/*********************************************************************

	Function:		ValidatePDDHandle

	Description:	Validates a PDD handle

*********************************************************************/

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle) {
	unsigned int i;
	for (i = 0; i < ADI_AD1938_NUM_DEVICES; i++) {
		if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i]) {
			return (ADI_DEV_RESULT_SUCCESS);
		}
	}
	return (ADI_DEV_RESULT_BAD_PDD_HANDLE);
}

/*********************************************************************

	Function:		ValidateArg

	Description:	Validates a value range

*********************************************************************/
static int ValidateArg(u32 Value, u32 MaxValue {
	
	if( Value > MaxValue)
	return (ADI_AD1938_RESULT_BAD_VALUE);
	else
	return (ADI_DEV_RESULT_SUCCESS);
}





#endif

/*********************************************************************

Entry point for device manager

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT ADIAD1938EntryPoint = {
	adi_pdd_Open,
	adi_pdd_Close,
	adi_pdd_Read,
	adi_pdd_Write,
	adi_pdd_Control
};




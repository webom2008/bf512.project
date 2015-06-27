/*********************************************************************************

Copyright(c) 2005 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_ad1836.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			This is the driver source code for the AD1836 audio codec. It is layered
			on top of the SPORT and SPI device drivers, which are configured for
			the specific use of the AD1836 peripheral.

*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/

#include <services/services.h>			// system service includes
#include <drivers/adi_dev.h>			// device manager includes
#include <drivers/spi/adi_spi.h>       	// spi driver includes
#include <drivers/sport/adi_sport.h>    // sport driver includes
#include <drivers/codec/adi_ad1836.h>	// AD1836 driver includes

/*********************************************************************

Enumerations and defines

*********************************************************************/

// SPI transfer mode
#define TIMOD_DMA_TX 0x0003

// SPORT word length
#define SLEN_24	0x0017

// SPORT word length
#define SLEN_32	0x001f

// DMA flow mode
#define FLOW_1	0x1000

// addresses for Port B in Flash A (used for resetting the codec)
#define pFlashA_PortA_Dir	(volatile unsigned char *)0x20270006

#define pFlashA_PortA_Data	(volatile unsigned char *)0x20270004

//Buffer for sending codec register values to codec
static ADI_DEV_1D_BUFFER 	SPI_1D_BUF_OUT;

//Flags completion of wait loop for dma out to codec
static volatile int spiSemaphore = 0;

static enum {
	ADI_AD1836_OPERATION_MODE_NONE,
	ADI_AD1836_OPERATION_MODE_I2S,    //I2S settings for SPT and Codec
	ADI_AD1836_OPERATION_MODE_TDM,    //TDM settings for SPT and Codec
} ADI_AD1836_OPERATION_MODE;

// Configure the SPI Control register (SPI_CTL) for the
// appropriate values to access the AD1836 device
static ADI_DEV_CMD_VALUE_PAIR SPI_Cfg[] = {
	{ ADI_DEV_CMD_SET_DATAFLOW_METHOD,	(void *)ADI_DEV_MODE_CHAINED   			},
	{ ADI_SPI_CMD_SET_BAUD_REG, 	    (void *)16			           			},
	{ ADI_SPI_CMD_SET_CONTROL_REG, 		(void *)(TIMOD_DMA_TX | SIZE | MSTR)	},
	{ADI_DEV_CMD_END, 					(void *)0 								}
};

#define ADI_AD1836_NUM_DEVICES	(sizeof(Device)/sizeof(ADI_AD1836))	// number of AD1836 devices in the system

typedef struct {
	ADI_DEV_DEVICE_HANDLE		DMHandle;			// Handle of Device Manager instance
	ADI_DEV_PDD_HANDLE			spiHandle;			// Handle to the underlying SPI device driver
	ADI_DEV_PDD_HANDLE			sportHandle;		// Handle to the underlying SPORT device driver
	u32							InUseFlag;			// flag to indicate whether an instance is in use or not
	ADI_DCB_CALLBACK_FN			DMCallback;			// the callback function supplied by the Device Manager
	u32   						OperationMode;      // I2S or TDM mode
	// Cache the regs sent to the codec
	u16							DACControl1;
	u16							DACControl2;
	u16							DACVolume0;
	u16							DACVolume1;
	u16							DACVolume2;
	u16							DACVolume3;
	u16							DACVolume4;
	u16							DACVolume5;
	u16							ADCControl1;
	u16							ADCControl2;
	u16							ADCControl3;
	u32                         SlaveSelectFlag;
} ADI_AD1836;

// The initial values for the device instance
static ADI_AD1836 Device[] = {
	{
		NULL,
		NULL,
		NULL,
		0,
		NULL,
		ADI_AD1836_OPERATION_MODE_NONE,
		DAC_CONTROL_1,
		DAC_CONTROL_2,
		DAC_VOLUME_0,
		DAC_VOLUME_1,
		DAC_VOLUME_2,
		DAC_VOLUME_3,
		DAC_VOLUME_4,
		DAC_VOLUME_5,
		ADC_CONTROL_1,
		ADC_CONTROL_2,
		ADC_CONTROL_3,
		0,
	},
};

// the callback function passed to the SPI driver
static void spiCallbackFunction(void* DeviceHandle, u32 Event, void* pArg);

// the callback function passed to the SPORT driver
static void sportCallbackFunction(void* DeviceHandle, u32 Event, void* pArg);


// Validation function (debug build only)
#if defined(ADI_DEV_DEBUG)
static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle);
#endif

static void reset_AD1836(void)
{
	int i;

	*pFlashA_PortA_Dir = 0x1;

	static unsigned char ucActive_LED = 0x01;
	
	// write to Port A to reset AD1836
	*pFlashA_PortA_Data = 0x00;
	
	// wait to recover from reset
	for (i=0; i<0xf0000; i++) asm("nop;");
	
	// write to Port A to enable AD1836
	*pFlashA_PortA_Data = ucActive_LED;
	
	// wait to recover from reset
	for (i=0; i<0xf0000; i++) asm("nop;");
	
	
}

static u32 Set_AD1836_Reg(ADI_DEV_PDD_HANDLE PDDHandle, u16* ad1836_regs, int regcount)
{
	int i;
	
	// default return code
	u32 Result = ADI_DEV_RESULT_SUCCESS;

	ADI_AD1836 *pAD1836 = (ADI_AD1836 *)PDDHandle;
	
	SPI_1D_BUF_OUT.Data = ad1836_regs;
	SPI_1D_BUF_OUT.ElementCount = regcount;
	SPI_1D_BUF_OUT.ElementWidth = 2;
	SPI_1D_BUF_OUT.CallbackParameter = &SPI_1D_BUF_OUT;	
	SPI_1D_BUF_OUT.pNext = NULL;

	// Submit the output buffer to the device manager
	Result = adi_dev_Write(pAD1836->spiHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&SPI_1D_BUF_OUT);	
	
	// Enable data flow
	Result = adi_dev_Control(pAD1836->spiHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)TRUE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	while (!spiSemaphore)
	{
		ssync();
	}
	spiSemaphore = 0;
	
	// wait for SPI to send to codec
	for (i=0; i<0xaff0; i++) asm("nop;");
	
	// Enable data flow
	Result = adi_dev_Control(pAD1836->spiHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)FALSE);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	return (Result);
}

/*********************************************************************
*
*	Function:		do_AD1836_spi_Open
*
*	Description:	Opens the spi device for use in configuring the
*                   AD1836
*
*********************************************************************/
static u32 SPI_Open(
	ADI_AD1836              *pAD1836,               // our 'parent' device driver
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

	// Open the SPI driver
	Result = adi_dev_Open(
					ManagerHandle,			// device manager handle
					&ADISPIDMAEntryPoint,	// SPI Entry point
					0,						// device number
					pAD1836,				// client handle - passed to internal callback function
					&pAD1836->spiHandle,	// pointer to DM handle (for SPI driver) location
					Direction,
					DMAHandle,				// handle to the DMA manager
					DCBHandle,				// handle to the callback manager
					spiCallbackFunction		// internal callback function
	);
	// return with appropriate code if SPI driver fails to open
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	Result = adi_dev_Control( pAD1836->spiHandle, ADI_DEV_CMD_TABLE, (void*)SPI_Cfg );
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	return (Result);
}

/*********************************************************************
*
*	Function:		do_AD1836_sport_Open
*
*	Description:	Opens the sport device for use with the
*                   AD1836 in I2S or TDM modes
*
*********************************************************************/
static u32 SPT_Open(
	ADI_AD1836              *pAD1836,               // our 'parent' device driver
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
	void *pExitCriticalArg;		// exit critical region parameter

	// Open the SPI driver
	Result = adi_dev_Open(
					ManagerHandle,			// device manager handle
					&ADISPORTEntryPoint,	// SPORT Entry point
					0,						// device number
					pAD1836,				// client handle - passed to internal callback function
					&pAD1836->sportHandle,	// pointer to DM handle (for SPORT driver) location
					Direction,				// data direction as input
					DMAHandle,				// handle to the DMA manager
					DCBHandle,				// handle to the callback manager
					sportCallbackFunction		// internal callback function
	);
	// return with appropriate code if SPI driver fails to open
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	return (Result);
}

/*********************************************************************
*
*	Function:		adi_pdd_Open
*
*	Description:	Opens the AD1836 device for use
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
	ADI_AD1836 *pAD1836;		// pointer to the AD1836 device we're working on
	void *pExitCriticalArg;		// exit critical region parameter

	// Check for a valid device number and that we're only doing input
#ifdef ADI_DEV_DEBUG
	if (DeviceNumber >= ADI_AD1836_NUM_DEVICES) return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
#endif

	// assign the pointer to the device instance
	pAD1836 = &Device[DeviceNumber];
	// and store the Device Manager handle
	pAD1836->DMHandle = DMHandle;
	// and callback function
	pAD1836->DMCallback = DMCallback;

	// Check that this device instance is not already in use. If not,
	// assign flag to indicate that it is now.
	// the device starts in powerup mode
	//pAD1836->CurMode = ADI_AD1836_MODE_NORMAL;
	pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
	if (pAD1836->InUseFlag == FALSE) {
		pAD1836->InUseFlag = TRUE;
		Result = ADI_DEV_RESULT_SUCCESS;
	}
	adi_int_ExitCriticalRegion(pExitCriticalArg);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Open the SPI driver 'child'
	Result = SPI_Open(
					pAD1836,                // pointer to device instance
					ManagerHandle,			// device manager handle
					0,						// device number
					DMHandle,				// client handle - passed to internal callback function
					&pAD1836->spiHandle,	// pointer to DM handle (for SPI driver) location
					ADI_DEV_DIRECTION_OUTBOUND,
					pEnterCriticalArg,
					DMAHandle,				// handle to the DMA manager
					DCBHandle,				// handle to the callback manager
					spiCallbackFunction		// internal callback function
	);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Open the SPORT driver 'child'
	Result = SPT_Open(
					pAD1836,                // pointer to device instance
					ManagerHandle,			// device manager handle
					0,						// device number
					pAD1836,				// client handle - passed to internal callback function
					&pAD1836->sportHandle,	// pointer to DM handle (for SPI driver) location
					Direction,				// data direction as input
					pEnterCriticalArg,
					DMAHandle,				// handle to the DMA manager
					DCBHandle,				// handle to the callback manager
					sportCallbackFunction	// internal callback function
	);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif


	// save the physical device handle in the client supplied location
	*pPDDHandle = (ADI_DEV_PDD_HANDLE *)pAD1836;

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
	ADI_AD1836 *pAD1836 = (ADI_AD1836 *)PDDHandle;

	// check for errors if required
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// close SPI driver
	Result = adi_dev_Close(pAD1836->spiHandle);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// close SPI driver
	Result = adi_dev_Close(pAD1836->sportHandle);
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// mark the device as closed
	pAD1836->InUseFlag = FALSE;

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
	ADI_AD1836 *pAD1836 = (ADI_AD1836 *)PDDHandle;
	Result = adi_dev_Read(pAD1836->sportHandle,BufferType,pBuffer);

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
	ADI_AD1836 *pAD1836 = (ADI_AD1836 *)PDDHandle;
	Result = adi_dev_Write(pAD1836->sportHandle,BufferType,pBuffer);

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

	ADI_AD1836 			*pAD1836;	// pointer to the device we're working on
	u32 				tmp;		// temporary storage
	u32 				Result;		// return value
	u32					u32Value;	// u32 type to avoid casts/warnings etc.
	u16					u16Value;	// u16 type to avoid casts/warnings etc.

	// avoid casts
	pAD1836 = (ADI_AD1836 *)PDDHandle; // Pointer to AD1836 device driver instance
    // assign 16 and 32 bit values for the Value argument
	u32Value = (u32)Value;
    u16Value = ((u16)((u32)Value));
	u16 functionmask = 0x3ff;

	// check for errors if required
#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// assume we're going to be successful
	Result = ADI_DEV_RESULT_SUCCESS;

	switch (Command)
	{
		case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
			*((u32 *)Value) = FALSE;
			break;

		case (ADI_AD1836_CMD_RESET):
			reset_AD1836();
			break;
			
		case (ADI_AD1836_CMD_SET_OPERATION_MODE_I2S):
		{
			pAD1836->OperationMode = ADI_AD1836_OPERATION_MODE_I2S;
			ADI_DEV_CMD_VALUE_PAIR I2S_Cfg[] = 
			{
				{ ADI_SPORT_CMD_SET_TCR1, 	    			(void *)(TFSR | LTFS | TCKFE)   },
				{ ADI_SPORT_CMD_SET_TCR2,       			(void *)(SLEN_24 | TSFSE) 		},
				{ ADI_SPORT_CMD_SET_RCR1, 	    			(void *)(RFSR | LRFS | RCKFE)   },
				{ ADI_SPORT_CMD_SET_RCR2,	    			(void *)(SLEN_24| RSFSE)        },
				{ ADI_DEV_CMD_END, 							(void*)0 						},
			};
			Result = adi_dev_Control(pAD1836->sportHandle, ADI_DEV_CMD_TABLE, I2S_Cfg);
			break;
		}
			
		case (ADI_AD1836_CMD_SET_OPERATION_MODE_TDM):
		{
			pAD1836->OperationMode = ADI_AD1836_OPERATION_MODE_TDM;
			ADI_DEV_CMD_VALUE_PAIR TDM_Cfg[] = 
			{
				{ ADI_SPORT_CMD_SET_TCR1, 	    	(void *)(TFSR)   	},
				{ ADI_SPORT_CMD_SET_TCR2,       	(void *)(SLEN_32) 	},
				{ ADI_SPORT_CMD_SET_RCR1, 	    	(void *)(RFSR)   	},
				{ ADI_SPORT_CMD_SET_RCR2,	    	(void *)(SLEN_32)   },
				{ ADI_SPORT_CMD_SET_MCMC1,	    	(void *)(0x0000)   	},
				{ ADI_SPORT_CMD_SET_MCMC2,	   		(void *)(0x101c)   	},
				{ ADI_SPORT_CMD_SET_MTCS0,	    	(void *)(0x000000FF)},
				{ ADI_SPORT_CMD_SET_MRCS0,	    	(void *)(0x000000FF)},
				{ ADI_DEV_CMD_END, 					(void*)0 			},
			};
			Result = adi_dev_Control(pAD1836->sportHandle, ADI_DEV_CMD_TABLE, TDM_Cfg);
			break;
		}
			
		case (ADI_AD1836_CMD_SET_AD1836_DAC_CONTROL_1):
		{
			pAD1836->DACControl1 = DAC_CONTROL_1 | (u16Value & functionmask);
			Result = Set_AD1836_Reg(PDDHandle, &pAD1836->DACControl1, 1);
			break;
		}
		case (ADI_AD1836_CMD_SET_AD1836_DAC_CONTROL_2):
		{
			pAD1836->DACControl2 = DAC_CONTROL_2 | (u16Value & functionmask);
			Result = Set_AD1836_Reg(PDDHandle, &pAD1836->DACControl2, 1);
			break;
		}
		case (ADI_AD1836_CMD_SET_AD1836_DAC_VOLUME_0):
		{
			pAD1836->DACVolume0 = DAC_VOLUME_0 | (u16Value & functionmask);
			Result = Set_AD1836_Reg(PDDHandle, &pAD1836->DACVolume0, 1);
			break;
		}
		case (ADI_AD1836_CMD_SET_AD1836_DAC_VOLUME_1):
		{
			pAD1836->DACVolume1 = DAC_VOLUME_1 | (u16Value & functionmask);
			Result = Set_AD1836_Reg(PDDHandle, &pAD1836->DACVolume1, 1);
			break;
		}
		case (ADI_AD1836_CMD_SET_AD1836_DAC_VOLUME_2):
		{
			pAD1836->DACVolume2 = DAC_VOLUME_2 | (u16Value & functionmask);
			Result = Set_AD1836_Reg(PDDHandle, &pAD1836->DACVolume2, 1);
			break;
		}
		case (ADI_AD1836_CMD_SET_AD1836_DAC_VOLUME_3):
		{
			pAD1836->DACVolume3 = DAC_VOLUME_3 | (u16Value & functionmask);
			Result = Set_AD1836_Reg(PDDHandle, &pAD1836->DACVolume3, 1);
			break;
		}
		case (ADI_AD1836_CMD_SET_AD1836_DAC_VOLUME_4):
		{
			pAD1836->DACVolume4 = DAC_VOLUME_4 | (u16Value & functionmask);
			Result = Set_AD1836_Reg(PDDHandle, &pAD1836->DACVolume4, 1);
			break;
		}
		case (ADI_AD1836_CMD_SET_AD1836_DAC_VOLUME_5):
		{
			pAD1836->DACVolume5 = DAC_VOLUME_5 | (u16Value & functionmask);
			Result = Set_AD1836_Reg(PDDHandle, &pAD1836->DACVolume5, 1);
			break;
		}
		case (ADI_AD1836_CMD_SET_AD1836_ADC_CONTROL_1):
		{
			pAD1836->ADCControl1 = ADC_CONTROL_1 | (u16Value & functionmask);
			Result = Set_AD1836_Reg(PDDHandle, &pAD1836->ADCControl1, 1);
			break;
		}
		case (ADI_AD1836_CMD_SET_AD1836_ADC_CONTROL_2):
		{
			pAD1836->ADCControl2 = ADC_CONTROL_2 | (u16Value & functionmask);
			Result = Set_AD1836_Reg(PDDHandle, &pAD1836->ADCControl2, 1);
			break;
		}
		case (ADI_AD1836_CMD_SET_AD1836_ADC_CONTROL_3):
		{
			pAD1836->ADCControl3 = ADC_CONTROL_3 | (u16Value & functionmask);
			Result = Set_AD1836_Reg(PDDHandle, &pAD1836->ADCControl3, 1);
			break;
		}
		
		case (ADI_AD1836_CMD_SET_AD1836_SPI_SLAVE_SELECT):
		{
			pAD1836->SlaveSelectFlag = u32Value;
			Result = adi_dev_Control(pAD1836->spiHandle, ADI_SPI_CMD_ENABLE_SLAVE_SELECT, (void*)pAD1836->SlaveSelectFlag);
			Result = adi_dev_Control(pAD1836->spiHandle, ADI_SPI_CMD_SELECT_SLAVE, (void*)pAD1836->SlaveSelectFlag);
            break;
		}
		default:
			// pass anything we don't specifically handle to the SPT driver
			Result = adi_dev_Control(pAD1836->sportHandle, Command, Value);
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
	ADI_AD1836 *pAD1836 = (ADI_AD1836 *)DeviceHandle;
#if defined(ADI_DEV_DEBUG)
	if (ValidatePDDHandle(DeviceHandle) != ADI_DEV_RESULT_SUCCESS) {  	// verify the DM handle
		return;
	}
#endif
	// simply pass the callback along to the Device Manager Callback without interpretation
	//(pAD1836->DMCallback)(pAD1836->DMHandle,Event,pArg);
	
	spiSemaphore = 1;
}

/*********************************************************************

	Function:		sportCallbackFunction

	Description:	Fields the callback from the SPORT Driver

*********************************************************************/

static void sportCallbackFunction(void* DeviceHandle, u32 Event, void* pArg)
{
	ADI_AD1836 *pAD1836 = (ADI_AD1836 *)DeviceHandle;
#if defined(ADI_DEV_DEBUG)
	if (ValidatePDDHandle(DeviceHandle) != ADI_DEV_RESULT_SUCCESS) {  	// verify the DM handle
		return;
	}
#endif
	// simply pass the callback along to the Device Manager Callback without interpretation
	(pAD1836->DMCallback)(pAD1836->DMHandle,Event,pArg);
}

#if defined(ADI_DEV_DEBUG)

/*********************************************************************

	Function:		ValidatePDDHandle

	Description:	Validates a PDD handle

*********************************************************************/

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle) {
	unsigned int i;
	for (i = 0; i < ADI_AD1836_NUM_DEVICES; i++) {
		if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i]) {
			return (ADI_DEV_RESULT_SUCCESS);
		}
	}
	return (ADI_DEV_RESULT_BAD_PDD_HANDLE);
}
#endif

/*********************************************************************

Entry point for device manager

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT ADIAD1836EntryPoint = {
	adi_pdd_Open,
	adi_pdd_Close,
	adi_pdd_Read,
	adi_pdd_Write,
	adi_pdd_Control
};




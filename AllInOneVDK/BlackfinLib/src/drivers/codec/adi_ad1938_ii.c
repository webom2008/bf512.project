/*****************************************************************************

Copyright (c) 2005 Analog Devices.  All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  
******************************************************************************

$File: adi_ad1938_ii.c $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Project:    AD1938 device driver (Mark 2)
Title:      AD1938 driver source code
Author(s):  bmk
Revised by: bmk

Description:
            This is the driver source code for the AD1938 Audio Codec. 
            
            This driver is layered on top of the SPI and SPORT device drivers, 
            which are configured for the specific use of the AD1938 peripheral. 
            The driver uses device access service to access AD1938 hardware registers
           
            The driver senses any change in AD1938 device settings (usually 
            carried out by the application) and automatically updates the 
            corresponding SPORT device configuration to support present 
            AD1938 mode (auto-SPORT config).This driver supports auto-SPORT config
            only when AD1938 is operated in I2S and TDM coupled (Aux) modes.

Note:		Whenever the application sets AD1938 in stereo mode, this driver will
			configure the corresponding SPORT deivce to I2S mode, regardless of the 
			stereo mode selected by the application.
			
			This driver DOES NOT support auto-SPORT configuration to AD1938 serial modes 
			such as LJ,RJ,DSP,Daisy chain and Dual line modes.
            Any application intend to use these mode can configure the SPORT device 
            to any of the above modes by issuing SPORT driver specific commands with 
            AD1938 driver handle.

            Whenever the application switches from TDM-Coupled Aux to I2S mode, 
			application should first change AD1938 ADC serial mode before 
			modifying its DAC serial mode. The reason is, when AD1938 ADC is set in 
			TDM-Coupled Aux, the driver bypasses DAC serial mode settings and 
			any modifications to DAC serial mode will be ignored by the auto-SPORT
			config module of this driver.
				
References:
******************************************************************************

Modification History:
====================
$Log: adi_ad1938_ii.c,v $
Revision 1.6  2007/03/02 21:26:43  bgiese
snapshot from VDSP 4.5

Revision 1.10  2006/11/17 03:59:42  bmk
Added workaround to address BF Audio extender - SPI hardware anomaly

Revision 1.9  2006/09/05 01:25:34  bmk
added ADI_AD1938_CMD_ENABLE_AUTO_SPORT_CONFIG command to Enable/Disable Auto-SPORT configuration mode
Auto-SPORT configuration mode is enabled by default

Revision 1.8  2006/08/16 04:15:36  bmk
Supports auto-SPORT configuration for I2S & TDM-coupled Aux modes
******************************************************************************

Include files

*****************************************************************************/

#include <services/services.h>							// system service includes
#include <drivers/adi_dev.h>							// device manager includes
#include <drivers/sport/adi_sport.h>			     	// SPORT driver includes
#include <drivers/deviceaccess/adi_device_access.h>  	// adi_device_access driver includes
#include <drivers/codec/adi_ad1938_ii.h>    			// AD1938 driver includes

/*********************************************************************

Enumerations and defines

*********************************************************************/
// number of AD1938 devices in the system
#define ADI_AD1938_NUM_DEVICES 		(sizeof(Device)/sizeof(ADI_AD1938)) 

// Address of the last register in AD1938
#define AD1938_FINAL_REG_ADDRESS	0x10    

// SPORT word length (value should be actual wordlength - 1)
#define SLEN_16						(15)	// 16 bits
#define SLEN_20						(19)	// 20 bits
#define SLEN_24						(23)	// 24 bits
#define SLEN_32						(31)	// 32 bits

// AD1938 Stereo modes
// this driver supports auto-SPORT config only for I2S(stereo) and TDM Aux modes
#define	AD1938_SERIAL_MODE_STEREO				0
#define	AD1938_SERIAL_MODE_TDM_DAISY_CHAIN		1
#define	AD1938_SERIAL_MODE_TDM_AUX				2
#define	AD1938_SERIAL_MODE_DUAL_LINE_TDM		3

// SPORT Config values for supported serial modes
// Stereo Mode (auto SPORT config assumes it as I2S mode)
#define	I2S_SPORT_TCR1				(TFSR  | TCKFE)
#define	I2S_SPORT_TCR2				(TSFSE | TXSE)
#define	I2S_SPORT_RCR1				(RFSR  | RCKFE)
#define	I2S_SPORT_RCR2				(RSFSE | RXSE)
// common for Stereo modes
#define	STEREO_MCMC2				(0)		// Multichannel disabled

// For TDM-coupled (Aux) mode

// Multichannel - Channel select values
#define	TDM_AUX_SPORT_MCN_2			0x0003	// 2 channels enabled
#define	TDM_AUX_SPORT_MCN_4			0x000F	// 4 channels enabled
#define	TDM_AUX_SPORT_MCN_8			0x00FF	// 8 channels enabled
#define	TDM_AUX_SPORT_MCN_16		0xFFFF	// 16 channels enabled

#define	TDM_AUX_SPORT_TCR1			(TFSR)
#define	TDM_AUX_SPORT_TCR2			(SLEN_32)	// Secondary disabled, always SPORT wordlength is 32bits for TDM
#define	TDM_AUX_SPORT_RCR1			(RFSR)
#define	TDM_AUX_SPORT_RCR2			(SLEN_32)	// Secondary disabled, always SPORT wordlength is 32bits for TDM
#define	TDM_AUX_SPORT_MCMC1_8		0		// SPORT Tx/Rx window size is 8 (when AD1938 ADC multichannels count <= 8)
#define	TDM_AUX_SPORT_MCMC1_16		0x1000	// SPORT Tx/Rx window size is 16(when AD1938 ADC multichannels count is 16)
#define	TDM_AUX_SPORT_MCMC2			(MCMEN | MCDTXPE | MCDRXPE)	// Multichannel enabled,DMA Tx/Rx packing enabled, no frame delay

// SPORT device configuration commands - bitwise location in SPORT_ConfigTable
#define	SPORT_TCR1					0x0001
#define	SPORT_TCR2					0x0002
#define	SPORT_RCR1					0x0004
#define	SPORT_RCR2					0x0008
#define	SPORT_MCMC1					0x0010
#define	SPORT_MCMC2					0x0020
#define	SPORT_MTCS0					0x0040
#define	SPORT_MRCS0					0x0080

// SPORT device register cache for auto SPORT configuration
typedef struct {
    u16		TCR1;
    u16		TCR2;
    u16		RCR1;
    u16		RCR2;
    u16		MCMC1;
    u16		MCMC2;
    u16		MCS0;
}	ADI_AD1938_SPORT_REGS;

// Structure to contain data for an instance of the AD1938 device driver
typedef struct {
    ADI_DEV_MANAGER_HANDLE      ManagerHandle;      // Manager Handle       
    ADI_DEV_DEVICE_HANDLE       DMHandle;           // Handle of Device Manager instance
    ADI_DMA_MANAGER_HANDLE      DMAHandle;          // handle to the DMA manager
    ADI_DCB_HANDLE              DCBHandle;          // callback handle    
    ADI_DCB_CALLBACK_FN         DMCallback;         // the callback function supplied by the Device Manager    
    ADI_DEV_PDD_HANDLE          sportHandle;        // Handle to the underlying SPORT device driver
    ADI_DEV_DIRECTION           sportDirection;     // SPORT data direction	
    u8                          sportDeviceNumber;  // SPORT Device number used for AD1938 audio dataflow	    
    u8                          ad1938_spi_cs;     	// SPI Chipselect for blackfin (SPI_FLG location to select AD1938)
    u32							spiDeviceAddress;	// SPI Device address for AD1938
	u8							InUseFlag;			// Device in use flag
	u8							DataflowStatus;		// Dataflow status flag (Data flow ON/OFF)
    ADI_AD1938_SPORT_REGS		Sport;				// SPORT configuration settings for AD1938
	u8							SerialModeADC;		// Present Serial mode of AD1938 ADC
	u8							SerialModeDAC;		// Present Serial mode of AD1938 DAC
	u16							AD1938DACwlen;		// Present DAC word length of AD1938
	u16							AD1938ADCwlen;		// Present ADC word length of AD1938
	u16							SportCmdSelect;		// Indicates list of sport registers to be configured	    
	u8							AutoSportEnable;	// Enable/Disable auto-SPORT configuration mode
} ADI_AD1938;

// The initial values for the device instance
static ADI_AD1938 Device[] = {
    {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        ADI_DEV_DIRECTION_BIDIRECTIONAL,    // SPORT data direction        
        0,          // SPORT 0 as default      
        0,          // no SPI CS by default
        0x04,		// default SPI device address for AD1938
        FALSE,		// device not in use
        FALSE,		// dataflow off
		{	// AD1938A in I2S after powerup - Init sport config settings to I2S as default
			I2S_SPORT_TCR1,	
			(I2S_SPORT_TCR2 | SLEN_24),	// 24 bit wordlength
			I2S_SPORT_RCR1,
			(I2S_SPORT_RCR2 | SLEN_24),
			TDM_AUX_SPORT_MCMC1_8,		// MCMC1 - don't care 
			STEREO_MCMC2,				// MCMC2 - disable Multichannel
			TDM_AUX_SPORT_MCN_2,		// MCS0 - 2 Tx/Rx channels by default
		},
		AD1938_SERIAL_MODE_STEREO,	// AD1938 ADC in Stereo (I2S) mode after powerup/reset
		AD1938_SERIAL_MODE_STEREO,	// AD1938 DAC in Stereo (I2S) mode after powerup/reset
		SLEN_24,					// default AD1938 DAC word length is 24 bits
		SLEN_24,					// default AD1938 ADC word length is 24 bits
		0,							// SPORT device is closed (not configured) by default  
		TRUE,						// Auto-SPORT configuration mode enabled by default      
    },
};

/*********************************************************************

Static functions

*********************************************************************/

static u32 adi_pdd_Open(                            // Open a device
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,          // device manager handle
    u32                     DeviceNumber,           // device number
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,           // device handle
    ADI_DEV_PDD_HANDLE      *pPDDHandle,            // pointer to PDD handle location 
    ADI_DEV_DIRECTION       Direction,              // data direction
    void                    *pCriticalRegionArg,    // critical region imask storage location
    ADI_DMA_MANAGER_HANDLE  DMAHandle,              // handle to the DMA manager
    ADI_DCB_HANDLE          DCBHandle,              // callback handle
    ADI_DCB_CALLBACK_FN     DMCallback              // device manager callback function
);

static u32 adi_pdd_Close(                           // Closes a device
    ADI_DEV_PDD_HANDLE      PDDHandle               // PDD handle
);

static u32 adi_pdd_Read(                            // Reads data or queues an inbound buffer to a device
    ADI_DEV_PDD_HANDLE      PDDHandle,              // PDD handle
    ADI_DEV_BUFFER_TYPE     BufferType,             // buffer type
    ADI_DEV_BUFFER          *pBuffer                // pointer to buffer
);
    
static u32 adi_pdd_Write(                           // Writes data or queues an outbound buffer to a device
    ADI_DEV_PDD_HANDLE      PDDHandle,              // PDD handle
    ADI_DEV_BUFFER_TYPE     BufferType,             // buffer type
    ADI_DEV_BUFFER          *pBuffer                // pointer to buffer
);

static u32 adi_pdd_SequentialIO(                    
    ADI_DEV_PDD_HANDLE      PDDHandle,              // PDD handle
    ADI_DEV_BUFFER_TYPE     BufferType,             // buffer type
    ADI_DEV_BUFFER          *pBuffer                // pointer to buffer
);
    
static u32 adi_pdd_Control(                         // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE      PDDHandle,              // PDD handle
    u32                     Command,                // command ID
    void                    *Value                  // command specific value
);

static u32 SPORT_Open( 
    ADI_AD1938 				*pAD1938				// pointer to the driver instance
);

static u32 SPORT_Config(
    ADI_AD1938 				*pAD1938				// pointer to the driver instance
);

static u32 SPORT_Close( 
    ADI_AD1938 				*pAD1938				// pointer to the driver instance
);

// Function specific to AD1938 driver passed to Device Access Service
static u32 DevAccessFN(
    void   					*Handle,   		// AD1938 driver Argument passed back by the device access service
    u16  					*RegAddr, 		// Register address selected to access by the application
    u16  					RegVal,			// Above device register will be configured to this  value
    ADI_DEVICE_ACCESS_MODE	access_mode		// present access mode of AD1938
);

// the callback function passed to the SPORT driver
static void sportCallbackFunction(
    void*   DeviceHandle, 
    u32     Event, 
    void*   pArg
);

/*********************************************************************

Debug Mode functions (debug build only)

*********************************************************************/
#if defined(ADI_DEV_DEBUG)

// Validate PDD handle
static int ValidatePDDHandle(
 	ADI_DEV_PDD_HANDLE 	PDDHandle
);

#endif

/*********************************************************************

Global Registers

*********************************************************************/

/*********************************************************************
Table for Register Field Error check and Register field access
Table structure - 	'Count' of Register addresses containing individual fields
					Register address containing individual fields, 
					Register field locations in the corresponding register, 
					Reserved bit locations in the corresponding register
Register field location - Bit indicating start of new field in a register will be 1
Reserved bit locations will be 1
*********************************************************************/

// AD1938 register address to perform Register Field Error check and Register field access
static u16 ad1938RegAddr1[]	= {	AD1938_PLL_CLK_CTRL0, AD1938_PLL_CLK_CTRL1, AD1938_DAC_CTRL0, 
								AD1938_DAC_CTRL1, AD1938_DAC_CTRL2, AD1938_DAC_CHNL_MUTE, 
								AD1938_ADC_CTRL0, AD1938_ADC_CTRL1, AD1938_ADC_CTRL2	};

// Register Field start locations corresponding to the entries in ad1938RegAddr1 (Reserved Bit locations marked as 1)
static u16 ad1938RegField[]	= {	0xAB, 0xFF, 0x4B, 
								0xFB, 0xEB, 0xFF, 
								0x7F, 0xA5, 0xDF	};

static ADI_DEVICE_ACCESS_REGISTER_FIELD RegisterField[] = {
	sizeof(ad1938RegAddr1)/2,	// 'Count' of Register addresses containing individual fields
	ad1938RegAddr1,			// array of AD1938 register addresses containing individual fields
	ad1938RegField,			// array of register field locations in the corresponding registers
};

/*********************************************************************
Table to configure the reserved bits in the device to its recommended values
Table structure -  'Count' of Register addresses containing Reserved Locations
     Register address containing Reserved Locations
     Recommended value for the Reserved Locations
*********************************************************************/

// AD1938 register address having reserved locations
static u16 ad1938RegAddr2[]			= {	AD1938_PLL_CLK_CTRL1, AD1938_DAC_CTRL2	};

// Reserved Bit locations corresponding to the entries in ad1938RegAddr2
static u16 ad1938ReservedBits[]		= { 0xF0, 0xC0	};

// Value of Reserved Bit locations corresponding to the entries in ad1938RegAddr2
static u16 ad1938ReservedValues[]	= { 0x00, 0x00	};
																		
static ADI_DEVICE_ACCESS_RESERVED_VALUES ReservedValues[] = {
	sizeof(ad1938RegAddr2)/2, 	// 'Count' of Register addresses containing Reserved Locations
	ad1938RegAddr2,				// array of AD1938 register addresses containing individual fields
 	ad1938ReservedBits,			// array of reserved bit locations in the corresponding register	
	ad1938ReservedValues,		// array of Recommended values for the Reserved Bit locations
};

/*********************************************************************
Table for AD1938 Register Error check for invalid & read-only register(s) access
Structure - 'Count' of invalid Register addresses in AD1938
			Invalid Register addresses in AD1938
			'Count' of Read-only Register addresses in AD1938
			Read-only Register addresses in AD1938
*********************************************************************/
   
// No read-only registers addresses are listed in AD1938 HRM
// No invalid registers addresses are in AD1938 HRM

/*********************************************************************
*
*   Function:       SPORT_Open
*
*   Description:    Opens the sport device for AD1938 audio dataflow
*
*********************************************************************/
static u32 SPORT_Open( 
    ADI_AD1938 				*pAD1938				// pointer to the driver instance
) {
    
    // default return code
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    // Open the SPORT driver
    Result = adi_dev_Open( 
        pAD1938->ManagerHandle,        // device manager handle
        &ADISPORTEntryPoint,           // SPORT Entry point
        pAD1938->sportDeviceNumber,    // SPORT device number
        pAD1938,                       // client handle - passed to internal callback function
        &pAD1938->sportHandle,         // pointer to DM handle (for SPORT driver) location
        pAD1938->sportDirection,       // SPORT data direcion
        pAD1938->DMAHandle,            // handle to the DMA manager
        pAD1938->DCBHandle,            // handle to the callback manager
        sportCallbackFunction          // internal callback function
    );

// return with appropriate code if SPORT driver fails to open
#ifdef ADI_DEV_DEBUG
    if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// if yes, configure SPORT device to present AD1938 device settings.
	// clear SPORT command select reg
	pAD1938->SportCmdSelect = 0;

	// The driver supports auto sport-config only for I2S and TDM-coupled Aux modes

    // Check the present serial mode ID
    // SPORT Rx (AD1938 ADC) config
    // TDM-Copuled AUX will bypass Sport Tx (DAC) configuration settings
    if (pAD1938->SerialModeADC == AD1938_SERIAL_MODE_TDM_AUX)
    	// for TDM-Copuled Aux, pass all Sport config commands in SPORT_ConfigTable
        pAD1938->SportCmdSelect 	= ( SPORT_TCR1 | SPORT_TCR2 | SPORT_RCR1 | SPORT_RCR2 | SPORT_MCMC1 | 
        								SPORT_MCMC2 | SPORT_MRCS0 | SPORT_MTCS0);
	// check if this driver supports auto-sport config for this serial mode
	else if (pAD1938->SerialModeADC == AD1938_SERIAL_MODE_TDM_AUX)
    	// Configure SPORT device in relevance to present values in AD1938 register cache
    	// Configure Sport Rx & Multichannel config2 registers
    	pAD1938->SportCmdSelect = (SPORT_RCR1 | SPORT_RCR2 | SPORT_MCMC2);

	// for SPORT Tx (AD1938 DAC config)
	// check if Ad1938 ADC is not in TDM-Copuled AUX
    if (pAD1938->SerialModeADC != AD1938_SERIAL_MODE_TDM_AUX)
    {
        // check if this driver supports auto-sport config for this serial mode
        if (pAD1938->SerialModeDAC == AD1938_SERIAL_MODE_TDM_AUX)
		{
	    	// Configure SPORT device in relevance to present values in AD1938 register cache
    		// Configure Sport Tx & Multichannel config2 registers
    		pAD1938->SportCmdSelect |= (SPORT_TCR1 | SPORT_TCR2 | SPORT_MCMC2);
		}
    }
       
	// call SPORT device configuration routine
    Result = SPORT_Config(pAD1938);
    
    return (Result);
}

/*********************************************************************
*
*   Function:       SPORT_Config
*
*   Description:    Configures SPORT device relative to AD1938 settings
*	Note:			Only marked commands will be issued to SPORT device
*					i.e., Command IDs corresponding to the bit location 
*					in SportCmdSelect which are SET to 1 will be passed
*					to the SPORT device driver 
*
*********************************************************************/
static u32 SPORT_Config( 
	ADI_AD1938 				*pAD1938	// pointer to the driver instance
){
 
    // default return code
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    u16	i = 0;	// index

	// SPORT Configuration Table
	ADI_DEV_CMD_VALUE_PAIR SPORT_ConfigTable[] = 
	{
		{	ADI_SPORT_CMD_SET_TCR1, 	(void *)(pAD1938->Sport.TCR1)	},
		{ 	ADI_SPORT_CMD_SET_TCR2,     (void *)(pAD1938->Sport.TCR2)	},
		{ 	ADI_SPORT_CMD_SET_RCR1, 	(void *)(pAD1938->Sport.RCR1)	},
		{ 	ADI_SPORT_CMD_SET_RCR2,	    (void *)(pAD1938->Sport.RCR2)	},	
		{ 	ADI_SPORT_CMD_SET_MCMC1,	(void *)(pAD1938->Sport.MCMC1)	},
		{ 	ADI_SPORT_CMD_SET_MCMC2,	(void *)(pAD1938->Sport.MCMC2)	},
		{ 	ADI_SPORT_CMD_SET_MTCS0,	(void *)(pAD1938->Sport.MCS0)	},
		{ 	ADI_SPORT_CMD_SET_MRCS0,	(void *)(pAD1938->Sport.MCS0)	},
		{ 	ADI_DEV_CMD_END, 			(void *)NULL					},
	};

	// is SPORT device open already?, Is there a need to configure SPORT device?
	// perform auto-SPORT configuration?
	if ((pAD1938->AutoSportEnable) && (pAD1938->sportHandle != NULL) && (pAD1938->SportCmdSelect != 0))
	{
	    // Configure SPORT only when auto-SPORT configuration enabled
		// if SPORT device is open, check for dataflow status
		// if dataflow is on, switch dataflow off before updating SPORT device configuration
		if (pAD1938->DataflowStatus == TRUE)
			Result	= adi_dev_Control( pAD1938->sportHandle, ADI_DEV_CMD_SET_DATAFLOW,	(void*)FALSE );

#ifdef ADI_DEV_DEBUG
    	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);  
#endif
			
		// parse through all commands in SPORT_ConfigTable until ADI_DEV_CMD_END is reached
		while (SPORT_ConfigTable[i].CommandID != ADI_DEV_CMD_END)
		{
		    // check if the command id corresponding to the present index value 
		    // is marked in 'SportCmdSelect' register
			if (pAD1938->SportCmdSelect & (1 << i))
		    {
		        // If a bit location is set in SportCmdSelect, pass the corresponding command to SPORT
    			Result = adi_dev_Control( pAD1938->sportHandle, SPORT_ConfigTable[i].CommandID, SPORT_ConfigTable[i].Value);		        
		    }
		    
#ifdef ADI_DEV_DEBUG
    	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);  
#endif
		    i++;	// increment the index and continue until ADI_DEV_CMD_END is reached
		}
		
		// if dataflow was previously on, switch the dataflow on again
		if (pAD1938->DataflowStatus == TRUE)
			Result	= adi_dev_Control( pAD1938->sportHandle, ADI_DEV_CMD_SET_DATAFLOW,	(void*)TRUE );
	}
	
	return (Result);
}

/*********************************************************************
*
*   Function:       SPORT_Close
*
*   Description:    Closes the sport device used for AD1938 audio dataflow
*
*********************************************************************/
static u32 SPORT_Close( 
    ADI_AD1938 			*pAD1938	// pointer to the driver instance
) {
    
    // default return code
    u32 Result = ADI_DEV_RESULT_SUCCESS;
					
	// Check if any sport device is open
	// if so, close the present sport device in use
 	if (pAD1938->sportHandle)
 	{
    	// close the present SPORT device
    	if ((Result = adi_dev_Close(pAD1938->sportHandle))!= ADI_DEV_RESULT_SUCCESS)
    		return (Result);
			// Mark SPORT Handle as NULL indicating SPORT device is closed
			pAD1938->sportHandle = NULL;        			
    }

    return (Result);
}

/*********************************************************************
*
* Function:  adi_pdd_Open
*
* Description: Opens the AD1938 device for use
*
*********************************************************************/
static u32 adi_pdd_Open(                            // Open a device
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,          // device manager handle
    u32                     DeviceNumber,           // device number
    ADI_DEV_DEVICE_HANDLE   DMHandle,               // device handle
    ADI_DEV_PDD_HANDLE      *pPDDHandle,            // pointer to PDD handle location 
    ADI_DEV_DIRECTION       Direction,              // data direction
    void                    *pEnterCriticalArg,     // enter critical region parameter
    ADI_DMA_MANAGER_HANDLE  DMAHandle,              // handle to the DMA manager
    ADI_DCB_HANDLE          DCBHandle,              // callback handle
    ADI_DCB_CALLBACK_FN     DMCallback              // device manager callback function
) {
 
    u32 Result = ADI_DEV_RESULT_SUCCESS;    // default return code
    ADI_AD1938  *pAD1938;     // pointer to the AD1938 device we're working on
    void *pExitCriticalArg;     // exit critical region parameter
  
    // Check for a valid device number
#ifdef ADI_DEV_DEBUG
    if (DeviceNumber >= ADI_AD1938_NUM_DEVICES) return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
#endif

    // assign the pointer to the device instance
    pAD1938 = &Device[DeviceNumber];
    //  store the Manager handle
    pAD1938->ManagerHandle = ManagerHandle;
    //  store the Device Manager handle
    pAD1938->DMHandle = DMHandle;
    //  store the DMA Manager handle
    pAD1938->DMAHandle = DMAHandle;
    //  store the Deffered Callback Manager handle
    pAD1938->DCBHandle = DCBHandle;
    //  callback function
    pAD1938->DMCallback = DMCallback;
    // data direction
    pAD1938->sportDirection = Direction;
	
    // insure the device the client wants is available    
    Result = ADI_DEV_RESULT_DEVICE_IN_USE;   
    
    // Check that this device instance is not already in use. 
    // If not, assign flag to indicate that it is now.
    pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
    if (pAD1938->InUseFlag == FALSE) 
    {
        pAD1938->InUseFlag = TRUE;
        Result = ADI_DEV_RESULT_SUCCESS;
    }
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
* Function:  adi_pdd_Close
*
* Description: Closes down AD1938 and SPORT device 
*
*********************************************************************/

static u32 adi_pdd_Close(                           // Closes a device
    ADI_DEV_PDD_HANDLE      PDDHandle               // PDD handle
)   {

    // default return code
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    //	Pointer	to AD1938 device driver instance
	ADI_AD1938 *pAD1938 = (ADI_AD1938 *)PDDHandle;

 // check for errors if required
#ifdef ADI_DEV_DEBUG
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Close the present SPORT device being used
	Result = SPORT_Close(PDDHandle);

    // mark the device as closed
    pAD1938->InUseFlag = FALSE;

    return(Result);
}

/*********************************************************************
*
* Function:  adi_pdd_Read
*
* Description: passes read to SPORT device (SPORT uses DMA)
*
*********************************************************************/

static u32 adi_pdd_Read(                            // Reads data or queues an inbound buffer to a device
    ADI_DEV_PDD_HANDLE      PDDHandle,              // PDD handle
    ADI_DEV_BUFFER_TYPE     BufferType,             // buffer type
    ADI_DEV_BUFFER          *pBuffer                // pointer to buffer
) {
    ADI_AD1938  *pAD1938 = (ADI_AD1938 *)PDDHandle;  

#ifdef ADI_DEV_DEBUG
    u32 Result = ADI_DEV_RESULT_SUCCESS;   	// return value
 	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result); 
#endif

   	// pass read operation to SPORT
   	return(adi_dev_Read(pAD1938->sportHandle,BufferType,pBuffer));

}

/*********************************************************************
*
* Function:  adi_pdd_Write
*
* Description: passes write to SPORT device (SPORT uses DMA)
*
*********************************************************************/
 
static u32 adi_pdd_Write(                           // Writes data or queues an outbound buffer to a device
    ADI_DEV_PDD_HANDLE      PDDHandle,              // PDD handle
    ADI_DEV_BUFFER_TYPE     BufferType,             // buffer type
    ADI_DEV_BUFFER          *pBuffer                // pointer to buffer
) {
    //	Pointer	to AD1938 device driver instance
	ADI_AD1938 *pAD1938 = (ADI_AD1938 *)PDDHandle;    
    
#ifdef ADI_DEV_DEBUG
    u32 Result = ADI_DEV_RESULT_SUCCESS;   	// return value
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

   	// pass write operation to SPORT
   	return(adi_dev_Write(pAD1938->sportHandle,BufferType,pBuffer));
    	
}

/*********************************************************************
*
* Function:  adi_pdd_SequentialIO
*
* Description: Function not supported by this driver
*
*********************************************************************/

static u32 adi_pdd_SequentialIO(                    
    ADI_DEV_PDD_HANDLE      PDDHandle,              // PDD handle
    ADI_DEV_BUFFER_TYPE     BufferType,             // buffer type
    ADI_DEV_BUFFER          *pBuffer                // pointer to buffer
) {

    // Function not supported
    return(ADI_DEV_RESULT_NOT_SUPPORTED);   
}

/*********************************************************************
*
* Function:  adi_pdd_Control
*
* Description: AD1938 command implementation
*
*********************************************************************/
    
static u32 adi_pdd_Control(                         // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE      PDDHandle,              // PDD handle
    u32                     Command,                // command ID
    void                    *Value                  // command specific value
) {

    ADI_AD1938    	*pAD1938; 	// pointer to the device we're working on
    u32     		u32Value;  	// u32 type to avoid casts/warnings etc.
    u8      		u8Value;    // u8 type to avoid casts/warnings etc.  

    u32 Result = ADI_DEV_RESULT_SUCCESS;   	// return value
        
    // avoid casts
    pAD1938 = (ADI_AD1938 *)PDDHandle; // Pointer to AD1938 device driver instance
    // assign 8, 16 and 32 bit values for the Value argument
    u32Value = (u32) Value;
    u8Value  = (u8) u32Value;

    // Location to hold SPI dummy read
    ADI_DEV_ACCESS_REGISTER		DummyRead;        
	// Structure passed to device access service
	ADI_DEVICE_ACCESS_REGISTERS		access_device; 	
	// Structure passed to select Device type and access type
	ADI_DEVICE_ACCESS_SELECT	SelectSPIAccess[] = 
					{	pAD1938->ad1938_spi_cs,				// SPI Chip-select
 						ADI_DEVICE_ACCESS_LENGTH1,			// AD1938 Global address length (1 byte)
 						ADI_DEVICE_ACCESS_LENGTH1,			// AD1938 register address length (1 byte)
 						ADI_DEVICE_ACCESS_LENGTH1,			// AD1938 register data length (1 byte)
 						ADI_DEVICE_ACCESS_TYPE_SPI,			// Select SPI access
					};
      
    // check for errors if required
#ifdef ADI_DEV_DEBUG
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    switch (Command)    
    {

    	// CASE (control dataflow)
    	case (ADI_DEV_CMD_SET_DATAFLOW):

     		// check if the SPORT device is already open
     		if (pAD1938->sportHandle != NULL)
     		{
     			// if so, Enable/Disable the SPORT device used by AD1938
     			Result = adi_dev_Control( pAD1938->sportHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)u8Value );
			}
			// else check if the client is trying to enable dataflow without opening the SPORT
			else if (u8Value == TRUE)
			{
				// can not pass this value to SPORT as no valid SPORT handle is available
				// which also means the client hasn't defined the dataflow method yet, return error.
 				return (ADI_DEV_RESULT_DATAFLOW_UNDEFINED);
 			}
 			
         	pAD1938->DataflowStatus = u8Value;	// Save the dataflow status
         		
        	break;

	    // CASE (query for processor DMA support)
    	case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
  
        	// AD1938 doesn't support DMA, but supports indirectly thru SPORT operation
        	*((u32 *)Value) = FALSE;
   
        	break;

    	// CASE (Set Dataflow method - applies only for SPORT)
    	case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):

			// Check if sport device to used by AD1938 is already open
			if (pAD1938->sportHandle == NULL)
				// if not, try to open the SPORT device corresponding to sportDeviceNumber for AD1938 dataflow
				Result = SPORT_Open(pAD1938);

// on occurance	of error, return the error code
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

			// Pass	the	dataflow method	to the SPORT device allocated to AD1938
			Result = adi_dev_Control( pAD1938->sportHandle, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void*)u32Value );
		
        	break;
        
    	// CASE (Read a specific register from the AD1938)
    	case(ADI_DEV_CMD_REGISTER_READ):
    	// CASE (Configure a specific register in the AD1938)
    	case(ADI_DEV_CMD_REGISTER_WRITE):
    	// CASE (Read a specific field from a single device register)
    	case(ADI_DEV_CMD_REGISTER_FIELD_READ):
    	// CASE (Write to a specific field in a single device register)
    	case(ADI_DEV_CMD_REGISTER_FIELD_WRITE):     	
    	// CASE (Read block of AD1938 registers starting from first given address)
    	case(ADI_DEV_CMD_REGISTER_BLOCK_READ):  
    	// CASE (Write to a block of AD1938 registers starting from first given address)
    	case(ADI_DEV_CMD_REGISTER_BLOCK_WRITE):    	
    	// CASE (Read a table of selective registers in AD1938)
    	case(ADI_DEV_CMD_REGISTER_TABLE_READ):
    	// CASE (Write to a table of selective registers in AD1938)
    	case(ADI_DEV_CMD_REGISTER_TABLE_WRITE):    	
    	// CASE (Read a table of selective register(s) field(s) in AD1938)
    	case(ADI_DEV_CMD_REGISTER_FIELD_TABLE_READ):
    	// CASE (Write to a table of selective register(s) field(s) in AD1938)
    	case(ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE):    
    
     		access_device.ManagerHandle  	= pAD1938->ManagerHandle;     	// device manager handle
     		access_device.ClientHandle  	= pAD1938;     					// client handle - passed back to the this driver by Device access
     		access_device.DeviceNumber  	= 0; 							// SPI device number (is always 0)
  			access_device.DeviceAddress  	= pAD1938->spiDeviceAddress; 	// SPI Global address of AD1938
     		access_device.DCBHandle   		= pAD1938->DCBHandle;         	// handle to the callback manager
     		access_device.DeviceFunction 	= DevAccessFN; 					// Function specific to AD1938 driver passed to the 'Device' access service
     		access_device.FinalRegAddr  	= AD1938_FINAL_REG_ADDRESS;		// Address of the last register in AD1938
     		access_device.RegisterField  	= RegisterField;         		// table for AD1938 Register Field Error check and Register field access
     		access_device.ReservedValues 	= ReservedValues;       		// table to configure reserved bits in AD1938 to recommended values
     		access_device.ValidateRegister 	= NULL;       					// table containing reserved and read-only registers in AD1938
     		access_device.ConfigTable  		= NULL; 						// SPI configuration table
  			access_device.SelectAccess  	= SelectSPIAccess;    			// Device Access type
  			access_device.pAdditionalinfo 	= (void *)NULL;        			// No Additional info

			/* Workaround for Blackfin Audio Extender rev 1.2 hardware anamoly
			Access to first register in a given register access table */
			DummyRead.Address 				= AD1938_PLL_CLK_CTRL0;			// AD1938 register to be read
     		access_device.Command   		= ADI_DEV_CMD_REGISTER_READ;  	// Read a single register
     		access_device.Value    			= &DummyRead;       			// Location to hold the dummy read value

			Result = adi_device_access (&access_device);
			     		     					
     		access_device.Command   		= Command;                  	// command ID
     		access_device.Value    			= Value;                    	// command specific value						
			
			Result = adi_device_access (&access_device);
			
        	break;
       
/************************
SPI related commands
************************/
                
    	// CASE (Set AD1938 SPI Chipselect)
    	case ADI_AD1938_CMD_SET_SPI_CS:

        	pAD1938->ad1938_spi_cs = u8Value;
         
        	break;
   
    	// CASE (Get AD1938 SPI Chipselect value)
    	case ADI_AD1938_CMD_GET_SPI_CS:
   
        	*((u8 *)Value) = pAD1938->ad1938_spi_cs;

        	break;

    	// CASE (Set SPI address for the AD1938 device)
    	case (ADI_AD1938_CMD_SET_SPI_DEVICE_ADDRESS):

        	// Update the SPI device address
        	pAD1938->spiDeviceAddress = u32Value;

    		break;

/************************
SPORT related commands
************************/

    	// CASE (Set SPORT Device Number that will be used for audio dataflow between Blackfin and AD1938)
    	case (ADI_AD1938_CMD_SET_SPORT_DEVICE_NUMBER):
       
			// Close the present SPORT device being used
			Result = SPORT_Close(PDDHandle);
			
        	// Update the SPORT device number
        	pAD1938->sportDeviceNumber = u8Value;

			// Application should set dataflow method for this SPORT device, load data buffer(s) & enable dataflow

        	break;
			        	
    	// CASE (OPEN/CLOSE SPORT Device to be used for audio dataflow between Blackfin and AD1938)
    	case (ADI_AD1938_CMD_SET_SPORT_STATUS):
					
			// check the SPORT mode
			if ((ADI_AD1938_SET_SPORT_STATUS) Value == ADI_AD1938_SPORT_OPEN)
			{
				// check if the SPORT device is already open
				if (pAD1938->sportHandle == NULL)
					// if not, OPEN	Sport Device, else do nothing
					Result = SPORT_Open(pAD1938);
			}			
			else	// this should be to close the SPORT device
				// Close any SPORT device presently being used by AD1938
				Result = SPORT_Close(pAD1938);
			
			break;

    	// CASE (Enable/Disable Auto-SPORT configuration mode)
    	case (ADI_AD1938_CMD_ENABLE_AUTO_SPORT_CONFIG):
					
			pAD1938->AutoSportEnable = u8Value;
			
			break;		
			
		// Command(s) no longer used/supported by this driver. Left here for backward compatibility
		case (ADI_AD1938_CMD_SET_SPORT_OPERATION_MODE):
			break;
            	   
    	default:
   
       		if (pAD1938->sportHandle) // if SPORT device is already opened
    			// pass the unknown command to SPORT
				Result = adi_dev_Control( pAD1938->sportHandle, Command, Value );
			else
	        	// return error indicating as command not valid
    	    	Result = ADI_AD1938_RESULT_CMD_NOT_SUPPORTED;
   
        	break;
    }
 
    return(Result);
}

/*********************************************************************

 Function:  DevAccessFN

 Description: Device Access Function - Detects any changes to AD1938
 			  device settings and updates Blackfin SPORT device settings
 			  in relevance to those changes

*********************************************************************/
static u32 DevAccessFN(
    void   					*Handle,   		// AD1938 driver Argument passed back by the device access service
    u16  					*RegAddr, 		// Application has selected to configure this Register address in AD1938
    u16  					RegData,		// Above AD1938 register will be configured to this value
    ADI_DEVICE_ACCESS_MODE	access_mode		// present access mode of AD1938
){

    //	Pointer	to AD1938 device driver instance
	ADI_AD1938 	*pAD1938 		= (ADI_AD1938 *)Handle;    
  	u32     	Result 			= ADI_DEV_RESULT_SUCCESS;	// result
  	u16			Temp,TempWlen	=0;	  	// temporary locations

#if defined(ADI_DEV_DEBUG)
	if ((Result = ValidatePDDHandle(Handle)) != ADI_DEV_RESULT_SUCCESS)   // verify the DM handle
    	return (Result);
#endif

	// reset SPORT command select register
	pAD1938->SportCmdSelect = 0;

	// check the access mode
	if (access_mode == ADI_DEVICE_ACCESS_PRE_WRITE)
	{
	    // Device access mode - Device access is about to configure AD1938 registers
	    
		// Check AD1938 register address being configured
		// if it is DAC control 2 or ADC control 1, extract DAC/ADC word length 
		if ((*RegAddr == AD1938_DAC_CTRL2)|| (*RegAddr == AD1938_ADC_CTRL1))
		{
			if (*RegAddr == AD1938_DAC_CTRL2)
				TempWlen	= 3;	// Shift count to extract DAC word length
				
			// extract AD1938 ADC/DAC word width
			Temp	= ((RegData >> TempWlen) & 0x03);

			// check AD1938 ADC/DAC word width and 
			// Update TempWlen with actual word length value to configure SPORT device word length
			if (Temp == 0)		// AD1938 ADC/DAC word width is 24
				TempWlen	= SLEN_24;
			else if (Temp == 1)	// AD1938 ADC/DAC word width is 20
				TempWlen	= SLEN_20;
			else if (Temp == 3)	// AD1938 ADC/DAC word width is 16
				TempWlen	= SLEN_16;
			else	// Client tries to set invalid ADC/DAC word width. Return error
				return(ADI_DEV_RESULT_INVALID_REG_FIELD_DATA);
		}

	    // Switch to selected register address
		switch(*RegAddr)
 		{
 			// case (DAC control register 0)
 		    case (AD1938_DAC_CTRL0):

 		    	// check ADC serial mode
 		    	if (pAD1938->SerialModeADC != AD1938_SERIAL_MODE_TDM_AUX)
 		    	{
 		    	    // continue auto-SPORT config only if ADC is not in TDM-coupled Aux mode
					// Extract DAC Serial mode
					Temp	= ((RegData >> 6) & 0x03);
						
					// check for changes in DAC Serial mode
					// DAC serial mode changes will be 
					if (Temp != pAD1938->SerialModeDAC)
					{				    
						// new serial mode detected					
						pAD1938->SerialModeDAC	= Temp;	// save the new serial mode
			    		// check if the serial mode is stereo
			    		if (pAD1938->SerialModeDAC == AD1938_SERIAL_MODE_STEREO)
				    	{		    	
				        	// AD1938 DAC is in Stereo Mode.
				        	// SPORT config settings for I2S mode
				        	// In Stereo mode, auto Sport Config always configures SPORT to I2S mode
				        	pAD1938->Sport.TCR1 = I2S_SPORT_TCR1;
				        	pAD1938->Sport.TCR2 = (I2S_SPORT_TCR2 | pAD1938->AD1938DACwlen); 
				        	pAD1938->Sport.MCMC2= STEREO_MCMC2;
        					// Select SPORT configuration commands for data Transmit (for AD1938 DAC)
							pAD1938->SportCmdSelect 	= (SPORT_TCR1 | SPORT_TCR2 | SPORT_MCMC2);
				    	}
				    	else
				    	{		
				    	    // auto-SPORT config is not supported for other serial modes
				    	}
						// Update Sport configuration
						Result = SPORT_Config(pAD1938);				    	
					}			
				}
				
				break;

			// case (DAC Control 2)
			case(AD1938_DAC_CTRL2):
				// TempWlen has AD1938 DAC word width value equivalent to SPORT word length
				// Compare the word width value
				if (TempWlen != pAD1938->AD1938DACwlen)
				{
					// new DAC word width detected. update the driver cache
					pAD1938->AD1938DACwlen	= TempWlen;
					// Check the present serial mode
					if (pAD1938->SerialModeDAC == AD1938_SERIAL_MODE_STEREO)
					{
						// AD1938 is in stereo mode. update the SPORT word length 
						pAD1938->Sport.TCR2 = (I2S_SPORT_TCR2 | pAD1938->AD1938DACwlen);					
        				// Select SPORT configuration commands for data Transmit (for AD1938 DAC)
						pAD1938->SportCmdSelect 	= (SPORT_TCR2);
						// Update Sport configuration
						Result = SPORT_Config(pAD1938);
					}
				}

				break;

 			// case (ADC control register 1)
 		    case (AD1938_ADC_CTRL1):
 		    
				// Extract ADC Serial mode
				Temp	= ((RegData >> 5) & 0x03);
				
				// TempWlen has AD1938 ADC word width value equivalent to SPORT word length
				
				// check for changes in ADC Serial mode or Word Length
				if ((Temp != pAD1938->SerialModeADC) || (TempWlen != pAD1938->AD1938ADCwlen))
				{
					// detected new serial mode / word length
					pAD1938->SerialModeADC	= Temp;	// save the serial mode
					pAD1938->AD1938ADCwlen	= TempWlen;	// save the word length
			    	// check if the serial mode is stereo
			    	if (pAD1938->SerialModeADC == AD1938_SERIAL_MODE_STEREO)
				    {		    	
				       	// AD1938 ADC is in Stereo Mode.
				        // SPORT config settings for I2S mode
				        // In Serial mode, auto Sport Config always configures SPORT to I2S mode			        
				        pAD1938->Sport.RCR1 = I2S_SPORT_RCR1;
				        pAD1938->Sport.RCR2 = (I2S_SPORT_RCR2 | pAD1938->AD1938ADCwlen); 
				        pAD1938->Sport.MCMC2= STEREO_MCMC2;
        				// Select SPORT configuration commands to receive data (for AD1938 ADC)
						pAD1938->SportCmdSelect 	= (SPORT_RCR1 | SPORT_RCR2 | SPORT_MCMC2);
				    }
				    // check if serial mode is TDM-coupled Aux
				    else if (pAD1938->SerialModeADC == AD1938_SERIAL_MODE_TDM_AUX)
				    {     												        
				    	// AD1938 ADC is in TDM-Aux mode
				    	// This will bypass DAC settings and SPORT Tx will be configured for TDM-Aux mode
						// load SPORT Tx control registers with TDM-coupled Aux value
				        pAD1938->Sport.TCR1 	= TDM_AUX_SPORT_TCR1;
				        pAD1938->Sport.TCR2 	= TDM_AUX_SPORT_TCR2;
				    	// load SPORT Rx control registers with TDM-coupled Aux value
				        pAD1938->Sport.RCR1 	= TDM_AUX_SPORT_RCR1;
				        pAD1938->Sport.RCR2 	= TDM_AUX_SPORT_RCR2;
				    	// load the SPORT Multichannel frame delay value  
				    	pAD1938->Sport.MCMC2	= TDM_AUX_SPORT_MCMC2;       
							
    					// for TDM-Copuled Aux, pass all Sport config commands in SPORT_ConfigTable
        				pAD1938->SportCmdSelect 	= ( SPORT_TCR1 | SPORT_TCR2 | SPORT_RCR1 | SPORT_RCR2 | 
        												SPORT_MCMC1 | SPORT_MCMC2 | SPORT_MRCS0 | SPORT_MTCS0);
					}
					else
					{
					    // auto-SPORT config is not supported for other serial modes
					}
					// Update Sport configuration
					Result = SPORT_Config(pAD1938);
				}
				
				break;
			
			// case (ADC Control 2)
			case(AD1938_ADC_CTRL2):

				// extract BCLKs per frame to calculate channel count
				Temp	= ((RegData >> 4) & 0x03);

				// Default SPORT MCMC1 reg to 8 channels
				pAD1938->Sport.MCMC1		= TDM_AUX_SPORT_MCMC1_8;
								
				if (Temp == 0)	// AD1938 ADC BCLKs per frame is 64 (2 channels)
					// enable 2 Tx/Rx channels in SPORT device
					pAD1938->Sport.MCS0	= TDM_AUX_SPORT_MCN_2;
				else if (Temp == 1)	// AD1938 ADC BCLKs per frame is 128 (4 channels)
					// enable 4 Tx/Rx channels in SPORT device
					pAD1938->Sport.MCS0	= TDM_AUX_SPORT_MCN_4;
				else if (Temp == 2)	// AD1938 ADC BCLKs per frame is 256 (8 channels)
					// enable 8 Tx/Rx channels in SPORT device
					pAD1938->Sport.MCS0	= TDM_AUX_SPORT_MCN_8;
				else	// AD1938 ADC BCLKs per frame is 512 (16 channels)
				{
					// set SPORT MCMC1 reg to 16 channels
					pAD1938->Sport.MCMC1	= TDM_AUX_SPORT_MCMC1_16;
					// enable 16 Tx/Rx channels in SPORT device
					pAD1938->Sport.MCS0		= TDM_AUX_SPORT_MCN_16;
					
				}

        		// Select SPORT configuration commands to transmit/receive data (for AD1938 DAC & ADC)
				pAD1938->SportCmdSelect 	= (SPORT_MCMC1 | SPORT_MTCS0 | SPORT_MRCS0);
				// Update Sport configuration
				Result = SPORT_Config(pAD1938);
				
				break;
		
			// no action for other register configuration        	
        	default:
        		break;
		}
    }
    
    return (Result); 		    
}    

/*********************************************************************

 Function:  sportCallbackFunction

 Description: callback from the SPORT Driver

*********************************************************************/

static void sportCallbackFunction (
	void* 	DeviceHandle, 
	u32 	Event, 
	void* 	pArg
){
    ADI_AD1938 *pAD1938 = (ADI_AD1938 *)DeviceHandle;

#ifdef ADI_DEV_DEBUG
	// verify the DM handle
    if (ValidatePDDHandle(DeviceHandle) != ADI_DEV_RESULT_SUCCESS) 
        return;
#endif

    // simply pass the callback along to the Device Manager Callback without interpretation
    (pAD1938->DMCallback)(pAD1938->DMHandle,Event,pArg);
}

// Debug build only
#ifdef ADI_DEV_DEBUG

/*********************************************************************

 Function:  ValidatePDDHandle

 Description: Validates a PDD handle

*********************************************************************/
static int ValidatePDDHandle(
	ADI_DEV_PDD_HANDLE 	PDDHandle
) {
 	unsigned int i;
 	for (i = 0; i < ADI_AD1938_NUM_DEVICES; i++) 
 	{
  		if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i])
   			return (ADI_DEV_RESULT_SUCCESS);
 	}
 	return (ADI_DEV_RESULT_BAD_PDD_HANDLE);
}

#endif
/*********************************************************************

Entry point for device manager

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT ADIAD1938IIEntryPoint = {
	adi_pdd_Open,
 	adi_pdd_Close,
 	adi_pdd_Read,
 	adi_pdd_Write,
 	adi_pdd_Control,
 	adi_pdd_SequentialIO
};



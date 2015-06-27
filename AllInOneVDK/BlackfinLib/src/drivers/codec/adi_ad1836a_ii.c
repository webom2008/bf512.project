/**********************************************************************

Copyright(c) Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software 
you agree to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_ad1836a_ii.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
            This is the driver source code for the AD1836A Audio Codec.
            
            This driver is layered on top of the SPI and SPORT device drivers, 
            which are configured for the specific use of the AD1836A peripheral. 
            The driver uses Device Access Service to access registers in AD1836A
            
            The driver senses any change in AD1836A device settings (usually 
            carried out by the application) and automatically updates the 
            corresponding SPORT device configuration to support present AD1836A mode.
            
            The driver doesnot support auto SPORT confing for Right Justified, 
            Packed 128  & Packed 256 modes. Any application intend to use these
            modes can configure the SPORT device in above modes by issuing SPORT 
            driver specific commands with AD1836A driver handle.            
            
Note: 
			Whenever the application switches from Packed AUX to other serial mode, 
			application should first change AD1836A ADC serial mode before 
			modifying its DAC serial mode. 
			The reason is, when AD1836A ADC is set in Packed mode Aux, it bypasses 
			DAC serial mode settings and any modifications to DAC serial mode 
			will be ignored by the driver as well as by AD1836A device.
***********************************************************************/

/*********************************************************************

Include files

*********************************************************************/

#include <services/services.h>							// system service includes
#include <drivers/adi_dev.h>							// device manager includes
#include <drivers/sport/adi_sport.h>			     	// SPORT driver includes
#include <drivers/deviceaccess/adi_device_access.h>  	// adi_device_access driver includes
#include <drivers/codec/adi_ad1836a_ii.h>   			// AD1836A driver includes

/*********************************************************************

Enumerations and defines

*********************************************************************/
// number of AD1836A devices in the system
#define ADI_AD1836A_NUM_DEVICES 	(sizeof(Device)/sizeof(ADI_AD1836A)) 
// Number of writeable registers in AD1836A
#define AD1836A_REG_COUNT			11
// Address of the last register in AD1836A
#define AD1836A_FINAL_REG_ADDRESS	0xE8FF    
// Masks the 1836 Address location & clears remaining bits
#define ADI_AD1836A_ADDR_MASK		0xF000
// Masks the 1836 Data location & clears remaining bits
#define ADI_AD1836A_REG_VALUE_MASK	0x03FF

// SPORT word length (value should be actual wordlength - 1)
#define SLEN_16						(15)	// 16 bits
#define SLEN_20						(19)	// 20 bits
#define SLEN_24						(23)	// 24 bits
#define SLEN_32						(31)	// 32 bits
#define	SLEN_MASK					0x1F	// Mask to extract SPORT word length

// SPORT Config values for supported serial modes
// for I2S mode (stereo)
#define	I2S_SPORT_TCR1				(TFSR  | TCKFE)
#define	I2S_SPORT_TCR2				(TSFSE | TXSE)
#define	I2S_SPORT_RCR1				(RFSR  | RCKFE)
#define	I2S_SPORT_RCR2				(RSFSE | RXSE)
// for Left-Justified mode (stereo)
#define	LJ_SPORT_TCR1				(LATFS | LTFS | TFSR )
#define	LJ_SPORT_TCR2				(TSFSE | TXSE)
#define	LJ_SPORT_RCR1				(LARFS | LRFS | RFSR )
#define	LJ_SPORT_RCR2				(RSFSE | RXSE)
// for DSP mode (stereo)
#define	DSP_SPORT_TCR1				(TFSR | DITFS)
#define	DSP_SPORT_TCR2				(TXSE)
#define	DSP_SPORT_RCR1				(RFSR | DITFS)
#define	DSP_SPORT_RCR2				(RXSE)
// common for Stereo modes
#define	STEREO_MCMC2				(0)		// Multichannel disabled
// for Packed mode AUX (TDM)
#define	PAUX_SPORT_TCR1				(TFSR)
#define	PAUX_SPORT_RCR1				(RFSR)
#define	PAUX_SPORT_WLEN				(SLEN_32)					// always SPORT wordlength should be 32bits
#define	PAUX_MCMC1					(0)							// 8 active channels starts from window 0

#if defined (__ADSP_EDINBURGH__)    ||\
    defined (__ADSP_BRAEMAR__)      ||\
    defined (__ADSP_STIRLING__)     ||\
    defined (__ADSP_TETON__)        ||\
    defined (__ADSP_KOOKABURRA__)   ||\
    defined (__ADSP_MOCKINGBIRD__)  ||\
    defined (__ADSP_BRODIE__)
#define	PAUX_MCMC2					(MCMEN | MCDTXPE | MCDRXPE)	// Multichannel enabled,DMA Tx/Rx packing enabled
#elif defined (__ADSP_MOAB__)
#define	PAUX_MCMC2					(MCMEM | MCDTXPE | MCDRXPE)	// Multichannel enabled,DMA Tx/Rx packing enabled
#endif

#define	PAUX_MTCS0					(0x000000FF)				// 8 Transmit channels enabled
#define	PAUX_MRCS0					(0x000000FF)				// 8 Receive channels enabled

// SPORT device configuration commands - bitwise location in SPORT_ConfigTable
#define	SPORT_TCR1					0x0001
#define	SPORT_TCR2					0x0002
#define	SPORT_RCR1					0x0004
#define	SPORT_RCR2					0x0008
#define	SPORT_MCMC1					0x0010
#define	SPORT_MCMC2					0x0020
#define	SPORT_MTCS0					0x0040
#define	SPORT_MRCS0					0x0080

// Cache for AD1836A registers
// AD1836A hardware allows only to write-to these register
typedef struct {
	u16		DACControl1;
	u16		DACControl2;
	u16		DAC1LVol;
	u16		DAC1RVol;
	u16		DAC2LVol;
	u16		DAC2RVol;	
	u16		DAC3LVol;
	u16		DAC3RVol;
	u16		ADCControl1;
	u16		ADCControl2;
	u16		ADCControl3;
} 	ADI_AD1836A_REGS;

// SPORT device register cache for auto SPORT configuration
typedef struct {
    u16		TCR1;
    u16		TCR2;
    u16		RCR1;
    u16		RCR2;
    u16		MCMC2;
}	ADI_AD1836A_SPORT_REGS;

// AD1836A Supported serial modes
// This driver doesnot support auto SPORT configuration for RJ_MODE, PACKED_128 & PACKED_256 modes.
enum
{
    I2S_MODE = 0,
    RJ_MODE,
    DSP_MODE,
    LJ_MODE,
    PACKED_256,
    PACKED_128,
    PACKED_AUX,		// mode applicable for ADC only & it over-rides DAC automatically
};

// Structure to contain data for an instance of the AD1836A device driver
typedef struct {
    ADI_DEV_MANAGER_HANDLE      ManagerHandle;      // Manager Handle       
    ADI_DEV_DEVICE_HANDLE       DMHandle;           // Handle of Device Manager instance
    ADI_DMA_MANAGER_HANDLE      DMAHandle;          // handle to the DMA manager
    ADI_DCB_HANDLE              DCBHandle;          // callback handle    
    ADI_DCB_CALLBACK_FN         DMCallback;         // the callback function supplied by the Device Manager    
    ADI_DEV_PDD_HANDLE          sportHandle;        // Handle to the underlying SPORT device driver
    ADI_DEV_DIRECTION           sportDirection;     // SPORT data direction  
	u8                          sportDeviceNumber;  // SPORT Device number used for AD1836A audio dataflow
    u8                          ad1836a_spi_cs;    	// SPI Chipselect for blackfin (SPI_FLG location to select AD1836A)
	u8							InUseFlag;			// Device in use flag
	u8							DataflowStatus;		// Dataflow status flag (Data flow ON/OFF)	
	u16							ad1836aRegs[11];	// AD1836A Register cache
	ADI_AD1836A_SPORT_REGS		Sport;				// SPORT config values for AD1836A operation
	u8							SerialModeADC;		// Present Serial mode of AD1836A ADC
	u8							SerialModeDAC;		// Present Serial mode of AD1836A DAC
	u16							SportCmdSelect;		// Indicates list of sport registers to be configured	
	u8							AutoSportEnable;	// Enable/Disable auto-SPORT configuration mode
	u8                          ad1836a_spi_device; // SPI Device number to use
} ADI_AD1836A;

// The initial values for the device instance
static ADI_AD1836A Device[] = {
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
        FALSE,		// device not in use
        FALSE,		// dataflow off        
		AD1836A_DAC_CTRL_1,
		AD1836A_DAC_CTRL_2,
		AD1836A_DAC_1L_VOL,
		AD1836A_DAC_1R_VOL,
		AD1836A_DAC_2L_VOL,
		AD1836A_DAC_2R_VOL,
		AD1836A_DAC_3L_VOL,
		AD1836A_DAC_3R_VOL,
		AD1836A_ADC_CTRL_1,
		AD1836A_ADC_CTRL_2,
		AD1836A_ADC_CTRL_3,
		{	// AD1836A in I2S after powerup - Init sport config settings to I2S as default
			I2S_SPORT_TCR1,	
			(I2S_SPORT_TCR2 | SLEN_24),		// 24 bit wordlength
			I2S_SPORT_RCR1,
			(I2S_SPORT_RCR2 | SLEN_24),
			STEREO_MCMC2,
		},
		0,			// AD1836A ADC in I2S mode after powerup/reset
		0,			// AD1836A DAC in I2S mode after powerup/reset
		0,			// SPORT device is closed (not configured) by default
		TRUE,	    // Auto-SPORT configuration mode enabled by default     
		0,          // use SPI 0 by default
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
    ADI_AD1836A 			*pAD1836A				// pointer to the driver instance
);

static u32 SPORT_Config( 
    ADI_AD1836A 			*pAD1836A				// pointer to the driver instance
);

// Automatically updates Sport device settings in relevance to the changes with AD1836A register settings
static u32	UpdateSportConfig(
	ADI_AD1836A 			*pAD1836A,				// pointer to the driver instance
	ADI_DEV_ACCESS_REGISTER	*Access1836				// AD1836A data location
);
    
static u32 SPORT_Close( 
    ADI_AD1836A 			*pAD1836A				// pointer to the driver instance
);

// Masks AD1836A register field
static void Mask1836Field(
	u16 					RegAddr,				// Register Address being accessed
	u16 					*RegData,				// Present value of the Register
	u16 					RegField,				// Register Field of the register to access
	u8						rwFlag					// Read/ Write Flag
);

// Validates AD1836A register address to be accessed
static u32	ValidateRegAddress ( 
	ADI_AD1836A 			*pAD1836A, 				// pointer to the device we're working on
	u16						RegAddr, 				// AD1836A Register address to be validated
	u8 						rwFlag, 				// read/write Flag
	u8 						*index 					// index of register match
);

// Accesses AD1836A Hardware via SPI using the Device Access Service
static u32 Access1836SPI (
	ADI_AD1836A  			*pAD1836A, 				// pointer to the device we're working on
	u32       				Command,   				// command ID
	ADI_DEV_ACCESS_REGISTER	*Access1836				// AD1836A data location
);

// Accesses AD1836A device registers
static u32	Access1836Regs (
    ADI_DEV_PDD_HANDLE      PDDHandle,              // PDD handle
    u32                     Command,                // command ID
    void                    *Value                  // command specific value
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
*
*   Function:       SPORT_Open
*
*   Description:    Opens the sport device for AD1836A audio dataflow
*
*********************************************************************/
static u32 SPORT_Open( 
	ADI_AD1836A 	*pAD1836A		// pointer to the driver instance
) {
    
    // default return code
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    // Open the SPORT driver
    Result = adi_dev_Open( 
        pAD1836A->ManagerHandle,        // device manager handle
        &ADISPORTEntryPoint,            // SPORT Entry point
        pAD1836A->sportDeviceNumber,    // SPORT device number
        pAD1836A,                       // client handle - passed to internal callback function
        &pAD1836A->sportHandle,         // pointer to DM handle (for SPORT driver) location
        pAD1836A->sportDirection,       // SPORT data direcion
        pAD1836A->DMAHandle,            // handle to the DMA manager
        pAD1836A->DCBHandle,            // handle to the callback manager
        sportCallbackFunction           // internal callback function
    );

// return with appropriate code if SPORT driver fails to open
#ifdef ADI_DEV_DEBUG
    if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// clear SPORT command select reg
	pAD1836A->SportCmdSelect = 0;

	// The driver doesnot support auto sport-config for Right Justified, Packed 128 & Packed 256 modes
	// IN these modes, Application can still connfigure this SPORT device by issuing 
	// SPORT driver specific commands with AD1836A driver handle

    // Check the present serial mode ID
    // SPORT Rx (AD1836A ADC) config
    // Packed AUX will bypass Sport Tx (DAC) configuration settings
    if (pAD1836A->SerialModeADC == PACKED_AUX)
    	// for Packed AUX, pass all Sport config commands in SPORT_ConfigTable
        pAD1836A->SportCmdSelect 	= ( SPORT_TCR1 | SPORT_TCR2 | SPORT_RCR1 | SPORT_RCR2 | SPORT_MCMC2 | 
        								SPORT_MCMC1 | SPORT_MRCS0 | SPORT_MTCS0);
	// check if this driver supports auto-sport config for this stereo mode
	else if ((pAD1836A->SerialModeADC != RJ_MODE) ||	(pAD1836A->SerialModeADC != PACKED_128) || 
			(pAD1836A->SerialModeADC != PACKED_256))
    	// Configure SPORT device in relevance to present values in AD1836A register cache
    	// Configure Sport Rx & Multichannel config2 registers
    	pAD1836A->SportCmdSelect = (SPORT_RCR1 | SPORT_RCR2 | SPORT_MCMC2);

	// for SPORT Tx (AD1836A DAC config)
	// check if Ad1836A ADC is not in Packed mode Aux
    if (pAD1836A->SerialModeADC != PACKED_AUX)
    {
        // check if this driver supports auto-sport config for this stereo mode
        if ((pAD1836A->SerialModeDAC != RJ_MODE) 	|| (pAD1836A->SerialModeDAC != PACKED_128) 	|| 
			(pAD1836A->SerialModeDAC != PACKED_256))
		{
	    	// Configure SPORT device in relevance to present values in AD1836A register cache
    		// Configure Sport Tx & Multichannel config2 registers
    		pAD1836A->SportCmdSelect |= (SPORT_TCR1 | SPORT_TCR2 | SPORT_MCMC2);
		}
    }
    
	// call SPORT device configuration routine
    Result = SPORT_Config(pAD1836A);
    
    return (Result);
}

/*********************************************************************
*
*   Function:       SPORT_Config
*
*   Description:    Configures SPORT device relative to AD1836A settings
*	Note:			Only marked commands will be issued to SPORT device
*					i.e., Command IDs corresponding to the bit location 
*					in SportCmdSelect which are SET to 1 will be passed
*					to the SPORT device driver 
*
*********************************************************************/
static u32 SPORT_Config( 
    ADI_AD1836A 	*pAD1836A		// pointer to the driver instance
){
    
    // default return code
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    u16	i = 0;	// index
	
// SPORT Configuration Table
ADI_DEV_CMD_VALUE_PAIR SPORT_ConfigTable[] = 
{
    	// SPORT TCR1 Config
		{	ADI_SPORT_CMD_SET_TCR1, 	(void *)(pAD1836A->Sport.TCR1)	},
		{ 	ADI_SPORT_CMD_SET_TCR2,     (void *)(pAD1836A->Sport.TCR2)	},
		{ 	ADI_SPORT_CMD_SET_RCR1, 	(void *)(pAD1836A->Sport.RCR1)	},
		{ 	ADI_SPORT_CMD_SET_RCR2,	    (void *)(pAD1836A->Sport.RCR2)	},	
		{ 	ADI_SPORT_CMD_SET_MCMC1,	(void *)PAUX_MCMC1				},
		{ 	ADI_SPORT_CMD_SET_MCMC2,	(void *)(pAD1836A->Sport.MCMC2)	},
		{ 	ADI_SPORT_CMD_SET_MTCS0,	(void *)PAUX_MTCS0				},
		{ 	ADI_SPORT_CMD_SET_MRCS0,	(void *)PAUX_MRCS0				},
		{ 	ADI_DEV_CMD_END, 			(void *)NULL					},
};

	// is SPORT device open already?, Is there a need to configure SPORT device?
	// perform auto-SPORT configuration?
	if ((pAD1836A->AutoSportEnable) && (pAD1836A->sportHandle != NULL) && (pAD1836A->SportCmdSelect != 0))
	{
	    // if SPORT device is open, check for dataflow status
		// if dataflow is on, switch dataflow off before updating SPORT device configuration
		if (pAD1836A->DataflowStatus == TRUE)
			Result	= adi_dev_Control( pAD1836A->sportHandle, ADI_DEV_CMD_SET_DATAFLOW,	(void*)FALSE );

#ifdef ADI_DEV_DEBUG
    	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);  
#endif

		// parse through all commands in SPORT_ConfigTable until ADI_DEV_CMD_END is reached
		while (SPORT_ConfigTable[i].CommandID != ADI_DEV_CMD_END)
		{
		    // check if the command id corresponding to the present index value 
		    // is marked in 'SportCmdSelect' register
			if (pAD1836A->SportCmdSelect & (1 << i))
		    {
		        // If a bit location is set in SportCmdSelect, pass the corresponding command to SPORT
    			Result = adi_dev_Control( pAD1836A->sportHandle, SPORT_ConfigTable[i].CommandID, SPORT_ConfigTable[i].Value);		        
		    }
		    
#ifdef ADI_DEV_DEBUG
    	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);  
#endif
		    i++;	// increment the index and continue until ADI_DEV_CMD_END is reached
		}
		
		// if dataflow was previously on, switch the dataflow on again
		if (pAD1836A->DataflowStatus == TRUE)
			Result	= adi_dev_Control( pAD1836A->sportHandle, ADI_DEV_CMD_SET_DATAFLOW,	(void*)TRUE );
	}
	
	return (Result);
}

/*********************************************************************
*
*   Function:       UpdateSportConfig
*
*   Description:    Senses any change in AD1836A device settings and 
*					automatically updates the corresponding	SPORT device 
*					configuration to support present AD1836A mode.
*
*********************************************************************/
static u32	UpdateSportConfig(
	ADI_AD1836A 				*pAD1836A,		// pointer to the driver instance
	ADI_DEV_ACCESS_REGISTER		*Access1836		// AD1836A data location
){
    
    u16	RegAddr,RegData;	// location to hold register address and its data passed by the application
    u16 Temp,TempWlen;		// temporary locations

    // default return code
    u32 Result = ADI_DEV_RESULT_SUCCESS;
        
    // Extract AD1836A Register address configured by the application 
    // and its corresponding data
    RegAddr	= (Access1836->Address & ADI_AD1836A_ADDR_MASK);
    RegData = (Access1836->Address & ADI_AD1836A_REG_VALUE_MASK);
	// reset SPORT command select register
	pAD1836A->SportCmdSelect = 0;

	// Check if the register address is DAC control 1 or ADC control 2
	// if so, extract ADC/DAC word length 
	if ((RegAddr == AD1836A_DAC_CTRL_1) || (RegAddr == AD1836A_ADC_CTRL_2))
	{

		if (RegAddr == AD1836A_DAC_CTRL_1)
			Temp	= 3;	// shift count to extract DAC word length
		else
			Temp	= 4;	// shift count to extract ADC word length

		// Extract the AD1836A ADC/DAC wordlength
		TempWlen	= ((RegData >> Temp) & 0x03);
        	
		if (TempWlen == 0)		// Application has set AD1836A DAC Wordlength to 24 bits
			// SPORT word length should be set to 24 bits
			TempWlen = SLEN_24;
		else if (TempWlen == 1)// Application has set AD1836A DAC Wordlength to 20 bits
			// SPORT word length should be set to 20 bits
			TempWlen = SLEN_20;
		else if (TempWlen == 2)	// Application must have set AD1836A DAC Wordlength to 16 bits
			// SPORT word length should be set to 16 bits
			TempWlen = SLEN_16;
		else	// Client is trying to set AD1836A to invalid ADC/DAC word width. Return error
			return(ADI_DEV_RESULT_INVALID_REG_FIELD_DATA);
	}

    switch (RegAddr)
    {
        // Case (DAC control register 1)
        case (AD1836A_DAC_CTRL_1):
        	// check if AD1836A is configured for Packed AUX
        	// this should bypass DAC serial mode and wordlength settings
        	if (pAD1836A->SerialModeADC == PACKED_AUX)
        		// in Packed AUX, driver rejects any change to DAC serial mode or wordlength
        		break;

        	// AD1836A is not in Packed mode AUX
        	// Extract serial mode data from RegData
        	Temp = ((RegData >> 5) & 0x07);
       	    // Check if driver supports auto SPORT config for present serial mode
           	if ((Temp != RJ_MODE) && (Temp != PACKED_128) && (Temp != PACKED_256))
			{         			            	
        		// check for changes in DAC Serial mode or word length
        		if ((Temp != pAD1836A->SerialModeDAC) || (TempWlen != (pAD1836A->Sport.TCR2 & SLEN_MASK)))
        		{
        	    	// detected new serial (stereo) mode/word length
        	    	// Update the Word length
        	    	pAD1836A->Sport.TCR2	= TempWlen;
        			// check present AD1836A stereo mode
        			if (Temp == I2S_MODE)
        			{
        			    // AD1836A DAC is in I2S mode
        			    pAD1836A->Sport.TCR1	= I2S_SPORT_TCR1;	// SPORT TC1
        			    pAD1836A->Sport.TCR2	|= I2S_SPORT_TCR2;	// SPORT TC2
        			}
        			else if (Temp == DSP_MODE)
        			{
        			    // AD1836A DAC is in DSP mode
						pAD1836A->Sport.TCR1	= DSP_SPORT_TCR1;	// SPORT TC1
						pAD1836A->Sport.TCR2	|= DSP_SPORT_TCR2;	// SPORT TC2
        			}
        			else
        			{
        			    // AD1836A DAC must be in Left Justified mode
        			    pAD1836A->Sport.TCR1	= LJ_SPORT_TCR1;	// SPORT TC1
        			    pAD1836A->Sport.TCR2	|= LJ_SPORT_TCR2;	// SPORT TC2        			    
        			}
        			pAD1836A->Sport.MCMC2	= STEREO_MCMC2;		// SPORT MCMC2
					// Select SPORT configuration commands for data Transmit settings (for AD1836A DAC)
					pAD1836A->SportCmdSelect 	= (SPORT_TCR1 | SPORT_TCR2 | SPORT_MCMC2);
					// call SPORT device configuration routine
    				Result = SPORT_Config(pAD1836A);
				}
			}
			// save the serial mode
        	pAD1836A->SerialModeDAC	= Temp;
        	
        	break;

        // Case (ADC control register 2)
        case (AD1836A_ADC_CTRL_2):
        	// Extract serial mode data from RegData
        	Temp = ((RegData >> 6) & 0x07);        	
			// Check if driver supports auto SPORT config for present serial mode
           	if ((Temp != RJ_MODE) && (Temp != PACKED_128) && (Temp != PACKED_256))
			{
        		// check for changes in ADC Serial mode or word length
        		if ((Temp != pAD1836A->SerialModeADC) || (TempWlen != (pAD1836A->Sport.RCR2 & SLEN_MASK)))
        		{
	       	    	// detected new serial mode/word length
			    	// check the present ADC serial mode
				    if (Temp == PACKED_AUX)
				    {
				        // AD1836A ADC is in Packed mode AUX. This will bypass DAC (SPORT Tx) settings
				        // SPORT config settings for Packed AUX
				        pAD1836A->Sport.RCR1 = PAUX_SPORT_RCR1;
				        pAD1836A->Sport.TCR1 = PAUX_SPORT_TCR1;
				        pAD1836A->Sport.RCR2 = PAUX_SPORT_WLEN;
				        pAD1836A->Sport.TCR2 = PAUX_SPORT_WLEN;
				        pAD1836A->Sport.MCMC2= PAUX_MCMC2;
        				// Select SPORT configuration commands for data Transmit & Receive settings (for AD1836A ADC & DAC)
						pAD1836A->SportCmdSelect 	= (SPORT_TCR1 | SPORT_TCR2 | SPORT_RCR1 | SPORT_RCR2 | SPORT_MCMC1 \
														| SPORT_MCMC2 | SPORT_MTCS0 | SPORT_MRCS0);
				    }
				    else
				    {
				    	// Serial mode should be one among the supported stereo mode
						// Update the Word length
        	    		pAD1836A->Sport.RCR2	= TempWlen;
        				// check present AD1836A ADC stereo mode
        				if (Temp == I2S_MODE)
        				{
        			    	// AD1836A DAC is in I2S mode
        			    	pAD1836A->Sport.RCR1	= I2S_SPORT_RCR1;	// SPORT RC1
        			    	pAD1836A->Sport.RCR2	|= I2S_SPORT_RCR2;	// SPORT RC2
        				}
        				else if (Temp == DSP_MODE)
        				{
        			    	// AD1836A DAC is in DSP mode
							pAD1836A->Sport.RCR1	= DSP_SPORT_RCR1;	// SPORT RC1
							pAD1836A->Sport.RCR2	|= DSP_SPORT_RCR2;	// SPORT RC2
        				}
        				else
        				{
        			    	// AD1836A DAC must be in Left Justified mode
        			    	pAD1836A->Sport.RCR1	= LJ_SPORT_RCR1;	// SPORT RC1
        			    	pAD1836A->Sport.RCR2	|= LJ_SPORT_RCR2;	// SPORT RC2        			    
        				}
        				pAD1836A->Sport.MCMC2	= STEREO_MCMC2;		// SPORT MCMC2
        				// Select SPORT configuration commands to Receive data (for AD1836A ADC)
						pAD1836A->SportCmdSelect 	= (SPORT_RCR1 | SPORT_RCR2 | SPORT_MCMC2);
				    }
					// call SPORT device configuration routine
    				Result = SPORT_Config(pAD1836A);
				}
        	}
			// save the serial mode
        	pAD1836A->SerialModeADC	= Temp;
        	
        	break;
        default:
        	break;
    }
    
    return (Result);
}
        	        	        	
/*********************************************************************
*
*   Function:       SPORT_Close
*
*   Description:    Closes the sport device used for AD1836A audio dataflow
*
*********************************************************************/
static u32 SPORT_Close( 
	ADI_AD1836A 	*pAD1836A		// pointer to the driver instance
) {
    
    // default return code
    u32 Result = ADI_DEV_RESULT_SUCCESS;
					
	// Check if any sport device is open
	// if so, close the present sport device in use
 	if (pAD1836A->sportHandle)
 	{
    	// close the present SPORT device
    	if ((Result = adi_dev_Close(pAD1836A->sportHandle))!= ADI_DEV_RESULT_SUCCESS)
    		return (Result);
			// Mark SPORT Handle as NULL indicating SPORT device is closed
			pAD1836A->sportHandle = NULL;        			
    }

    return (Result);
}

/*********************************************************************
*
* Function:  Mask1836Field
*
* Description: for AD1836A register field access
* Write mode => Masks other register fields and updates 
*               the corresponding register field with new value
*
* Read mode =>  Masks corresponding register fields and clears 
*               other register field values
*
*********************************************************************/

static void Mask1836Field(
	u16 RegAddr,		// Register Address being accessed
	u16 *RegData,		// Present value of the Register
	u16 RegField,		// Register Field of the register to access
	u8	rwFlag			// Read/ Write Flag
){
	
	u16 i=0,MaskVal,ShiftCount,Fieldlen = 1;
	
	if 		(RegAddr == AD1836A_DAC_CTRL_1)	MaskVal = 0xFD2E;
	else if (RegAddr == AD1836A_DAC_CTRL_2)	MaskVal = 0xFFFF;
	else if (RegAddr == AD1836A_ADC_CTRL_1)	MaskVal = 0xFFC9;
	else if (RegAddr == AD1836A_ADC_CTRL_2)	MaskVal = 0xFE5F;
	// RegAddr must be AD1836A_ADC_CTRL_3
	else									MaskVal = 0xFF7F;	
						
	ShiftCount = RegField;
	MaskVal >>= (RegField + 1);
	
	// get the register field length
    while ((!(MaskVal & 1)) && (ShiftCount!=0x09)) 
    {
    	Fieldlen++;    // increment register field length
		MaskVal >>= 1;
		ShiftCount++;
	}
	
	
	// shift number of register field times to mask bits present after the register field to be accessed
    if (RegField)	i = 0xFFFF >> (16 - RegField);        

    // Mask bits present before the register field to be accessed
    i |= (0xFFFF << (RegField + Fieldlen));

    // is the selected operation is read?
    if (rwFlag)
    {
        // Perform mask operation (clears other register field values)
        *RegData = *RegData & ~i;
		*RegData >>= RegField;
	}
	// Perform mask operation (clears present values of register field to be accessed)
 	else        *RegData = *RegData & i;

// return

}

/*********************************************************************
*
* Function:  ValidateRegAddress
*
* Description: Validates AD1836A register address to be accessed
*
*********************************************************************/

static u32	ValidateRegAddress ( 
	ADI_AD1836A *pAD1836A, 
	u16			RegAddr, 
	u8 			rwFlag, 
	u8 			*index 
)	{	
	u8		count = 0;
	bool 	AddressMatch = false;	// address match indicator
		
	while (!AddressMatch)	// compare the given address with the driver cache
	{
       	// Match the Address with the cache
    	if (RegAddr == (pAD1836A->ad1836aRegs[count] & ADI_AD1836A_ADDR_MASK))
    	{
    		// A register match was found
    		*index 	= count;	
			AddressMatch = true;
		}
        // if the register is one of the ADC VOLUME registers
        else if ((RegAddr & 0xC000) == 0x8000)
        {        	
        	// in write mode, return error as these registers are read only        	
        	if (!rwFlag)
        		return(ADI_DEV_RESULT_ATTEMPT_TO_WRITE_READONLY_REG);

     		AddressMatch = true;	// exit this function
       	}
       	++count;	// increment count
       	
       	// Check if the count is beyond the register count limit or register address is beyond the device boundary
		if ((count > AD1836A_REG_COUNT) || (RegAddr > AD1836A_FINAL_REG_ADDRESS))
			// The RegAddr must be beyond the boundry, return error
			return (ADI_DEV_RESULT_INVALID_REG_ADDRESS);
	}	
	return ADI_DEV_RESULT_SUCCESS;
}

/*********************************************************************
*
* Function:  Access1836SPI
*
* Description:  Accesses AD1836A Hardware 
*				via SPI using the Device Access Service
*
*********************************************************************/
static u32 Access1836SPI (
	ADI_AD1836A  				*pAD1836A, 		// pointer to the device we're working on
	u32       					Command,   		// command ID
	ADI_DEV_ACCESS_REGISTER		*Access1836		// AD1836A data location
) {

	// Structure passed to device access service
 	ADI_DEVICE_ACCESS_REGISTERS		Access_AD1836A;
 	u32 Result = ADI_DEV_RESULT_SUCCESS;   	// default return value
 		
	// Structure passed to select Device type and access type
	// This device is a special case
	ADI_DEVICE_ACCESS_SELECT	SelectSPIAccess[] = 
	{
 		pAD1836A->ad1836a_spi_cs,			// SPI Chip-select
 		ADI_DEVICE_ACCESS_LENGTH0,			// 'Device' Global address (no global address
 		ADI_DEVICE_ACCESS_LENGTH2,			// 'Device' register address length (2 bytes)
 		ADI_DEVICE_ACCESS_LENGTH0,			// 'Device' register data length (0 byte)
 		ADI_DEVICE_ACCESS_TYPE_SPI,			// Select SPI access
	};	

	Access_AD1836A.ManagerHandle  	= pAD1836A->ManagerHandle;  // device manager handle
   	Access_AD1836A.ClientHandle  	= NULL;      				// client handle - passed to the internal 'Device' specific function
    Access_AD1836A.DeviceNumber  	= pAD1836A->ad1836a_spi_device;// SPI device number 
  	Access_AD1836A.DeviceAddress  	= 0; 						// No SPI Global address of AD1836A
    Access_AD1836A.DCBHandle   		= pAD1836A->DCBHandle;    	// handle to the callback manager
    Access_AD1836A.DeviceFunction 	= NULL;        				// Function specific to AD1836A driver passed to the 'Device' access service
    Access_AD1836A.Command   		= Command;                  // command ID
    Access_AD1836A.Value    		= (void*)Access1836;		// command specific value
    Access_AD1836A.FinalRegAddr  	= AD1836A_FINAL_REG_ADDRESS;// Address of the last register in AD1836A
    Access_AD1836A.RegisterField  	= NULL;         			// table for AD1836A Register Field Error check and Register field access
    Access_AD1836A.ReservedValues 	= NULL;       				// table to configure reserved bits in AD1836A to recommended values
    Access_AD1836A.ValidateRegister = NULL;       				// table containing reserved and read-only registers in AD1836A
    Access_AD1836A.ConfigTable  	= NULL; 					// SPI configuration table
  	Access_AD1836A.SelectAccess  	= SelectSPIAccess;    		// Device Access type          
  	Access_AD1836A.pAdditionalinfo 	= (void *)NULL;        		// No Additional info

  	// call device access to configure corresponding AD1836A register
	Result = adi_device_access (&Access_AD1836A);
	
	// Check if the device access resulted in success
	if (Result == ADI_DEV_RESULT_SUCCESS)
	{
	    // check if the Device access was called to configure a AD1836A register
	    if (Command	== ADI_DEV_CMD_REGISTER_WRITE)
	    	// if so, update the sport configuration register in relevance to the new AD1836A register value
			Result = UpdateSportConfig(pAD1836A,Access1836);
	}

	return (Result);
	        
}

/*********************************************************************
*
* Function:  Access1836Regs
*
* Description: Accesses AD1836A device registers
* This function access 1836 device register values in cache & also
* calls another function (Access1836SPI) to access the 1836 Hardware
*
*********************************************************************/
static u32	Access1836Regs (
    ADI_DEV_PDD_HANDLE      PDDHandle,              // PDD handle
    u32                     Command,                // command ID
    void                    *Value                  // command specific value
) {	
    //	Pointer	to AD1836A device driver instance
	ADI_AD1836A *pAD1836A = (ADI_AD1836A *)PDDHandle;
	
	u8		rwFlag,index=0;
    u16  	RegAddr,RegData;
    u32  	regcount=0;
    u32 	Result = ADI_DEV_RESULT_SUCCESS;   	// default return value
        
    // holds address of application program table location being accessed
    ADI_DEV_ACCESS_REGISTER			*access_selective;
    ADI_DEV_ACCESS_REGISTER_BLOCK	*access_block;    
    ADI_DEV_ACCESS_REGISTER_FIELD	*access_field;  
    
    ADI_DEV_ACCESS_REGISTER			Access1836;

	if ((Command == ADI_DEV_CMD_REGISTER_WRITE)				||	
		(Command == ADI_DEV_CMD_REGISTER_TABLE_WRITE)		||
		(Command == ADI_DEV_CMD_REGISTER_BLOCK_WRITE)		||
    	(Command == ADI_DEV_CMD_REGISTER_FIELD_WRITE)		||
    	(Command == ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE))	
		// Set read/write flag to write mode
        rwFlag = 0;
	else
  		// Set read/write flag to read mode
        rwFlag = 1;
		
	switch (Command)
	{
		// CASE (Read block of AD1836A registers starting from first given address)
    	case(ADI_DEV_CMD_REGISTER_BLOCK_READ):
		// CASE (Write to a block of AD1836A registers starting from first given address)
    	case(ADI_DEV_CMD_REGISTER_BLOCK_WRITE):  
    		
    		// pointer to block access table
        	access_block = (ADI_DEV_ACCESS_REGISTER_BLOCK *) Value;   	
			// Load the register block start address
        	RegAddr = access_block->Address;

	   		while (regcount < access_block->Count)     // access all registers until regcount reaches zero
    	   	{
    	   		// Match the register address with the driver register cache
    	   		if ((Result = ValidateRegAddress(pAD1836A, RegAddr, rwFlag, &index ))!= ADI_DEV_RESULT_SUCCESS)
    	   			return Result;

	    		if (rwFlag)		// read mode
    	   		{     	   			
					// if the register is one of the ADC VOLUME registers
        			if ((RegAddr & 0xC000) == 0x8000)
        			{
	        			Access1836.Address = ((RegAddr & 0xF000) | 0x0800);	// Mark 1836 R/W bit to Read(1)
	        			// pass Access1836 structure to device access
	           			if ((Result = Access1836SPI(pAD1836A,ADI_DEV_CMD_REGISTER_READ,&Access1836)) != ADI_DEV_RESULT_SUCCESS)
        	   				return (Result);
        				// Pass the Register value to application array
           				*(access_block->pData+regcount) = (Access1836.Data & ADI_AD1836A_REG_VALUE_MASK);	        		
	        		}
					else	// Pass the Register value to the application array
           				*(access_block->pData+regcount) = (pAD1836A->ad1836aRegs[index] & ADI_AD1836A_REG_VALUE_MASK);
				}
				else		// write mode
				{
					// load the data from the application table & append its corresponding register address
					Access1836.Address = ((pAD1836A->ad1836aRegs[index] & ADI_AD1836A_ADDR_MASK) | (*(access_block->pData+regcount) & ADI_AD1836A_REG_VALUE_MASK));
           			// Pass the new register data to device access service which inturn configures AD1836A
           			if ((Result = Access1836SPI(pAD1836A,ADI_DEV_CMD_REGISTER_WRITE,&Access1836)) != ADI_DEV_RESULT_SUCCESS)
           				return (Result);
       				// Update the Register cache with the new value onlf if AD1836A access results in success
       				pAD1836A->ad1836aRegs[index] = ((pAD1836A->ad1836aRegs[index] & ADI_AD1836A_ADDR_MASK) | (*(access_block->pData+regcount) & ADI_AD1836A_REG_VALUE_MASK));
           		}
		
				++regcount;
			           					
				// Generate the next address to access
				if (regcount < access_block->Count)
				{
					// Generate the next address to access
					RegAddr += 0x1000;													
					// if the register address is among ADC VOLUME registers
           			if ((RegAddr & 0xC000) == 0x8000)
           			{
           				// Cannot write to these registers
           				// Load RegAddr with the next writeable address
           				if (!rwFlag)	RegAddr = AD1836A_ADC_CTRL_1;
           			}
				}
           	}         	
			break;

    	// CASE (Read a specific register from the AD1836A)
    	case (ADI_DEV_CMD_REGISTER_READ):
    	// CASE (Configure a specific register in the AD1836A)
    	case(ADI_DEV_CMD_REGISTER_WRITE):
    	// CASE (Read a table of selective register(s) in AD1836A)
    	case(ADI_DEV_CMD_REGISTER_TABLE_READ):    
    	// CASE (Configure a table of selective register(s) in AD1836A)
    	case(ADI_DEV_CMD_REGISTER_TABLE_WRITE):        

    		// pointer to selective access table
        	access_selective = (ADI_DEV_ACCESS_REGISTER *) Value;        
        	RegAddr = access_selective->Address;	// get the first register address to be accessed

       		while (RegAddr != ADI_DEV_REGEND)  // access all registers in the array until we reach the delimiter
       		{
    	   		// Match the register address with the driver register cache
    	   		if ((Result = ValidateRegAddress(pAD1836A, RegAddr, rwFlag, &index ))!= ADI_DEV_RESULT_SUCCESS)
    	   			return Result;
    	   			
	    		if (rwFlag)		// read mode
    	   		{     	   			
					// if the register is one of the ADC VOLUME registers
        			if ((RegAddr & 0xC000) == 0x8000)
        			{
	        			Access1836.Address = ((RegAddr & 0xF000) | 0x0800);	// Mark 1836 R/W bit to Read(1)
	        			// pass Access1836 structure to device access
	           			if ((Result = Access1836SPI(pAD1836A,ADI_DEV_CMD_REGISTER_READ,&Access1836)) != ADI_DEV_RESULT_SUCCESS)
        	   				return (Result);
        				// Pass the Register value to application array
           				access_selective->Data = (Access1836.Data & ADI_AD1836A_REG_VALUE_MASK);	        		
	        		}
					else	// Pass the Register value to the application array
           				access_selective->Data = (pAD1836A->ad1836aRegs[index] & ADI_AD1836A_REG_VALUE_MASK);
				}
				else		// write mode
				{
					// load the data from the application table & append its corresponding register address
					Access1836.Address = ((pAD1836A->ad1836aRegs[index] & ADI_AD1836A_ADDR_MASK) | (access_selective->Data & ADI_AD1836A_REG_VALUE_MASK));
           			// Pass the new register data to device access service which inturn configures AD1836A
           			if ((Result = Access1836SPI(pAD1836A,ADI_DEV_CMD_REGISTER_WRITE,&Access1836)) != ADI_DEV_RESULT_SUCCESS)
           				return (Result);
       				// Update the Register cache with the new value onlf if AD1836A access results in success
       				pAD1836A->ad1836aRegs[index] = ((pAD1836A->ad1836aRegs[index] & ADI_AD1836A_ADDR_MASK) | (access_selective->Data & ADI_AD1836A_REG_VALUE_MASK));
           		}

           		++access_selective;		// go to next array element
           	
           		// check if the access command is for single register or selective registers
           		if ((Command == ADI_DEV_CMD_REGISTER_WRITE)	||
           			(Command == ADI_DEV_CMD_REGISTER_READ))
           			// for single register access
           			RegAddr = ADI_DEV_REGEND;	// exit the while loop after accessing one register
           		else
           			// for selective register access
           			RegAddr = access_selective->Address;	// get the next register address to be accessed
       		}
			break;

	    // CASE (Read a specific field from a single device register)
    	case(ADI_DEV_CMD_REGISTER_FIELD_READ):
    	// CASE (Write to a specific field in a single device register)
    	case(ADI_DEV_CMD_REGISTER_FIELD_WRITE):
    	// CASE (Read a table of selective register field(s) in AD1836A)
    	case(ADI_DEV_CMD_REGISTER_FIELD_TABLE_READ):    
    	// CASE (Write to a table of selective register field(s) in AD1836A)
    	case(ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE):
          	
			// pointer to selective field access table
        	access_field = (ADI_DEV_ACCESS_REGISTER_FIELD *) Value;
			RegAddr = access_field->Address;	// get the first register address to be accessed
			         	
       		while (RegAddr != ADI_DEV_REGEND)  // access all registers in the array until we reach the delimiter
       		{
    	   		// Match the register address with the driver register cache
    	   		if ((Result = ValidateRegAddress(pAD1836A, RegAddr, rwFlag, &index ))!= ADI_DEV_RESULT_SUCCESS)
    	   			return Result;
    	   			
	    		if (rwFlag)		// read mode
    	   		{     	   			
					// if the register is one of the ADC VOLUME registers
        			if ((RegAddr & 0xC000) == 0x8000)
        			{
	        			Access1836.Address = ((RegAddr & 0xF000) | 0x0800);	// Mark 1836 R/W bit to Read(1)
	        			// pass Access1836 structure to device access
	           			if ((Result = Access1836SPI(pAD1836A,ADI_DEV_CMD_REGISTER_READ,&Access1836)) != ADI_DEV_RESULT_SUCCESS)
        	   				return (Result);
        				// Pass the Register value to application array
           				access_field->Data = (Access1836.Data & ADI_AD1836A_REG_VALUE_MASK);	        		
	        		}
	        		// if the Address is other than DAC Volume registers
					else if ((RegAddr < AD1836A_DAC_1L_VOL) || (RegAddr > AD1836A_DAC_3R_VOL))
					{
						// Load the present register value from the cache
						RegData = pAD1836A->ad1836aRegs[index];
						// Mask the Register Field
						Mask1836Field(RegAddr,&RegData,access_field->Field,rwFlag);
						// Update the Register field value to the Application array
						access_field->Data = RegData;
					}
					else	// Pass the Register value to the application array
           				access_field->Data = (pAD1836A->ad1836aRegs[index] & ADI_AD1836A_REG_VALUE_MASK);
				}
				else		// write mode
				{		
					// Load the register field data
					RegData = access_field->Data;
					// if the Address is other than DAC Volume registers & ADC Peak Level Registers
					if ((RegAddr < AD1836A_DAC_1L_VOL) || (RegAddr > AD1836A_ADC_2R_VOL))
					{
						// Load the present register value from the cache
						RegData = pAD1836A->ad1836aRegs[index];
						// Mask the Register Field
						Mask1836Field(RegAddr,&RegData,access_field->Field,rwFlag);
						// update RegData with the new Regfield value
						RegData |= (access_field->Data << access_field->Field);
					}
					// append RegData with its corresponding register address and pass it to device access
					Access1836.Address = ((pAD1836A->ad1836aRegs[index] & ADI_AD1836A_ADDR_MASK) | (RegData & ADI_AD1836A_REG_VALUE_MASK));
           			// Pass the new register data to device access service which inturn configures AD1836A
           			if ((Result = Access1836SPI(pAD1836A,ADI_DEV_CMD_REGISTER_WRITE,&Access1836)) != ADI_DEV_RESULT_SUCCESS)
           				return (Result);
       				// Update the Register cache with the new value onlf if AD1836A access results in success
       				pAD1836A->ad1836aRegs[index] = ((pAD1836A->ad1836aRegs[index] & ADI_AD1836A_ADDR_MASK) | (RegData & ADI_AD1836A_REG_VALUE_MASK));
           		}
           		
           		++access_field;	// go to next array element
           		
           		// check if the access command is for single register field or selective register fields
           		if ((Command == ADI_DEV_CMD_REGISTER_FIELD_READ)	||
           			(Command == ADI_DEV_CMD_REGISTER_FIELD_WRITE))
           			// for single register field access
           			RegAddr = ADI_DEV_REGEND;			// exit the while loop after accessing one register
           		else
           			// for selective register fields access
           			RegAddr = access_field->Address;	// get the next register address to be accessed           		
           	}
      		break;

		default:
		
		 	if (pAD1836A->sportHandle) // if SPORT device is already opened
    			// pass the unknown command to SPORT
				Result = adi_dev_Control( pAD1836A->sportHandle, Command, Value );
			else
				// return error indicating as the command is not valid
        		Result = ADI_AD1836A_RESULT_CMD_NOT_SUPPORTED;
        	break;
	}
	return (Result);
}

/*********************************************************************
*
* Function:  adi_pdd_Open
*
* Description: Opens the AD1836A device for use
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
    ADI_AD1836A  *pAD1836A;     // pointer to the AD1836A device we're working on
    void *pExitCriticalArg;     // exit critical region parameter
  
    // Check for a valid device number
#ifdef ADI_DEV_DEBUG
    if (DeviceNumber >= ADI_AD1836A_NUM_DEVICES) return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
#endif

    // assign the pointer to the device instance
    pAD1836A = &Device[DeviceNumber];
    //  store the Manager handle
    pAD1836A->ManagerHandle = ManagerHandle;
    //  store the Device Manager handle
    pAD1836A->DMHandle = DMHandle;
    //  store the DMA Manager handle
    pAD1836A->DMAHandle = DMAHandle;
    //  store the Deffered Callback Manager handle
    pAD1836A->DCBHandle = DCBHandle;
    //  callback function
    pAD1836A->DMCallback = DMCallback;
    // data direction
    pAD1836A->sportDirection = Direction;
	
    // insure the device the client wants is available    
    Result = ADI_DEV_RESULT_DEVICE_IN_USE;    
    // Check that this device instance is not already in use. 
    // If not, assign flag to indicate that it is now.
    pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
    if (pAD1836A->InUseFlag == FALSE) 
    {
        pAD1836A->InUseFlag = TRUE;
        Result = ADI_DEV_RESULT_SUCCESS;
    }
    adi_int_ExitCriticalRegion(pExitCriticalArg);
    
#ifdef ADI_DEV_DEBUG
    if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);  
#endif

    // save the physical device handle in the client supplied location
    *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pAD1836A;

 // return after successful completion
 return(Result);
}

/*********************************************************************
*
* Function:  adi_pdd_Close
*
* Description: Closes down a SPI & SPORT device 
*
*********************************************************************/

static u32 adi_pdd_Close(                           // Closes a device
    ADI_DEV_PDD_HANDLE      PDDHandle               // PDD handle
)   {

    // default return code
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    //	Pointer	to AD1836A device driver instance
	ADI_AD1836A *pAD1836A = (ADI_AD1836A *)PDDHandle;

 // check for errors if required
#ifdef ADI_DEV_DEBUG
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

	// Close the present SPORT device being used
	Result = SPORT_Close(pAD1836A);

    // mark the device as closed
    pAD1836A->InUseFlag = FALSE;

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

    ADI_AD1836A  *pAD1836A = (ADI_AD1836A *)PDDHandle;  

#ifdef ADI_DEV_DEBUG
    // check for errors if required
    u32 Result = ADI_DEV_RESULT_SUCCESS;
	if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    // pass read operation to SPORT
    return(adi_dev_Read(pAD1836A->sportHandle,BufferType,pBuffer));

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
    
    //	Pointer	to AD1836A device driver instance
	ADI_AD1836A *pAD1836A = (ADI_AD1836A *)PDDHandle;
    
#ifdef ADI_DEV_DEBUG
	// check for errors if required
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    // pass write operation to SPORT
    return(adi_dev_Write(pAD1836A->sportHandle,BufferType,pBuffer));
 
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
* Description: AD1836A command implementation
*
*********************************************************************/
    
static u32 adi_pdd_Control(                         // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE      PDDHandle,              // PDD handle
    u32                     Command,                // command ID
    void                    *Value                  // command specific value
) {

    ADI_AD1836A    	*pAD1836A; 	// pointer to the device we're working on
    u32     		u32Value;  	// u32 type to avoid casts/warnings etc.
    u8      		u8Value;    // u8 type to avoid casts/warnings etc.  

    u32 Result = ADI_DEV_RESULT_SUCCESS;   	// return value
        
    // avoid casts
    pAD1836A = (ADI_AD1836A *)PDDHandle; // Pointer to AD1836A device driver instance
    // assign 8, 16 and 32 bit values for the Value argument
    u32Value = (u32) Value;
    u8Value  = (u8) u32Value;
	
    // check for errors if required
#ifdef ADI_DEV_DEBUG
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif
        	
    switch (Command)    
    {

    	// CASE (control dataflow)
    	case (ADI_DEV_CMD_SET_DATAFLOW):
    	
     		// check if the SPORT device is already open
     		if (pAD1836A->sportHandle != NULL)
     		{
     			// if so, Enable/Disable the SPORT device used by AD1836A
     			Result = adi_dev_Control( pAD1836A->sportHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)u8Value );
			}
			// else check if the client is trying to enable dataflow without opening the SPORT
			else if (u8Value == TRUE)
			{
				// can not pass this value to SPORT as no valid SPORT handle is available
				// which means the client hasn't defined the dataflow method yet, return error.
 				return (ADI_DEV_RESULT_DATAFLOW_UNDEFINED);
 			}
 			
         	pAD1836A->DataflowStatus = u8Value;	// Save the dataflow status
         		
        break;

	    // CASE (query for processor DMA support)
    	case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
  
        	// AD1836A doesn't support DMA, but supports indirectly thru SPORT operation
        	*((u32 *)Value) = FALSE;
   
        	break;

    	// CASE (Set Dataflow method - applies only for SPORT)
    	case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):

			// Check if sport device to used by AD1836A is already open
			if (pAD1836A->sportHandle == NULL)
				// if not, try to open the SPORT device corresponding to sportDeviceNumber for AD1836A dataflow
				Result = SPORT_Open(pAD1836A);

// on occurance	of error, return the error code
#ifdef ADI_DEV_DEBUG
	if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

			// Pass	the	dataflow method	to the SPORT device allocated to AD1836A
			Result = adi_dev_Control( pAD1836A->sportHandle, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void*)u32Value );
		    	
        	break;	
        	      
/************************
SPI related commands
************************/
                
    	// CASE (Set AD1836A SPI Chipselect)
    	case ADI_AD1836A_CMD_SET_SPI_CS:
 
        	pAD1836A->ad1836a_spi_cs = u8Value;
         
        	break;
   
    	// CASE (Get AD1836A SPI Chipselect value)
    	case ADI_AD1836A_CMD_GET_SPI_CS:
   
        	*((u8 *)Value) = pAD1836A->ad1836a_spi_cs;

        	break;
        	
    	// CASE (Set SPI Device number to be used to access AD1836A registers)
    	case ADI_AD1836A_CMD_SET_SPI_DEVICE_NUMBER:
    	
			pAD1836A->ad1836a_spi_device = u8Value;

        	break;

/************************
SPORT related commands
************************/

    	// CASE (Set SPORT Device Number that will be used for audio dataflow between Blackfin and AD1836A)
    	case (ADI_AD1836A_CMD_SET_SPORT_DEVICE_NUMBER):
       
			// Close the present SPORT device being used
			Result = SPORT_Close(pAD1836A);
			
        	// Update the SPORT device number
        	pAD1836A->sportDeviceNumber = u8Value;

			// Application should set dataflow method for this SPORT device, load data buffer(s) & enable dataflow

        	break;

			        
    	// CASE (OPEN/CLOSE SPORT Device to be used for audio dataflow between Blackfin and AD1836A)
    	case (ADI_AD1836A_CMD_SET_SPORT_STATUS):

			// check the SPORT mode
			if ((ADI_AD1836A_SET_SPORT_STATUS) Value == ADI_AD1836A_SPORT_OPEN)
			{
				// check if the SPORT device is already open
				if (pAD1836A->sportHandle == NULL)
					// if not, OPEN	Sport Device, else do nothing
					Result = SPORT_Open(pAD1836A);
			}			
			else	// this should be to close the SPORT device
				// Close any SPORT device presently being used by AD1836A
				Result = SPORT_Close(pAD1836A);
			
			break;

    	// CASE (Enable/Disable Auto-SPORT configuration mode)
    	case (ADI_AD1836A_CMD_ENABLE_AUTO_SPORT_CONFIG):
					
			pAD1836A->AutoSportEnable = u8Value;
			
			break;
						
		// Command(s) no longer used/supported by this driver. Left here for backward compatibility
		case (ADI_AD1836A_CMD_SET_SPORT_OPERATION_MODE):
			break;
			
    	default:
   
   			// Call AD1836A Register access function
        	Result = Access1836Regs(PDDHandle,Command,Value);
   
        	break;
    }
	
    return(Result);
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
    ADI_AD1836A *pAD1836A = (ADI_AD1836A *)DeviceHandle;

#ifdef ADI_DEV_DEBUG
	// verify the DM handle
    if (ValidatePDDHandle(DeviceHandle) != ADI_DEV_RESULT_SUCCESS) 
        return;
#endif

    // simply pass the callback along to the Device Manager Callback without interpretation
    (pAD1836A->DMCallback)(pAD1836A->DMHandle,Event,pArg);
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
 	for (i = 0; i < ADI_AD1836A_NUM_DEVICES; i++) 
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

ADI_DEV_PDD_ENTRY_POINT ADIAD1836AEntryPoint = {
	adi_pdd_Open,
 	adi_pdd_Close,
 	adi_pdd_Read,
 	adi_pdd_Write,
 	adi_pdd_Control,
 	adi_pdd_SequentialIO
};

/*****/

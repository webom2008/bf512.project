/**********************************************************************

Copyright(c) Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software 
you agree to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_max1233.c,v $

Description:
    This is the driver source code for MAX1233 Touchscreen Controller.
    Use Device access commands to access MAX1233 device registers 
    (refer to adi_dev.h)

***********************************************************************/
/*********************************************************************

Include files

*********************************************************************/

#include <services/services.h>						/* system service includes  */
#include <drivers/adi_dev.h>						/* device manager includes  */
#include <drivers/spi/adi_spi.h>					/* pickup SPI config macros */
#include <drivers/deviceaccess/adi_device_access.h> /* adi_device_access driver includes */
#include <drivers/touchscreen/adi_max1233.h>		/* MAX1233 driver includes   */

/*********************************************************************

Enumerations and defines

*********************************************************************/

 /* number of MAX1233 devices in the system  */
#define MAX1233_NUM_DEVICES ( sizeof(MaxDevice) / sizeof(MaxDevice[0]))

#define MAX1233_FINAL_REG_ADDRESS MAX1233_REG_COL_MASK

/*********************************************************************

Data Structures

*********************************************************************/

/* Device Driver Control Struct */
typedef struct {                                 /* MAX1233 device structure                    */

    /* Driver-Related Fields...                                                                 */
    MAX1233_INTERRUPT_PORT  *pPenIrqPort;        /* Port info for PENIRQ interrupt signal       */
    MAX1233_INTERRUPT_PORT 	*pKeyIrqPort;        /* Port info for KEYIRQ interrupt signal       */
    u8                       SpiDeviceNumber;    /* Holds SPI Device Number to use              */
    u8                       SpiChipSelect;      /* Holds SPI Chipselect                        */
    u16                      SpiSlaveSelect;     /* Programs Kookaburra/Mockingbird SPI select  */
    u8                       InUseFlag;          /* in use flag (in use when TRUE)              */
    void                    *pEnterCriticalArg;  /* critical region argument                    */
    ADI_DEV_MANAGER_HANDLE   ManagerHandle;      /* Device Manager Handle                       */
    ADI_DEV_DEVICE_HANDLE    DeviceHandle;       /* Device handle                               */
    ADI_DCB_HANDLE           DCBHandle;          /* callback handle                             */
    ADI_DCB_CALLBACK_FN      DMCallback;         /* Device manager's Callback function          */
    ADI_DEV_PDD_HANDLE	     SpiHandle;          /* Handle to SPI device (driver)               */
} MAX1233_DEVICE_STRUCT;



/*********************************************************************

Device specific initialization data

*********************************************************************/

/********************
    Kookaburra/Mockingbird
********************/

#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)

/* The default values for the device instance */
static MAX1233_DEVICE_STRUCT MaxDevice[] = {
    {
		NULL,                                   /* pen port pointer                         */
		NULL,                                   /* kep port pointer                         */
        0,                                      /* uses SPI 0 - only choice on 527          */
        0,                                      /* No SPI Chipselect by default             */
        0,                                      /* SPISSEL number                           */
        FALSE,                                  /* in use flag                              */
        NULL,                                   /* critical region argument                 */
        NULL,                                   /* Device manager handle                    */
        NULL,                                   /* Device handle                            */
        NULL,                                   /* callback handle                          */
        NULL,                                   /* Device manager Callback function         */
        NULL,                                   /* Handle to SPI device (driver)            */
    },
};

#else
#error "MAX1233 Device instance is yet to be defined for this processor"
#endif

/*********************************************************************

Static functions

*********************************************************************/
/********************************
Device Manager entry
********************************/
static u32 adi_pdd_Open(                        /* Opens MAX1233 device                     */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* Device Manager handle                    */
    u32                     DeviceNumber,       /* EPPI Device number to open               */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* Device handle                            */
    ADI_DEV_PDD_HANDLE     *pPDDHandle,         /* pointer to PDD handle location           */
    ADI_DEV_DIRECTION       Direction,          /* data direction                           */
    void                   *pEnterCriticalArg,  /* critical region storage location         */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager                */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle                          */
    ADI_DCB_CALLBACK_FN     DMCallback          /* device manager callback function         */
);

static u32 adi_pdd_Close(                       /* Closes a MAX1233 device                  */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of the EPPI device to close   */
);

static u32 adi_pdd_Read(                        /* Reads data/queues inbound buffer         */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a device                   */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER         *pBuffer             /* pointer to buffer                        */
);
    
static u32 adi_pdd_Write(                       /* Writes data/queues outbound buffer       */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a device                   */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER         *pBuffer             /* pointer to buffer                        */
);

static u32 adi_pdd_SequentialIO(                /* Sequentially read/writes data to a device*/ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a device                   */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER         *pBuffer             /* pointer to buffer                        */
);

static u32 adi_pdd_Control(                     /* Sets or senses device specific parameter */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a MAX1233 device           */
    u32                     Command,            /* Command ID                               */
    void                   *pVal                /* Command specific value                   */
);

/********************************
Callback functions
********************************/

static void PenIrqCallback (                    /* Callback for MAX1233 PENIRQ Interrupt    */
	void  	               *DeviceHandle,       /* Device handle                            */
	u32 	                Event,              /* callback Event ID                        */
	void  	               *pArg                /* Callback argument                        */
);

static void KeyIrqCallback (                    /* Callback for MAX1233 DAV Interrupt       */
	void* 	                DeviceHandle,       /* Device handle                            */
	u32 	                Event,              /* callback Event ID                        */
	void* 	                pArg                /* Callback argument                        */
);

static void SpiCallback (                       /* SPI callback routine                     */
	void  	               *DeviceHandle,       /* Device handle                            */
	u32 	                Event,              /* callback Event ID                        */
	void  	               *pArg                /* Callback argument                        */
);

/*********************************************************************

Debug Mode functions (debug build only)

*********************************************************************/
#if defined(ADI_DEV_DEBUG)

static u32 ValidatePDDHandle(                   /* Validates Physical Device Driver Handle  */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of a MAX1233 device           */
);

#endif

/*********************************************************************

Entry point for device manager

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT ADIMAX1233EntryPoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,    
    adi_pdd_Control,
    adi_pdd_SequentialIO
};


/* Configuration Table to for SPI internals */
ADI_DEV_CMD_VALUE_PAIR SpiBaudRateConfig[] = {
	{ ADI_SPI_CMD_SET_BAUD_REG, 		(void*)0		},  // if zero, use system default
	{ ADI_DEV_CMD_END, 					NULL    		}
};

	
// Special tables to describe bitfields, and resrved bits \\

/*********************************************************************
Table for Register Field Error check and Register field access
Table structure - 	'Count' of Register addresses containing individual fields
					Register address containing individual fields, 
					Register field locations in the corresponding register, 
					Reserved bit locations in the corresponding register
Register field location - Bit indicating start of new field in a register will be 1
Reserved bit locations will be 1
*********************************************************************/

// MAX1233 register address to perform Register Field Error check and Register field access
static u16 MAX1233BitfieldRegs [] = {
	MAX1233_REG_ADC,
	MAX1233_REG_KEY,
	MAX1233_REG_DAC_CNTL,
	MAX1233_REG_GPIO_PULL,
	MAX1233_REG_GPIO_CFG,
	MAX1233_REG_KEY_MASK,
	MAX1233_REG_COL_MASK,
};

// Register Field locations corresponding to MAX1233BitfieldRegs (***LS bit of each bitfield set as 1***)
static u16 MAX1233BitfieldMap [] = {
	0x4553,
	0x4900,
	0x8000,
	0x0100,
	0x0101,
	0x0001,
	0x1000,
};

static ADI_DEVICE_ACCESS_REGISTER_FIELD BitfieldTable[] = {
	sizeof(MAX1233BitfieldRegs)/sizeof(MAX1233BitfieldRegs[0]),	// 'Count' of Register addresses containing individual fields
	MAX1233BitfieldRegs,										// array of MAX1233 register addresses containing individual fields
	MAX1233BitfieldMap											// array of valid register field locations in the corresponding registers
};


/*********************************************************************
Table to configure the reserved bits in the device to its recommended values
Table structure -  'Count' of Register addresses containing Reserved Locations
     Register address containing Reserved Locations
     Recommended value for the Reserved Locations
*********************************************************************/

// MAX1233 register address having reserved locations
static u16 MAX1233ReservedBitRegs [] = {
	MAX1233_REG_X,
	MAX1233_REG_Y,
	MAX1233_REG_Z1,
	MAX1233_REG_Z2,
	MAX1233_REG_KPD,
	MAX1233_REG_BAT1,
	MAX1233_REG_BAT2,
	MAX1233_REG_AUX1,
	MAX1233_REG_AUX2,
	MAX1233_REG_TEMP1,
	MAX1233_REG_TEMP2,
	MAX1233_REG_DAC,
	MAX1233_REG_GPIO,
	MAX1233_REG_KEY,
	MAX1233_REG_DAC_CNTL,
	MAX1233_REG_GPIO_PULL,
	MAX1233_REG_COL_MASK
};

// Reserved Bit locations corresponding to the entries in MAX1233ReservedBitRegs
static u16 MAX1233ReservedBitPos [] = {
	0xF000,	// MAX1233_REG_X
	0xF000,	// MAX1233_REG_Y
	0xF000,	// MAX1233_REG_Z1
	0xF000,	// MAX1233_REG_Z2
	0xF000,	// MAX1233_REG_KPD
	0xF000,	// MAX1233_REG_BAT1
	0xF000,	// MAX1233_REG_BAT2
	0xF000,	// MAX1233_REG_AUX1
	0xF000,	// MAX1233_REG_AUX2
	0xF000,	// MAX1233_REG_TEMP1
	0xF000,	// MAX1233_REG_TEMP2
	0xFF00,	// MAX1233_REG_DAC
	0x00FF,	// MAX1233_REG_GPIO
	0x00FF,	// MAX1233_REG_KEY
	0x7FFF,	// MAX1233_REG_DAC_CNTL
	0x00FF,	// MAX1233_REG_GPIO_PULL
	0x0FFF,	// MAX1233_REG_COL_MASK
};

// Value of Reserved Bit locations corresponding to the entries in MAX1233ReservedBitRegs
static u16 MAX1233ReservedBitVal [] = {
	0x0000,	// MAX1233_REG_X
	0x0000,	// MAX1233_REG_Y
	0x0000,	// MAX1233_REG_Z1
	0x0000,	// MAX1233_REG_Z2
	0x0000,	// MAX1233_REG_KPD
	0x0000,	// MAX1233_REG_BAT1
	0x0000,	// MAX1233_REG_BAT2
	0x0000,	// MAX1233_REG_AUX1
	0x0000,	// MAX1233_REG_AUX2
	0x0000,	// MAX1233_REG_TEMP1
	0x0000,	// MAX1233_REG_TEMP2
	0x0000,	// MAX1233_REG_DAC
	0x0000,	// MAX1233_REG_GPIO
	0x0000,	// MAX1233_REG_KEY
	0x0000,	// MAX1233_REG_DAC_CNTL
	0x0000,	// MAX1233_REG_GPIO_PULL
	0x0000,	// MAX1233_REG_COL_MASK
};
																		
static ADI_DEVICE_ACCESS_RESERVED_VALUES ReservedBitTable[] = {
	sizeof(MAX1233ReservedBitRegs)/sizeof(MAX1233ReservedBitRegs[0]),	// 'Count' of Register addresses containing Reserved Locations
	MAX1233ReservedBitRegs,												// array of MAX1233 register addresses containing individual fields
 	MAX1233ReservedBitPos,												// array of reserved bit locations in the corresponding register	
	MAX1233ReservedBitVal												// array of Recommended values for the Reserved Bit locations
};


/*********************************************************************
Table for MAX1233 Register Error check for invalid & read-only register(s) access
Structure - 'Count' of invalid Register addresses in MAX1233
			Invalid Register addresses in MAX1233
			'Count' of Read-only Register addresses in MAX1233
			Read-only Register addresses in MAX1233
*********************************************************************/

static u16 MAX1233InvalidRegs [] = {
	//	invalid max1233 register adrsses are: 0x0C-0x0E, 0x12-0x1F, 0x43-0x4D, 0x52-0x5F
	0x000C, 0x000D, 0x000E,
	0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001E, 0x001F,
	0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D,
	0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005F
};

static u16 MAX1233ReadOnlyRegs [] = {
	// All ADC conversion result registers are read-only
	 MAX1233_REG_X,
	 MAX1233_REG_Y,
	 MAX1233_REG_Z1,
	 MAX1233_REG_Z2,
	 MAX1233_REG_KPD,
	 MAX1233_REG_BAT1,
	 MAX1233_REG_BAT2,
	 MAX1233_REG_AUX1,
	 MAX1233_REG_AUX2,
	 MAX1233_REG_TEMP1,
	 MAX1233_REG_TEMP2,
	 MAX1233_REG_DAC,
	 MAX1233_REG_GPIO,
	 MAX1233_REG_KPDATA1,
	 MAX1233_REG_KPDATA2,
};

static ADI_DEVICE_ACCESS_VALIDATE_REGISTER InvalidAndReadOnlyTable [] = {
	sizeof(MAX1233InvalidRegs)/sizeof(MAX1233InvalidRegs[0]),	// 'Count' of invalid register addresses
	MAX1233InvalidRegs,											// Pointer to array of invalid register addresses
	sizeof(MAX1233ReadOnlyRegs)/sizeof(MAX1233ReadOnlyRegs[0]),	// 'Count' of read-only register addresses
	MAX1233ReadOnlyRegs											// Pointer to array of read-only register addresses
};


/*********************************************************************
*
*   Function:       adi_pdd_Open
*
*   Description:    Opens a MAX1233 device for use
*
*********************************************************************/

static u32 adi_pdd_Open(                        /* Open MAX1233 device              */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* Device Manager handle            */
    u32                     DeviceNumber,       /* MAX1233 Device number to open    */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* Device handle                    */
    ADI_DEV_PDD_HANDLE     *pPDDHandle,         /* pointer to PDD handle location   */
    ADI_DEV_DIRECTION       Direction,          /* data direction                   */
    void                   *pEnterCriticalArg,  /* critical region storage location */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager        */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle                  */
    ADI_DCB_CALLBACK_FN     DMCallback          /* device manager callback function */
) 
{

    u32                    Result = ADI_DEV_RESULT_SUCCESS; /* Return value - assume we're going to be successful */
    MAX1233_DEVICE_STRUCT *pMaxDev;                            /* pointer to the device we're working on             */
    void                  *pExitCriticalArg;                /* exit critical region parameter                     */

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    if (DeviceNumber >= MAX1233_NUM_DEVICES)                 /* check the device number */
    {
        Result = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;           /* Invalid Device number */
    }
    
    /*  Continue only if the device number is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* insure the device the client wants is available  */
        Result  = ADI_DEV_RESULT_DEVICE_IN_USE;
        
        /* MAX1233 device we're working on */
        pMaxDev = &MaxDevice[DeviceNumber];
    
        /* Protect this section of code - entering a critical region    */
        pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
        
        /* Check the device usage status */
        if (pMaxDev->InUseFlag == FALSE) 
        {
            /* Device is not in use. Reserve the device for this client */
            pMaxDev->InUseFlag = TRUE;
            Result = ADI_DEV_RESULT_SUCCESS;
        }
        
        /* Exit the critical region */
        adi_int_ExitCriticalRegion(pExitCriticalArg);
    
        /* Continue only when the MAX1233 device is available for use  */
        if (Result == ADI_DEV_RESULT_SUCCESS) 
        {    
            /* MAX1233 device is reserved now */

            /* No SPI config by default */
            pMaxDev->SpiChipSelect   = 0;
            pMaxDev->SpiDeviceNumber = 0;
            pMaxDev->SpiSlaveSelect  = 0;
            
            /* no interrupt ports by default */
			pMaxDev->pPenIrqPort = NULL;
			pMaxDev->pKeyIrqPort = NULL;
            
            /* initialize the device driver settings */
            
            /* Pointer to critical region */
            pMaxDev->pEnterCriticalArg  = pEnterCriticalArg;
            
            /* Pointer to Device manager handle */
            pMaxDev->ManagerHandle      = ManagerHandle;
            
            /* Pointer to Device handle */
            pMaxDev->DeviceHandle       = DeviceHandle;
            
            /* Pointer to Deffered Callback manager handle */
            pMaxDev->DCBHandle          = DCBHandle;
            
            /* Pointer to Device manager callback function  */
            pMaxDev->DMCallback         = DMCallback;
            
            /* save the physical device handle in the client supplied location */
            *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pMaxDev;
        }
        
/* for Debug build only */
#if defined(ADI_DEV_DEBUG)        
    }
#endif
    
    /* return */
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Close
*
*   Description:    Closes down a MAX1233 device
*
*********************************************************************/

static u32 adi_pdd_Close(               /* Closes a MAX1233 device                     */
    ADI_DEV_PDD_HANDLE      PDDHandle   /* PDD handle of the MAX1233 device to close   */
)
{  
    u32                     Result = ADI_DEV_RESULT_SUCCESS; /* Return value - assume we're going to be successful   */
    MAX1233_DEVICE_STRUCT  *pMaxDev;                         /* pointer to the device we're working on */
    
    /* Configuration table to disable MAX1233 */
    ADI_DEV_ACCESS_REGISTER DisableMAX1233[] =
    {	
    	{MAX1233_REG_ADC,       0},
    	{MAX1233_REG_KEY,       0},
    	{MAX1233_REG_DAC_CNTL,  0},
    	{MAX1233_REG_GPIO_PULL, 0},
    	{MAX1233_REG_GPIO_CFG,  0},
    	{MAX1233_REG_KEY,       0},
    	{MAX1233_REG_KEY_MASK,  0},
    	{MAX1233_REG_COL_MASK,  0},
    	{ADI_DEV_REGEND,        0}
    };
    
/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    /* Validate the given PDDHandle */
    Result = ValidatePDDHandle(PDDHandle);
    
    /* Continue only if the given PDDHandle is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {    
#endif
       
        /* avoid casts */
        pMaxDev = (MAX1233_DEVICE_STRUCT *) PDDHandle;
        
        /* Unhook all MAX1233 interrupts */
        
		/* Disable PENIRQ Interrupt Flag callback  */
		if (pMaxDev->pPenIrqPort) {
			Result = adi_flag_RemoveCallback(pMaxDev->pPenIrqPort->FlagId);
			pMaxDev->pPenIrqPort = NULL;
		}
		
		/* Disable KEYIRQ Interrupt Flag callback  */
		if (pMaxDev->pKeyIrqPort) {
			Result = adi_flag_RemoveCallback(pMaxDev->pKeyIrqPort->FlagId);
			pMaxDev->pKeyIrqPort = NULL;
		}
        
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
        	/* disable MAX1233 */
        	Result = adi_dev_Control(pMaxDev->DeviceHandle, ADI_DEV_CMD_REGISTER_TABLE_WRITE, (void *)DisableMAX1233);
        	
            /* mark this MAX1233 device as closed */
            pMaxDev->InUseFlag = FALSE;
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Read
*
*   Description:    Never called as MAX1233 uses Device Access commands
*                   to access its internal registers
*
*********************************************************************/
static u32 adi_pdd_Read(                /* Not supported by MAX1233 */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle               */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer        */
){
    /* MAX1233 uses Device Access commands to access its registers  */
    return(ADI_MAX1233_RESULT_CMD_NOT_SUPPORTED);
}
 
/*********************************************************************
*
*   Function:       adi_pdd_Write
*
*   Description:    Never called as MAX1233 uses Device Access commands
*                   to access its internal registers
*
*********************************************************************/
static u32 adi_pdd_Write(               /* Not supported by MAX1233 */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle               */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer        */
){
    /* MAX1233 uses Device Access commands to access its registers  */
    return(ADI_MAX1233_RESULT_CMD_NOT_SUPPORTED);
}

/*********************************************************************
*
*   Function:       adi_pdd_SequentialIO
*
*   Description:    Never called as MAX1233 uses Device Access commands
*                   to access its internal registers
*
*********************************************************************/
static u32 adi_pdd_SequentialIO(        /* Not supported by MAX1233 */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle               */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer        */
){
    /* MAX1233 uses Device Access commands to access its registers  */
    return(ADI_MAX1233_RESULT_CMD_NOT_SUPPORTED);
}

/*********************************************************************
*
*   Function:       adi_pdd_Control
*
*   Description:    Senses or Configures MAX1233 device registers
*
*********************************************************************/
static u32 adi_pdd_Control(             /* Sets or senses device specific parameter */
    ADI_DEV_PDD_HANDLE  PDDHandle,      /* PDD handle of a MAX1233 device           */
    u32                 Command,        /* Command ID                               */
    void                *pVal           /* Command specific value                   */
){
    MAX1233_DEVICE_STRUCT      *pMaxDev;    /*pointer to the device we're working on*/
    u32                         Result;     /* return value                         */
    u32                         u32Value;
    MAX1233_INTERRUPT_PORT     *pFlagPort;  /* pointer to Flag info structure       */
    
	// Structure passed to device access service
	ADI_DEVICE_ACCESS_REGISTERS AccessParams;

    /* assume we're going to be successful */
    Result  = ADI_DEV_RESULT_SUCCESS;
            
    /* avoid casts */
    pMaxDev     = (MAX1233_DEVICE_STRUCT *)PDDHandle;
    
    /* assign 32 bit values for the pVal argument */
    u32Value    = (u32)pVal;
    
	// Structure passed to manage device access service
	ADI_DEVICE_ACCESS_SELECT SelectAccess [] = 
	{
		pMaxDev->SpiChipSelect,			// SPI Chip-select Value
		ADI_DEVICE_ACCESS_LENGTH0,		// 'Device' Global address (no global address)
		ADI_DEVICE_ACCESS_LENGTH2,		// 'Device' register address length (2 bytes)
		ADI_DEVICE_ACCESS_LENGTH2,		// 'Device' register data length (2 byte)
		ADI_DEVICE_ACCESS_TYPE_SPI_2,	// Configure r/w bit at control word bit-15
	};

	/* for Debug build only - check for errors if required */
#if defined(ADI_DEV_DEBUG)
    /* Validate the given PDDHandle */
    Result = ValidatePDDHandle(PDDHandle);
    
    /* Continue only if the given PDDHandle is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {    
#endif
        /* CASEOF (Command ID)  */
        switch (Command)
        {

/***************************************
Device Manager commands
***************************************/

            /* CASE: control dataflow */
            case (ADI_DEV_CMD_SET_DATAFLOW):
            
            /* CASE: set dataflow method */
            case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):
                /* Do nothing & simply return back for these commands */
                break;

	        /* CASE: query for processor DMA support */
    	    case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
  
        	    /* MAX1233 does not support DMA */
        	    *((u32 *)pVal) = FALSE;   
        	    break;
        	
/***************************************
SPI related commands
***************************************/

    	    /* CASE (Set MAX1233 SPI Device Number to use) */
    	    case (ADI_MAX1233_CMD_SET_SPI_DEVICE_NUMBER):
                /* Save the SPI device number to use */
        	    pMaxDev->SpiDeviceNumber   = (u8)u32Value;
        	    break;
        	
    	    /* CASE (Set MAX1233 SPI Chipselect) */
    	    case (ADI_MAX1233_CMD_SET_SPI_CS):
                /* Save the SPI chipselect for MAX1233 */
        	    pMaxDev->SpiChipSelect  = (u8)u32Value;
        	    break;

            /* CASE (Set MAX1233 SPI Slave Select Number -- SPISSEL#) */
    	    case (ADI_MAX1233_CMD_SET_SPI_SLAVE):
                /* Save the SPI chipselect for MAX1233 */
        	    pMaxDev->SpiSlaveSelect  = (u16)u32Value;
        	    break;

            /* CASE (Set SPI Baud Rate Register) */
    	    case (ADI_MAX1233_CMD_SET_SPI_BAUD):
                /* Save the SPI Baudrate value for MAX1233 SPI communications */
                
                // get the value from the pointer
                u32Value = *((u32*)pVal);
                
                /* validate the command ID and new baud rate value */
                if ((SpiBaudRateConfig[0].CommandID != ADI_SPI_CMD_SET_BAUD_REG)
                	|| (0 == u32Value)	// value 0 disables SPI clock... value not allowed
                	|| (1 == u32Value))	// value 1 disables SPI clock... value not allowed
                {
                	Result = ADI_MAX1233_RESULT_CMD_BAD_BAUD_RATE;	// flag bad command
                } else {
                	SpiBaudRateConfig[0].Value = (void*)u32Value;	// save new value
                }
        	    break;

/***************************************
Commands to Manage Interrupt Monitoring
***************************************/

            /* CASE: Enable PENIRQ (allow the driver to monitor the pendown (touch) interrupt signal) */
            case (ADI_MAX1233_CMD_INSTALL_PENIRQ):
            	// uninstall if existing
            	if (pMaxDev->pPenIrqPort) {
					if (ADI_FLAG_RESULT_SUCCESS != (Result = adi_flag_RemoveCallback(pMaxDev->pPenIrqPort->FlagId)))
            		    break;
					pMaxDev->pPenIrqPort = NULL;
           	}
            	

                /* Map PENIRQ Interrupt to this Flag */
                pFlagPort = (MAX1233_INTERRUPT_PORT *)pVal;
                
                /* open corresponding Blackfin flag */
                if ((Result = adi_flag_Open (pFlagPort->FlagId)) != ADI_FLAG_RESULT_SUCCESS)
                    break;  /* exit on error */
                
                /* set flag direction as input */
                if ((Result = adi_flag_SetDirection(pFlagPort->FlagId, ADI_FLAG_DIRECTION_INPUT)) != ADI_FLAG_RESULT_SUCCESS)
                    break;  /* exit on error */
                 
        		/* set flag trigger to sense interrupt & generate callback                          */
                /* initially senses on falling edge transition                                      */
                /* we switch between falling edge and rising edge triggering in the PENIRQ callback */
                /* to facilitate synthesis of PENDOWN and PENUP events (touch and release)          */
                /* note that the keypad interrupt is strictly level-sensitive                       */
                if ((Result = adi_flag_InstallCallback (  
                                pFlagPort->FlagId,              /* Flag ID                 */
                                pFlagPort->FlagIntId,           /* Peripheral Interrupt ID */
                                ADI_FLAG_TRIGGER_LEVEL_LOW,  	/* Flag trigger mode       */
        		                TRUE,							/* this is a wakeup flag   */
                                (void *)pMaxDev,	            /* Client param            */
                                pMaxDev->DCBHandle,	            /* Callback Manager        */
                                PenIrqCallback				    /* PENIRQ flag callback    */
                    )) == ADI_FLAG_RESULT_SUCCESS)
                {
              	
		        	// Disable PENIRQ initially until we get detect command
		        	if (ADI_DEV_RESULT_SUCCESS != (Result = adi_int_SICDisable(pFlagPort->FlagIntId)))
		        		break;  /* exit on error */
  		
		            /* Save the Flag info */
		            pMaxDev->pPenIrqPort = pFlagPort;
                }
                break;

            /* CASE: Enable KEYIRQ (allow the driver to monitor the keypress interrupt signal) */
            case (ADI_MAX1233_CMD_INSTALL_KEYIRQ):
            	// uninstall if existing
            	if (pMaxDev->pKeyIrqPort) {
					if (ADI_FLAG_RESULT_SUCCESS != (Result = adi_flag_RemoveCallback(pMaxDev->pKeyIrqPort->FlagId)))
            		    break;
					pMaxDev->pKeyIrqPort = NULL;
            	}

                /* Map KEYIRQ Interrupt to this Flag */
                pFlagPort = (MAX1233_INTERRUPT_PORT *)pVal;
                
                /* open corresponding Blackfin flag */
                if ((Result = adi_flag_Open (pFlagPort->FlagId)) != ADI_FLAG_RESULT_SUCCESS)
                    break;  /* exit on error */

                /* set flag direction as input */
                if ((Result = adi_flag_SetDirection(pFlagPort->FlagId, ADI_FLAG_DIRECTION_INPUT)) != ADI_FLAG_RESULT_SUCCESS)
                    break;  /* exit on error */
                
                /* set flag trigger to sense interrupt & generate callback */
                /* active low */
                if ((Result = adi_flag_InstallCallback (  
                                pFlagPort->FlagId,              /* Flag ID                 */
                                pFlagPort->FlagIntId,           /* Peripheral Interrupt ID */
                                ADI_FLAG_TRIGGER_LEVEL_LOW,		/* Flag trigger mode       */
        		        		TRUE,							/* this is a wakeup flag   */
                                (void *)pMaxDev,	            /* Client param            */
                                pMaxDev->DCBHandle,	            /* Callback Manager        */
                                KeyIrqCallback		            /* KEYIRQ flag callback    */
                    )) == ADI_FLAG_RESULT_SUCCESS)
                {
              
					// Disable KEYIRQ initially until we get detect command
					if (ADI_DEV_RESULT_SUCCESS != (Result = adi_int_SICDisable(pFlagPort->FlagIntId)))
						break;  /* exit on error */

					/* Save the Flag info */
					pMaxDev->pKeyIrqPort = pFlagPort;
                }
                break;
                
            /* CASE: Uninstall PENIRQ handeling */
            case (ADI_MAX1233_CMD_UNINSTALL_PENIRQ):
                Result = adi_flag_RemoveCallback(pMaxDev->pPenIrqPort->FlagId); 
                break;

            /* CASE: Uninstall KEYIRQ handeling */
            case (ADI_MAX1233_CMD_UNINSTALL_KEYIRQ):
                Result = adi_flag_RemoveCallback(pMaxDev->pKeyIrqPort->FlagId);
                break;
            
            /* CASE: Reenable PENIRQ interrupt */
            case ADI_MAX1233_CMD_REENABLE_PENIRQ:
            	// enable IRQ and send notification upon receipt
            	// (IRPT handler disables further interrupts untill we get this command)
                pFlagPort = (MAX1233_INTERRUPT_PORT *)pVal;
				Result = adi_int_SICEnable(pFlagPort->FlagIntId);
				break;
                	
            /* CASE: Reenable KEYIRQ interrupt */
  	    	case ADI_MAX1233_CMD_REENABLE_KEYIRQ:
            	// enable KEYIRQ and send notification upon receipt
                pFlagPort = (MAX1233_INTERRUPT_PORT *)pVal;
				Result = adi_int_SICEnable(pFlagPort->FlagIntId);
				break;     

/***************************************
Device access commands - Access MAX1233
***************************************/

    	    case ADI_DEV_CMD_REGISTER_READ:
    	    case ADI_DEV_CMD_REGISTER_WRITE:
    	    case ADI_DEV_CMD_REGISTER_FIELD_READ:
    	    case ADI_DEV_CMD_REGISTER_FIELD_WRITE:
    	    case ADI_DEV_CMD_REGISTER_BLOCK_READ: 
    	    case ADI_DEV_CMD_REGISTER_BLOCK_WRITE:
    	    case ADI_DEV_CMD_REGISTER_TABLE_READ:
    	    case ADI_DEV_CMD_REGISTER_TABLE_WRITE:
    	    case ADI_DEV_CMD_REGISTER_FIELD_TABLE_READ:
    	    case ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE:
    	    	
				AccessParams.ManagerHandle  	= pMaxDev->ManagerHandle;  	// device manager handle
				AccessParams.ClientHandle  		= NULL;      				// client handle - passed to the internal 'Device' specific function
				AccessParams.DeviceNumber  		= pMaxDev->SpiDeviceNumber;	// SPI device number 
				AccessParams.DeviceAddress  	= 0; 						// No SPI Global address for MAX1233
				AccessParams.DCBHandle   		= pMaxDev->DCBHandle;    	// handle to the callback manager
				AccessParams.DeviceFunction 	= NULL;        				// No device access callback defined here
				AccessParams.Command   			= Command;                  // command ID
				AccessParams.Value    			= (void*)pVal;				// command specific value
				AccessParams.FinalRegAddr  		= MAX1233_FINAL_REG_ADDRESS;// Address of the last register in MAX1233
				AccessParams.RegisterField  	= BitfieldTable;         	// bitfield table for MAX1233
				AccessParams.ReservedValues 	= ReservedBitTable;       	// reserved bits table for MAX1233
				AccessParams.ValidateRegister 	= InvalidAndReadOnlyTable;  // invalid and read-only table for MAX1233

				// choose default or user override for SPI Baud Rate
				if (0 == SpiBaudRateConfig[0].Value)
					AccessParams.ConfigTable  	= (void*)NULL;        		// use system default
				else	
					AccessParams.ConfigTable  	= SpiBaudRateConfig;        // apply user override

				AccessParams.SelectAccess  		= SelectAccess;    			// Device Access type          
				AccessParams.pAdditionalinfo 	= (void *)NULL;        		// No Additional info

				// pass all read/write commands down through device access service to manage the MAX1233 device
				Result = adi_device_access (&AccessParams);
    	        break;

/***************************************
Default: Commands not supported
***************************************/

            /* DEFAULT: Command not supported */
            default:
                Result = ADI_MAX1233_RESULT_CMD_NOT_SUPPORTED;
                break;
        }
/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return(Result);
}


/*********************************************************************

 Function:  PenIrqCallback

 Description: Callback routine for MAX1233 PENIRQ Interrupt

*********************************************************************/
static void PenIrqCallback (
	void* 	DeviceHandle,       /* Device handle            */
	u32 	Event,              /* callback Event ID        */
	void* 	pArg                /* Callback argument        */
){
    /* retrive the device we're working on */
    MAX1233_DEVICE_STRUCT  *pMaxDev    = (MAX1233_DEVICE_STRUCT *)DeviceHandle;
	u32	EventToPost;
		

/* Debug build only */
#ifdef ADI_DEV_DEBUG
	/* verify the Device handle */
    if (ValidatePDDHandle(DeviceHandle) == ADI_DEV_RESULT_SUCCESS) 
    {
#endif

		// lockout further PENIRQ interrupts until re-enabled by ADI_MAX1233_CMD_REENABLE_PENIRQ
		// disable PENIRQ and send notification back to app
		adi_int_SICDisable(pMaxDev->pPenIrqPort->FlagIntId);
		
		// call/post (depending on USE_DEFERRED_CALLBACKS) event to application callback
		EventToPost = ADI_MAX1233_EVENT_PENIRQ_NOTIFICATION;
		(pMaxDev->DMCallback)(pMaxDev->DeviceHandle, EventToPost, (void *) NULL);

/* Debug build only */
#ifdef ADI_DEV_DEBUG
    }
#endif
}

/*********************************************************************

 Function:  KeyIrqCallback

 Description: Callback routine for MAX1233 KEYIRQ Interrupt

*********************************************************************/
static void KeyIrqCallback (
	void* 	DeviceHandle,       /* Device handle            */
	u32 	Event,              /* callback Event ID        */
	void* 	pArg                /* Callback argument        */
){

    /* retrive the device we're working on */
    MAX1233_DEVICE_STRUCT *pMaxDev = (MAX1233_DEVICE_STRUCT *) DeviceHandle;
	u32	EventToPost;

/* Debug build only */
#ifdef ADI_DEV_DEBUG
	/* verify the Device handle */
    if (ValidatePDDHandle(DeviceHandle) == ADI_DEV_RESULT_SUCCESS) 
    {
#endif

		// lockout further KEYIRQ interrupts until re-enabled by ADI_MAX1233_CMD_REENABLE_KEYIRQ
		// disable KEYIRQ and send notification back to app
		adi_int_SICDisable(pMaxDev->pKeyIrqPort->FlagIntId);

		// call/post (depending on USE_DEFERRED_CALLBACKS) event to application callback
		EventToPost = ADI_MAX1233_EVENT_KEYIRQ_NOTIFICATION;
		(pMaxDev->DMCallback)(pMaxDev->DeviceHandle, EventToPost, (void *) NULL);

/* Debug build only */
#ifdef ADI_DEV_DEBUG
    }
#endif
}

/*********************************************************************

 Function:  SpiCallback

 Description: SPI callback routine

*********************************************************************/

static void SpiCallback (
	void* 	DeviceHandle,       /* Device handle            */
	u32 	Event,              /* callback Event ID        */
	void* 	pArg                /* Callback argument        */
){
	/* do nothing */
}

#if defined(ADI_DEV_DEBUG)  /* Debug build only */
/*********************************************************************

    Function:       ValidatePDDHandle

    Description:    Validates a PDD handle

*********************************************************************/

static u32 ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE      PDDHandle   /* PDD handle of a MAX1233 device  */
)
{    
    /* Return value - insure the client has provided a valid PDD Handle */
    u32     Result = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    /* index */
    u32     i;
    
    /* compare the given PDD handle with PDDHandle allocated to all EPPI devices in the list    */
    for (i = 0; i < MAX1233_NUM_DEVICES; i++) 
    {        
        if (PDDHandle == (ADI_DEV_PDD_HANDLE)&MaxDevice[i])
        {
            /* Given PDDHandle is valid. quit this loop */
            Result = ADI_DEV_RESULT_SUCCESS;
            break;
        }
    }
    
    /* return */
    return (Result);
}

#endif


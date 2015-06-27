/**********************************************************************

Copyright(c) Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software 
you agree to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_ad7877.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
    This is the driver source code for AD7877 Touchscreen Controller.
    Use Device access commands to access AD7877 device registers 
    (refer to adi_dev.h)
    
***********************************************************************/
/*********************************************************************

Include files

*********************************************************************/

#include <services/services.h>						/* system service includes  */
#include <drivers/adi_dev.h>						/* device manager includes  */
#include <drivers/spi/adi_spi.h>                    /* SPI driver includes      */
#include <drivers/touchscreen/adi_ad7877.h>   		/* AD7877 driver includes   */

/*********************************************************************

Enumerations and defines

*********************************************************************/

/* number of AD1836A devices in the system  */
#define ADI_AD7877_NUM_DEVICES 	            (sizeof(Device)/sizeof(ADI_AD7877))

/* Address of last register in AD7877   */
#define ADI_AD7877_END_REG_ADDR             AD7877_GPIO_DATA_REG
/* Shift count to reach Address bits in AD7877 data packet  */
#define ADI_AD7877_REG_ADDR_SHIFT           12
/* Shift count to reach RD field in Control Register 1 */
#define ADI_AD7877_RD_FIELD_SHIFT           2
/* Shift count to reach CHADD field in Control Register 1 */
#define ADI_AD7877_CHADD_FIELD_SHIFT        7
/* Shift count to reach Extended Address bits in AD7877 write data packet */
#define ADI_AD7877_EXTND_REG_ADDR_SHIFT     8
/* Address of first Read-only Type Register in AD7877       */
#define ADI_AD7877_RO_REG_START_ADDR        AD7877_YPOS
/* Address of first Read-only Type Register in AD7877       */
#define ADI_AD7877_RO_REG_END_ADDR          AD7877_Z1
/* Address of first Extended Read/Write Type Register in AD7877 */
#define ADI_AD7877_EXTND_RW_REG_START_ADDR  AD7877_GPIO_CONTROL_REG1
/* Invalid registers in AD7877 */
#define ADI_AD7877_INVALID_REG1             0x00
#define ADI_AD7877_INVALID_REG2             0x0F
/* Address to access Extended registers */
#define ADI_AD7877_EXTND_REG_ACCESS         0xF000

/* Masks AD7877 Register data Mask */
#define ADI_AD7877_REG_VALUE_MASK	        0x0FFF
/* Masks AD7877 Extended Register data Mask */
#define ADI_AD7877_EXTND_REG_VALUE_MASK	    0x00FF
/* Masks AD7877 Control Reg 2 TMR field */
#define ADI_AD7877_CTRL_REG2_TMR_MASK	    0x0003
/* Masks AD7877 Control Reg 1 ADC Mode field */
#define ADI_AD7877_CTRL_REG1_MODE_MASK	    0x0003
/* Masks AD7877 Control Reg 1 RD field */
#define ADI_AD7877_CTRL_REG1_RD_MASK	    0x0F83
/* Masks AD7877 Control Reg 1 CHADD field */
#define ADI_AD7877_CTRL_REG1_CHADD_MASK	    0x087F

/* AD7877 Conversion modes         */
/* AD7877 is in Single Conversion mode      */
#define ADI_AD7877_SINGLE_CONVERSION        1
/* AD7877 is in Repeat Conversion mode      */
#define ADI_AD7877_REPEAT_CONVERSION        2
/* AD7877 Register Read/Write modes         */
/* Client requests to read a register       */
#define ADI_AD7877_REGISTER_READ            1
/* Client requests to configure a register  */
#define ADI_AD7877_REGISTER_WRITE           2
/* AD7877 ADC modes                         */
/* AD7877 is in Single channel mode         */
#define ADI_AD7877_SINGLE_CHANNEL           1
/* AD7877 is in Master sequencer mode       */
#define ADI_AD7877_MASTER_SEQUENCE          3

/* AD7877 Interrupt indicator values */
#define ADI_AD7877_PENIRQ_BIT               1
#define ADI_AD7877_DAV_BIT                  2
#define ADI_AD7877_ALERT_BIT                4
/* bit indicates PENIRQ status, TRUE when detects a screen touch,
								FALSE when detects a screen release */
#define ADI_AD7877_PENIRQ_STATUS            8	

/*********************************************************************

Data Structures

*********************************************************************/
/* Structure to contain data for an instance of the AD7877 device driver                    */
typedef struct {                                /* AD7877 device structure                  */
	u16		                    ControlReg1;    /* Control Register 1                       */
	u16		                    ControlReg2;    /* Control Register 2                       */
	u16		                    AlertReg;       /* Alert Enable/Status Register             */
    ADI_AD7877_RESULT_REGS      ResultReg;      /* Result Registers                         */
    ADI_AD7877_INTERRUPT_PORT   PenIrqPort;     /* Port info for PENIRQ interrupt signal    */
    ADI_AD7877_INTERRUPT_PORT   DavIrqPort;     /* Port info for DAV interrupt signal       */
    ADI_AD7877_INTERRUPT_PORT   AlertIrqPort;   /* Port info for ALERT interrupt signal     */
    u8                          IntEnableFlag;  /* Interrupt Enable indication Flag
                                                   Bit 0 will be set when PENIRQ is enabled, 
                                                   Bit 1 for DAV and Bit 2 for ALERT        */
    u8                          SpiDevNumber;   /* Holds SPI Device Number to use           */
    u8                          SpiChipselect;  /* Holds SPI Chipselect                     */
    u8                          InUseFlag;      /* in use flag (in use when TRUE)           */
    void                        *pEnterCriticalArg; /* critical region argument             */
    ADI_DEV_MANAGER_HANDLE      ManagerHandle;  /* Device Manager Handle                    */
    ADI_DEV_DEVICE_HANDLE       DeviceHandle;   /* Device handle                            */
    ADI_DCB_HANDLE              DCBHandle;      /* callback handle                          */
    ADI_DCB_CALLBACK_FN         DMCallback;     /* Device manager's Callback function       */
    ADI_DEV_PDD_HANDLE	        SpiHandle;      /* Handle to SPI device (driver)            */
} ADI_AD7877;

/*********************************************************************

Device specific data

*********************************************************************/
/********************
    Moab  
********************/

#if defined(__ADSP_MOAB__)                  /* settings for Moab class devices              */

/* The initial values for the device instance */
static ADI_AD7877 Device[] = {
    {
	    0,                                      /* Control Register 1                       */
	    0,                                      /* Control Register 2                       */
        0,                                      /* Alert Register                           */
        { 0 },                                  /* Result Registers                         */  
        {                                       /* Port info for PENIRQ interrupt signal    */
            ADI_FLAG_PJ12,                      /* PENIRQ Flag ID for BF548 Ez-Kit          */
            ADI_INT_PINT2,                      /* Peripheral interrupt id for above flag   */
        },
        {                                       /* Port info for DAV interrupt signal       */
            ADI_FLAG_PJ11,                      /* DAV Flag ID for BF548 Ez-Kit             */
            ADI_INT_PINT2,                      /* Peripheral interrupt id for above flag   */
        },
        {                                       /* Port info for ALERT interrupt signal     */
            ADI_FLAG_PJ10,                      /* Dummy Flag ID for ALERT                  */
            ADI_INT_PINT2,                      /* Peripheral interrupt id for above flag   */
        },
        0,                                      /* All AD7877 interrupts disabled by default*/
        0,                                      /* uses SPI 0 by default                    */
        0,                                      /* No SPI Chipselect by default             */
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
#error "AD7877 Device instance is yet to be defined for this processor"
#endif

/*********************************************************************

Static functions

*********************************************************************/
/********************************
Device Manager entry
********************************/
static u32 adi_pdd_Open(                        /* Opens AD7877 device                      */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* Device Manager handle                    */
    u32                     DeviceNumber,       /* EPPI Device number to open               */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* Device handle                            */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,        /* pointer to PDD handle location           */
    ADI_DEV_DIRECTION       Direction,          /* data direction                           */
    void                    *pEnterCriticalArg, /* critical region storage location         */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager                */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle                          */
    ADI_DCB_CALLBACK_FN     DMCallback          /* device manager callback function         */
);

static u32 adi_pdd_Close(                       /* Closes a AD7877 device                   */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of the EPPI device to close   */
);

static u32 adi_pdd_Read(                        /* Reads data/queues inbound buffer         */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a device                   */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);
    
static u32 adi_pdd_Write(                       /* Writes data/queues outbound buffer       */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a device                   */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);

static u32 adi_pdd_SequentialIO(                /* Sequentially read/writes data to a device*/ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a device                   */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);

static u32 adi_pdd_Control(                     /* Sets or senses device specific parameter */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a AD7877 device            */
    u32                     Command,            /* Command ID                               */
    void                    *Value              /* Command specific value                   */
);

/********************************
AD7877 register access functions
********************************/

static u32 AccessRegs (                         /* Access AD7877 device registers           */
    ADI_AD7877              *pDevice,           /* pointer to the device we're working on   */
    u32                     Command,            /* Command ID from the client               */
    void                    *Value              /* Command specific value                   */
);

static void MaskField(                          /* For AD7877 register field access         */
	u16                     RegAddr,	        /* Register Address being accessed          */
	u16                     *RegData,	        /* Present value of the Register            */
	u16                     RegField,	        /* Register Field of the register to access */
	u8	                    AccessFlag	        /* Read/ Write Flag                         */
);

/********************************
SPI related functions
********************************/

static u32 SpiOpen(                             /* Open SPI device to access AD7877 regs    */
    ADI_AD7877              *pDevice            /* pointer to the device we're working on   */
);

static u32 SpiAccess(                           /* Access AD7877 device registers via SPI   */
    ADI_AD7877              *pDevice,           /* pointer to the device we're working on   */
    ADI_DEV_1D_BUFFER       *Spi1DBuf,          /* 1D buffer for SPI Access                 */
    u16                     RegAddr,            /* AD7877 Register address to access        */
    u16                     *RegData,           /* AD7877 Register data                     */
    u8                      AccessFlag          /* SPI Read/Write indicator                 */
);

static u32 SpiClose(                            /* Closes SPI device used for AD7877 access */
    ADI_AD7877              *pDevice            /* pointer to the device we're working on   */
);

/********************************
Callback functions
********************************/

static void PenIrqCallback (                    /* Callback for AD7877 PENIRQ Interrupt     */
	void* 	                DeviceHandle,       /* Device handle                            */
	u32 	                Event,              /* callback Event ID                        */
	void* 	                pArg                /* Callback argument                        */
);

static void DavIrqCallback (                    /* Callback for AD7877 DAV Interrupt        */
	void* 	                DeviceHandle,       /* Device handle                            */
	u32 	                Event,              /* callback Event ID                        */
	void* 	                pArg                /* Callback argument                        */
);

static void AlertIrqCallback (                  /* Callback for AD7877 ALERT Interrupt      */
	void* 	                DeviceHandle,       /* Device handle                            */
	u32 	                Event,              /* callback Event ID                        */
	void* 	                pArg                /* Callback argument                        */
);

static void SpiCallback (                       /* SPI callback routine                     */
	void* 	                DeviceHandle,       /* Device handle                            */
	u32 	                Event,              /* callback Event ID                        */
	void* 	                pArg                /* Callback argument                        */
);

/*********************************************************************

Debug Mode functions (debug build only)

*********************************************************************/
#if defined(ADI_DEV_DEBUG)

static u32 ValidatePDDHandle(                   /* Validates Physical Device Driver Handle  */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of a AD7877 device            */
);

#endif

/*********************************************************************

Entry point for device manager

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT ADIAD7877EntryPoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,    
    adi_pdd_Control,
    adi_pdd_SequentialIO
};

/*********************************************************************
*
*   Function:       adi_pdd_Open
*
*   Description:    Opens a AD7877 device for use
*
*********************************************************************/

static u32 adi_pdd_Open(                        /* Open AD7877 device               */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* Device Manager handle            */
    u32                     DeviceNumber,       /* AD7877 Device number to open     */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* Device handle                    */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,        /* pointer to PDD handle location   */
    ADI_DEV_DIRECTION       Direction,          /* data direction                   */
    void                    *pEnterCriticalArg, /* critical region storage location */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager        */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle                  */
    ADI_DCB_CALLBACK_FN     DMCallback          /* device manager callback function */
) 
{   
    /* Return value - assume we're going to be successful   */
    u32         Result = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we're working on               */
    ADI_AD7877  *pDevice;
    /* exit critical region parameter                       */
    void        *pExitCriticalArg;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    if (DeviceNumber >= ADI_AD7877_NUM_DEVICES)         /* check the device number */
    {
        Result = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;      /* Invalid Device number */
    }
    
    /*  Continue only if the device number is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* insure the device the client wants is available  */
        Result  = ADI_DEV_RESULT_DEVICE_IN_USE;
        /* AD7877 device we're working on */
        pDevice = &Device[DeviceNumber];
    
        /* Protect this section of code - entering a critical region    */
        pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
        /* Check the device usage status*/
        if (pDevice->InUseFlag == FALSE) 
        {
            /* Device is not in use. Reserve the device for this client */
            pDevice->InUseFlag = TRUE;
            Result = ADI_DEV_RESULT_SUCCESS;
        }
        /* Exit the critical region */
        adi_int_ExitCriticalRegion(pExitCriticalArg);
    
        /* Continue only when the AD7877 device is available for use  */
        if (Result == ADI_DEV_RESULT_SUCCESS) 
        {    
            /* AD7877 device is reserved for this client */

            /* Reset the actual device */
            /* Clear all AD7877 register cache */
            pDevice->ControlReg1    = 0;
            pDevice->ControlReg2    = 0;
            pDevice->AlertReg       = 0;
            /* clear results register*/
            pDevice->ResultReg.Y    = 0;
            pDevice->ResultReg.X    = 0;
            pDevice->ResultReg.Z2   = 0;
            pDevice->ResultReg.Aux1 = 0;
            pDevice->ResultReg.Aux2 = 0;
            pDevice->ResultReg.Aux3 = 0;
            pDevice->ResultReg.Bat1 = 0;
            pDevice->ResultReg.Bat2 = 0;
            pDevice->ResultReg.Temp1= 0;
            pDevice->ResultReg.Temp2= 0;
            pDevice->ResultReg.Z1   = 0;
            /* No SPI Chipselect by default */
            pDevice->SpiChipselect  = 0;
            /* All AD7877 interrupts are disabled by default */
            pDevice->IntEnableFlag  = 0;
            
            /* initialize the device settings */
            /* Pointer to critical region */
            pDevice->pEnterCriticalArg  = pEnterCriticalArg;
            /* Pointer to Device manager handle */
            pDevice->ManagerHandle      = ManagerHandle;
            /* Pointer to Device handle */
            pDevice->DeviceHandle       = DeviceHandle;
            /* Pointer to Deffered Callback manager handle */
            pDevice->DCBHandle          = DCBHandle;
            /* Pointer to Device manager callback function  */
            pDevice->DMCallback         = DMCallback;
            /* save the physical device handle in the client supplied location */
            *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;
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
*   Description:    Closes down a AD7877 device
*
*********************************************************************/

static u32 adi_pdd_Close(               /* Closes a AD7877 device                     */
    ADI_DEV_PDD_HANDLE      PDDHandle   /* PDD handle of the AD7877 device to close   */
)
{  
    /* Return value - assume we're going to be successful   */
    u32         Result = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we're working on */
    ADI_AD7877  *pDevice;
    
    /* Configuration table to disable AD7877 */
    ADI_DEV_ACCESS_REGISTER		DisableAD7877[] =
    {	
    	{AD7877_CONTROL_REG1,	0	},
    	{AD7877_CONTROL_REG2,	0	},
    	{ADI_DEV_REGEND,		0	}
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
        pDevice = (ADI_AD7877 *)PDDHandle;
        /* Unhook all AD7877 interrupts */
        /* IF (any AD7877 interrupt is enabled) */
        if (pDevice->IntEnableFlag)
        {
            /* IF (PENIRQ interrupt is enabled & monitored by this driver) */
            if (pDevice->IntEnableFlag & ADI_AD7877_PENIRQ_BIT)
            {
                /* Disable PENIRQ Interrupt Flag callback  */
                Result = adi_flag_RemoveCallback(pDevice->PenIrqPort.FlagId);
            }
            /* IF (DAV interrupt is enabled & monitored by this driver) */
            if ((Result == ADI_DEV_RESULT_SUCCESS) && 
                (pDevice->IntEnableFlag & ADI_AD7877_DAV_BIT))
            {
                /* Disable DAV Interrupt Flag callback  */
                Result = adi_flag_RemoveCallback(pDevice->DavIrqPort.FlagId);
            }
            /* IF (ALERT interrupt is enabled & monitored by this driver) */
            if ((Result == ADI_DEV_RESULT_SUCCESS) && 
                (pDevice->IntEnableFlag & ADI_AD7877_ALERT_BIT))
            {
                /* Disable ALERT Interrupt Flag callback */
                Result = adi_flag_RemoveCallback(pDevice->AlertIrqPort.FlagId);
            }            
        }
        
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
        	/* disable AD7877 */
        	Result = adi_dev_Control(pDevice->DeviceHandle,ADI_DEV_CMD_REGISTER_TABLE_WRITE, (void *)DisableAD7877);
            /* Clear the interrupt enable indicator */
            pDevice->IntEnableFlag = 0;
            /* mark this AD7877 device as closed */
            pDevice->InUseFlag = FALSE;
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
*   Description:    Never called as AD7877 uses Device Access commands
*                   to access its internal registers
*
*********************************************************************/
static u32 adi_pdd_Read(                /* Not supported by AD7877  */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle               */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer        */
){
    
    /* AD7877 uses Device Access commands to access its registers   */
    return(ADI_DEV_RESULT_FAILED);
}
 
/*********************************************************************
*
*   Function:       adi_pdd_Write
*
*   Description:    Never called as AD7877 uses Device Access commands
*                   to access its internal registers
*
*********************************************************************/
static u32 adi_pdd_Write(               /* Not supported by AD7877  */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle               */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer        */
){
    
    /* AD7877 uses Device Access commands to access its registers   */
    return(ADI_DEV_RESULT_FAILED);
}

/*********************************************************************
*
*   Function:       adi_pdd_SequentialIO
*
*   Description:    Never called as AD7877 uses Device Access commands
*                   to access its internal registers
*
*********************************************************************/
static u32 adi_pdd_SequentialIO(        /* Not supported by AD7877  */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle               */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer        */
){
    
    /* AD7877 uses Device Access commands to access its registers   */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************
*
*   Function:       adi_pdd_Control
*
*   Description:    Senses or Configures AD7877 device registers
*
*********************************************************************/
static u32 adi_pdd_Control(             /* Sets or senses device specific parameter */
    ADI_DEV_PDD_HANDLE  PDDHandle,      /* PDD handle of a AD7877 device            */
    u32                 Command,        /* Command ID                               */
    void                *Value          /* Command specific value                   */
){
    ADI_AD7877                  *pDevice;   /*pointer to the device we're working on*/
    u32                         Result;     /* return value                         */
    u32                         u32Value;
    ADI_AD7877_INTERRUPT_PORT   *pFlagPort; /* pointer to Flag info structure       */
    
    /* assume we're going to be successful */
    Result  = ADI_DEV_RESULT_SUCCESS;
            
    /* avoid casts */
    pDevice     = (ADI_AD7877 *)PDDHandle;
    /* assign 32 bit values for the Value argument */
    u32Value    = (u32)Value;
    
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
  
        	    /* AD7877 does not support DMA */
        	    *((u32 *)Value) = FALSE;   
        	    break;
        	
/***************************************
SPI related commands
***************************************/

    	    /* CASE (Set AD7877 SPI Device Number to use) */
    	    case (ADI_AD7877_CMD_SET_SPI_DEVICE_NUMBER):
                /* Save the SPI device number to use */
        	    pDevice->SpiDevNumber   = (u8)u32Value;         
        	    break;
        	
    	    /* CASE (Set AD7877 SPI Chipselect) */
    	    case (ADI_AD7877_CMD_SET_SPI_CS):
                /* Save the SPI chipselect for AD7877 */
        	    pDevice->SpiChipselect  = (u8)u32Value;         
        	    break;

/***************************************
Commands to Enable Interrupt monitoring
***************************************/

            /* CASE: Enable PENIRQ (allow the driver to monitor PENIRQ interrupt signal) */
            case (ADI_AD7877_CMD_ENABLE_INTERRUPT_PENIRQ):
            case (ADI_AD877_ENABLE_INTERRUPT_PENIRQ):
                /* IF (PENIRQ is already enabled and monitored by this driver */
                if (pDevice->IntEnableFlag & ADI_AD7877_PENIRQ_BIT)
                {
                    /* Remove callback for previously mapped Flag ID */
                    if ((Result = adi_flag_RemoveCallback(pDevice->PenIrqPort.FlagId)) != ADI_FLAG_RESULT_SUCCESS)
                    {
                        break;  /* exit on error */
                    }
                }
                /* Map PENIRQ Interrupt to this Flag */
                pFlagPort = (ADI_AD7877_INTERRUPT_PORT *)Value;
                /* open corresponding Blackfin flag */
                if ((Result = adi_flag_Open (pFlagPort->FlagId)) != ADI_FLAG_RESULT_SUCCESS)
                {
                    break;  /* exit on error */
                }
                /* set flag direction as input */
                if ((Result = adi_flag_SetDirection(pFlagPort->FlagId,ADI_FLAG_DIRECTION_INPUT)) != ADI_FLAG_RESULT_SUCCESS)
                {
                    break;  /* exit on error */
                }
                /* set flag trigger to sense interrupt & generate callback */
                /* senses on falling edge */
                if ((Result = adi_flag_InstallCallback (  
                                pFlagPort->FlagId,              /* Flag ID                  */
                                pFlagPort->FlagIntId,           /* Peripheral Interrupt ID  */
                                ADI_FLAG_TRIGGER_FALLING_EDGE,	/* Flag trigger mode        */
        		                TRUE,							/* this is a wakeup flag    */
                                (void *)pDevice,	            /* Client handle            */
                                pDevice->DCBHandle,	            /* Callback Manager         */
                                PenIrqCallback				    /* PENIRQ flag callback     */
                    )) == ADI_FLAG_RESULT_SUCCESS)
                {
                    /* Save the Flag info */
                    /* save Flag ID */
                    pDevice->PenIrqPort.FlagId = pFlagPort->FlagId;
                    /* save Peripheral Interrupt ID */
                    pDevice->PenIrqPort.FlagIntId = pFlagPort->FlagIntId;
                    /* update 'IntEnableFlag' to indicate that PENIRQ is enabled */
                    pDevice->IntEnableFlag |= ADI_AD7877_PENIRQ_BIT;
                    /* update PENIRQ status bit as screen not touched */
                    pDevice->IntEnableFlag &= ~ADI_AD7877_PENIRQ_STATUS;
                }
                break;

            /* CASE: Enable DAV (allow the driver to monitor DAV interrupt signal) */
            case (ADI_AD7877_CMD_ENABLE_INTERRUPT_DAV):
            case (ADI_AD877_ENABLE_INTERRUPT_DAV):
                /* IF (DAV is already enabled and monitored by this driver */
                if (pDevice->IntEnableFlag & ADI_AD7877_DAV_BIT)
                {
                    /* Remove callback for previously mapped Flag ID */
                    if ((Result = adi_flag_RemoveCallback(pDevice->DavIrqPort.FlagId)) != ADI_FLAG_RESULT_SUCCESS)
                    {
                        break;  /* exit on error */
                    }
                }
                /* Map DAV Interrupt to this Flag */
                pFlagPort = (ADI_AD7877_INTERRUPT_PORT *)Value;
                /* open corresponding Blackfin flag */
                if ((Result = adi_flag_Open (pFlagPort->FlagId)) != ADI_FLAG_RESULT_SUCCESS)
                {
                    break;  /* exit on error */
                }
                /* set flag direction as input */
                if ((Result = adi_flag_SetDirection(pFlagPort->FlagId,ADI_FLAG_DIRECTION_INPUT)) != ADI_FLAG_RESULT_SUCCESS)
                {
                    break;  /* exit on error */
                }
                /* set flag trigger to sense interrupt & generate callback */
                /* active low */
                if ((Result = adi_flag_InstallCallback (  
                                pFlagPort->FlagId,              /* Flag ID                  */
                                pFlagPort->FlagIntId,           /* Peripheral Interrupt ID  */
                                ADI_FLAG_TRIGGER_LEVEL_LOW,		/* Flag trigger mode        */
        		                TRUE,							/* this is a wakeup flag    */
                                (void *)pDevice,	            /* Client handle            */
                                pDevice->DCBHandle,	            /* Callback Manager         */
                                DavIrqCallback		            /* DAV flag callback        */
                    )) == ADI_FLAG_RESULT_SUCCESS)
                {
                    /* Save the Flag info */
                    /* save Flag ID */
                    pDevice->DavIrqPort.FlagId = pFlagPort->FlagId;
                    /* save Peripheral Interrupt ID */
                    pDevice->DavIrqPort.FlagIntId = pFlagPort->FlagIntId;
                    /* update 'IntEnableFlag' to indicate that DAV is enabled */
                    pDevice->IntEnableFlag |= ADI_AD7877_DAV_BIT;
                }
                break;
                
            /* CASE: Enable ALERT (allow the driver to monitor ALERT interrupt signal) */
            case (ADI_AD7877_CMD_ENABLE_INTERRUPT_ALERT):
            case (ADI_AD877_ENABLE_INTERRUPT_ALERT):
                /* IF (ALERT is already enabled and monitored by this driver */
                if (pDevice->IntEnableFlag & ADI_AD7877_ALERT_BIT)
                {
                    /* Remove callback for previously mapped Flag ID */
                    if ((Result = adi_flag_RemoveCallback(pDevice->AlertIrqPort.FlagId)) != ADI_FLAG_RESULT_SUCCESS)
                    {
                        break;  /* exit on error */
                    }
                }
                /* Map ALERT Interrupt to this Flag */
                pFlagPort = (ADI_AD7877_INTERRUPT_PORT *)Value;
                /* open corresponding Blackfin flag */
                if ((Result = adi_flag_Open (pFlagPort->FlagId)) != ADI_FLAG_RESULT_SUCCESS)
                {
                    break;  /* exit on error */
                }
                /* set flag direction as input */
                if ((Result = adi_flag_SetDirection(pFlagPort->FlagId,ADI_FLAG_DIRECTION_INPUT)) != ADI_FLAG_RESULT_SUCCESS)
                {
                    break;  /* exit on error */
                }
                /* set flag trigger to sense interrupt & generate callback */
                /* active low */
                if ((Result = adi_flag_InstallCallback (  
                                pFlagPort->FlagId,              /* Flag ID                  */
                                pFlagPort->FlagIntId,           /* Peripheral Interrupt ID  */
                                ADI_FLAG_TRIGGER_LEVEL_LOW,		/* Flag trigger mode        */
        		                TRUE,							/* this is a wakeup flag    */
                                (void *)pDevice,	            /* Client handle            */
                                pDevice->DCBHandle,	            /* Callback Manager         */
                                AlertIrqCallback			    /* ALERT flag callback      */
                    )) == ADI_FLAG_RESULT_SUCCESS)
                {
                    /* Save the Flag info */
                    /* save Flag ID */
                    pDevice->AlertIrqPort.FlagId = pFlagPort->FlagId;
                    /* save Peripheral Interrupt ID */
                    pDevice->AlertIrqPort.FlagIntId = pFlagPort->FlagIntId;
                    /* update 'IntEnableFlag' to indicate that ALERT is enabled */
                    pDevice->IntEnableFlag |= ADI_AD7877_ALERT_BIT;
                }
                break;

/***************************************
Commands to Disable Interrupt monitoring
***************************************/

            /* CASE: Disable PENIRQ (Remove PENIRQ from monitoring) */
            case (ADI_AD7877_CMD_DISABLE_INTERRUPT_PENIRQ):
            case (ADI_AD877_DISABLE_INTERRUPT_PENIRQ):
                /* IF (PENIRQ is already enabled and monitored by this driver */
                if (pDevice->IntEnableFlag & ADI_AD7877_PENIRQ_BIT)
                {
                    /* Remove callback for previously mapped Flag ID */
                    if ((Result = adi_flag_RemoveCallback(pDevice->PenIrqPort.FlagId)) == ADI_FLAG_RESULT_SUCCESS)
                    {
                        /* Update 'IntEnableFlag' indicating that PENIRQ interrupt is disabled */
                        pDevice->IntEnableFlag &= ~ADI_AD7877_PENIRQ_BIT;
                    }
                }
                
                break;

            /* CASE: Disable DAV (Remove DAV from monitoring) */
            case (ADI_AD7877_CMD_DISABLE_INTERRUPT_DAV):
            case (ADI_AD877_DISABLE_INTERRUPT_DAV):
                /* IF (DAV is already enabled and monitored by this driver */
                if (pDevice->IntEnableFlag & ADI_AD7877_DAV_BIT)
                {
                    /* Remove callback for previously mapped Flag ID */
                    if ((Result = adi_flag_RemoveCallback(pDevice->DavIrqPort.FlagId)) == ADI_FLAG_RESULT_SUCCESS)
                    {
                        /* Update 'IntEnableFlag' indicating that DAV interrupt is disabled */
                        pDevice->IntEnableFlag &= ~ADI_AD7877_DAV_BIT;
                    }
                }
                break;

            /* CASE: Disable ALERT (Remove ALERT from monitoring) */
            case (ADI_AD7877_CMD_DISABLE_INTERRUPT_ALERT):
            case (ADI_AD877_DISABLE_INTERRUPT_ALERT):
                /* IF (ALERT is already enabled and monitored by this driver */
                if (pDevice->IntEnableFlag & ADI_AD7877_ALERT_BIT)
                {
                    /* Remove callback for previously mapped Flag ID */
                    if ((Result = adi_flag_RemoveCallback(pDevice->AlertIrqPort.FlagId)) == ADI_FLAG_RESULT_SUCCESS)
                    {
                        /* Update 'IntEnableFlag' indicating that ALERT interrupt is disabled */
                        pDevice->IntEnableFlag &= ~ADI_AD7877_ALERT_BIT;
                    }
                }
                break;

/***************************************
Device access commands - Access AD7877
***************************************/

    	    /* CASE: (Read a specific register from the AD7877)     */
    	    case(ADI_DEV_CMD_REGISTER_READ):
    	    /* CASE: (Configure a specific register in the AD7877)  */
    	    case(ADI_DEV_CMD_REGISTER_WRITE):
    	    /* CASE: (Read a specific field from a given AD7877 device register) */
    	    case(ADI_DEV_CMD_REGISTER_FIELD_READ):
    	    /* CASE: (Write to a specific field in a given AD7877 device register) */
    	    case(ADI_DEV_CMD_REGISTER_FIELD_WRITE):
    	    /* CASE: (Read block of AD7877 registers) */
    	    case(ADI_DEV_CMD_REGISTER_BLOCK_READ): 
    	    /* CASE: (Write to a block of AD7877 registers) */
    	    case(ADI_DEV_CMD_REGISTER_BLOCK_WRITE):
    	    /* CASE: (Read a table of selective registers in AD7877) */
    	    case(ADI_DEV_CMD_REGISTER_TABLE_READ):
    	    /* CASE: (Write to a table of selective registers in AD7877) */
    	    case(ADI_DEV_CMD_REGISTER_TABLE_WRITE):
    	    /* CASE (Read a table of selective register(s) field(s) in AD7877) */
    	    case(ADI_DEV_CMD_REGISTER_FIELD_TABLE_READ):
    	    /* CASE (Write to a table of selective register(s) field(s) in AD7877) */
    	    case(ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE):
    	
    	        /* Access AD7877 registers */
    	        Result = AccessRegs(pDevice,Command,Value);
    	        break;

/***************************************
Default: Commands not supported
***************************************/

            /* DEFAULT: Command not supported */
            default:
                Result = ADI_AD7877_RESULT_CMD_NOT_SUPPORTED;
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
*
*   Function:       AccessRegs
*
*   Description:    Access AD7877 device registers
*
*********************************************************************/
static u32 AccessRegs (             /* Access AD7877 device registers           */
    ADI_AD7877          *pDevice,   /* pointer to the device we're working on   */
    u32                 Command,    /* Command ID from the client               */
    void                *Value      /* Command specific value                   */
){
    /* Return value - assume we're going to be successful   */
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    ADI_DEV_1D_BUFFER   Spi1DBuf;   /* 1D buffer for SPI access                 */
    u8  ConversionMode; /* AD7877 Conversion mode (single or repeat conversion) */
    u8  AccessFlag;     /* AD7877 Register read/write indicator                 */
    u16 RegAddr, RegData, NoConversion;
    u8  i;

    /* Pointers to Device access structure passed by the client */
    ADI_DEV_ACCESS_REGISTER			*AccessSelective;
    ADI_DEV_ACCESS_REGISTER_BLOCK	*AccessBlock;
    ADI_DEV_ACCESS_REGISTER_FIELD	*AccessField;

    /* Initialise SPI 1D buffer */
    Spi1DBuf.ElementCount       = 1;/* AD7877 requires CS pulse for each packet */
    Spi1DBuf.ElementWidth       = 2;/* AD7877 data packet is always 2 bytes     */    
    Spi1DBuf.pNext              = NULL;
    Spi1DBuf.CallbackParameter  = &Spi1DBuf;

    /* Configure AD7877 registers? */
    if ((Command == ADI_DEV_CMD_REGISTER_WRITE)             || 
        (Command == ADI_DEV_CMD_REGISTER_FIELD_WRITE)       ||
        (Command == ADI_DEV_CMD_REGISTER_TABLE_WRITE)       ||
        (Command == ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE) ||
        (Command == ADI_DEV_CMD_REGISTER_BLOCK_WRITE))
    {
        /* Write to AD7877 register(s) */
        AccessFlag = ADI_AD7877_REGISTER_WRITE;
    }
    else
    {
        /* Read AD7877 register(s) */
        AccessFlag = ADI_AD7877_REGISTER_READ;
    }

    /* Open SPI device for AD7877 device Access */
    if ((Result = SpiOpen(pDevice)) == ADI_DEV_RESULT_SUCCESS)
    {        
        /* Check AD7877 Control Register 2 TMR status */    
        if (!(pDevice->ControlReg2 & ADI_AD7877_CTRL_REG2_TMR_MASK))
        {
            /* TMR field has non-zero value (AD7877 is in single conversion mode)   */
			/* IF (AD7877 in single conversion or slave sequencer mode ) */
        	if ((pDevice->ControlReg1 & ADI_AD7877_CTRL_REG1_MODE_MASK) != ADI_AD7877_MASTER_SEQUENCE)
			{
				/* Clear Mode bits in chached Control register 1 */
        		pDevice->ControlReg1 &= ~ADI_AD7877_CTRL_REG1_MODE_MASK;
			}
        }        

        /* Config AD7877 ADC in Do Not Convert mode */
        NoConversion = (pDevice->ControlReg1 & ~ADI_AD7877_CTRL_REG1_MODE_MASK);
        
        /* Continue only if SPI Open/Read results in success */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            /* Write to AD7877 - Configures AD7877 in Do Not Convert mode */
            Result = SpiAccess(pDevice,&Spi1DBuf,AD7877_CONTROL_REG1,&NoConversion,ADI_AD7877_REGISTER_WRITE);
        }

        if (Result != ADI_DEV_RESULT_SUCCESS)
        {
            /* Close SPI Driver opened for AD7877 access */
            SpiClose(pDevice);
        }
    }
    
    /* Continue only if SPI Open/Read/Write results in success */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
        /* CASEOF (Command ID)  */
        switch (Command)
        {
            /* CASE: Read a block of AD7877 registers       */
            case (ADI_DEV_CMD_REGISTER_BLOCK_READ):
            /* CASE: Configure a block of AD7877 registers  */
            case (ADI_DEV_CMD_REGISTER_BLOCK_WRITE):
            
                /* pointer to block access table */
        	    AccessBlock = (ADI_DEV_ACCESS_REGISTER_BLOCK *) Value;
        	    /* Start address of register block to access */
        	    RegAddr = AccessBlock->Address;
/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
        	    /* Validate register access count */
        	    if (AccessBlock->Count > ADI_AD7877_END_REG_ADDR)
        	    {
        	        /* Register access count is greater than Max number of AD7877 registers available */
        	        Result = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
        	        break;  /* exit on error */
                }
#endif
				/* IF (read a block of AD7877 registers) */
				if (AccessFlag == ADI_AD7877_REGISTER_READ)
                {
					/* Configure Control Register 1 RD field with the start address of register read block */
            		RegData = (pDevice->ControlReg1 & ADI_AD7877_CTRL_REG1_RD_MASK);
            		RegData |= (RegAddr << ADI_AD7877_RD_FIELD_SHIFT);        
					/* Write to AD7877 - Configures AD7877 Control reg 1 with start address mentioned in the block read struct*/
            		Result = SpiAccess(pDevice,&Spi1DBuf,AD7877_CONTROL_REG1,&RegData,ADI_AD7877_REGISTER_WRITE);
					/* Continue only if SPI Open/Read results in success */
        			if (Result != ADI_DEV_RESULT_SUCCESS)
        			{
        			    break;
            		}
                }
                
        	    /* Continue until Block of register access is done */
        	    for (i=0;i<AccessBlock->Count;)
        	    {
        	        /* Validate AD7877 register address to access */
        	        if ((RegAddr == ADI_AD7877_INVALID_REG1) ||
        	            (RegAddr == ADI_AD7877_INVALID_REG2) ||
        	            (RegAddr > ADI_AD7877_END_REG_ADDR))
                    {
                        /* if (Read AD7877 device register) */
                        if (AccessFlag == ADI_AD7877_REGISTER_READ)
                        {
                            /* Do Dummy read for this address, as AD7877 
                               auto-increments read register address in Control Reg 1 RD field */
                            if ((Result = SpiAccess(pDevice,&Spi1DBuf,RegAddr,&RegData,AccessFlag)) 
                                                    != ADI_DEV_RESULT_SUCCESS)
                            {
                                break;  /* exit on error */
                            }
                        }
                        /* RegAddr holds invalid AD7877 register address to access */
                        /* Increment AD7877 register address to access */
                        RegAddr++;
                    }
                    else
                    {
                        /* Configure AD7877 device? */
                        if (AccessFlag == ADI_AD7877_REGISTER_WRITE)
                        {
                            /* If (this is a Read-only register) */
                            if ((RegAddr >= ADI_AD7877_RO_REG_START_ADDR) &&
                                (RegAddr <= ADI_AD7877_RO_REG_END_ADDR))
                            {
                                RegAddr++;
                            }
                            /* if (client configures ControlReg1) */
                            else if(RegAddr == AD7877_CONTROL_REG1)
                            {
                                /* update internal ControlReg1 cache */
                                pDevice->ControlReg1 = *(AccessBlock->pData+i);
                            }
                            else
                            {
                                if ((Result = SpiAccess(pDevice,&Spi1DBuf,RegAddr,(AccessBlock->pData+i),AccessFlag)) 
                                                    != ADI_DEV_RESULT_SUCCESS)
                                {
                                    break;  /* exit on error */
                                }
                            }
                            i++;    /* Move to next register data in the list */
                        }
                        else    /* Read AD7877 device register */
                        {
                        	/* Read the selected register value */
							if ((Result = SpiAccess(pDevice,&Spi1DBuf,RegAddr,&RegData,AccessFlag)) 
                                                   != ADI_DEV_RESULT_SUCCESS)
                            {
                                break;  /* exit on error */
                            }
                            /* IF (Register address to read = Control Register 1) */
                            if(RegAddr == AD7877_CONTROL_REG1)
                            {
                                /* update value from ControlReg1 cache */
                                RegData = pDevice->ControlReg1;
                                RegAddr++;	/* next register address to be read */
                            }                            
                            /* update client table with read data */
                            *(AccessBlock->pData+i) = RegData;
                            i++;    /* Move to next register data location in the list */
                        }
                    }
                }
                break;
    	
            /* CASE: Read a specific AD7877 register value */
            case (ADI_DEV_CMD_REGISTER_READ):
            /* CASE: Configure a specific AD7877 register  */
            case (ADI_DEV_CMD_REGISTER_WRITE):
            /* CASE: Read a table of AD7877 registers */
            case (ADI_DEV_CMD_REGISTER_TABLE_READ):
            /* CASE: Configure a table of AD7877 registers */
            case (ADI_DEV_CMD_REGISTER_TABLE_WRITE):
            
                /* pointer to selective access table */
        	    AccessSelective = (ADI_DEV_ACCESS_REGISTER *) Value;
                /* Load the first register address in the table to access */
                RegAddr = AccessSelective->Address;

        	    /* Continue until all registers in the given table are accessed */
        	    while (RegAddr != ADI_DEV_REGEND)
        	    {
/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
                    /* Validate AD7877 register address to access */
        	        if ((RegAddr == ADI_AD7877_INVALID_REG1) ||
        	            (RegAddr == ADI_AD7877_INVALID_REG2) ||
        	            (RegAddr > ADI_AD7877_END_REG_ADDR))
                    {
        	            Result = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
                        break;  /* exit on error */
                    }
                    /* Check for Write to Read-only register */
                    else if (((RegAddr >= ADI_AD7877_RO_REG_START_ADDR) &&
                              (RegAddr <= ADI_AD7877_RO_REG_END_ADDR)) &&
                              (AccessFlag == ADI_AD7877_REGISTER_WRITE))
                    {
        	            Result = ADI_DEV_RESULT_ATTEMPT_TO_WRITE_READONLY_REG;
                        break;  /* exit on error */
                    }
#endif

                    /* Configure AD7877 device? */
                    if (AccessFlag == ADI_AD7877_REGISTER_WRITE)
                    {
                        /* if (configure Control Reg 1?) */
                        if(RegAddr == AD7877_CONTROL_REG1)
                        {
                            /* update internal ControlReg1 cache */
                            pDevice->ControlReg1 = AccessSelective->Data;
                        }
                        else
                        {
                            /* update the selected register with given value */
                            if ((Result = SpiAccess(pDevice,&Spi1DBuf,RegAddr,&AccessSelective->Data,AccessFlag)) 
                                                    != ADI_DEV_RESULT_SUCCESS)
                            {
                                break;  /* exit on error */
                            }
                        }
                    }
                    else    /* Read AD7877 device register */
                    {
                        /* Read Control Register 1 value? */
                        if(RegAddr == AD7877_CONTROL_REG1)
                        {
                            /* update value from internal cache */
                            RegData = pDevice->ControlReg1;
                        }
                        else    /* Read AD7877 device register other than Control Reg 1*/
                        {
                            /* Clear RD field in Control Reg 1 */
                            RegData = (NoConversion & ADI_AD7877_CTRL_REG1_RD_MASK);
                            /* update Control Reg 1 RD field with register address to read */
                            RegData |= (RegAddr << ADI_AD7877_RD_FIELD_SHIFT);
                            /* write to AD7877 Control Register 1 */
                            if ((Result = SpiAccess(pDevice,&Spi1DBuf,AD7877_CONTROL_REG1,&RegData,ADI_AD7877_REGISTER_WRITE)) 
                                                    != ADI_DEV_RESULT_SUCCESS)
                            {
                                break;  /* exit on error */
                            }
                            /* Read the selected AD7877 register value */
                            if ((Result = SpiAccess(pDevice,&Spi1DBuf,RegAddr,&RegData,ADI_AD7877_REGISTER_READ)) 
                                                    != ADI_DEV_RESULT_SUCCESS)
                            {
                                break;  /* exit on error */
                            }
                        }
                        /* update client table with read data */
                        AccessSelective->Data = RegData;
                    }
                    
                    AccessSelective++;  /* Move to next Register Access pair */
                    
           		    /* check if the access command is for single register or selective registers */
           		    if ((Command == ADI_DEV_CMD_REGISTER_READ)	||
           			    (Command == ADI_DEV_CMD_REGISTER_WRITE))
                    {
           			    /* for single register access */
           			    RegAddr = ADI_DEV_REGEND;	/* exit the while loop after first AD7877 access */
                    }
           		    else
           		    {
           			    /* for selective register access */
           			    RegAddr = AccessSelective->Address;    /* get the next register address in the client list */
                    }
                }
                break;

            /* CASE: Read a specific AD7877 register field value */
            case (ADI_DEV_CMD_REGISTER_FIELD_READ):
            /* CASE: Configure a specific AD7877 register field */
            case (ADI_DEV_CMD_REGISTER_FIELD_WRITE):
            /* CASE: Read a table of AD7877 register(s) fields */
            case (ADI_DEV_CMD_REGISTER_FIELD_TABLE_READ):
            /* CASE: Configure a table of AD7877 register(s) fields */
            case (ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE):
            
                /* pointer to register field access table */
        	    AccessField = (ADI_DEV_ACCESS_REGISTER_FIELD *) Value;
                /* Load the first register address in the table to access */
                RegAddr = AccessField->Address;

        	    /* Continue until all registers in the given table are accessed */
        	    while (RegAddr != ADI_DEV_REGEND)
        	    {
/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
                    /* Validate AD7877 register address to access */
        	        if ((RegAddr == ADI_AD7877_INVALID_REG1) ||
        	            (RegAddr == ADI_AD7877_INVALID_REG2) ||
        	            (RegAddr > ADI_AD7877_END_REG_ADDR))
                    {
        	            Result = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
                        break;  /* exit on error */
                    }
                    /* Check for Write to Read-only register */
                    else if (((RegAddr >= ADI_AD7877_RO_REG_START_ADDR) &&
                              (RegAddr <= ADI_AD7877_RO_REG_END_ADDR)) &&
                              (AccessFlag == ADI_AD7877_REGISTER_WRITE))
                    {
        	            Result = ADI_DEV_RESULT_ATTEMPT_TO_WRITE_READONLY_REG;
                        break;  /* exit on error */
                    }
                    /* Check if this register has individual fields to access */
                    else if (((RegAddr >= AD7877_AUX1_HIGH_LIMIT) &&
                              (RegAddr <= AD7877_TEMP1_HIGH_LIMIT)) ||
                             ((RegAddr >= AD7877_YPOS) && (RegAddr <= AD7877_Z1)))
                    {
                        Result = ADI_DEV_RESULT_INVALID_REG_FIELD;
                        break;  /* exit on error */
                    }
#endif
                    /* Access Control Register 1 value? */
                    if(RegAddr == AD7877_CONTROL_REG1)
                    {
                        /* update value from internal cache */
                        RegData = pDevice->ControlReg1;
                    }
                    else
                    {
                        /* Read AD7877 device register to be accessed */
                        /* Clear RD field in Control Reg 1 */
                        RegData = (NoConversion & ADI_AD7877_CTRL_REG1_RD_MASK);
                        /* update Control Reg 1 RD field with register address to read */
                        RegData |= (RegAddr << ADI_AD7877_RD_FIELD_SHIFT);
                        /* write to AD7877 Control Register 1 */
                        if ((Result = SpiAccess(pDevice,&Spi1DBuf,AD7877_CONTROL_REG1,&RegData,ADI_AD7877_REGISTER_WRITE)) 
                                                    != ADI_DEV_RESULT_SUCCESS)
                        {
                            break;  /* exit on error */
                        }
                        /* Read the selected AD7877 register value */
                        if ((Result = SpiAccess(pDevice,&Spi1DBuf,RegAddr,&RegData,ADI_AD7877_REGISTER_READ)) 
                                                != ADI_DEV_RESULT_SUCCESS)
                        {
                            break;  /* exit on error */
                        }
                    }
                    /* 'RegData' holds the present value of AD7877 register to be accessed */
                    /* Mask the register field we're interested in */
                    MaskField(RegAddr,&RegData,AccessField->Field,AccessFlag);                    
                    /* Configure AD7877 device? */
                    if (AccessFlag == ADI_AD7877_REGISTER_WRITE)
                    {
                        /* update register field with new value */
                        RegData |= (AccessField->Data << AccessField->Field);
                        /* if (client configures ControlReg1) */
                        if(RegAddr == AD7877_CONTROL_REG1)
                        {
                            /* update internal ControlReg1 cache */
                            pDevice->ControlReg1 = RegData;
                        }
                        else
                        {
                            /* update the selected register with new value */
                            if ((Result = SpiAccess(pDevice,&Spi1DBuf,RegAddr,&RegData,ADI_AD7877_REGISTER_WRITE)) 
                                                    != ADI_DEV_RESULT_SUCCESS)
                            {
                                break;  /* exit on error */
                            }
                        }
                    }
                    else    /* Read AD7877 device register */
                    {
                        /* update client table with read data */
                        AccessField->Data = RegData;
                    }
                    
                    AccessField++;  /* Move to next Register Field Access pair */
                    
           		    /* check if the access command is for single register or selective registers */
           		    if ((Command == ADI_DEV_CMD_REGISTER_FIELD_READ)	||
           			    (Command == ADI_DEV_CMD_REGISTER_FIELD_WRITE))
                    {
           			    /* for single register access */
           			    RegAddr = ADI_DEV_REGEND;	/* exit the while loop */
                    }
           		    else
           		    {
           			    /* for selective register access */
           			    RegAddr = AccessField->Address;    /* get the next register address to be accessed */
                    }
                }
                break;
        }
        
        /* Close AD7877 register access by configuring AD7877 Control Reg 1 */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            /* finally configure AD7877 Control Reg 1 */
            RegData = (pDevice->ControlReg1 & ADI_AD7877_CTRL_REG1_RD_MASK);
            /* Let Control Reg 1 RD field holds Control reg 1 Address */
            RegData |= (AD7877_CONTROL_REG1 << ADI_AD7877_RD_FIELD_SHIFT);            
            /* write to AD7877 Control Register 1 */
            Result = SpiAccess(pDevice,&Spi1DBuf,AD7877_CONTROL_REG1,&RegData,ADI_AD7877_REGISTER_WRITE);
        }
        
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
        	/* Close SPI Driver opened for AD7877 access */
            Result = SpiClose(pDevice);
        }
        else
        {
            /* Close SPI Driver opened for AD7877 access */
            SpiClose(pDevice);
        }
    }
    
    /* return */
    return (Result);
}

/*********************************************************************
*
* Function:  MaskField
*
* Description: for AD7877 register field access
* Write mode => Masks other register fields and updates 
*               the corresponding register field with new value
*
* Read mode =>  Masks corresponding register fields and clears 
*               other register field values
*
*********************************************************************/
static void MaskField(
	u16     RegAddr,	/* Register Address being accessed          */
	u16     *RegData,	/* Present value of the Register            */
	u16     RegField,	/* Register Field of the register to access */
	u8	    AccessFlag	/* Read/ Write Flag                         */
){

	u16 Mask,ShiftCount,Fieldlength = 1;
	
	/* CASEOF (Register Address) */
	switch (RegAddr)
	{
	    /* CASE: Control Register 1 */
	    case (AD7877_CONTROL_REG1):
	        Mask = 0x885;
	        break;
        
        /* CASE: Control Register 2 */
	    case (AD7877_CONTROL_REG2):
	        Mask = 0x55D;
	        break;
	        
	    /* CASE: DAC Register */
	    case (AD7877_DAC_REG):
	        Mask = 0x01F;
	        break;
	        
	    /* CASE: GPIO Control Register 1 */
	    case (AD7877_GPIO_CONTROL_REG1):
	    /* CASE: GPIO Control Register 2 */
	    case (AD7877_GPIO_CONTROL_REG2):
	        Mask = 0x0FF;
	        break;
	        
	    /* CASE: GPIO Data Register */
	    case (AD7877_GPIO_DATA_REG):
	        Mask = 0x0F0;
            break;
            
        /* default: Alert Register / Sequencer Register 0 / Sequencer Register 1 */
	    default:
	        Mask = 0xFFF;
	        break;
    }

    /* Calculate shift count to reach the register field */
	ShiftCount = RegField;
    Mask >>= (RegField + 1);
	
	/* get the register field length    */
    while ((!(Mask & 1)) && (ShiftCount < 11)) 
    {
        Fieldlength++;    /* increment register field length */
    	Mask >>= 1;
	    ShiftCount++;
    }

    Mask = 0;   /* clear mask */
    
    /* shift register field times to mask bits after the register field to be accessed */
    if (RegField)
    {
        Mask = (0x0FFF >> (12 - RegField));
    }
    
    /* Mask bits present before the register field to be accessed */
    Mask |= (0xFFFF << (RegField + Fieldlength));

    /* if (Read register field?) */
    if (AccessFlag == ADI_AD7877_REGISTER_READ)
    {
        /* clears other register field values */
        *RegData = *RegData & ~Mask;
        /* extract register field data */
		*RegData >>= RegField;
	}
	/* Configure a register field */
 	else
 	{
 	    /* clear present register field value */
 	    *RegData = *RegData & Mask;
    }

    /* return */
    return;
}

/*********************************************************************
*
*   Function:       SpiAccess
*
*   Description:    Controls SPI signals to access AD7877 registers
*
*********************************************************************/
static u32 SpiAccess(               /* Access AD7877 device registers via SPI   */
    ADI_AD7877          *pDevice,   /* pointer to the device we're working on   */
    ADI_DEV_1D_BUFFER   *Spi1DBuf,  /* 1D buffer for SPI Access                 */
    u16                 RegAddr,    /* AD7877 Register address to access        */
    u16                 *RegData,   /* AD7877 Register data                     */
    u8                  AccessFlag  /* SPI Read/Write indicator                 */
){
    /* Return value - assume we're going to be successful   */
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    u16 SpiData = 0;    /* location to hold AD7877 register data    */
    
    /* Initialise SPI 1D buffer */
    Spi1DBuf->Data          = &SpiData;
    Spi1DBuf->ProcessedFlag = FALSE;        /* Clear processed flag             */

    /* Write to AD7877? */
    if (AccessFlag == ADI_AD7877_REGISTER_WRITE)
    {
        /* if (this is extended register write) */
        if (RegAddr >= ADI_AD7877_EXTND_RW_REG_START_ADDR)
        {
            /* Extended register write */
            SpiData = (ADI_AD7877_EXTND_REG_ACCESS | 
                      ((RegAddr - ADI_AD7877_EXTND_RW_REG_START_ADDR) << ADI_AD7877_EXTND_REG_ADDR_SHIFT));
            /* Load Extended register data */
            SpiData |= (*RegData & ADI_AD7877_EXTND_REG_VALUE_MASK);
        }
        else
        {
            /* Load AD7877 register data */
            SpiData = (*RegData & ADI_AD7877_REG_VALUE_MASK);
            /* update internal register cache */
            /* if (configure Control Reg 2) */
            if (RegAddr == AD7877_CONTROL_REG2)
            {
                pDevice->ControlReg2    = SpiData;
            }
            /* else if (configure Alert Reg) */
            else if (RegAddr == AD7877_ALERT_REG)
            {
                pDevice->AlertReg       = SpiData;
            }
            /* AD7877 register address to write */
            SpiData |= (RegAddr << ADI_AD7877_REG_ADDR_SHIFT);
        }
        /* Submit the 1D output buffer to the SPI device */
        Result = adi_dev_Write(pDevice->SpiHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)Spi1DBuf);
    }
    else    /* Read AD7877 */
    {
        /* Submit the 1D dummy output buffer to the SPI device */
        Result = adi_dev_Write(pDevice->SpiHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)Spi1DBuf);
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            /* Submit the 1D input buffer to the SPI device */
            Result = adi_dev_Read(pDevice->SpiHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)Spi1DBuf);
        }
    }
    
    /* SPI Chipselect is controlled by hardware */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
        /* wait untill SPI device is done with AD7877 register read/write   */
        /* SPI Callback function will update SpiCompleteFlag */
        while(Spi1DBuf->ProcessedFlag == FALSE)
        {
            asm("nop;");
        }
        
        /* Read AD7877 register? */
        if (AccessFlag == ADI_AD7877_REGISTER_READ)
        {
            *RegData = SpiData; /* pass the read value */
        }
    }
    
    /* return */
    return (Result);
}

/*********************************************************************
*
*   Function:       SpiOpen
*
*   Description:    Opens the SPI device to access AD7877 registers
*
*********************************************************************/
static u32 SpiOpen( 
    ADI_AD7877          *pDevice    /* pointer to the device we're working on   */
) {
  
    /* Return value - assume we're going to be successful   */
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    
    /* SPI configuration table to access AD7877 registers */
    /* Baud rate, Active low SCLK, chained dataflow, Blackfin SPI as Master, 
       16bit SPI data packet, Hardware controlled chip select, Activate AD7877 chip select,
       Enable SPI dataflow */
    ADI_DEV_CMD_VALUE_PAIR SpiConfig[] = 
    {     	
   	    { ADI_SPI_CMD_SET_BAUD_REG,         (void *)0x1FF                   }, 	
  	    { ADI_SPI_CMD_SET_CLOCK_POLARITY,   (void *)0                       },  
  	    { ADI_DEV_CMD_SET_DATAFLOW_METHOD,  (void *)ADI_DEV_MODE_CHAINED    },  
   	    { ADI_SPI_CMD_SET_MASTER,           (void *)TRUE                    },  
	    { ADI_SPI_CMD_SET_WORD_SIZE,  		(void *)16    				    },	
   	    { ADI_SPI_CMD_SET_CLOCK_PHASE,      (void *)0                       },  
   	    { ADI_SPI_CMD_ENABLE_SLAVE_SELECT,  (void *)pDevice->SpiChipselect  },
   	    { ADI_DEV_CMD_SET_DATAFLOW,         (void *)TRUE                    },  
   	    { ADI_DEV_CMD_END,                  (void *)NULL                    }
    };

    /* Open SPI device (driver) */
    if ((Result = adi_dev_Open(
                        pDevice->ManagerHandle,         /* Device Manager handle            */
                        &ADISPIIntEntryPoint,         	/* SPI Entry point                  */
                        pDevice->SpiDevNumber,          /* SPI device number                */
                        NULL,                      	    /* client handle                    */
                        &pDevice->SpiHandle,            /* pointer to SPI driver handle     */
                        ADI_DEV_DIRECTION_BIDIRECTIONAL,/* Bidirectional data transfer      */
                        NULL,                           /* No DMA                           */
                        pDevice->DCBHandle,             /* handle to the callback manager   */
                        SpiCallback                     /* Spi Callback Function            */
        )) == ADI_DEV_RESULT_SUCCESS)
    {
        /* Configure SPI driver for AD7877 device access */
 	    Result = adi_dev_Control(pDevice->SpiHandle,ADI_DEV_CMD_TABLE,(void*)SpiConfig );
    }
    
    /* Return */
    return (Result);
}

/*********************************************************************
*
*   Function:       SpiClose
*
*   Description:    Closes SPI device opened for AD7877 access
*
*********************************************************************/
static u32 SpiClose( 
    ADI_AD7877          *pDevice    /* pointer to the device we're working on   */
) {
  
    /* Return value - assume we're going to be successful   */
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    /* Configuration table to Terminate SPI device */
    /* Disable SPI dataflow, Deactivate AD7877 slave select */
    ADI_DEV_CMD_VALUE_PAIR SpiTerminate[] = 
    {
        { ADI_DEV_CMD_SET_DATAFLOW,         (void *)FALSE                   },
        { ADI_SPI_CMD_DISABLE_SLAVE_SELECT, (void *)pDevice->SpiChipselect  },
        { ADI_DEV_CMD_END,                  (void *)NULL                    }
    };

    /* Terminate SPI Device */
    if ((Result = adi_dev_Control(pDevice->SpiHandle,ADI_DEV_CMD_TABLE,
                                 (void*)SpiTerminate)) == ADI_DEV_RESULT_SUCCESS)
    {
        /* Close SPI device */
        Result = adi_dev_Close(pDevice->SpiHandle);
    }
    
    /* return */
    return (Result);
}

/*********************************************************************

 Function:  PenIrqCallback

 Description: Callback routine for AD7877 PENIRQ Interrupt

*********************************************************************/
static void PenIrqCallback (
	void* 	DeviceHandle,       /* Device handle            */
	u32 	Event,              /* callback Event ID        */
	void* 	pArg                /* Callback argument        */
){
    /* retrive the device we're working on */
    ADI_AD7877  *pDevice    = (ADI_AD7877 *)DeviceHandle;
	u32	PenirqEvent;
/* Debug build only */
#ifdef ADI_DEV_DEBUG
	/* verify the Device handle */
    if (ValidatePDDHandle(DeviceHandle) == ADI_DEV_RESULT_SUCCESS) 
    {
#endif
        /* sense whether the screen is in touch or released state */
        if (pDevice->IntEnableFlag & ADI_AD7877_PENIRQ_STATUS)
		{
			/* PENIRQ interrupt status bit indicates the screen was in touch state. 
        		This interrupt must be caused by screen release event */
        	/* clear this status bit to indicate that screen is not touched */
        	pDevice->IntEnableFlag &= ~ADI_AD7877_PENIRQ_STATUS;
        	PenirqEvent = ADI_AD7877_EVENT_PENIRQ_RELEASE;
        	/* change flag to trigger on falling edge so that we could detect next screen touch */
			adi_flag_SetTrigger(pDevice->PenIrqPort.FlagId,ADI_FLAG_TRIGGER_FALLING_EDGE);
        }
        else
		{
			/* PENIRQ interrupt status bit indicates the screen was in release state. 
        		This interrupt must be caused by screen touch event */
        	/* Set this status bit to indicate that screen is touched */
        	pDevice->IntEnableFlag |= ADI_AD7877_PENIRQ_STATUS;
        	PenirqEvent = ADI_AD7877_EVENT_PENIRQ;
			/* change flag to trigger on raising edge so that we could detect screen release */
			adi_flag_SetTrigger(pDevice->PenIrqPort.FlagId,ADI_FLAG_TRIGGER_RISING_EDGE);
        }
        	
        /* if (AD7877 ADC in Master sequence mode?) */
        if ((pDevice->ControlReg1 & ADI_AD7877_CTRL_REG1_MODE_MASK) == ADI_AD7877_MASTER_SEQUENCE) 
		{
            /* if (DAV monitoring enabled) */
            if (pDevice->IntEnableFlag & ADI_AD7877_DAV_BIT)
			{
                /* post callback to client indicating that PENIRQ has occurred */
                /* (parameters are Device Handle, PenirqEvent, NULL) */
            	(pDevice->DMCallback)(pDevice->DeviceHandle, PenirqEvent, (void *) NULL);
        	}
        }
        /*  Single channel / Slave sequence / Do not convert ADC modes */
        else
		{
            /* post callback to client indicating that PENIRQ has occurred */
            /* (parameters are Device Handle, PenirqEvent, NULL) */
        	(pDevice->DMCallback)(pDevice->DeviceHandle, PenirqEvent, (void *) NULL);
		}

/* Debug build only */
#ifdef ADI_DEV_DEBUG
    }
#endif
}

/*********************************************************************

 Function:  DavIrqCallback

 Description: Callback routine for AD7877 DAV Interrupt

*********************************************************************/
static void DavIrqCallback (
	void* 	DeviceHandle,       /* Device handle            */
	u32 	Event,              /* callback Event ID        */
	void* 	pArg                /* Callback argument        */
){
    /* retrive the device we're working on */
    ADI_AD7877  *pDevice    = (ADI_AD7877 *)DeviceHandle;
    /* instance to access block of registers */
    ADI_DEV_ACCESS_REGISTER_BLOCK   AccessBlock;
    u16 Temp;
    u32 EventId;  /* Callback event to client */
    
/* Debug build only */
#ifdef ADI_DEV_DEBUG
	/* verify the Device handle */
    if (ValidatePDDHandle(DeviceHandle) == ADI_DEV_RESULT_SUCCESS) 
    {
#endif
		/* if (AD7877 ADC in Single channel mode?) */
        if ((pDevice->ControlReg1 & ADI_AD7877_CTRL_REG1_MODE_MASK) == ADI_AD7877_SINGLE_CHANNEL)
        {
        	/* Read the Result value of the converted ADC channel */
            /* Access a single ADC channel Result register */
            AccessBlock.Count   = 1;
            /* ADC channel selected for conversion */
            Temp = ((pDevice->ControlReg1 & ~ADI_AD7877_CTRL_REG1_CHADD_MASK) 
                                        >> ADI_AD7877_CHADD_FIELD_SHIFT);
            /* Result register address to access */
            AccessBlock.Address = (Temp + AD7877_YPOS);
            /* location to save the read result value */
            AccessBlock.pData   = (((u16 *)(&pDevice->ResultReg)) + Temp);
            /* Callback Event ID to client */
            EventId = ADI_AD7877_EVENT_SINGLE_DAV;
		}                
        /*  other ADC modes */
        else
        {
        	/* Read all ADC channel Results */
            AccessBlock.Count   = 11;
            /* Start in address of result register block to access */
            AccessBlock.Address = AD7877_YPOS;
            AccessBlock.pData   = (u16 *)(&pDevice->ResultReg);
            /* Callback Event ID to client */
            EventId = ADI_AD7877_EVENT_SEQUENCER_DAV;
		}
            
        /* Read ADC channel result(s) */
        if (adi_dev_Control (
        		pDevice->DeviceHandle,          /* AD7877 device handle */
                ADI_DEV_CMD_REGISTER_BLOCK_READ,/* Read block of AD7877 regs */
                (void *) &AccessBlock           /* pointer to block access structure */
               ) == ADI_DEV_RESULT_SUCCESS)
		{
        	/* post callback to client indicating that DAV has occurred */
            /* (parameters are Device Handle, EventId, pointer to location holding ADC result(s)) */
            (pDevice->DMCallback)(pDevice->DeviceHandle, EventId, (void *) AccessBlock.pData);
       	}

/* Debug build only */
#ifdef ADI_DEV_DEBUG
    }
#endif
}

/*********************************************************************

 Function:  AlertIrqCallback

 Description: Callback routine for AD7877 ALERT Interrupt

*********************************************************************/

static void AlertIrqCallback (
	void* 	DeviceHandle,       /* Device handle            */
	u32 	Event,              /* callback Event ID        */
	void* 	pArg                /* Callback argument        */
){
    /* retrive the device we're working on */
    ADI_AD7877  *pDevice    = (ADI_AD7877 *)DeviceHandle;
    /* instance to access a AD7877 device register */
    ADI_DEV_ACCESS_REGISTER   AccessReg;
    
/* Debug build only */
#ifdef ADI_DEV_DEBUG
	/* verify the Device handle */
    if (ValidatePDDHandle(DeviceHandle) == ADI_DEV_RESULT_SUCCESS) 
    {
#endif
		/* Read AD7877 Alert status register value */
        AccessReg.Address   = AD7877_ALERT_REG;
        AccessReg.Data      = 0;
            
        /* Read Alert reg value */
        if (adi_dev_Control (
        		pDevice->DeviceHandle,      /* AD7877 device handle     */
                ADI_DEV_CMD_REGISTER_READ,  /* Read a AD7877 register   */
                (void *) &AccessReg         /* pointer to register access structure */
               ) == ADI_DEV_RESULT_SUCCESS)
		{
        	/* update internal Alert reg cache */
            pDevice->AlertReg = AccessReg.Data;
            /* post callback to client indicating that ALERT has occurred */
            /* (parameters are Device Handle, ADI_AD7877_EVENT_ALERT, pointer to location holding Alert reg value ) */
            (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_AD7877_EVENT_ALERT, (void *) &pDevice->AlertReg);
		}

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
    ADI_DEV_PDD_HANDLE      PDDHandle   /* PDD handle of a AD7877 device  */
)
{    
    /* Return value - insure the client has provided a valid PDD Handle */
    u32     Result = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    /* index */
    u32     i;
    
    /* compare the given PDD handle with PDDHandle allocated to all EPPI devices in the list    */
    for (i = 0; i < ADI_AD7877_NUM_DEVICES; i++) 
    {        
        if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i])
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

/*****/

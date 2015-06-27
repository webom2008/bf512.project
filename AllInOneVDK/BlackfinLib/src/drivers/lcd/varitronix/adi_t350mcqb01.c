/**********************************************************************

Copyright(c) Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software 
you agree to the terms of the associated Analog Devices License Agreement.  


Description:
    This is the driver source code for Valitronix COG-T350MCQB-01
            
***********************************************************************/
/*********************************************************************

Include files

*********************************************************************/

#include <services/services.h>              	/* system service includes             	*/
#include <drivers/adi_dev.h>                	/* device manager includes            	*/
#include <drivers/ppi/adi_ppi.h>          	        /* PPI driver includes            */
#include <drivers/lcd/varitronix/adi_t350mcqb01.h>	/* Varitronix  LCD driver includes*/

/*********************************************************************

Enumerations and defines

*********************************************************************/

/* number of Varitronix T350MCQB01 LCD(s) in the system                                     */
#define ADI_NUM_T350MCQB01_DEVICES     (sizeof(Device)/sizeof(ADI_T350MCQB01))

/* Varitronix T350MCQB01 LCD operating frequency (8MHz) */

/*********************************************************************

Data Structures

*********************************************************************/

typedef struct {                                /* T350MCQB01 device structure             */
    void                    *pEnterCriticalArg; /* critical region argument                 */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle;      /* Manager Handle                           */  
    ADI_DEV_PDD_HANDLE      PPIHandle;          /* Handle to the underlying PPI driver 	*/
    ADI_DEV_DEVICE_HANDLE   DeviceHandle;       /* Handle of Device instance                */
    ADI_DMA_MANAGER_HANDLE  DMAHandle;          /* Handle to the DMA manager                */
    ADI_DCB_HANDLE          DCBHandle;          /* callback handle                          */
    ADI_DCB_CALLBACK_FN     DMCallback;         /* callback function supplied by Dev Manager*/
    u8                      InUseFlag;          /* in use flag (in use when TRUE)           */
    u8                      PpiDeviceNumber;    /* PPI device number to use            	*/
} ADI_T350MCQB01;

/*********************************************************************

Device specific data

*********************************************************************/

static ADI_T350MCQB01 Device[] = {             /* Actual T350MCQB01 devices               */
    {
        NULL,                                   /* critical region argument                 */ 
        NULL,                                   /* Manager Handle                           */
        NULL,                                   /* Handle to the underlying PPI driver 	*/
        NULL,                                   /* Handle of Device Manager instance        */
        NULL,                                   /* Handle to the DMA manager                */
        NULL,                                   /* Callback handle                          */
        NULL,                                   /* callback function supplied by Dev Manager*/
        FALSE,                                  /* Device Not in use                        */
        0,                                      /* PPI Device 0 by default             	*/
    },
};

/*********************************************************************

Static functions

*********************************************************************/

static u32 adi_pdd_Open(                        /* Opens T350MCQB01 device                 */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* Device Manager handle                    */
    u32                     DeviceNumber,       /* T350MCQB01 Device number to open        */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* Device handle                            */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,        /* pointer to PDD handle location           */
    ADI_DEV_DIRECTION       Direction,          /* data direction                           */
    void                    *pEnterCriticalArg, /* critical region storage location         */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager                */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle                          */
    ADI_DCB_CALLBACK_FN     DMCallback          /* device manager callback function         */
);

static u32 adi_pdd_Close(                       /* Closes a T350MCQB01 device              */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of the device to close        */
);

static u32 adi_pdd_Read(                        /* Reads data/queues inbound buffer to PPI */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a T350MCQB01 device       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);
    
static u32 adi_pdd_Write(                       /*Writes data/queues outbound buffer to PPI*/ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a T350MCQB01 device       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);

static u32 adi_pdd_SequentialIO(                /* Sequentially read/writes data to a device*/ 
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a T350MCQB01 device       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);

static u32 adi_pdd_Control(                     /* Sets or senses device specific parameter */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a PPI device              */
    u32                     Command,            /* Command ID                               */
    void                    *Value              /* Command specific value                   */
);

/* Function to open a PPI device */
static u32 PpiOpen( 
    ADI_T350MCQB01         *pDevice            /* Pointer to the device we're working on   */
);

/* Function to close a PPI device */
static u32 PpiClose( 
    ADI_T350MCQB01         *pDevice            /* Pointer to the device we're working on   */
);

/* PPI Callback function */
static void PpiCallbackFunction(
    void                    *DeviceHandle,      /* Device Handle                            */
    u32                     Event,              /* Callback event                           */
    void                    *pArg               /* Callback Argument                        */
);



/* Debug Build only */
#if defined(ADI_DEV_DEBUG)

static u32 ValidatePDDHandle(                   /* Validates Physical Device Driver Handle  */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of a T350MCQB01 device       */
);

#endif

/*********************************************************************

Entry point for device manager

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT     ADIT350MCQB01EntryPoint = {
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
*   Description:    Opens a T350MCQB01 device for use
*
*********************************************************************/

static u32 adi_pdd_Open(                        /* Open T350MCQB01 device          */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* Device Manager handle            */
    u32                     DeviceNumber,       /* T350MCQB01 Device number to open*/
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
    u32                 Result = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we're working on               */
    ADI_T350MCQB01    *pDevice;
    /* exit critical region parameter                       */
    void                *pExitCriticalArg;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    if (DeviceNumber >= ADI_NUM_T350MCQB01_DEVICES)    /* check the device number */
    {
        Result = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;      /* Invalid Device number */
    }
    
    /*  Continue only if the device number is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* insure the device the client wants is available  */
        Result  = ADI_DEV_RESULT_DEVICE_IN_USE;
        /* T350MCQB01 device we're working on */
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
    
        /* Continue only when the PPI device is available for use  */
        if (Result == ADI_DEV_RESULT_SUCCESS) 
        {    
            /* This T350MCQB01 device has been reserved for this client */

            /* Configure this T350MCQB01 device instance */            
            /* store the Manager handle */
            pDevice->ManagerHandle  = ManagerHandle;
            /* and store the Device handle */
            pDevice->DeviceHandle   = DeviceHandle;
            /* and store the DMA Manager handle */
            pDevice->DMAHandle      = DMAHandle;
            /* and store the DCallback Manager handle */
            pDevice->DCBHandle      = DCBHandle;
            /* and callback function */
            pDevice->DMCallback     = DMCallback;
            /* clear the PPI device Handle */
            pDevice->PPIHandle     = NULL;
            /* save the physical device handle in the client supplied location */
            *pPDDHandle 			= (ADI_DEV_PDD_HANDLE *)pDevice;
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
*   Description:    Closes down a PPI device
*
*********************************************************************/

static u32 adi_pdd_Close(                       /* Closes a PPI device                     */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of the PPI device to close   */
)
{  
    /* Return value - assume we're going to be successful   */
    u32                 Result = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we're working on   */
    ADI_T350MCQB01    *pDevice;
    
/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    /* Validate the given PDDHandle */
    Result = ValidatePDDHandle(PDDHandle);
    
    /* Continue only if the given PDDHandle is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {    
#endif
       
        /* avoid casts */
        pDevice = (ADI_T350MCQB01 *)PDDHandle;
       
 		// close PPI driver
		Result = adi_dev_Close(pDevice->PPIHandle);
       		
        /* continue only if PPI dataflow is disabled */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {            
            /* mark this PPI device as closed */
            pDevice->InUseFlag = FALSE;
            /* unhook the PPI Error interrupt handler from the system */
            Result = adi_pdd_Control(PDDHandle, ADI_DEV_CMD_SET_ERROR_REPORTING, (void *)FALSE);
                    
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
*   Description:    Never called as T350MCQB01 is an outbound device
*
*********************************************************************/
static u32 adi_pdd_Read(                /* Reads data/queues inbound buffer to PPI */ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle of a T350MCQB01 device       */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer                        */
)
{
    
    /* Return error as T350MCQB01 is an outbound device */
    return(ADI_DEV_RESULT_FAILED);
}
 
/*********************************************************************
*
*   Function:       adi_pdd_Write
*
*   Description:    Pass this to PPI device
*
*********************************************************************/
static u32 adi_pdd_Write(               /*Writes data/queues outbound buffer to PPI*/ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle of a T350MCQB01 device       */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer                        */
)
{
    /*  Pointer to the device instance we're working on */
    ADI_T350MCQB01    *pDevice;
    /* assume we're going to be success */
    u32                 Result = ADI_DEV_RESULT_SUCCESS; 
    /* avoid casts */
    pDevice         = (ADI_T350MCQB01 *)PDDHandle;
    
/* Debug build only */
#if defined(ADI_DEV_DEBUG)
    /* Validate the PDD handle */
    if ((Result = ValidatePDDHandle(PDDHandle)) == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* IF (PPI is already open) */
        if (pDevice->PPIHandle)
        {
            /* pass the request on to PPI */
            Result = adi_dev_Write (pDevice->PPIHandle, BufferType, pBuffer);
        }
        /* ELSE (return error) */
        else
        {
            Result = ADI_T350MCQB01_RESULT_PPI_STATE_INVALID;
        }
/* Debug Build Only */
#ifdef ADI_DEV_DEBUG
    }
#endif

    /* return */
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_SequentialIO
*
*   Description:    T350MCQB01 driver doesnot support this function
*
*********************************************************************/
static u32 adi_pdd_SequentialIO(        /* Sequentially read/writes data to a device*/ 
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle of a T350MCQB01 device       */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer                        */
)
{
    
    /* this function is not supported by T350MCQB01 */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************
*
*   Function:       adi_pdd_Control
*
*   Description:    Senses or Configures T350MCQB01 driver or 
*                   PPI device registers
*
*********************************************************************/
static u32 adi_pdd_Control(             /* Sets or senses device specific parameter */
    ADI_DEV_PDD_HANDLE  PDDHandle,      /* PDD handle of a PPI device              */
    u32                 Command,        /* Command ID                               */
    void                *Value          /* Command specific value                   */
)
{
    /*  Pointer to the device instance we're working on */
    ADI_T350MCQB01    *pDevice;
    u32                 Result;         /* return value                             */
    u32                 u32Value;       /* u32 type to avoid casts/warnings etc.    */    
    u8                  u8Value;        /* u8 type to avoid casts/warnings etc.     */
    
    /* assume we're going to be successful */
    Result          = ADI_DEV_RESULT_SUCCESS;
    /* avoid casts */
    pDevice         = (ADI_T350MCQB01 *)PDDHandle;
    /* assign 16 and 32 bit values for the Value argument */
    u32Value        = (u32)Value;
    u8Value         = (u8)u32Value;
    
/* for Debug build only - check for errors if required  */
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
            /* CASE: control dataflow */
            case (ADI_DEV_CMD_SET_DATAFLOW):

                    /* Enable dataflow? */
                if (u8Value)
                {
                    /* Enable PPI dataflow */
                    if ((Result = adi_dev_Control(pDevice->PPIHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)TRUE)) != ADI_DEV_RESULT_SUCCESS)
                    {                            
                        break;  /* exit on error */
                    }
                }
                break;
            
            /* CASE: query for processor DMA support */
            case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
        
                /* PPI uses DMA but T350MCQB01  doesn't */
                *((u32 *)Value) = FALSE;
                break;


            /* CASE: Set PPI Device Number to use */
            case (ADI_T350MCQB01_CMD_SET_PPI_DEV_NUMBER):

                /* save the PPI Device Number passed by the client */
                pDevice->PpiDeviceNumber = u8Value;
                break;
                
            /* CASE: Open/Close PPI Device connected to T350MCQB01 LCD */
            case (ADI_T350MCQB01_CMD_SET_OPEN_PPI_DEVICE):

                /* IF (Open the PPI device) */
                if (u8Value)
                {
                    Result = PpiOpen(pDevice);
                }
                /* ELSE (Close the PPI device) */
                else
                {
                    Result = PpiClose(pDevice);
                }
                break;
            
            /* CASE (Not a T350MCQB01 driver specific command. Pass it to PPI driver */    
            default:

                /* IF (PPI device is already open) */
                if (pDevice->PPIHandle)
                {
                    /* pass the unknown command to PPI */
                    Result = adi_dev_Control( pDevice->PPIHandle, Command, Value );
                }
                /* ELSE (Command not supported, return error) */
                else
                {
                    Result = ADI_T350MCQB01_RESULT_CMD_NOT_SUPPORTED;
                }
                break;
        }
    
/* Debug Build Only */
#ifdef ADI_DEV_DEBUG
    }
#endif
    return(Result);
}



static ADI_PPI_FS_TMR T350MCQB01_HS_tmr =
{
	false, //u32	pulse_hi;				// TRUE for pulse high, FALSE for pulse low
	true, //u32 emu_run;				    // TRUE for timer to run when emulator halted, FALSE otherwise
	T350MCQB01_H_PERIOD, //u32 period;					// number of PPI_CLK cycles per period
	T350MCQB01_H_PULSE, //u32 width;					// number of PPI_CLK cycles per pulse width
	2                                       // Number of blanking data clocks before the horizontal synch
	                                        // This represents the number of PPI clock delays before enabling
	
};
static ADI_PPI_FS_TMR T350MCQB01_VS_tmr =
{
	false, //u32	pulse_hi;				// TRUE for pulse high, FALSE for pulse low
	true, //u32 emu_run;				    // TRUE for timer to run when emulator halted, FALSE otherwise
	T350MCQB01_V_PERIOD, //u32 period;					// number of PPI_CLK cycles per period
	T350MCQB01_V_PULSE, //u32 width;					// number of PPI_CLK cycles per pulse width
	
};

#define T350MCQB01_PPI_CONTROL ( \
  (1 << 1) |  /* PORT_DIR = 1   PPI Transmit mode (output) */ \
  (3 << 2) |  /* XFER_TYPE = 3  Output Mode with 1,2 or 3 frame sync (using 2 frame syncs)  */ \
  (1 << 4) |  /* PORT_CFG = 1   2 or 3 frame syncs (using 2 frame syncs) */ \
              /* FLD_SEL = 0    n/a for LCD RGB driver */ \
  (1 << 7) |  /* PACK_EN = 1    unpacking enabled (2 bytes per clock) */ \
              /* SKIP_EN = 0 */ \
              /* SKIP_E0 = 0 */ \
              /* DLEN = 0       8 bit data */ \
              /* POLC = n/a */ \
  (1 << 15)   /* POLS = 1       Do not send data to LCD until it is ready for it */ \
  )
  
  
#define USE_STREAMING

/*********************************************************************
*
*   Function:       PpiOpen
*
*   Description:    Opens PPI device
*
*********************************************************************/
static u32 PpiOpen( 
    ADI_T350MCQB01         *pDevice            /* Pointer to the device we're working on   */
)
{   
    /* assume we're going to be success */
    u32 Result = ADI_DEV_RESULT_SUCCESS; 
    
    
	ADI_DEV_CMD_VALUE_PAIR PPILcdConfig [] = {		// table of PPI driver configuration values
		{ ADI_PPI_CMD_SET_CONTROL_REG,	 			(void *)(T350MCQB01_PPI_CONTROL)	},
		{ ADI_PPI_CMD_SET_TRANSFER_COUNT_REG,		(void *)(T350MCQB01_H_LINES*3-1)    },
		{ ADI_PPI_CMD_SET_DELAY_COUNT_REG	,		(void *)T350MCQB01_HSTART           },
#if defined(USE_STREAMING)
		{ ADI_DEV_CMD_SET_STREAMING,				(void *)TRUE						},
#endif
		{ ADI_PPI_CMD_SET_TIMER_FRAME_SYNC_1,       &T350MCQB01_HS_tmr                  },
		{ ADI_PPI_CMD_SET_TIMER_FRAME_SYNC_2,       &T350MCQB01_VS_tmr                  },
        /* The BF52x HRM Rev 0.3 wrongly mentions that this register is not required 
           to be programmed with internal frame syns and Tx mode. 
           But the design team confirms that this register needs to be programmed for
           Rx and Tx modes with internal and external frame syncs. This will be updated
           in future revisions of the mannual.
        */
        { ADI_PPI_CMD_SET_LINES_PER_FRAME_REG,      (void *)T350MCQB01_V_LINES          },
		{ ADI_DEV_CMD_END,							NULL								},
	};
	
	
    /* Open the PPI driver */
    Result = adi_dev_Open( 
        pDevice->ManagerHandle,     /* Device manager handle                                */
        &ADIPPIEntryPoint,         /* PPI Entry point                                     */
        pDevice->PpiDeviceNumber,   /* PPI device number                               	*/
        pDevice,                    /* client handle - passed to internal callback function */
        &pDevice->PPIHandle,        /* pointer to DM handle (for PPI driver) location  	*/
        ADI_DEV_DIRECTION_OUTBOUND, /* PPI used only to output video data              	*/
        pDevice->DMAHandle,         /* handle to the DMA manager                            */
        pDevice->DCBHandle,         /* handle to the callback manager                       */
        PpiCallbackFunction         /* callback function for PPI driver       				*/
    );

    /* Continue only when the PPI driver open is a success */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
	    /* table of PPI driver configuration values */
        /* Configure PPI device for T350MCQB01 */    
        Result = adi_dev_Control( pDevice->PPIHandle, ADI_DEV_CMD_TABLE, (void*)PPILcdConfig );
    }
    
    /* return */
    return (Result);
}

/*********************************************************************
*
*   Function:       PpiClose
*
*   Description:    Closes PPI device used by T350MCQB01
*
*********************************************************************/
static u32 PpiClose( 
    ADI_T350MCQB01         *pDevice            /* Pointer to the device we're working on   */
)
{    
    /* assume we're going to be success */
    u32 Result = ADI_DEV_RESULT_SUCCESS; 
        
    /* Check if any PPI device is open */
    /* if so, close the present PPI device in use */
    if (pDevice->PPIHandle != NULL)
    {
        /* close PPI device */
        if ((Result = adi_dev_Close(pDevice->PPIHandle))== ADI_DEV_RESULT_SUCCESS)
        {
            /* Mark PPI Handle as NULL indicating that the device is closed */
            pDevice->PPIHandle = NULL;
        }
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       PpiCallbackFunction

    Description:    Callback for PPI driver

*********************************************************************/
static void PpiCallbackFunction(
    void		*DeviceHandle,     	/* Device Handle 	 */ 
    u32         Event,              /* Callback event 	 */
    void        *pArg				/* Callback Argument */
)
{
    /*  Pointer to the device instance we're working on */
    ADI_T350MCQB01    *pDevice;
    pDevice = (ADI_T350MCQB01 *)DeviceHandle;
    
/* Debug build only */
#if defined(ADI_DEV_DEBUG)
    /* Validate the PDD handle */
    if (ValidatePDDHandle(DeviceHandle) == ADI_DEV_RESULT_SUCCESS)
    {
#endif

    	/* simply pass the callback along to the Device Manager Callback */
    	(pDevice->DMCallback)(pDevice->DeviceHandle,Event,pArg);

/* Debug Build Only */
#ifdef ADI_DEV_DEBUG
    }
#endif
}

   
#if defined(ADI_DEV_DEBUG)  /* Debug build only */
/*********************************************************************

    Function:       ValidatePDDHandle

    Description:    Validates a PDD handle

*********************************************************************/

static u32 ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of a PPI device  */
)
{    
    /* Return value - insure the client has provided a valid PDD Handle */
    u32     Result = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    /* index */
    u32     i;
    
    /* compare the given PDD handle with PDDHandle allocated to all PPI devices in the list    */
    for (i = 0; i < ADI_NUM_T350MCQB01_DEVICES; i++) 
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

#endif /* ADI_DEV_DEBUG */

/*****/

/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_ppi.c,v $
$Revision: 5506 $
$Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $

Description:
            This is the driver source code for the PPI peripheral.

*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/


#include <services/services.h>      // system service includes
#include <drivers/adi_dev.h>        // device manager includes
#include <drivers/ppi/adi_ppi.h>    // ppi driver includes

/*********************************************************************

Enumerations and defines

*********************************************************************/

#define ADI_PPI_NUM_DEVICES (sizeof(Device)/sizeof(ADI_PPI))    // number of PPI devices in the system


/*********************************************************************

Data Structures

*********************************************************************/

typedef struct {                                // PPI device structure
    ADI_PPI_CONTROL_REG     *PPIControl;            // PPI control register
    ADI_PPI_STATUS_REG      *PPIStatus;             // PPI status register
    volatile u16            *PPICount;              // PPI count register
    volatile u16            *PPIDelay;              // PPI delay register
    volatile u16            *PPIFrame;              // PPI frame register
    u32                     InUseFlag;              // in use flag (TRUE - in use, FALSE otherwise)
    u32                     TripleFrameSyncFlag;    // flag indicating triple frame syncs are in use
    ADI_INT_PERIPHERAL_ID   ErrorPeripheralID;      // peripheral ID of the PPI's error interrupt
    void                    *pEnterCriticalArg;     // critical region argument
    ADI_DMA_PMAP            MappingID;              // DMA peripheral mapping ID
    u32                     TimerID_FS1;            // frame sync 1 timer ID
    u32                     TimerID_FS2;            // frame sync 2 timer ID
    u16                     TimerOpenFlag_FS1;      // frame sync 1 timer open flag
    u16                     TimerOpenFlag_FS2;      // frame sync 2 timer open flag
    u16                     ErrorReportingFlag;     // error reporting flag
    ADI_DCB_HANDLE          DCBHandle;              // callback handle
    ADI_DEV_DEVICE_HANDLE   DeviceHandle;           // device manager handle
    void (*DMCallback) (                            // device manager's callback function
        ADI_DEV_DEVICE_HANDLE DeviceHandle,             // device handle
        u32 Event,                                      // event ID
        void *pArg);                                    // argument pointer
    eADI_PPI_FRAME_SYNC_MODE_CFG  FrameSyncCFG;         // frame sync Configuration
} ADI_PPI;



/*********************************************************************

Device specific data

*********************************************************************/

/********************
    Edinburgh
********************/

#if defined(__ADSP_EDINBURGH__)     // settings for Edinburgh class devices

static ADI_PPI Device[] = {         // Actual PPI devices
    {                                   // device 0
        (ADI_PPI_CONTROL_REG *)pPPI_CONTROL,// PPI control register
        (ADI_PPI_STATUS_REG *)pPPI_STATUS,  // PPI status register
        (volatile u16 *)pPPI_COUNT,         // PPI count register
        (volatile u16 *)pPPI_DELAY,         // PPI delay register
        (volatile u16 *)pPPI_FRAME,         // PPI frame register
        FALSE,                              // in use flag
        FALSE,                              // triple frame sync flag
        ADI_INT_PPI_ERROR,                  // peripheral ID of the PPI's error interrupt
        NULL,                               // critical region argument
        ADI_DMA_PMAP_PPI,                   // DMA peripheral mapping ID
        ADI_TMR_GP_TIMER_1,                 // frame sync 1 timer ID
        ADI_TMR_GP_TIMER_2,                 // frame sync 2 timer ID
        FALSE,                              // frame sync 1 timer open flag
        FALSE,                              // frame sync 2 timer open flag
        FALSE,                              // error reporting flag
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_PPI_FRAME_SYNC_CFG_DEFAULT,     // Default frame sync Configuration
    },
};

#endif


/********************
    Braemar
********************/

#if defined(__ADSP_BRAEMAR__)       // settings for Braemar class devices

static ADI_PPI Device[] = {         // Actual PPI devices
    {                                   // device 0
        (ADI_PPI_CONTROL_REG *)pPPI_CONTROL,// PPI control register
        (ADI_PPI_STATUS_REG *)pPPI_STATUS,  // PPI status register
        (volatile u16 *)pPPI_COUNT,         // PPI count register
        (volatile u16 *)pPPI_DELAY,         // PPI delay register
        (volatile u16 *)pPPI_FRAME,         // PPI frame register
        FALSE,                              // in use flag
        FALSE,                              // triple frame sync flag
        ADI_INT_PERIPHERAL_ERROR,           // peripheral ID of the PPI's error interrupt
        NULL,                               // critical region argument
        ADI_DMA_PMAP_PPI,                   // DMA peripheral mapping ID
        ADI_TMR_GP_TIMER_0,                 // frame sync 1 timer ID
        ADI_TMR_GP_TIMER_1,                 // frame sync 2 timer ID
        FALSE,                              // frame sync 1 timer open flag
        FALSE,                              // frame sync 2 timer open flag
        FALSE,                              // error reporting flag
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_PPI_FRAME_SYNC_CFG_DEFAULT,     // Default frame sync Configuration
    },
};

#endif

/********************
    Kookaburra/Mockingbird
********************/

#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__) || defined(__ADSP_BRODIE__)

static ADI_PPI Device[] = {         // Actual PPI devices
    {                                   // device 0
        (ADI_PPI_CONTROL_REG *)pPPI_CONTROL,// PPI control register
        (ADI_PPI_STATUS_REG *)pPPI_STATUS,  // PPI status register
        (volatile u16 *)pPPI_COUNT,         // PPI count register
        (volatile u16 *)pPPI_DELAY,         // PPI delay register
        (volatile u16 *)pPPI_FRAME,         // PPI frame register
        FALSE,                              // in use flag
        FALSE,                              // triple frame sync flag
        ADI_INT_PPI_ERROR,                  // peripheral ID of the PPI's error interrupt
        NULL,                               // critical region argument
        ADI_DMA_PMAP_PPI,                   // DMA peripheral mapping ID
        ADI_TMR_GP_TIMER_0,                 // frame sync 1 timer ID
        ADI_TMR_GP_TIMER_1,                 // frame sync 2 timer ID
        FALSE,                              // frame sync 1 timer open flag
        FALSE,                              // frame sync 2 timer open flag
        FALSE,                              // error reporting flag
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_PPI_FRAME_SYNC_CFG_DEFAULT,     // Default frame sync Configuration
    },
};

#endif

/********************
    Teton-Lite
********************/

#if defined(__ADSP_TETON__)         // settings for Teton class devices

static ADI_PPI Device[] = {         // Actual PPI devices
    {                                   // device 0
        (ADI_PPI_CONTROL_REG *)pPPI0_CONTROL,   // PPI control register
        (ADI_PPI_STATUS_REG *)pPPI0_STATUS,     // PPI status register
        (volatile u16 *)pPPI0_COUNT,            // PPI count register
        (volatile u16 *)pPPI0_DELAY,            // PPI delay register
        (volatile u16 *)pPPI0_FRAME,            // PPI frame register
        FALSE,                                  // in use flag
        FALSE,                                  // triple frame sync flag
        ADI_INT_PPI0_ERROR,                     // peripheral ID of the PPI's error interrupt
        NULL,                                   // critical region argument
        ADI_DMA_PMAP_PPI1,                      // DMA peripheral mapping ID
        ADI_TMR_GP_TIMER_8,                     // frame sync 1 timer ID
        ADI_TMR_GP_TIMER_9,                     // frame sync 2 timer ID
        FALSE,                                  // frame sync 1 timer open flag
        FALSE,                                  // frame sync 2 timer open flag
        FALSE,                                  // error reporting flag
        NULL,                                   // callback handle
        NULL,                                   // device manager handle
        NULL,                                   // device manager callback function
        ADI_PPI_FRAME_SYNC_CFG_DEFAULT,         // Default frame sync Configuration
    },
    {                                   // device 1
        (ADI_PPI_CONTROL_REG *)pPPI1_CONTROL,   // PPI control register
        (ADI_PPI_STATUS_REG *)pPPI1_STATUS,     // PPI status register
        (volatile u16 *)pPPI1_COUNT,            // PPI count register
        (volatile u16 *)pPPI1_DELAY,            // PPI delay register
        (volatile u16 *)pPPI1_FRAME,            // PPI frame register
        FALSE,                                  // in use flag
        FALSE,                                  // triple frame sync flag
        ADI_INT_PPI1_ERROR,                     // peripheral ID of the PPI's error interrupt
        NULL,                                   // critical region argument
        ADI_DMA_PMAP_PPI2,                      // DMA peripheral mapping ID
        ADI_TMR_GP_TIMER_10,                    // frame sync 1 timer ID
        ADI_TMR_GP_TIMER_11,                    // frame sync 2 timer ID
        FALSE,                                  // frame sync 1 timer open flag
        FALSE,                                  // frame sync 2 timer open flag
        FALSE,                                  // error reporting flag
        NULL,                                   // callback handle
        NULL,                                   // device manager handle
        NULL,                                   // device manager callback function
        ADI_PPI_FRAME_SYNC_CFG_DEFAULT,         // Default frame sync Configuration
    },
};

#endif


/********************
    Stirling
********************/

#if defined(__ADSP_STIRLING__)      // settings for Stirling class devices

static ADI_PPI Device[] = {         // Actual PPI devices
    {                                   // device 0
        (ADI_PPI_CONTROL_REG *)pPPI_CONTROL,// PPI control register
        (ADI_PPI_STATUS_REG *)pPPI_STATUS,  // PPI status register
        (volatile u16 *)pPPI_COUNT,         // PPI count register
        (volatile u16 *)pPPI_DELAY,         // PPI delay register
        (volatile u16 *)pPPI_FRAME,         // PPI frame register
        FALSE,                              // in use flag
        FALSE,                              // triple frame sync flag
        ADI_INT_PPI_ERROR,                  // peripheral ID of the PPI's error interrupt
        NULL,                               // critical region argument
        ADI_DMA_PMAP_PPI,                   // DMA peripheral mapping ID
        ADI_TMR_GP_TIMER_1,                 // frame sync 1 timer ID
        ADI_TMR_GP_TIMER_2,                 // frame sync 2 timer ID
        FALSE,                              // frame sync 1 timer open flag
        FALSE,                              // frame sync 2 timer open flag
        FALSE,                              // error reporting flag
        NULL,                               // callback handle
        NULL,                               // device manager handle
        NULL,                               // device manager callback function
        ADI_PPI_FRAME_SYNC_CFG_DEFAULT,     // Default frame sync Configuration
    },
};

#endif

/********************
   Moy (ADSP-BF50x)
********************/

#if defined(__ADSP_MOY__)  || defined(__ADSP_DELTA__)

/* Instance to handle Actual PPI devices available */
static ADI_PPI Device[] =
{
    /* Device 0 */
    {
        /* PPI control register */
        (ADI_PPI_CONTROL_REG *)pPPI_CONTROL,
        /* PPI status register */
        (ADI_PPI_STATUS_REG *)pPPI_STATUS,
        /* PPI count register */
        (volatile u16 *)pPPI_COUNT,
        /* PPI delay register */
        (volatile u16 *)pPPI_DELAY,
        /* PPI frame register */
        (volatile u16 *)pPPI_FRAME,
        /* Device in use flag */
        FALSE,
        /* triple frame sync flag */
        FALSE,
        /* peripheral ID of the PPI's error interrupt */
        ADI_INT_PPI_ERROR,
        /* critical region argument */
        NULL,
        /* DMA peripheral mapping ID */
        ADI_DMA_PMAP_PPI,
        /* frame sync 1 timer ID */
        ADI_TMR_GP_TIMER_0,
        /* frame sync 2 timer ID */
        ADI_TMR_GP_TIMER_1,
        /* frame sync 1 timer open flag */
        FALSE,
        /* frame sync 2 timer open flag */
        FALSE,
        /* error reporting flag */
        FALSE,
        /* callback handle */
        NULL,
        /* device manager handle */
        NULL,
        /* device manager callback function */
        NULL,
        /* Default frame sync Configuration */
        ADI_PPI_FRAME_SYNC_CFG_DEFAULT,
    },
};

#endif

/*********************************************************************

Control register setting data

*********************************************************************/

typedef struct {
    u16 Mask;
    u16 ShiftCount;
} ADI_PPI_COMMAND_MASK_ENTRY;

static ADI_PPI_COMMAND_MASK_ENTRY CommandMask[] = {     // Masks for adi_pdd_Control commands
    {0x8000,    15},                                        // invert frame sync polarity
    {0x4000,    14},                                        // invert PPI clk polarity
    {0x3800,    11},                                        // data length
    {0x0400,    10},                                        // skip even or odd elements
    {0x0200,    9},                                         // enable skipping
    {0x0080,    7},                                         // packing enable
    {0x0040,    6},                                         // active field select
    {0x0030,    4},                                         // port config
    {0x000C,    2},                                         // transfer type
    {0x0002,    1},                                         // port direction
};

static ADI_PPI_CONTROL_REG DefaultControlReg = {0};

/*********************************************************************

Static functions

*********************************************************************/


static u32 adi_pdd_Open(                        // Open a device
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

static u32 adi_pdd_Close(       // Closes a device
    ADI_DEV_PDD_HANDLE PDDHandle    // PDD handle
);

static u32 adi_pdd_Read(        // Reads data or queues an inbound buffer to a device
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER *pBuffer         // pointer to buffer
);

static u32 adi_pdd_Write(       // Writes data or queues an outbound buffer to a device
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER *pBuffer         // pointer to buffer
);

static u32 adi_pdd_Control(     // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    u32 Command,                    // command ID
    void *Value                     // command specific value
);

static u32 ppiSetFrameSyncTimer(    // configures frame sync timers
    u32             TimerID,            // timer ID
    u16             *pTimerOpenFlag,    // pointer to the timer open flag
    ADI_PPI_FS_TMR  *config             // pointer to timer config
);

static ADI_INT_HANDLER(ErrorHandler);       // interrupt handler for errors

#if defined(__ADSP_BRAEMAR__)       // static functions for 534, 536, 537
// note that although Stirling has port control, PPI is not affected by port control

static u32 ppiSetPortControl(           // configures the port control registers
    ADI_PPI                 *pDevice,       // pointer to the device
    u32                     OpenFlag        // open flag (TRUE - open, FALSE - close)
);

#endif

/* Static gunctions for ADSP-BF52x family */
#if defined(__ADSP_KOOKABURRA__)    ||\
    defined(__ADSP_MOCKINGBIRD__)   ||\
    defined(__ADSP_BRODIE__)        ||\
    defined(__ADSP_MOY__)           ||\
    defined(__ADSP_DELTA__)

static u32 ppiSetPortControl(
    ADI_PPI     *pDevice,
    u32         OpenFlag
);

#endif

#if defined(ADI_DEV_DEBUG)

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle);

#endif


/*********************************************************************

Entry point for device manager

*********************************************************************/


ADI_DEV_PDD_ENTRY_POINT ADIPPIEntryPoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control
};



/*********************************************************************
*
*   Function:       adi_pdd_Open
*
*   Description:    Opens a PPI device for use
*
*********************************************************************/


static u32 adi_pdd_Open(                        // Open a device
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,          // device manager handle
    u32                     DeviceNumber,           // device number
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,           // device handle
    ADI_DEV_PDD_HANDLE      *pPDDHandle,            // pointer to PDD handle location
    ADI_DEV_DIRECTION       Direction,              // data direction
    void                    *pEnterCriticalArg,     // enter critical region parameter
    ADI_DMA_MANAGER_HANDLE  DMAHandle,              // handle to the DMA manager
    ADI_DCB_HANDLE          DCBHandle,              // callback handle
    ADI_DCB_CALLBACK_FN     DMCallback              // client callback function
) {

    ADI_INT_IMASK   IMask;                  // temporary storage of IMask register
    u32             Result;                 // return value
    ADI_PPI         *pDevice;               // pointer to the device we're working on
    void            *pExitCriticalArg;      // exit critical region parameter

    // check for errors if required
#if defined(ADI_DEV_DEBUG)
    if (DeviceNumber >= ADI_PPI_NUM_DEVICES) {      // check the device number
        return (ADI_DEV_RESULT_BAD_DEVICE_NUMBER);
    }
    if (Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL) {         // check the direction
        return (ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED);
    }
#endif

    // insure the device the client wants is available
    Result = ADI_DEV_RESULT_DEVICE_IN_USE;
    pDevice = &Device[DeviceNumber];
    pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
    if (pDevice->InUseFlag == FALSE) {
        pDevice->InUseFlag = TRUE;
        Result = ADI_DEV_RESULT_SUCCESS;
    }
    adi_int_ExitCriticalRegion(pExitCriticalArg);
    if (Result != ADI_DEV_RESULT_SUCCESS) return (Result);

    // reset the actual device
    *pDevice->PPIControl = DefaultControlReg;
    *pDevice->PPICount = 0;
    *pDevice->PPIDelay = 0;
    *pDevice->PPIFrame = 0;

    // initialize the device settings
    pDevice->DCBHandle = DCBHandle;
    pDevice->DMCallback = DMCallback;
    pDevice->DeviceHandle = DeviceHandle;
    pDevice->TripleFrameSyncFlag = FALSE;
    pDevice->TimerOpenFlag_FS1 = FALSE;
    pDevice->TimerOpenFlag_FS2 = FALSE;
    pDevice->ErrorReportingFlag = FALSE;
    pDevice->pEnterCriticalArg = pEnterCriticalArg;
    pDevice->FrameSyncCFG = ADI_PPI_FRAME_SYNC_CFG_DEFAULT;

    // set the direction
    if (Direction == ADI_DEV_DIRECTION_OUTBOUND) {
        pDevice->PPIControl->port_dir = 1;
    }

    // save the physical device handle in the client supplied location
    *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;

    // return
    return(ADI_DEV_RESULT_SUCCESS);
}






/*********************************************************************
*
*   Function:       adi_pdd_Close
*
*   Description:    Closes down a PPI device
*
*********************************************************************/


static u32 adi_pdd_Close(       // Closes a device
    ADI_DEV_PDD_HANDLE PDDHandle    // PDD handle
) {

    u32             Result;     // return value
    ADI_PPI         *pDevice;   // pointer to the device we're working on

    // check for errors if required
#if defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    // avoid casts
    pDevice = (ADI_PPI *)PDDHandle;

    // disable dataflow
    if ((Result = adi_pdd_Control (PDDHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)FALSE)) != ADI_DEV_RESULT_SUCCESS) {
        return (Result);
    }

    // close any open timers
    if (pDevice->TimerOpenFlag_FS1) {
         if (adi_tmr_Close(pDevice->TimerID_FS1) != ADI_TMR_RESULT_SUCCESS) {
             return (ADI_PPI_RESULT_TIMER_ERROR);
         }
     }
    if (pDevice->TimerOpenFlag_FS2) {
         if (adi_tmr_Close(pDevice->TimerID_FS2) != ADI_TMR_RESULT_SUCCESS) {
             return (ADI_PPI_RESULT_TIMER_ERROR);
         }
     }

    // shut down error reporting
    Result = adi_pdd_Control(PDDHandle, ADI_DEV_CMD_SET_ERROR_REPORTING, (void *)FALSE);

    // mark the device as closed
    pDevice->InUseFlag = FALSE;

    // return
    return(Result);
}





/*********************************************************************
*
*   Function:       adi_pdd_Read
*
*   Description:    Never called as PPI uses DMA
*
*********************************************************************/


static u32 adi_pdd_Read(        // Reads data or queues an inbound buffer to a device
    ADI_DEV_PDD_HANDLE  PDDHandle,  // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER      *pBuffer    // pointer to buffer
){

    // this device uses processor DMA so we should never get here
    return(ADI_DEV_RESULT_FAILED);
}





/*********************************************************************
*
*   Function:       adi_pdd_Write
*
*   Description:    Never called as PPI uses DMA
*
*********************************************************************/


static u32 adi_pdd_Write(       // Writes data or queues an outbound buffer to a device
    ADI_DEV_PDD_HANDLE  PDDHandle,  // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER      *pBuffer    // pointer to buffer
){

    // this device uses processor DMA so we should never get here
    return(ADI_DEV_RESULT_FAILED);
}





/*********************************************************************
*
*   Function:       adi_pdd_Control
*
*   Description:    Configures the PPI device
*
*********************************************************************/


static u32 adi_pdd_Control(     // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE  PDDHandle,  // PDD handle
    u32                 Command,    // command ID
    void                *Value      // command specific value
) {

    ADI_PPI                     *pDevice;       // pointer to the device we're working on
    ADI_PPI_COMMAND_MASK_ENTRY  *pCommandMask;  // used to set/clear bitfields
    u32                         tmp;            // temporary storage
    u32                         Result;         // return value
    u32                         ErrorIVG;       // IVG for PPI error interrupts
    u16                         u16Value;       // u16 type to avoid casts/warnings etc.
    u32                         TimerIDs;       // timer IDs for frame syncs

    // avoid casts
    pDevice = (ADI_PPI *)PDDHandle;
    u16Value = ((u16)((u32)Value));

    // check for errors if required
#if defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    // assume we're going to be successful
    Result = ADI_DEV_RESULT_SUCCESS;

    // CASEOF (Command ID)
    switch (Command) {

        // CASE (control dataflow)
        case (ADI_DEV_CMD_SET_DATAFLOW):

/* Configure port control for the PPI if we're a
   Braemar/Kookaburra/Mockingbird/Brodie/Moy class device and we're enabling dataflow */
#if defined(__ADSP_BRAEMAR__) ||\
    defined(__ADSP_KOOKABURRA__) ||\
    defined(__ADSP_MOCKINGBIRD__) || \
    defined(__ADSP_BRODIE__)        ||\
    defined(__ADSP_MOY__)        ||\
    defined(__ADSP_DELTA__)

            if (u16Value)
            {
                if ((Result = ppiSetPortControl(pDevice, TRUE)) != ADI_DEV_RESULT_SUCCESS)
                {
                    break;
                }
            }
#endif
            // enable or disable PPI accordingly
            pDevice->PPIControl->port_en = u16Value;

            // enable/disable frame sync timers as necessary
            TimerIDs = 0;
            if (pDevice->TimerOpenFlag_FS1) {
                TimerIDs |= pDevice->TimerID_FS1;
                // enable Timer output pad for PWM OUT
                Result |= adi_tmr_GPControl(pDevice->TimerID_FS1, ADI_TMR_GP_CMD_SET_OUTPUT_PAD_DISABLE,(void *)FALSE);
            }
            if (pDevice->TimerOpenFlag_FS2) {
                TimerIDs |= pDevice->TimerID_FS2;
                // enable Timer output pad for PWM OUT
                Result |= adi_tmr_GPControl(pDevice->TimerID_FS2, ADI_TMR_GP_CMD_SET_OUTPUT_PAD_DISABLE,(void *)FALSE);
            }

            Result |= adi_tmr_GPGroupEnable(TimerIDs, u16Value);

            if (Result != ADI_TMR_RESULT_SUCCESS) {
                Result = ADI_PPI_RESULT_TIMER_ERROR;
            }
            break;

        // CASE (query for processor DMA support)
        case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):

            // yes, we do support it
            *((u32 *)Value) = TRUE;
            break;

        // CASE (query for our inbound or outbound DMA peripheral ID)
        case (ADI_DEV_CMD_GET_INBOUND_DMA_PMAP_ID):
        case (ADI_DEV_CMD_GET_OUTBOUND_DMA_PMAP_ID):

            // tell him what it is
            *((ADI_DMA_PMAP *)Value) = pDevice->MappingID;
            break;

        // CASE ( error reporting)
        case (ADI_DEV_CMD_SET_ERROR_REPORTING):

            // don't do anything if nothing is being changed
            if (((u32)Value) == pDevice->ErrorReportingFlag) {
                break;
            }

            // get the error interrupt IVG
            adi_int_SICGetIVG(pDevice->ErrorPeripheralID, &ErrorIVG);

            // IF (enabling)
            if (((u32)Value) == TRUE) {

                // hook the PPI error interrupt handler into the system
                if(adi_int_CECHook(ErrorIVG, ErrorHandler, pDevice, TRUE) != ADI_INT_RESULT_SUCCESS){
                    return (ADI_DEV_RESULT_CANT_HOOK_INTERRUPT);
                }
                adi_int_SICWakeup(pDevice->ErrorPeripheralID, TRUE);
                adi_int_SICEnable(pDevice->ErrorPeripheralID);

            // ELSE
            } else {

                // unhook the PPI error interrupt handler from the system
                adi_int_SICDisable(pDevice->ErrorPeripheralID);
                adi_int_SICWakeup(pDevice->ErrorPeripheralID, FALSE);
                if (adi_int_CECUnhook(ErrorIVG, ErrorHandler, pDevice) != ADI_INT_RESULT_SUCCESS) {
                    return(ADI_DEV_RESULT_CANT_UNHOOK_INTERRUPT);
                }

            // ENDIF
            }

            // update status
            pDevice->ErrorReportingFlag = ((u32)Value);
            break;

        // CASE (set PPI control register)
        case (ADI_PPI_CMD_SET_CONTROL_REG):

            // update the register and return
            *((u16 *)pDevice->PPIControl) = u16Value;
            break;

        // CASE (set delay count register)
        case (ADI_PPI_CMD_SET_DELAY_COUNT_REG):

            // update the register and return
            *pDevice->PPIDelay = u16Value;
            break;

        // CASE (set transfer count register)
        case (ADI_PPI_CMD_SET_TRANSFER_COUNT_REG):

            // update the register and return
            *pDevice->PPICount = u16Value;
            break;

        // CASE (set lines per frame)
        case (ADI_PPI_CMD_SET_LINES_PER_FRAME_REG):

            // update the register and return
            *pDevice->PPIFrame = u16Value;
            break;

        // CASE (set triple frame syncs)
        case (ADI_PPI_CMD_SET_TRIPLE_FRAME_SYNC):

            // update the value
            pDevice->TripleFrameSyncFlag = ((u32)Value);
            break;

        // CASE (set frame sync 1 timer)
        case (ADI_PPI_CMD_SET_TIMER_FRAME_SYNC_1):

            // set up the timer
            Result = ppiSetFrameSyncTimer(pDevice->TimerID_FS1, &pDevice->TimerOpenFlag_FS1, (ADI_PPI_FS_TMR *)Value);
            break;

        // CASE (set frame sync 2 timer)
        case (ADI_PPI_CMD_SET_TIMER_FRAME_SYNC_2):

            // set up the timer
            Result = ppiSetFrameSyncTimer(pDevice->TimerID_FS2, &pDevice->TimerOpenFlag_FS2, (ADI_PPI_FS_TMR *)Value);
            break;

            // Set the non default frame sync configuration
        case ADI_PPI_CMD_SET_FRAME_SYNC_CONFIGURATION:
            if ( ADI_PPI_FRAME_SYNC_CFG_END <= (eADI_PPI_FRAME_SYNC_MODE_CFG)((u32)Value))
            {
                Result = ADI_DEV_RESULT_NOT_SUPPORTED;
            }
            else
            {
                pDevice->FrameSyncCFG = (eADI_PPI_FRAME_SYNC_MODE_CFG)((u32)Value);
            }
            break;

        // CASE (setting a value in the config register)
        case ADI_PPI_CMD_SET_FS_INVERT:
        case ADI_PPI_CMD_SET_CLK_INVERT:
        case ADI_PPI_CMD_SET_DATA_LENGTH:
        case ADI_PPI_CMD_SET_SKIP_EVEN_ODD:
        case ADI_PPI_CMD_SET_SKIP_ENABLE:
        case ADI_PPI_CMD_SET_PACK_ENABLE:
        case ADI_PPI_CMD_SET_ACTIVE_FIELD_SELECT:
        case ADI_PPI_CMD_SET_PORT_CFG:
        case ADI_PPI_CMD_SET_TRANSFER_TYPE:
        case ADI_PPI_CMD_SET_PORT_DIRECTION:

            // point to the command mask
            pCommandMask = &CommandMask[Command - ADI_PPI_CMD_SET_FS_INVERT];

            // update the config register with the new value
            tmp = *((u16 *) pDevice->PPIControl);
            tmp &= ~(pCommandMask->Mask);
            tmp |= (u16Value << pCommandMask->ShiftCount);
            *((u16 *) pDevice->PPIControl) = tmp;
            break;

        // CASE ELSE
        default:

            // we don't understand this command
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;

    // ENDCASE
    }

    // return
    return(Result);
}



/*********************************************************************

    Function:       ErrorHandler

    Description:    Processes PPI error interrupts

*********************************************************************/

static ADI_INT_HANDLER(ErrorHandler)    // PPI error handler
{

    ADI_PPI                 *pDevice;   // pointer to the device we're working on
    ADI_PPI_STATUS_REG      PPIStatus;  // ppi status register
    ADI_INT_HANDLER_RESULT  Result;     // result

    // assume the interrupt is not for us
    Result = ADI_INT_RESULT_NOT_PROCESSED;

    // point to the device for this handler
    pDevice = (ADI_PPI *)ClientArg;

    // IF (this device is asserting an interrupt)
    if (adi_int_SICInterruptAsserted(pDevice->ErrorPeripheralID) == ADI_INT_RESULT_ASSERTED) {

        // read the PPIStatus 
        PPIStatus = *pDevice->PPIStatus;

		// Write back the status register to clear any pending interrupt
		*pDevice->PPIStatus	=	PPIStatus;
		
        // post the callback (parameters are device handle, ADI_PPI_EVENT_ERROR_INTERRUPT, NULL)
        (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_PPI_EVENT_ERROR_INTERRUPT, NULL);

        // indicate that we processed the interrupt
        Result = ADI_INT_RESULT_PROCESSED;

    // ENDIF
    }

    // return
    return(Result);
}


#if defined(__ADSP_BRAEMAR__)       // static functions for 534, 536, 537

/*********************************************************************

    Function:       ppiSetPortControl

    Description:    Configures the general purpose ports for Braemar
                    class devices upon opening and closing the device.

*********************************************************************/
static u32 ppiSetPortControl(   // configures the port control registers
    ADI_PPI *pDevice,               // pointer to the device
    u32     OpenFlag                // open flag (TRUE - open, FALSE - close)
){

    u32 Directives[] = {            // port control directives
        ADI_PORTS_DIR_PPI_BASE,         // base PPI functionality
        ADI_PORTS_DIR_PPI_FS3           // third frame sync
    };
    u32 Result;                     // return code
    u32 Count;                      // number of directives to pass to port control


    // assume success
    Result = ADI_DEV_RESULT_SUCCESS;

    // IF (opening the port)
    if (OpenFlag) {

        // IF (we're using 3 frame syncs)
        if (pDevice->TripleFrameSyncFlag) {

            // send both directives
            Count = 2;

        // ELSE
        } else {

            // send only the first directive
            Count = 1;

        // ENDIF
        }

        // call port control
        Result = adi_ports_EnablePPI(Directives, Count, TRUE);

    // ENDIF
    }

    // return
    return (Result);
}

#endif      // __ADSP_BRAEMAR__

/* IF (ADSP-BF52x family) */
#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)

/*********************************************************************

    Function: ppiSetPortControl

        Configures the general purpose ports of Kookaburra
        and Mockingbird class devices for PPI usage.

    Parameters:
        pDevice     - Pointer to PPI Device instance we're woring on
        OpenFlag    - 'TRUE' to open ports for PPI use

*********************************************************************/
static u32 ppiSetPortControl(
    ADI_PPI     *pDevice,
    u32         OpenFlag
)
{

    /* Number of directives to be passed */
    u32                 nDirectives = 0;

    /* Port configuration to use. */
    ADI_PORTS_DIRECTIVE *pPortCfg = NULL;

    /* Return code */
    u32                 eResult = ADI_DEV_RESULT_FAILED;

    /* Directives to enable PPI Clock and Data ports */
    ADI_PORTS_DIRECTIVE     aePpiClkDataDirectives [] =
    {
        ADI_PORTS_DIRECTIVE_PPICLK_TMRCLK,
        ADI_PORTS_DIRECTIVE_PPI_D0,
        ADI_PORTS_DIRECTIVE_PPI_D1,
        ADI_PORTS_DIRECTIVE_PPI_D2,
        ADI_PORTS_DIRECTIVE_PPI_D3,
        ADI_PORTS_DIRECTIVE_PPI_D4,
        ADI_PORTS_DIRECTIVE_PPI_D5,
        ADI_PORTS_DIRECTIVE_PPI_D6,
        ADI_PORTS_DIRECTIVE_PPI_D7,
        ADI_PORTS_DIRECTIVE_PPI_D8,
        ADI_PORTS_DIRECTIVE_PPI_D9,
        ADI_PORTS_DIRECTIVE_PPI_D10,
        ADI_PORTS_DIRECTIVE_PPI_D11,
        ADI_PORTS_DIRECTIVE_PPI_D12,
        ADI_PORTS_DIRECTIVE_PPI_D13,
        ADI_PORTS_DIRECTIVE_PPI_D14,
        ADI_PORTS_DIRECTIVE_PPI_D15
    };

    /* Directives to enable PPI Frame sync ports */
    ADI_PORTS_DIRECTIVE     aePpiFsDirectivesD [] =
    {
        ADI_PORTS_DIRECTIVE_PPI_FS2_MUX0,
        //ADI_PORTS_DIRECTIVE_PPI_FS2_MUX2,
        ADI_PORTS_DIRECTIVE_PPI_FS3
        };
    ADI_PORTS_DIRECTIVE     aePpiFsDirectives1 [] =
    {
        ADI_PORTS_DIRECTIVE_PPI_FS2_MUX0,
        ADI_PORTS_DIRECTIVE_PPI_FS2_MUX2
        };
    ADI_PORTS_DIRECTIVE     aePpiFsDirectives2 [] =
    {
        ADI_PORTS_DIRECTIVE_PPI_FS2_MUX2,
        ADI_PORTS_DIRECTIVE_PPI_FS3
        };
    ADI_PORTS_DIRECTIVE     aePpiFsDirectives3 [] =
    {
        ADI_PORTS_DIRECTIVE_PPI_FS2_MUX0,
        ADI_PORTS_DIRECTIVE_PPI_FS2_MUX2,
        ADI_PORTS_DIRECTIVE_PPI_FS3
        };

    /* List of pointers to each directive. To be used to select  */
    /* which directive to use via defined enum (adi_ppi.h)        */
    ADI_PORTS_DIRECTIVE     *papiFsDirectives [] = {aePpiFsDirectivesD,
                                                    aePpiFsDirectives1,
                                                    aePpiFsDirectives2,
                                                    aePpiFsDirectives3};

    /* PPI device needs Clock & Data pins 0 to 7 by default */
    nDirectives = 9;

    /* IF (PPI Data length is more than 8 bits) */
    if (pDevice->PPIControl->dlen > 0)
    {
        /* Enable rest of the data pins depending on PPI data length */
        /* PPI does not support 9-bit data length, so increase
           directive count by data length + 1 */
        nDirectives += (pDevice->PPIControl->dlen + 1);
    }

    /* Call port control to enable PPI Clock and data pins */
    eResult = adi_ports_Configure(aePpiClkDataDirectives, nDirectives);

    /* IF (Successfully enabled PPI Clock and data pins) */
    if (eResult == ADI_PORTS_RESULT_SUCCESS)
    {
        /* Set pointer to the port configuration to use. */
        switch ( pDevice->FrameSyncCFG )
        {
        case ADI_PPI_FRAME_SYNC_CFG_MUX0_MUX2:
            pPortCfg = papiFsDirectives[1];
            nDirectives = 2;
            break;
        case ADI_PPI_FRAME_SYNC_CFG_MUX2_FS3:
            pPortCfg = papiFsDirectives[2];
            nDirectives = 2;
            break;
        case ADI_PPI_FRAME_SYNC_CFG_FS_ALL:
            pPortCfg = papiFsDirectives[3];
            nDirectives = 3;
            break;
        case ADI_PPI_FRAME_SYNC_CFG_DEFAULT:
        default:
            nDirectives = 2;
            pPortCfg = papiFsDirectives[0];
        } /* End of switch */

        /* IF (PPI configured to use Frame syncs) */
        if (pDevice->PPIControl->xfr_type == 3)
        {
            /* assume no FS directives required to be sent */
            nDirectives = 0;

            /* IF (PPI configured in transmit mode) */
            if (pDevice->PPIControl->port_dir)
            {
                /* IF (Use 2 or 3 frame syncs) */
                if ((pDevice->PPIControl->port_cfg == 1) ||
                    (pDevice->PPIControl->port_cfg == 3))
                {
                    /* Enable FS2 & FS3 */
                    nDirectives = 2;
                }
            }
            /* ELSE (PPI configured in receive mode) */
            else
            {
                /* IF (Use 2 or 3 frame syncs) */
                if ((pDevice->PPIControl->port_cfg == 1) ||
                    (pDevice->PPIControl->port_cfg == 2))
                {
                    /* Enable FS2 & FS3 */
                    nDirectives = 2;
                }
            }

            /* Call port control to enable PPI FS pins */
            eResult = adi_ports_Configure(pPortCfg, nDirectives);
        }
    }

    /* return */
    return (eResult);
}

#endif


#if defined(__ADSP_BRODIE__)

/*********************************************************************

    Function: ppiSetPortControl

        Configures the general purpose ports of Brodie
        class devices for PPI usage.

    Parameters:
        pDevice     - Pointer to PPI Device instance we're woring on
        OpenFlag    - 'TRUE' to open ports for PPI use

*********************************************************************/
static u32 ppiSetPortControl(
    ADI_PPI     *pDevice,
    u32         OpenFlag
)
{

    /* Number of directives to be passed */
    u32 nDirectives;
    /* Return code */
    u32 eResult;

    /* Directives to enable PPI Clock and Data ports */
    ADI_PORTS_DIRECTIVE     aePpiClkDataDirectives [] =
    {
        ADI_PORTS_DIRECTIVE_PPI_CLK_MUX2,
        ADI_PORTS_DIRECTIVE_PPI_D0,
        ADI_PORTS_DIRECTIVE_PPI_D1,
        ADI_PORTS_DIRECTIVE_PPI_D2,
        ADI_PORTS_DIRECTIVE_PPI_D3,
        ADI_PORTS_DIRECTIVE_PPI_D4,
        ADI_PORTS_DIRECTIVE_PPI_D5,
        ADI_PORTS_DIRECTIVE_PPI_D6,
        ADI_PORTS_DIRECTIVE_PPI_D7,
        ADI_PORTS_DIRECTIVE_PPI_D8,
        ADI_PORTS_DIRECTIVE_PPI_D9,
        ADI_PORTS_DIRECTIVE_PPI_D10,
        ADI_PORTS_DIRECTIVE_PPI_D11,
        ADI_PORTS_DIRECTIVE_PPI_D12,
        ADI_PORTS_DIRECTIVE_PPI_D13,
        ADI_PORTS_DIRECTIVE_PPI_D14,
        ADI_PORTS_DIRECTIVE_PPI_D15
    };

     /* Directives to enable PPI Frame sync ports */
    ADI_PORTS_DIRECTIVE     aePpiFsDirectives [] =
    {
        ADI_PORTS_DIRECTIVE_PPI_FS1_MUX2,
        ADI_PORTS_DIRECTIVE_PPI_FS2_MUX2,
        ADI_PORTS_DIRECTIVE_PPI_FS3
    };

    /* PPI device needs Clock & Data pins 0 to 7 by default */
    nDirectives = 9;

    /* IF (PPI Data length is more than 8 bits) */
    if (pDevice->PPIControl->dlen > 0)
    {
        /* Enable rest of the data pins depending on PPI data length */
        /* PPI does not support 9-bit data length, so increase
           directive count by data length + 1 */
        nDirectives += (pDevice->PPIControl->dlen + 1);
    }

    /* Call port control to enable PPI Clock and data pins */
    eResult = adi_ports_Configure(aePpiClkDataDirectives, nDirectives);

    /* IF (Successfully enabled PPI Clock and data pins) */
    if (eResult == ADI_PORTS_RESULT_SUCCESS)
    {
        /* IF (PPI configured to use Frame syncs) */
        if (pDevice->PPIControl->xfr_type == 3)
        {
            /* assume no FS directives required to be sent */
            nDirectives = 0;

            /* IF (PPI configured in transmit mode) */
            if (pDevice->PPIControl->port_dir)
            {
                /* IF (Use 2 or 3 frame syncs) */
                if ((pDevice->PPIControl->port_cfg == 1) ||
                    (pDevice->PPIControl->port_cfg == 3))
                {
                    /* Enable FS2 & FS3 */
                    nDirectives = 2;
                }
            }
            /* ELSE (PPI configured in receive mode) */
            else
            {
                /* IF (Use 2 or 3 frame syncs) */
                if ((pDevice->PPIControl->port_cfg == 1) ||
                    (pDevice->PPIControl->port_cfg == 2))
                {
                    /* Enable FS2 & FS3 */
                    nDirectives = 2;
                }
            }

            /* Call port control to enable PPI FS pins */
            eResult = adi_ports_Configure(aePpiFsDirectives, nDirectives);
        }
    }

    /* return */
    return (eResult);
}

#endif

#if defined(__ADSP_MOY__)

/*********************************************************************

    Function: ppiSetPortControl

        Configures the general purpose ports of Moy class devices for PPI usage

    Parameters:
        pDevice     - Pointer to PPI Device instance we're woring on
        OpenFlag    - 'TRUE' to open ports for PPI use

*********************************************************************/
static u32 ppiSetPortControl(
    ADI_PPI     *pDevice,
    u32         OpenFlag
)
{

    /* Number of directives to be passed */
    u32 nDirectives;
    /* Return code */
    u32 eResult;

    /* Directives to enable PPI Clock and Data ports */
    ADI_PORTS_DIRECTIVE     aePpiClkDataDirectives [] =
    {
        ADI_PORTS_DIRECTIVE_PPICLK_TMRCLK,
        ADI_PORTS_DIRECTIVE_PPI_D0,
        ADI_PORTS_DIRECTIVE_PPI_D1,
        ADI_PORTS_DIRECTIVE_PPI_D2,
        ADI_PORTS_DIRECTIVE_PPI_D3,
        ADI_PORTS_DIRECTIVE_PPI_D4,
        ADI_PORTS_DIRECTIVE_PPI_D5,
        ADI_PORTS_DIRECTIVE_PPI_D6,
        ADI_PORTS_DIRECTIVE_PPI_D7,
        ADI_PORTS_DIRECTIVE_PPI_D8,
        ADI_PORTS_DIRECTIVE_PPI_D9,
        ADI_PORTS_DIRECTIVE_PPI_D10,
        ADI_PORTS_DIRECTIVE_PPI_D11,
        ADI_PORTS_DIRECTIVE_PPI_D12,
        ADI_PORTS_DIRECTIVE_PPI_D13,
        ADI_PORTS_DIRECTIVE_PPI_D14,
        ADI_PORTS_DIRECTIVE_PPI_D15
    };

     /* Directives to enable PPI Frame sync ports */
    ADI_PORTS_DIRECTIVE     aePpiFsDirectives [] =
    {
        ADI_PORTS_DIRECTIVE_PPI_FS1_TMR0,
        ADI_PORTS_DIRECTIVE_PPI_FS2_TMR1,
        ADI_PORTS_DIRECTIVE_PPI_FS3
    };

    /* PPI device needs Clock & Data pins 0 to 7 by default */
    nDirectives = 9;

    /* IF (PPI Data length is more than 8 bits) */
    if (pDevice->PPIControl->dlen > 0)
    {
        /* Enable rest of the data pins depending on PPI data length */
        /* PPI does not support 9-bit data length, so increase
           directive count by data length + 1 */
        nDirectives += (pDevice->PPIControl->dlen + 1);
    }

    /* Call port control to enable PPI Clock and data pins */
    eResult = adi_ports_Configure(aePpiClkDataDirectives, nDirectives);

    /* IF (Successfully enabled PPI Clock and data pins) */
    if (eResult == ADI_PORTS_RESULT_SUCCESS)
    {
        /* IF (PPI configured to use Frame syncs) */
        if (pDevice->PPIControl->xfr_type == 3)
        {
            /* assume no FS directives required to be sent */
            nDirectives = 0;

            /* IF (PPI configured in transmit mode) */
            if (pDevice->PPIControl->port_dir)
            {
                /* IF (Use 2 or 3 frame syncs) */
                if ((pDevice->PPIControl->port_cfg == 1) ||
                    (pDevice->PPIControl->port_cfg == 3))
                {
                    /* Enable FS2 & FS3 */
                    nDirectives = 2;
                }
            }
            /* ELSE (PPI configured in receive mode) */
            else
            {
                /* IF (Use 2 or 3 frame syncs) */
                if ((pDevice->PPIControl->port_cfg == 1) ||
                    (pDevice->PPIControl->port_cfg == 2))
                {
                    /* Enable FS2 & FS3 */
                    nDirectives = 2;
                }
            }

            /* Call port control to enable PPI FS pins */
            eResult = adi_ports_Configure(aePpiFsDirectives, nDirectives);
        }
    }

    return (eResult);
}

#endif /* __ADSP_MOY__ */

#if defined(__ADSP_DELTA__)

/*********************************************************************

    Function: ppiSetPortControl

        Configures the general purpose ports of Delta class devices for PPI usage

    Parameters:
        pDevice     - Pointer to PPI Device instance we're woring on
        OpenFlag    - 'TRUE' to open ports for PPI use

*********************************************************************/
static u32 ppiSetPortControl(
    ADI_PPI     *pDevice,
    u32         OpenFlag
)
{

    /* Number of directives to be passed */
    u32 nDirectives;
    /* Return code */
    u32 eResult;

    /* Directives to enable PPI Clock and Data ports */
    ADI_PORTS_DIRECTIVE     aePpiClkDataDirectives [] =
    {
        ADI_PORTS_DIRECTIVE_PPI_D0,
        ADI_PORTS_DIRECTIVE_PPI_D1,
        ADI_PORTS_DIRECTIVE_PPI_D2,
        ADI_PORTS_DIRECTIVE_PPI_D3,
        ADI_PORTS_DIRECTIVE_PPI_D4,
        ADI_PORTS_DIRECTIVE_PPI_D5,
        ADI_PORTS_DIRECTIVE_PPI_D6,
        ADI_PORTS_DIRECTIVE_PPI_D7,
        ADI_PORTS_DIRECTIVE_PPI_D8,
        ADI_PORTS_DIRECTIVE_PPI_D9,
        ADI_PORTS_DIRECTIVE_PPI_D10,
        ADI_PORTS_DIRECTIVE_PPI_D11,
        ADI_PORTS_DIRECTIVE_PPI_D12,
        ADI_PORTS_DIRECTIVE_PPI_D13,
        ADI_PORTS_DIRECTIVE_PPI_D14,
        ADI_PORTS_DIRECTIVE_PPI_D15
    };

     /* Directives to enable PPI Frame sync ports */
    ADI_PORTS_DIRECTIVE     aePpiFsDirectives [] =
    {
        ADI_PORTS_DIRECTIVE_PPI_FS1_TMR0,
        ADI_PORTS_DIRECTIVE_PPI_FS2_TMR1,
        ADI_PORTS_DIRECTIVE_PPI_FS3
    };

    /* PPI device needs Clock & Data pins 0 to 7 by default */
    nDirectives = 9;

    /* IF (PPI Data length is more than 8 bits) */
    if (pDevice->PPIControl->dlen > 0)
    {
        /* Enable rest of the data pins depending on PPI data length */
        /* PPI does not support 9-bit data length, so increase
           directive count by data length + 1 */
        nDirectives += (pDevice->PPIControl->dlen + 1);
    }

    /* Call port control to enable PPI Clock and data pins */
    eResult = adi_ports_Configure(aePpiClkDataDirectives, nDirectives);

    /* IF (Successfully enabled PPI Clock and data pins) */
    if (eResult == ADI_PORTS_RESULT_SUCCESS)
    {
        /* IF (PPI configured to use Frame syncs) */
        if (pDevice->PPIControl->xfr_type == 3)
        {
            /* assume no FS directives required to be sent */
            nDirectives = 0;

            /* IF (PPI configured in transmit mode) */
            if (pDevice->PPIControl->port_dir)
            {
                /* IF (Use 2 or 3 frame syncs) */
                if ((pDevice->PPIControl->port_cfg == 1) ||
                    (pDevice->PPIControl->port_cfg == 3))
                {
                    /* Enable FS2 & FS3 */
                    nDirectives = 2;
                }
            }
            /* ELSE (PPI configured in receive mode) */
            else
            {
                /* IF (Use 2 or 3 frame syncs) */
                if ((pDevice->PPIControl->port_cfg == 1) ||
                    (pDevice->PPIControl->port_cfg == 2))
                {
                    /* Enable FS2 & FS3 */
                    nDirectives = 2;
                }
            }

            /* Call port control to enable PPI FS pins */
            eResult = adi_ports_Configure(aePpiFsDirectives, nDirectives);
        }
    }

    return (eResult);
}

#endif /* __ADSP_DELTA__ */

/*********************************************************************
*
*   Function:       ppiSetFrameSyncTimer
*
*   Description:    Configures frame sync timers
*
*********************************************************************/

static u32 ppiSetFrameSyncTimer(u32 TimerID, u16 *pTimerOpenFlag, ADI_PPI_FS_TMR *config)
{

    static ADI_TMR_GP_CMD_VALUE_PAIR Configuration[] = {
        { ADI_TMR_GP_CMD_SET_OUTPUT_PAD_DISABLE,(void *)TRUE        }, // disable timer output pad to avoid glitches
        { ADI_TMR_GP_CMD_SET_TIMER_MODE,        (void *)1           }, // pwm mode
        { ADI_TMR_GP_CMD_SET_COUNT_METHOD,      (void *)TRUE        }, // count to end of period
        { ADI_TMR_GP_CMD_SET_INPUT_SELECT,      (void *)TRUE        }, // input from PPI clock
        { ADI_TMR_GP_CMD_SET_CLOCK_SELECT,      (void *)TRUE        }, // use PWM clock
        { ADI_TMR_GP_CMD_END,                   NULL                },
    };
    u32 TimerResult;
    u32 Result;

    // open the timer if necessary
    if (!*pTimerOpenFlag) {
        if (adi_tmr_Open(TimerID) != ADI_TMR_RESULT_SUCCESS) {
            return (ADI_PPI_RESULT_TIMER_ERROR);
        }
    }
    *pTimerOpenFlag = TRUE;

    // configure the timer
    TimerResult  = adi_tmr_GPControl(TimerID, ADI_TMR_GP_CMD_TABLE,                 Configuration);
    TimerResult |= adi_tmr_GPControl(TimerID, ADI_TMR_GP_CMD_SET_PULSE_HI,          (void *)config->pulse_hi);
    TimerResult |= adi_tmr_GPControl(TimerID, ADI_TMR_GP_CMD_RUN_DURING_EMULATION,  (void *)config->emu_run);
    TimerResult |= adi_tmr_GPControl(TimerID, ADI_TMR_GP_CMD_SET_PERIOD,            (void *)config->period);
    TimerResult |= adi_tmr_GPControl(TimerID, ADI_TMR_GP_CMD_SET_WIDTH,             (void *)config->width);
    TimerResult |= adi_tmr_GPControl(TimerID, ADI_TMR_GP_CMD_SET_ENABLE_DELAY,      (void *)config->enable_delay);

    // update the return value
    if (TimerResult) {
        Result = ADI_PPI_RESULT_TIMER_ERROR;
    } else {
        Result = ADI_DEV_RESULT_SUCCESS;
    }

    // return
    return (Result);
}



#if defined(ADI_DEV_DEBUG)

/*********************************************************************

    Function:       ValidatePDDHandle

    Description:    Validates a PDD handle

*********************************************************************/

static int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle) {
    unsigned int i;
    for (i = 0; i < ADI_PPI_NUM_DEVICES; i++) {
        if (PDDHandle == (ADI_DEV_PDD_HANDLE)&Device[i]) {
            return (ADI_DEV_RESULT_SUCCESS);
        }
    }
    return (ADI_DEV_RESULT_BAD_PDD_HANDLE);
    }


#endif



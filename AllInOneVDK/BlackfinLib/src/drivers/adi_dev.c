/**********************************************************************

Copyright(c) Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_dev.c,v $
$Revision: 5506 $
$Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $

Description:
        This is the main source file for the device manager.

        Fields within the DMA config register are set at various points
        in the process of controlling a device.  The table below shows
        where these bits are controlled for each dataflow method:

                    Circular                  Chained/Seq Chained
        flow    DEV_SET_DATAFLOW_METHOD     DEV_SET_DATAFLOW_METHOD
        ndsize  DEV_SET_DATAFLOW_METHOD     DEV_SET_DATAFLOW_METHOD
        di_en   devPrepareBuffer            devPrepareBuffer
        di_sel  devPrepareBuffer            DEV_SET_DATAFLOW_METHOD
        restart devOpen                     devOpen
        dma2d   DEV_SET_DATAFLOW_METHOD     devPrepareBuffer
        wdsize  devPrepareBuffer            devPrepareBuffer
        wnr     devPrepareBuffer            devPrepareBuffer
        dma_en  devOpen                     devOpen


*********************************************************************/

/*********************************************************************

Include files

*********************************************************************/

#include <services/services.h>      /* system service includes */
#include <drivers/adi_dev.h>        /* device manager includes */

/*********************************************************************

Enumerations and defines

*********************************************************************/

/* DEBUG */
#define ADI_DEV_LARGE               /* Build for large descriptors */
//#define ADI_DEV_SMALL             /* Build for small descriptors */

/*********************************************************************

Data Structures

*********************************************************************/


typedef struct ADI_DEV_DEVICE {                 /* Device structure                                             */
    ADI_DEV_PDD_ENTRY_POINT *pEntryPoint;       /* Entry point                                                  */
    struct ADI_DEV_MANAGER  *pManager;          /* device manager this device belongs to                        */
    ADI_DEV_PDD_HANDLE      PDDHandle;          /* PDD handle                                                   */
    ADI_DEV_MODE            DataflowMethod;     /* Dataflow method                                              */
    ADI_DEV_DIRECTION       Direction;          /* Direction                                                    */
    u8                      InUseFlag;          /* Open flag                                                    */
    u8                      DataflowFlag;       /* dataflow flag                                                */
    u8                      SynchronousFlag;    /* synchronous flag                                             */
    u8                      StreamingFlag;      /* streaming flag                                               */
    u32                     PeripheralDMAFlag;  /* peripheral dma support flag                                  */
    ADI_DMA_CONFIG_REG      Config;             /* value for DMA config register                                */
    ADI_DMA_MANAGER_HANDLE  DMAHandle;          /* DMA manager handle                                           */
    ADI_DCB_HANDLE          DCBHandle;          /* handle to the callback manager for this device               */
    ADI_DEV_DMA_INFO        InboundDma;         /* to hold Inbound DMA channel info (for backward compatiblity) */
    ADI_DEV_DMA_INFO        OutboundDma;        /* to hold Outbound DMA channel info (for backward compatiblity)*/
    ADI_DEV_DMA_INFO        *pInboundDma;       /* pointer to Inbound DMA channel info                          */
    ADI_DEV_DMA_INFO        *pOutboundDma;      /* pointer to Outbound DMA channel info                         */
    void                    *ClientHandle;      /* Client handle                                                */
    ADI_DCB_CALLBACK_FN     ClientCallback;     /* client callback                                              */
} ADI_DEV_DEVICE;

typedef struct ADI_DEV_MANAGER {                /* Device manager instance  data                                */
    u32             DeviceCount;                /* number of devices in array                                   */
    void            *pEnterCriticalArg;         /* enter critical region parameter                              */
    ADI_DEV_DEVICE  *Device;                    /* pointer to an array of device entries                        */
} ADI_DEV_MANAGER;

/*********************************************************************

Static data

*********************************************************************/


const static ADI_DEV_DEVICE InitialDeviceSettings = {   /* initial device settings                              */
    NULL,                                           /* Entry point                                              */
    NULL,                                           /* device manager                                           */
    NULL,                                           /* PDD handle                                               */
    ADI_DEV_MODE_UNDEFINED,                         /* Dataflow  method                                         */
    ADI_DEV_DIRECTION_UNDEFINED,                    /* Direction                                                */
    TRUE,                                           /* Open flag                                                */
    FALSE,                                          /* dataflow flag                                            */
    FALSE,                                          /* synchronous flag                                         */
    FALSE,                                          /* streaming flag                                           */
    FALSE,                                          /* peripheral dma support flag                              */
    {                                               /* value for DMA config register                            */
        ADI_DMA_EN_ENABLE, ADI_DMA_WNR_READ, ADI_DMA_WDSIZE_8BIT,
        ADI_DMA_DMA2D_LINEAR, ADI_DMA_RESTART_RETAIN,
        ADI_DMA_DI_SEL_OUTER_LOOP, ADI_DMA_DI_EN_DISABLE,
        ADI_DMA_NDSIZE_STOP, ADI_DMA_FLOW_STOP
    },
    NULL,                                           /* handle to the DMA manager                                */
    NULL,                                           /* handle to the callback manager                           */
    {
        ADI_DMA_PMAP_UNDEFINED,                     /* DMA peripheral mapping ID                                */
        NULL,                                       /* handle to this DMA channel                               */
        FALSE,                                      /* switch mode disabled by default                          */
        NULL,                                       /* pointer to switch queue head                             */
        NULL,                                       /* pointer to switch queue tail                             */
        NULL,                                       /* pointer to structure holding next DMA channel info       */
    },
    {
        ADI_DMA_PMAP_UNDEFINED,                     /* DMA peripheral mapping ID                                */
        NULL,                                       /* handle to this DMA channel                               */
        FALSE,                                      /* switch mode disabled by default                          */
        NULL,                                       /* pointer to switch queue head                             */
        NULL,                                       /* pointer to switch queue tail                             */
        NULL,                                       /* pointer to structure holding next DMA channel info       */
    },
    NULL,                                           /* pointer to Inbound DMA channel info table                */
    NULL,                                           /* pointer to Outbound DMA channel info table               */
    NULL,                                           /* Client handle                                            */
    NULL,                                           /* client callback                                          */
};

/*********************************************************************

macro to properly set the data pointer in a descriptor

*********************************************************************/
/* IF (Built for large descriptors) */
#if defined(ADI_DEV_LARGE)
#define ADI_DEV_SET_DESCRIPTOR_START_ADDRESS(pBuffer,pData) \
    ((ADI_DMA_DESCRIPTOR_LARGE *)pBuffer)->StartAddress = pData
#endif
/* IF (Built for Small descriptors) */
#if defined(ADI_DEV_SMALL)
#define ADI_DEV_SET_DESCRIPTOR_START_ADDRESS(pBuffer,pData) \
    ((ADI_DMA_DESCRIPTOR_SMALL *)pBuffer)->StartAddressLow  = ((u32)(pData)) & 0xffff;   \
    ((ADI_DMA_DESCRIPTOR_SMALL *)pBuffer)->StartAddressHigh = ((u32)(pData) & 0xffff0000) >> 16
#endif

/*********************************************************************

Static functions

*********************************************************************/

/* callback function for PDD events */
static void PDDCallback(
    ADI_DEV_DEVICE_HANDLE       DeviceHandle,   /* Handle to the device that generated callback         */
    u32                         Event,          /* Callback event                                       */
    void                        *pArg           /* Callback argument                                    */
);

/* callback function for DMA events */
static void DMACallback(
    void                        *Ptr1,          /* Handle to the device that generated callback         */
    u32                         Value1,         /* DMA Callback event                                   */
    void                        *pArg           /* Callback argument                                    */
);

/* routine to control dataflow */
static u32 SetDataflow (
    ADI_DEV_DEVICE              *pDevice,       /* pointer to the device we're working on               */
    u32                         Flag            /* Dataflow flag (TRUE to enable dataflow)              */
);

/* Routine to prepare data buffers */
static u32 PrepareBufferList(
    ADI_DEV_DEVICE              *pDevice,       /* pointer to the device we're working on               */
    ADI_DEV_BUFFER_TYPE         BufferType,     /* type of Buffer                                       */
    ADI_DEV_BUFFER              *pBuffer,       /* pointer to the start of buffer chain                 */
    ADI_DEV_DIRECTION           Direction,      /* Receive/Transmit data                                */
    ADI_DEV_BUFFER              **pLastBuffer   /* pointer to the last buffer in the given buffer chain */
);

/* routine to process data buffers */
static u32 ProcessBuffer(
    ADI_DEV_DEVICE              *pDevice,       /* Pointer to the device we're working on               */
    ADI_DEV_BUFFER_TYPE         BufferType,     /* type of Buffer                                       */
    ADI_DEV_BUFFER              *pBuffer,       /* pointer to buffer                                    */
    ADI_DEV_DIRECTION           Direction       /* data direction                                       */
);

/* Debug Routines - Debug Build only */
#if defined(ADI_DEV_DEBUG)

/* Validates Device Handle */
static u32 ValidateDeviceHandle(
    ADI_DEV_DEVICE_HANDLE       Handle          /* Device handle to vaildate                            */
);
/* Validates Device manager handle */
static u32 ValidateManagerHandle(
    ADI_DEV_MANAGER_HANDLE      Handle          /* Device Manager handle to validate                    */
);

#endif  /* end of Debug routines - Debug build only */

/*********************************************************************

    Function:       adi_dev_Init

    Description:    Provides memory for the device manager to use.  This
                    function initializes the memory to support 'n' number
                    of devices.

*********************************************************************/

u32 adi_dev_Init(                               /* Initializes the DMA Manager */
    void                    *pMemory,           /* pointer to memory */
    const size_t            MemorySize,         /* size of memory (in bytes) */
    u32                     *pMaxDevices,       /* address where the device manager will store the number of devices that can be supported */
    ADI_DEV_MANAGER_HANDLE  *pManagerHandle,    /* address where the device manager will store the manager handle */
    void                    *pEnterCriticalArg  /* critical region imask storage location */
){

    ADI_DEV_MANAGER *pManager;      /* pointer to the DMA Manager data */
    ADI_DEV_DEVICE  *pDevice;       /* pointer to a device structure */
    u32             i;              /* counter */
    u32             Result;         /* return code */

    /* assume we're going to be successful */
    Result = ADI_DEV_RESULT_SUCCESS;
    /* insure we have enough memory for the device manager itself */
    pManager = pMemory;
    if (MemorySize < sizeof(ADI_DEV_MANAGER))
    {
        Result = ADI_DEV_RESULT_NO_MEMORY;
    }
    else
    {
        *pManagerHandle = (ADI_DEV_MANAGER_HANDLE *)pManager;

        /* determine how many devices we can support and notify the application of such */
        pManager->DeviceCount = (MemorySize - sizeof(ADI_DEV_MANAGER))/(sizeof(ADI_DEV_DEVICE));
        *pMaxDevices = pManager->DeviceCount;
        /* IF (the given memory not sufficient enough to hold a device) */
        if (pManager->DeviceCount == 0)
        {
            Result = ADI_DEV_RESULT_NO_MEMORY;
        }
        else
        {
            /* allocate memory for this device */
            pManager->Device = (ADI_DEV_DEVICE *)(pManager + 1);
            /* save the parameter for critical region protection */
            pManager->pEnterCriticalArg = pEnterCriticalArg;
            /* initialize each device */
            for (i = pManager->DeviceCount, pDevice = pManager->Device; i; i--, pDevice++)
            {
                pDevice->InUseFlag = FALSE;
            }
        }
    }

    /* return */
    return (Result);
}

/*********************************************************************
*
*   Function:       adi_dev_Terminate
*
*   Description:    Closes all devices and shuts down the device manager
*
*********************************************************************/
u32 adi_dev_Terminate(                      /* terminates the device manager */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle   /* device manager handle */
){

    u32                 i;                  /* general purpose index */
    u32                 Result;             /* return value */
    ADI_DEV_DEVICE      *pDevice;           /* pointer to the device we're working on */
    ADI_DEV_MANAGER     *pManager;          /* pointer to the device manager */

    /* assume we're going to be successful */
    Result = ADI_DEV_RESULT_SUCCESS;

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)
    /* continue only if the given Device manager handle is valid */
    if ((Result = ValidateManagerHandle(ManagerHandle)) == ADI_DEV_RESULT_SUCCESS)
    {
#endif

    /* avoid casts */
    pManager = (ADI_DEV_MANAGER *)ManagerHandle;

    /* close all open devices */
    for (i = pManager->DeviceCount, pDevice = pManager->Device; i; i--, pDevice++)
    {
        if (pDevice->InUseFlag == TRUE)
        {
            if ((Result = adi_dev_Close(pDevice)) != ADI_DEV_RESULT_SUCCESS)
            {
                break;
            }
        }
    }

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_dev_Open
*
*   Description:    Opens a device driver for use
*
*********************************************************************/
u32 adi_dev_Open(                           /* Open a device                    */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,  /* device manager handle            */
    ADI_DEV_PDD_ENTRY_POINT *pEntryPoint,   /* PDD entry point                  */
    u32                     DevNumber,      /* device number                    */
    void                    *ClientHandle,  /* client handle                    */
    ADI_DEV_DEVICE_HANDLE   *pDeviceHandle, /* pointer to Device handle location*/
    ADI_DEV_DIRECTION       Direction,      /* data direction                   */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,      /* handle to the DMA manager        */
    ADI_DCB_HANDLE          DCBHandle,      /* handle to the callback manager   */
    ADI_DCB_CALLBACK_FN     ClientCallback  /* client callback function         */
){

    u32                 i;                  /* general purpose index */
    u32                 Result;             /* return value */
    ADI_DEV_DEVICE      *pDevice;           /* pointer to the device we're working on */
    ADI_DEV_MANAGER     *pManager;          /* pointer to the device manager */
    void                *pExitCriticalArg;  /* parameter for exit critical region */
    ADI_DEV_DMA_INFO    *pDmaInfo;          /* pointer to DMA channel information table of the device we're working on */

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
    /* Validate Device Manager Handle */
    Result = ValidateManagerHandle(ManagerHandle);
    /* IF (no callback function supplied) */
    if (ClientCallback == NULL)
    {
        Result = ADI_DEV_RESULT_NO_CALLBACK_FUNCTION_SUPPLIED;
    }
    /* continue only on success */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif

    /* avoid casts */
    pManager = (ADI_DEV_MANAGER *)ManagerHandle;

    /* find a free device */
    pExitCriticalArg = adi_int_EnterCriticalRegion(pManager->pEnterCriticalArg);
    for (i = pManager->DeviceCount, pDevice = pManager->Device; i; i--, pDevice++)
    {
        if (pDevice->InUseFlag == FALSE)
        {
            pDevice->InUseFlag = TRUE;
            break;
        }
    }
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    /* IF (no sufficient memory for new device */
    if (i == 0)
    {
        Result = ADI_DEV_RESULT_NO_MEMORY;
    }
    else
    {
        /* save the Device handle in the application provided location */
        *pDeviceHandle = (ADI_DEV_DEVICE_HANDLE *)pDevice;

        /* initialize the device settings */
        *pDevice = InitialDeviceSettings;
        pDevice->pManager = pManager;

        /* populate the device structure with what we know about the device and the client */
        pDevice->pEntryPoint    = (ADI_DEV_PDD_ENTRY_POINT *)pEntryPoint;
        pDevice->ClientHandle   = ClientHandle;
        pDevice->Direction      = Direction;
        pDevice->DMAHandle      = DMAHandle;
        pDevice->DCBHandle      = DCBHandle;
        pDevice->ClientCallback = ClientCallback;

        /* open the physical device */
        /* NOTE: We now always pass NULL for the DCB handle as we've centralized the check for DCB callbacks in PDDCallback
                 This saves the physical drivers from having to check for deferred handles all the time. */
        Result = (pDevice->pEntryPoint->adi_pdd_Open) ( ManagerHandle,
                                                        DevNumber,
                                                        (ADI_DEV_DEVICE_HANDLE *)pDevice,
                                                        &(pDevice->PDDHandle),
                                                        Direction,
                                                        pDevice->pManager->pEnterCriticalArg,
                                                        pDevice->DMAHandle,
                                                        NULL,
                                                        PDDCallback);
        /* IF (Failed to open the physical device) */
        if (Result != ADI_DEV_RESULT_SUCCESS)
        {
            pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pManager->pEnterCriticalArg);
            pDevice->InUseFlag = FALSE;
            adi_int_ExitCriticalRegion(pExitCriticalArg);
        }
        else
        {
            /* determine if the device is supported by peripheral DMA */
            Result = (pDevice->pEntryPoint->adi_pdd_Control) (pDevice->PDDHandle, ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT, &pDevice->PeripheralDMAFlag);

            /* IF (the selected Device supports DMA) */
            if ((Result == ADI_DEV_RESULT_SUCCESS) && (pDevice->PeripheralDMAFlag == TRUE))
            {
                /* Get the Device Inbound DMA channel information */
                Result = (pDevice->pEntryPoint->adi_pdd_Control) (pDevice->PDDHandle, ADI_DEV_CMD_GET_INBOUND_DMA_INFO, &pDevice->pInboundDma);
                /* is this command is supported by this device? */
                if ((Result == ADI_DEV_RESULT_NOT_SUPPORTED) || (Result == ADI_DEV_RESULT_CMD_NOT_SUPPORTED))
                {
                    /* if not, create a DMA Inbound table for this device */
                    pDevice->pInboundDma = &pDevice->InboundDma;
                    /* query for Inbound DMA Mapping ID */
                    Result = (pDevice->pEntryPoint->adi_pdd_Control) (pDevice->PDDHandle, ADI_DEV_CMD_GET_INBOUND_DMA_PMAP_ID, &pDevice->pInboundDma->MappingID);
                }

                /* Initialise the fields in Inbound DMA channel info table */
                if(Result == ADI_DEV_RESULT_SUCCESS)
                {
                    /* Do it for all Inbound DMA channels listed by the device */
                    for (pDmaInfo=pDevice->pInboundDma;pDmaInfo; pDmaInfo = pDmaInfo->pNext )
                    {
                        pDmaInfo->ChannelHandle     = NULL;     /* clear DMA channel handle             */
                        pDmaInfo->SwitchModeFlag    = FALSE;    /* Switch mode disabled by default      */
                        pDmaInfo->pSwitchHead       = NULL;     /* clear pointer to switch queue head   */
                        pDmaInfo->pSwitchTail       = NULL;     /* clear pointer to switch queue tail   */
                    }
                }
                else
                {
                    /* This device does not have any Inbound DMA channel(s) */
                    pDevice->pInboundDma = NULL;
                }

                /* Get the Device Outbound DMA channel information */
                Result = (pDevice->pEntryPoint->adi_pdd_Control) (pDevice->PDDHandle, ADI_DEV_CMD_GET_OUTBOUND_DMA_INFO, &pDevice->pOutboundDma);
                /* is this command is supported by this device? */
                if ((Result == ADI_DEV_RESULT_NOT_SUPPORTED) || (Result == ADI_DEV_RESULT_CMD_NOT_SUPPORTED))
                {
                    /* if not, create a DMA Outbound table for this device */
                    pDevice->pOutboundDma = &pDevice->OutboundDma;
                    /* query for Outbound DMA Mapping ID */
                    Result = (pDevice->pEntryPoint->adi_pdd_Control) (pDevice->PDDHandle, ADI_DEV_CMD_GET_OUTBOUND_DMA_PMAP_ID, &pDevice->pOutboundDma->MappingID);
                }

                /* Initialise the fields in Outbound DMA channel info table */
                if(Result == ADI_DEV_RESULT_SUCCESS)
                {
                    /* Do it for all Outbound DMA channels listed by the device */
                    for (pDmaInfo=pDevice->pOutboundDma;pDmaInfo; pDmaInfo = pDmaInfo->pNext )
                    {
                        pDmaInfo->ChannelHandle     = NULL;     /* clear DMA channel handle             */
                        pDmaInfo->SwitchModeFlag    = FALSE;    /* Switch mode disabled by default      */
                        pDmaInfo->pSwitchHead       = NULL;     /* clear pointer to switch queue head   */
                        pDmaInfo->pSwitchTail       = NULL;     /* clear pointer to switch queue tail   */
                    }
                }
                else
                {
                    /* This device does not have any Outbound DMA channel(s) */
                    pDevice->pOutboundDma = NULL;
                }
                Result = ADI_DEV_RESULT_SUCCESS;
            }
        }
    }

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return (Result);
}

/*********************************************************************
*
*   Function:       adi_dev_Close
*
*   Description:    Closes down a device driver
*
*********************************************************************/
u32 adi_dev_Close(                              /* Closes a device                          */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle        /* Device handle                            */
){
    u32                     i;                  /* general purpose index                    */
    u32                     Result;             /* return value                             */
    ADI_DEV_DEVICE          *pDevice;           /* pointer to the device we're working on   */
    void                    *pExitCriticalArg;  /* parameter for exit critical              */
    ADI_DEV_DMA_INFO        *pDmaInfo;          /* pointer to DMA channel information table of the device we're working on */

    /* avoid casts */
    pDevice = (ADI_DEV_DEVICE *)DeviceHandle;

    /* assume we're going to be successfull */
    Result = ADI_DEV_RESULT_SUCCESS;

    /* Valiate the given device handle */
#if defined(ADI_DEV_DEBUG)
    Result = ValidateDeviceHandle(DeviceHandle);
#endif

    /* IF (no errors) */
    if (Result == ADI_DEV_RESULT_SUCCESS) {

        /* IF (we can shutdown dataflow) */
        if ((Result = adi_dev_Control(pDevice, ADI_DEV_CMD_SET_DATAFLOW, (void *)FALSE)) == ADI_DEV_RESULT_SUCCESS) {

            /* IF (this device is using peripheral DMA) */
            if (pDevice->PeripheralDMAFlag == TRUE) {

                /* close all outbound DMA channels used by this device */
                for (pDmaInfo=pDevice->pOutboundDma; pDmaInfo; pDmaInfo = pDmaInfo->pNext) {
                    if (pDmaInfo->ChannelHandle) {
                        if ((Result = adi_dma_Close(pDmaInfo->ChannelHandle, FALSE)) != ADI_DMA_RESULT_SUCCESS) {
                            break;
                        }
                        pDmaInfo->ChannelHandle = NULL;
                    }
                }

                /* IF (no errors yet) */
                if (Result == ADI_DEV_RESULT_SUCCESS) {

                    /* close all Inbound DMA channels used by this device */
                    for (pDmaInfo=pDevice->pInboundDma; pDmaInfo; pDmaInfo = pDmaInfo->pNext) {
                        if (pDmaInfo->ChannelHandle) {
                            if ((Result = adi_dma_Close(pDmaInfo->ChannelHandle, FALSE)) != ADI_DMA_RESULT_SUCCESS) {
                                break;
                            }
                            pDmaInfo->ChannelHandle = NULL;
                        }
                    }

                /* ENDIF */
                }

            /* ENDIF */
            }

            /* IF (no errors yet) */
            if (Result == ADI_DEV_RESULT_SUCCESS) {

                /* IF (we can successfully close the physical device) */
                if ((Result = (pDevice->pEntryPoint->adi_pdd_Close) (pDevice->PDDHandle)) == ADI_DEV_RESULT_SUCCESS) {

                    /* free the device */
                    pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pManager->pEnterCriticalArg);
                    pDevice->InUseFlag = FALSE;
                    adi_int_ExitCriticalRegion(pExitCriticalArg);

                /* ENDIF */
                }

            /* ENDIF */
            }

        /* ENDIF */
        }

    /* ENDIF */
    }

    /* return */
    return (Result);
}



/*********************************************************************
*
*   Function:       adi_dev_Read
*
*   Description:    Provides buffers to which inbound data is stored
*
*********************************************************************/

u32 adi_dev_Read(                           /* Reads data or queues an inbound buffer to a device   */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,   /* Device handle                                        */
    ADI_DEV_BUFFER_TYPE     BufferType,     /* buffer type                                          */
    ADI_DEV_BUFFER          *pBuffer        /* pointer to buffer                                    */
){

    u32                     Result;         /* return value */
    ADI_DEV_DEVICE          *pDevice;       /* pointer to the device we're working on */

    /* avoid casts */
    pDevice = (ADI_DEV_DEVICE *)DeviceHandle;
    /* assume we're going to be successful */
    Result = ADI_DEV_RESULT_SUCCESS;

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
    /* Validate Device Handle */
    if ((Result = ValidateDeviceHandle(DeviceHandle)) == ADI_DEV_RESULT_SUCCESS)
    {
        /* Verify Direction */
        if (pDevice->Direction == ADI_DEV_DIRECTION_OUTBOUND)
        {
            Result = ADI_DEV_RESULT_ATTEMPTED_READ_ON_OUTBOUND_DEVICE;
        }
        /* verify dataflow method established */
        if (pDevice->DataflowMethod == ADI_DEV_MODE_UNDEFINED)
        {
            Result = ADI_DEV_RESULT_DATAFLOW_UNDEFINED;
        }
        /* can't use adi_dev_Read with sequential I/O */
        if ((pDevice->DataflowMethod == ADI_DEV_MODE_SEQ_CHAINED) ||
            (pDevice->DataflowMethod == ADI_DEV_MODE_SEQ_CHAINED_LOOPBACK))
        {
            Result = ADI_DEV_RESULT_DATAFLOW_INCOMPATIBLE;
        }
        /* don't allow if synchronous mode and dataflow is not enabled */
        if ((pDevice->SynchronousFlag == TRUE) && (pDevice->DataflowFlag != TRUE))
        {
            Result = ADI_DEV_RESULT_DATAFLOW_NOT_ENABLED;
        }
        /* Validate DMA channel information table for DMA driven devices */
        if ((pDevice->PeripheralDMAFlag == TRUE) && (pDevice->pInboundDma == NULL))
        {
            Result = ADI_DEV_RESULT_DMA_CHANNEL_UNAVAILABLE;
        }
    }

    /* Continue only on success */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif

    /* process the given buffers */
    Result = ProcessBuffer(pDevice, BufferType, pBuffer, ADI_DEV_DIRECTION_INBOUND);

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return (Result);
}

/*********************************************************************
*
*   Function:       adi_dev_Write
*
*   Description:    Provides buffers from which outbound data is transmitted
*
*********************************************************************/
u32 adi_dev_Write(                              /* Writes data or queues an outbound buffer to a device */
    ADI_DEV_DEVICE_HANDLE       DeviceHandle,   /* Device handle                                        */
    ADI_DEV_BUFFER_TYPE         BufferType,     /* buffer type                                          */
    ADI_DEV_BUFFER              *pBuffer        /* pointer to buffer                                    */
){
    u32                         Result;         /* return value */
    ADI_DEV_DEVICE              *pDevice;       /* pointer to the device we're working on */

    /* avoid casts */
    pDevice     = (ADI_DEV_DEVICE *)DeviceHandle;
    /* assume we're going to be successful */
    Result = ADI_DEV_RESULT_SUCCESS;

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
    /* Validate Device Handle */
    if ((Result = ValidateDeviceHandle(DeviceHandle)) == ADI_DEV_RESULT_SUCCESS)
    {
        /* Verify Direction */
        if (pDevice->Direction == ADI_DEV_DIRECTION_INBOUND)
        {
            Result = ADI_DEV_RESULT_ATTEMPTED_WRITE_ON_INBOUND_DEVICE;
        }
        /* verify dataflow method established */
        if (pDevice->DataflowMethod == ADI_DEV_MODE_UNDEFINED)
        {
            Result = ADI_DEV_RESULT_DATAFLOW_UNDEFINED;
        }
        /* can't use adi_dev_Read with sequential I/O */
        if ((pDevice->DataflowMethod == ADI_DEV_MODE_SEQ_CHAINED) ||
            (pDevice->DataflowMethod == ADI_DEV_MODE_SEQ_CHAINED_LOOPBACK))
        {
            Result = ADI_DEV_RESULT_DATAFLOW_INCOMPATIBLE;
        }
        /* don't allow if synchronous mode and dataflow is not enabled */
        if ((pDevice->SynchronousFlag == TRUE) && (pDevice->DataflowFlag != TRUE))
        {
            Result = ADI_DEV_RESULT_DATAFLOW_NOT_ENABLED;
        }
        /* Validate DMA channel information table for DMA driven devices */
        if ((pDevice->PeripheralDMAFlag == TRUE) && (pDevice->pOutboundDma == NULL))
        {
            Result = ADI_DEV_RESULT_DMA_CHANNEL_UNAVAILABLE;
        }
    }

    /* Continue only on success */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif

    /* process the given buffers */
    Result = ProcessBuffer(pDevice, BufferType, pBuffer, ADI_DEV_DIRECTION_OUTBOUND);

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return (Result);
}

/*********************************************************************
*
*   Function:       ProcessBuffer
*
*   Description:    Processes Inbound/Outbound buffers
*
*********************************************************************/
static u32 ProcessBuffer(
    ADI_DEV_DEVICE              *pDevice,       /* Pointer to the device we're working on */
    ADI_DEV_BUFFER_TYPE         BufferType,     /* buffer type */
    ADI_DEV_BUFFER              *pBuffer,       /* pointer to buffer */
    ADI_DEV_DIRECTION           Direction       /* data direction */
){

    ADI_DEV_BUFFER_PAIR         *pBufPair;          /* pointer to a buffer pair */
    ADI_DEV_BUFFER              *pShadowBuffer;     /* temp pointer to buffer to be queued */
    ADI_DEV_BUFFER              *pLastBuffer;       /* pointer to the last buffer in the list past in */
    ADI_DEV_BUFFER_TYPE         ShadowBufferType;   /* temp location holding the actual buffer type to be queued */
    ADI_DEV_DMA_INFO            *pDmaInfo = NULL;   /* pointer to DMA channel information table of the device we're working on */
    volatile u32                *pProcessedFlag;    /* pointer to the processed flag of last buffer queued to this DMA channel */
    u32                         PreElementCount;    /* for synchronous mode */
    u32                         i;                  /* index/counter */
    u32                         Result;             /* return value */

    /* Pointer to buffer pair (if there is any) */
    pBufPair = (ADI_DEV_BUFFER_PAIR *)pBuffer;
    /* store the buffer type to a shadow location */
    ShadowBufferType = BufferType;
    /* location to hold maximum element count amoung the given buffers to support synchronous mode */
    PreElementCount  = 0;
    /* assume we're going to be successful */
    Result = ADI_DEV_RESULT_SUCCESS;

    /* assuming we've a buffer table to process */
    while (ShadowBufferType < ADI_DEV_BUFFER_END)
    {
        /* IF (the client has actually submitted a buffer table) */
        if (BufferType == ADI_DEV_BUFFER_TABLE)
        {

/* Debug build only - Check for errors */
#if defined ADI_DEV_DEBUG
            /* look for buffer table nesting */
            if (pBufPair->BufferType == ADI_DEV_BUFFER_TABLE)
            {
                /* nesting of buffer table is not allowed. return error */
                Result = ADI_DEV_RESULT_ATTEMPTED_BUFFER_TABLE_NESTING;
                break;
            }
#endif
            /* load the listed buffer type & buffer pointer to shadow locations */
            pShadowBuffer       = pBufPair->pBuffer;
            ShadowBufferType    = pBufPair->BufferType;
            /* move to next buffer pair */
            pBufPair++;
        }
        /* ELSE (this is not a buffer table) */
        else
        {
            /* load the listed buffer type & buffer pointer to shadow locations */
            ShadowBufferType    = BufferType;
            pShadowBuffer       = pBuffer;
        }

/* Debug build only - Check for errors */
#if defined ADI_DEV_DEBUG
        /* IF (the buffer type is for switch scheme) */
        if ((ShadowBufferType == ADI_DEV_SWITCH) || (ShadowBufferType == ADI_DEV_UPDATE_SWITCH))
        {
            /* check if the switch/update switch buffer pair is valid */
            if ((((ADI_DEV_BUFFER_PAIR *)pShadowBuffer)->BufferType != ADI_DEV_1D)   &&
                (((ADI_DEV_BUFFER_PAIR *)pShadowBuffer)->BufferType != ADI_DEV_2D))
            {
                Result = ADI_DEV_RESULT_SWITCH_BUFFER_PAIR_INVALID;
                break;
            }
        }
#endif

        /* IF (device is using peripheral dma) */
        if (pDevice->PeripheralDMAFlag == TRUE)
        {
            /* update temp pointer to DMA channel information table */
            if (pDmaInfo == NULL)
            {
                /* check the dataflow direction  */
                if (Direction == ADI_DEV_DIRECTION_INBOUND)
                {
                    /* Load pDmaInfo with address to Inbound DMA channel table */
                    pDmaInfo    = pDevice->pInboundDma;
                }
                else
                {
                    /* Load pDmaInfo with address to Outbound DMA channel table */
                    pDmaInfo    = pDevice->pOutboundDma;
                }
            }

/* Debug build only - Check for errors */
#if defined ADI_DEV_DEBUG
            /* IF (client tries to update swith buffer even before providing a base switch buffer chain */
            if ((ShadowBufferType == ADI_DEV_UPDATE_SWITCH) && (pDmaInfo->SwitchModeFlag == FALSE))
            {
                Result = ADI_DEV_RESULT_BUFFER_TYPE_INCOMPATIBLE;
                break;
            }
#endif
            /* Make sure to Flush the DMA channel while entering/leaving switch mode */
            /* IF (this is a switch buffer or
                   the device is presently in switch mode and client decides to disable it) */
            if ((ShadowBufferType == ADI_DEV_SWITCH) ||
                (pDmaInfo->SwitchModeFlag && (ShadowBufferType != ADI_DEV_UPDATE_SWITCH)))
            {
                /* IF (Dataflow is already enabled) */
                if (pDevice->DataflowFlag)
                {
                    /* Disable the DMA */
                    if ((Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *) FALSE))!=ADI_DMA_RESULT_SUCCESS)
                    {
                        break;  /* exit on error */
                    }
                }
                /* Flush the DMA channel */
                if ((Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_FLUSH, (void *) NULL))!=ADI_DMA_RESULT_SUCCESS)
                {
                    break;  /* exit on error */
                }
                /* update switch mode flag */
                if (ShadowBufferType == ADI_DEV_SWITCH)
                {
                    pDmaInfo->SwitchModeFlag = TRUE;
                    /* Force DMA manager to enable Loopback mode regardless of device dataflow method */
                    if ((Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_SET_LOOPBACK, (void *) TRUE))!=ADI_DMA_RESULT_SUCCESS)
                    {
                        break;  /* exit on error */
                    }
                }
                else
                {
                    pDmaInfo->SwitchModeFlag = FALSE;
                    /* revert DMA manager loopback mode to device dataflow method */
                    if ((pDevice->DataflowMethod != ADI_DEV_MODE_CHAINED_LOOPBACK) &&
                        (pDevice->DataflowMethod != ADI_DEV_MODE_SEQ_CHAINED_LOOPBACK))
                    {
                        if ((Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_SET_LOOPBACK, (void *) FALSE))!=ADI_DMA_RESULT_SUCCESS)
                        {
                            break;  /* exit on error */
                        }
                    }
                }
                /* make sure to re-enable DMA dataflow if we're enabled */
                if (pDevice->DataflowFlag)
                {
                    /* Re-enable the DMA */
                    if ((Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *) TRUE))!=ADI_DMA_RESULT_SUCCESS)
                    {
                        break;  /* exit on error */
                    }
                }
            }

            /* IF (the buffer type is to set base switch buffer or to update the switch buffer) */
            if ((ShadowBufferType == ADI_DEV_SWITCH) || (ShadowBufferType == ADI_DEV_UPDATE_SWITCH))
            {
                /* prepare the list of buffers and make note of the last buffer in the list of buffers */
                if ((Result = PrepareBufferList(    pDevice,
                                                    ((ADI_DEV_BUFFER_PAIR *)pShadowBuffer)->BufferType,
                                                    ((ADI_DEV_BUFFER_PAIR *)pShadowBuffer)->pBuffer,
                                                    Direction,
                                                    &pLastBuffer))!= ADI_DEV_RESULT_SUCCESS)
                {
                    break;  /* exit on error */
                }
            }
            /* ELSE (this must be a circular or 1D or 2D buffer) */
            else
            {
                /* prepare the list of buffers and make note of the last buffer in the list of buffers */
                if ((Result = PrepareBufferList(    pDevice,
                                                    ShadowBufferType,
                                                    pShadowBuffer,
                                                    Direction,
                                                    &pLastBuffer)) != ADI_DEV_RESULT_SUCCESS)
                {
                    break;  /* exit on error */
                }
            }

            /* IF (the buffer type is to set switch buffer) */
            if (ShadowBufferType == ADI_DEV_SWITCH)
            {
                /* update the shadow locations from the given switch buffer pair */
                ShadowBufferType = ((ADI_DEV_BUFFER_PAIR *)pShadowBuffer)->BufferType;
                pShadowBuffer    = ((ADI_DEV_BUFFER_PAIR *)pShadowBuffer)->pBuffer;
                /* IF (1D Switch buffer) */
                if (ShadowBufferType == ADI_DEV_1D)
                {
                    /* Save the first buffer address as switch head to this DMA channel */
                    pDmaInfo->pSwitchHead = (ADI_DMA_DESCRIPTOR_UNION *)pShadowBuffer->OneD.Reserved;
                    /* Save the last buffer address as switch tail to this DMA channel */
                    pDmaInfo->pSwitchTail = (ADI_DMA_DESCRIPTOR_UNION *)pLastBuffer->OneD.Reserved;
                }
                /* ELSE IF (2D Switch buffer) */
                else if (ShadowBufferType == ADI_DEV_2D)
                {
                    /* Save the first buffer address as switch head to this DMA channel */
                    pDmaInfo->pSwitchHead = (ADI_DMA_DESCRIPTOR_UNION *)pShadowBuffer->TwoD.Reserved;
                    /* Save the last buffer address as switch tail to this DMA channel */
                    pDmaInfo->pSwitchTail = (ADI_DMA_DESCRIPTOR_UNION *)pLastBuffer->TwoD.Reserved;
                }
            }

            /* provide the buffer(s) to the DMA manager using the proper dataflow method */
            switch (ShadowBufferType)
            {
                /* CASE (Circular buffer) */
                case ADI_DEV_CIRC:
                    Result = adi_dma_Buffer(pDmaInfo->ChannelHandle,
                                            pShadowBuffer->Circular.Data,
                                            pDevice->Config,
                                            pShadowBuffer->Circular.SubBufferElementCount,  /* XCOUNT   */
                                            pShadowBuffer->Circular.ElementWidth,           /* XMODIFY  */
                                            pShadowBuffer->Circular.SubBufferCount,         /* YCOUNT   */
                                            0);                                             /* YMODIFY (ignored by DMA Manager) */
                    break;
                /* CASE (1D buffer) */
                case ADI_DEV_1D:
                    Result = adi_dma_Queue(pDmaInfo->ChannelHandle, (ADI_DMA_DESCRIPTOR_HANDLE)pShadowBuffer->OneD.Reserved);
                    break;

                /* CASE (2D buffer) */
                case ADI_DEV_2D:
                    Result = adi_dma_Queue(pDmaInfo->ChannelHandle, (ADI_DMA_DESCRIPTOR_HANDLE)pShadowBuffer->TwoD.Reserved);
                    break;

                /* CASE (update Switch buffer chain) */
                case ADI_DEV_UPDATE_SWITCH:
                    /* extract the buffer information from given update switch buffer pair */
                    ShadowBufferType = ((ADI_DEV_BUFFER_PAIR *)pShadowBuffer)->BufferType;
                    pShadowBuffer    = ((ADI_DEV_BUFFER_PAIR *)pShadowBuffer)->pBuffer;

                    /* IF (Update switch with 1D buffer) */
                    if (ShadowBufferType == ADI_DEV_1D)
                    {
                        /* Set the configuration word for given switch-update buffer chain */
                        if ((Result = adi_dma_SetConfigWord(pDmaInfo->ChannelHandle,
                                                            (ADI_DMA_DESCRIPTOR_HANDLE)pShadowBuffer->OneD.Reserved)) == ADI_DMA_RESULT_SUCCESS)
                        {
/* IF (Built for large descriptors) */
#if defined(ADI_DEV_LARGE)
                            /* Link the last buffer in this update chain to the switch buffer head */
                            ((ADI_DMA_DESCRIPTOR_LARGE *)pLastBuffer->OneD.Reserved)->pNext = (ADI_DMA_DESCRIPTOR_LARGE *)pDmaInfo->pSwitchHead;
                            /* Update switch tail with new buffer chain */
                            pDmaInfo->pSwitchTail->Large.pNext = (ADI_DMA_DESCRIPTOR_LARGE *)pShadowBuffer->OneD.Reserved;
#endif
/* IF (Built for small descriptors) */
#if defined(ADI_DEV_SMALL)
                            /* Link the last buffer in this update chain to the switch buffer head */
                            ((ADI_DMA_DESCRIPTOR_SMALL *)pLastBuffer->OneD.Reserved)->pNext = (u16)((u32)(pDmaInfo->pSwitchHead));
                            /* Update switch tail with new buffer chain */
                            pDmaInfo->pSwitchTail->Small.pNext = (u16)((u32)(pShadowBuffer->OneD.Reserved));
#endif
                        }
                    }
                    /* ELSE IF (Update switch with 2D buffer) */
                    else if (ShadowBufferType == ADI_DEV_2D)
                    {
                        /* Set the configuration word for given switch-update buffer chain */
                        if ((Result = adi_dma_SetConfigWord(pDmaInfo->ChannelHandle,
                                                            (ADI_DMA_DESCRIPTOR_HANDLE)pShadowBuffer->TwoD.Reserved)) == ADI_DMA_RESULT_SUCCESS)
                        {
/* IF (Built for large descriptors) */
#if defined(ADI_DEV_LARGE)
                            /* Link the last buffer in this update chain to the switch buffer head */
                            ((ADI_DMA_DESCRIPTOR_LARGE *)pLastBuffer->TwoD.Reserved)->pNext = (ADI_DMA_DESCRIPTOR_LARGE *)pDmaInfo->pSwitchHead;
                            /* Update switch tail with new buffer chain */
                            pDmaInfo->pSwitchTail->Large.pNext = (ADI_DMA_DESCRIPTOR_LARGE *)pShadowBuffer->TwoD.Reserved;
#endif
/* IF (Built for small descriptors) */
#if defined(ADI_DEV_SMALL)
                            /* Link the last buffer in this update chain to the switch buffer head */
                            ((ADI_DMA_DESCRIPTOR_SMALL *)pLastBuffer->TwoD.Reserved)->pNext = (u16)((u32)(pDmaInfo->pSwitchHead));
                            /* Update switch tail with new buffer chain */
                            pDmaInfo->pSwitchTail->Small.pNext = (u16)((u32)(pShadowBuffer->TwoD.Reserved));
#endif
                        }
                    }

                    break;

                /* CASE (Skip this buffer & corresponding DMA channel) */
                case ADI_DEV_BUFFER_SKIP:
                    break;
            }
        }
        else
        {
            /* prepare the list of buffers and make note of the last buffer in the list of buffers */
            if ((Result = PrepareBufferList(    pDevice,
                                                ShadowBufferType,
                                                pShadowBuffer,
                                                Direction,
                                                &pLastBuffer)) != ADI_DEV_RESULT_SUCCESS)
            {
                break;  /* exit on error */
            }
            /* send the buffer directly to the physical driver */
            if (Direction == ADI_DEV_DIRECTION_OUTBOUND)
            {
                Result = (pDevice->pEntryPoint->adi_pdd_Write) (pDevice->PDDHandle, ShadowBufferType, pShadowBuffer);
            }
            else
            {
                Result = (pDevice->pEntryPoint->adi_pdd_Read) (pDevice->PDDHandle, ShadowBufferType, pShadowBuffer);
            }
        }

        /* buffer submission success? */
        if (Result != ADI_DEV_RESULT_SUCCESS)
        {
            break;  /* Buffer submission failed. return error */
        }
        /* successfully queued buffer to a DMA channel. move to next DMA channel in the list */
        else if (pDevice->PeripheralDMAFlag == TRUE)
        {
            pDmaInfo = pDmaInfo->pNext;
        }

        /* Synchronous mode */
        /* wait till the last buffer in the list is processed if it's synchronous */
        if ((ShadowBufferType != ADI_DEV_CIRC) &&
            (ShadowBufferType != ADI_DEV_BUFFER_SKIP) &&
            (pDevice->SynchronousFlag == TRUE))
        {
            /* update pointer to pProcessedFlag of last buffer for multi channel DMA peripheral */
            if (ShadowBufferType == ADI_DEV_1D)
            {
                /* check if the element count for this buffer is greater than the previous */
                if (pShadowBuffer->OneD.ElementCount >= PreElementCount)
                {
                    /* update the PreElementCount with new value */
                    PreElementCount = pShadowBuffer->OneD.ElementCount;
                    /* update pProcessedFlag as this buffer has the maximum element count
                       and would take more time to finish than the previous */
                    pProcessedFlag = &(pLastBuffer->OneD.ProcessedFlag);
                }
            }
            else if (ShadowBufferType == ADI_DEV_2D)
            {
                if ((pShadowBuffer->TwoD.XCount * pShadowBuffer->TwoD.YCount) >= PreElementCount)
                {
                    /* update the PreElementCount with new value */
                    PreElementCount = (pShadowBuffer->TwoD.XCount  * pShadowBuffer->TwoD.YCount);
                    /* update pProcessedFlag as this buffer has the maximum element count
                       and would take more time to finish than the previous */
                    pProcessedFlag = &(pLastBuffer->TwoD.ProcessedFlag);
                }
            }

            /* IF (device is using peripheral dma & has reached end of DMA channel information table) */
            if ((pDevice->PeripheralDMAFlag == TRUE) && (pDmaInfo == NULL))
            {
                /* all DMA channels are queued with even buffers. wait until all buffers are processed */
                while (*pProcessedFlag == FALSE) ;
            }
            /* ELSE (this is not a DMA driven device) */
            else
            {
                /* wait until all buffers are processed */
                while (*pProcessedFlag == FALSE) ;
            }
        }

        /* IF (process a table of buffers) */
        if (BufferType == ADI_DEV_BUFFER_TABLE)
        {
            /* move to next buffer type in the given buffer table */
            ShadowBufferType = pBufPair->BufferType;
        }
        /* ELSE (process only one buffer) */
        else
        {
            break;  /* Done with buffer submission. exit this loop */
        }
    }

    /* return */
    return (Result);
}

/*********************************************************************
*
*   Function:       adi_dev_SequentialIO
*
*   Description:    Performs reads and/or writes in a specific sequence
*
*********************************************************************/
u32 adi_dev_SequentialIO(                       /* Performs sequential reads and/or writes through the device   */
    ADI_DEV_DEVICE_HANDLE       DeviceHandle,   /* Device handle                                                */
    ADI_DEV_BUFFER_TYPE         BufferType,     /* buffer type                                                  */
    ADI_DEV_BUFFER              *pBuffer        /* pointer to buffer                                            */
){

    u32                         Result;         /* return value */
    ADI_DEV_DEVICE              *pDevice;       /* pointer to the device we're working on */
    ADI_DEV_BUFFER              *pLastBuffer;   /* pointer to the last buffer in the list past in */

/* IF (Built for large descriptors) */
#if defined(ADI_DEV_LARGE)
    ADI_DMA_DESCRIPTOR_LARGE    *pDescriptor;   // pointer to a descriptor within a buffer
#endif
/* IF (Built for small descriptors) */
#if defined(ADI_DEV_SMALL)
    ADI_DMA_DESCRIPTOR_SMALL    *pDescriptor;   // pointer to a descriptor within a buffer
#endif

    /* avoid casts */
    pDevice = (ADI_DEV_DEVICE *)DeviceHandle;

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
    /* Validate Device Handle */
    if ((Result = ValidateDeviceHandle(DeviceHandle)) == ADI_DEV_RESULT_SUCCESS)
    {
        /* verify dataflow method established */
        if (pDevice->DataflowMethod == ADI_DEV_MODE_UNDEFINED)
        {
            Result = ADI_DEV_RESULT_DATAFLOW_UNDEFINED;
        }
        /* only allowed with these modes */
        if ((pDevice->DataflowMethod != ADI_DEV_MODE_SEQ_CHAINED) &&
            (pDevice->DataflowMethod != ADI_DEV_MODE_SEQ_CHAINED_LOOPBACK))
        {
            Result = ADI_DEV_RESULT_DATAFLOW_INCOMPATIBLE;
        }
        /* Validate DMA channel information table for DMA driven devices */
        if ((pDevice->PeripheralDMAFlag == TRUE) && (pDevice->pInboundDma == NULL))
        {
            Result = ADI_DEV_RESULT_DMA_CHANNEL_UNAVAILABLE;
        }
        /* only support sequential buffers */
        if (BufferType != ADI_DEV_SEQ_1D)
        {
            Result = ADI_DEV_RESULT_BUFFER_TYPE_INCOMPATIBLE;
        }
        /* don't allow if synchronous mode and dataflow is not enabled */
        if ((pDevice->SynchronousFlag == TRUE) && (pDevice->DataflowFlag != TRUE))
        {
            Result = ADI_DEV_RESULT_DATAFLOW_NOT_ENABLED;
        }
    }

    /* Continue only on success */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif

    /* prepare the list of buffers and make note of the last buffer in the list of buffers */
    /* NOTE: the direction being passed in as a parameter is meaningless as each buffer has a direction
       associated with the buffer */
    if ((Result = PrepareBufferList(    pDevice,
                                        BufferType,
                                        pBuffer,
                                        ADI_DEV_DIRECTION_INBOUND,
                                        &pLastBuffer)) == ADI_DEV_RESULT_SUCCESS)
    {
        /* IF (device is using peripheral dma) */
        if (pDevice->PeripheralDMAFlag == TRUE)
        {

        /* Queue it on the inbound channel as we only use 1 DMA channel for sequential IO */
        /* All buffers will be queued to the first DMA channel listed in Inbound DMA Channel information table */
        Result = adi_dma_Queue(pDevice->pInboundDma->ChannelHandle, (ADI_DMA_DESCRIPTOR_HANDLE)pBuffer->Seq1D.Buffer.Reserved);
        /* ELSE */
        }
        else
        {
            /* send the buffer directly to the physical driver */
            Result = (pDevice->pEntryPoint->adi_pdd_SequentialIO) (pDevice->PDDHandle, BufferType, pBuffer);
        }

        /* IF (buffer submission results in success */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            /* wait till the last buffer in the list is processed if it's synchronous */
            if (pDevice->SynchronousFlag == TRUE)
            {
                while (pLastBuffer->Seq1D.Buffer.ProcessedFlag == FALSE);
            }
        }
    }

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return (Result);
}

/*********************************************************************
*
*   Function:       adi_dev_Control
*
*   Description:    Processes configuration control commands for a device driver
*
*********************************************************************/
u32 adi_dev_Control(                            /* Sets or senses a device specific parameter   */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* Device handle                                */
    u32                     CommandID,          /* command ID                                   */
    void                    *Value              /* command specific value                       */
){

    u32                     Result;             /* return value */
    ADI_DEV_DEVICE          *pDevice;           /* pointer to the device we're working on */
    u32                     PassToPDD;          /* flag indicating whether or not we pass the command to the PDD */
    ADI_DMA_MODE            DMAMode;            /* DMA mode value */
    u32                     DMALoopbackFlag;    /* DMA loopback flag */
    ADI_DMA_CHANNEL_ID      ChannelID;          /* DMA channel ID */
    ADI_DEV_CMD_VALUE_PAIR  *pPair;             /* pointer to command pair */
    ADI_DMA_PMAP            pmap;               /* DMA pmap value */
    u32                     pddCommandID;       /* command to pdd */
    ADI_DEV_1D_BUFFER       *pBuffer;           /* pointer to a buffer */
    u32                     i;                  /* generic counter/index */
    ADI_DMA_CONFIG_REG      Config;             /* DMA configuration register */
    ADI_DEV_DMA_INFO        *pDmaInfo = NULL;   /* pointer to DMA channel information table of the device we're working on */

    /* avoid casts, assume we're going to be successful and assume the PDD will need to process this */
    pDevice     = (ADI_DEV_DEVICE *)DeviceHandle;
    Result      = ADI_DEV_RESULT_SUCCESS;
    PassToPDD   = TRUE;

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
    /* Validate Device Handle */
    if ((Result = ValidateDeviceHandle(DeviceHandle)) == ADI_DEV_RESULT_SUCCESS)
    {
#endif

    /* CASEOF (command ID) */
    switch (CommandID)
    {

        /* CASE (command table) */
        case ADI_DEV_CMD_TABLE:

            /* avoid casts */
            pPair = (ADI_DEV_CMD_VALUE_PAIR *)Value;

            /* process each command pair */
            while (pPair->CommandID != ADI_DEV_CMD_END)
            {
                if ((Result = adi_dev_Control(DeviceHandle, pPair->CommandID, pPair->Value)) != ADI_DEV_RESULT_SUCCESS)
                {
                    break;  /* exit on error */
                }
                pPair++;
            }

            /* don't need to pass this to the PDD */
            PassToPDD = FALSE;
            break;

        /* CASE (command table terminator) */
        case ADI_DEV_CMD_END:

            /* don't need to pass this to the PDD */
            PassToPDD = FALSE;
            break;

        /* CASE (command pair) */
        case ADI_DEV_CMD_PAIR:

            /* avoid casts */
            pPair = (ADI_DEV_CMD_VALUE_PAIR *)Value;
            /* don't need to pass this to the PDD */
            PassToPDD = FALSE;
            /* process the command pair */
            Result = adi_dev_Control(DeviceHandle, pPair->CommandID, pPair->Value);
            break;

        /* CASE (a 2D DMA setting) */
        case (ADI_DEV_CMD_GET_2D_SUPPORT):

            /* IF (we're using peripheral dma for the device) */
            if (pDevice->PeripheralDMAFlag == TRUE)
            {
                /* we support 2D DMA for the chained dataflow with/without loopback modes only (normal only, not sequential) */
                if ((pDevice->DataflowMethod == ADI_DEV_MODE_CHAINED) || (pDevice->DataflowMethod == ADI_DEV_MODE_CHAINED_LOOPBACK))
                {
                    *((u32 *)Value) = TRUE;
                }
                else
                {
                    *((u32 *)Value) = FALSE;
                }

                /* don't need to pass this to the PDD */
                PassToPDD = FALSE;
            }
            break;

        /* CASE (set a dataflow method) */
        case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
            /* check for incompatibilities */
            /* CASEOF (Dataflow method) */
            switch ((ADI_DEV_MODE)Value)
            {
                /* CASE (Sequential chained) */
                case ADI_DEV_MODE_SEQ_CHAINED:
                /* CASE (Sequential chained with loopback) */
                case ADI_DEV_MODE_SEQ_CHAINED_LOOPBACK:
                    if (pDevice->Direction != ADI_DEV_DIRECTION_BIDIRECTIONAL)
                    {
                        Result = ADI_DEV_RESULT_REQUIRES_BIDIRECTIONAL_DEVICE;
                    }
                    break;
            }
            /* continue only on success */
            if (Result != ADI_DEV_RESULT_SUCCESS)
            {
                break;
            }
#endif

            /* save the dataflow method in our device data */
            pDevice->DataflowMethod = ((ADI_DEV_MODE)Value);

            /* IF (we're using peripheral dma for the device) */
            if (pDevice->PeripheralDMAFlag == TRUE)
            {
                if (pDevice->DataflowMethod == ADI_DEV_MODE_CIRCULAR)
                {
                    pDevice->Config.b_FLOW   = ADI_DMA_FLOW_AUTOBUFFER;
                    pDevice->Config.b_NDSIZE = ADI_DMA_NDSIZE_STOP;
                    pDevice->Config.b_DMA2D  = ADI_DMA_DMA2D_2D;
                }
                else
                {
/* IF (Built for large descriptors) */
#if defined(ADI_DEV_LARGE)
                    pDevice->Config.b_FLOW   = ADI_DMA_FLOW_LARGE;
                    pDevice->Config.b_NDSIZE = ADI_DMA_NDSIZE_LARGE;
                    pDevice->Config.b_DI_SEL = ADI_DMA_DI_SEL_OUTER_LOOP;
#endif
/* IF (Built for small descriptors) */
#if defined(ADI_DEV_SMALL)
                    pDevice->Config.b_FLOW   = ADI_DMA_FLOW_SMALL;
                    pDevice->Config.b_NDSIZE = ADI_DMA_NDSIZE_SMALL;
                    pDevice->Config.b_DI_SEL = ADI_DMA_DI_SEL_OUTER_LOOP;
#endif
                }

                /* open inbound DMA channel(s) and tell the DMA controller about loopback */
                if ((pDevice->Direction == ADI_DEV_DIRECTION_INBOUND) ||
                    (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL))
                {
                    /* for all Inbound DMA channels */
                    for (pDmaInfo = pDevice->pInboundDma; pDmaInfo; pDmaInfo = pDmaInfo->pNext )
                    {
                        if ((Result = adi_dev_Control(DeviceHandle, ADI_DEV_CMD_OPEN_PERIPHERAL_DMA, pDmaInfo)) != ADI_DEV_RESULT_SUCCESS)
                        {
                            break;
                        }
                    }
                }
                if (Result != ADI_DEV_RESULT_SUCCESS)
                {
                    /* don't need to pass this to the PDD */
                    PassToPDD = FALSE;
                    break;
                }

                /* open inbound DMA channel(s) and tell the DMA controller about loopback */
                if ((pDevice->Direction == ADI_DEV_DIRECTION_OUTBOUND) || (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL))
                {
                    /* for all Outbound DMA channels */
                    for (pDmaInfo = pDevice->pOutboundDma; pDmaInfo; pDmaInfo = pDmaInfo->pNext )
                    {
                        if ((Result = adi_dev_Control(DeviceHandle, ADI_DEV_CMD_OPEN_PERIPHERAL_DMA, pDmaInfo)) != ADI_DEV_RESULT_SUCCESS)
                        {
                            break;
                        }
                    }
                }

                /* don't need to pass this to the PDD */
                PassToPDD = FALSE;
            }
            break;

        /* CASE (Open a peripheral DMA channel) */
        case (ADI_DEV_CMD_OPEN_PERIPHERAL_DMA):

            /* DMA channel to be opened */
            pDmaInfo = (ADI_DEV_DMA_INFO *) Value;
            /* continue only when pDmaInfo is valid & Device is set with a vaild dataflow method */
            if ((pDmaInfo != NULL) && (pDevice->DataflowMethod != ADI_DEV_MODE_UNDEFINED))
            {
                if (pDevice->DataflowMethod == ADI_DEV_MODE_CIRCULAR)
                {
                    DMAMode = ADI_DMA_MODE_CIRCULAR;
                }
                else
                {
/* IF (Built for large descriptors) */
#if defined(ADI_DEV_LARGE)
                    DMAMode = ADI_DMA_MODE_DESCRIPTOR_LARGE;
#endif
/* IF (Built for small descriptors) */
#if defined(ADI_DEV_SMALL)
                    DMAMode = ADI_DMA_MODE_DESCRIPTOR_SMALL;
#endif
                }
                /* update the DMA flags */
                if ((pDevice->DataflowMethod == ADI_DEV_MODE_CHAINED_LOOPBACK) ||
                    (pDevice->DataflowMethod == ADI_DEV_MODE_SEQ_CHAINED_LOOPBACK))
                {
                    DMALoopbackFlag = TRUE;
                }
                else
                {
                    DMALoopbackFlag = FALSE;
                }
                /* Get the DMA channel ID */
                if ((Result = adi_dma_GetMapping(pDmaInfo->MappingID, &ChannelID)) != ADI_DMA_RESULT_SUCCESS)
                {
                    break;
                }
                /* Open the DMA channel */
                if ((Result = adi_dma_Open( pDevice->DMAHandle,
                                            ChannelID,
                                            pDevice,
                                            &(pDmaInfo->ChannelHandle),
                                            DMAMode,
                                            pDevice->DCBHandle,
                                            DMACallback)) != ADI_DMA_RESULT_SUCCESS)
                {
                    break;
                }
                /* set loopback mode */
                if ((Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_SET_LOOPBACK, (void *)DMALoopbackFlag)) != ADI_DMA_RESULT_SUCCESS)
                {
                    break;
                }
                /* set DMA stream mode */
                Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_SET_STREAMING, (void *)pDevice->StreamingFlag);
            }
            /* don't to pass this to the PDD */
            PassToPDD = FALSE;
            break;

        /* CASE (Close a peripheral DMA channel) */
        case (ADI_DEV_CMD_CLOSE_PERIPHERAL_DMA):

            /* DMA channel to be closed */
            pDmaInfo = (ADI_DEV_DMA_INFO *) Value;
            /* continue only when pDmaInfo is valid & Device is set with a vaild dataflow method */
            if ((pDmaInfo != NULL) && (pDevice->DataflowMethod != ADI_DEV_MODE_UNDEFINED))
            {
                if (pDmaInfo->ChannelHandle)
                {
                    /* Close this DMA channel */
                    if ((Result = adi_dma_Close(pDmaInfo->ChannelHandle, FALSE)) == ADI_DMA_RESULT_SUCCESS)
                    {
                        pDmaInfo->ChannelHandle = NULL;
                    }
                }
            }
            /* don't to pass this to the PDD */
            PassToPDD = FALSE;
            break;

        /* CASE (set dataflow) */
        case (ADI_DEV_CMD_SET_DATAFLOW):

            /* do nothing if dataflow is not being changed */
            if ((u32)Value == pDevice->DataflowFlag) {
                PassToPDD = FALSE;
                break;
            }

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
            /* verify dataflow method established */
            if (pDevice->DataflowMethod == ADI_DEV_MODE_UNDEFINED)
            {
                Result = ADI_DEV_RESULT_DATAFLOW_UNDEFINED;
                break;
            }
#endif

            /* IF (we're using peripheral dma for the device) */
            if (pDevice->PeripheralDMAFlag == TRUE)
            {
                /* process it */
                Result = SetDataflow (pDevice, ((u32)Value));

                /* don't need to pass this to the PDD as the SetDataflow routine does this as necessary */
                PassToPDD = FALSE;
            }
            else
            {
                /* update the device manager status of dataflow */
                pDevice->DataflowFlag = ((u8)((u32)Value));
            }
            break;

        /* CASE (setting synchronous mode) */
        case (ADI_DEV_CMD_SET_SYNCHRONOUS):


/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)

            /* tar 37860 - If data flow method is not chained, then setting synchronous
               mode to TRUE should produce a failure result code */

            if  ( ((u8)((u32)Value) == TRUE ) && !((pDevice->DataflowMethod == ADI_DEV_MODE_SEQ_CHAINED)
                || (pDevice->DataflowMethod == ADI_DEV_MODE_SEQ_CHAINED_LOOPBACK)))
            {
                Result = ADI_DEV_RESULT_DATAFLOW_INCOMPATIBLE;
            }
#endif


            /* update the synchronous flag for the device */
            pDevice->SynchronousFlag = ((u8)((u32)Value));

            /* don't need to pass this to the PDD */
            PassToPDD = FALSE;
            break;

        /* CASE (setting streaming) */
        case (ADI_DEV_CMD_SET_STREAMING):

            /* IF (we're using peripheral dma for the device) */
            if (pDevice->PeripheralDMAFlag == TRUE)
            {
                /* don't need to pass this to the PDD as the SetDataflow routine does this as necessary */
                PassToPDD = FALSE;

                /* Save the streaming state */
                pDevice->StreamingFlag = ((u8)((u32)Value));
                /* process it */
                /* Set this streaming status to all DMA channels in use */
                /* for all Inbound DMA channels */
                for (pDmaInfo=pDevice->pInboundDma; pDmaInfo; pDmaInfo = pDmaInfo->pNext )
                {
                    /* IF (this DMA channel is already open) */
                    if (pDmaInfo->ChannelHandle)
                    {
                        if ((Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_SET_STREAMING, Value)) != ADI_DMA_RESULT_SUCCESS)
                        {
                            break;
                        }
                    }
                }

                if (Result != ADI_DEV_RESULT_SUCCESS)
                {
                    break;
                }

                /* for all Outbound DMA channels */
                for (pDmaInfo=pDevice->pOutboundDma; pDmaInfo; pDmaInfo = pDmaInfo->pNext )
                {
                    /* IF (this DMA channel is already open) */
                    if (pDmaInfo->ChannelHandle)
                    {
                        if ((Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_SET_STREAMING, Value)) != ADI_DMA_RESULT_SUCCESS)
                        {
                            break;
                        }
                    }
                }
            }
            break;

        /* CASE (getting DMA channel) */
        case (ADI_DEV_CMD_GET_INBOUND_DMA_CHANNEL_ID):
        case (ADI_DEV_CMD_GET_OUTBOUND_DMA_CHANNEL_ID):

            /* IF (we're using peripheral dma for the device) */
            if (pDevice->PeripheralDMAFlag == TRUE)
            {
                /* determine whether we need inbound or outbound */
                /* IF (Get inbound DMA channel id & if this device supports inbound data) */
                if ((CommandID == ADI_DEV_CMD_GET_INBOUND_DMA_CHANNEL_ID) && (pDevice->pInboundDma != NULL))
                {
                    /* query the DMA manager for the channel ID */
                    Result = adi_dma_GetMapping(pDevice->pInboundDma->MappingID, ((ADI_DMA_CHANNEL_ID *)Value));
                }
                /* IF (Get outbound DMA channel id & if this device supports outbound data) */
                else if ((CommandID == ADI_DEV_CMD_GET_OUTBOUND_DMA_CHANNEL_ID) && (pDevice->pOutboundDma != NULL))
                {
                    Result = adi_dma_GetMapping(pDevice->pOutboundDma->MappingID, ((ADI_DMA_CHANNEL_ID *)Value));
                }
                /* ELSE (No DMA channel listed by this device, return error */
                else
                {
                    Result = ADI_DEV_RESULT_DMA_CHANNEL_UNAVAILABLE;
                }

                /* don't need to pass this to the PDD */
                PassToPDD = FALSE;
            }
            break;

        /* CASE (setting DMA channel) */
        case (ADI_DEV_CMD_SET_INBOUND_DMA_CHANNEL_ID):
        case (ADI_DEV_CMD_SET_OUTBOUND_DMA_CHANNEL_ID):

            /* only allow if the dataflow method is not yet established */
            if (pDevice->DataflowMethod != ADI_DEV_MODE_UNDEFINED)
            {
                Result = ADI_DEV_RESULT_INVALID_SEQUENCE;
                break;
            }

            /* IF (we're using peripheral dma for the device) */
            if (pDevice->PeripheralDMAFlag == TRUE)
            {
                /* determine whether we need inbound or outbound */
                /* IF (Set inbound DMA channel id & if this device supports inbound data) */
                if ((CommandID == ADI_DEV_CMD_SET_INBOUND_DMA_CHANNEL_ID) && (pDevice->pInboundDma != NULL))
                {
                    /* query the DMA manager for the channel ID */
                    Result = adi_dma_SetMapping(pDevice->pInboundDma->MappingID, ((ADI_DMA_CHANNEL_ID)Value));
                }
                /* IF (Set outbound DMA channel id & if this device supports outbound data) */
                else if ((CommandID == ADI_DEV_CMD_SET_OUTBOUND_DMA_CHANNEL_ID) && (pDevice->pOutboundDma != NULL))
                {
                    Result = adi_dma_SetMapping(pDevice->pOutboundDma->MappingID, ((ADI_DMA_CHANNEL_ID)Value));
                }
                /* ELSE (No DMA channel listed by this device, return error */
                else
                {
                    Result = ADI_DEV_RESULT_DMA_CHANNEL_UNAVAILABLE;
                }

                /* don't need to pass this to the PDD */
                PassToPDD = FALSE;
            }
            break;

        /* CASE (update data pointer) */
        case (ADI_DEV_CMD_UPDATE_1D_DATA_POINTER):
        case (ADI_DEV_CMD_UPDATE_2D_DATA_POINTER):
        case (ADI_DEV_CMD_UPDATE_SEQ_1D_DATA_POINTER):

            /* IF (we're using peripheral dma for the device) */
            if (pDevice->PeripheralDMAFlag == TRUE)
            {
                /* don't need to pass this to the PDD */
                PassToPDD = FALSE;

                /* update the data pointer within the descriptor */
                /* NOTE: it doesn't matter whether this is 1D, 2D or sequential because we require that
                         for all buffer types, the data start address is the first entry in the structure */
                pBuffer = (ADI_DEV_1D_BUFFER *)Value;
                ADI_DEV_SET_DESCRIPTOR_START_ADDRESS(pBuffer,pBuffer->Data);
            }
            break;
        /* ENDCASE */


        /* CASE (get current address register value of Inbound DMA) */
        case (ADI_DEV_CMD_GET_INBOUND_DMA_CURRENT_ADDRESS):
        /* CASE (get current address register value of Outbound DMA) */
        case (ADI_DEV_CMD_GET_OUTBOUND_DMA_CURRENT_ADDRESS):

            /* IF (we're using peripheral dma for the device) */
            if (pDevice->PeripheralDMAFlag == TRUE)
            {
                /* don't need to pass this to the PDD */
                PassToPDD = FALSE;

                /* determine whether we need inbound or outbound */
                /* IF (the command is for inbound DMA and we've an open Inbound DMA channel, qurey for current address value) */
                if ((CommandID == ADI_DEV_CMD_GET_INBOUND_DMA_CURRENT_ADDRESS) && (pDevice->pInboundDma != NULL))
                {
                    pDmaInfo=pDevice->pInboundDma;
                }
                /* ELSE IF (the command is for outbound DMA and we've an open Outbound DMA channel, qurey for current address value) */
                else if ((CommandID == ADI_DEV_CMD_GET_OUTBOUND_DMA_CURRENT_ADDRESS) && (pDevice->pOutboundDma != NULL))
                {
                    pDmaInfo=pDevice->pOutboundDma;
                }
                /* ELSE (No DMA channel listed by this device, return error */
                else
                {
                    Result = ADI_DEV_RESULT_DMA_CHANNEL_UNAVAILABLE;
                    break;
                }
                /* IF (we've a open DMA channel) */
                if (pDmaInfo->ChannelHandle)
                {
                    Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_GET_CURRENT_ADDRESS, (void *)Value);
                }
                else
                {
                    *((u32 *)Value) = 0;
                }
            }
            break;

        /* CASE (getting DMA chain) */
        case (ADI_DEV_CMD_GET_INBOUND_DMA_CHAIN_CHANNEL_ID):
        case (ADI_DEV_CMD_GET_OUTBOUND_DMA_CHAIN_CHANNEL_ID):

            /* IF (we're using peripheral dma for the device) */
            if (pDevice->PeripheralDMAFlag == TRUE)
            {
                /* avoid casts */
                u8  DmaChannelCount = ((ADI_DEV_DMA_ACCESS *)Value)->DmaChannelCount;
                ADI_DMA_CHANNEL_ID  *pDevDmaChannelId   = ((ADI_DEV_DMA_ACCESS *)Value)->pData;

                /* determine whether we need inbound or outbound */
                /* IF (Get inbound DMA channel id & if this device supports inbound data) */
                if ((CommandID == ADI_DEV_CMD_GET_INBOUND_DMA_CHAIN_CHANNEL_ID) && (pDevice->pInboundDma != NULL))
                {
                    pDmaInfo=pDevice->pInboundDma;
                }
                /* ELSE IF (Get outbound DMA channel id & if this device supports outbound data) */
                else if ((CommandID == ADI_DEV_CMD_GET_OUTBOUND_DMA_CHAIN_CHANNEL_ID) && (pDevice->pOutboundDma != NULL))
                {
                    pDmaInfo=pDevice->pOutboundDma;
                }
                /* ELSE (No DMA channel listed by this device, return error */
                else
                {
                    Result = ADI_DEV_RESULT_DMA_CHANNEL_UNAVAILABLE;
                    break;
                }

                /* FOR (all DMA channels in the chain or until we reach the DMA count requested by the client) */
                for (i=0;i<DmaChannelCount;pDmaInfo = pDmaInfo->pNext,pDevDmaChannelId++, i++)
                {
                    if ((Result != ADI_DMA_RESULT_SUCCESS) || (pDmaInfo == NULL))
                    {
                        break;
                    }
                    /* query the DMA manager for the channel ID */
                    Result = adi_dma_GetMapping(pDmaInfo->MappingID, ((ADI_DMA_CHANNEL_ID *)pDevDmaChannelId));
                }
                /* don't need to pass this to the PDD */
                PassToPDD = FALSE;
            }
            break;
        /* END CASE */

        /* CASE (setting DMA chain) */
        case (ADI_DEV_CMD_SET_INBOUND_DMA_CHAIN_CHANNEL_ID):
        case (ADI_DEV_CMD_SET_OUTBOUND_DMA_CHAIN_CHANNEL_ID):

            /* only allow if the dataflow method is not yet established */
            if (pDevice->DataflowMethod != ADI_DEV_MODE_UNDEFINED)
            {
                Result = ADI_DEV_RESULT_INVALID_SEQUENCE;
                break;
            }

            /* IF (we're using peripheral dma for the device) */
            if (pDevice->PeripheralDMAFlag == TRUE)
            {
                /* avoid casts */
                u8  DmaChannelCount = ((ADI_DEV_DMA_ACCESS *)Value)->DmaChannelCount;
                ADI_DMA_CHANNEL_ID  *pDevDmaChannelId   = ((ADI_DEV_DMA_ACCESS *)Value)->pData;

                /* determine whether we need inbound or outbound */
                /* IF (Set inbound DMA channel id & if this device supports inbound data) */
                if ((CommandID == ADI_DEV_CMD_SET_INBOUND_DMA_CHAIN_CHANNEL_ID) && (pDevice->pInboundDma != NULL))
                {
                    pDmaInfo=pDevice->pInboundDma;
                }
                /* ELSE IF (Set outbound DMA channel id & if this device supports outbound data) */
                else if ((CommandID == ADI_DEV_CMD_SET_OUTBOUND_DMA_CHAIN_CHANNEL_ID) && (pDevice->pOutboundDma != NULL))
                {
                    pDmaInfo=pDevice->pOutboundDma;
                }
                /* ELSE (No DMA channel listed by this device, return error */
                else
                {
                    Result = ADI_DEV_RESULT_DMA_CHANNEL_UNAVAILABLE;
                    break;
                }

                /* FOR (all DMA channels in the chain or until we reach the DMA count requested by the client) */
                for (i=0;i<DmaChannelCount;pDmaInfo = pDmaInfo->pNext,pDevDmaChannelId++, i++)
                {
                    if ((Result != ADI_DMA_RESULT_SUCCESS) || (pDmaInfo == NULL))
                    {
                        break;
                    }
                    /* query the DMA manager for the channel ID */
                    Result = adi_dma_SetMapping(pDmaInfo->MappingID, ((ADI_DMA_CHANNEL_ID)pDevDmaChannelId));
                }
                /* don't need to pass this to the PDD */
                PassToPDD = FALSE;
            }
            break;
        /* END CASE */

        /* CASE (get current address register value of Inbound DMA chain) */
        case (ADI_DEV_CMD_GET_INBOUND_DMA_CHAIN_CURRENT_ADDRESS):
        /* CASE (get current address register value of Outbound DMA chain) */
        case (ADI_DEV_CMD_GET_OUTBOUND_DMA_CHAIN_CURRENT_ADDRESS):

            /* IF (we're using peripheral dma for the device) */
            if (pDevice->PeripheralDMAFlag == TRUE)
            {
                /* don't need to pass this to the PDD */
                PassToPDD = FALSE;

                /* avoid casts */
                u8  DmaChannelCount = ((ADI_DEV_DMA_ACCESS *)Value)->DmaChannelCount;
                u32 *pDmaCurrAddr   = ((ADI_DEV_DMA_ACCESS *)Value)->pData;

                /* determine whether we need inbound or outbound */
                /* IF (Get current address register value of Inbound DMA chain & if this device supports inbound data) */
                if ((CommandID == ADI_DEV_CMD_GET_INBOUND_DMA_CHAIN_CURRENT_ADDRESS) && (pDevice->pInboundDma != NULL))
                {
                    pDmaInfo=pDevice->pInboundDma;
                }
                /* ELSE IF (Get current address register value of Outbound DMA chain & if this device supports outbound data) */
                else if ((CommandID == ADI_DEV_CMD_GET_OUTBOUND_DMA_CHAIN_CURRENT_ADDRESS) && (pDevice->pOutboundDma != NULL))
                {
                    pDmaInfo=pDevice->pOutboundDma;
                }
                /* ELSE (No DMA channel listed by this device, return error */
                else
                {
                    Result = ADI_DEV_RESULT_DMA_CHANNEL_UNAVAILABLE;
                    break;
                }

                /* FOR (all DMA channels in the chain or until we reach the DMA count requested by the client) */
                for (i=0;i<DmaChannelCount;pDmaInfo = pDmaInfo->pNext,pDmaCurrAddr++, i++)
                {
                    if ((Result != ADI_DMA_RESULT_SUCCESS) || (pDmaInfo == NULL))
                    {
                        break;
                    }
                    /* IF (we've a open DMA channel) */
                    if (pDmaInfo->ChannelHandle)
                    {
                        Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_GET_CURRENT_ADDRESS, (void *)pDmaCurrAddr);
                    }
                    else
                    {
                        *pDmaCurrAddr = 0;
                    }
                }
            }
            break;

        /* CASE (change client callback function ) */
        case (ADI_DEV_CMD_CHANGE_CLIENT_CALLBACK_FUNCTION):

            /* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
            if (Value == NULL) {
                Result = ADI_DEV_RESULT_NO_CALLBACK_FUNCTION_SUPPLIED;
                break;
            }
#endif

            /* replace the callback function for the device */
            pDevice->ClientCallback = (ADI_DCB_CALLBACK_FN)Value;

            /* don't need to pass this to the PDD */
            PassToPDD = FALSE;
            break;

        /* CASE (change client callback function ) */
        case (ADI_DEV_CMD_CHANGE_CLIENT_HANDLE):

            /* replace the callback function for the device */
            pDevice->ClientHandle = Value;

            /* don't need to pass this to the PDD */
            PassToPDD = FALSE;
            break;

    /* ENDCASE */
    }

    /* IF (the PDD needs to process the command) */
    if ((Result == ADI_DEV_RESULT_SUCCESS) && (PassToPDD == TRUE))
    {
        /* pass it on to the physical device driver */
        Result = (pDevice->pEntryPoint->adi_pdd_Control) (pDevice->PDDHandle, CommandID, Value);
    }

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return (Result);
}

/*********************************************************************
*
*   Function:       adi_dev_GetLibraryDetails
*
*   Description:    Get details about this library
*
*********************************************************************/

u32 adi_dev_GetLibraryDetails(ADI_DEV_LIBRARY_DETAILS *pLibraryDetails)
{
    ADI_DEV_LIBRARY_DETAILS *p;

    p = pLibraryDetails;

#if defined(__ADSP_DELTA__)
    p->Processor = ADI_DEV_LIBRARY_PROC_BF592;
#elif defined(__ADSP_MOY__)
    p->Processor = ADI_DEV_LIBRARY_PROC_BF506F;
#elif defined(__ADSP_BRODIE__)
    p->Processor = ADI_DEV_LIBRARY_PROC_BF512;
#elif defined(__ADSPBF526__)
    p->Processor = ADI_DEV_LIBRARY_PROC_BF526;
#elif defined(__ADSPBF527__)
    p->Processor = ADI_DEV_LIBRARY_PROC_BF527;
#elif defined(__ADSPBF533__)
    p->Processor = ADI_DEV_LIBRARY_PROC_BF533;
#elif defined(__ADSPBF537__)
    p->Processor = ADI_DEV_LIBRARY_PROC_BF537;
#elif defined(__ADSPBF538__)
    p->Processor = ADI_DEV_LIBRARY_PROC_BF538;
#elif defined(__ADSPBF548__) || defined(__ADSPBF548M__)
    p->Processor = ADI_DEV_LIBRARY_PROC_BF548;
#elif defined(__ADSPBF561__)
    p->Processor = ADI_DEV_LIBRARY_PROC_BF561;
#else
    p->Processor = ADI_DEV_LIBRARY_PROC_UNKNOWN;
#endif


#if defined(ADI_DEV_DEBUG)
    p->Config = ADI_DEV_LIBRARY_CONFIG_DEBUG;
#else
    p->Config = ADI_DEV_LIBRARY_CONFIG_RELEASE;
#endif

#if defined(ADI_DEV_AUTO)
    p->ChipRev = ADI_DEV_LIBRARY_CHIP_REV_AUTO;
#else
#if defined(ADI_DEV_CHIP_REV)
    p->ChipRev = ADI_DEV_CHIP_REV;
#else
    p->ChipRev = ADI_DEV_LIBRARY_CHIP_REV_NONE;         /* since 0 is a valid chip rev, we need a special value */
#endif  /* not ADI_DEV_CHIP_REV */
#endif  /* not ADI_DEV_AUTO */

#if defined(ADI_DEV_WORKAROUNDS)
    p->Workarounds = ADI_DEV_LIBRARY_WRKRNDS_ENABLED;
#else
    p->Workarounds = ADI_DEV_LIBRARY_WRKRNDS_DISABLED;
#endif

    return ADI_DEV_RESULT_SUCCESS;
}

/*********************************************************************

    Function:       PDDCallback

    Description:    Callback function for physical driver events
                    Note that this function always has to check for deferred
                    callbacks as we don't give the PDD drivers the DCB handle
                    anymore but always pass them NULL for the DCB handle.
                    By doing this, we can simplify the PDD drivers so they
                    always make live callbacks and we only need to check for
                    deferred callbacks here.

*********************************************************************/
/* callback function for PDD events */
static void PDDCallback(
    ADI_DEV_DEVICE_HANDLE       DeviceHandle,   /* Handle to the device that generated callback */
    u32                         Event,          /* Callback event                               */
    void                        *pArg           /* Callback argument                            */
){

    ADI_DEV_DEVICE              *pDevice;       /* pointer to the device we're working on */

    /* avoid casts */
    pDevice = (ADI_DEV_DEVICE *)DeviceHandle;

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
    /* Validate Device Handle */
    if (ValidateDeviceHandle(DeviceHandle) == ADI_DEV_RESULT_SUCCESS)
    {
#endif

    /* call the applications callback function */
    if (pDevice->DCBHandle)
    {
        adi_dcb_Post(pDevice->DCBHandle, 0, pDevice->ClientCallback, pDevice->ClientHandle, Event, pArg);
    }
    else
    {
        (pDevice->ClientCallback) (pDevice->ClientHandle, Event, pArg);
    }

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
}

/*********************************************************************

    Function:       DMACallback

    Description:    Callback function for DMA events.
                    Note that this function never has to check for deferred
                    callbacks but can always call the client directly.  The
                    reason for this is that the DMA Manager is passed the
                    callback service handle so when this routine gets invoked
                    we're already deferred.

*********************************************************************/
static void DMACallback(
    void                        *Ptr1,          /* Handle to the device that generated callback */
    u32                         Value1,         /* DMA Callback event */
    void                        *pArg           /* Callback argument */
){

    ADI_DEV_DEVICE              *pDevice;           /* pointer to the device we're working on */
    ADI_DMA_EVENT               Event;              /* DMA event */
    ADI_DEV_BUFFER              *pBuffer;           /* pointer to a buffer */
    void                        *CallbackParameter; /* argument pointer that is passed to callback */
#if defined(ADI_DEV_LARGE)
    ADI_DMA_DESCRIPTOR_LARGE    *pDescriptor;       /* pointer to a DMA descriptor */
#endif
#if defined(ADI_DEV_SMALL)
    ADI_DMA_DESCRIPTOR_SMALL    *pDescriptor;       /* pointer to a DMA descriptor */
#endif

    /* avoid casts */
    pDevice = (ADI_DEV_DEVICE *)Ptr1;
    Event   = (ADI_DMA_EVENT)Value1;

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
    /* Validate Device Handle */
    if (ValidateDeviceHandle(pDevice) == ADI_DEV_RESULT_SUCCESS)
    {
#endif

    /* CASEOF (event type) */
    switch (Event)
    {
        /* CASE (descriptor was processed) */
        case ADI_DMA_EVENT_DESCRIPTOR_PROCESSED:

            /* avoid casts since we know the descriptor is the start of the buffer regardless of the buffer type */
            pBuffer = (ADI_DEV_BUFFER *)pArg;
#if defined(ADI_DEV_LARGE)
            pDescriptor = (ADI_DMA_DESCRIPTOR_LARGE *)pArg;
#endif
#if defined(ADI_DEV_SMALL)
            pDescriptor = (ADI_DMA_DESCRIPTOR_SMALL *)pArg;
#endif

            /* mark it processed and update the processed data count */
            if (pDescriptor->Config.b_DMA2D == ADI_DMA_DMA2D_LINEAR)
            {
                pBuffer->OneD.ProcessedFlag = TRUE;
                pBuffer->OneD.ProcessedElementCount = pBuffer->OneD.ElementCount;
                CallbackParameter = pBuffer->OneD.CallbackParameter;
            }
            else
            {
                pBuffer->TwoD.ProcessedFlag = TRUE;
                pBuffer->TwoD.ProcessedElementCount = pBuffer->TwoD.XCount * pBuffer->TwoD.YCount;
                CallbackParameter = pBuffer->TwoD.CallbackParameter;
            }

            /* call the applications callback function unless the device is in synchronous mode */
            /* callback parameters are Client Handle, ADI_DEV_BUFFER_PROCESSED, CallbackParameter) */
            if (pDevice->SynchronousFlag == FALSE)
            {
                (pDevice->ClientCallback) (pDevice->ClientHandle, ADI_DEV_EVENT_BUFFER_PROCESSED, CallbackParameter);
            }
            break;

        /* CASE (circular buffer inner loop processed) */
        case ADI_DMA_EVENT_INNER_LOOP_PROCESSED:

            /* tell the application about the event  */
            /* callback parameters are Client Handle, DEV_SUB_BUFFER_PROCESSED, Buffer address */
            (pDevice->ClientCallback) (pDevice->ClientHandle, ADI_DEV_EVENT_SUB_BUFFER_PROCESSED, pArg);
            break;

        /* CASE (circular buffer outer loop processed) */
        case ADI_DMA_EVENT_OUTER_LOOP_PROCESSED:

            /* tell the application about the event  */
            /* callback parameters are Client Handle, ADI_DEV_BUFFER_PROCESSED, Buffer address */
            (pDevice->ClientCallback) (pDevice->ClientHandle, ADI_DEV_EVENT_BUFFER_PROCESSED, pArg);
            break;

        /* CASE (DMA error) */
        case ADI_DMA_EVENT_ERROR_INTERRUPT:

            /* tell the application about the event */
            /* callback parameters are Client Handle, ADI_DEV_RESULT_DMA_ERROR_INTERRUPT, NULL */
            (pDevice->ClientCallback) (pDevice->ClientHandle, ADI_DEV_EVENT_DMA_ERROR_INTERRUPT, NULL);
            break;

    /* ENDCASE */
    }

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
}

/*********************************************************************
*
*   Function:       PrepareBufferList
*
*   Description:    Prepares a single or list of buffers for submission to
*                   the physical device driver or the DMA manager.  All
*                   fields within the ADI_DEV_BUFFER structure and the embedded
*                   DMA descriptor fields are set up appropriately.  The
*                   function returns the last buffer in the list of buffer.
*
*********************************************************************/

static u32 PrepareBufferList(
    ADI_DEV_DEVICE              *pDevice,       /* pointer to the device we're working on */
    ADI_DEV_BUFFER_TYPE         BufferType,     /* type of Buffer */
    ADI_DEV_BUFFER              *pBuffer,       /* pointer to the start of buffer chain */
    ADI_DEV_DIRECTION           Direction,      /* Receive/Transmit data */
    ADI_DEV_BUFFER              **pLastBuffer   /* pointer to the last buffer in the given buffer chain */
){

    ADI_DEV_BUFFER              *pWorkingBuffer;    /* pointer to the buffer we're currently working on */
    ADI_DMA_WNR                 wnr;                /* value of the direction field within the DMA config register */
#if defined(ADI_DEV_LARGE)
    ADI_DMA_DESCRIPTOR_LARGE    *pDescriptor;       /* pointer to the descriptor within the buffer */
#endif
#if defined(ADI_DEV_SMALL)
    ADI_DMA_DESCRIPTOR_SMALL    *pDescriptor;       /* pointer to the descriptor within the buffer */
#endif
    u32                         Result;             /* return code */

    /* assume we're going to be successful */
    Result = ADI_DEV_RESULT_SUCCESS;

    /* set the value of wnr for the config register */
    /* if using sequential IO this test doesn't do any harm but is meaningless as
       direction is set on a buffer by buffer basis */
    if (Direction == ADI_DEV_DIRECTION_INBOUND)
    {
        wnr = ADI_DMA_WNR_WRITE;
    }
    else
    {
        wnr = ADI_DMA_WNR_READ;
    }

    /* CASEOF (buffer type) */
    switch (BufferType)
    {
        /* CASE (circular buffer) */
        case ADI_DEV_CIRC:

            /* update the device's config register based on the circular buffer */
            pDevice->Config.b_DI_EN = ADI_DMA_DI_EN_ENABLE;
            switch (pBuffer->Circular.CallbackType)
            {
                case ADI_DEV_CIRC_NO_CALLBACK:
                    pDevice->Config.b_DI_EN = ADI_DMA_DI_EN_DISABLE;
                    break;
                case ADI_DEV_CIRC_SUB_BUFFER:
                    pDevice->Config.b_DI_SEL = ADI_DMA_DI_SEL_INNER_LOOP;
                    break;
                case ADI_DEV_CIRC_FULL_BUFFER:
                    pDevice->Config.b_DI_SEL = ADI_DMA_DI_SEL_OUTER_LOOP;
                    break;
            }
            pDevice->Config.b_WNR = wnr;
            break;

        /* CASE (1D buffer) */
        case ADI_DEV_1D:

            /* FOR each buffer in the list */
            for (pWorkingBuffer = pBuffer; pWorkingBuffer != NULL; pWorkingBuffer = (ADI_DEV_BUFFER *)pWorkingBuffer->OneD.pNext)
            {
                /* reset the processed fields */
                pWorkingBuffer->OneD.ProcessedFlag = FALSE;
                pWorkingBuffer->OneD.ProcessedElementCount = 0;

                /* IF (device is using peripheral DMA) */
                if (pDevice->PeripheralDMAFlag == TRUE)
                {
                    /* point to the descriptor  */
#if defined(ADI_DEV_LARGE)
                    pDescriptor = (ADI_DMA_DESCRIPTOR_LARGE *)pWorkingBuffer->OneD.Reserved;
#endif
#if defined(ADI_DEV_SMALL)
                    pDescriptor = (ADI_DMA_DESCRIPTOR_SMALL *)pWorkingBuffer->OneD.Reserved;
#endif

                    /* copy the config register from our device's master copy and set the
                       word size, direction and transfer type according to the buffer */
                    pDescriptor->Config = pDevice->Config;
                    pDescriptor->Config.b_WDSIZE = pWorkingBuffer->OneD.ElementWidth >> 1;
                    pDescriptor->Config.b_WNR = wnr;
                    pDescriptor->Config.b_DMA2D = ADI_DMA_DMA2D_LINEAR;

                    /* set the callback flag in the descriptor based upon the buffer's callback parameter value */
                    if (pWorkingBuffer->OneD.CallbackParameter == NULL)
                    {
                        pDescriptor->CallbackFlag = FALSE;
                    }
                    else
                    {
                        pDescriptor->CallbackFlag = TRUE;
                    }

                    /* set the descriptor values */
                    ADI_DEV_SET_DESCRIPTOR_START_ADDRESS(pWorkingBuffer,pWorkingBuffer->OneD.Data);
                    pDescriptor->XCount = pWorkingBuffer->OneD.ElementCount;
                    pDescriptor->XModify = pWorkingBuffer->OneD.ElementWidth;

                    /* update the descriptor to point to the next descriptor in the chain */
                    if (pWorkingBuffer->OneD.pNext == NULL)
                    {
#if defined(ADI_DEV_LARGE)
                        pDescriptor->pNext = NULL;
#endif
#if defined(ADI_DEV_SMALL)
                        pDescriptor->pNext = 0;
#endif
                    }
                    else
                    {
#if defined(ADI_DEV_LARGE)
                        pDescriptor->pNext = (ADI_DMA_DESCRIPTOR_LARGE *)pWorkingBuffer->OneD.pNext->Reserved;
#endif
#if defined(ADI_DEV_SMALL)
                        pDescriptor->pNext = (u16)(u32)(ADI_DMA_DESCRIPTOR_SMALL *)pWorkingBuffer->OneD.pNext->Reserved;
#endif
                    }
                /* ENDIF */
                }

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
                /* verify that the buffer list chain is NULL terminated */
                if ((ADI_DEV_BUFFER *)pWorkingBuffer->OneD.pNext == pBuffer)
                {
                    Result = ADI_DEV_RESULT_NON_TERMINATED_LIST;
                    break;
                }
#endif

                /* update the pointer to the last buffer */
                *pLastBuffer = pWorkingBuffer;
            /* ENDFOR */
            }

            /* make sure the last buffer has a callback enabled if synchronous and using peripheral dma */
            if ((pDevice->SynchronousFlag == TRUE) && (pDevice->PeripheralDMAFlag == TRUE))
            {
                (*pLastBuffer)->OneD.CallbackParameter = *pLastBuffer;
            }
            break;

        /* CASE (2D buffer) */
        case ADI_DEV_2D:

            /* FOR each buffer in the list */
            for (pWorkingBuffer = pBuffer; pWorkingBuffer != NULL; pWorkingBuffer = (ADI_DEV_BUFFER *)pWorkingBuffer->TwoD.pNext)
            {
                /* reset the processed fields */
                pWorkingBuffer->TwoD.ProcessedFlag = FALSE;
                pWorkingBuffer->TwoD.ProcessedElementCount = 0;

                /* IF (device is using peripheral DMA)  */
                if (pDevice->PeripheralDMAFlag == TRUE)
                {
                    /* point to the descriptor */
#if defined(ADI_DEV_LARGE)
                    pDescriptor = (ADI_DMA_DESCRIPTOR_LARGE *)pWorkingBuffer->TwoD.Reserved;
#endif
#if defined(ADI_DEV_SMALL)
                    pDescriptor = (ADI_DMA_DESCRIPTOR_SMALL *)pWorkingBuffer->TwoD.Reserved;
#endif

                    /* copy the config register from our device's master copy and set the
                       word size, direction and transfer type according to the buffer */
                    pDescriptor->Config = pDevice->Config;
                    pDescriptor->Config.b_WDSIZE = pWorkingBuffer->TwoD.ElementWidth >> 1;
                    pDescriptor->Config.b_WNR = wnr;
                    pDescriptor->Config.b_DMA2D = ADI_DMA_DMA2D_2D;

                    /* set the callback flag in the descriptor based upon the buffer's callback parameter value */
                    if (pWorkingBuffer->TwoD.CallbackParameter == NULL)
                    {
                        pDescriptor->CallbackFlag = FALSE;
                    }
                    else
                    {
                        pDescriptor->CallbackFlag = TRUE;
                    }

                    /* set the descriptor values */
                    ADI_DEV_SET_DESCRIPTOR_START_ADDRESS(pWorkingBuffer,pWorkingBuffer->TwoD.Data);
                    pDescriptor->XCount = pWorkingBuffer->TwoD.XCount;
                    pDescriptor->XModify = pWorkingBuffer->TwoD.XModify;
                    pDescriptor->YCount = pWorkingBuffer->TwoD.YCount;
                    pDescriptor->YModify = pWorkingBuffer->TwoD.YModify;

                    /* update the descriptor to point to the next descriptor in the chain */
                    if (pWorkingBuffer->TwoD.pNext == NULL)
                    {
#if defined(ADI_DEV_LARGE)
                        pDescriptor->pNext = NULL;
#endif
#if defined(ADI_DEV_SMALL)
                        pDescriptor->pNext = 0;
#endif
                    }
                    else
                    {
#if defined(ADI_DEV_LARGE)
                        pDescriptor->pNext = (ADI_DMA_DESCRIPTOR_LARGE *)pWorkingBuffer->TwoD.pNext->Reserved;
#endif
#if defined(ADI_DEV_SMALL)
                        pDescriptor->pNext = (u16)(u32)(ADI_DMA_DESCRIPTOR_SMALL *)pWorkingBuffer->TwoD.pNext->Reserved;
#endif
                    }
                /* ENDIF */
                }

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
                /* verify that the buffer list chain is NULL terminated */
                if ((ADI_DEV_BUFFER *)pWorkingBuffer->TwoD.pNext == pBuffer)
                {
                    Result = ADI_DEV_RESULT_NON_TERMINATED_LIST;
                    break;
                }
#endif

                /* update the pointer to the last buffer */
                *pLastBuffer = pWorkingBuffer;

            /* ENDFOR */
            }

            /* make sure the last buffer has a callback enabled if synchronous and using peripheral dma */
            if ((pDevice->SynchronousFlag == TRUE) && (pDevice->PeripheralDMAFlag == TRUE))
            {
                (*pLastBuffer)->TwoD.CallbackParameter = *pLastBuffer;
            }
            break;

        /* CASE (sequential 1D buffer) */
        case ADI_DEV_SEQ_1D:

            /* FOR each buffer in the list */
            for (pWorkingBuffer = pBuffer; pWorkingBuffer != NULL; pWorkingBuffer = (ADI_DEV_BUFFER *)pWorkingBuffer->OneD.pNext)
            {
                /* reset the processed fields */
                pWorkingBuffer->Seq1D.Buffer.ProcessedFlag = FALSE;
                pWorkingBuffer->Seq1D.Buffer.ProcessedElementCount = 0;

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
                /* verify that the direction is set properly */
                if ((pWorkingBuffer->Seq1D.Direction != ADI_DEV_DIRECTION_INBOUND) && (pWorkingBuffer->Seq1D.Direction != ADI_DEV_DIRECTION_OUTBOUND))
                {
                    Result = ADI_DEV_RESULT_BAD_DIRECTION_FIELD;
                    break;
                }
#endif

                /* IF (device is using peripheral DMA)  */
                if (pDevice->PeripheralDMAFlag == TRUE)
                {
                    /* point to the descriptor */
#if defined(ADI_DEV_LARGE)
                    pDescriptor = (ADI_DMA_DESCRIPTOR_LARGE *)pWorkingBuffer->Seq1D.Buffer.Reserved;
#endif
#if defined(ADI_DEV_SMALL)
                    pDescriptor = (ADI_DMA_DESCRIPTOR_SMALL *)pWorkingBuffer->Seq1D.Buffer.Reserved;
#endif

                    /* copy the config register from our device's master copy and set the
                       word size, direction and transfer type according to the buffer */
                    pDescriptor->Config = pDevice->Config;
                    pDescriptor->Config.b_WDSIZE = pWorkingBuffer->Seq1D.Buffer.ElementWidth >> 1;
                    pDescriptor->Config.b_WNR = ((pWorkingBuffer->Seq1D.Direction == ADI_DEV_DIRECTION_INBOUND)?ADI_DMA_WNR_WRITE:ADI_DMA_WNR_READ);
                    pDescriptor->Config.b_DMA2D = ADI_DMA_DMA2D_LINEAR;

                    /* set the callback flag in the descriptor based upon the buffer's callback parameter value */
                    if (pWorkingBuffer->Seq1D.Buffer.CallbackParameter == NULL)
                    {
                        pDescriptor->CallbackFlag = FALSE;
                    }
                    else
                    {
                        pDescriptor->CallbackFlag = TRUE;
                    }

                    /* set the descriptor values */
                    ADI_DEV_SET_DESCRIPTOR_START_ADDRESS(pWorkingBuffer,pWorkingBuffer->Seq1D.Buffer.Data);
                    pDescriptor->XCount = pWorkingBuffer->Seq1D.Buffer.ElementCount;
                    pDescriptor->XModify = pWorkingBuffer->Seq1D.Buffer.ElementWidth;

                    /* update the descriptor to point to the next descriptor in the chain */
                    if (pWorkingBuffer->Seq1D.Buffer.pNext == NULL)
                    {
#if defined(ADI_DEV_LARGE)
                        pDescriptor->pNext = NULL;
#endif
#if defined(ADI_DEV_SMALL)
                        pDescriptor->pNext = 0;
#endif
                    }
                    else
                    {
#if defined(ADI_DEV_LARGE)
                        pDescriptor->pNext = (ADI_DMA_DESCRIPTOR_LARGE *)pWorkingBuffer->Seq1D.Buffer.pNext->Reserved;
#endif
#if defined(ADI_DEV_SMALL)
                        pDescriptor->pNext = (u16)(u32)(ADI_DMA_DESCRIPTOR_SMALL *)pWorkingBuffer->Seq1D.Buffer.pNext->Reserved;
#endif
                    }
                /* ENDIF */
                }

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
                /* verify that the buffer list chain is NULL terminated */
                if ((ADI_DEV_BUFFER *)pWorkingBuffer->Seq1D.Buffer.pNext == pBuffer)
                {
                    Result = ADI_DEV_RESULT_NON_TERMINATED_LIST;
                    break;
                }
#endif

                /* update the pointer to the last buffer */
                *pLastBuffer = pWorkingBuffer;

            /* ENDFOR */
            }

            /* make sure the last buffer has a callback enabled if synchronous and using peripheral dma */
            if ((pDevice->SynchronousFlag == TRUE) && (pDevice->PeripheralDMAFlag == TRUE))
            {
                (*pLastBuffer)->Seq1D.Buffer.CallbackParameter = *pLastBuffer;
            }
            break;

        /* CASE (skip this buffer) */
        case (ADI_DEV_BUFFER_SKIP):
        /* CASE (Switch buffer) */
        case (ADI_DEV_SWITCH):
        /* CASE (Update switch buffer) */
        case (ADI_DEV_UPDATE_SWITCH):
            /* do nothing */
            break;

    /* ENDCASE */
    }

    /* return */
    return(Result);
}

/*********************************************************************
*
*   Function:       SetDataflow
*
*   Description:    Turns dataflow on or off depending on the Flag.
*
*********************************************************************/

static u32 SetDataflow (
    ADI_DEV_DEVICE      *pDevice,       /* pointer to the device we're working on */
    u32                 Flag            /* Dataflow flag (TRUE to enable dataflow)*/
){

    u32                 Result = ADI_DEV_RESULT_SUCCESS;        /* return value */
    ADI_DEV_DMA_INFO    *pDmaInfo;  /* pointer to DMA channel information table of the device we're working on */

    /* do nothing if we're not really changing the status of dataflow */
    if (((pDevice->DataflowFlag == TRUE) && (Flag == TRUE)) ||
        ((pDevice->DataflowFlag == FALSE) && (Flag == FALSE)))
    {
        /* Do nothing*/
    }
    /* else if disabling dataflow, shut down the device first and then shut down DMA (all DMA channels if necessary) */

    /* NOTE: the above comment was the original preferred sequence.
       For anomaly 05000278, we would change the default behavior to disabling the DMA
       first, then the peripheral.*/

    /* Mar 2, 2007 - This workaround was removed--
       See CVS log, file rev 1.31,  Feb 5, 2008
       for details of why workaround was removed.  */

//#define ADI_DEV_ENABLE_WORKAROUND_FOR_05000278
#if defined(ADI_DEV_ENABLE_WORKAROUND_FOR_05000278)
    else if (Flag == FALSE)
    {
        /* shut down all DMA channels used by this device */
        /* Disable dataflow of all Inbound DMA channels used by this device */
        if ((pDevice->Direction == ADI_DEV_DIRECTION_INBOUND) || (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL))
        {
            for (pDmaInfo=pDevice->pInboundDma; pDmaInfo; pDmaInfo = pDmaInfo->pNext )
            {
                if ((Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)Flag)) != ADI_DMA_RESULT_SUCCESS)
                {
                    break;
                }
            }
        }

        /* shutdown Outbound DMA channels */
        if ((Result == ADI_DEV_RESULT_SUCCESS) &&
            ((pDevice->Direction == ADI_DEV_DIRECTION_OUTBOUND) || (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)))
        {
            /* Disable dataflow of all Outbound DMA channels used by this device */
            for (pDmaInfo=pDevice->pOutboundDma; pDmaInfo; pDmaInfo = pDmaInfo->pNext )
            {
                if ((Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)Flag)) != ADI_DMA_RESULT_SUCCESS)
                {
                    break;
                }
            }
        }

        /* shut down the physical device and update the Dataflow flag*/
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            if ((Result = (pDevice->pEntryPoint->adi_pdd_Control) (pDevice->PDDHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)Flag)) == ADI_DEV_RESULT_SUCCESS)
            {
                pDevice->DataflowFlag = FALSE;
            }
        }
    }
#else
    else if (Flag == FALSE)
    {
        /* shut down the physical device */
        Result = (pDevice->pEntryPoint->adi_pdd_Control) (pDevice->PDDHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)Flag);

        /* shut down all DMA channels used by this device */
        /* Disable dataflow of all Inbound DMA channels used by this device */
        if ((Result == ADI_DEV_RESULT_SUCCESS) &&
            ((pDevice->Direction == ADI_DEV_DIRECTION_INBOUND) || (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)))
        {
            for (pDmaInfo=pDevice->pInboundDma; pDmaInfo; pDmaInfo = pDmaInfo->pNext )
            {
                if ((Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)Flag)) != ADI_DMA_RESULT_SUCCESS)
                {
                    break;
                }
            }
        }

        /* shutdown Outbound DMA channels */
        if ((Result == ADI_DEV_RESULT_SUCCESS) &&
            ((pDevice->Direction == ADI_DEV_DIRECTION_OUTBOUND) || (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)))
        {
            /* Disable dataflow of all Outbound DMA channels used by this device */
            for (pDmaInfo=pDevice->pOutboundDma; pDmaInfo; pDmaInfo = pDmaInfo->pNext )
            {
                if ((Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)Flag)) != ADI_DMA_RESULT_SUCCESS)
                {
                    break;
                }
            }
        }

        /* update the Dataflow flag */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            pDevice->DataflowFlag = FALSE;
        }
    }
#endif
    /* ELSE (we now know we're enabling dataflow on a device that supports peripheral dma) */
    else
    {
        /* startup the DMA channel(s) if necessary */
        if ((pDevice->Direction == ADI_DEV_DIRECTION_INBOUND) || (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL))
        {
            /* Enable dataflow of all Inbound DMA channels used by this device */
            for (pDmaInfo=pDevice->pInboundDma; pDmaInfo; pDmaInfo = pDmaInfo->pNext )
            {
                if ((Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)Flag)) != ADI_DMA_RESULT_SUCCESS)
                {
                    break;
                }
            }
        }

        /* enable dataflow for outbound DMA channels */
        if ((Result == ADI_DEV_RESULT_SUCCESS) &&
            ((pDevice->Direction == ADI_DEV_DIRECTION_OUTBOUND) || (pDevice->Direction == ADI_DEV_DIRECTION_BIDIRECTIONAL)))
        {
            /* Enable dataflow of all Outbound DMA channels used by this device */
            for (pDmaInfo=pDevice->pOutboundDma;pDmaInfo; pDmaInfo = pDmaInfo->pNext )
            {
                if ((Result = adi_dma_Control(pDmaInfo->ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)Flag)) != ADI_DMA_RESULT_SUCCESS)
                {
                    break;
                }
            }
        }

        /* tell the physical device to enable dataflow */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            if ((Result = (pDevice->pEntryPoint->adi_pdd_Control) (pDevice->PDDHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)Flag)) == ADI_DEV_RESULT_SUCCESS)
            {
                pDevice->DataflowFlag = TRUE;
            }
        }
    }

    /* return */
    return (Result);
}

/* Debug Routines - Debug Build only */
#if defined(ADI_DEV_DEBUG)

/*********************************************************************

    Function:       ValidateDeviceHandle

    Description:    Attempts to verify the device handle is valid

*********************************************************************/
static u32 ValidateDeviceHandle(
    ADI_DEV_DEVICE_HANDLE       Handle          /* Device handle to vaildate */
) {

    ADI_DEV_DEVICE  *pDevice;
    u32             Result = ADI_DEV_RESULT_SUCCESS;

    /* avoid casts */
    pDevice = (ADI_DEV_DEVICE *)Handle;

    /* make sure the device handle is not NULL */
    if (pDevice == NULL)
    {
        Result = ADI_DEV_RESULT_BAD_DEVICE_HANDLE;
    }
    else
    {
        /* verify the manager address */
        Result = ValidateManagerHandle(pDevice->pManager);
    }

    /* return */
    return (Result);
}


/*********************************************************************

    Function:       ValidateManagerHandle

    Description:    Attempts to verify the manager handle is valid

*********************************************************************/
static u32 ValidateManagerHandle(
    ADI_DEV_MANAGER_HANDLE      Handle          /* Device Manager handle to validate */
){

    ADI_DEV_MANAGER     *pManager;
    u32                 Result = ADI_DEV_RESULT_SUCCESS;

    /* avoid casts */
    pManager = (ADI_DEV_MANAGER *)Handle;

    /* at least make it's not NULL */
    if (pManager == NULL)
    {
        Result = ADI_DEV_RESULT_BAD_MANAGER_HANDLE;
    }

    /* return */
    return(Result);
}

#endif  /* End of Debug Routines - Debug Build only */

/*****/

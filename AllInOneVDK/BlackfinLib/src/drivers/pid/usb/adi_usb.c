/*********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_usb.c,v $
$Revision: 3459 $
$Date: 2010-09-29 10:37:28 -0400 (Wed, 29 Sep 2010) $

Description:
            PID Device driver for USB Host Mass Storage.

********************************************************************************
*/
#include <services/services.h>                                          /* system service includes */
#if defined(__ADSP_KOOKABURRA__) && defined(__SILICON_REVISION__) && ( (__SILICON_REVISION__ <= 0x01) || (__SILICON_REVISION__ == 0xFFFF))
#define _BF527_SDRAM_ISSUE_WORKAROUND
#endif

#define _USE_BACKG_XFER

#include <services/fss/adi_fss.h>                                       /* File System Service includes */
#include <drivers/adi_dev.h>                                            /* device manager includes */
#include <drivers/pid/adi_ata.h>                                        /* ATA structures, comamnds and macros */
#include <drivers/pid/usb/adi_usb.h>                                        /* ATA structures, comamnds and macros */
#include <drivers/usb/usb_core/adi_usb_core.h>                          /* USB Core */

#include <drivers/usb/class/otg/mass_storage/adi_usb_msd_class_host.h>  /* MSD Class Host include */

#include <string.h>
#include <stdio.h>

#include <ccblkfn.h>
#include <cplb.h>

#include <drivers/usb/controller/otg/adi/hdrc/adi_usb_hdrc.h>




typedef struct ADI_USB_DEF {
    ADI_DEV_MANAGER_HANDLE  ManagerHandle;          /* device manager handle                                        */
    ADI_DMA_MANAGER_HANDLE  DMAHandle;              /* handle to the DMA manager                                    */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle;           /* device manager handle                                        */
    u32                     DeviceNumber;           /* Device Number                                                */
    ADI_DEV_DIRECTION       Direction;              /* Direction of Device Driver                                   */
    ADI_DCB_HANDLE          DCBHandle;              /* callback manager handle                                      */
    ADI_DCB_CALLBACK_FN     DMCallback;             /* the callback function supplied by the Device Manager         */
    void                    *pEnterCriticalArg;     /* critical region argument                                     */
    int                     CacheHeapID;            /* Heap Index for Device transfer buffers                       */
    ADI_FSS_LBA_REQUEST     CurrentLBARequest;      /* The current LBA request being processed                      */
    ADI_SEM_HANDLE          DataSemaphoreHandle;    /* Semaphore for Internal data transfers                        */
    ADI_SEM_HANDLE          LockSemaphoreHandle;    /* Semaphore for Lock Semaphore operation                       */
    ADI_DEV_DEVICE_HANDLE   ControllerHandle;       /* Device handle of USB controller driver                       */
    ADI_DEV_DEVICE_HANDLE   ClassDriverHandle;      /* Device handle of USB Mass Storage Class Driver               */
    u32                     UseDefaultController;   /* Flag to indicate that the default controller is used         */
    u32                     UseDefaultClassDriver;   /* Flag to indicate that the default controller is used         */
    u32                     MediaPresent;           /* Flag to indicate whether media is available                  */
    u32                     ClassCommandComplete;   /* Flag to indicate whether class driver command is completed   */

    u32                     SCSICommandError;       /* Flag to indicate SCSI command error */

    SCSI_CAPACITY_10_DEF       CapacityTen;
    
#if defined(_BF527_SDRAM_ISSUE_WORKAROUND)
    ADI_DMA_STREAM_HANDLE   MemDMAStreamHandle;     /* Memory DMA Stream Handle for copies from internal buffer     */
#endif
} ADI_USB_DEF;

#if defined(_BF527_SDRAM_ISSUE_WORKAROUND)
static ADI_SEM_HANDLE MemDmaSemaphoreHandle;    /* Semaphore for Internal data transfers                        */
static void MemDmaCallback( void *AppHandle, u32 Event, void *pArg );
#endif

#define PDD_DEFAULT_SECTOR_SIZE     512
#define ADI_USB_GEN_HEAPID          (-1)

#ifdef ENABLE
#undef ENABLE
#endif
#define ENABLE  1

#ifdef DISABLE
#undef DISABLE
#endif
#define DISABLE 0

#ifdef READ
#undef READ
#endif
#define READ 1

#ifdef WRITE
#undef WRITE
#endif
#define WRITE 0

#ifdef BLOCK
#undef BLOCK
#endif
#define BLOCK 1

#ifdef NOBLOCK
#undef NOBLOCK
#endif
#define NOBLOCK 0


/* ***************************************************************************
 * Device Driver Model entry point functions
 * ***************************************************************************
 */
static u32 adi_pdd_Open(    /* Open a device instance */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,          /* device manager handle                    */
    u32                     DeviceNumber,           /* device number                            */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,           /* device handle                            */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,            /* pointer to PDD handle location           */
    ADI_DEV_DIRECTION       Direction,              /* data direction                           */
    void                    *pCriticalRegionArg,    /* critical region imask storage location   */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,              /* handle to the DMA manager                */
    ADI_DCB_HANDLE          DCBHandle,              /* callback handle                          */
    ADI_DCB_CALLBACK_FN     DMCallback              /* device manager callback function         */
);

static u32 adi_pdd_Close(       /* Closes a device instance */
    ADI_DEV_PDD_HANDLE PDDHandle    /* PDD handle */
);

static u32 adi_pdd_Read(        /* Reads data or queues an inbound buffer to a device */
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle           */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type          */
    ADI_DEV_BUFFER *pBuffer         /* pointer to buffer    */
);

static u32 adi_pdd_Write(       // Writes data or queues an outbound buffer to a device
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle           */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type          */
    ADI_DEV_BUFFER *pBuffer         /* pointer to buffer    */
);

static u32 adi_pdd_Control(     // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle               */
    u32 Command,                    /* command ID               */
    void *Value                     /* command specific value   */
);

/* ***************************************************************************
 * Device Driver Model entry point structure
 * ***************************************************************************
 */
ADI_DEV_PDD_ENTRY_POINT ADI_USB_Entrypoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control
};

static void CallbackFromFSS(void *pHandle, u32 Event, void *pArg);
static void ClassDriverCallback(void *pHandle, u32 Event, void *pArg);
static void ControllerCallback(void *pHandle, u32 Event, void *pArg);

static ADI_DCB_CALLBACK_FN FSSDirectCallbackFunction=NULL;
static ADI_DCB_CALLBACK_FN MediaDetectionCallbackFunction=NULL;

static u32  Activate(ADI_USB_DEF *pDevice, u32 EnableFlag);
static u32  DevicePollMedia(ADI_USB_DEF *pDevice);
static u32  DetectVolumes(ADI_USB_DEF *pDevice, u32 Drive);
//static u32 TransferSectors(ADI_USB_DEF *pDevice, u32 Drive, u32 SectorNumber, u32 SectorCount, u16 *buf, u32 ReadFlag);
static u32 ProcessExtendedPartitions(ADI_USB_DEF *pDevice, u32 Drive, ADI_ATA_PARTITION_ENTRY *pPartTabEntry, u32 *Count);
static void NewVolumeDef(ADI_USB_DEF *pDevice, u32 Drive, ADI_ATA_PARTITION_ENTRY *pPartTab);
static u32 AssignFileSystemType(u32 type);
static u32 SendLbaRequest(ADI_USB_DEF *pDevice, ADI_FSS_LBA_REQUEST *pLBARequest);
static void WaitOnCommandCompletion(ADI_USB_DEF *pDevice);
static void CheckForSCSICommandFailed(ADI_USB_DEF *pDevice);
static void ConfigureUsbDevice(ADI_USB_DEF *pDevice);
static u32 GetGlobalVolumeDef(ADI_USB_DEF *pDevice, ADI_FSS_VOLUME_DEF *pVolumeDef);

/* ***************************************************************************
 * FSS housekeeping functions
 * ***************************************************************************
 */
extern void *_adi_fss_malloc( int id, size_t size );
extern void _adi_fss_free( int id, void *p );
extern u32 _adi_fss_DetectVolumes( 
                            ADI_DEV_DEVICE_HANDLE   hDevice, 
                            u32                     Drive,
                            int                     HeapID,
                            u32                     SectorSize,
                            u32                     DataElementWidth,
                            ADI_SEM_HANDLE          hSemaphore
);

static ADI_FSS_SUPER_BUFFER *pNextBufferInChain = NULL;


#if defined(_BF527_SDRAM_ISSUE_WORKAROUND)
//#define TEMP_BUF_SIZE 512
//#define TEMP_BUF_SIZE 4096
#define TEMP_BUF_SIZE 8192
#pragma align(4)
static section("L1_data") u8 TempBufferData[TEMP_BUF_SIZE];
#endif

/* ***************************************************************************
 * ***************************************************************************
 * Device Driver Model entry point functions
 * ***************************************************************************
 * ***************************************************************************
 */

/*********************************************************************
*
*   Function:       adi_pdd_Open
*
*   Description:    Opens the Mass Storage Class device
*                   in Device mode
*
*********************************************************************/
static u32 adi_pdd_Open(
    ADI_DEV_MANAGER_HANDLE       ManagerHandle,         /* device manager handle */
    u32                          DeviceNumber,          /* device number */
    ADI_DEV_DEVICE_HANDLE        DeviceHandle,          /* device handle */
    ADI_DEV_PDD_HANDLE           *pPDDHandle,           /* pointer to PDD handle location */
    ADI_DEV_DIRECTION            Direction,             /* data direction */
    void                         *pCriticalRegionArg,   /* critical region imask storage location */
    ADI_DMA_MANAGER_HANDLE       DMAHandle,             /* handle to the DMA manager */
    ADI_DCB_HANDLE               DCBHandle,             /* callback handle */
    ADI_DCB_CALLBACK_FN          DMCallback             /* device manager callback function */
)
{
    u32 i           = 0;
    u32 Result      = ADI_DEV_RESULT_SUCCESS;

    /* Create an instance of the device driver */
    ADI_USB_DEF *pDevice = (ADI_USB_DEF*)_adi_fss_malloc(ADI_USB_GEN_HEAPID,sizeof(ADI_USB_DEF));
    if (!pDevice) {
        Result = ADI_FSS_RESULT_NO_MEMORY;
    }
    else
    {
        /* initialize the Device Header structure */
        pDevice->ManagerHandle = ManagerHandle;
        pDevice->Direction = Direction;
        pDevice->DCBHandle = DCBHandle;
        pDevice->DMCallback = DMCallback;
        pDevice->DeviceHandle = DeviceHandle;
        pDevice->DeviceNumber = DeviceNumber;
        pDevice->pEnterCriticalArg = pCriticalRegionArg;
        pDevice->MediaPresent = FALSE;
#if defined(_BF527_SDRAM_ISSUE_WORKAROUND)
        pDevice->MemDMAStreamHandle = (ADI_DMA_STREAM_HANDLE)NULL;
#endif

        /* Use the on-chip USB controller by default */
        pDevice->UseDefaultController = true;
        pDevice->ControllerHandle = NULL;

        /* Use the ADI USB MSD class driver by default */
        pDevice->UseDefaultClassDriver = true;
        pDevice->ClassDriverHandle = NULL;

        /* Use the General Heap for data buffers by default */
        pDevice->CacheHeapID = ADI_USB_GEN_HEAPID;

        /* save the physical device handle in the client supplied location */
        *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;

        Result = (u32)adi_sem_Create(0,&pDevice->DataSemaphoreHandle, NULL);
#if defined(_BF527_SDRAM_ISSUE_WORKAROUND)
        /* we can use the data semaphore for internal Memory DMA transfers as well */
        MemDmaSemaphoreHandle = pDevice->DataSemaphoreHandle;
#endif

        if (Result == (u32)ADI_SEM_RESULT_SUCCESS)
        {
            Result = (u32)adi_sem_Create(1,&pDevice->LockSemaphoreHandle, NULL);
        }
     }

    return(Result);
}

/*********************************************************************
*
*   Function:       pddClose
*
*   Description:    Closes down a device
*
*********************************************************************/

static u32 adi_pdd_Close(
    ADI_DEV_PDD_HANDLE PDDHandle    /* PDD handle */
)
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    ADI_USB_DEF   *pDevice = (ADI_USB_DEF *)PDDHandle;

    if (pDevice->DataSemaphoreHandle)
    {
        adi_sem_Delete(pDevice->DataSemaphoreHandle);
        adi_sem_Delete(pDevice->LockSemaphoreHandle);
    }

    /* Close the Class Driver */
    if (pDevice->UseDefaultClassDriver)
    {
        Result = adi_dev_Close(pDevice->ClassDriverHandle);
    }
    /* Close the Controller Driver if the driver owns it*/
    if (pDevice->UseDefaultController)
    {
        Result = adi_dev_Close(pDevice->ControllerHandle);
    }

    /* free the device instance */
    _adi_fss_free(ADI_USB_GEN_HEAPID,(void*)pDevice);

    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Read
*
*   Description:    Queue read request buffer with the Class Driver
*
*********************************************************************/

static u32 adi_pdd_Read(
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type */
    ADI_DEV_BUFFER *pBuffer         /* pointer to buffer */
)
{
    u32                         Result;

    /* Expose the Device Definition structure */
    ADI_USB_DEF                 *pDevice = (ADI_USB_DEF *)PDDHandle;

    /* Expose the FSS super buffer structure */
    ADI_FSS_SUPER_BUFFER        *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pBuffer;

    /* Assign callback function and handle to Super Buffer */
    pSuperBuffer->PIDCallbackFunction = CallbackFromFSS;
    pSuperBuffer->PIDCallbackHandle = (void*)pDevice;

#if defined(_BF527_SDRAM_ISSUE_WORKAROUND)
    /* tranfser 'gEndPointMaxPacketSize' bytes at at time & copy to request buffer */
    ADI_DEV_1D_BUFFER  TempBuffer, *pClientBuffer;
    u32                 BytesRemaining = pBuffer->OneD.ElementCount*pBuffer->OneD.ElementWidth;
    u32                 *pClientData   = (u32*)pBuffer->OneD.Data;
    u32                 *pTempData;
    u32                 i;
    u32                 SectorNumber   = pSuperBuffer->LBARequest.StartSector;
    u32                 BufferSize;
    u32                 SectorIncrement;

    if (BytesRemaining< TEMP_BUF_SIZE) {
        BufferSize = BytesRemaining;
    } else {
        BufferSize = TEMP_BUF_SIZE;
    }

    SectorIncrement = BufferSize/512;

    /* change LBA request to 1 sector */
    pSuperBuffer->LBARequest.SectorCount = SectorIncrement;

    /* get max end point buffer size from MSH class driver */
    //adi_dev_Control( pDevice->ClassDriverHandle, ADI_USB_MSD_CMD_GET_MAX_EP_PACKET_SIZE, (void*)&TempBuffer.ElementCount );

    TempBuffer.Data = (void*)&TempBufferData[0];
    TempBuffer.ElementCount = BufferSize;
    TempBuffer.ElementWidth = sizeof(u8);
    TempBuffer.ProcessedElementCount = 0;
    TempBuffer.ProcessedFlag = FALSE;
    TempBuffer.CallbackParameter = &TempBuffer;
    TempBuffer.pNext = NULL;

    pClientBuffer = &pSuperBuffer->Buffer;

    while (pClientBuffer)
    {
        while (BytesRemaining)
        {
            pDevice->ClassCommandComplete = FALSE;
            /* Pass LBA request to the Class Driver */
            SendLbaRequest(pDevice,&pSuperBuffer->LBARequest);
            /* Queue the buffer */
            Result = adi_dev_Read( pDevice->ClassDriverHandle, ADI_DEV_1D, (ADI_DEV_BUFFER*)&TempBuffer );
            if (Result!=ADI_DEV_RESULT_SUCCESS) {
                return Result;
            }

            /* and wait for completion */
            WaitOnCommandCompletion(pDevice);

            pTempData = (u32*)&TempBufferData[0];

            /* If a Memory DMA stream has been registered with the driver then it is used for the
             * transfer between the L1 temporary buffer and the client buffer
            */
            if (pDevice->MemDMAStreamHandle) {
                adi_dma_MemoryCopy(
                                    pDevice->MemDMAStreamHandle,            /* Stream Handle      */
                                    pClientData,                            /* Destination buffer */
                                    pTempData,                              /* Source buffer      */
                                    sizeof(u32),                            /* Element Width      */
                                    (TempBuffer.ElementCount)/sizeof(u32),  /* Number of elements */
                                    MemDmaCallback                          /* Callback function  */
                                    );

                /* pend on completion */
                adi_sem_Pend(MemDmaSemaphoreHandle,ADI_SEM_TIMEOUT_FOREVER);
                pClientData += (TempBuffer.ElementCount)/sizeof(u32);
            }
            /* Otherwise a core copy is used */
            else {
                for(i=0;i<(TempBuffer.ElementCount)/sizeof(u32);i++,pClientData++,pTempData++)
                {
                    *pClientData = *pTempData;
                }
            }

            BytesRemaining -= TempBuffer.ElementCount;
            pSuperBuffer->LBARequest.StartSector += SectorIncrement;
        }

        pClientBuffer = pClientBuffer->pNext;
    }

    (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_DEV_EVENT_BUFFER_PROCESSED, (void *)pBuffer );
    (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_PID_EVENT_DEVICE_INTERRUPT, (void *)pBuffer);


#else
#if defined(_USE_BACKG_XFER)
    /* save aside the next buffer in the chain */
    pNextBufferInChain = (ADI_FSS_SUPER_BUFFER *)pBuffer->OneD.pNext;

    /* and clear it so that the USB driver only deals with a single buffer */
    pBuffer->OneD.pNext = NULL;
#endif

#if defined(_USE_BACKG_XFER)
    /* Pass LBA request to the Class Driver */
    SendLbaRequest(pDevice,&pSuperBuffer->LBARequest);
#endif

    /* Pass on the request to the Class Driver */
    Result = adi_dev_Read( pDevice->ClassDriverHandle, BufferType, pBuffer );
#endif

    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Write
*
*   Description:    Queue write request buffer with the Class Driver
*
*********************************************************************/

static u32 adi_pdd_Write(
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
    ADI_DEV_BUFFER_TYPE BufferType, /* buffer type */
    ADI_DEV_BUFFER *pBuffer         /* pointer to buffer */
)
{
    u32 Result;

    /* Expose the Device Definition structure */
    ADI_USB_DEF          *pDevice      = (ADI_USB_DEF *)PDDHandle;

    /* Expose the FSS super buffer structure */
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pBuffer;

    /* Assign callback function and handle to Super Buffer */
    pSuperBuffer->PIDCallbackFunction = CallbackFromFSS;
    pSuperBuffer->PIDCallbackHandle = (void*)pDevice;

#if defined(_BF527_SDRAM_ISSUE_WORKAROUND)
    /* tranfser 'gEndPointMaxPacketSize' bytes at at time & copy to request buffer */
    ADI_DEV_1D_BUFFER  TempBuffer, *pClientBuffer;
    u32                 BytesRemaining = pBuffer->OneD.ElementCount*pBuffer->OneD.ElementWidth;
    u32                 *pClientData   = (u32*)pBuffer->OneD.Data;
    u32                 *pTempData;
    u32                 i;
    u32                 SectorNumber   = pSuperBuffer->LBARequest.StartSector;
    u32                 BufferSize;
    u32                 SectorIncrement;

    if (BytesRemaining< TEMP_BUF_SIZE) {
        BufferSize = BytesRemaining;
    } else {
        BufferSize = TEMP_BUF_SIZE;
    }

    SectorIncrement = BufferSize/512;

    /* change LBA request to 1 sector */
    pSuperBuffer->LBARequest.SectorCount = SectorIncrement;

    /* get max end point buffer size from MSH class driver */
    //adi_dev_Control( pDevice->ClassDriverHandle, ADI_USB_MSD_CMD_GET_MAX_EP_PACKET_SIZE, (void*)&TempBuffer.ElementCount );

    TempBuffer.Data = (void*)&TempBufferData[0];
    TempBuffer.ElementWidth = sizeof(u8);
    TempBuffer.ElementCount = BufferSize;
    TempBuffer.ProcessedElementCount = 0;
    TempBuffer.ProcessedFlag = FALSE;
    TempBuffer.CallbackParameter = &TempBuffer;
    TempBuffer.pNext = NULL;

    pClientBuffer = &pSuperBuffer->Buffer;


    while (pClientBuffer)
    {
        while (BytesRemaining)
        {
            pTempData = (u32*)&TempBufferData[0];
            /* If a Memory DMA stream has been registered with the driver then it is used for the
             * transfer between the L1 temporary buffer and the client buffer
            */
            if (pDevice->MemDMAStreamHandle) {
                adi_dma_MemoryCopy(
                                    pDevice->MemDMAStreamHandle,            /* Stream Handle      */
                                    pTempData,                              /* Destination buffer      */
                                    pClientData,                            /* Source buffer */
                                    sizeof(u32),                            /* Element Width      */
                                    (TempBuffer.ElementCount)/sizeof(u32),  /* Number of elements */
                                    MemDmaCallback                          /* Callback function  */
                                    );

                /* pend on completion */
                adi_sem_Pend(MemDmaSemaphoreHandle,ADI_SEM_TIMEOUT_FOREVER);
                pClientData += (TempBuffer.ElementCount)/sizeof(u32);
            }
            /* Otherwise a core copy is used */
            else {
                for(i=0;i<(TempBuffer.ElementCount)/sizeof(u32);i++,pClientData++,pTempData++)
                {
                    *pTempData = *pClientData;
                }
            }
            pDevice->ClassCommandComplete = FALSE;
            /* Pass LBA request to the Class Driver */
            SendLbaRequest(pDevice,&pSuperBuffer->LBARequest);
            /* queue the buffer */
            Result = adi_dev_Write( pDevice->ClassDriverHandle, ADI_DEV_1D, (ADI_DEV_BUFFER*)&TempBuffer );
            if (Result!=ADI_DEV_RESULT_SUCCESS) {
                return Result;
            }

            /* and wait for completion */
            WaitOnCommandCompletion(pDevice);

            BytesRemaining -= TempBuffer.ElementCount;
            pSuperBuffer->LBARequest.StartSector += SectorIncrement;
        }

        pClientBuffer = pClientBuffer->pNext;
    }

    (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_DEV_EVENT_BUFFER_PROCESSED, (void *)pBuffer );
    (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_PID_EVENT_DEVICE_INTERRUPT, (void *)pBuffer);

#else
#if defined(_USE_BACKG_XFER)
    /* save aside the next buffer in the chain */
    pNextBufferInChain = (ADI_FSS_SUPER_BUFFER *)pBuffer->OneD.pNext;

    /* and clear it so that the USB driver only deals with a single buffer */
    pBuffer->OneD.pNext = NULL;
#endif
#if defined(_USE_BACKG_XFER)
    /* Pass LBA request to the Class Driver */
    SendLbaRequest(pDevice,&pSuperBuffer->LBARequest);
#endif

    /* Pass on the request to the Class Driver */
    Result = adi_dev_Write( pDevice->ClassDriverHandle, BufferType, pBuffer );
#endif

    return(Result);
}

/*********************************************************************
*
*   Function:       pddControl
*
*   Description:    List of I/O control commands to the driver
*
*********************************************************************/
static u32 adi_pdd_Control(
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
    u32 Command,                    /* command ID */
    void *Value                     /* pointer to argument */
)
{
    ADI_USB_DEF   *pDevice;
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    pDevice = (ADI_USB_DEF *)PDDHandle;

    switch(Command)
    {
        case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):
        case (ADI_DEV_CMD_SET_DATAFLOW):
        break;

        case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
            /* this driver does not support DMA by itself */
            *((u32 *)Value) = FALSE;
            break;

        case ADI_FSS_CMD_GET_BACKGRND_XFER_SUPPORT:
            /* FALSE results in File Cache not being used - let's start with this */
#if defined(_USE_BACKG_XFER)
            *((u32 *)Value) = TRUE;
#else
            *((u32 *)Value) = FALSE;
#endif
            break;

        case (ADI_PID_CMD_ENABLE_DATAFLOW):
            break;

        case ADI_FSS_CMD_SET_CACHE_HEAP_ID:
            pDevice->CacheHeapID = (int)Value;
            break;

        case (ADI_PID_CMD_GET_FIXED):
            *((u32 *)Value) = FALSE;
            break;

        case (ADI_PID_CMD_MEDIA_ACTIVATE):
            Result=Activate(pDevice,(u32)Value);
            break;

        case ADI_PID_CMD_POLL_MEDIA_CHANGE:
            Result = DevicePollMedia(pDevice);
            break;

        case (ADI_PID_CMD_SEND_LBA_REQUEST):
#if !defined(_USE_BACKG_XFER) && !defined(_BF527_SDRAM_ISSUE_WORKAROUND)
            {  /* deferred to pdd_read */
                ADI_USB_MSD_SCSI_CMD_BLOCK SCSICmd;
                ADI_FSS_LBA_REQUEST *pLBARequest = (ADI_FSS_LBA_REQUEST*)Value;

                SCSICmd.LBASector = pLBARequest->StartSector;
                SCSICmd.BlockSize = pLBARequest->SectorCount;

                /* Pass on request to the Class Driver */
                if (pLBARequest->ReadFlag)
                {
                    Result = adi_dev_Control( pDevice->ClassDriverHandle,
                                              ADI_USB_MSD_CMD_SCSI_READ10,
                                              (void*)&SCSICmd
                                            );
                }
                else
                {
                    Result = adi_dev_Control( pDevice->ClassDriverHandle,
                                              ADI_USB_MSD_CMD_SCSI_WRITE10,
                                              (void*)&SCSICmd
                                            );
                }
            }
#endif
            break;

        case ADI_PID_CMD_DETECT_VOLUMES:
            Result = _adi_fss_DetectVolumes( 
                            pDevice->DeviceHandle, 
                            0,
                            pDevice->CacheHeapID,
                            512,
                            sizeof(u8),
                            pDevice->DataSemaphoreHandle
                    );
            break;

        case ADI_USB_CMD_SET_MEDIA_DETECTION_CALLBACK:
            MediaDetectionCallbackFunction = (ADI_DCB_CALLBACK_FN)Value;
            break;

        case ADI_PID_CMD_SET_DIRECT_CALLBACK:
            FSSDirectCallbackFunction = (ADI_DCB_CALLBACK_FN)Value;
            break;

        case ADI_FSS_CMD_GET_DATA_ELEMENT_WIDTH:
            *(u32*)Value = sizeof(u8);
            break;

        /* CASE ( Get overall sizes of teh media (as if one partition) ) */
        case (ADI_PID_CMD_GET_GLOBAL_MEDIA_DEF):
            Result = GetGlobalVolumeDef(pDevice,(ADI_FSS_VOLUME_DEF*)Value);
            break;

        case ADI_PID_CMD_GET_MAX_TFRCOUNT:
            /* For a USB drive, the limit is governed by the MSC class driver 
             * SCSI read/write commands. these are READ10/WRITE10 limiting the
             * sector count to 28 bits. However, it is likely that the limit 
             * is far less than this, as an ATA drive would be limited to 256
             * sectors. Thumb drives are unknown. It may make sense to limit
             * this to 64K bytes.
            */
            *((u32 *)Value) = 64*1024;
            break;
            
        case ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE:
            adi_sem_Pend (pDevice->LockSemaphoreHandle, ADI_SEM_TIMEOUT_FOREVER);
            break;

        case ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE:
            adi_sem_Post (pDevice->LockSemaphoreHandle);
            break;

#if defined(_BF527_SDRAM_ISSUE_WORKAROUND)
        case ADI_USB_CMD_SET_MEMDMA_HANDLE:
            pDevice->MemDMAStreamHandle = (ADI_DMA_STREAM_HANDLE)Value;
            break;
#endif
        case ADI_USB_CMD_SET_CONTROLLER_DRIVER_HANDLE:
            pDevice->UseDefaultController = false;
            pDevice->ControllerHandle = (ADI_DEV_DEVICE_HANDLE)Value;
            break;

        case ADI_USB_CMD_SET_CLASS_DRIVER_HANDLE:
            pDevice->UseDefaultClassDriver = false;
            pDevice->ClassDriverHandle = (ADI_DEV_DEVICE_HANDLE)Value;
            break;

        default:
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;
        break;
    }

    return(Result);
}

/*********************************************************************

    Function:       SendLbaRequest

    Description:    Sends the LBA request to the class driver

*********************************************************************/
static u32 SendLbaRequest(ADI_USB_DEF *pDevice, ADI_FSS_LBA_REQUEST *pLBARequest)
{
    ADI_USB_MSD_SCSI_CMD_BLOCK  SCSICmd;
    u32                         Result;

    SCSICmd.LBASector = pLBARequest->StartSector;
    SCSICmd.BlockSize = pLBARequest->SectorCount;

    /* Pass on request to the Class Driver */
    if (pLBARequest->ReadFlag)
    {
        Result = adi_dev_Control( pDevice->ClassDriverHandle,
                                  ADI_USB_MSD_CMD_SCSI_READ10,
                                  (void*)&SCSICmd
                                );
    }
    else
    {
        Result = adi_dev_Control( pDevice->ClassDriverHandle,
                                  ADI_USB_MSD_CMD_SCSI_WRITE10,
                                  (void*)&SCSICmd
                                );
    }

    return Result;
}


/*********************************************************************

    Function:       Activate

    Description:    Activates the host and enumerates the attached
                    device

*********************************************************************/
static u32 Activate(ADI_USB_DEF *pDevice, u32 EnableFlag)
{
    u32 Result = ADI_FSS_RESULT_SUCCESS;
    u32 i      = 0;

    /* The reset is only performed once for all PID's in chain */
    if (EnableFlag)
    {
        /* Initialize the core */
        if (pDevice->UseDefaultController)
        {
            adi_usb_CoreInit((void*)&Result);

            /*  Open the BF54x USB Driver */
            Result = adi_dev_Open(
                                    pDevice->ManagerHandle,                 /* DevMgr handle */
                                    &ADI_USBDRC_Entrypoint,                 /* pdd entry point */
                                    0,                                      /* device instance */
                                    (void*)1,                               /* client handle callback identifier */
                                    &pDevice->ControllerHandle,             /* device handle */
                                    ADI_DEV_DIRECTION_BIDIRECTIONAL,        /* data direction for this device */
                                    pDevice->DMAHandle,                     /* handle to DmaMgr for this device */
                                    pDevice->DCBHandle,                     /* handle to deferred callback service */
                                    ControllerCallback                      /* client's callback function */
                                );
        }

        /* Get the current Device ID. Once the driver is opened its */
        /* suppose to create the device. If the device is already opened then */
        /* it must return the same device ID. */
        if ((Result != ADI_DEV_RESULT_SUCCESS) || (pDevice->ControllerHandle == NULL))
        {
            Result = ADI_FSS_RESULT_FAILED;
        }

        if ( Result==ADI_DEV_RESULT_SUCCESS && !pDevice->UseDefaultClassDriver )
        {
            Result = adi_dev_Control ( pDevice->ClassDriverHandle, ADI_DEV_CMD_CHANGE_CLIENT_HANDLE, (void*)pDevice );
            Result = adi_dev_Control ( pDevice->ClassDriverHandle, ADI_DEV_CMD_CHANGE_CLIENT_CALLBACK_FUNCTION, (void*)ClassDriverCallback );
        }

        if ( Result==ADI_DEV_RESULT_SUCCESS && pDevice->UseDefaultClassDriver )
        {
            /* open the class driver */
            Result = adi_dev_Open(
                                    pDevice->ManagerHandle,                    /* DevMgr handle */
                                    &ADI_USB_Host_MassStorageClass_Entrypoint, /* pdd entry point */
                                    0,                                         /* device instance */
                                    pDevice,                                   /* client handle callback identifier */
                                    &pDevice->ClassDriverHandle,               /* device handle */
                                    ADI_DEV_DIRECTION_BIDIRECTIONAL,           /* data direction for this device */
                                    pDevice->DMAHandle,                        /* handle to DmaMgr for this device */
                                    pDevice->DCBHandle,                        /* handle to deferred callback service */
                                    ClassDriverCallback                        /* client's callback function */
                                );
        }
        if ((Result != ADI_DEV_RESULT_SUCCESS) || (pDevice->ClassDriverHandle == NULL))
        {
            Result = ADI_FSS_RESULT_FAILED;
        }

        /* If the controller and class drivers are supplied, we assume that these steps
         * have already been taken
        */
        if (pDevice->UseDefaultClassDriver && Result == ADI_DEV_RESULT_SUCCESS)
        {
            /* configure the controller mode */
            Result = adi_dev_Control(
                                        pDevice->ClassDriverHandle,
                                        ADI_USB_CMD_CLASS_SET_CONTROLLER_HANDLE,
                                        (void*)pDevice->ControllerHandle
                                    );

            /* configure the class driver */
            if (Result == ADI_DEV_RESULT_SUCCESS)
            {
                Result = adi_dev_Control(
                                            pDevice->ClassDriverHandle,
                                            ADI_USB_CMD_CLASS_CONFIGURE,
                                            (void*)0
                                        );
            }

            /* configure the data flow method */
            if (Result == ADI_DEV_RESULT_SUCCESS)
            {
                Result = adi_dev_Control(
                                            pDevice->ClassDriverHandle,
                                            ADI_DEV_CMD_SET_DATAFLOW_METHOD,
                                            (void*)ADI_DEV_MODE_CHAINED
                                        );
            }
            /* enable data flow */
            if (Result == ADI_DEV_RESULT_SUCCESS)
            {
                Result = adi_dev_Control(
                                            pDevice->ClassDriverHandle,
                                            ADI_DEV_CMD_SET_DATAFLOW,
                                            (void*)TRUE
                                        );
            }
            if (Result == ADI_DEV_RESULT_SUCCESS)
            {
                Result = adi_dev_Control(
                                            pDevice->ControllerHandle,
                                            ADI_USB_CMD_ENABLE_USB,
                                            0
                                        );
            }
        }

    }
    else {
        /* Deactivate */
        pDevice->MediaPresent = false;
    }


    return Result;
}

/*********************************************************************

  Function:    DevicePollMedia

  Description: Detect media change, this is used in particular to detect
               when a USB memory stick is removed.or inserted.

*********************************************************************/
u32 DevicePollMedia(ADI_USB_DEF *pDevice)
{

    u32 Result = ADI_DEV_RESULT_FAILED;
    u32 DeviceEnumerated = FALSE;
    u32 FSSValue = 0;   /* initiate the value to be sent to FSS as 0 (USB Device number in use) */

    if( pDevice->MediaPresent == FALSE )
    {
        /* The following callback to the FSS will result in any previously mounted file system
         * being unmounted; if first time the call is benign. This has to be done at thread level
         * as it may involve memory allocation tasks.
        */
        if (FSSDirectCallbackFunction) {
            (FSSDirectCallbackFunction)( &pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_REMOVED, &FSSValue );
        } else {
            (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_REMOVED, &FSSValue );
        }

        adi_dev_Control( pDevice->ClassDriverHandle, ADI_USB_MSD_CMD_IS_DEVICE_ENUMERATED, &DeviceEnumerated );
        if ( DeviceEnumerated )
        {
            /* Perform additional communications with device to establish
             * the connection and inform the FSS of media insertion.
            */
            ConfigureUsbDevice(pDevice);

            /* Use Result to pass the device number */
            Result = 0;
            /* call back to FSS to inform it of media insertion. The FSS will unmount any previous mount, and will
             * instruct this PID to detect volumes by reading the MBR. On successful return to FSS the Result value
             * will be set accordingly. If successful the MediaPresent flag is set to true.
            */
            if (FSSDirectCallbackFunction)
            {
                (FSSDirectCallbackFunction)( &pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_INSERTED, &Result );
            }
            else
            {
                (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_INSERTED, &Result );
            }
            if ( Result==ADI_FSS_RESULT_SUCCESS )
            {
                pDevice->MediaPresent = TRUE;
            } else
            {
                pDevice->MediaPresent = FALSE;
            }
            Result = ADI_DEV_RESULT_SUCCESS;
        }
        else
        {
            Result=ADI_DEV_RESULT_FAILED;
        }
    }

    /* otherwise if media was already present then this call successfully resulted
     * in nothing to do
    */
    else if (pDevice->MediaPresent)
    {
        Result = ADI_DEV_RESULT_SUCCESS;
    }

    return Result;
}

static void ConfigureUsbDevice(ADI_USB_DEF *pDevice)
{
    /* Before we can use the USB mass storage media we need to send the following commands */

    /* Allocate memory so we can Read the MBR */
#if defined(_BF527_SDRAM_ISSUE_WORKAROUND)
    u8 *pMBR = &TempBufferData[0];
#else
    u8 *pMBR = (u8*)_adi_fss_malloc( pDevice->CacheHeapID, 512 );
#endif
    /* Send 'Inquiry' command tro Class Driver */
    pDevice->ClassCommandComplete = FALSE;
    pDevice->SCSICommandError = FALSE;
    adi_dev_Control(pDevice->ClassDriverHandle, ADI_USB_MSD_CMD_INQUIRY, 0);
    /* and wait for completion */
    WaitOnCommandCompletion(pDevice);

    /* Check if SCSI commad failed */
    CheckForSCSICommandFailed(pDevice);

    /* Send 'read format' command to Class Driver */
    pDevice->ClassCommandComplete = FALSE;
    pDevice->SCSICommandError = FALSE;
    adi_dev_Control(pDevice->ClassDriverHandle, ADI_USB_MSD_CMD_READ_FORMAT_CAPACITIES, 0);
    /* and wait for completion */
    WaitOnCommandCompletion(pDevice);

    /* Check if SCSI commad failed */
    CheckForSCSICommandFailed(pDevice);

    /* Send 'read capacity' command to Class Driver */
    pDevice->ClassCommandComplete = FALSE;
    pDevice->SCSICommandError = FALSE;
    adi_dev_Control(pDevice->ClassDriverHandle, ADI_USB_MSD_CMD_READ_CAPACITY, 0);
    /* and wait for completion */
    WaitOnCommandCompletion(pDevice);

    /* Check if SCSI commad failed */
    CheckForSCSICommandFailed(pDevice);

    adi_fss_TransferSectors(
                            pDevice->DeviceHandle, 
                            (u16*)pMBR,
                            0,
                            0,
                            1,
                            512,
                            sizeof(u8),
                            READ,
                            pDevice->DataSemaphoreHandle
    );
    
    /* Send 'test unit ready' command to Class Driver */
    pDevice->ClassCommandComplete = FALSE;
    pDevice->SCSICommandError = FALSE;
    adi_dev_Control(pDevice->ClassDriverHandle, ADI_USB_MSD_CMD_TEST_UNIT_READY, 0);
    /* and wait for completion */
    WaitOnCommandCompletion(pDevice);

    /* Check if SCSI commad failed */
    CheckForSCSICommandFailed(pDevice);

#if !defined(_BF527_SDRAM_ISSUE_WORKAROUND)
    /* Remove the allocated memory */
    _adi_fss_free( pDevice->CacheHeapID, pMBR );
#endif
}

/*********************************************************************

    Function:       CallbackFromFSS

    Description:    The callback to be taken upon buffer completion and
                    device interrupt callbacks.
*********************************************************************/
static void CallbackFromFSS(void *pHandle, u32 Event, void *pArg)
{
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pArg;
    ADI_USB_DEF          *pDevice      = (ADI_USB_DEF *)pHandle;

    if (  Event == ADI_PID_EVENT_DEVICE_INTERRUPT
          && pSuperBuffer->SemaphoreHandle == pDevice->DataSemaphoreHandle )
    {
        adi_dev_Control( pDevice->DeviceHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
        adi_sem_Post( pSuperBuffer->SemaphoreHandle );
    }
}

/*********************************************************************

    Function:       CallbackFromFSS

    Description:    This is the callback to be taken from the Class
                    Driver.
*********************************************************************/
static void ClassDriverCallback(void *pHandle, u32 Event, void *pArg)
{
    ADI_USB_DEF *pDevice = (ADI_USB_DEF *)pHandle;
    void        *pExitCriticalArg;
    u32         Result;

    switch (Event)
    {
        /* CASE (USB DEVICE INSERTED) */
        case ADI_USB_EVENT_CONNECT:
            /* Callback FSS with ADI_FSS_EVENT_MEDIA_DETECTED event, which may pass the event on to
             * the application which can take action to Poll for Media.
            */
#if 0            
            if (FSSDirectCallbackFunction) {
                (FSSDirectCallbackFunction)( &pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_DETECTED, 0 );
            } else {
                (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_DETECTED, 0 );
            }
#endif            
            if (MediaDetectionCallbackFunction) {
                (MediaDetectionCallbackFunction)( &pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_DETECTED, 0 );
            }
            break;

        /* CASE (USB DEVICE REMOVED) */
        case ADI_USB_EVENT_DISCONNECT:
            /* simply flag as not present. Next ADI_PID_CMD_POLL_MEDIA_CHANGE command will
             * result in the ADI_FSS_EVENT_MEDIA_REMOVED event being sent to FSS.
            */
            if (pDevice->MediaPresent)
            {
                pDevice->MediaPresent = FALSE;
            }
            if (MediaDetectionCallbackFunction) {
                (MediaDetectionCallbackFunction)( &pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_REMOVED, 0 );
            }
            break;

        /* CASE (USB DATA TRANSFER COMPLETE) */
        case ADI_DEV_EVENT_BUFFER_PROCESSED:
           /* pass on event to FSS via DM callback */
#if !defined(_BF527_SDRAM_ISSUE_WORKAROUND)
           (pDevice->DMCallback)( pDevice->DeviceHandle, Event, pArg);
#endif

           /* If there is another buffer in the chain, send the next LBA request and
            * queue the next buffer with the class driver
           */
#if defined(_USE_BACKG_XFER)
           if (pNextBufferInChain)
           {
               ADI_FSS_SUPER_BUFFER *pSuperBuffer = pNextBufferInChain;

               /* The next buffer may not have an LBA Request associated with it, as
                * it may have been combimed with the previous buffer. In which case
                * we do not need to send the LBA request
               */
               if (pSuperBuffer->LBARequest.SectorCount)
               {
                   SendLbaRequest(pDevice, &pSuperBuffer->LBARequest);
               }
               pNextBufferInChain = (ADI_FSS_SUPER_BUFFER *)pSuperBuffer->Buffer.pNext;
               pSuperBuffer->Buffer.pNext = NULL;

               /* Queue the data buffer with the class driver */
               if (pSuperBuffer->LBARequest.ReadFlag)
               {
                    adi_dev_Read( pDevice->ClassDriverHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)pSuperBuffer);
               }
               else
               {
                    adi_dev_Write( pDevice->ClassDriverHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)pSuperBuffer );
               }
           }
           else
#endif
           {
               /* At the end of the chain we issue the Device Interrupt event to signal
                * total completion
               */
#if defined(_BF527_SDRAM_ISSUE_WORKAROUND)
                pExitCriticalArg = adi_int_EnterCriticalRegion( pDevice->pEnterCriticalArg );
                pDevice->ClassCommandComplete = TRUE;
                adi_int_ExitCriticalRegion(pExitCriticalArg);
#else
               (pDevice->DMCallback)( pDevice->DeviceHandle, ADI_PID_EVENT_DEVICE_INTERRUPT, pArg);
#endif
           }
            break;

        case ADI_USB_MSD_EVENT_SCSI_CMD_COMPLETE:
            /* post the semaphore upon which the DevicePollMedia function is pending */
            pExitCriticalArg = adi_int_EnterCriticalRegion( pDevice->pEnterCriticalArg );
            pDevice->ClassCommandComplete = TRUE;
            adi_int_ExitCriticalRegion(pExitCriticalArg);
        break;

        case ADI_USB_MSD_EVENT_SCSI_CMD_ERROR:
            pExitCriticalArg = adi_int_EnterCriticalRegion( pDevice->pEnterCriticalArg );
            pDevice->SCSICommandError = TRUE;
            adi_int_ExitCriticalRegion(pExitCriticalArg);
        break;
        
        case ADI_FSS_EVENT_MEDIA_ERROR:
        {
            if (FSSDirectCallbackFunction)
            {
                (FSSDirectCallbackFunction)( &pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_REMOVED, 0 );
            } else 
            {
                (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_REMOVED, 0 );
            }
        }
        break;
    }
}

/*********************************************************************

    Function:       ControllerCallback

    Description:    This is just a stub as the Controller driver does
                    not use callbacks in this way.
*********************************************************************/
static void ControllerCallback(void *pHandle, u32 Event, void *pArg)
{
}

/*********************************************************************

    Function:       WaitOnCommandCompletion

    Description:    Waits for Class Driver to complete a command.

*********************************************************************/
static void WaitOnCommandCompletion(ADI_USB_DEF *pDevice)
{
    void    *pExitCriticalArg;
    u32     completed;

    /* spin awaiting completion */
    do {
        pExitCriticalArg = adi_int_EnterCriticalRegion( pDevice->pEnterCriticalArg );
        completed = (pDevice->ClassCommandComplete==TRUE);
        adi_int_ExitCriticalRegion(pExitCriticalArg);
    } while (!completed);

}

/*********************************************************************

    Function:       MemDmaCallback

    Description:    Callback for Memory DMA
*********************************************************************/
static void MemDmaCallback( void *AppHandle, u32 Event, void *pArg )
{

    /* CASEOF (event type) */
    switch (Event)
    {
        /* CASE (buffer processed) */
        case ADI_DMA_EVENT_DESCRIPTOR_PROCESSED:
#if defined(_BF527_SDRAM_ISSUE_WORKAROUND)
            adi_sem_Post(MemDmaSemaphoreHandle);
#endif
            break;

        case ADI_DMA_EVENT_ERROR_INTERRUPT:
            break;

        default:
            break;
    }

    /* return */
}

/*********************************************************************

    Function:       CheckForSCSICommandFailed

    Description:

*********************************************************************/
static void CheckForSCSICommandFailed(ADI_USB_DEF *pDevice)
{
        /* If SCSI command error */
        if(pDevice->SCSICommandError == TRUE)
        {
            pDevice->SCSICommandError = FALSE;
            pDevice->ClassCommandComplete = FALSE;
            adi_dev_Control(pDevice->ClassDriverHandle, ADI_USB_MSD_CMD_REQ_SENSE, 0);
            /* and wait for completion */
            WaitOnCommandCompletion(pDevice);
        }
}

/*********************************************************************

    Function:       GetGlobalVolumeDef

    Description:    Assign and return pointer to index'd volume definition

*********************************************************************/
static u32 GetGlobalVolumeDef(ADI_USB_DEF *pDevice, ADI_FSS_VOLUME_DEF *pVolumeDef)
{
    /* Retrieve the Capacity 10 data from the media 
    */
    u32 Result = adi_dev_Control(pDevice->ClassDriverHandle, ADI_USB_MSD_CMD_GET_CAPACITY_TEN_DATA, (void*)&pDevice->CapacityTen);
    
    /* If successful, assign the values 
    */
    if (Result == ADI_DEV_RESULT_SUCCESS) {
        pVolumeDef->StartAddress = 0;
        pVolumeDef->VolumeSize   = pDevice->CapacityTen.NumBlocks;
        pVolumeDef->SectorSize   = pDevice->CapacityTen.BlockSize;
    }
    
    return Result;
}


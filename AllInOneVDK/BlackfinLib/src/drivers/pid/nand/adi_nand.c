/*********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_nand.c,v $
$Revision: 3578 $
$Date: 2010-10-19 09:36:04 -0400 (Tue, 19 Oct 2010) $

Description:
            PID Device driver for NAND Flash.

********************************************************************************
*/

#define __ADI_NAND_HOST_C__

#ifdef __USE_MEM_PLACEMENT__
#define __ADI_NAND_SECTION_CODE    section("adi_nand_pid_code")
#define __ADI_NAND_SECTION_DATA    section("adi_nand_pid_data")
#else
#define __ADI_NAND_SECTION_CODE
#define __ADI_NAND_SECTION_DATA
#endif 

#include <services/services.h>                           /* system service includes */
#include <services/fss/adi_fss.h>                        /* File System Service includes */
#include <drivers/adi_dev.h>                             /* device manager includes */
#include <drivers/pid/adi_ata.h>                         /* ATA structures, comamnds and macros */
#include <drivers/pid/nand/adi_nand.h>                   /* NAND PID structures, comamnds and macros */
#include <string.h>
#include <stdio.h>
#include <ccblkfn.h>
#include <drivers/pid/nand/ftl/adi_ftl.h>
#include <drivers/nfc/adi_nfc.h>


#pragma pack(4)

typedef struct ADI_NAND_DEF {
    ADI_DEV_MANAGER_HANDLE  ManagerHandle;          /* device manager handle                                        */
    ADI_DMA_MANAGER_HANDLE  DMAHandle;              /* handle to the DMA manager                                    */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle;           /* device manager handle                                        */
    u32                     DeviceNumber;           /* Device Number                                                */
    ADI_DEV_DIRECTION       Direction;              /* Direction of Device Driver                                   */
    ADI_DCB_HANDLE          DCBHandle;              /* callback manager handle                                      */
    ADI_DCB_CALLBACK_FN     DMCallback;             /* the callback function supplied by the Device Manager         */
    void                    *pEnterCriticalArg;     /* critical region argument                                     */
    int                     CacheHeapID;            /* Heap Index for Device transfer buffers                       */
    ADI_SEM_HANDLE          DataSemaphoreHandle;    /* Semaphore for Internal data transfers                        */
    ADI_SEM_HANDLE          LockSemaphoreHandle;    /* Semaphore for Lock Semaphore operation                       */
    u32                     Active;                 /* Driver is active */
    u32                     MediaPresent;           /* Flag to indicate whether media is available                  */
    u32                     ClassCommandComplete;   /* Flag to indicate whether class driver command is completed   */

    u32                     SCSICommandError;       /* Flag to indicate SCSI command error */
    volatile u8             *pRamDiskStart;         /* address of RAM disk location in memory                       */
    u8                      *pImageData;            /* address of MFBR image to copy on activation */
    ADI_FSS_VOLUME_DEF      GlobalVolumeDef;        /* Global parameters of media */
    u32                     DataElementWidth;       /* width of data bus */
    F_DRIVER                *pFtlEntryPoints;       /* HCC FTL entry points */
    ADI_DEV_DEVICE_HANDLE   NFC_DeviceHandle;       /* NFC device driver handle                                     */
    ADI_NFD_INFO_TABLE      NFDInfo;                /* Configuration information about NAND array                   */
    ADI_DMA_STREAM_HANDLE   MemDMAStreamHandle;     /*                                    */
    u32                     Perform_Format_Flag;    /* Flag to indicate to perform format */
    u8                      *pNFDAlignBuf;
} ADI_NAND_DEF;

__ADI_NAND_SECTION_DATA
static ADI_SEM_HANDLE MemDmaSemaphoreHandle;    /* Semaphore for Internal data transfers                        */
static void MemDmaCallback( void *AppHandle, u32 Event, void *pArg );

#define PDD_DEFAULT_SECTOR_SIZE     512
#define ADI_NAND_GEN_HEAPID      (-1)
#define ADI_NFC_PID_GEN_HEAPID  ADI_NAND_GEN_HEAPID

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
ADI_DEV_PDD_ENTRY_POINT ADI_NAND_Entrypoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control
};

static void CallbackFromFSS(void *pHandle, u32 Event, void *pArg);
static void CallbackFromNFC(void *pHandle, u32 Event, void *pArg);

__ADI_NAND_SECTION_DATA
static ADI_DCB_CALLBACK_FN FSSDirectCallbackFunction=NULL;

static   u32    Activate(ADI_NAND_DEF *pDevice, u32 EnableFlag);
static   u32    DevicePollMedia(ADI_NAND_DEF *pDevice);
static   u32    ProcessExtendedPartitions(ADI_NAND_DEF *pDevice, u32 Drive, ADI_ATA_PARTITION_ENTRY *pPartTabEntry, u32 *Count);
static   void   NewVolumeDef(ADI_NAND_DEF *pDevice, u32 Drive, ADI_ATA_PARTITION_ENTRY *pPartTab);
static   u32    AssignFileSystemType(u32 type);
static   void   GetGlobalVolumeDef(ADI_NAND_DEF *pDevice, ADI_FSS_VOLUME_DEF *pVolumeDef);
static   void   SetGlobalVolumeDef(ADI_NAND_DEF *pDevice, u32 VolumeSizeMB);
static   u32    FormatRamDisk( ADI_NAND_DEF *pDevice );
unsigned char   adi_NAND_NandFormat( ADI_DEV_PDD_HANDLE PDDHandle );
static   u32    adi_pdd_ReadWrite(
                            ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
                            ADI_DEV_BUFFER_TYPE BufferType, /* buffer type */
                            ADI_DEV_BUFFER *pBuffer         /* pointer to buffer */
);
u32     ConfigNFC( ADI_NAND_DEF *pDevice );

/* entry point into FTL to set sector size */
extern void adi_ftl_SetSectorSize(unsigned long nSectorSize);


/* ***************************************************************************
 * FSS housekeeping functions
 * ***************************************************************************
 */
extern void *_adi_fss_malloc( int id, size_t size );
extern void _adi_fss_free( int id, void *p );
extern void *_adi_fss_MallocAligned( int id, size_t size );
extern void _adi_fss_FreeAligned( int id, void *p );
extern u32 _adi_fss_DetectVolumes( 
                            ADI_DEV_DEVICE_HANDLE   hDevice, 
                            u32                     Drive,
                            int                     HeapID,
                            u32                     SectorSize,
                            u32                     DataElementWidth,
                            ADI_SEM_HANDLE          hSemaphore
);

extern ADI_DMA_MANAGER_HANDLE adi_dma_ManagerHandle;   // handle to the DMA manager
extern ADI_DEV_MANAGER_HANDLE adi_dev_ManagerHandle;   // handle to the device manager


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
__ADI_NAND_SECTION_CODE
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
    ADI_NAND_DEF *pDevice = (ADI_NAND_DEF*)_adi_fss_malloc(ADI_NAND_GEN_HEAPID,sizeof(ADI_NAND_DEF));
    if (!pDevice) {
        Result = ADI_FSS_RESULT_NO_MEMORY;
    }
    else
    {
        memset( pDevice, 0, sizeof(ADI_NAND_DEF));

        /* initialize the Device Header structure */
        pDevice->ManagerHandle = ManagerHandle;
        pDevice->Direction = Direction;
        pDevice->DMAHandle = DMAHandle;
        pDevice->DCBHandle = DCBHandle;
        pDevice->DMCallback = DMCallback;
        pDevice->DeviceHandle = DeviceHandle;
        pDevice->DeviceNumber = DeviceNumber;
        pDevice->pEnterCriticalArg = pCriticalRegionArg;
        pDevice->Active = FALSE;
        pDevice->MediaPresent = FALSE;
        pDevice->MemDMAStreamHandle = (ADI_DMA_STREAM_HANDLE)NULL;
        pDevice->DataElementWidth = sizeof(u32);
        pDevice->Perform_Format_Flag = FALSE;

        /* Use the General Heap for data buffers by default */
        pDevice->CacheHeapID = ADI_NAND_GEN_HEAPID;

        /* Initialize pointers to NULL */
        pDevice->pFtlEntryPoints = NULL;
        pDevice->GlobalVolumeDef.StartAddress = 0;
        pDevice->pNFDAlignBuf = NULL;

        /* set up FTL globals  */
        gnAdiFtlFssStartBlock = 0xDEFA;
        gnAdiFtlFssEndBlock   = 0;

        /* save the physical device handle in the client supplied location */
        *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;

        Result = (u32)adi_sem_Create(0,&pDevice->DataSemaphoreHandle, NULL);
        /* we can use the data semaphore for internal Memory DMA transfers as well */
        MemDmaSemaphoreHandle = pDevice->DataSemaphoreHandle;

        if (Result == (u32)ADI_SEM_RESULT_SUCCESS)
        {
            Result = (u32)adi_sem_Create(1,&pDevice->LockSemaphoreHandle, NULL);
        }
    }

    if (Result!=ADI_DEV_RESULT_SUCCESS && pDevice)
    {
        /* free memory reserved for ADI NFC PID */
        _adi_fss_free(ADI_NFC_PID_GEN_HEAPID,pDevice);
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

__ADI_NAND_SECTION_CODE
static u32 adi_pdd_Close(
    ADI_DEV_PDD_HANDLE PDDHandle    /* PDD handle */
)
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    ADI_NAND_DEF   *pDevice = (ADI_NAND_DEF *)PDDHandle;

    if (pDevice->DataSemaphoreHandle)
    {
        adi_sem_Delete(pDevice->DataSemaphoreHandle);
        adi_sem_Delete(pDevice->LockSemaphoreHandle);
    }

    /* free the alignment buffer */
    if (pDevice->pNFDAlignBuf) {
        _adi_fss_FreeAligned(pDevice->CacheHeapID,(void*)pDevice->pNFDAlignBuf);
    }
    /* free the device instance */
    _adi_fss_free(ADI_NAND_GEN_HEAPID,(void*)pDevice);

    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Read
*
*   Description:    Queue read request buffer with the Class Driver
*
*********************************************************************/

__ADI_NAND_SECTION_CODE
static u32 adi_pdd_Read(
                            ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
                            ADI_DEV_BUFFER_TYPE BufferType, /* buffer type */
                            ADI_DEV_BUFFER *pBuffer         /* pointer to buffer */
)
{
    u32 Result=ADI_FSS_RESULT_SUCCESS;

    Result = adi_pdd_ReadWrite(PDDHandle, BufferType, pBuffer);

    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Write
*
*   Description:    Queue write request buffer with the Class Driver
*
*********************************************************************/

__ADI_NAND_SECTION_CODE
static u32 adi_pdd_Write(
                            ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
                            ADI_DEV_BUFFER_TYPE BufferType, /* buffer type */
                            ADI_DEV_BUFFER *pBuffer         /* pointer to buffer */
)
{
    u32 Result=ADI_FSS_RESULT_SUCCESS;

    Result = adi_pdd_ReadWrite(PDDHandle, BufferType, pBuffer);

    return(Result);
}

/*********************************************************************
*
*   Function:       pddControl
*
*   Description:    List of I/O control commands to the driver
*
*********************************************************************/
__ADI_NAND_SECTION_CODE
static u32 adi_pdd_Control(
    ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
    u32 Command,                    /* command ID */
    void *Value                     /* pointer to argument */
)
{
    ADI_NAND_DEF   *pDevice;
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    pDevice = (ADI_NAND_DEF *)PDDHandle;

    switch(Command)
    {
        case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):
        case (ADI_DEV_CMD_SET_DATAFLOW):
        break;

        case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
            /* this driver does not support peripheral DMA by itself */
            *((u32 *)Value) = false;
            break;

        case ADI_FSS_CMD_GET_BACKGRND_XFER_SUPPORT:
            /* even though we do not background transfers, this flag triggers
             * the use of the file cache which is advantageous for
             * Flash media
            */
            *((u32 *)Value) = true;
            break;

        case (ADI_PID_CMD_ENABLE_DATAFLOW):
            break;

        case ADI_FSS_CMD_SET_CACHE_HEAP_ID:
            pDevice->CacheHeapID = (int)Value;
            break;

        case (ADI_PID_CMD_GET_FIXED):
            /* Whilst the NAND can not be removed, setting this to false will result
             * in USB hosts treating it as a removable disk
            */
            *((u32 *)Value) = false;
            break;

        case (ADI_PID_CMD_MEDIA_ACTIVATE):
            Result=Activate(pDevice,(u32)Value);
            break;

        case ADI_PID_CMD_POLL_MEDIA_CHANGE:
            Result = DevicePollMedia(pDevice);
            break;

        case (ADI_PID_CMD_SEND_LBA_REQUEST):
            break;

        /* CASE ( Get overall sizes of the media (as if one partition) ) */
        case (ADI_PID_CMD_GET_GLOBAL_MEDIA_DEF):
             GetGlobalVolumeDef(pDevice,(ADI_FSS_VOLUME_DEF*)Value);
            break;

        case ADI_PID_CMD_DETECT_VOLUMES:
            Result = _adi_fss_DetectVolumes( 
                            pDevice->DeviceHandle, 
                            0,
                            pDevice->CacheHeapID,
                            pDevice->GlobalVolumeDef.SectorSize,
                            pDevice->DataElementWidth,
                            pDevice->DataSemaphoreHandle
                    );
            break;

        case ADI_PID_CMD_SET_DIRECT_CALLBACK:
            FSSDirectCallbackFunction = (ADI_DCB_CALLBACK_FN)Value;
            break;

        case ADI_FSS_CMD_GET_DATA_ELEMENT_WIDTH:
            *(u32*)Value = sizeof(u16);
            break;

        case ADI_PID_CMD_GET_MAX_TFRCOUNT:
            /* The NAND FTL supports up to max signed int nubmber of sectors, 
             * which would result in a value too large for the u32 result, 
             * so limit to max unsigned integer value of 0xFFFFFFFF
            */
            *((u32 *)Value) = 0xFFFFFFFF;
            break;

        case ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE:
            adi_sem_Pend (pDevice->LockSemaphoreHandle, ADI_SEM_TIMEOUT_FOREVER);
            break;

        case ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE:
            adi_sem_Post (pDevice->LockSemaphoreHandle);
            break;

        case ADI_NAND_CMD_SET_NFC_HANDLE:
            pDevice->NFC_DeviceHandle = (ADI_DEV_DEVICE_HANDLE)Value;
            ghAdiNfcDriver = (ADI_DEV_DEVICE_HANDLE)Value;
            break;

        case ADI_NAND_CMD_SET_RESERVED_SIZE:
            gnAdiFtlFssStartBlock = (u32)Value;
            break;

        case ADI_FSS_CMD_SET_DATA_SEMAPHORE_TIMEOUT:
            //g_ADI_NAND_DataSemaphoreTimeout = (u32)Value;
            break;

        /* CASE (Set NAND Flash Device specific information) */
        case (ADI_NAND_CMD_SET_NFD_INFO):
            /* save NFD Information table  */
            pDevice->NFDInfo = *(ADI_NFD_INFO_TABLE *)Value;
            break;

        /* CASE (internal format NAND call.  ) */
        case (ADI_NAND_CMD_PREPARE_NFD_FOR_FTL):
            /* Format the NAND flash device. */
            pDevice->Perform_Format_Flag = TRUE;
            break;

        /* CASE (internal format NAND call.  ) */
        case (ADI_NAND_CMD_SET_SECTOR_SIZE):
            /* Format the NAND flash device. */
            if ( ((u32)Value)==512U || ((u32)Value)==2048 ) {
                adi_ftl_SetSectorSize((u32)Value);
            }
            else {
                Result = ADI_DEV_RESULT_FAILED;
            }
                
            break;

       default:
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;
        break;
    }

    return(Result);
}

/*********************************************************************

    Function:       Activate

    Description:    Activates the host and enumerates the attached
                    device

*********************************************************************/
__ADI_NAND_SECTION_CODE
static u32 Activate(ADI_NAND_DEF *pDevice, u32 EnableFlag)
{
    u32                     Result = ADI_FSS_RESULT_SUCCESS;
    u32                     i      = 0;
    void                    *Value = NULL;              /* pointer to argument */
    ADI_FSS_SUPER_BUFFER    FSSBuffer;
    F_PHY                   FtlPhyLayer;


    /* The reset is only performed once for all PID's in chain */
    if (!pDevice->Active)
    {

        if (!pDevice->NFC_DeviceHandle)
        {
            /* The NFC was not previously opened so open it  */
            Result = adi_dev_Open(  pDevice->ManagerHandle, /* Device Manager handle                */
                                    &ADI_NFC_EntryPoint,    /* NFC driver Entry point               */
                                    pDevice->DeviceNumber,  /* NFC device number                    */
                                    NULL,                   /* client handle                        */
                                    &pDevice->NFC_DeviceHandle,      /* Location to store NFC device handle  */
                                    pDevice->Direction,     /* NFC Direction                        */
                                    pDevice->DMAHandle,     /* DMA manager Handle                   */
                                    pDevice->DCBHandle,     /* DCB Manager Handle                   */
                                    CallbackFromNFC         /* NFC callback function                */
                                    );
        }

        /* configure the NFC driver */
        if (Result==ADI_FSS_RESULT_SUCCESS)
        {
            Result = ConfigNFC( pDevice );
        }

        /* If needed and enabled format the device */
        if (( pDevice->Perform_Format_Flag == TRUE ) && (Result==ADI_FSS_RESULT_SUCCESS))
        {
            if (adi_ftl_NandFormat())
            {
                /* if format NFD failed, return error */
                Result = ADI_FSS_RESULT_FAILED;
            }
        }

        /*  Initialize the entry points for the wear leveling. */
        if (Result == ADI_FSS_RESULT_SUCCESS)
        {
            /* initialize FTL layer */
            pDevice->pFtlEntryPoints = adi_ftl_Init();
            if (!pDevice->pFtlEntryPoints) {
                Result = ADI_FSS_RESULT_NO_MEDIA;
            }
        }

        if (Result==ADI_FSS_RESULT_SUCCESS)
        {
            /* Get Global Size parameters from FTL */
            (pDevice->pFtlEntryPoints->getphy)(pDevice->pFtlEntryPoints, &FtlPhyLayer);
            pDevice->GlobalVolumeDef.VolumeSize = FtlPhyLayer.number_of_sectors;
            pDevice->GlobalVolumeDef.SectorSize = FtlPhyLayer.bytes_per_sector;

        }
        if (Result==ADI_FSS_RESULT_SUCCESS) {
            pDevice->Active = true;
        }

    }
    else {
        /* Deactivate */
        pDevice->Active = false;
        pDevice->MediaPresent = false;

    }

    return Result;
}

/*********************************************************************

  Function:    DevicePollMedia

  Description: Detect media change, this is used in particular to detect
               when a USB memory stick is removed.or inserted.

*********************************************************************/
__ADI_NAND_SECTION_CODE
u32 DevicePollMedia(ADI_NAND_DEF *pDevice)
{

    u32 Result = ADI_DEV_RESULT_SUCCESS;
    u32 DeviceEnumerated = FALSE;
    u32 FSSValue = 0;   /* initiate the value to be sent to FSS as 0 (USB Device number in use) */

    if( pDevice->Active && pDevice->MediaPresent == FALSE )
    {
        /* The following callback to the FSS will result in any previously mounted file system
         * being unmounted; if first time the call is benign. This has to be done at thread level
         * as it may involve memory allocation tasks.
        */
        if (FSSDirectCallbackFunction) {
            (FSSDirectCallbackFunction)( &pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_INSERTED, &FSSValue );
        } else {
            (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_INSERTED, &FSSValue );
        }

    }

    if (FSSValue==ADI_DEV_RESULT_SUCCESS) {
        pDevice->MediaPresent = true;
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

/*********************************************************************

    Function:       CallbackFromFSS

    Description:    The callback to be taken upon buffer completion and
                    device interrupt callbacks.
*********************************************************************/
__ADI_NAND_SECTION_CODE
static void CallbackFromFSS(void *pHandle, u32 Event, void *pArg)
{
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pArg;
    ADI_NAND_DEF          *pDevice      = (ADI_NAND_DEF *)pHandle;

    if (  Event == ADI_PID_EVENT_DEVICE_INTERRUPT
          && pSuperBuffer->SemaphoreHandle == pDevice->DataSemaphoreHandle )
    {
        adi_dev_Control( pDevice->DeviceHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
        adi_sem_Post( pSuperBuffer->SemaphoreHandle );
    }
}

/*********************************************************************

    Function:       CallbackFromNFC

    Description:    The callback to be taken upon buffer completion and
                    device interrupt callbacks.
*********************************************************************/
__ADI_NAND_SECTION_CODE
static void CallbackFromNFC(void *pHandle, u32 Event, void *pArg)
{
return;
}



/*********************************************************************
*
*   Function:       adi_pdd_ReadWrite
*
*   Description:    Same code is required for Read and Write
*
*********************************************************************/

__ADI_NAND_SECTION_CODE
static u32 adi_pdd_ReadWrite(
                            ADI_DEV_PDD_HANDLE PDDHandle,   /* PDD handle */
                            ADI_DEV_BUFFER_TYPE BufferType, /* buffer type */
                            ADI_DEV_BUFFER *pBuffer         /* pointer to buffer */
)
{
    u32 Result=ADI_FSS_RESULT_SUCCESS;
    int sResult;
    void *CurrentCallbackParameter;

    /* Expose the Device Definition structure */
    ADI_NAND_DEF      *pDevice = (ADI_NAND_DEF *)PDDHandle;

    /* Expose the FSS super buffer structure */
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pBuffer;
    ADI_FSS_SUPER_BUFFER *pCurrentBuffer = (ADI_FSS_SUPER_BUFFER *)pBuffer;

    /* Assign callback function and handle to Super Buffer */
    pSuperBuffer->PIDCallbackFunction = CallbackFromFSS;
    pSuperBuffer->PIDCallbackHandle = (void*)pDevice;

    while (pCurrentBuffer)
    {
        /* Read/write from FTL */
        if (pCurrentBuffer->LBARequest.ReadFlag)
        {
            sResult = (pDevice->pFtlEntryPoints->readmultiplesector)(
                                pDevice->pFtlEntryPoints,
                                pCurrentBuffer->Buffer.Data,
                                pCurrentBuffer->LBARequest.StartSector,
                                pCurrentBuffer->LBARequest.SectorCount
                                );

        }
        else
        {
            sResult = (pDevice->pFtlEntryPoints->writemultiplesector)(
                                pDevice->pFtlEntryPoints,
                                pCurrentBuffer->Buffer.Data,
                                pCurrentBuffer->LBARequest.StartSector,
                                pCurrentBuffer->LBARequest.SectorCount
                                );

        }

        if (sResult) {
            /* If FTL access fails then abort */
            Result = ADI_FSS_RESULT_FAILED;
            pCurrentBuffer = NULL;
        }
        else {
            /* Signal that Data completion has completed */
            CurrentCallbackParameter = pCurrentBuffer->Buffer.CallbackParameter;
            pCurrentBuffer->Buffer.ProcessedFlag = true;
            pCurrentBuffer->Buffer.ProcessedElementCount = pCurrentBuffer->Buffer.ElementCount;
            (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_DEV_EVENT_BUFFER_PROCESSED, CurrentCallbackParameter );

            pCurrentBuffer = (ADI_FSS_SUPER_BUFFER *)pCurrentBuffer->Buffer.pNext;
        }
    }

    if (Result==ADI_FSS_RESULT_SUCCESS) {
        /* issue 'Device interrupt' event on completion of all buffers */
        (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_PID_EVENT_DEVICE_INTERRUPT, CurrentCallbackParameter );
    }

    return(Result);
}

/*********************************************************************

    Function:       GetGlobalVolumeDef

    Description:    Assign and return pointer to index'd volume definition

*********************************************************************/
__ADI_NAND_SECTION_CODE
static void GetGlobalVolumeDef(ADI_NAND_DEF *pDevice, ADI_FSS_VOLUME_DEF *pVolumeDef)
{
    pVolumeDef->StartAddress = pDevice->GlobalVolumeDef.StartAddress;
    pVolumeDef->VolumeSize   = pDevice->GlobalVolumeDef.VolumeSize;
    pVolumeDef->SectorSize   = pDevice->GlobalVolumeDef.SectorSize;
}


/*********************************************************************

    Function:       ConfigNFC

    Description:    Configure the NAND flash controller settings.

*********************************************************************/
__ADI_NAND_SECTION_CODE
u32 ConfigNFC( ADI_NAND_DEF *pDevice )
{
    u32                         Result              = ADI_DEV_RESULT_SUCCESS;
    u8                          u8Temp              = 0;
    ADI_NFC_DMA_FRAME_BUFFER    oNFCFrameBuf        = {0};

    /*  We have a handle to the NAND flash device so we may continue. */
    /* Set dataflow method */
    Result = adi_dev_Control( pDevice->NFC_DeviceHandle,
                            ADI_DEV_CMD_SET_DATAFLOW_METHOD,
                            (void *)ADI_DEV_MODE_CHAINED);

    /*  We have a handle to the NAND flash device so we may continue. */
    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Save the device handle */
        ghAdiNfcDriver = pDevice->NFC_DeviceHandle;
        //g_ADI_NAND_DataSemaphoreHandle = pDevice->DataSemaphoreHandle;

        /* Set up the NFD Information */
        Result = adi_dev_Control ( pDevice->NFC_DeviceHandle,
                                   ADI_NFC_CMD_PASS_NFD_INFO,
                                   (void *)&pDevice->NFDInfo);

    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Calculate Number of DMA Frames required by this NFD */
        //u8Temp = (ADI_EZKIT_NFD_Info.DataAreaPerPage / ADI_NFD_DATA_PKT_SIZE_IN_BYTES);
        u8Temp = (2048 / ADI_NFD_DATA_PKT_SIZE_IN_BYTES);


        /* Get Memory for NFD Alignment buffer & DMA Frame for this NFD */
        pDevice->pNFDAlignBuf = (u8*)_adi_fss_MallocAligned(pDevice->CacheHeapID,
                                            (ADI_NFD_DATA_PKT_SIZE_IN_BYTES +\
                                            ADI_NFC_DMA_DATA_FRAME_BASE_MEMORY * u8Temp));

        /* IF (we've memory for NFD alignment buffer & DMA Frames */
        if (pDevice->pNFDAlignBuf)
        {
            /* pass NFD Alignment buffer to NFC driver */
            Result = adi_dev_Control(pDevice->NFC_DeviceHandle,
                                     ADI_NFC_CMD_SET_256BYTES_ALIGN_BUFFER,
                                     (void*)pDevice->pNFDAlignBuf);

            if (Result == ADI_DEV_RESULT_SUCCESS)
            {
                /* populate NFC Frame buffer structure */
                oNFCFrameBuf.pDMAFrameMemory = (void*) (pDevice->pNFDAlignBuf + ADI_NFD_DATA_PKT_SIZE_IN_BYTES);
                oNFCFrameBuf.NumDMAFrames    = u8Temp;

                /* pass DMA Frame buffer memory to NFC driver */
                Result = adi_dev_Control(pDevice->NFC_DeviceHandle,
                                         ADI_NFC_CMD_SET_DMA_FRAME_BUFFER,
                                         (void*)&oNFCFrameBuf);
            }

        }
    }

    return Result;

}





/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/




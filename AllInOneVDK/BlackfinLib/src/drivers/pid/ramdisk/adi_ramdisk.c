/*********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_ramdisk.c,v $
$Revision: 3459 $
$Date: 2010-09-29 10:37:28 -0400 (Wed, 29 Sep 2010) $

Description:
            PID Device driver for RamDisk.

********************************************************************************
*/

#define __ADI_RAMDISK_C__

#include <services/services.h>                                          /* system service includes */
#include <services/fss/adi_fss.h>                                       /* File System Service includes */
#include <drivers/adi_dev.h>                                            /* device manager includes */
#include <drivers/pid/adi_ata.h>                                        /* ATA structures, comamnds and macros */
#include <drivers/fsd/fat/adi_fat.h>                                        /* ATA structures, comamnds and macros */
#include <drivers/pid/ramdisk/adi_ramdisk.h>                            /* Ramdisk PID structures, comamnds and macros */
#include <string.h>
#include <stdio.h>
#include <ccblkfn.h>

typedef struct ADI_RAMDISK_DEF {
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
    u32                     Active;                 /* Driver is active                                             */
    u32                     MediaPresent;           /* Flag to indicate whether media is available                  */
    volatile u8             *pRamDiskStart;         /* address of RAM disk location in memory                       */
    u8                      *pImageData;            /* address of MFBR image to copy on activation                  */
    ADI_FSS_VOLUME_DEF      GlobalVolumeDef;        /* Global parameters of media                                   */
    u32                     DataElementWidth;       /* width of data bus                                            */
    u32                     nUsbDelay;              /* delay to enable USB transfers to work                        */
} ADI_RAMDISK_DEF;

static ADI_SEM_HANDLE MemDmaSemaphoreHandle;    /* Semaphore for Internal data transfers                        */
static void MemDmaCallback( void *AppHandle, u32 Event, void *pArg );

#define PDD_DEFAULT_SECTOR_SIZE     512
#define ADI_RAMDISK_GEN_HEAPID      (-1)

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

/* Macro to calculate address within Ramdisk */
#define ADI_RAMDISK_ADDRESS(Base,Sector) ( (u8*)Base + (Sector)*512 )


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
ADI_DEV_PDD_ENTRY_POINT ADI_RAMDISK_Entrypoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control
};

static void CallbackFromFSS(void *pHandle, u32 Event, void *pArg);

static ADI_DCB_CALLBACK_FN FSSDirectCallbackFunction=NULL;

static u32  Activate(ADI_RAMDISK_DEF *pDevice, u32 EnableFlag);
static u32  DevicePollMedia(ADI_RAMDISK_DEF *pDevice);
static u32  DetectVolumes(ADI_RAMDISK_DEF *pDevice, u32 Drive);
static u32  ProcessExtendedPartitions(ADI_RAMDISK_DEF *pDevice, u32 Drive, ADI_ATA_PARTITION_ENTRY *pPartTabEntry, u32 *Count);
static void NewVolumeDef(ADI_RAMDISK_DEF *pDevice, u32 Drive, u32 StartAddress, u32 VolumeSize, u32 FileSystemType);
static u32  AssignFileSystemType(u32 type);
static void GetGlobalVolumeDef(ADI_RAMDISK_DEF *pDevice, ADI_FSS_VOLUME_DEF *pVolumeDef);
static void SetGlobalVolumeDef(ADI_RAMDISK_DEF *pDevice, u32 VolumeSizeMB);
static void CopyToFromMemory(ADI_RAMDISK_DEF *pDevice, ADI_FSS_SUPER_BUFFER *pFSSBuffer);

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


/* ***************************************************************************
 * Delay functions
 * ***************************************************************************
 */
static void WaitOneMicroSec(void);
static int WaitMicroSec(unsigned int msec);


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
    ADI_RAMDISK_DEF *pDevice = (ADI_RAMDISK_DEF*)_adi_fss_malloc(ADI_RAMDISK_GEN_HEAPID,sizeof(ADI_RAMDISK_DEF));
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
        pDevice->Active = FALSE;
        pDevice->MediaPresent = FALSE;
        pDevice->DataElementWidth = sizeof(u32);

        /* default delay = 0 microsecs */
        pDevice->nUsbDelay = 0;

        /* Use the General Heap for data buffers by default */
        pDevice->CacheHeapID = ADI_RAMDISK_GEN_HEAPID;

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

    ADI_RAMDISK_DEF   *pDevice = (ADI_RAMDISK_DEF *)PDDHandle;

    if (pDevice->DataSemaphoreHandle)
    {
        adi_sem_Delete(pDevice->DataSemaphoreHandle);
        adi_sem_Delete(pDevice->LockSemaphoreHandle);
    }

    /* free the device instance */
    _adi_fss_free(ADI_RAMDISK_GEN_HEAPID,(void*)pDevice);

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
    u32 Result=ADI_FSS_RESULT_SUCCESS;

    /* Expose the Device Definition structure */
    ADI_RAMDISK_DEF      *pDevice = (ADI_RAMDISK_DEF *)PDDHandle;

    /* Expose the FSS super buffer structure */
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pBuffer;
    ADI_FSS_SUPER_BUFFER *pCurrentBuffer = (ADI_FSS_SUPER_BUFFER *)pBuffer;

    /* Assign callback function and handle to Super Buffer */
    pSuperBuffer->PIDCallbackFunction = CallbackFromFSS;
    pSuperBuffer->PIDCallbackHandle = (void*)pDevice;

    while (pCurrentBuffer)
    {
        /* Read from RamDisk */
        CopyToFromMemory( pDevice, pCurrentBuffer);

        /* Signal that Data completion has completed */
        pCurrentBuffer->Buffer.ProcessedFlag = true;
        pCurrentBuffer->Buffer.ProcessedElementCount = pCurrentBuffer->Buffer.ElementCount;
        (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_DEV_EVENT_BUFFER_PROCESSED, (void *)pBuffer );

        /* Get Next Buffer in chain */
        pCurrentBuffer = (ADI_FSS_SUPER_BUFFER *)pCurrentBuffer->Buffer.pNext;

    }

    /* issue 'Device interrupt' event on completion of all buffers */
    (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_PID_EVENT_DEVICE_INTERRUPT, (void *)pBuffer );

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
    u32 Result=ADI_FSS_RESULT_SUCCESS;

    /* Expose the Device Definition structure */
    ADI_RAMDISK_DEF      *pDevice = (ADI_RAMDISK_DEF *)PDDHandle;

    /* Expose the FSS super buffer structure */
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pBuffer;
    ADI_FSS_SUPER_BUFFER *pCurrentBuffer = (ADI_FSS_SUPER_BUFFER *)pBuffer;

    /* Assign callback function and handle to Super Buffer */
    pSuperBuffer->PIDCallbackFunction = CallbackFromFSS;
    pSuperBuffer->PIDCallbackHandle = (void*)pDevice;

    while (pCurrentBuffer)
    {
        /* Read from RamDisk */
        CopyToFromMemory( pDevice, pCurrentBuffer);

        /* Signal that Data completion has completed */
        pCurrentBuffer->Buffer.ProcessedFlag = true;
        pCurrentBuffer->Buffer.ProcessedElementCount = pCurrentBuffer->Buffer.ElementCount;
        (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_DEV_EVENT_BUFFER_PROCESSED, (void *)pBuffer );

        /* Get Next Buffer in chain */
        pCurrentBuffer = (ADI_FSS_SUPER_BUFFER *)pCurrentBuffer->Buffer.pNext;

    }

    /* issue 'Device interrupt' event on completion of all buffers */
    (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_PID_EVENT_DEVICE_INTERRUPT, (void *)pBuffer );

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
    ADI_RAMDISK_DEF   *pDevice;
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    pDevice = (ADI_RAMDISK_DEF *)PDDHandle;

    switch(Command)
    {
        case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):
        case (ADI_DEV_CMD_SET_DATAFLOW):
        break;

        case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
            /* this driver does not support peripheral DMA by itself */
            *((u32 *)Value) = FALSE;
            break;

        case ADI_FSS_CMD_GET_BACKGRND_XFER_SUPPORT:
            /* FALSE results in File Cache not being used - let's start with this */
            *((u32 *)Value) = true;
            break;

        case (ADI_PID_CMD_ENABLE_DATAFLOW):
            break;

        case ADI_FSS_CMD_SET_CACHE_HEAP_ID:
            pDevice->CacheHeapID = (int)Value;
            break;

        case (ADI_PID_CMD_GET_FIXED):
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

        /* CASE ( Get overall sizes of teh media (as if one partition) ) */
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
            *(u32*)Value = pDevice->DataElementWidth;
            break;

        case ADI_PID_CMD_GET_MAX_TFRCOUNT:
            /* For a RAM disk the real limit is the ElementCount*ElementWidth 
             * value of the ADI_DEV_1D_BUFFER, 0xFFFFFFFF * 4, but we cannot
             * pass this value back, so we limit the size to 0xFFFFFFFF
            */
            *((u32 *)Value) = 0xFFFFFFFF;
            break;

        case ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE:
            adi_sem_Pend (pDevice->LockSemaphoreHandle, ADI_SEM_TIMEOUT_FOREVER);
            break;

        case ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE:
            adi_sem_Post (pDevice->LockSemaphoreHandle);
            break;

        case ADI_RAMDISK_CMD_SET_RAMDISK_START_LOCATION:
            pDevice->pRamDiskStart = (u8*)Value;
            break;

        case ADI_RAMDISK_CMD_SET_SIZE:
            SetGlobalVolumeDef(pDevice,(u32)Value);
            break;

        case ADI_RAMDISK_CMD_SET_USB_DELAY:
            pDevice->nUsbDelay = (u32)Value;
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
static u32 Activate(ADI_RAMDISK_DEF *pDevice, u32 EnableFlag)
{
    u32 Result = ADI_FSS_RESULT_SUCCESS;

    /* The reset is only performed once for all PID's in chain */
    if (!pDevice->Active)
    {
        pDevice->Active = true;
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
u32 DevicePollMedia(ADI_RAMDISK_DEF *pDevice)
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
#if 0
/*********************************************************************

    Function:       DetectVolumes

    Description:    Reads the partition table from the Master Boot Record

*********************************************************************/
u32 DetectVolumes(ADI_RAMDISK_DEF *pDevice, u32 Drive)
{
    u32 fs_type, Count=0, Result=ADI_FSS_RESULT_SUCCESS;

    ADI_ATA_PARTITION_ENTRY Primary[4];
    ADI_ATA_PARTITION_ENTRY *pPartTab;
    u8 *pMBR;

    /* Read the first 512 bytes from the media */
    pMBR = (u8*)ADI_RAMDISK_ADDRESS(pDevice->pRamDiskStart, 0);

    /* Copy the area where a partition table may exist
     * (copy is required to prevent misaligned access)
    */
    memcpy( Primary, &pMBR[446], 4*sizeof(ADI_ATA_PARTITION_ENTRY) );

    /* First, assume the partition table is valid, i.e. for HDD or CF or SD */
    pPartTab = &Primary[0];
    while (pPartTab->size!=0
                && pPartTab->type
                && (fs_type=AssignFileSystemType(pPartTab->type))!=ADI_FSS_FSD_TYPE_UNKNOWN
                )
    {
        if (pPartTab->type==ADI_ATA_VOLUME_TYPE_PRI_EXTENDED)
        {
            ProcessExtendedPartitions(pDevice, 0, pPartTab, &Count);
        }
        else
        {
            NewVolumeDef(pDevice, Drive, pPartTab->lba_start, pPartTab->size, AssignFileSystemType(pPartTab->type) );
            if (Result!=ADI_FSS_RESULT_SUCCESS) {
                break;
            }
            Count++;
        }
        pPartTab++;
    }

    if (Count==0)
    {
        if (pMBR[0] == 0xEB || pMBR[0] == 0xE9 ) {
            NewVolumeDef(pDevice, 0, 0, 0,ADI_FSS_FSD_TYPE_UNKNOWN);
        }
        Result = ADI_FSS_RESULT_FAILED;
    }

    return Result;
}

/*********************************************************************

    Function:       ProcessExtendedPartitions

    Description:    Iterate the Extended partitions

*********************************************************************/
static u32 ProcessExtendedPartitions(ADI_RAMDISK_DEF *pDevice, u32 Drive, ADI_ATA_PARTITION_ENTRY *pPartTabEntry, u32 *Count)
{
    /* Buffer for first sector in extended partition */
    u8 *sector;
    u32 PartitionOffset = pPartTabEntry->lba_start;

    /* Result code set to success for first time round while loop */
    u32 Result = ADI_FSS_RESULT_SUCCESS;

    ADI_ATA_PARTITION_ENTRY ExtPartTab[2];
    /* assign local pointer to the extended partition entry in the primary table */
    ADI_ATA_PARTITION_ENTRY *pEntry = pPartTabEntry;

    /* Recurse down the extended partition linked list */
    while (    Result==ADI_FSS_RESULT_SUCCESS
            && (
                   pEntry->type==ADI_ATA_VOLUME_TYPE_EXT_FAT16
                || pEntry->type==ADI_ATA_VOLUME_TYPE_PRI_EXTENDED
                )
    )
    {
        /* Read the first 512 bytes from the media */
        sector = (u8*)ADI_RAMDISK_ADDRESS(pDevice->pRamDiskStart, PartitionOffset);

        /* Copy the area where a partition table may exist
         * (copy is required to prevent misaligned access)
        */
        memcpy( ExtPartTab, &sector[446], 2*sizeof(ADI_ATA_PARTITION_ENTRY) );

        /* First Partition entry */
        pEntry = &ExtPartTab[0];
        if (pEntry->size!=0
                    && pEntry->type
                    && AssignFileSystemType(pEntry->type)!=ADI_FSS_FSD_TYPE_UNKNOWN
        )
        {
            /* A failure means that dynamic memory is exhauseted */
            pEntry->lba_start += PartitionOffset;
            NewVolumeDef(pDevice, Drive, pEntry->lba_start, pEntry->size, AssignFileSystemType(pEntry->type) );
            (*Count)++;
        }
        /* Increment to second partition table entry, which will either be
         * empty indicating the end of the linked list or will point to another
         * extended partition.
        */
        if (Result==ADI_FSS_RESULT_SUCCESS)
        {
            pEntry++;
            pEntry->lba_start += PartitionOffset;
            PartitionOffset = pEntry->lba_start;
        }

    }

    return Result;
}

/*********************************************************************

    Function:       NewVolumeDef

    Description:    Inform FSS of newly detected volume

*********************************************************************/
static void
NewVolumeDef(ADI_RAMDISK_DEF *pDevice, u32 Drive, u32 StartAddress, u32 VolumeSize, u32 FileSystemType)
{
    ADI_FSS_VOLUME_DEF VolumeDef;
    VolumeDef.StartAddress   = StartAddress;
    VolumeDef.VolumeSize     = VolumeSize;
    VolumeDef.SectorSize     = 512;
    VolumeDef.DeviceNumber   = Drive;
    if (FileSystemType!=ADI_FSS_FSD_TYPE_UNKNOWN) {
        VolumeDef.FileSystemType = FileSystemType;
    } else {
        VolumeDef.FileSystemType = ADI_FSS_FSD_TYPE_FAT;
    }

    if (FSSDirectCallbackFunction)
    {
        (FSSDirectCallbackFunction)( &pDevice->DeviceHandle, ADI_FSS_EVENT_VOLUME_DETECTED, (void *)&VolumeDef );
    } else {
        (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_FSS_EVENT_VOLUME_DETECTED, (void *)&VolumeDef );
    }

}

/*********************************************************************

    Function:       AssignFileSystemType

    Description:    Checks for valid volume types

*********************************************************************/
static u32 AssignFileSystemType(u32 type)
{
    switch(type) {
        case ADI_ATA_VOLUME_TYPE_PRI_FAT12:
        case ADI_ATA_VOLUME_TYPE_PRI_FAT16_A:
        case ADI_ATA_VOLUME_TYPE_PRI_FAT16_B:
        case ADI_ATA_VOLUME_TYPE_EXT_FAT16:
        case ADI_ATA_VOLUME_TYPE_PRI_FAT16_LBA:
        case ADI_ATA_VOLUME_TYPE_PRI_FAT32:
        case ADI_ATA_VOLUME_TYPE_PRI_FAT32_LBA:
        //case 0x72: /* windows formatted */
            return ADI_FSS_FSD_TYPE_FAT;

        case ADI_ATA_VOLUME_TYPE_PRI_UNASSIGNED:
        case ADI_ATA_VOLUME_TYPE_PRI_EXTENDED:
            return ADI_FSS_FSD_TYPE_UNASSIGNED;

        default:
            return ADI_FSS_FSD_TYPE_UNKNOWN;
    }
}
#endif
/*********************************************************************

    Function:       CallbackFromFSS

    Description:    The callback to be taken upon buffer completion and
                    device interrupt callbacks.
*********************************************************************/
static void CallbackFromFSS(void *pHandle, u32 Event, void *pArg)
{
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pArg;
    ADI_RAMDISK_DEF          *pDevice      = (ADI_RAMDISK_DEF *)pHandle;

    if (  Event == ADI_PID_EVENT_DEVICE_INTERRUPT
          && pSuperBuffer->SemaphoreHandle == pDevice->DataSemaphoreHandle )
    {
        adi_dev_Control( pDevice->DeviceHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
        adi_sem_Post( pSuperBuffer->SemaphoreHandle );
    }

    /* a delay is required to enable large copies from windows to be successful */
    WaitMicroSec(pDevice->nUsbDelay);
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
            adi_sem_Post(MemDmaSemaphoreHandle);
            break;

        case ADI_DMA_EVENT_ERROR_INTERRUPT:
            break;

        default:
            break;
    }

    /* return */
}


/*********************************************************************

    Function:       CopyToFromMemory

    Description:    reads or writes to RamDisk Memory

*********************************************************************/
static void CopyToFromMemory(ADI_RAMDISK_DEF *pDevice, ADI_FSS_SUPER_BUFFER *pFSSBuffer)
{
    u32 i;

    u32 SectorNumber = pFSSBuffer->LBARequest.StartSector;
    u32 SectorCount  = pFSSBuffer->LBARequest.SectorCount;

    u32 RamDiskOffset  = pDevice->GlobalVolumeDef.SectorSize * SectorNumber;

    u32 *pSource, *pDest;

    if (pFSSBuffer->LBARequest.ReadFlag)
    {
        pSource = (u32*)&pDevice->pRamDiskStart[RamDiskOffset];
        pDest   = (u32*)pFSSBuffer->Buffer.Data;
    }
    else
    {
        pDest   = (u32*)&pDevice->pRamDiskStart[RamDiskOffset];
        pSource = (u32*)pFSSBuffer->Buffer.Data;
    }
    
    for ( i=0; i<pFSSBuffer->Buffer.ElementCount; i++ )
    {
        pDest[i] = pSource[i];
    }

}

/*********************************************************************

    Function:       GetGlobalVolumeDef

    Description:    Assign and return pointer to index'd volume definition

*********************************************************************/
static void GetGlobalVolumeDef(ADI_RAMDISK_DEF *pDevice, ADI_FSS_VOLUME_DEF *pVolumeDef)
{
    pVolumeDef->StartAddress = pDevice->GlobalVolumeDef.StartAddress;
    pVolumeDef->VolumeSize   = pDevice->GlobalVolumeDef.VolumeSize;
    pVolumeDef->SectorSize   = pDevice->GlobalVolumeDef.SectorSize;
}


/*********************************************************************

    Function:       SetGlobalVolumeDef

    Description:    Assigns Global RamDisk metrics for the given size in 
                    512 byte sectors

*********************************************************************/
static void SetGlobalVolumeDef(ADI_RAMDISK_DEF *pDevice, u32 VolumeSize)
{
    pDevice->GlobalVolumeDef.StartAddress = 0;
    pDevice->GlobalVolumeDef.SectorSize   = 512;
    pDevice->GlobalVolumeDef.VolumeSize   = VolumeSize;
}


/*********************************************************************

    Function:       WaitOneMicroSec

    Description:    Inserts a delay for one microsecond

*********************************************************************/
static void WaitOneMicroSec(void)
{
volatile unsigned long long int cur,nd;

    _GET_CYCLE_COUNT(cur);

    nd = cur + (_Processor_cycles_per_sec/1000000);
    while (cur < nd) {
    _GET_CYCLE_COUNT(cur);
    }
}

/*********************************************************************

    Function:       WaitMicroSec

    Description:    Inserts a delay for usec microseconds

*********************************************************************/
static int WaitMicroSec(unsigned int usec)
{
    while (usec != 0) {
           WaitOneMicroSec();
           usec--;
    }
    return(0);
}

/************************************
 * definition structure for RAM Disk PID
 ************************************
 */
static ADI_FSS_DEVICE_DEF goRamPidDef;

static void FormatCallback(void *hArg, u32 Event, void *pArg);

extern ADI_DMA_MANAGER_HANDLE adi_dma_ManagerHandle;   // handle to the DMA manager
extern ADI_DEV_MANAGER_HANDLE adi_dev_ManagerHandle;   // handle to the device manager


ADI_FSS_DEVICE_DEF *ConfigRamDiskFssDeviceDef(
                            u8                      *pRamDiskFrame,     /* Pointer to memory to use as RAM */
                            u32                     nRamDiskSize,       /* Size of RAM Disk array in 512 byte sectors  */
                            ADI_DEV_MANAGER_HANDLE  hDevMgrHandle,      /* Device Manager Handle */
                            ADI_DCB_HANDLE          hDcbHandle,         /* DCB Queue handle */
                            u32                     nUsbDelay           /* Delay (microseconds) for USB connectivity */
)
{
    ADI_DEV_DEVICE_HANDLE hRamPidDeviceHandle;
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    ADI_DEV_DEVICE_HANDLE hFatDeviceHandle;

    if (pRamDiskFrame) {

        memset(&goRamPidDef, 0, sizeof(ADI_FSS_DEVICE_DEF));

        /* open the RANDISK PID */
        if (Result==ADI_DEV_RESULT_SUCCESS) {
            Result = adi_dev_Open(
                                    hDevMgrHandle,
                                    &ADI_RAMDISK_Entrypoint,
                                    0,
                                    NULL,
                                    &hRamPidDeviceHandle,
                                    ADI_DEV_DIRECTION_BIDIRECTIONAL,
                                    NULL,
                                    hDcbHandle,
                                    FormatCallback
                               );
        }

        /* These are the only two fields that are used when a PID is registered in this way */
        goRamPidDef.DeviceHandle = hRamPidDeviceHandle;
        goRamPidDef.DefaultMountPoint = 'r';


        /* Set data flow method */
        if (Result==ADI_DEV_RESULT_SUCCESS) {
            Result = adi_dev_Control( hRamPidDeviceHandle, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void *)ADI_DEV_MODE_CHAINED );
        }

        /*  Set the ramdisk start location */
        if (Result==ADI_FSS_RESULT_SUCCESS){
            Result = adi_dev_Control ( hRamPidDeviceHandle, ADI_RAMDISK_CMD_SET_RAMDISK_START_LOCATION, (void *)pRamDiskFrame);
        }

        /* Set the ramdisk size (if not set, default to smallest FAT16 partition) */
        if (Result==ADI_DEV_RESULT_SUCCESS) {
            if (nRamDiskSize) {
                Result = adi_dev_Control( hRamPidDeviceHandle, ADI_RAMDISK_CMD_SET_SIZE, (void*)nRamDiskSize );
            } else {
                Result = adi_dev_Control( hRamPidDeviceHandle, ADI_RAMDISK_CMD_SET_SIZE, (void*)8401 );
            }
        }

        /* Set the ramdisk size */
        if (nUsbDelay && Result==ADI_DEV_RESULT_SUCCESS) {
            Result = adi_dev_Control( hRamPidDeviceHandle, ADI_RAMDISK_CMD_SET_USB_DELAY, (void*)nUsbDelay );
        }

        /* Set the ramdisk format */
        if (Result==ADI_DEV_RESULT_SUCCESS) {

            /* Volume definition structure */
            ADI_FSS_VOLUME_DEF oRamDiskVolumeDef;
            /* Format Definition structure */
            ADI_FSS_FORMAT_DEF oFormatDef;

            /* Get Volume definition from RamDisk PID */
            if (Result==ADI_DEV_RESULT_SUCCESS) {
                Result = adi_dev_Control( hRamPidDeviceHandle, ADI_PID_CMD_GET_GLOBAL_MEDIA_DEF, (void*)&oRamDiskVolumeDef );
            }

            /* Activate the RamDisk PID */
            if (Result==ADI_DEV_RESULT_SUCCESS) {
                Result = adi_dev_Control( hRamPidDeviceHandle, ADI_PID_CMD_MEDIA_ACTIVATE, (void*)true );
            }

            /* Open the FAT driver */
            if (Result==ADI_DEV_RESULT_SUCCESS) {


                Result = adi_dev_Open(
                                    adi_dev_ManagerHandle,
                                    &ADI_FAT_EntryPoint,
                                    0,
                                    NULL,
                                    &hFatDeviceHandle,
                                    ADI_DEV_DIRECTION_BIDIRECTIONAL,
                                    NULL,
                                    hDcbHandle,
                                    FormatCallback
                             );
            }

            /* register the RamDisk PID handle with the FAT driver */
            if (Result==ADI_DEV_RESULT_SUCCESS) {
                Result = adi_dev_Control( hFatDeviceHandle, ADI_FSD_CMD_SET_PID_HANDLE,(void*)hRamPidDeviceHandle);
            }

            if (Result==ADI_DEV_RESULT_SUCCESS) {
                adi_dev_Control( hFatDeviceHandle, ADI_FAT_CMD_SET_FAT_CACHE_SIZE,(void*)4);
                adi_dev_Control( hFatDeviceHandle, ADI_FAT_CMD_SET_DIR_CACHE_SIZE,(void*)2);
            }

            /* Format the volume */
            if (Result==ADI_DEV_RESULT_SUCCESS) {
                /* copy parameters from PID volume definition structure */
                oFormatDef.VolumeDef = oRamDiskVolumeDef;
                /* Set File System type to FAT */
                oFormatDef.VolumeDef.FileSystemType = ADI_FSS_FSD_TYPE_FAT;
                /* Set options to specify FAT 16 */
                oFormatDef.OptionMask = ADI_FSS_FMT_OPTION_VALUE(ADI_FSS_FSD_TYPE_FAT,1);
                /* set the volume label */
                oFormatDef.label = "FAT16RAM";
                oFormatDef.label_len = 8;

                /* Instruct the FAT driver to format the volume */
                Result = adi_dev_Control(hFatDeviceHandle, ADI_FSD_CMD_FORMAT_VOLUME, (void*)&oFormatDef );
            }

            /* De-activate the PID - it will be reactivated by FSS for USB MSD */
            if (Result==ADI_DEV_RESULT_SUCCESS) {
                Result = adi_dev_Control( hRamPidDeviceHandle, ADI_PID_CMD_MEDIA_ACTIVATE, (void*)false );
            }

            /* Close the FAT driver */
            adi_dev_Close( hFatDeviceHandle );

        }

        if (Result==ADI_DEV_RESULT_SUCCESS) {
            return &goRamPidDef;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

/* macros to encode CompletionBitMask Field */
#define __DEV_INT_MASK  0x00000001
#define __BUF_PROC_MASK 0x00000002
#define __ADI_FSS_MASK_ 0xADF50000

#define SET_DEV_INT(P)  \
        { \
            if ( ((P)&__ADI_FSS_MASK_)==__ADI_FSS_MASK_ ) { \
                (*(u32*)&(P)) |= __DEV_INT_MASK; \
            } \
        }
#define SET_BUF_PROC(P)  \
        { \
            if ( ((P)&__ADI_FSS_MASK_)==__ADI_FSS_MASK_ ) { \
                (*(u32*)&(P)) |= __BUF_PROC_MASK; \
            } \
        }

/*********************************************************************

    Function:       FormatCallback

    Description:    Callback Function for the application

*********************************************************************/
static void FormatCallback(void *hArg, u32 Event, void *pArg)
{
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pArg;

    /* CASEOF ( Event flag ) */
    switch (Event)
    {
        /* CASE (Device Interrupt Event) */
        case (ADI_PID_EVENT_DEVICE_INTERRUPT):
        case (ADI_DEV_EVENT_BUFFER_PROCESSED):
            /* Set completion flags */
            if (Event == ADI_PID_EVENT_DEVICE_INTERRUPT) {
                SET_DEV_INT(pSuperBuffer->CompletionBitMask);
            }
            else {
                SET_BUF_PROC(pSuperBuffer->CompletionBitMask);
            }
            /* Call FSD callback function */
            if (pSuperBuffer->PIDCallbackFunction) {
                (pSuperBuffer->PIDCallbackFunction)( pSuperBuffer->PIDCallbackHandle, Event, pArg );
            }
            if (pSuperBuffer->FSDCallbackFunction) {
                (pSuperBuffer->FSDCallbackFunction)( pSuperBuffer->FSDCallbackHandle, Event, pArg );
            }
            break;

        /* CASE (Media removal event) */
        case (ADI_FSS_EVENT_VOLUME_DETECTED):
            break;

        /* END CASEOF */
    }
}


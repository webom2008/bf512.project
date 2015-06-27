/*********************************************************************************

Copyright(c) 2009 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_fss.c,v $
$Revision: 3939 $
$Date: 2010-12-10 09:07:11 -0500 (Fri, 10 Dec 2010) $

Description:
                This is the main source file for the ADI File System Service (FSS).

*********************************************************************************/

#include <services/fss/adi_fss.h>
#include <drivers/pid/adi_ata.h>    /* ATA structures, comamnds and macros */
#include <stdlib.h>
#include <string.h>

#ifdef __USE_MEM_PLACEMENT__
#define __ADI_FSS_SECTION_CODE    section("adi_fss_code")
#define __ADI_FSS_SECTION_DATA    section("adi_fss_data")
#else
#define __ADI_FSS_SECTION_CODE
#define __ADI_FSS_SECTION_DATA
#endif


/*********************************************************************
This flag allows the user to perform the "format" function on the
storage devices whose PID's accept the format command. It does increase
code size should it be enabled. If the User needs to reduce code size
they can undefine the ADI_USE_FAT_FORMAT flag and either re-compile
the proper library or include the adi_fss.c file in their project
and recompile their project with that file included. Doing so will
remove the support for the "format" function, thus reducing code size.
This define is located in two places, the file adi_fss.c and adi_fss.h.
BOTH locations need to be changed for the change to take effect.
*********************************************************************/
#if !defined(ADI_USE_FAT_FORMAT)
#define ADI_USE_FAT_FORMAT 1
#endif

/*********************************************************************
* Cache Control
*********************************************************************/

#include "adi_fss_cache.h"

#if !defined(CACHE_DISABLED)
#define CACHE_ENABLED               (1)     // enables the cache
#endif

#define CACHE_BLOCK_COUNT           (4)     // number of blocks for each cache
#define CACHE_CLUSTERS_PER_BLOCK    (1)     // number of clusters in each block

__ADI_FSS_SECTION_DATA
static u32 gSemTimeOutArg = ADI_SEM_TIMEOUT_FOREVER;
__ADI_FSS_SECTION_DATA
static u32 gMaxRetryCount = 0;

static u32 gAlignmentOption = ADI_FSS_MEM_ALIGN;

#define NOBLOCK 0
#define BLOCK   1
#define READ    1
#define WRITE   0

/*********************************************************************
* Device Types
*********************************************************************/

__ADI_FSS_SECTION_DATA
typedef enum {
    ADI_FSS_DEVICE_TYPE_UNKNOWN = 0,
    ADI_FSS_DEVICE_TYPE_PID     = 1,
    ADI_FSS_DEVICE_TYPE_FSD     = 2,
    ADI_FSS_DEVICE_TYPE_REGISTERED_PID     = 3,
} ADI_FSS_DEVICE_TYPE;

/*******************************************************************
* Structure for Device in device linked list
*******************************************************************/
typedef struct adi_fss_device_list {
    ADI_FSS_DEVICE_DEF          *pDev;
    ADI_FSS_DEVICE_TYPE         Type;
    struct adi_fss_device_list  *pNext;
} ADI_FSS_DEVICE_LIST;

/*******************************************************************
* Structure for each entry in the Mounted Volume linked list
*******************************************************************/
typedef struct ADI_FSS_MOUNT_DEF {
    ADI_FSS_VOLUME_DEF          VolumeDef;                  /* Volume Definition                                */
    ADI_FSS_VOLUME_IDENT        MountPoint;                 /* Unique volume mount point, NULL indicates volume
                                                                has not been mounted */
    ADI_DEV_DEVICE_HANDLE       FSD_DeviceHandle;           /* Handle to File System driver                     */
    ADI_DEV_DEVICE_HANDLE       PID_DeviceHandle;           /* Handle to Media Device driver                    */
    ADI_FSS_FULL_FNAME          *pCurrentWorkingDirectory;  /* Full path name of Current working directory      */
    struct ADI_FSS_MOUNT_DEF    *pNext;
} ADI_FSS_MOUNT_DEF;

/*******************************************************************
* Structure for each entry in the Mounted Volume linked list
*******************************************************************/
typedef struct ADI_FSS_POLLMEDIA_DEF {
    void                        *Handle;
    ADI_DCB_CALLBACK_FN         Callback;
} ADI_FSS_POLLMEDIA_DEF;

/*********************************************************************
*   Linked list of PID and FSD Devices
*********************************************************************/
__ADI_FSS_SECTION_DATA
static ADI_FSS_DEVICE_LIST      *pDeviceList=NULL;

/*********************************************************************
*   Linked list of mounted partitions
*********************************************************************/
__ADI_FSS_SECTION_DATA
static ADI_FSS_MOUNT_DEF        *pMountList=NULL;

/*********************************************************************
*   Pointer to the Volume that is currently active
*********************************************************************/
__ADI_FSS_SECTION_DATA
static ADI_FSS_MOUNT_DEF        *pActiveVolume=NULL;

/*********************************************************************
*   Structure defining Media Change Timer
*********************************************************************/
__ADI_FSS_SECTION_DATA
static ADI_FSS_POLLMEDIA_DEF    PollMedia={NULL,NULL};

/*********************************************************************
*   Path name separators
*********************************************************************/

#define ADI_FSS_DEFAULT_VOLUME_SEPARATOR    0x3A  /* : */
#define ADI_FSS_DEFAULT_DIRECTORY_SEPARATOR 0x2F  /* / */

static ADI_FSS_WCHAR volume_separator = (ADI_FSS_WCHAR)ADI_FSS_DEFAULT_VOLUME_SEPARATOR;
static ADI_FSS_WCHAR directory_separator = (ADI_FSS_WCHAR)ADI_FSS_DEFAULT_DIRECTORY_SEPARATOR;

static void *_adi_fss_pEnterCriticalArg = NULL;

/*********************************************************************
*   Validate the filehandle to make sure that the Volume hasn't
*   unmounted sice opening the file, this has to be enabled by
*   defining ADI_FSS_VALIDATE_FILE_HANDLE
*********************************************************************/
#ifdef ADI_FSS_VALIDATE_FILE_HANDLE
u32 _adi_fss_validate_filehandle(ADI_FSS_FILE_DESCRIPTOR *pHandle)
{
    ADI_FSS_MOUNT_DEF *pMount;

    pMount=pMountList;

    while(pMount&&pMount->FSD_DeviceHandle!=pHandle->FSD_device_handle)
    {
        pMount=pMount->pNext;
    }
    return pMount?ADI_FSS_RESULT_SUCCESS:ADI_FSS_RESULT_BAD_VOLUME;
}
#else
#define _adi_fss_validate_filehandle(x) ADI_FSS_RESULT_SUCCESS
#endif

/*********************************************************************
*   Static Function Prototypes
*********************************************************************/

static ADI_DEV_DEVICE_HANDLE AssignFileSystemDevice(u32 type);
static u32 SplitPath(const ADI_FSS_WCHAR *path, u32 pathlen, ADI_FSS_FULL_FNAME **pFile, ADI_FSS_MOUNT_DEF **pVolume);
static u32 MergePaths(ADI_FSS_FULL_FNAME **pPath1, ADI_FSS_FULL_FNAME **pPath2);
void _adi_fss_CopyPath(ADI_FSS_FULL_FNAME *pDest, ADI_FSS_FULL_FNAME *pSource);
static void FreePathList(ADI_FSS_FULL_FNAME *pFile);
static u32 FormPathString(ADI_FSS_FULL_FNAME *pFile, ADI_FSS_WCHAR *path, u32 *pathlen );
static ADI_FSS_MOUNT_DEF *LookUpVolume(const ADI_FSS_VOLUME_IDENT VolumeID);
static u32 DuplicateUnicodeString(ADI_FSS_WCHAR **pDest, ADI_FSS_WCHAR *pSrc, u32 len);

static void FSSCallback(void *hArg, u32 Event, void *pArg);
static u32 DirectoryOpenCreate(ADI_FSS_WCHAR *name, u32 namelen, ADI_FSS_DIR_HANDLE *DirHandle, u32 mode);
static u32 GetVolumeInfo(ADI_FSS_VOLUME_INFO* pVolumes );
static void ProcessDetectedVolume( ADI_DEV_DEVICE_HANDLE PID_DeviceHandle, ADI_FSS_VOLUME_DEF *pVolumeDef );

#if defined(ADI_USE_FAT_FORMAT)
static u32 _adi_fss_FormatVolume( ADI_FSS_FORMAT_DEF *pFormatDef );
#endif

static u32 UnMountDevice( ADI_DEV_DEVICE_HANDLE DeviceHandle );

static ADI_FSS_FILE_DESCRIPTOR *CreateFileDescriptor(
    ADI_FSS_WCHAR *name,               /* Unicode UTF-8 array identifying file to open     */
    u32 namelen                        /* Length of name array                              */
);
static void DestroyFileDescriptor(
    ADI_FSS_FILE_DESCRIPTOR *pFileDesc
);

/* ******************************************************************************
 * COMMON PID FUNCTIONALITY
 * ******************************************************************************
*/

/* global variables for the current device */
static ADI_DEV_DEVICE_HANDLE   f_pidDeviceHandle;
static u32                     f_pidDriveNumber;
static u32                     f_pidSectorSize;
static u32                     f_pidDataElementWidth;
static ADI_SEM_HANDLE          f_pidhSemaphore;
static int                     f_pidHeapID;

static void NewVolumeDef(
                    u32 FileSystemType,
                    u32 StartAddress,
                    u32 VolumeSize
                    );

static u32 GetFileSystemType(u32 type);
static u32 ProcessExtendedPartitions(ADI_ATA_PARTITION_ENTRY *pPartTabEntry, u32 *Count);
static u32 IsValidFATPartition(u8 *pData);
static void WaitOneMilliSec(void);
static void WaitOneMicroSec(void);

static void FlushArea(void *StartAddress, void *EndAddress);
static void FlushInvArea(void *StartAddress, void *EndAddress);
static bool IsCPLBEnabled(void);
static bool IsAlignmentRequired(int id);

/* ******************************************************************************
 * ******************************************************************************
*/


/*********************************************************************
* exportable non-public (ie non advertized) functions
*********************************************************************/

u32 _adi_fss_WriteBuffer(ADI_DEV_DEVICE_HANDLE DeviceHandle, void *pData, s32 size, void* pAddInfo, void *pCriticalRegionData);
u32 _adi_fss_WriteTextStream( ADI_DEV_DEVICE_HANDLE DeviceHandle, unsigned char *buf, s32 size, void* pAddInfo, void *pCriticalRegionData);
u32 _adi_fss_ProcessTextStream(u8 *buf, u32 size);

#define ADI_FSS_DEVICE_DEF_ENTRY(T,I) ( ((ADI_FSS_DEVICE_DEF **)(T).entries)[(I)] )

/*********************************************************************
*   Memory allocation functions. Default to malloc/free
*********************************************************************/

#if defined(__ECC__)

/* For VisualDSP++ Environment separate heaps can be used
*/

typedef void* (*ADI_FSS_MALLOC_FUNC) (int, size_t);
typedef void* (*ADI_FSS_REALLOC_FUNC) (int, void *,size_t);
typedef void  (*ADI_FSS_FREE_FUNC) (int, void*);

ADI_FSS_MALLOC_FUNC _adi_fss_malloc_func = _heap_malloc;
ADI_FSS_REALLOC_FUNC _adi_fss_realloc_func = _heap_realloc;
ADI_FSS_FREE_FUNC _adi_fss_free_func = _heap_free;

#else

/* For other environments, this may vary and so the default
 * situation in this instance is to use the standard C library
 * functions:
*/

typedef void* (*ADI_FSS_MALLOC_FUNC) (size_t);
typedef void* (*ADI_FSS_REALLOC_FUNC) (void *,size_t);
typedef void  (*ADI_FSS_FREE_FUNC) (void*);

__ADI_FSS_SECTION_DATA
ADI_FSS_MALLOC_FUNC _adi_fss_malloc_func = malloc;
__ADI_FSS_SECTION_DATA
ADI_FSS_REALLOC_FUNC _adi_fss_realloc_func = realloc;
__ADI_FSS_SECTION_DATA
ADI_FSS_FREE_FUNC _adi_fss_free_func = free;

#endif

void _adi_fss_free(int id,  void *p );
void *_adi_fss_malloc(int id,  size_t size );
void *_adi_fss_realloc( int id, void *p, size_t size );

void *_adi_fss_MallocAligned( int id, size_t size );
void *_adi_fss_ReallocAligned( int id, void *p, size_t size );
void _adi_fss_FreeAligned( int id, void *p );

void _adi_fss_FlushMemory( char *StartAddress, u32 NumBytes, ADI_DMA_WNR Direction);


/*********************************************************************
*   Pend function.
*********************************************************************/

typedef u32  (*ADI_FSS_PEND_FUNC) (void*, volatile u32*, u32);
u32 _adi_fss_PendOnFlag( void *pArg, volatile u32 *pFlag, u32 Value );

/* Default case is not to simply spin-lock */
ADI_FSS_PEND_FUNC _adi_fss_pend_func = NULL;

void *_adi_fss_PendArgument = NULL;

/*********************************************************************
*   Generic Transfer Functions.
*********************************************************************/
u32 _adi_fss_PIDTransfer( ADI_DEV_DEVICE_HANDLE PIDHandle, ADI_FSS_SUPER_BUFFER *pBuffer, u32 BlockFlag );

/********************************************************************
* Deferred Callback queue, DMA and Device Manager Handles
* are defined by the SSL initialization.
*********************************************************************/


__ADI_FSS_SECTION_DATA
static ADI_DMA_MANAGER_HANDLE DMAManagerHandle = NULL;      /*  handle to the DMA manager      */
__ADI_FSS_SECTION_DATA
static ADI_DEV_MANAGER_HANDLE DeviceManagerHandle = NULL;   /*  handle to the device manager   */
__ADI_FSS_SECTION_DATA
static ADI_DCB_HANDLE         DCBQueueHandle  = NULL;       /*  handle to the DCB queue server */

__ADI_FSS_SECTION_DATA
static int GeneralHeapID = 0;
__ADI_FSS_SECTION_DATA
static int CacheHeapID = -1;
__ADI_FSS_SECTION_DATA
static int CacheBlockCount = CACHE_BLOCK_COUNT;  /* default number of cache blocks */
__ADI_FSS_SECTION_DATA
static int CacheSubBlockCount = CACHE_CLUSTERS_PER_BLOCK;  /* default number of cache sub-blocks */

__ADI_FSS_SECTION_DATA
static u32 gOpenFileCount = 0;

#include <cplb.h>

/*============= D E F I N E S =============*/

/*
** Macros for flushing and invalidating data buffers in cached memory.
** Used in functions FlushArea() and FlushInvArea,
*/
#define FLUSH(P)           {asm volatile("FLUSH[%0++];":"+p"(P));}
#define FLUSHINV(P)        {asm volatile("FLUSHINV[%0++];":"+p"(P));}

/* Aligns memory to next 32 byte boundary */
#define ALIGN_MEM32(P)       (((u32)(P) + 32) & ~(0x1F))

static bool IsCPLBEnabled(void);


/*=============  E X T E R N A L S  ============*/

/*
** Variable: __cplb_ctrl
** CPLB CONTROL value defind in cplb library module
*/
extern int __cplb_ctrl;

/*============= D A T A =============*/

/*
** Variable: f_bCPLBEnabled
** Boolean flag t
*/
static bool f_bCPLBEnabled = false;
static bool f_bCPLBEnabledFlagSet = false;


/************************************************************************************
*************************************************************************************

                            PUBLIC (API) FUNCTIONS

*************************************************************************************
*************************************************************************************/

/*********************************************************************

    Function:       adi_fss_Init

    Description:    Initializes the File System Service

*********************************************************************/
__ADI_FSS_SECTION_CODE
u32
adi_fss_Init (          /* Initialize File System Service */
    ADI_FSS_CMD_VALUE_PAIR *pTable         /* Address of Command Table                      */
)
{
    u32 Result,Check;
    ADI_FSS_DEVICE_LIST *pDevice;
    ADI_FSS_CMD_VALUE_PAIR *pPair;

    /* Assign command-value pair pointer to first in table */
    pPair = &pTable[0];

    Result=ADI_FSS_RESULT_SUCCESS;
    Check=0;

    // process each command pair
    while (Result==ADI_FSS_RESULT_SUCCESS && pPair->CommandID!=ADI_FSS_CMD_END)
    {
        /* CASEOF (command ID) */
        switch(pPair->CommandID)
        {

        case ADI_FSS_CMD_SET_CACHE_HEAP_ID:
            CacheHeapID = (int)pPair->Value;
            break;

        case ADI_FSS_CMD_SET_GENERAL_HEAP_ID:
            GeneralHeapID = (int)pPair->Value;
            break;

        case ADI_FSS_CMD_SET_NUMBER_CACHE_BLOCKS:
            CacheBlockCount = (int)pPair->Value;
            break;

        case ADI_FSS_CMD_SET_NUMBER_CACHE_SUB_BLOCKS:
            CacheSubBlockCount = (int)pPair->Value;
            break;

        /* CASE (Add a driver definition to the table of media devices) */
        case ADI_FSS_CMD_ADD_DRIVER:
        {
            pDevice=_adi_fss_malloc(-1,sizeof(ADI_FSS_DEVICE_LIST));
            if (pDevice)
            {
                /* copy device definition */
                pDevice->pDev=(ADI_FSS_DEVICE_DEF *)pPair->Value;
                pDevice->Type=ADI_FSS_DEVICE_TYPE_UNKNOWN;
                pDevice->pNext=pDeviceList;
                pDeviceList=pDevice;
            }
            else
            {
                Result=ADI_FSS_RESULT_NO_MEMORY;
            }
            break;
        }

        /* CASE (Set Memory alignment option) */
        case ADI_FSS_CMD_SET_MEM_ALIGNMENT:
            /* can only be done if no calls to _adi_fss_malloc already made */
            if (!pDeviceList) {
                gAlignmentOption = (u32)pPair->Value;
            }
            break;

        /* CASE (Set Device Manager Handle) */
        case ADI_FSS_CMD_SET_DEV_MGR_HANDLE:
            Check|=0x04;

        /* CASE (default) */
        default:
            Result=adi_fss_Control(pPair->CommandID,pPair->Value);
        }
        pPair++;
    }

    /* Now workout the device type for each device in the devicelist */
    pDevice = pDeviceList;
    while (pDevice&&Result==ADI_FSS_RESULT_SUCCESS)
    {
        ADI_DEV_DEVICE_HANDLE pHandle;

        if (!pDevice->pDev->DeviceHandle)
        {
            /* Open the device */
            Result = adi_dev_Open(
                DeviceManagerHandle,
                pDevice->pDev->pEntryPoint,
                0,
                &pDevice->pDev->DeviceHandle,
                &pDevice->pDev->DeviceHandle,
                ADI_DEV_DIRECTION_BIDIRECTIONAL,
                DMAManagerHandle,
                DCBQueueHandle,
                FSSCallback
            );
            if (Result==ADI_FSS_RESULT_SUCCESS)
            {
                /*
                 * determine the type of device by executing a command only supported by PIDs.
                 * if ADI_FSS_RESULT_SUCCESS is returned then the driver is a PID device,
                 * otherwise we have to assume it's an FSD. The actual value of the IsFixed
                 * identifier is not required, but its address needs to be passed to avoid the
                 * erroneous assignment of location 0 in external memory.
                 */
                u32 IsFixed = FALSE;
                Result = adi_dev_Control(pDevice->pDev->DeviceHandle,ADI_PID_CMD_GET_FIXED,(void *)&IsFixed);

                if (Result==ADI_FSS_RESULT_SUCCESS) {
                    pDevice->Type = ADI_FSS_DEVICE_TYPE_PID;
                    Check |= 0x01;
                } else {
                    pDevice->Type = ADI_FSS_DEVICE_TYPE_FSD;
                    adi_dev_Close(pDevice->pDev->DeviceHandle);
                    pDevice->pDev->DeviceHandle = NULL;
                    Check |= 0x02;
                }
                Result=ADI_FSS_RESULT_SUCCESS;
            }
        }
        else {

            /* Change the client handle */
            Result = adi_dev_Control ( pDevice->pDev->DeviceHandle, ADI_DEV_CMD_CHANGE_CLIENT_HANDLE, (void*)&pDevice->pDev->DeviceHandle );

            /* device is already resgistered */
            if ((pDevice->Type&ADI_FSS_DEVICE_TYPE_PID)==ADI_FSS_DEVICE_TYPE_PID)
            {
                Check |= 0x01;
            }
            else {
                Check |= 0x02;
            }
        }
        pDevice = pDevice->pNext;
    }

    /* Check that all required information is available */
    if ( (Check&0x6)!=0x06 )
    {
        Result = ADI_FSS_RESULT_FAILED;
    }

    /* Initialize each media device and activate */
    pDevice = pDeviceList;
    while (pDevice && Result==ADI_FSS_RESULT_SUCCESS)
    {
        if ( (pDevice->Type&ADI_FSS_DEVICE_TYPE_PID)==ADI_FSS_DEVICE_TYPE_PID)
        {
            if (pDevice->Type!=ADI_FSS_DEVICE_TYPE_REGISTERED_PID)
            {

                /* set up the device driver for chained dataflow */
                Result = adi_dev_Control(
                                            pDevice->pDev->DeviceHandle,
                                            ADI_DEV_CMD_SET_DATAFLOW_METHOD,
                                            (void *)ADI_DEV_MODE_CHAINED
                                        );

                /* Configure the Device Driver */
                if (Result==ADI_FSS_RESULT_SUCCESS && pDevice->pDev->pConfigTable)
                {
                    Result = adi_dev_Control(   pDevice->pDev->DeviceHandle,
                                                ADI_DEV_CMD_TABLE,
                                                (void *)pDevice->pDev->pConfigTable
                                            );
                }

                if (Result==ADI_DEV_RESULT_SUCCESS)
                {
                    /* activate device */
                    Result = adi_dev_Control(
                                                pDevice->pDev->DeviceHandle,
                                                ADI_PID_CMD_MEDIA_ACTIVATE,
                                                (void*)TRUE
                                          );
                } else
                {
                    Result=ADI_FSS_RESULT_INVALID_DEVICE; // ????? abort all volumes ???????
                }
            }

            /* *** dont abort if any volume fail to initialize *** */
            Result=ADI_FSS_RESULT_SUCCESS;
        }
        pDevice=pDevice->pNext;
    }

    /* If the Initialization failed then free up any memory allocated */
    if (Result!=ADI_FSS_RESULT_SUCCESS)
    {
        while (pDeviceList)
        {
            ADI_FSS_DEVICE_LIST *pDevice;

            pDevice = pDeviceList;
            pDeviceList = pDeviceList->pNext;
            _adi_fss_free(-1,pDevice);
        }
    }

    adi_fss_PollMedia();

    return Result;
}


/*********************************************************************

    Function:       adi_fss_Terminate

    Description:    Terminates the File System Service, unmounting all
                    media and closing all open device drivers. Open
                    files will not be closed tidily.

*********************************************************************/
__ADI_FSS_SECTION_CODE
u32
adi_fss_Terminate (          /* Terminate File System Service */
                    void
)
{
    ADI_FSS_DEVICE_LIST *pDevice, *pCurrDevice;
    ADI_FSS_MOUNT_DEF *pMount, *pCurrMount;
    u32 Result;

    if (gOpenFileCount) {
        Result = ADI_FSS_RESULT_FAILED;
    }
    else
    {
        /* Iterate mounted volume list and remove mounts */

        pMount = pMountList;
        while (pMount)
        {
            /* Close file system driver */
            if (pMount->FSD_DeviceHandle)
            {
                adi_dev_Close(pMount->FSD_DeviceHandle);
            }
            /* Close physical interface driver */
            if (pMount->PID_DeviceHandle)
            {
                adi_dev_Close(pMount->PID_DeviceHandle);
            }
            pCurrMount = pMount;
            pMount = pMount->pNext;

            /* Free pMount and related memory*/
            FreePathList(pCurrMount->pCurrentWorkingDirectory);
            _adi_fss_free(-1,pCurrMount);

        }

        /* reset mount point list pointer */
        pMountList = NULL;
        /* Iterate device list */

        pDevice = pDeviceList;
        while (pDevice)
        {
            pCurrDevice = pDevice;
            pDevice = pDevice->pNext;
            pCurrDevice->pDev->DeviceHandle = NULL;
            _adi_fss_free(-1,pCurrDevice);
        }

        /* reset device list pointer */
        pDeviceList = NULL;

        /* Clear Active volume pointer */
        pActiveVolume=NULL;
        Result = ADI_FSS_RESULT_SUCCESS;

    }

    return Result;

}


/*********************************************************************

    Function:       adi_fss_FileOpen

    Description:    Opens the named file for access according to the
                    mode argument. Returns File Descriptor Handle

*********************************************************************/
__ADI_FSS_SECTION_CODE
u32
adi_fss_FileOpen (      /* Open a file stream */
    ADI_FSS_WCHAR *name,                /* Unicode UTF-8 array identifying file to open     */
    u32 namelen,                        /* Length of Unicode array                          */
    u32 mode,                           /* Mode in which file is to be opened               */
    ADI_FSS_FILE_HANDLE *pFileHandle    /* location to store Handle identifying file stream */
)
{
    u32 Result;
    u32 UseFileCache;
    ADI_FSS_MOUNT_DEF *pVolume;
    ADI_FSS_FULL_FNAME *pFile;
    ADI_FSS_FILE_DESCRIPTOR *pFileDesc;
    u32 ulblocksize = 0;
    u32 bIsSpace = false;

    pVolume = pActiveVolume;
    pFileDesc = 0;

    Result=ADI_FSS_RESULT_SUCCESS;

    /* Determine volume from name. If not specified assume first in volume table.
       Load up directory name linked list
     */
    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result = SplitPath(name, namelen, &pFile, &pVolume);
    }

    /* Create a File Descriptor */
    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        pFileDesc = (ADI_FSS_FILE_DESCRIPTOR*)_adi_fss_malloc(-1,sizeof(ADI_FSS_FILE_DESCRIPTOR));
        if (!pFileDesc) {
            Result = ADI_FSS_RESULT_NO_MEMORY;
        }
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Assign name */
        pFileDesc->pFullFileName = pFile;

        /* Assign mode */
        pFileDesc->mode = mode;

        /* Assign File System Device Driver Handle */
        pFileDesc->FSD_device_handle = pVolume->FSD_DeviceHandle;

        /* no cache yet */
        pFileDesc->Cache_data_handle = NULL;

    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result = adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSD_CMD_GET_BLOCK_SIZE, (void*)&ulblocksize );
        Result = adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_GET_BACKGRND_XFER_SUPPORT, (void*)&UseFileCache );
        /* attempt to initialize the file cache ahead of the call to the FSD, to prevent the latter from
        * creating a file that we cannot use.
        */
        if ( UseFileCache ) {

            /* initalize the cache */
            Result = adi_fss_CacheInit (pFileDesc, CacheHeapID, CacheBlockCount, CacheSubBlockCount, ulblocksize, gSemTimeOutArg, gMaxRetryCount);
        }

        if (Result==ADI_FSS_RESULT_SUCCESS)
        {
            /* Instruct FSD to open the file */
            adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
            Result = adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSD_CMD_OPEN_FILE, (void*)pFileDesc );
            adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
        }

        /* grab block size to size the cache. */
        /* we are going to default the number of sub-buffers*/
        if ( UseFileCache && Result==ADI_FSS_RESULT_SUCCESS )
        {
            /* precharge the cache */
            Result = adi_fss_CachePrecharge(pFileDesc, pFileDesc->curpos);
            /* Failed to precharge the file so do garbage collection */
            if ( Result != ADI_FSS_RESULT_SUCCESS )
            {
                /* close file with FSD */
                adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
                adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSD_CMD_CLOSE_FILE, (void*)pFileDesc );
                adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
                /* destroy cache */
                adi_fss_CacheTerminate (pFileDesc);

            }
        }
        else {
            /* FSD failed to open the file so close the file cache */
            if ( NULL != (pFileDesc->Cache_data_handle) )
            {
                /* destroy cache */
                adi_fss_CacheTerminate (pFileDesc);
            }

        }
        if (Result==ADI_FSS_RESULT_SUCCESS)
        {
            /* Assign File Descriptor Handle */
            *pFileHandle = (ADI_FSS_FILE_HANDLE)pFileDesc;
            gOpenFileCount++;
        }
        else
        {
            if (pFile)
            {
                FreePathList(pFile);
            }
            if (pFileDesc)
            {
                _adi_fss_free(-1,pFileDesc);
            }
        }
   }


    return Result;
}

/*********************************************************************

    Function:       adi_fss_FileClose

    Description:    Closes the file, and frees memory

*********************************************************************/
__ADI_FSS_SECTION_CODE
u32
adi_fss_FileClose(      /* Close a file stream */
    ADI_FSS_FILE_HANDLE FileHandle     /* Handle identifying file stream                   */
)
{
    u32 Result;
    ADI_FSS_FILE_DESCRIPTOR *pFileDesc;

    pFileDesc = (ADI_FSS_FILE_DESCRIPTOR *)FileHandle;

#ifdef ADI_SSL_DEBUG
    Result=pFileDesc?ADI_FSS_RESULT_SUCCESS:ADI_FSS_RESULT_BAD_FILE_HANDLE;
#else
    Result=ADI_FSS_RESULT_SUCCESS;
#endif

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result=_adi_fss_validate_filehandle(FileHandle);
    }

    if ( NULL != (pFileDesc->Cache_data_handle) )
    {
        /* destroy cache */
        Result = adi_fss_CacheTerminate (pFileDesc);
    }

    /* close the file */
    adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
    Result = adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSD_CMD_CLOSE_FILE, (void*)pFileDesc );
    adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );

    /* free memory in linked list */
    if (pFileDesc)
    {
        if (pFileDesc->pFullFileName)
        {
            FreePathList(pFileDesc->pFullFileName);
        }
        /* Free memory for File Descriptor */
        _adi_fss_free(-1,pFileDesc);
    }

#ifdef ADI_SSL_DEBUG
    Result=Result?ADI_FSS_RESULT_CLOSE_FAILED:Result;
#endif
    /* decrement open file count */
    gOpenFileCount--;

    return Result;
}

/*********************************************************************

    Function:       adi_fss_FileWrite

    Description:    writes given buffer to current file position

*********************************************************************/
__ADI_FSS_SECTION_CODE
u32
adi_fss_FileWrite(      /* Write to file stream */
    ADI_FSS_FILE_HANDLE FileHandle,     /* Handle identifying file stream                   */
    u8 *buf,                            /* Start address of buffer to write                 */
    u32 size,                           /* Number of bytes to write                         */
    u32 *BytesWritten                   /* Location to store actual size written            */
)
{
    u32 Result;
    u32 newpos, savedSize;

    ADI_FSS_FILE_DESCRIPTOR *pFileDesc;

    pFileDesc = (ADI_FSS_FILE_DESCRIPTOR *)FileHandle;

#ifdef ADI_SSL_DEBUG
    if (pFileDesc)  {
        Result = ADI_FSS_RESULT_SUCCESS;
    } else {
        Result = ADI_FSS_RESULT_BAD_FILE_HANDLE;
    }
#else
    Result=ADI_FSS_RESULT_SUCCESS;
#endif

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result=_adi_fss_validate_filehandle(FileHandle);
    }

    Result=Result==ADI_FSS_RESULT_SUCCESS&&!buf?ADI_FSS_RESULT_FAILED:Result;

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* save current file size in case we need to back out on error */
        savedSize = pFileDesc->fsize;

        u32 szWritten=0;

        /* if opened for append mode, seek to end of file. */
        if (pFileDesc->mode & ADI_FSS_MODE_APPEND)
        {
            adi_fss_FileSeek( (ADI_FSS_FILE_HANDLE)(pFileDesc), 0, 2, &pFileDesc->curpos);
        }

        /* anticipate new position in file */
        newpos = pFileDesc->curpos + size;

        /* Instruct FSD to Write the data  */
        if ( NULL != (pFileDesc->Cache_data_handle) )
        {
            Result = adi_fss_CacheWrite (pFileDesc, buf, size, &szWritten);
        }
        else
        {
            /* If file opened as a text stream then replace LF with CRLF */
            if ( 0 /* pFileDesc->mode&ADI_FSS_MODE_TEXT_STREAM */)
            {
                szWritten = _adi_fss_WriteTextStream(
                    pFileDesc->FSD_device_handle,
                    buf,
                    size,
                    (void*)pFileDesc,
                    pFileDesc->pCriticalRegionData);
            }
            /* Otherwise output buffer unmodified */
            else
            {
                ADI_FSS_SUPER_BUFFER FSSBuffer;

                FSSBuffer.Buffer.Data               = buf;
                FSSBuffer.Buffer.ElementWidth       = sizeof(u8);
                FSSBuffer.Buffer.ElementCount       = size/FSSBuffer.Buffer.ElementWidth;
                FSSBuffer.Buffer.CallbackParameter  = &FSSBuffer;
                FSSBuffer.Buffer.pAdditionalInfo    = NULL;
                FSSBuffer.Buffer.ProcessedFlag      = FALSE;
                FSSBuffer.Buffer.pNext              = NULL;

                FSSBuffer.pFileDesc                 = pFileDesc;

                /* the call to the FSD will block until data is ready */
                adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
                Result = adi_dev_Write( pFileDesc->FSD_device_handle, ADI_DEV_1D, (ADI_DEV_BUFFER*)&FSSBuffer );
                adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );

                szWritten = FSSBuffer.Buffer.ProcessedElementCount;

            }
        }
        if (Result == ADI_FSS_RESULT_SUCCESS)
        {
            /* increment the file size if necessary */
            if ( newpos > pFileDesc->fsize ) {
                pFileDesc->fsize = newpos;
            }

            if (Result==ADI_FSS_RESULT_SUCCESS) {
                /* Assign the number of bytes written */
                *BytesWritten = szWritten;

                /* update the current position */
                pFileDesc->curpos += szWritten;
            }
            else {
                *BytesWritten = 0;
                pFileDesc->fsize = savedSize;
            }
        }
        else {
                *BytesWritten = 0;
        }
    }

    return Result;
}


/*********************************************************************

    Function:       adi_fss_FileRead

    Description:    reads into given buffer from current file position

*********************************************************************/
__ADI_FSS_SECTION_CODE
u32
adi_fss_FileRead (      /* Read from file stream */
    ADI_FSS_FILE_HANDLE FileHandle,     /* Handle identifying file stream                   */
    u8 *buf,                            /* Start address of buffer to fill                  */
    u32 size,                           /* Number of bytes to read                          */
    u32 *BytesRead                      /* Location to store actual size read               */
)
{
    u32 Result;
    ADI_FSS_FILE_DESCRIPTOR *pFileDesc;

    pFileDesc = (ADI_FSS_FILE_DESCRIPTOR *)FileHandle;

#ifdef ADI_SSL_DEBUG
    Result=pFileDesc?ADI_FSS_RESULT_SUCCESS:ADI_FSS_RESULT_BAD_FILE_HANDLE;
#else
    Result=ADI_FSS_RESULT_SUCCESS;
#endif

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result=_adi_fss_validate_filehandle(FileHandle);
    }

    Result=Result==ADI_FSS_RESULT_SUCCESS&&!buf?ADI_FSS_RESULT_FAILED:Result;

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* if at end of file return EOF */
        if (pFileDesc->curpos>=pFileDesc->fsize)
        {
            *BytesRead = 0;
            Result=ADI_FSS_RESULT_EOF;
        }
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        u32 NewPos;

        /* adjust request buffer size if request passed end of file */
        if ( (pFileDesc->curpos+size) > pFileDesc->fsize)
        {
            size = pFileDesc->fsize - pFileDesc->curpos;
        }
        NewPos=pFileDesc->curpos+size;

        /* Instruct FSD to read the data  */
        if ( NULL != (pFileDesc->Cache_data_handle) )
        {
            Result = adi_fss_CacheRead (pFileDesc, buf, size, &size);
        }
        else
        {
            ADI_FSS_SUPER_BUFFER FSSBuffer;

            FSSBuffer.Buffer.Data               = buf;
            FSSBuffer.Buffer.ElementWidth       = sizeof(u8);
            FSSBuffer.Buffer.ElementCount       = size/FSSBuffer.Buffer.ElementWidth;
            FSSBuffer.Buffer.CallbackParameter  = &FSSBuffer;
            FSSBuffer.Buffer.pAdditionalInfo    = NULL;
            FSSBuffer.Buffer.ProcessedFlag      = FALSE;
            FSSBuffer.Buffer.pNext              = NULL;

            FSSBuffer.pFileDesc                 = pFileDesc;

            /* the call to the FSD will block until data is ready */
            adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
            Result = adi_dev_Read( pFileDesc->FSD_device_handle, ADI_DEV_1D, (ADI_DEV_BUFFER*)&FSSBuffer );
            adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );

            size = FSSBuffer.Buffer.ProcessedElementCount;
        }

        pFileDesc->curpos = NewPos;

        /* If Text Stream we need to replace CRLF with LF */
        if ( 0 /* (pFileDesc->mode & ADI_FSS_MODE_TEXT_STREAM)==ADI_FSS_MODE_TEXT_STREAM */ )
        {
            size = _adi_fss_ProcessTextStream(buf, size);
        }

        /* Assign the number of bytes read */
        *BytesRead = size;
    }

    return Result;
}

/*********************************************************************

    Function:       adi_fss_FileSeek

    Description:    seeks to new position according to arguments

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_FileSeek (      /* Seek to location in file stream */
    ADI_FSS_FILE_HANDLE FileHandle,     /* Handle identifying file stream                   */
    s32 offset,                         /* Offset from 'whence' location                    */
    u32 whence,                         /* Location to begin seek from                      */
    u32 *tellpos                        /* Location to store current position after seek    */
)
{
    u32 Result;
    ADI_FSS_FILE_DESCRIPTOR *pFileDesc;

    pFileDesc = (ADI_FSS_FILE_DESCRIPTOR *)FileHandle;

#ifdef ADI_SSL_DEBUG
    Result=pFileDesc?ADI_FSS_RESULT_SUCCESS:ADI_FSS_RESULT_BAD_FILE_HANDLE;
#else
    Result=ADI_FSS_RESULT_SUCCESS;
#endif

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result=_adi_fss_validate_filehandle(FileHandle);
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        u32 SavedPos;

        SavedPos=pFileDesc->curpos;

        /* We will calculate the new position. Old position is saved incase
         * of an error. If no error then the saved position will be dropped.
         */
        switch (whence)
        {
        case 0: /* absolute position within the file */
            pFileDesc->curpos = (u32)offset;
            break;

        case 1: /* relative to current location within the file */
            if (offset < 0) {
                pFileDesc->curpos -= -offset;
            } else {
                pFileDesc->curpos += offset;
            }
            //pFileDesc->curpos += (offset>0?1:-1)*(u32)offset;
            break;

        case 2: /* relative to end of file */
            if ( offset < 0 && (u32)(-offset) > pFileDesc->fsize )
            {
                Result = ADI_FSS_RESULT_FAILED;
            } else {
                pFileDesc->curpos = pFileDesc->fsize + offset;
            }
            break;
        }
    if ( NULL != (pFileDesc->Cache_data_handle) )
    {
        Result = adi_fss_CacheSeek(pFileDesc, pFileDesc->curpos, FALSE);
    }
    else
    {
        if (Result==ADI_FSS_RESULT_SUCCESS)
        {
            ADI_FSS_SEEK_REQUEST Seek;

            /* Assign Seek Request */
            Seek.pFileDesc = pFileDesc;
            Seek.whence = whence;
            Seek.offset = offset;

            /* Instruct the FSD to seek to the NEW requested position */
            adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
            Result = adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSD_CMD_SEEK_FILE, (void*)&Seek );
            adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
        }
    }


        /* if error restore the old position and report failed */
        if (Result!=ADI_FSS_RESULT_SUCCESS)
        {
            pFileDesc->curpos = SavedPos;
            Result=ADI_FSS_RESULT_FAILED;
        }

        /* return the adjusted current position */
        *tellpos = pFileDesc->curpos;
    }

    return Result;
}

/*********************************************************************

    Function:       adi_fss_FileTell

    Description:    Reports the current read/write pointer position in
                    the file associated with the given file descriptor.

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_FileTell (      /* Report current location in file stream */
    ADI_FSS_FILE_HANDLE FileHandle,     /* Handle identifying file stream                   */
    u32 *tellpos                        /* Location to store current position in stream     */
)
{
    u32 Result;
    ADI_FSS_FILE_DESCRIPTOR *pFileDesc;

    pFileDesc = (ADI_FSS_FILE_DESCRIPTOR *)FileHandle;

#ifdef ADI_SSL_DEBUG
    Result=pFileDesc?ADI_FSS_RESULT_SUCCESS:ADI_FSS_RESULT_BAD_FILE_HANDLE;
#else
    Result=ADI_FSS_RESULT_SUCCESS;
#endif

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result=_adi_fss_validate_filehandle(FileHandle);
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* return the current position */
        *tellpos = pFileDesc->curpos;
    }

    return Result;
}

/*********************************************************************

    Function:       adi_fss_FileRemove

    Description:    Removes a file

*********************************************************************/
__ADI_FSS_SECTION_CODE
u32 adi_fss_FileRemove(
    ADI_FSS_WCHAR *name,               /* Unicode UTF-8 array identifying file to open     */
    u32 namelen                        /* Length of name array                              */
)
{
    u32 Result;
    ADI_FSS_FULL_FNAME *pFile;
    ADI_FSS_MOUNT_DEF *pVolume;

    pVolume = pActiveVolume;

    /* Determine volume from name. If not specified assume first in volume table.
       Load up directory name linked list
     */

    Result = SplitPath(name, namelen, &pFile, &pVolume);

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Assign FSD Callback function to invoke on data transfer completion */
        adi_dev_Control( pVolume->FSD_DeviceHandle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
        Result = adi_dev_Control( pVolume->FSD_DeviceHandle, ADI_FSD_CMD_REMOVE, (void*)pFile );
        adi_dev_Control( pVolume->FSD_DeviceHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
    }

    /* Free allocated resources */
    FreePathList(pFile);

    /* return result */
    return Result;
}

/*********************************************************************

    Function:       adi_fss_FileRename

    Description:    Rename a file

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32 adi_fss_FileRename(
    ADI_FSS_WCHAR *old_path,           /* Unicode UTF-8 array identifying file to rename  */
    u32 old_pathlen,                        /* Length of name array                            */
    ADI_FSS_WCHAR *new_path,           /* Unicode UTF-8 array identifying new file name   */
    u32 new_pathlen                         /* Length of new name array                        */
)
{
    u32 Result;
    /* Form Linked list of names */
    ADI_FSS_RENAME_DEF rename_def;
    ADI_FSS_MOUNT_DEF *pOldVolume = pActiveVolume;
    ADI_FSS_MOUNT_DEF *pNewVolume = pActiveVolume;

    /* Both old_path ans new_path must reside on same file system */
    char *pOldFileSystemType=0;
    char *pNewFileSystemType=0;


    /* Determine volume from name. If not specified assume first in volume table.
     * Load up directory name linked list
     */
    pOldVolume = pNewVolume = pActiveVolume;

    Result = SplitPath(old_path, old_pathlen, &rename_def.pSource, &pOldVolume);

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result = SplitPath(new_path, new_pathlen, &rename_def.pTarget, &pNewVolume);
    }

    if ( pOldVolume == pNewVolume )
    {
        /* If both files are in the same volume, send a rename request to the FSD */
        Result = adi_dev_Control( pOldVolume->FSD_DeviceHandle, ADI_FSD_CMD_RENAME, (void*)&rename_def );
    }
    else
    {
        /* Otherwise, copy from source file to destination file and then remove source file */

        adi_dev_Control( pOldVolume->FSD_DeviceHandle, ADI_FSD_CMD_GET_TYPE_STRING, (void*)&pOldFileSystemType );
        adi_dev_Control( pNewVolume->FSD_DeviceHandle, ADI_FSD_CMD_GET_TYPE_STRING, (void*)&pNewFileSystemType );

        if (!strncmp(pOldFileSystemType,pNewFileSystemType,8) ) {
            Result = ADI_FSS_RESULT_FAILED;
        }

        if(Result==ADI_FSS_RESULT_SUCCESS)
        {
            u8 *pTransferData;
            ADI_FSS_FILE_HANDLE OldFileHandle;
            ADI_FSS_FILE_HANDLE NewFileHandle;

            /* Otherwise, a copy operation is performed followed by a remove */
            pTransferData = (u8*)_adi_fss_malloc(-1,1024);
            if (!pTransferData) {
                Result = ADI_FSS_RESULT_NO_MEMORY;
            }

            if (Result==ADI_FSS_RESULT_SUCCESS)
            {
                /* Open old file as read-only binary */
                Result = adi_fss_FileOpen(
                                            old_path,
                                            old_pathlen,
                                            ADI_FSS_MODE_READ|ADI_FSS_MODE_BINARY_STREAM,
                                            &OldFileHandle
                                          );

                if (Result!=ADI_FSS_RESULT_SUCCESS)
                {
                    _adi_fss_free(-1,pTransferData);
                    Result=ADI_FSS_RESULT_FAILED;
                }

                /* Open new file as write-only binary, create/truncate */
                Result = adi_fss_FileOpen(
                                            new_path,
                                            new_pathlen,
                                            ADI_FSS_MODE_WRITE|ADI_FSS_MODE_CREATE|ADI_FSS_MODE_TRUNCATE|ADI_FSS_MODE_BINARY_STREAM,
                                            &NewFileHandle
                                          );

                if (Result!=ADI_FSS_RESULT_SUCCESS)
                {
                    _adi_fss_free(-1,pTransferData);
                    adi_fss_FileClose(OldFileHandle);
                }
            }

            if (Result==ADI_FSS_RESULT_SUCCESS)
            {
                /* Copy file contents */
                u32 bytes_read, bytes_written;

                do
                {
                    Result = adi_fss_FileRead( (ADI_FSS_FILE_HANDLE)OldFileHandle, pTransferData, 1024, &bytes_read );
                    if ( Result!=ADI_FSS_RESULT_EOF )
                    {
                        Result = adi_fss_FileWrite( (ADI_FSS_FILE_HANDLE)NewFileHandle, pTransferData, bytes_read, &bytes_written );
                    }
                }
                while (Result==ADI_FSS_RESULT_SUCCESS && bytes_read);

                /* Free resources */
                _adi_fss_free(-1,pTransferData);
                adi_fss_FileClose(OldFileHandle);
                adi_fss_FileClose(NewFileHandle);

                /* successful copy - remove old file */
                if (Result==ADI_FSS_RESULT_EOF)
                {
                    Result = adi_fss_FileRemove(old_path, old_pathlen);
                }
            }
        }
    }

    /* Free allocated resources */
    FreePathList(rename_def.pSource);
    FreePathList(rename_def.pTarget);

    /* return result */
    return Result;
}

/*********************************************************************

    Function:       adi_fss_FileSetAttr

    Description:    sets the attributes for a file (FSD specific)

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_FileSetAttr(
    ADI_FSS_WCHAR *name,               /* Unicode UTF-8 array identifying file to open     */
    u32 namelen,                       /* Length of name array                              */
    u32 Attributes,                    /* Attributes to change */
    u32 ActionCommand                  /* Add/remove/replace option */
)
{
    u32 Result;
    ADI_FSS_MOUNT_DEF *pVolume;
    ADI_FSS_DIR_DEF FileDirDef;

    FileDirDef.pFileDesc = CreateFileDescriptor(name, namelen);
    FileDirDef.entry.Attributes = Attributes;

    if (FileDirDef.pFileDesc)
    {
        /* Instruct FSD to change the attributes of the file */
        adi_dev_Control( FileDirDef.pFileDesc->FSD_device_handle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
        Result = adi_dev_Control( FileDirDef.pFileDesc->FSD_device_handle, ActionCommand, (void*)&FileDirDef );
        adi_dev_Control( FileDirDef.pFileDesc->FSD_device_handle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );

        DestroyFileDescriptor(FileDirDef.pFileDesc);
    }

    return Result;

}


/*********************************************************************

    Function:       adi_fss_IsEOF

    Description:    Determines whether file is positioned at end

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_IsEOF(          /* Determine if end of file is reached */
    ADI_FSS_FILE_HANDLE FileHandle     /* Handle identifying file stream                   */
)
{
    u32 Result = ADI_FSS_RESULT_SUCCESS;
    ADI_FSS_FILE_DESCRIPTOR *pFileDesc;

    pFileDesc = (ADI_FSS_FILE_DESCRIPTOR *)FileHandle;

#ifdef ADI_SSL_DEBUG
    if (!pFileDesc) {
        Result = ADI_FSS_RESULT_BAD_FILE_HANDLE;
    }
#endif

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result=_adi_fss_validate_filehandle(FileHandle);
    }

    if (Result==ADI_FSS_RESULT_SUCCESS && pFileDesc->curpos==pFileDesc->fsize)
    {
        Result = ADI_FSS_RESULT_EOF;
    }

    return Result;
}

/*********************************************************************

    Function:       adi_fss_DirOpen

    Description:    Opens a directory stream for reading

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_DirOpen  (      /* Open a directory stream */
    ADI_FSS_WCHAR *name,                /* Unicode UTF-8 array identifying directory to open */
    u32 namelen,                        /* Length of name array                              */
    ADI_FSS_DIR_HANDLE *pDirHandle      /* location to store Handle identifying dir stream   */
)
{
    return DirectoryOpenCreate(
        name,
        namelen,
        pDirHandle,
        ADI_FSS_MODE_READ);
}

/*********************************************************************

    Function:       adi_fss_DirClose

    Description:    Closes a directory stream

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_DirClose (      /* Close directory stream */
    ADI_FSS_DIR_HANDLE DirHandle       /* Handle identifying directory stream               */
)
{
    u32 Result;
    ADI_FSS_DIR_DEF *pDirDef;

    pDirDef = (ADI_FSS_DIR_DEF *)DirHandle;

#ifdef ADI_SSL_DEBUG
    Result=pDirDef?ADI_FSS_RESULT_SUCCESS:ADI_FSS_RESULT_BAD_FILE_HANDLE;
#else
    Result=ADI_FSS_RESULT_SUCCESS;
#endif

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result=_adi_fss_validate_filehandle(pDirDef->pFileDesc);
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Close the directory */
        adi_dev_Control( pDirDef->pFileDesc->FSD_device_handle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
        Result = adi_dev_Control( pDirDef->pFileDesc->FSD_device_handle, ADI_FSD_CMD_CLOSE_DIR, (void*)pDirDef );
        adi_dev_Control( pDirDef->pFileDesc->FSD_device_handle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
    }

    if (pDirDef)
    {
        if (pDirDef->pFileDesc)
        {
            FreePathList(pDirDef->pFileDesc->pFullFileName);
            _adi_fss_free(-1,pDirDef->pFileDesc);
        }
        _adi_fss_free(-1,pDirDef);
    }

    return Result;
}

/*********************************************************************

    Function:       adi_fss_DirRead

    Description:    Reads the next directory entry from the directory stream

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_DirRead (       /* Read next entry from directory stream */
    ADI_FSS_DIR_HANDLE DirHandle,      /* Handle identifying directory stream               */
    ADI_FSS_DIR_ENTRY **pDirEntry      /* Location to store pointer to dir entry structure  */
)
{
    u32 Result;
    ADI_FSS_DIR_DEF *pDirDef;

    pDirDef = (ADI_FSS_DIR_DEF *)DirHandle;

#ifdef ADI_SSL_DEBUG
    Result=pDirDef?ADI_FSS_RESULT_SUCCESS:ADI_FSS_RESULT_BAD_FILE_HANDLE;
#else
    Result=ADI_FSS_RESULT_SUCCESS;
#endif

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result=_adi_fss_validate_filehandle(pDirDef->pFileDesc);
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Read the directory entry*/
        adi_dev_Control( pDirDef->pFileDesc->FSD_device_handle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
        Result = adi_dev_Control( pDirDef->pFileDesc->FSD_device_handle, ADI_FSD_CMD_READ_DIR, (void*)pDirDef );
        adi_dev_Control( pDirDef->pFileDesc->FSD_device_handle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        *pDirEntry = &pDirDef->entry;
    }
    else {
        *pDirEntry = NULL;
    }

    return ADI_FSS_RESULT_SUCCESS;
}

/*********************************************************************

    Function:       adi_fss_DirSeek

    Description:    Seeks to the given point in the directory stream

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_DirSeek (       /* Seek to location in directory stream */
    ADI_FSS_DIR_HANDLE DirHandle,      /* Handle identifying directory stream               */
    u32 tellpos                        /* Position in dir stream to seek to                 */
)
{
    u32 Result;
    ADI_FSS_DIR_DEF *pDirDef;

    pDirDef = (ADI_FSS_DIR_DEF *)DirHandle;

#ifdef ADI_SSL_DEBUG
    Result=pDirDef?ADI_FSS_RESULT_SUCCESS:ADI_FSS_RESULT_BAD_FILE_HANDLE;
#else
    Result=ADI_FSS_RESULT_SUCCESS;
#endif

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result=_adi_fss_validate_filehandle(pDirDef->pFileDesc);
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Assign the Directory structure tellpos member to the given value */
        pDirDef->tellpos = tellpos;

        /* Instruct the FSD to seek to the required position */
        adi_dev_Control( pDirDef->pFileDesc->FSD_device_handle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
        adi_dev_Control( pDirDef->pFileDesc->FSD_device_handle, ADI_FSD_CMD_SEEK_DIR, (void*)pDirDef );
        adi_dev_Control( pDirDef->pFileDesc->FSD_device_handle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
    }

    return Result;
}


/*********************************************************************

    Function:       adi_fss_DirTell

    Description:    Reports the current location within the directory
                    stream.

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_DirTell (       /* Report current location in directory stream */
    ADI_FSS_DIR_HANDLE DirHandle,      /* Handle identifying directory stream               */
    u32 *tellpos                       /* Location to store current position                */
)
{
    u32 Result;
    ADI_FSS_DIR_DEF *pDirDef;

    pDirDef = (ADI_FSS_DIR_DEF *)DirHandle;

#ifdef ADI_SSL_DEBUG
    Result=pDirDef?ADI_FSS_RESULT_SUCCESS:ADI_FSS_RESULT_BAD_FILE_HANDLE;
#else
    Result=ADI_FSS_RESULT_SUCCESS;
#endif

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result=_adi_fss_validate_filehandle(pDirDef->pFileDesc);
    }

    *tellpos = Result==ADI_FSS_RESULT_SUCCESS?pDirDef->tellpos:0;

    return Result;
}


/*********************************************************************

    Function:       adi_fss_DirRewind

    Description:    Rewinds to start of directory stream

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_DirRewind (     /* Rewind directory stream */
    ADI_FSS_DIR_HANDLE DirHandle       /* Handle identifying directory stream               */
)
{
    u32 Result = ADI_FSS_RESULT_SUCCESS;
    ADI_FSS_DIR_DEF *pDirDef;

    pDirDef = (ADI_FSS_DIR_DEF *)DirHandle;

    if (!pDirDef) {
        Result = ADI_FSS_RESULT_BAD_FILE_HANDLE;
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result=_adi_fss_validate_filehandle(pDirDef->pFileDesc);
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        adi_dev_Control( pDirDef->pFileDesc->FSD_device_handle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
        adi_dev_Control( pDirDef->pFileDesc->FSD_device_handle, ADI_FSD_CMD_REWIND_DIR, (void*)pDirDef );
        adi_dev_Control( pDirDef->pFileDesc->FSD_device_handle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
    }

    return Result;
}


/*********************************************************************

    Function:       adi_fss_DirCreate

    Description:    Creates a directory entry. Mode is currently ignored.

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_DirCreate  (      /* Open a directory stream */
    ADI_FSS_WCHAR *name,                /* Unicode UTF-8 array identifying directory to open */
    u32 namelen,                        /* Length of name array                              */
    u32 mode                            /* mode (ignored)                                    */
)
{
    ADI_FSS_DIR_HANDLE DirHandle;       /* location to store Handle identifying dir stream   */
    u32             Result;
    /* set mode to open directory so that it creates it if it doesn't exist,
     * but also to ensure that the directory is not truncated if it exists
     * and set the write mode to append to always add data to end of
     * directory
     */
    Result = DirectoryOpenCreate(
        name,
        namelen,
        &DirHandle,
        (ADI_FSS_MODE_READ_WRITE|ADI_FSS_MODE_CREATE|ADI_FSS_MODE_APPEND));

    return Result;
}


/*********************************************************************

    Function:       adi_fss_DirRemove

    Description:    Removes a Directory. Fails if directory not empty, or
                    file system read-only

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32 adi_fss_DirRemove  (    /* Remove directory */
    ADI_FSS_WCHAR *name,                /* Unicode UTF-8 array identifying directory to open */
    u32 namelen                         /* Length of name array                              */
)
{
    /* Determine volume from name. If not specified assume first in volume table.
     * Load up directory name linked list
     */
    u32 Result;
    ADI_FSS_FULL_FNAME *pFile;
    ADI_FSS_MOUNT_DEF *pVolume;

    pVolume = pActiveVolume;

    Result = SplitPath(name, namelen, &pFile, &pVolume);

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Assign FSD Callback function to invoke on data transfer completion */
        adi_dev_Control( pVolume->FSD_DeviceHandle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
        Result = adi_dev_Control( pVolume->FSD_DeviceHandle, ADI_FSD_CMD_REMOVE_DIR, (void*)pFile);
        adi_dev_Control( pVolume->FSD_DeviceHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
    }

    /* Free allocated resources */
    FreePathList(pFile);

    /* return result */
    return Result;
}


/*********************************************************************

    Function:       adi_fss_DirChange

    Description:    Changes the location of the Current Working
                    Directory

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_DirChange(      /* Change current working directory */
    ADI_FSS_WCHAR *name,                /* Unicode UTF-8 array identifying new CWD */
    u32 namelen                         /* Length of name array                             */
)
{
    u32 Result;
    ADI_FSS_DIR_DEF DirDef = {0};
    ADI_FSS_FILE_DESCRIPTOR FileDesc = {0};

    DirDef.pFileDesc = &FileDesc;
    FileDesc.mode = ADI_FSS_MODE_READ;
    FileDesc.pCriticalRegionData = NULL;

    Result = SplitPath(name, namelen, &DirDef.pFileDesc->pFullFileName, &pActiveVolume);

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* and instruct appropriate driver to move to it */
        adi_dev_Control( pActiveVolume->FSD_DeviceHandle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
        Result = adi_dev_Control( pActiveVolume->FSD_DeviceHandle, ADI_FSD_CMD_CHANGE_DIR, (void*)&DirDef);
        adi_dev_Control( pActiveVolume->FSD_DeviceHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        MergePaths(&pActiveVolume->pCurrentWorkingDirectory, &FileDesc.pFullFileName);
    }

    /* free memory in linked list */
    FreePathList(DirDef.pFileDesc->pFullFileName);

    return Result;
}

/*********************************************************************

    Function:       adi_fss_GetCurrentDir

    Description:    Requests the location of the Current Working Directory

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_GetCurrentDir(  /* Open a directory stream */
    ADI_FSS_WCHAR *name,                /* Array to store Unicode UTF-8 name of CWD */
    u32 *namelen                        /* Length of name array                             */
)
{
    u32 Result;

    Result=ADI_FSS_RESULT_SUCCESS;

    /* assign volume identifier */
    if (pActiveVolume)
    {
        name[0] = pActiveVolume->MountPoint;
        name[1] = volume_separator;

        /* reduce available character count for remainder of path */
        *namelen -= 2;

        /* Forms a single string from the full path name linked list */
        u32 result = FormPathString(
            pActiveVolume->pCurrentWorkingDirectory,
            &name[2],
            namelen);
    }
    else
    {
        name[0]=0;
    }

    return Result;
}

/*********************************************************************

    Function:       adi_fss_Stat

    Description:    Retrieves the status information about a file/direnctory

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_Stat (  /* Get file/directory status */
    ADI_FSS_WCHAR *name,                /* Array to store Unicode UTF-8 name of file/dir    */
    u32 namelen,                        /* Length of name array                            */
    struct stat *pStat                  /* Address of structure to hold information         */
)
{
    u32 Result;
    ADI_FSS_FULL_FNAME *pFile;
    ADI_FSS_MOUNT_DEF *pVolume;
    ADI_FSS_FILE_DESCRIPTOR *pFileDesc;

    pVolume = pActiveVolume;

    /* Determine volume from name. If not specified assume first in volume table.
     * Load up directory name linked list
     */
    Result = SplitPath(name, namelen, &pFile, &pVolume);

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Create a File Descriptor */
        pFileDesc = (ADI_FSS_FILE_DESCRIPTOR*)_adi_fss_malloc(-1,sizeof(ADI_FSS_FILE_DESCRIPTOR));
        Result=pFileDesc?Result:ADI_FSS_RESULT_NO_MEMORY;
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Assign name */
        pFileDesc->pFullFileName = pFile;

        /* Assign Status to FSD data handle  */
        pFileDesc->FSD_data_handle = (void*)pStat;

        /* Assign File System Device Driver Handle */
        pFileDesc->FSD_device_handle = pVolume->FSD_DeviceHandle;

        /* Instruct FSD to locate the file and assign the status information */
        adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
        Result = adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSD_CMD_STAT, (void*)pFileDesc);
        adi_dev_Control( pFileDesc->FSD_device_handle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
    }

    /* clean up and return */
    FreePathList(pFile);
    if (pFileDesc)
    {
        _adi_fss_free(-1,pFileDesc);
    }


    return Result;
}

/*********************************************************************

    Function:       adi_fss_PollMedia

    Description:    Poll Physical Interface Devices for changes in
                    Media.

*********************************************************************/
__ADI_FSS_SECTION_CODE
u32
adi_fss_PollMedia(
    void
)
{
    u32 Result;
    ADI_FSS_DEVICE_LIST *pDevice;

    Result=ADI_FSS_RESULT_SUCCESS;
    pDevice=pDeviceList;

    while (pDevice)
    {
        if ((pDevice->Type&ADI_FSS_DEVICE_TYPE_PID)==ADI_FSS_DEVICE_TYPE_PID && pDevice->pDev->DeviceHandle)
        {
            /* Set up Callback from media/volume detection to be direct to FSS to
             * ensure it's live in all events
            */
            Result = adi_dev_Control( pDevice->pDev->DeviceHandle, ADI_PID_CMD_SET_DIRECT_CALLBACK, (void*)FSSCallback );

            /* Instruct PID to look for media */
            Result = adi_dev_Control( pDevice->pDev->DeviceHandle, ADI_PID_CMD_POLL_MEDIA_CHANGE, (void*)NULL );
        }
        pDevice=pDevice->pNext;
    }

    return Result;
}

/*********************************************************************

    Function:       adi_fss_Control

    Description:    Control function.

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
adi_fss_Control(        /* Control function */
    u32     CommandID,                  /* command ID                                       */
    void    *Value                      /* command specific value                           */
)
{
    u32 Result;
    ADI_FSS_CMD_VALUE_PAIR *pPair;      /* pointer to command pair */

    Result = ADI_FSS_RESULT_SUCCESS;

    /* CASEOF (command ID) */
    switch (CommandID)
    {
    /* CASE (default) */
    default:
        Result = ADI_FSS_RESULT_NOT_SUPPORTED;
        break;

    /* CASE (command table) */
    case ADI_FSS_CMD_TABLE:
        /* avoid casts */
        pPair = (ADI_FSS_CMD_VALUE_PAIR *)Value;

        // process each command pair
        while (Result==ADI_FSS_RESULT_SUCCESS&&pPair->CommandID!=ADI_FSS_CMD_END)
        {
            Result = adi_fss_Control(pPair->CommandID,pPair->Value);
            pPair++;
        }
        break;

    /* CASE (command table terminator) */
    case ADI_FSS_CMD_END:
        break;

    /* CASE (command pair) */
    case ADI_FSS_CMD_PAIR:
        // avoid casts
        pPair = (ADI_FSS_CMD_VALUE_PAIR *)Value;

        // process and return
        Result = adi_fss_Control(pPair->CommandID,pPair->Value);
        break;

    /* CASE (Volume separator) */
    case ADI_FSS_CMD_SET_VOLUME_SEPARATOR:
        volume_separator = (ADI_FSS_WCHAR)(u32)Value;
        break;

    /* CASE (Directory seperator) */
    case ADI_FSS_CMD_SET_DIRECTORY_SEPARATOR:
        directory_separator = (ADI_FSS_WCHAR)(u32)Value;
        break;

    /* CASE (Get number of partitions) */
    case ADI_FSS_CMD_GET_NUMBER_VOLUMES:
    {
        ADI_FSS_MOUNT_DEF *pMount;

        pMount=pMountList;
        *(u32*)Value=0;
        while (pMount)
        {
            (*(u32*)Value)++;
            pMount=pMount->pNext;
        }
        break;
    }

    /* CASE (Get Volume Info) */
    case ADI_FSS_CMD_GET_VOLUME_INFO:
        Result=GetVolumeInfo((ADI_FSS_VOLUME_INFO*)Value);
        break;

    /* CASE (Set DMA manager handle) */
    case ADI_FSS_CMD_SET_DMA_MGR_HANDLE:
        DMAManagerHandle = (ADI_DMA_MANAGER_HANDLE)(Value);
        break;

    /* CASE (Set Device manager handle) */
    case ADI_FSS_CMD_SET_DEV_MGR_HANDLE:
        DeviceManagerHandle = (ADI_DEV_MANAGER_HANDLE)(Value);
        break;

    /* CASE (Set DCB queue server handle) */
    case ADI_FSS_CMD_SET_DCB_MGR_HANDLE:
        DCBQueueHandle = (ADI_DCB_HANDLE)(Value);
        break;

    /* CASE (Assign memory allocation function) */
    case ADI_FSS_CMD_SET_MALLOC_FUNC:
        _adi_fss_malloc_func = (ADI_FSS_MALLOC_FUNC)Value;
        break;

    /* CASE (Assign memory re-allocation function) */
    case ADI_FSS_CMD_SET_REALLOC_FUNC:
        _adi_fss_realloc_func = (ADI_FSS_REALLOC_FUNC)Value;
        break;

    /* CASE (Assign memory release function) */
    case ADI_FSS_CMD_SET_FREE_FUNC:
        _adi_fss_free_func = (ADI_FSS_FREE_FUNC)Value;
        break;

    case ADI_FSS_CMD_REGISTER_DEVICE:
        adi_fss_RegisterDevice ( (ADI_FSS_DEVICE_DEF*)Value, 0 );
        break;

    case ADI_FSS_CMD_DEREGISTER_DEVICE:
        adi_fss_DeRegisterDevice ( (ADI_DEV_DEVICE_HANDLE)Value );
        break;

#if defined(ADI_USE_FAT_FORMAT)
    /* CASE (Format a volume ) */
    case ADI_FSD_CMD_FORMAT_VOLUME:
        _adi_fss_FormatVolume( (ADI_FSS_FORMAT_DEF*)Value );
        break;
#endif

    case ADI_FSS_CMD_SET_MEDIA_CHANGE_CALLBACK:
        PollMedia.Callback = (ADI_DCB_CALLBACK_FN)Value;
        break;

    case ADI_FSS_CMD_SET_MEDIA_CHANGE_HANDLE:
        PollMedia.Handle = Value;
        break;

    case ADI_FSS_CMD_SET_DATA_SEMAPHORE_TIMEOUT:
        gSemTimeOutArg = (u32)Value;
        break;

    case ADI_FSS_CMD_SET_TRANSFER_RETRY_COUNT:
        gMaxRetryCount = (u32)Value;
        break;

    /* END CASEOF(Command ID) */
    }

    return Result;
}

__ADI_FSS_SECTION_CODE
static ADI_FSS_WCHAR GetVolumeIdent(ADI_FSS_WCHAR *path, u32 pathlen)
{
    u32 i;

    /* iterate path string and return when volume separator found */
    for (i=0;i<pathlen&&path[i]!=volume_separator;i++);

    /* no volume separator found - return NULL */
    return i==pathlen?(ADI_FSS_WCHAR)0:path[0];

}


/************************************************************************************
*************************************************************************************

                                PRIVATE FUNCTIONS

*************************************************************************************
*************************************************************************************/

/*********************************************************************

    Function:       _adi_fss_PendOnFlag

    Description:    Pends on data completion. Central routine to
                    Tie in with RTOS requirements.

*********************************************************************/
__ADI_FSS_SECTION_CODE
u32 _adi_fss_PendOnFlag( void *pArg, volatile u32 *pFlag, u32 Value )
{
    u32 Result;

    if (_adi_fss_pend_func) {
        Result = (_adi_fss_pend_func) ( pArg, pFlag, Value );
    }
    else {
        while ( (*pFlag)!=Value );
        Result = ADI_FSS_RESULT_SUCCESS;
    }
    return Result;

}

/*********************************************************************

    Function:       IsAlignmentRequired

    Description:    Determines whether allocation should be aligned to
                    32 bytes.

*********************************************************************/
__ADI_FSS_SECTION_CODE
static bool IsAlignmentRequired(int id)
{
    switch (gAlignmentOption) {
        case ADI_FSS_MEM_ALIGN:
            return IsCPLBEnabled();

        case ADI_FSS_MEM_NOALIGN:
        default:
            break;
    }

    return false;
}

/*********************************************************************

    Function:       _adi_fss_malloc

    Description:    Wrapper for memory allocation of choice

*********************************************************************/
__ADI_FSS_SECTION_CODE
void *
_adi_fss_malloc( int id, size_t size )
{
#if defined(__ECC__)
    int HeapID = id==-1 ? GeneralHeapID : id ;
    return (_adi_fss_malloc_func)( HeapID, size );
#else
    return (_adi_fss_malloc_func)( size );
#endif
}

/*********************************************************************

    Function:       _adi_fss_realloc

    Description:    Wrapper for memory re-allocation function of choice

*********************************************************************/
__ADI_FSS_SECTION_CODE
void *
_adi_fss_realloc( int id, void *p, size_t size )
{
#if defined(__ECC__)
    int HeapID = id==-1 ? GeneralHeapID : id ;
    return (_adi_fss_realloc_func)( HeapID, p, size );
#else
    return (_adi_fss_realloc_func)( p, size );
#endif
}

/*********************************************************************

    Function:       _adi_fss_free

    Description:    Wrapper for memory release function of choice.

*********************************************************************/
__ADI_FSS_SECTION_CODE
void
_adi_fss_free( int id, void *p )
{
#if defined(__ECC__)
    int HeapID = id==-1 ? GeneralHeapID : id ;
    (_adi_fss_free_func)( HeapID, p );
#else
    (_adi_fss_free_func)( p );
#endif
}

/*********************************************************************

    Function:       _adi_fss_MallocAligned

    Description:    Wrapper for memory allocation of choice
                    if data cache is enabled it will align buffers to
                    32 byte boundary

*********************************************************************/
__ADI_FSS_SECTION_CODE
void *
_adi_fss_MallocAligned( int id, size_t size )
{
    size_t RequiredSize = size;
    u8 *pActual, *pReturned;
    if (IsAlignmentRequired(id))
    {
        RequiredSize += 32;
    }

#if defined(__ECC__)
    int HeapID = id==-1 ? (CacheHeapID==-1 ? GeneralHeapID : CacheHeapID ) : id ;
    pActual = (u8*)(_adi_fss_malloc_func)( HeapID, RequiredSize );
#else
    pActual = (u8*)(_adi_fss_malloc_func)( RequiredSize );
#endif

    if ( RequiredSize == size )
    {
        pReturned = pActual;
    }
    else {
        /* Align buffer to next 32 byte alignment boundary */
        pReturned = (u8*)ALIGN_MEM32(pActual);
        /* Store actual memory in 4 bytes ahead of returned pointer */
        *(u32*)(pReturned - 4) = (u32)pActual;
    }

    return (void*)pReturned;
}

/*********************************************************************

    Function:       _adi_fss_ReallocAligned

    Description:    Wrapper for memory re-allocation function of choice
                    if data cache is enabled it will align buffers to
                    32 byte boundary

*********************************************************************/
__ADI_FSS_SECTION_CODE
void *
_adi_fss_ReallocAligned( int id, void *p, size_t size )
{
    size_t RequiredSize = size;
    u8 *pActual, *pReturned;

    if (IsAlignmentRequired(id))
    {
        RequiredSize += 32;
    }

#if defined(__ECC__)
    int HeapID = id==-1 ? (CacheHeapID==-1 ? GeneralHeapID : CacheHeapID ) : id ;
    pActual = (_adi_fss_realloc_func)( HeapID, p, size );
#else
    pActual = (_adi_fss_realloc_func)( p, size );
#endif

    if ( RequiredSize == size )
    {
        pReturned = pActual;
    }
    else {
        /* Align buffer to next 32 byte alignment boundary */
        pReturned = (u8*)ALIGN_MEM32(pActual);
        /* Store actual memory in 4 bytes ahead of returned pointer */
        *(u32*)(pReturned - 4) = (u32)pActual;
    }

    return (void*)pReturned;
}


/*********************************************************************

    Function:       _adi_fss_FreeAligned

    Description:    Wrapper for memory release function of choice
                    if data cache is enabled buffer is assuemd to
                    be aligned 32 byte boundary

*********************************************************************/
__ADI_FSS_SECTION_CODE
void
_adi_fss_FreeAligned( int id, void *p )
{
    void *pActual = p;

    if (IsAlignmentRequired(id))
    {
        /* Retrieve actual allocated memory pointer */
        pActual = (void*)(*(u32*)((u8*)p - 4));
    }

#if defined(__ECC__)
    int HeapID = id==-1 ? (CacheHeapID==-1 ? GeneralHeapID : CacheHeapID ) : id ;
    (_adi_fss_free_func)( HeapID, pActual );
#else
    (_adi_fss_free_func)( pActual );
#endif
}


/* A NULL String is used for types and labels the FSS cannot determine */
__ADI_FSS_SECTION_DATA
#if defined(_UNICODE_SUPPORT_)
static ADI_FSS_WCHAR _NO_NAME[1] = { 0x0000 };
#else
static ADI_FSS_WCHAR _NO_NAME[1] = "";
#endif

/* String 'UNKNOWN' is used for volumes the FSS does not recognize */
__ADI_FSS_SECTION_DATA
#if defined(_UNICODE_SUPPORT_)
static ADI_FSS_WCHAR _UNKNOWN[] = { 0x0055, 0x004E, 0x004B, 0x004E, 0x004F, 0x0057, 0x004E, 0x0000 };
#else
static ADI_FSS_WCHAR _UNKNOWN[] = "UNKNOWN";
#endif

/* String 'EMPTY' is used for volumes the FSS recognizes as not partitioned */
__ADI_FSS_SECTION_DATA
#if defined(_UNICODE_SUPPORT_)
static ADI_FSS_WCHAR _EMPTY[] = { 0x0045, 0x004D, 0x0050, 0x0054, 0x0059, 0x0000 };
#else
static ADI_FSS_WCHAR _EMPTY[] = "EMPTY";
#endif

/*********************************************************************

    Function:       GetVolumeInfo

    Description:    Retrieve the information for all partitions

*********************************************************************/

__ADI_FSS_SECTION_CODE
static u32
GetVolumeInfo(ADI_FSS_VOLUME_INFO *pVolume )
{
    u32 Index;
    ADI_FSS_WCHAR *pString;
    ADI_FSS_MOUNT_DEF *pMount;

    pMount=pMountList;
    Index=pVolume->Index+1;
    while (Index--&&pMount)
    {
        unsigned long long size;

        size=pMount->VolumeDef.VolumeSize;
        size*=pMount->VolumeDef.SectorSize;
        size/=1024L*1024L;

        pVolume->Ident=pMount->MountPoint;
        pVolume->size=size;
        pVolume->status=pMount->FSD_DeviceHandle?1:0;
        if (pMount->FSD_DeviceHandle)
        {
            adi_dev_Control(
                pMount->FSD_DeviceHandle,
                ADI_FSD_CMD_GET_TYPE_STRING,
                (void*)&(pVolume->type)
            );
            adi_dev_Control(
                pMount->FSD_DeviceHandle,
                ADI_FSD_CMD_GET_LABEL,
                (void*)&(pVolume->label)
            );
        }
        else
        {
            switch (pMount->VolumeDef.FileSystemType)
            {
            /* Volume is of unknown type */
            case ADI_FSS_FSD_TYPE_UNKNOWN:
                pVolume->type = &_UNKNOWN[0];
                break;

            /* Volume is unassigned space */
            case ADI_FSS_FSD_TYPE_FREE:
                pVolume->type = &_EMPTY[0];
                break;

            default:
                pVolume->type = &_NO_NAME[0];
            }

            /* For all types set the label as a NULL string */
            pVolume->label = &_NO_NAME[0];
        }
        pMount=pMount->pNext;
    }

    return Index+1?ADI_FSS_RESULT_FAILED:ADI_FSS_RESULT_SUCCESS;
}


/*********************************************************************

    Function:       SplitPath

    Description:    Splits a path string into a linked list of
                    elements. Each element is a subdirectory or
                    filename (if last).

*********************************************************************/

__ADI_FSS_SECTION_CODE
static u32
SplitPath(
    const ADI_FSS_WCHAR *path,
    u32 pathlen,
    ADI_FSS_FULL_FNAME **pFile,
    ADI_FSS_MOUNT_DEF **pVolume)
{
    u32 i=0,j=0;
    ADI_FSS_MOUNT_DEF   *pVolTemp       = NULL;
    ADI_FSS_WCHAR       ident[256]      = {0};
    u32                 NumPathElements = 0;
    ADI_FSS_WCHAR         *p                 = (ADI_FSS_WCHAR*)path;
    ADI_FSS_WCHAR         *p1             = NULL;
    ADI_FSS_WCHAR         *p2                = NULL;

    /* Start argument checking */
    if (pathlen==0 || p[0]==0)
    {
        return ADI_FSS_RESULT_BAD_NAME;
    }

    /* Scan the path name for a valid format */

    /* Check for a single base device */
    p1 = strpbrk(p,":");
    p2 = strrchr(p,':');

    if (p1 != p2 )
    {
        return ADI_FSS_RESULT_BAD_NAME;
    }

    /* Checked for a missing directory */
    p1 = strstr (p, "//");
    if (p1 != NULL )
    {
        return ADI_FSS_RESULT_BAD_NAME;
    }
#if 1
    /* Checked for improperly terminated line */
    if ((p[pathlen - 1] == (ADI_FSS_WCHAR)'/' ) &&
        ((p[pathlen - 2] != (ADI_FSS_WCHAR)':') &&
         (p[pathlen - 2] != (ADI_FSS_WCHAR)'.')))
    {
        return ADI_FSS_RESULT_BAD_NAME;
    }
#endif

    /*int isalnum( int ch ); */

    ADI_FSS_FULL_FNAME *pFileTemp = (ADI_FSS_FULL_FNAME *)_adi_fss_malloc(-1,sizeof(ADI_FSS_FULL_FNAME));
    if (!pFileTemp)
        return ADI_FSS_RESULT_NO_MEMORY;

    pFileTemp->pPrevious = NULL;
    pFileTemp->pNext = NULL;
    pFileTemp->name = NULL;
    pFileTemp->namelen = 0;
    *pFile = pFileTemp;

    /* Traverse the input string and take appropriate action for each character
     */
    for (i=0; i<=pathlen ; p++,i++)
    {
        /* IF (character is the volume separator) */
        if ( *p == volume_separator)
        {
            /* Volume identifier must be followed by directory separator */
            if (p[1] != directory_separator)
                return ADI_FSS_RESULT_BAD_NAME;

            ident[j] = 0;

            /* ident is volume identifier */
            pVolTemp = LookUpVolume((ADI_FSS_VOLUME_IDENT)ident[0]);
            if (!pVolTemp)
                return ADI_FSS_RESULT_BAD_NAME;
            else
                *pVolume = pVolTemp;

            /* set first directory entry as root */
            pFileTemp->name = NULL;
            pFileTemp->namelen = 0;

            /* reset ident index */
            j=0;

            /* move index and pointer to beyond directory specifier */
            i++; p++;

            /* Increment path element count to trigger allocation of next in line */
            NumPathElements++;
        }
        /* ELSE IF (character is the directory separator) */
        else if (*p == directory_separator || (i==pathlen) )
        {

            if (i==0)
            {
                /* If first in path, then first set first directory entry as root
                 */
                pFileTemp->name = NULL;
                pFileTemp->namelen = 0;
                /* Increment path element count to trigger allocation of next in line */
                NumPathElements++;
            }
            else if (j!=0)
            {
                /* Ignore leading . entry */
                if (j!=1 || ident[0]!='.' || pathlen==1 )
                {
                    /* If characters between separators then allocate space for
                     * new entry if required
                     */
                    if (NumPathElements)
                    {
                        pFileTemp->pNext = (ADI_FSS_FULL_FNAME *)_adi_fss_malloc(-1,sizeof(ADI_FSS_FULL_FNAME));
                        if (!pFileTemp->pNext)
                            return ADI_FSS_RESULT_NO_MEMORY;
                        pFileTemp->pNext->pPrevious = pFileTemp;
                        pFileTemp->pNext->pNext = NULL;
                        pFileTemp = pFileTemp->pNext;
                    }

                    /* increment number of path elements */
                    NumPathElements++;

                    /* Copy name from identifier string to the name field and set
                       the namelen field
                     */
                    DuplicateUnicodeString(&pFileTemp->name, ident, j);
                    pFileTemp->namelen = j;
                }

                /* reset the current identifier count */
                j = 0;
            }
        }
        /* ELSE (character is a normal character) */
        else
        {
            /* Add the character to the current identifier
             */
            ident[j++] = *p;
        }
        /* ENDIF */

    }

    return *pVolume==NULL?ADI_FSS_RESULT_BAD_NAME:ADI_FSS_RESULT_SUCCESS;
}

/*********************************************************************

    Function:       _adi_fss_CopyPath

    Description:    Duplicates a path linked list.

*********************************************************************/

__ADI_FSS_SECTION_CODE
void
_adi_fss_CopyPath(ADI_FSS_FULL_FNAME *pDest, ADI_FSS_FULL_FNAME *pSource)
{
    ADI_FSS_FULL_FNAME *pS = pSource;
    ADI_FSS_FULL_FNAME *pD = pDest;

    pD->pPrevious = NULL;
    for ( ; pS!=NULL ; pS = pS->pNext )
    {
        if (pS->name) {
            DuplicateUnicodeString(&pD->name, pS->name, pS->namelen);
            pD->namelen = pS->namelen;
        }
        else {
            pD->name = (ADI_FSS_WCHAR*)0;
            pD->namelen = 0;
        }

        if (pS->pNext) {
            pD->pNext = (ADI_FSS_FULL_FNAME *)_adi_fss_malloc(-1,sizeof(ADI_FSS_FULL_FNAME));
            pD->pNext->pPrevious = pD;
            pD = pD->pNext;
            pD->pNext = NULL;
        }
        else {
            pD->pNext = NULL;
        }
    }
}


/*********************************************************************

    Function:       FormPathString

    Description:    Forms one character string from a path linked list

*********************************************************************/

__ADI_FSS_SECTION_CODE
static u32
FormPathString(ADI_FSS_FULL_FNAME *pFile, ADI_FSS_WCHAR *path, u32 *pathlen)
{
    ADI_FSS_FULL_FNAME *pF = pFile;
    u32 i = 0, j;

    while(pF)
    {
        /* Path name overflows available buffer - abort */
        if (i>=(*pathlen))
            return ADI_FSS_RESULT_FAILED;

        if (pF->name!=NULL)
        {
            path[i++] = directory_separator;
            for (j=0;j<(pF->namelen);j++)
            {
                path[i+j] = pF->name[j];
            }
            i += pF->namelen;
        }
        path[i] = 0;
        pF = pF->pNext;
    }

    /* if root directory add directory separator */
    if (pFile->namelen==0 && pFile->pNext==NULL )
    {
        path[i++] = directory_separator;
        path[i] = 0;
    }

    /* Assign actual path length */
    *pathlen = i;

    return ADI_FSS_RESULT_SUCCESS;
}


/*********************************************************************

    Function:       MergePaths

    Description:    Merges two path linked lists

*********************************************************************/

__ADI_FSS_SECTION_CODE
static u32 MergePaths(ADI_FSS_FULL_FNAME **pPath1, ADI_FSS_FULL_FNAME **pPath2)
{
    ADI_FSS_FULL_FNAME *p2 = *pPath2;
    ADI_FSS_FULL_FNAME  *pTemp = (ADI_FSS_FULL_FNAME *)_adi_fss_malloc(-1,sizeof(ADI_FSS_FULL_FNAME));

    /* If path2 is fully qualified, simply copy it to the target */
    if (p2->name==NULL) {
        _adi_fss_CopyPath(pTemp,*pPath2);
        FreePathList(*pPath1);
        *pPath1 = pTemp;
        return ADI_FSS_RESULT_SUCCESS;
    }

    /* Copy the existing path and work on the copy */
    _adi_fss_CopyPath(pTemp,*pPath1);
    ADI_FSS_FULL_FNAME *p1 = pTemp;

    /* move to end of the existing path */
    for ( ; p1->pNext!=NULL ; p1 = p1->pNext );

    /* and add/remove elements */
    for ( ; p2!=NULL ; p2 = p2->pNext )
    {
        /* moving back a directory */
        if ((char)(p2->name[0]&0x00FF) == '.' && (char)(p2->name[1]&0x00FF) == '.') {
            /* attempting to go back from root directory */
            if (!p1->pPrevious) {
                FreePathList(pTemp);
                return ADI_FSS_RESULT_FAILED;
            }
            _adi_fss_free(-1,p1->name);
            p1 = p1->pPrevious;
            _adi_fss_free(-1,p1->pNext);
            p1->pNext = NULL;
        /* otherwise add a sub-directory */
        } else {
            p1->pNext = (ADI_FSS_FULL_FNAME *)_adi_fss_malloc(-1,sizeof(ADI_FSS_FULL_FNAME));
            p1->pNext->pPrevious = p1;
            p1 = p1->pNext;
            DuplicateUnicodeString(&p1->name, p2->name, p2->namelen);
            p1->namelen = p2->namelen;
            p1->pNext = NULL;
        }
    }

    /* Replace the existing path with the modified copy */
    FreePathList(*pPath1);
    *pPath1 = pTemp;

    return ADI_FSS_RESULT_SUCCESS;
}

/*********************************************************************

    Function:       FreePathList

    Description:    Frees a path linked list.

*********************************************************************/

__ADI_FSS_SECTION_CODE
static void
FreePathList(ADI_FSS_FULL_FNAME *pFile)
{
    while (pFile)
    {
        ADI_FSS_FULL_FNAME *pTemp;

        pTemp=pFile;
        pFile=pFile->pNext;

        if (pTemp->name) _adi_fss_free(-1,pTemp->name);
        _adi_fss_free(-1,pTemp);
    }
}

/*********************************************************************

    Function:       LookUpVolume

    Description:    Locates a volume in the Mounted Volume Table

*********************************************************************/

__ADI_FSS_SECTION_CODE
static ADI_FSS_MOUNT_DEF *
LookUpVolume(const ADI_FSS_VOLUME_IDENT MountPoint)
{
    ADI_FSS_MOUNT_DEF *pVolume;

    pVolume=pMountList;
    while (pVolume&&pVolume->MountPoint!=MountPoint)
    {
        pVolume=pVolume->pNext;
    }
    return pVolume;
}


/*********************************************************************

    Function:       dupCommandTable

    Description:    Duplicates a command table

*********************************************************************/

#define _WORKAROUND__INCORRECT_ASSIGNMENT_
#ifdef _WORKAROUND__INCORRECT_ASSIGNMENT_
__ADI_FSS_SECTION_CODE
static ADI_DEV_CMD_VALUE_PAIR *
dupCommandTable(ADI_DEV_CMD_VALUE_PAIR *tab)
{
    /* Assume ADI_DEV_CMD_END at least */
    u32 szTab = sizeof(ADI_DEV_CMD_VALUE_PAIR);

    /* Determine size to allocate */
    ADI_DEV_CMD_VALUE_PAIR *tmp = &tab[0];
    while(tmp->CommandID!= ADI_DEV_CMD_END) {
        szTab += sizeof(ADI_DEV_CMD_VALUE_PAIR);
        if (tmp->CommandID!= ADI_DEV_CMD_END)
            tmp++;
    }

    /* Allocate memory */
    ADI_DEV_CMD_VALUE_PAIR *retval = (ADI_DEV_CMD_VALUE_PAIR *)_adi_fss_malloc(-1,szTab);

    /* Copy table contents */
    ADI_DEV_CMD_VALUE_PAIR *pRetval = &retval[0];
    tmp = &tab[0];
    u8 looping=1;
    while(looping) {
        pRetval->CommandID = tmp->CommandID;
        pRetval->Value = tmp->Value;
        if (tmp->CommandID!= ADI_DEV_CMD_END) {
            tmp++;
            pRetval++;
        } else {
            looping = 0;
        }
    }

    /* return pointer to new table */
    return retval;
}
#endif

/*********************************************************************

    Function:       RemoveMountedVolume

    Description:    Removes mounted volume

*********************************************************************/
__ADI_FSS_SECTION_CODE
static void RemoveMountedVolume( ADI_FSS_MOUNT_DEF *pMount)
{
    /*
     * If the volume being removed is the active volume
     * then clear the active volume
     */
    ADI_FSS_MOUNT_DEF *pCurrMount = pMountList;

    if (pActiveVolume == pMount)
    {
        pActiveVolume = NULL;
    }

    /* if at top of list then set top to next if present, else set to NULL  */
    if ( pMount == pMountList )
    {
        /* If we are the top of the list we move everyone up one. */
        /* If we are the only one we get the NULL by default      */
        pMountList = pMount->pNext;
    }
    else
    {
        /* iterate list until we reach the one before the one to be removed */
        while ( pCurrMount->pNext && (pCurrMount->pNext != pMount) )
        {
            pCurrMount = pCurrMount->pNext;
        }

        /* Remove Mount Volume definition from Mount List */
        pCurrMount->pNext = pMount->pNext;
    }

    /* Close FSD device */
    adi_dev_Close(pMount->FSD_DeviceHandle);

    /* Free pMount and related memory*/
    FreePathList(pMount->pCurrentWorkingDirectory);
    _adi_fss_free(-1,pMount);

}

bool isaligned(u32 addr, u32 mask)
{
    if ((addr&mask)==0)
        return true;
    else
        return false;
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

    Function:       _adi_fss_CallbackFunction

    Description:    external wrapper for FSS callback

*********************************************************************/

void _adi_fss_CallbackFunction( void *pHandle, u32 Event, void *pArg )
{
    FSSCallback(pHandle, Event, pArg);
}

/*********************************************************************

    Function:       FSSCallback

    Description:    Callback function for FSD and PID events

*********************************************************************/

__ADI_FSS_SECTION_CODE
static void
FSSCallback(void *pHandle, u32 Event, void *pArg)
{
    u32 Result;
    u32 imask;
    void *pExitCriticalArg;
    ADI_FSS_FILE_DESCRIPTOR *pFileDesc=NULL;
    ADI_FSS_SUPER_BUFFER *pFSSBuffer;

    /* CASEOF ( Event flag ) */
    switch (Event)
    {
    /* CASE (Data transfer completion event) */
    case (ADI_PID_EVENT_DEVICE_INTERRUPT):
    case (ADI_DEV_EVENT_BUFFER_PROCESSED):
    {
        ADI_DEV_DEVICE_HANDLE PID_DeviceHandle = *((ADI_DEV_DEVICE_HANDLE *)pHandle);

        /* We perform flag update and PID/FSD/Cache callbacks in a critical region
         * that masks out (only) the device interrupt of the mass storage device.
         * This ensures that the flag settign and its use are atomic
        */
        adi_dev_Control( PID_DeviceHandle, ADI_PID_CMD_DISABLE_INTERRUPT, NULL);

        pFSSBuffer = (ADI_FSS_SUPER_BUFFER *)pArg;
        if (Event == ADI_PID_EVENT_DEVICE_INTERRUPT) {
            SET_DEV_INT(pFSSBuffer->CompletionBitMask);
        }
        else {
            SET_BUF_PROC(pFSSBuffer->CompletionBitMask);
        }
        /* Pass control to PID for event processing */
        if ( pFSSBuffer->PIDCallbackFunction ) {
            (pFSSBuffer->PIDCallbackFunction)( pFSSBuffer->PIDCallbackHandle, Event, pArg );
        }

        /* Pass control to FSD for event processing */
        if ( pFSSBuffer->FSDCallbackFunction ) {
            (pFSSBuffer->FSDCallbackFunction)( pFSSBuffer->FSDCallbackHandle, Event, pArg );
        }
        /* Pass control to the File Cache to process the callback */
        if (pFSSBuffer->pFileDesc) {
            adi_fss_CacheCallback( Event, (void*)pArg );
        }
        /* Exit crtical reqion */
        adi_dev_Control( PID_DeviceHandle, ADI_PID_CMD_ENABLE_INTERRUPT, NULL);

    }
    break;

    /* CASE (DMA error interrupt ) */
    case (ADI_DMA_EVENT_ERROR_INTERRUPT):
        /* TBD: call into PID to determine & resolve problem */
        break;

    /* CASE (Media detected) */
    case (ADI_FSS_EVENT_MEDIA_DETECTED):
        if (PollMedia.Callback) {
            (PollMedia.Callback)( PollMedia.Handle, ADI_FSS_EVENT_MEDIA_DETECTED, NULL);
        }
        break;

    /* CASE (Media removal event) */
    case (ADI_FSS_EVENT_MEDIA_REMOVED):
    /* CASE (Media Insertion event) */
    case (ADI_FSS_EVENT_MEDIA_INSERTED):
    {
        ADI_FSS_MOUNT_DEF *pMount,*pLast;
        u32 DeviceNumber = *(u32 *)pArg;
        ADI_DEV_DEVICE_HANDLE PID_DeviceHandle = *((ADI_DEV_DEVICE_HANDLE *)pHandle);

        /* first remove any Mounted Volumes with PID Handle = *pHandle */
        pMount = pMountList;
        pLast = NULL;

        while (pMount)
        {
            if ( pMount->PID_DeviceHandle==PID_DeviceHandle && pMount->VolumeDef.DeviceNumber==DeviceNumber )
            {
                /* Generate Un-Mount event */
                if (PollMedia.Callback)
                {
                    (PollMedia.Callback)( PollMedia.Handle, ADI_FSS_EVENT_VOLUME_UNMOUNT, (void*)pMount);
                }
                /*
                 * If the volume being removed is the active volume
                 * then clear the active volume
                 */
                if (pActiveVolume==pMount) {
                    pActiveVolume = NULL;
                }

                /* Remove Mount Volume definition from Mount List */
                if ( pLast ) {
                    pLast->pNext = pMount->pNext;
                } else {
                    pMountList = pMount->pNext;
                }

                /* Close FSD device */
                adi_dev_Close(pMount->FSD_DeviceHandle);

                /* Free pMount and related memory*/
                FreePathList(pMount->pCurrentWorkingDirectory);
                _adi_fss_free(-1,pMount);

                pMount=pLast;
            }
            pLast = pMount;
            if ( pMount ) {
                pMount = pMount->pNext;
            } else {
                pMount = pMountList;
            }
        }

        /* set result value for PID use */
        if (Event==ADI_FSS_EVENT_MEDIA_INSERTED)
        {
            /* now detect new volumes, pArg should contain device number */
            Result=(u32)adi_dev_Control( PID_DeviceHandle, ADI_PID_CMD_DETECT_VOLUMES, (void*)DeviceNumber );

            *((u32 *)pArg)=Result;
        }
        break;
    }

    /* CASE (Media removal event) */
    case (ADI_FSS_EVENT_VOLUME_DETECTED):
    {
        ADI_DEV_DEVICE_HANDLE PID_DeviceHandle = *(ADI_DEV_DEVICE_HANDLE*)pHandle;
        ADI_FSS_VOLUME_DEF *pVolumeDef = (ADI_FSS_VOLUME_DEF *)pArg;

        if ( pVolumeDef ) {
            ProcessDetectedVolume( PID_DeviceHandle, pVolumeDef );
        }
        break;
    }

    /* END CASEOF */
    }
}

/*********************************************************************

    Function:       DirectoryOpenCreate

    Description:    opens/creates a directory entry

*********************************************************************/

__ADI_FSS_SECTION_CODE
static u32
DirectoryOpenCreate  (      /* Open a directory stream */
    ADI_FSS_WCHAR *name,                /* Unicode UTF-8 array identifying directory to open */
    u32 namelen,                        /* Length of name array                              */
    ADI_FSS_DIR_HANDLE *DirHandle,      /* location to store Handle identifying dir stream   */
    u32 mode                            /* mode of file opening                              */
)
{
    u32 Result;
    ADI_FSS_DIR_DEF *pDirDef;
    ADI_FSS_MOUNT_DEF *pVolume;
    ADI_FSS_FULL_FNAME *pDir;

    pVolume = pActiveVolume;
    pDir=0;
    pDirDef=0;

    /* Assign Directory Handle */
    *DirHandle = (ADI_FSS_DIR_HANDLE)NULL;

    /* Determine volume from name. If not specified assume first in volume table.
     * Load up directory name linked list
     */
    Result = SplitPath(name, namelen, &pDir, &pVolume);

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Create Directory Descriptor */
        pDirDef = (ADI_FSS_DIR_DEF *)_adi_fss_malloc(-1,sizeof(ADI_FSS_DIR_DEF));
        Result=pDirDef?Result:ADI_FSS_RESULT_NO_MEMORY;
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Create accompanying File Descriptor */
        pDirDef->pFileDesc = (ADI_FSS_FILE_DESCRIPTOR*)_adi_fss_malloc(-1,sizeof(ADI_FSS_FILE_DESCRIPTOR));
        Result=pDirDef->pFileDesc?Result:ADI_FSS_RESULT_NO_MEMORY;
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Assign name */
        pDirDef->pFileDesc->pFullFileName = pDir;

        /* Assign mode   */
        pDirDef->pFileDesc->mode = mode;

        /* Assign File System Device Driver Handle */
        pDirDef->pFileDesc->FSD_device_handle = pVolume->FSD_DeviceHandle;

        /* Open the directory */
        if ((mode&ADI_FSS_MODE_CREATE)==ADI_FSS_MODE_CREATE)
        {
            Result = adi_dev_Control(
                pDirDef->pFileDesc->FSD_device_handle,
                ADI_FSD_CMD_MAKE_DIR,
                (void*)pDirDef);

            /* Directory Creation does not require memory retention */
            FreePathList(pDir);
            if (pDirDef)
            {
                if (pDirDef->pFileDesc)
                {
                    _adi_fss_free(-1,pDirDef->pFileDesc);
                }
                _adi_fss_free(-1,pDirDef);
            }

        }
        else
        {
            Result = adi_dev_Control(
                pDirDef->pFileDesc->FSD_device_handle,
                ADI_FSD_CMD_OPEN_DIR,
                (void*)pDirDef);
            if (Result==ADI_FSS_RESULT_SUCCESS)
            {
                *DirHandle = (ADI_FSS_DIR_HANDLE)pDirDef;
            }
            if (Result!=ADI_FSS_RESULT_SUCCESS)
            {
                /* If not found then clean up and report error */
                FreePathList(pDir);
                if (pDirDef)
                {
                    if (pDirDef->pFileDesc)
                    {
                        _adi_fss_free(-1,pDirDef->pFileDesc);
                    }
                    _adi_fss_free(-1,pDirDef);
                }
            }
        }
    }


    return Result;
}

extern void WaitMilliSec(unsigned int msec);
extern void WaitMicroSec(unsigned int msec);

/*********************************************************************

    Function:       _adi_fss_PIDTransfer

    Description:    Submits an FSS Super Buffer to the PID

*********************************************************************/
__ADI_FSS_SECTION_CODE
u32 _adi_fss_PIDTransfer( ADI_DEV_DEVICE_HANDLE PIDHandle, ADI_FSS_SUPER_BUFFER *pBuffer, u32 BlockFlag )
{
    u32 Result;
    /* gMaxRetryCount is zero if only only one attempt is allowed */
    u32 RetryCount = gMaxRetryCount+1;

    /* reset completion mask */
    pBuffer->CompletionBitMask = __ADI_FSS_MASK_;
    /* Acquire PID Lock Semaphore
    */
    Result = adi_dev_Control (PIDHandle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
    if (Result==ADI_FSS_RESULT_SUCCESS)
    {

        /* Send LBA request, if required
        */
        if ( pBuffer->LBARequest.SectorCount ) {
            Result = adi_dev_Control (PIDHandle, ADI_PID_CMD_SEND_LBA_REQUEST, (void*)&pBuffer->LBARequest );
        }

        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            /* Queue Buffer
            */
            do {

                if ( pBuffer->LBARequest.ReadFlag ) {
                    _adi_fss_FlushMemory( pBuffer->Buffer.Data, pBuffer->Buffer.ElementCount * pBuffer->Buffer.ElementWidth, ADI_DMA_WNR_WRITE);
                    Result = adi_dev_Read( PIDHandle, ADI_DEV_1D, (ADI_DEV_BUFFER*)pBuffer );
                } else {
                    _adi_fss_FlushMemory( pBuffer->Buffer.Data, pBuffer->Buffer.ElementCount * pBuffer->Buffer.ElementWidth, ADI_DMA_WNR_READ);
                    Result = adi_dev_Write( PIDHandle, ADI_DEV_1D, (ADI_DEV_BUFFER*)pBuffer );
                }

                /* Enable PID Data Flow
                */
                if (Result == ADI_DEV_RESULT_SUCCESS)
                {
                    Result = adi_dev_Control( PIDHandle, ADI_PID_CMD_ENABLE_DATAFLOW, (void*)TRUE );
                }

                /* Pend on data completion - depends on BlockFlag
                */
                if ( Result==ADI_DEV_RESULT_SUCCESS)
                {
                    if (BlockFlag ) {
                        Result = adi_sem_Pend ( pBuffer->SemaphoreHandle, gSemTimeOutArg );
                    }
                    if (Result) {
                        adi_dev_Control (PIDHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
                    }

                }
                else {
                    adi_dev_Control (PIDHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
                }

                RetryCount--;

            /* while we have an error code, but as long as it's not fatal and while the retry count is
             * non zero, try again
            */
            } while(Result && RetryCount);
        }
        else {
            adi_dev_Control (PIDHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
        }
    }
    //_adi_fss_WaitMilliSec(1);
    return Result;
}




/*********************************************************************

    Function:       _adi_fss_WriteBuffer

    Description:    Writes a buffer of data to the specified Device
                    driver, using a single ADI_DEV_1D_BUFFER structure,
                    and pends on the callback function setting the
                    ProcessedFlag.

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
_adi_fss_WriteBuffer(ADI_DEV_DEVICE_HANDLE DeviceHandle, void *pData, s32 size, void* pAddInfo, void *pCriticalRegionData)
{
    u32 Result,Loop;
    ADI_DEV_1D_BUFFER Buffer;

    Buffer.Data = pData;
    Buffer.ElementWidth = sizeof(u8);
    Buffer.ElementCount = size/Buffer.ElementWidth;
    Buffer.CallbackParameter = &Buffer;
    Buffer.pAdditionalInfo = pAddInfo;
    Buffer.ProcessedFlag = FALSE;
    Buffer.pNext = NULL;

    Result = adi_dev_Write(
        DeviceHandle,
        ADI_DEV_1D,
        (ADI_DEV_BUFFER*)&Buffer);

    // Pend on callback
    Result = _adi_fss_PendOnFlag (_adi_fss_PendArgument, &Buffer.ProcessedFlag, TRUE);

    return Buffer.ProcessedElementCount;
}

#define LF   '\n'
#define CR   '\r'
static char crlf[] = "\r\n";

/*********************************************************************

    Function:       _adi_fss_WriteTextStream

    Description:    Processes the input stream, and writes out CRLF
                    in place of each LF.

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
_adi_fss_WriteTextStream( ADI_DEV_DEVICE_HANDLE DeviceHandle, unsigned char *buf, s32 size, void* pAddInfo, void *pCriticalRegionData)
{
    u32 szWritten=0, szWritten2=0, startpos=0, result;
    u32 i, nbytes=0;

    for (i=0;i<size;i++)
    {
        // increment byte counter for the current buffer section
        nbytes++;
        if ( buf[i]==LF )
        {
            // replace LF with CR
            buf[i] = CR;
            // and write accumulated buffer to file
            szWritten = _adi_fss_WriteBuffer(
                                DeviceHandle,
                                (void*)&buf[startpos],
                                nbytes,
                                pAddInfo,
                                pCriticalRegionData
                            );

            // reinstate LF
            buf[i] = LF;
            // and set startpos to the position of LF to make it the first thing out
            startpos = i;
            szWritten2 += ( szWritten<nbytes ? szWritten : nbytes-1);
            // and reset byte counter to 1 (current LF)
            nbytes = 1;

        }
    }
    u8 LF_at_end = startpos && startpos==(size-1) && i==size;
    u8 Single_LF = szWritten2==0 && size==1 && (*(char*)buf)==LF;
    u8 Trailing_chars = i==size && szWritten2 && startpos<(size-1);

    if ( szWritten2==0 /* None written */ || Trailing_chars || LF_at_end )
    {
        void *pData;
        u32 nbytes;
        if (  Single_LF  )
        {
            pData = (void*)crlf;
            nbytes = 2;
        }
        else if( LF_at_end )
        {
            pData = (void*)&buf[startpos];
            nbytes = 1;
        }
        else if( Trailing_chars )
        {
            pData = (void*)&buf[startpos];
            nbytes = size - startpos;
        }
        else /* no LF in stream */
        {
            pData = (void*)buf;
            nbytes = size;
        }
        szWritten = _adi_fss_WriteBuffer(
                            DeviceHandle,
                            pData,
                            nbytes,
                            pAddInfo,
                            pCriticalRegionData
                        );
        if( !startpos || startpos!=(size-1) )
            szWritten2 = szWritten;
    }

    return size;
}

/*********************************************************************

    Function:       _adi_fss_ProcessTextStream

    Description:    Parses input buffer and replaces CRLF with LF.
                    The buffer is compacted.

*********************************************************************/

__ADI_FSS_SECTION_CODE
u32
_adi_fss_ProcessTextStream(u8 *buf, u32 size)
{
    int i,j, size2=size;
    for (i=0,j=0;i<size;i++,j++)
    {
        buf[j] = buf[i];
        if (i<(size-1) && buf[i]==CR && buf[i+1]==LF)
        {
            // found CRLF sequence
            // replace \r with \n
            buf[j] = LF;
            // shrink buffer
            i++;
            size2--;
        }
    }
    return size2;
}

#if defined(_UNICODE_SUPPORT_)

/*********************************************************************

    Function:       AsciiToUnicode

    Description:    Converts an ascii string to a Unicode UTF-8 string.

*********************************************************************/

__ADI_FSS_SECTION_CODE
static ADI_FSS_WCHAR *
AsciiToUnicode(const char *name, u32 *namelen)
{
    u32 i;
    u32 namelen = strlen(name);
    // generaate Unicode name
    ADI_FSS_WCHAR *wname = (ADI_FSS_WCHAR*)_adi_fss_malloc(-1, sizeof(ADI_FSS_WCHAR)*namelen );

    for (i=0;i<namelen;i++)
        wname[i] = (u16)name[i];

    return wname;

}


/*********************************************************************

    Function:       UnicodeToAscii

    Description:    Converts a Unicode UTF-8 string to ascii.

*********************************************************************/

__ADI_FSS_SECTION_CODE
static char *
UnicodeToAscii(char *name, ADI_FSS_WCHAR *wname, u32 namelen)
{
    u32 i;
    for (i=0;i<namelen;i++)
        name[i] = (char)wname[i]&0x00FF;
    name[i] = '\0';

    return name;
}

#endif

/*********************************************************************

    Function:       DuplicateUnicodeString

    Description:    Duplicates a Unicode UTF-8 string.

*********************************************************************/

__ADI_FSS_SECTION_CODE
static u32
DuplicateUnicodeString(ADI_FSS_WCHAR **pDest, ADI_FSS_WCHAR *pSrc, u32 len)
{
    u32 i;
    *pDest = (ADI_FSS_WCHAR*)_adi_fss_malloc(-1,len*sizeof(ADI_FSS_WCHAR));
    if (!pDest)
        return ADI_FSS_RESULT_NO_MEMORY;

    for (i=0;i<len;i++)
        (*pDest)[i] = pSrc[i];

    return ADI_FSS_RESULT_SUCCESS;
}



/*********************************************************************

    Function:       ProcessDetectedVolume

    Description:    Attempts to mount a detected volume

*********************************************************************/
__ADI_FSS_SECTION_CODE
static void ProcessDetectedVolume( ADI_DEV_DEVICE_HANDLE PID_DeviceHandle, ADI_FSS_VOLUME_DEF *pVolumeDef )
{
    {
        ADI_FSS_MOUNT_DEF *pVolume;
        ADI_FSS_DEVICE_LIST *pDevice;
        u32 Result;

        /* allocate some memory for device */
        pVolume=_adi_fss_malloc(-1,sizeof(*pVolume));
        if (!pVolume)
        {
            return;
        }

        pVolume->FSD_DeviceHandle=NULL;

        /* find FSD that matches volume type */
        pDevice=pDeviceList;
        while (pDevice)
        {
            if (pDevice->Type==ADI_FSS_DEVICE_TYPE_FSD)
            {
                pDevice->pDev->DeviceHandle=NULL;

                /*
                 * Open the FSD device using the Mount Handle as the
                 * clienthandle since the FFS Callback can have access
                 * to both the FSD and PID handles.
                 */
                Result=adi_dev_Open(
                                        DeviceManagerHandle,
                                        pDevice->pDev->pEntryPoint,
                                        0,
                                        NULL,
                                        &pDevice->pDev->DeviceHandle,
                                        ADI_DEV_DIRECTION_BIDIRECTIONAL,
                                        DMAManagerHandle,
                                        DCBQueueHandle,
                                        FSSCallback
                                   );
                if (Result==ADI_FSS_RESULT_SUCCESS)
                {
                    /*
                     * Determine whether the opened FSD supports the
                     * requested file system
                     */
                    Result = adi_dev_Control(
                                                pDevice->pDev->DeviceHandle,
                                                ADI_FSD_CMD_GET_FILE_SYSTEM_SUPPORT,
                                                (void *)pVolumeDef->FileSystemType
                                            );

                    /* set up the device driver for chained dataflow */
                    if (Result==ADI_FSS_RESULT_SUCCESS)
                    {
                        Result = adi_dev_Control(
                                                    pDevice->pDev->DeviceHandle,
                                                    ADI_DEV_CMD_SET_DATAFLOW_METHOD,
                                                    (void *)ADI_DEV_MODE_CHAINED
                                                );
                    }

                    if (Result==ADI_FSS_RESULT_SUCCESS&&pDevice->pDev->pConfigTable)
                    {
                        Result = adi_dev_Control(
                                                    pDevice->pDev->DeviceHandle,
                                                    ADI_DEV_CMD_TABLE,
                                                    (void *)pDevice->pDev->pConfigTable
                                                );
                    }

                    if (Result==ADI_FSS_RESULT_SUCCESS)
                    {
                        Result = adi_dev_Control(
                                                    pDevice->pDev->DeviceHandle,
                                                    ADI_FSD_CMD_SET_PID_HANDLE,
                                                    (void *)PID_DeviceHandle
                                                );
                    }

                    if (Result==ADI_FSS_RESULT_SUCCESS)
                    {
                        Result = adi_dev_Control(
                                                    pDevice->pDev->DeviceHandle,
                                                    ADI_FSD_CMD_MOUNT_VOLUME,
                                                    (void *)pVolumeDef
                                                );
                    }

                    /*
                     * Yes! this FSD matches, so init the rest of the
                     * volume
                     */
                    if (Result==ADI_FSS_RESULT_SUCCESS)
                    {
                        ADI_FSS_VOLUME_IDENT MountPoint;
                        ADI_FSS_MOUNT_DEF *pMount;
                        ADI_FSS_DEVICE_LIST *pList;

                        pVolume->FSD_DeviceHandle=pDevice->pDev->DeviceHandle;

                        /*
                         * Get the default start ident, Priority is
                         * FSD(High) -> PID(Medium) -> Default 'c'(Low)
                         */
                        if (pDevice->pDev->DefaultMountPoint)
                        {
                            MountPoint=pDevice->pDev->DefaultMountPoint;
                        }
                        else
                        {
                            /*
                             * Find the default MountPoint for the PID
                             * this is done by matching pHandle with the
                             * DeviceHandle in the pDeviceList
                             */
                            pList=pDeviceList;
                            while (pList&&pList->pDev->DeviceHandle!=PID_DeviceHandle)
                            {
                                pList=pList->pNext;
                            }
                            MountPoint=(pList&&pList->pDev->DefaultMountPoint)?
                                pList->pDev->DefaultMountPoint:'c';
                        }

                        /* now make sure this volume ident hasnt been used */
                        pMount=pMountList;
                        while (Result==ADI_FSS_RESULT_SUCCESS&&pMount)
                        {
                            if (pMount->MountPoint==MountPoint)
                            {
                                if (MountPoint=='z')
                                {
                                    /* ran out of letters so abort mount */
                                    Result=ADI_FSS_RESULT_FAILED;
                                }
                                /* increment mount point and reset pointer */
                                MountPoint++;
                                pMount=pMountList;
                            }
                            else
                            {
                                pMount=pMount->pNext;
                            }
                        }

                        /* initialize the volume and prepare for mounting */
                        if (Result==ADI_FSS_RESULT_SUCCESS)
                        {
                            memcpy(
                                &(pVolume->VolumeDef),
                                pVolumeDef,
                                sizeof(pVolume->VolumeDef));
                            pVolume->MountPoint=MountPoint;
                            pVolume->PID_DeviceHandle=PID_DeviceHandle;
                            pVolume->pCurrentWorkingDirectory=(ADI_FSS_FULL_FNAME *)_adi_fss_malloc(-1,sizeof(ADI_FSS_FULL_FNAME));
                            if (!pVolume->pCurrentWorkingDirectory)
                            {
                                Result=ADI_FSS_RESULT_NO_MEMORY;
                            }
                        }
                        if (Result==ADI_FSS_RESULT_SUCCESS)
                        {
                            pVolume->pCurrentWorkingDirectory->name = (char*)0;
                            pVolume->pCurrentWorkingDirectory->namelen = 0;
                            pVolume->pCurrentWorkingDirectory->pNext = NULL;
                            pVolume->pCurrentWorkingDirectory->pPrevious = NULL;
                        }
                    }

                    /* there was an error initializing the FSD */
                    if (Result!=ADI_FSS_RESULT_SUCCESS)
                    {
                        adi_dev_Close(pDevice->pDev->DeviceHandle);
                        pDevice->pDev->DeviceHandle=NULL;
                    }
                    else
                    {
                        pDevice->pDev->DeviceHandle=NULL;
                        /* exit outer while loop */
                        break;
                    }
                }
            }

            /* try the next one */
            pDevice=pDevice->pNext;
        }

        if (pDevice)
        {
            ADI_FSS_MOUNT_DEF *pMount,*pLast;

            /*
             * Now add it to the mounted volume list in alphabetical order
             */

            pMount=pMountList;
            pLast=NULL;
            while (pMount&&pMount->MountPoint<pVolume->MountPoint)
            {
                pLast=pMount;
                pMount=pMount->pNext;
            }

            /* update MountList with new Mount definition */
            if (pLast)
            {
                /* insert MountDef into list */
                pVolume->pNext=pLast->pNext;
                pLast->pNext=pVolume;
            }
            else
            {
                /* add MountDef to the start of the list */
                pVolume->pNext=pMountList;
                pMountList=pVolume;
            }

            /* Generate Mount event */
            if (PollMedia.Callback)
            {
                (PollMedia.Callback)( PollMedia.Handle, ADI_FSS_EVENT_VOLUME_MOUNT, (void*)&pMountList->VolumeDef);
            }


            /* If ActiveVolume is not set then set it */
            if (pActiveVolume==NULL)
            {
                pActiveVolume=pMountList;
            }
        }
        else if (pVolume)
        {
            /* there was a problem trying to mount volume */
            _adi_fss_free(-1,pVolume);
        }
    }

}

#if defined(ADI_USE_FAT_FORMAT)

/*********************************************************************

    Function:       _adi_fss_FormatVolume

    Description:    Formats a volume according to given definition.

*********************************************************************/

__ADI_FSS_SECTION_CODE
static u32 _adi_fss_FormatVolume(               /* Format a named volume */
    ADI_FSS_FORMAT_DEF *pFormatDef                  /* Format definitition */
)
{
    u32                     Result              = ADI_FSS_RESULT_SUCCESS;
    ADI_FSS_MOUNT_DEF       *pMountedVolumeDef  = NULL;
    ADI_FSS_VOLUME_DEF      VolumeDef           = {0};
    ADI_DEV_DEVICE_HANDLE   PID_DeviceHandle    = NULL;

    /* Look up volume Ident */
    pMountedVolumeDef = LookUpVolume(pFormatDef->ident);

    if (!pMountedVolumeDef) {
        Result = ADI_FSS_RESULT_BAD_NAME;
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Assign volume information */
        u32 Size = pFormatDef->VolumeDef.VolumeSize;
        pFormatDef->VolumeDef = pMountedVolumeDef->VolumeDef;
        pFormatDef->VolumeDef.VolumeSize = Size;

        /* Grab Lock Semaphore */
        adi_dev_Control( pMountedVolumeDef->FSD_DeviceHandle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );

        /* Unmount the volume */
        Result = adi_dev_Control( pMountedVolumeDef->FSD_DeviceHandle, ADI_FSD_CMD_UNMOUNT_VOLUME, NULL );

        /* Format the volume */
        if (Result==ADI_FSS_RESULT_SUCCESS){
            Result = adi_dev_Control( pMountedVolumeDef->FSD_DeviceHandle, ADI_FSD_CMD_FORMAT_VOLUME, (void*)pFormatDef);
        }

        /* Release Lock Semaphore */
        adi_dev_Control( pMountedVolumeDef->FSD_DeviceHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );

        VolumeDef = pFormatDef->VolumeDef;
        PID_DeviceHandle = pMountedVolumeDef->PID_DeviceHandle;

        /* Close the FSD & remount */
        RemoveMountedVolume (pMountedVolumeDef);

        ProcessDetectedVolume( PID_DeviceHandle, &VolumeDef);
    }

    return Result;
}

#endif


__ADI_FSS_SECTION_CODE
u32 adi_fss_RegisterDevice( ADI_FSS_DEVICE_DEF *pDeviceDef, u32 PollForMedia )

{
    u32 Result = ADI_FSS_RESULT_SUCCESS;
    ADI_FSS_DEVICE_LIST *pDevice;
    ADI_DEV_DEVICE_HANDLE pHandle;
    ADI_DEV_DEVICE_HANDLE DeviceHandle = pDeviceDef->DeviceHandle;

    pDevice=_adi_fss_malloc(-1,sizeof(ADI_FSS_DEVICE_LIST));

    if (pDevice)
    {
        pDevice->pDev =_adi_fss_malloc(-1,sizeof(ADI_FSS_DEVICE_DEF));
        if (pDevice->pDev)
        {
            /* copy device definition */
            pDevice->Type  = ADI_FSS_DEVICE_TYPE_UNKNOWN;
            pDevice->pNext = pDeviceList;
            pDeviceList    = pDevice;
        }
        else
        {
            Result=ADI_FSS_RESULT_NO_MEMORY;
        }
    }
    else
    {
        Result=ADI_FSS_RESULT_NO_MEMORY;
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Now workout the device type */

        /*
         * workout what type of device by executing a PID command,
         * if ADI_FSS_RESULT_SUCCESS is returned then the driver
         * is a PID device, otherwise we have to assume it's an FSD
         */
        u32 IsFixed = FALSE;
        Result = adi_dev_Control(pDeviceDef->DeviceHandle, ADI_PID_CMD_GET_FIXED,(void *)&IsFixed);

        if (Result==ADI_FSS_RESULT_SUCCESS) {
            pDevice->Type = ADI_FSS_DEVICE_TYPE_REGISTERED_PID;
            pDevice->pDev->DeviceHandle = pDeviceDef->DeviceHandle;
        } else {
            pDevice->Type = ADI_FSS_DEVICE_TYPE_FSD;
            pDevice->pDev->DeviceHandle = NULL;
        }

        pDevice->pDev->DefaultMountPoint = pDeviceDef->DefaultMountPoint;

        Result=ADI_FSS_RESULT_SUCCESS;
    }

    if (Result==ADI_FSS_RESULT_SUCCESS )
    {
        /* Change the client callback function */
        Result = adi_dev_Control( pDevice->pDev->DeviceHandle, ADI_DEV_CMD_CHANGE_CLIENT_CALLBACK_FUNCTION, (void*)FSSCallback );
        /* Change the client handle */
        if ( Result==ADI_DEV_RESULT_SUCCESS ) {
            Result = adi_dev_Control( pDevice->pDev->DeviceHandle, ADI_DEV_CMD_CHANGE_CLIENT_HANDLE,(void *)&pDevice->pDev->DeviceHandle );
        }
        /* Activate the driver */
        if (Result==ADI_FSS_RESULT_SUCCESS )
        {
            Result = adi_dev_Control( pDevice->pDev->DeviceHandle, ADI_PID_CMD_MEDIA_ACTIVATE, (void*)true );
        }
        /* Poll for media if requested */
        if ( PollForMedia && (pDevice->Type&ADI_FSS_DEVICE_TYPE_PID)==ADI_FSS_DEVICE_TYPE_PID )
        {
            adi_fss_PollMediaOnDevice( pDeviceDef->DeviceHandle );
        }
    }

    return Result;
}

__ADI_FSS_SECTION_CODE
u32 adi_fss_DeRegisterDevice( ADI_DEV_DEVICE_HANDLE DeviceHandle )
{
    u32 Result = ADI_FSS_RESULT_FAILED;
    u32 DeviceFound = FALSE;
    ADI_FSS_DEVICE_LIST *pDevice, *pAhead=NULL;

    /* Search Device List for device handle */
    pDevice = pDeviceList;
    while (!DeviceFound && pDevice)
    {
        /* If Device Handle matches and it is a PID then poll for media */
        if ( pDevice->pDev->DeviceHandle==DeviceHandle )
        {
            DeviceFound = TRUE;
        }
        if (!DeviceFound) {
            pAhead = pDevice;
            pDevice = pDevice->pNext;
        }
    }

    if (DeviceFound) {
        UnMountDevice( DeviceHandle );
        adi_dev_Control( DeviceHandle, ADI_PID_CMD_MEDIA_ACTIVATE, (void*)false );

        /* Remove the item from the list and free up memory */
        if (pAhead) {
            pAhead->pNext = pDevice->pNext;
        }
        else {
            pDeviceList = pDevice->pNext;
        }
        _adi_fss_free( -1, pDevice->pDev );
        _adi_fss_free( -1, pDevice );

        Result = ADI_FSS_RESULT_SUCCESS;
    }

    return Result;
}

__ADI_FSS_SECTION_CODE
u32 adi_fss_PollMediaOnDevice( ADI_DEV_DEVICE_HANDLE DeviceHandle )
{
    u32 Result = ADI_FSS_RESULT_FAILED;
    u32 DeviceFound = FALSE;

    ADI_FSS_DEVICE_LIST *pDevice;

    /* Search Device List for device handle */
    pDevice = pDeviceList;
    while (!DeviceFound && pDevice)
    {
        /* If Device Handle matches and it is a PID then poll for media */
        if ( pDevice->pDev->DeviceHandle==DeviceHandle && (pDevice->Type&ADI_FSS_DEVICE_TYPE_PID)==ADI_FSS_DEVICE_TYPE_PID )
        {
            /* Set up Callback from media/volume detection to be direct to FSS to
             * ensure it's live in all events
            */
            Result = adi_dev_Control( DeviceHandle, ADI_PID_CMD_SET_DIRECT_CALLBACK, (void*)FSSCallback );
            if (Result==ADI_FSS_RESULT_SUCCESS) {
                /* Instruct PID to look for media */
                Result = adi_dev_Control( DeviceHandle, ADI_PID_CMD_POLL_MEDIA_CHANGE, (void*)NULL );
                DeviceFound = TRUE;
            }
        }
        if (!DeviceFound) {
            pDevice = pDevice->pNext;
        }
    }
    if (DeviceFound) {
        Result = ADI_FSS_RESULT_SUCCESS;
    }

    return Result;
}


__ADI_FSS_SECTION_CODE
static u32 UnMountDevice( ADI_DEV_DEVICE_HANDLE DeviceHandle )
{
    u32 Result = ADI_FSS_RESULT_FAILED;
    ADI_FSS_MOUNT_DEF *pMount,*pLast;

    /* first remove any Mounted Volumes with PID Handle = *pHandle */
    pMount = pMountList;
    pLast = NULL;

    while (pMount)
    {
        if ( pMount->PID_DeviceHandle==DeviceHandle )
        {
            /* Generate Un-Mount event */
            if (PollMedia.Callback)
            {
                (PollMedia.Callback)( PollMedia.Handle, ADI_FSS_EVENT_MEDIA_REMOVED, (void*)&pMount->VolumeDef);
            }
            /*
             * If the volume being removed is the active volume
             * then clear the active volume
             */
            if (pActiveVolume==pMount) {
                pActiveVolume = NULL;
            }

            /* Remove Mount Volume definition from Mount List */
            if ( pLast ) {
                pLast->pNext = pMount->pNext;
            } else {
                pMountList = pMount->pNext;
            }

            /* Close FSD device */
            adi_dev_Close(pMount->FSD_DeviceHandle);

            /* Free pMount and related memory*/
            FreePathList(pMount->pCurrentWorkingDirectory);
            _adi_fss_free(-1,pMount);

            Result = ADI_FSS_RESULT_SUCCESS;

            pMount=pLast;
        }
        pLast = pMount;
        if ( pMount ) {
            pMount = pMount->pNext;
        } else {
            pMount = pMountList;
        }
    }
    return Result;
}


u32 adi_fss_GetVolumeUsage( ADI_FSS_VOLUME_IDENT Ident, ADI_FSS_VOLUME_USAGE_DEF* poVolumeUsage )
{
    u32 Result;
    /* Find FAT driver for given ident */
    u32 Index;
    ADI_FSS_WCHAR *pString;
    ADI_FSS_MOUNT_DEF *pMount;

    pMount = pMountList;
    while (pMount)
    {
        unsigned long long size;

        if (Ident == pMount->MountPoint)
        {
            Result = adi_dev_Control( pMount->FSD_DeviceHandle, ADI_FSD_CMD_GET_VOLUME_USAGE, (void*)poVolumeUsage );
            break;
        }

        pMount=pMount->pNext;
    }

    return Result;
}

/* ******************************************************************************
 * COMMON PID FUNCTIONALITY
 * ******************************************************************************
*/

/*********************************************************************

    Function:       adi_fss_TransferSectors

    Description:    Read/Write a sector from the media

*********************************************************************/
u32 adi_fss_TransferSectors(
                            ADI_DEV_DEVICE_HANDLE   hDevice,
                            void                    *pData,
                            u32                     Drive,
                            u32                     SectorNumber,
                            u32                     SectorCount,
                            u32                     SectorSize,
                            u32                     DataElementWidth,
                            u32                     bReadFlag,
                            ADI_SEM_HANDLE          hSemaphore
)
{
    u32                  Result;
    ADI_FSS_SUPER_BUFFER FSSBuffer;

    memset(&FSSBuffer,0,sizeof(ADI_FSS_SUPER_BUFFER));

    /* set up LBA request */
    FSSBuffer.LBARequest.StartSector    = SectorNumber;
    FSSBuffer.LBARequest.SectorCount    = SectorCount;
    FSSBuffer.LBARequest.DeviceNumber   = Drive;
    FSSBuffer.LBARequest.ReadFlag       = bReadFlag;
    FSSBuffer.LBARequest.pBuffer        = &FSSBuffer;

    /* Set up One D Buffer */
    FSSBuffer.Buffer.Data               = pData;
    FSSBuffer.Buffer.ElementCount       = SectorCount*SectorSize/DataElementWidth;
    FSSBuffer.Buffer.ElementWidth       = DataElementWidth;
    FSSBuffer.Buffer.CallbackParameter  = &FSSBuffer;
    FSSBuffer.Buffer.pAdditionalInfo    = NULL;
    FSSBuffer.Buffer.ProcessedFlag      = FALSE;
    FSSBuffer.Buffer.pNext              = NULL;

    /* Assign Semaphore */
    FSSBuffer.SemaphoreHandle           = hSemaphore;

    Result = _adi_fss_PIDTransfer ( hDevice, &FSSBuffer, BLOCK );

    return Result;
}


/*********************************************************************

    Function:       _adi_fss_DetectVolumes

    Description:    Detects Valid File System Volumes

*********************************************************************/
u32 _adi_fss_DetectVolumes(
                            ADI_DEV_DEVICE_HANDLE   hDevice,
                            u32                     Drive,
                            int                     HeapID,
                            u32                     SectorSize,
                            u32                     DataElementWidth,
                            ADI_SEM_HANDLE          hSemaphore
)
{
    u32 Result = ADI_FSS_RESULT_SUCCESS;
    void *pMBR;
    u32 FileSystemType;
    ADI_ATA_PARTITION_ENTRY Primary[4];
    ADI_ATA_PARTITION_ENTRY *pEntry;
    u32 Count = 0;

    f_pidDeviceHandle           = hDevice;
    f_pidDriveNumber             = Drive;
    f_pidSectorSize        = SectorSize;
    f_pidDataElementWidth  = DataElementWidth;
    f_pidhSemaphore        = hSemaphore;
    f_pidHeapID            = HeapID;

    /* Allocate the buffer for the MBR */
    pMBR = _adi_fss_MallocAligned( f_pidHeapID, f_pidSectorSize );

    if (pMBR) {
        /* First we read the Master boot record */
        Result = adi_fss_TransferSectors (
                        f_pidDeviceHandle,
                        pMBR,
                        f_pidDriveNumber,
                        0,
                        1,
                        f_pidSectorSize,
                        f_pidDataElementWidth,
                        READ,
                        f_pidhSemaphore
                    );
        if (Result == ADI_FSS_RESULT_SUCCESS)
        {
            /* Copy the area where a partition table may exist
             * (copy is required to prevent misaligned access)
            */
            memcpy( Primary, &((u8*)pMBR)[446], 4*sizeof(ADI_ATA_PARTITION_ENTRY) );

            /* First, assume the partition table is valid, i.e. for HDD or CF or SD */
            pEntry = &Primary[0];
            while (pEntry->size!=0
                        && pEntry->type
                        && (FileSystemType=GetFileSystemType(pEntry->type))!=ADI_FSS_FSD_TYPE_UNKNOWN
                        )
            {
                if (pEntry->type==ADI_ATA_VOLUME_TYPE_PRI_EXTENDED)
                {
                    ProcessExtendedPartitions(pEntry, &Count);
                }
                else
                {
                    NewVolumeDef(FileSystemType, pEntry->lba_start, pEntry->size);
                    if (Result!=ADI_FSS_RESULT_SUCCESS) {
                        break;
                    }
                    Count++;
                }
                pEntry++;
            }

            /* If nothing found, check for compatibility with FAT Bios Parameter Block
            */
            if (Count==0 && IsValidFATPartition((u8*)pMBR))
            {
                NewVolumeDef(ADI_FSS_FSD_TYPE_FAT, 0, 0);
            }


        }

        /* free up the memory */
        _adi_fss_FreeAligned(HeapID, pMBR);
    }
    else {
        Result = ADI_FSS_RESULT_NO_MEMORY;
    }

    return Result;
}


/*********************************************************************

    Function:       _adi_fss_WaitMilliSec

    Description:    Wait for passed number of milli-seconds

*********************************************************************/
void _adi_fss_WaitMilliSec(unsigned int msec)
{
    while (msec != 0) {
           WaitOneMilliSec();
           msec--;
    }

}

/*********************************************************************

    Function:       _adi_fss_WaitMicroSec

    Description:    Wait for passed number of micro-seconds

*********************************************************************/
void _adi_fss_WaitMicroSec(unsigned int msec)
{
    while (msec != 0) {
           WaitOneMicroSec();
           msec--;
    }

}

/*********************************************************************

    Function:       IsValidFATPartition

    Description:    Check that MBR is a valid FAT partition
                    For now we use the simple criteria of the
                    First byte of the BS_jmpBoot being 0xEB or 0xE9
                    and the last two bytes being 0x55 0xAA.
                    Later we could make a call on the FAT driver to
                    see if it recognized it.

*********************************************************************/
static u32 IsValidFATPartition(u8 *pData)
{
    return    ( (pData[0] == 0xEB) || (pData[0]== 0xE9) )
           && pData[510] == 0x55 && pData[511] == 0xAA;
}

/*********************************************************************

    Function:       ProcessExtendedPartitions

    Description:    Iterate the Extended partitions

*********************************************************************/
static u32 ProcessExtendedPartitions(ADI_ATA_PARTITION_ENTRY *pPartTabEntry, u32 *Count)
{
    /* Buffer for first sector in extended partition */
    void *pSector;
    u32 PartitionOffset = pPartTabEntry->lba_start;
    u32 FileSystemType;

    /* Result code set to success for first time round while loop */
    u32 Result = ADI_FSS_RESULT_SUCCESS;

    ADI_ATA_PARTITION_ENTRY ExtPartTab[2];
    /* assign local pointer to the extended partition entry in the primary table */
    ADI_ATA_PARTITION_ENTRY *pEntry = pPartTabEntry;

        /* Allocate the buffer for the MBR */
    pSector = _adi_fss_MallocAligned( f_pidHeapID, f_pidSectorSize );

    if (!pSector) {
        return ADI_FSS_RESULT_NO_MEMORY;
    }

    /* Recurse down the extended partition linked list */
    while (    Result==ADI_FSS_RESULT_SUCCESS
            && (
                   pEntry->type==ADI_ATA_VOLUME_TYPE_EXT_FAT16
                || pEntry->type==ADI_ATA_VOLUME_TYPE_PRI_EXTENDED
                )
    )
    {
        /* Read the Sector from the media */
        Result = adi_fss_TransferSectors (
                        f_pidDeviceHandle,
                        pSector,
                        f_pidDriveNumber,
                        0,
                        1,
                        f_pidSectorSize,
                        f_pidDataElementWidth,
                        READ,
                        f_pidhSemaphore
                    );

        /* Copy the area where a partition table may exist
         * (copy is required to prevent misaligned access)
        */
        memcpy( ExtPartTab, &((u8*)pSector)[446], 2*sizeof(ADI_ATA_PARTITION_ENTRY) );

        /* First Partition entry */
        pEntry = &ExtPartTab[0];
        if (pEntry->size!=0
                    && pEntry->type
                    && (FileSystemType=GetFileSystemType(pEntry->type)!=ADI_FSS_FSD_TYPE_UNKNOWN)
        )
        {
            pEntry->lba_start += PartitionOffset;
            NewVolumeDef(FileSystemType, pEntry->lba_start, pEntry->size);
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

    /* free up the memory */
    _adi_fss_FreeAligned(f_pidHeapID, pSector);

    return Result;
}

/*********************************************************************

    Function:       NewVolumeDef

    Description:    Inform FSS of newly detected volume

*********************************************************************/
static void NewVolumeDef(
                    u32 FileSystemType,
                    u32 StartAddress,
                    u32 VolumeSize
                    )
{
    ADI_FSS_VOLUME_DEF VolumeDef;
    VolumeDef.StartAddress   = StartAddress;
    VolumeDef.SectorSize     = f_pidSectorSize;
    VolumeDef.VolumeSize     = VolumeSize;
    VolumeDef.FileSystemType = FileSystemType;
    VolumeDef.DeviceNumber   = f_pidDriveNumber;

    _adi_fss_CallbackFunction( &f_pidDeviceHandle, ADI_FSS_EVENT_VOLUME_DETECTED, (void *)&VolumeDef );
}

/*********************************************************************

    Function:       GetFileSystemType

    Description:    Checks for valid volume types

*********************************************************************/
static u32 GetFileSystemType(u32 type)
{
    switch(type) {
        case ADI_ATA_VOLUME_TYPE_PRI_FAT12:
        case ADI_ATA_VOLUME_TYPE_PRI_FAT16_A:
        case ADI_ATA_VOLUME_TYPE_PRI_FAT16_B:
        case ADI_ATA_VOLUME_TYPE_EXT_FAT16:
        case ADI_ATA_VOLUME_TYPE_PRI_FAT16_LBA:
        case ADI_ATA_VOLUME_TYPE_PRI_FAT32:
        case ADI_ATA_VOLUME_TYPE_PRI_FAT32_LBA:
            return ADI_FSS_FSD_TYPE_FAT;

        case ADI_ATA_VOLUME_TYPE_PRI_UNASSIGNED:
        case ADI_ATA_VOLUME_TYPE_PRI_EXTENDED:
            return ADI_FSS_FSD_TYPE_UNASSIGNED;

        default:
            return ADI_FSS_FSD_TYPE_UNKNOWN;
    }
}

/*********************************************************************

    Function:       CreateFileDescriptor

    Description:    Creates a file descriptor object from the name

*********************************************************************/
static ADI_FSS_FILE_DESCRIPTOR *CreateFileDescriptor(
    ADI_FSS_WCHAR *name,               /* Unicode UTF-8 array identifying file to open     */
    u32 namelen                        /* Length of name array                              */
)
{
    u32                     Result;
    ADI_FSS_MOUNT_DEF       *pVolume   = pActiveVolume;
    ADI_FSS_FULL_FNAME      *pFile;
    ADI_FSS_FILE_DESCRIPTOR *pFileDesc = NULL;

    Result=ADI_FSS_RESULT_SUCCESS;

    /* Determine volume from name. If not specified assume first in volume table.
       Load up directory name linked list
     */
    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result = SplitPath(name, namelen, &pFile, &pVolume);
    }

    /* Create a File Descriptor */
    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        pFileDesc = (ADI_FSS_FILE_DESCRIPTOR*)_adi_fss_malloc(-1,sizeof(ADI_FSS_FILE_DESCRIPTOR));
        if (!pFileDesc) {
            Result = ADI_FSS_RESULT_NO_MEMORY;
        }
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Assign name */
        pFileDesc->pFullFileName = pFile;

        /* Assign File System Device Driver Handle */
        pFileDesc->FSD_device_handle = pVolume->FSD_DeviceHandle;

    }

    return pFileDesc;
}

/*********************************************************************

    Function:       DestroyFileDescriptor

    Description:    Frees memory from teh file descriptor

*********************************************************************/
static void DestroyFileDescriptor(
    ADI_FSS_FILE_DESCRIPTOR *pFileDesc
)
{
    /* free memory in linked list */
    if (pFileDesc)
    {
        if (pFileDesc->pFullFileName)
        {
            FreePathList(pFileDesc->pFullFileName);
        }
        /* Free memory for File Descriptor */
        _adi_fss_free(-1,pFileDesc);
    }
}


/*********************************************************************

    Function:       _adi_fss_WaitMilliSec

    Description:    Wait for one milli-second

*********************************************************************/
static void WaitOneMilliSec(void)
{
    volatile unsigned long long int cur,nd;

    _GET_CYCLE_COUNT(cur);

    nd = cur + (__PROCESSOR_SPEED__/1000);
    while (cur < nd) {
    _GET_CYCLE_COUNT(cur);
    }
}

/*********************************************************************

    Function:       _adi_fss_WaitMicroSec

    Description:    Wait for one micro-second

*********************************************************************/
static void WaitOneMicroSec(void)
{
    volatile unsigned long long int cur,nd;

    _GET_CYCLE_COUNT(cur);

    nd = cur + (__PROCESSOR_SPEED__/1000000);
    while (cur < nd) {
    _GET_CYCLE_COUNT(cur);
    }
}

/*
**  Public Function Definition section
*/

/*****************************************************************************
Function: IsCPLBEnabled

  Queries CPLB manager control variable to ascertain whether data cache is
  enabled. Stores result in global, f_bCPLBEnabled, on first time. Subsequent
  queries simple access this global value.

Parameters:
  none

Returns:
  true(1) or false(0) to indicate whether cache is enabled.

Notes:
  The global, f_bCPLBEnabled, must be set of 0xFF prior to first call.

*****************************************************************************/

static bool IsCPLBEnabled(void)
{
    if (!f_bCPLBEnabledFlagSet)
    {
        /* If data cache is enabled then we will let the physical driver know about it*/
        if( (__cplb_ctrl & CPLB_ENABLE_DCACHE ) || (__cplb_ctrl & CPLB_ENABLE_DCACHE2))
        {
            f_bCPLBEnabled = true;
        }
        else {
            f_bCPLBEnabled = false;
        }
        f_bCPLBEnabledFlagSet = true;
    }
    return f_bCPLBEnabled;
}


/*****************************************************************************
Function: _adi_fss_FlushMemory

    Flushes memory according to configuration settings

Parameters:
    *StartAddress - pointer to start of memory to be flushed
    NumBytes    - Number of bytes to flush
    Direction   - Directiom of Data flow

Returns:
    no return code


*****************************************************************************/

void _adi_fss_FlushMemory( char *StartAddress, u32 NumBytes, ADI_DMA_WNR Direction)
{
    /* If CPLB enabled, we need to flush (and invalidate) the data buffers to
     * ensure Cache-DMA coherency
    */
    if ( IsCPLBEnabled() )
    {
        if (Direction == ADI_DMA_WNR_WRITE)
        {
            FlushInvArea(  (void*)StartAddress, (void*)(StartAddress + NumBytes));
        }
        else
        {
            FlushArea(  (void*)StartAddress, (void*)(StartAddress + NumBytes));
        }
    }
}



/*****************************************************************************
Function: FlushArea

    Flushes the contents of the cache between the specified start and end
    addresses.

Parameters:
    StartAddress    - First location in memory to be flushed
    EndAddress      - Last location in memory to be flushed

Returns:
  none

Notes:
  none

*****************************************************************************/

static void FlushArea(void *StartAddress, void *EndAddress)
{
    StartAddress = (void *)(((unsigned long)StartAddress)&(~31));
    ssync();
    while (StartAddress < EndAddress)
        FLUSH(StartAddress);
    ssync();
}


/*****************************************************************************
Function: FlushArea

    Flushes and invalidates the contents of the cache between the specified
    start and end addresses.

Parameters:
    StartAddress    - First location in memory to be flushed
    EndAddress      - Last location in memory to be flushed

Returns:
  none

Notes:
  none

*****************************************************************************/

static void FlushInvArea(void *StartAddress, void *EndAddress)
{
    StartAddress = (void *)(((unsigned long)StartAddress)&(~31));
    ssync();
    while (StartAddress < EndAddress)
        FLUSHINV(StartAddress);
    ssync();
}


/*********************************************************************
                    END OF FILE
*********************************************************************/


/*********************************************************************************

Copyright(c) 2005 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_fat.c,v $
$Revision: 3939 $
$Date: 2010-12-10 09:07:11 -0500 (Fri, 10 Dec 2010) $

Description:
            This is the driver source code for FAT Filesystem support.

*********************************************************************************/

/*********************************************************************

Include files

*********************************************************************/

#define __ADI_FAT_C__


#include <services/services.h>          /* system services          */
#include <services/fss/adi_fss.h>       /* File System Service      */
#include <drivers/fsd/fat/adi_fat.h>    /* FAT driver header        */
#include <drivers/fsd/adi_fsd_cache.h>  /* FSD cache header         */


#include <ctype.h>
#include <string.h>
#include <stdio.h>

/*********************************************************************

Enumerations and defines

*********************************************************************/

/* Always build with format capability */
#ifndef ADI_USE_FAT_FORMAT
#define ADI_USE_FAT_FORMAT
#endif

#if 0
#define __ADI_FAT_SECTION_CODE          section("adi_fat_code") static
#define __ADI_FAT_SECTION_DATA          section("adi_fat_data")
#if defined(ADI_USE_FAT_FORMAT)
#define __ADI_FAT_FORMAT_SECTION_CODE   section("adi_fat_format_code") static
#define __ADI_FAT_FORMAT_SECTION_DATA   section("adi_fat_format_data")
#endif
#else
#define __ADI_FAT_SECTION_CODE          static
#define __ADI_FAT_SECTION_DATA
#if defined(ADI_USE_FAT_FORMAT)
#define __ADI_FAT_FORMAT_SECTION_CODE   static
#define __ADI_FAT_FORMAT_SECTION_DATA
#endif
#endif

#if defined(CACHE_DISABLED)
#undef __ADI_FAT_BLOCK_MODE
#else
#define __ADI_FAT_BLOCK_MODE
#endif

#if !defined(__ADI_FAT_DEFAULT_PERFORMANCE)
#define __ADI_FAT_DEFAULT_PERFORMANCE       ADI_FAT_CACHE_PERFORMANCE_FAST
#endif

#if !defined(__ADI_FAT_DEFAULT_FAT_CACHE_SIZE)
#define __ADI_FAT_DEFAULT_FAT_CACHE_SIZE    256
#endif

#if !defined(__ADI_FAT_DEFAULT_DIR_CACHE_SIZE)
#define __ADI_FAT_DEFAULT_DIR_CACHE_SIZE    16
#endif

#if !defined(__ADI_FAT_DEFAULT_FILE_CACHE_SIZE)
#define __ADI_FAT_DEFAULT_FILE_CACHE_SIZE    0
#endif

#if !defined(__ADI_FAT_DEFAULT_FORMAT_CACHE_SIZE)
#define __ADI_FAT_DEFAULT_FORMAT_CACHE_SIZE  __ADI_FAT_DEFAULT_FAT_CACHE_SIZE
#endif

#if !defined(__ADI_FAT_LFN_ENABLE)
#if !defined(__ADI_FAT_LFN_DISABLE)
#define __ADI_FAT_LFN_ENABLE
#endif
#endif


#define PDD_DEFAULT_HEAP        -1

#define PDD_READ                1
#define PDD_WRITE               0

#define PDD_BLOCK               1
#define PDD_NOBLOCK             0

#define ADI_FAT_SHORT_ENTRY_SIZE    11
#define ADI_FAT_SHORT_NAME_SIZE     13

#if defined(__ADI_FAT_LFN_ENABLE)
#define PDD_FAT_FILENAME_SIZE   256
#else
#define PDD_FAT_FILENAME_SIZE   ADI_FAT_SHORT_NAME_SIZE
#endif
#define PDD_FAT_PATHNAME_SIZE   260

#define ADI_FAT_ROOT_CLUSTER    0x00000000
#define ADI_FAT_START_OF_FILE   0xFFFFFFFF

#define ADI_FAT_FAT12_MASK      0x00000FFF
#define ADI_FAT_FAT16_MASK      0x0000FFFF
#define ADI_FAT_FAT32_MASK      0x0FFFFFFF

#define ADI_FAT_FIRST_CLUSTER   2

/* Macros to navigate chains of clsuter buffers (Block mode) */

#define GET_NEXT_LBA_BUFFER(P)    ((ADI_FSS_SUPER_BUFFER*)(P)->Buffer.pAdditionalInfo)
#define SET_NEXT_LBA_BUFFER(P,V)  ((P)->Buffer.pAdditionalInfo = (void*)(V));

/*******************************************************************
* Directory entry attribute values
*******************************************************************/

typedef enum
{
    ADI_FAT_SFN,                                // short filename (8.3 format)
    ADI_FAT_LFN,                                // long filename
    ADI_FAT_BAD_FILENAME = 0xFFFF
} ADI_FAT_NAMETYPE;

#define IS_ATTRIBUTE(V,A)       ( ((V)&(A))==A )

/* the following are used to distinguish only between files and sub-directories
 * in opening, closing, removing & renaming files and directories
*/
#define FILE_TYPE_FILE          1
#define FILE_TYPE_SUBDIR        2

/*******************************************************************
* FAT special clustesr values
*******************************************************************/
enum {
    ADI_FAT_FREE_CLUSTER        = 0x00000000,
    ADI_FAT_RESERVED_CLUSTER    = 0x00000001,
    ADI_FAT_BAD_CLUSTER         = 0x0FFFFFF7,
    ADI_FAT_LAST_CLUSTER        = 0x0FFFFFFF
};


/*******************************************************************
* FAT Type values
*******************************************************************/
enum {
    ADI_FAT_TYPE_FAT12,
    ADI_FAT_TYPE_FAT16,
    ADI_FAT_TYPE_FAT32,
    ADI_FAT_TYPE_NONE = 9999,
};


/*******************************************************************
* FAT seek enumeration type values
*******************************************************************/
typedef enum ADI_FAT_SEEK_TYPE {
    ADI_FAT_SEEK_TYPE_CHILD,
    ADI_FAT_SEEK_TYPE_PARENT
} ADI_FAT_SEEK_TYPE;


/*******************************************************************
* FAT Type values
*******************************************************************/
typedef enum ADI_FAT_CLUSTER_TYPE {
    ADI_FAT_CLUSTER_TYPE_ADD,
    ADI_FAT_CLUSTER_TYPE_READ
} ADI_FAT_CLUSTER_TYPE;


/*******************************************************************
* Directory Entry macros
*******************************************************************/
#define IsLastLongEntry(value)  ( (value & 0x40)==0x40 )
#define IsLongDirEntry(value)   ( value==ADI_FAT_DIR_LONG_FILE_NAME )




/*********************************************************************

Data Structures

*********************************************************************/

/*******************************************************************
* Directory entry structure - Short File name entry
*******************************************************************/
typedef struct {
    char    Name[ADI_FAT_SHORT_ENTRY_SIZE];       /* Short file name                                      */
    u8      Attr;           /* Type of entry                                        */
    u8      NTRes;          /* Reserved for NT                                      */
    u8      CrtTimeTenth;   /* Count of tenths of sec of creation time              */
    u16     CrtTime;        /* Creation time                                        */
    u16     CrtDate;        /* Creation Date                                        */
    u16     LastAccDate;    /* Date last accessed                                   */
    u16     FstClusHi;      /* High word of this entry's first cluster no. in FAT   */
    u16     WrtTime;        /* Time last written                                    */
    u16     WrtDate;        /* Date last written                                    */
    u16     FstClusLo;      /* Low word of this entry's first cluster no. in FAT    */
    u32     FileSize;       /* File size in bytes                                   */
} ADI_FAT_DIR_ENTRY;


/*******************************************************************
* Directory entry structure - Long File name entry
*******************************************************************/
typedef struct {
    u8      Ord;            /* This entry's position in long file name list (0x40 terminates list) */
    u8      Name1[10];      /* First part of long name                                             */
    u8      Attr;           /* Attribute, must be ADI_FAT_DIR_LONG_FILE_NAME (0xF)                 */
    u8      Type;           /* If zero - sub-element of long name                                  */
    u8      Chksum;         /* Checksum of short dir entry name                                    */
    u8      Name2[12];      /* Second part of long name                                            */
    u16     FstClusLo;      /* Must be zero                                                        */
    u8      Name3[4];       /* third part of long name                                             */
} ADI_FAT_LFN_ENTRY;


/*******************************************************************
* FAT File Date/Time Macros
*******************************************************************/
#define FAT_YEAR(X)     (((X>>9)&0x007F)+80)
#define FAT_MONTH(X)    (((X>>5)&0x000F)-1)
#define FAT_DAY(X)      (X&0x001F)
#define FAT_HOUR(X)     ((X>>11)&0x001F)
#define FAT_MINUTE(X)   ((X>>5)&0x003F)
#define FAT_SECOND(X)   (X&0x001E)

#define FAT_DATE(X) (               \
    (X.tm_mday&0x001F)|             \
    (((X.tm_mon+1)&0x000F)<<5)|     \
    (((X.tm_year-80)&0x007F)<<9) )

#define FAT_TIME(X) (               \
    (FAT_SECOND(X.tm_sec))|              \
    (((X.tm_min)&0x003F)<<5)|       \
    (((X.tm_hour)&0x001F)<<11) )


/*******************************************************************
* Boot Jump structure
*******************************************************************/
#pragma pack(1)
typedef struct {
    u8 instr;
    u16 addr;
} ADI_FAT_BOOTJUMP;


/*******************************************************************
* Boot Sector/Bios Partition Block structure = FAT12/16
*******************************************************************/
typedef struct {
    // offset       size &  Entity                 Description
    /*   0    */    ADI_FAT_BOOTJUMP    BS_jmpBoot;         // Jump Instruction to boot code; { 0xEB, 0x??, 0x90 } is common
                                                                // alternatively: { 0xE9, 0x??, 0x?? }
    /*   3    */    char    BS_OEMName[8];      // "MSWIN4.1"
    /*   11   */    u16     BPB_BytsPerSec;     // Count of bytes per sector. one of 512,1024,2048 or 4096.
                                                // 512 guarantees maximum compatibility
    /*   13   */    u8      BPB_SecPerClus;     // Number of sectors per allocation unit. 1, 2, 4, 8, 16, 32, 64 or 128
                                                // BPB_BytsPerSec*BPB_SecPerClus <=32K
    /*   14   */    u16     BPB_RsvdSecCnt;     // Number of reserved sectors in the Reserved region of the volume
                                                // FAT12/FAT16 - 1; FAT32 it is typically 32
    /*   16   */    u8      BPB_NumFATs;        // The count of FAT data structures on the volume.
    /*   17   */    u16     BPB_RootEntCnt;     // For FAT12 and FAT16 volumes, this field contains the count of 32-
                                                // byte directory entries in the root directory. For FAT32 volumes,
                                                // this field must be set to 0.
    /*   19   */    u16     BPB_TotSec16;       // the old 16-bit total count of sectors on the volume.
    /*   21   */    u8      BPB_Media;          // Media Type: 0xF8 = fixed, 0xF0 is common for removable media
                                                // legal values: 0xF0, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, and 0xFF
    /*   22   */    u16     BPB_FATSz16;        // the FAT12/FAT16 16-bit count of sectors occupied by ONE FAT
    /*   24   */    u16     BPB_SecPerTrk;      // Sectors per track geometry value for relevant media visible on interrupt 0x13.
    /*   26   */    u16     BPB_NumHeads;       // Number of heads for interrupt 0x13.
    /*   28   */    u32     BPB_HiddenSec;      // Count of hidden sectors preceding the partition that contains this FAT volume.
    /*   32   */    u32     BPB_TotSec32;       // This field is the new 32-bit total count of sectors on the volume.
} ADI_FAT_BOOT_SECTOR_START;


typedef struct {
    /*   36   */    u32     BPB_FATSz32;        // the FAT32 32-bit count of sectors occupied by ONE FAT. BPB_FATSz16 must be 0.
    /*   40   */    u16     BPB_ExtFlags;       // Defines FAT mirroring
    /*   42   */    u16     BPB_FSVer;          // High byte is major revision number. Low byte is minor revision number. 0:0
    /*   44   */    u32     BPB_RootClus;       // the cluster number of the first cluster of the root directory.
    /*   48   */    u16     BPB_FSInfo;         // Sector number of FSINFO structure in the reserved area of the FAT32 volume
    /*   50   */    u16     BPB_BkBootSec;      // If non-zero, indicates the sector number in the reserved area of the volume
                                                // of a copy of the boot record. Usually 6. No value other than 6 is recommended.
    /*   52   */    u8      BPB_Reserved[12];   // Code that formats FAT volumes should always set all bytes to 0
} ADI_FAT_BOOT_SECTOR_MIDDLE;


typedef struct {
    /*   36   */    u8      BS_DrvNum;          // Int 0x13 drive number.
    /*   37   */    u8      BS_Reserved1;       // Reserved (used by Windows NT).Code that formats FAT volumes should always set this byte to 0.
    /*   38   */    u8      BS_BootSig;         // Extended boot signature (0x29). Indicates that the following three fields in the boot sector are present
    /*   39   */    u32     BS_VolID;           // Volume serial number.
    /*   43    */   char    BS_VolLab[11];      // Volume label
    /*   54    */   char    BS_FilSysType[8];   // One of the strings "FAT12   ", "FAT16   ", or "FAT     ". Informational only
                                                // Always set to "FAT32   " for FAT32
} ADI_FAT_BOOT_SECTOR_END;


typedef struct {
    ADI_FAT_BOOT_SECTOR_START;
    ADI_FAT_BOOT_SECTOR_END;
} ADI_FAT_BOOT_SECTOR_FAT16;


typedef struct {
    ADI_FAT_BOOT_SECTOR_START;
    ADI_FAT_BOOT_SECTOR_MIDDLE;
    ADI_FAT_BOOT_SECTOR_END;
} ADI_FAT_BOOT_SECTOR_FAT32;


typedef struct {
    u32     ExtendedBootSignature;  /* This equals 0x41615252 */
    u8      Reserved1[480];         /* Unused */
    u32     FSInfoSignature;        /* Must equal 0x61417272h */
    u32     FreeClusterCount;       /* The number of unused clusters, -1 if unknown */
    u32     NextFreeCluster;        /* The last allocated cluster */
    u8      Reserved2[12];          /* Unused */
    u32     FSInfoEndSignature;     /* This equals 0xAA550000h */
} ADI_FAT_FSINFO_SECTOR_FAT32;

#pragma pack()




/*********************************************************************

   Device Initialization data

*********************************************************************/

/*******************************************************************
* FAT Cluster Definition
*******************************************************************/
typedef struct ADI_FAT_CLUSTER_DEF
{
    u32                     Cluster;    /* Current cluster number, if cluster
                                            number is zero then we are in the
                                            root directory of a FAT12/FAT16
                                            partition */
    u32                     Offset;     /* Offset in bytes within current chunk */
} ADI_FAT_CLUSTER_DEF;


/*******************************************************************
* FAT File Descriptor Structure
*******************************************************************/
typedef struct ADI_FAT_FILE_DESCRIPTOR
{
    ADI_FAT_CLUSTER_DEF     Entry;      /* Location of file directory entry     */
    ADI_FAT_CLUSTER_DEF     File;       /* File location                        */

    u32                     FirstCluster;   /* File/Dir content cluster address */
    u32                     ParentCluster;  /* Parent cluster address           */
    u32                     NextCluster;    /* Next cluster address             */

    u32                     Position;       /* Current position in file         */
    u32                     SeekPosition;   /* Desired position in file         */
} ADI_FAT_FILE_DESCRIPTOR;


/*******************************************************************
* Mounted Volume Instance structure
*******************************************************************/
typedef struct {
    ADI_DEV_MANAGER_HANDLE  DMHandle;
    ADI_DEV_DEVICE_HANDLE   DeviceHandle;           /* device manager handle                                    */
    ADI_DCB_HANDLE          DCBHandle;              /* callback manager handle                                  */
    void                    *ClientHandle;          /* Handle to pass as 1st argument in callback               */
    ADI_DCB_CALLBACK_FN     DMCallback;                         /* the callback function supplied by the Device Manager     */
    ADI_DMA_MANAGER_HANDLE  DMAHandle;                          /* The DMA Manager Handle                                   */
    void                    *pCriticalRegionArg;                /* critical region argument                                 */
    ADI_DEV_DEVICE_HANDLE   PIDHandle;                    /* Handle to Physical Interface Driver                      */
    u32                     DataElementWidth;                   /* Width of Data Element                                    */
    u32                     PID_supports_background_transfer;   /* flag to indicate whether PID uses peripheral DMA */

    u32 ReadOnly;           /* flag to indicate whether volume is mounted for read-only access */
    int CacheHeapID;        /* Index of heap to use for data buffers */
    ADI_SEM_HANDLE          LockSemaphoreHandle;
    ADI_SEM_HANDLE          DataSemaphoreHandle;

    ADI_FSD_CACHE_HANDLE    FatCacheHandle;
    ADI_FSD_CACHE_HANDLE    DirCacheHandle;

    u32                     Type;               /* type of FAT (12, 16, or 32)  */
    u32                     DeviceNumber;       /* The Device Number */

    u32                     AccessDateFlag;     /* TRUE: Access date is updated on open/close/modify,
                                                   FALSE: Access date is updated on modify only */
    u32                     bShowHiddenSystem;  /* TRUE enables the reporting of the directory 
                                                   entry FALSE disables the reporting. */

    u32                     FSInfoAddress;      /* Sector location of FSInfo */
    
    u32                     FatAddress;         /* Sector location of FAT */
    u32                     FatSize;            /* Size of the FAT in sectors */
    u32                     DataAddress;        /* Sector location of start of data area */
    u32                     DataSize;           /* Size of the Data area in clusters */

    u32                     SectorSize;         /* Sector Size */
    u32                     ClusterSize;        /* Number of sectors per cluster */
    u32                     VolumeSize;         /* Total number of sectors for FAT partition */
    u32                     FreeCluster;        /* Sector number of the last known free cluster */
    u32                     Clusters;           /* The maximum cluster number */
    u32                     bMediaFull;         /* boolean flag indicating whether volume is full */
    u32                     bCollectUsageStats; /* boolean flag indicating whether to generate usage stats */

    u32                     ClusterMask;        /* FAT Cluster mask */
    u32                     nFreeClusterCount;  /* Number of available clusters */
    u32                     nTotalClusters;     /* Total number of clusters in volume */

    u32                     RootDirAddress;     /* Sector number of root directory */
    u32                     CWDAddress;

    u32                     Performance;
    u32                     FatCacheSize;
    u32                     DirCacheSize;
    u32                     MaxTransferCount;
    u32                     SemTimeOutArg;

    ADI_FSS_WCHAR           VolumeLabel[12];
    ADI_FSS_WCHAR           VolumeType[9];
} ADI_FAT_VOLUME;

/* ************************************************
 * DEVICE DRIVER ENTRY POINTS
 * ************************************************
 */

static u32 adi_pdd_Open(        /* Open a device */
    ADI_DEV_MANAGER_HANDLE  DMHandle,               /* device manager handle                   */
    u32                     DeviceNumber,           /* device number                           */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,           /* device handle                           */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,            /* pointer to PDD handle location          */
    ADI_DEV_DIRECTION       Direction,              /* data direction                          */
    void                    *pCriticalRegionArg,    /* critical region imask storage location  */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,              /* handle to the DMA manager               */
    ADI_DCB_HANDLE          DCBHandle,              /* callback handle                         */
    ADI_DCB_CALLBACK_FN     DMCallback              /* device manager callback function        */
);

static u32 adi_pdd_Close(       /* Closes a device */
    ADI_DEV_PDD_HANDLE      PDDHandle               /* PDD handle               */
);

static u32 adi_pdd_Read(        /* Reads data or queues an inbound buffer to a device */
    ADI_DEV_PDD_HANDLE      PDDHandle,              /* PDD handle               */
    ADI_DEV_BUFFER_TYPE     BufferType,             /* buffer type              */
    ADI_DEV_BUFFER          *pBuffer                /* pointer to buffer        */
);

static u32 adi_pdd_Write(       /* Writes data or queues an outbound buffer to a device */
    ADI_DEV_PDD_HANDLE      PDDHandle,              /* PDD handle               */
    ADI_DEV_BUFFER_TYPE     BufferType,             /* buffer type              */
    ADI_DEV_BUFFER          *pBuffer                /* pointer to buffer        */
);

static u32 adi_pdd_Control(     /* Sets or senses a device specific parameter */
    ADI_DEV_PDD_HANDLE      PDDHandle,              /* PDD handle               */
    u32                     Command,                /* command ID               */
    void                    *Value                  /* command specific value   */
);


/* ************************************************
 * Entry point structure for device manager
 * ************************************************
 */

ADI_DEV_PDD_ENTRY_POINT ADI_FAT_EntryPoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control
};


/* ************************************************
 * DRIVER HOUSEKEEPING FUNCTIONS
 * ************************************************
 */

static u32  MountVolume(
    ADI_FAT_VOLUME          *pVolume,
    u32                     DeviceNumber,
    u32                     StartSector,
    u32                     SectorSize
);

static u32 UnmountVolume(
    ADI_FAT_VOLUME          *pVolume
);


/* ************************************************
 * BOOT SECTOR OPERATIONS
 * ************************************************
 */
static u32 ReadBootSector(
    ADI_FAT_VOLUME          *pVolume,
    u32                     StartSector,
    u8                      *pData
);

static u32 ReadFSInfoSector(
    ADI_FAT_VOLUME          *pVolume
);

static void WriteFSInfoSector(
    ADI_FAT_VOLUME          *pVolume,
    bool                    bWriteBack
);

/* ************************************************
 * FILE OPERATIONS
 * ************************************************
 */

static u32 OpenFile(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_FILE_DESCRIPTOR *pFileDef,
    u32                     FileType
);

static u32 CloseFile(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_FILE_DESCRIPTOR *pFileDef,
    u32                     FileType
);

static u32 RemoveFile(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_FULL_FNAME      *pFullFileName,
    u32                     FileType
);

static u32 RenameFile(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_RENAME_DEF      *pRenameDef
);

static u32 StatFile(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_FILE_DESCRIPTOR *pFileDef
);

static u32 SeekFile(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_SEEK_REQUEST    *pSeekDef
);

static u32 ChangeFileAttributes(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_DIR_DEF         *pDirDef,
    u32                     Action
);

/* ************************************************
 * DIRECTORY OPERATIONS
 * ************************************************
 */

static u32 OpenDirectory(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_DIR_DEF         *pDirDef
);

static u32 GetDirectory(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_DIR_DEF         *pDirDef
);

static u32 SeekDirectory(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_DIR_DEF         *pDirDef
);

static u32 RewindDirectory(
    ADI_FSS_DIR_DEF         *pDirDef
);

static u32 CreateDirectory(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_DIR_DEF         *pDirDef
);

static u32 ResetDirectory(
    ADI_FAT_VOLUME          *pVolume
);

static u32 SetDirectory(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_DIR_DEF         *pDirDef
);


/* ************************************************
 * DIRECTORY ENTRY OPERATIONS
 * ************************************************
 */

static u32 SeekDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_FULL_FNAME      *pFileName,
    ADI_FAT_SEEK_TYPE       Type,
    u32                     *pCluster,
    u32                     *pOffset,
    u8                      *pAttribute,
    u32                     *pFirstCluster,
    u32                     *pFileSize
);

static u32 FindDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ParentCluster,
    ADI_FSS_WCHAR           *pFileName,
    u32                     *pCluster,
    u32                     *pOffset,
    u8                      *pAttribute,
    u32                     *pFirstCluster,
    u32                     *pFileSize
);

static u32 GetDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     FirstCluster,
    u32                     *pCluster,
    u32                     *pOffset,
    ADI_FSS_WCHAR           *pShortName,
    ADI_FSS_WCHAR           *pLongName,
    u8                      *pAttribute,
    u32                     *pFirstCluster,
    u32                     *pFileSize
);

static u32 AddDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ParentCluster,
    ADI_FSS_WCHAR           *pFileName,     /* Filename of new Dir entry */
    u8                      FileType,       /* Type of Dir entry to add */
    u32                     FirstCluster,   /* First cluster */
    u32                     *pNewCluster,
    u32                     *pNewOffset
);

static u32 CreateDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FAT_DIR_ENTRY       *pEntry,
    u8                      *Name,
    u8                      Type,
    u32                     Cluster,
    u32                     Size
);

static u32 UpdateDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     EntrySector,
    u32                     EntryOffset,
    u32                     FileSize
);

static u32 RemoveDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     EntrySector,
    u32                     EntryOffset
);

static u32 GetSizeDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_WCHAR           *pFileName
);


static u32 GetClusterDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FAT_DIR_ENTRY       *pEntry
);

static u32 SetClusterDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FAT_DIR_ENTRY       *pEntry,
    u32                     Cluster
);


static u32 ReadDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ParentCluster,
    u32                     *pCluster,
    u32                     *pOffset,
    ADI_FAT_DIR_ENTRY       **pEntry
);

static u32 ReadNextDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     *pCluster,
    u32                     *pOffset,
    ADI_FAT_DIR_ENTRY       **pEntry,
    ADI_FAT_CLUSTER_TYPE    Type
);

static u32 WriteDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FAT_DIR_ENTRY       **pEntry
);

static u32 WriteNextDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     *pCluster,
    u32                     *pOffset,
    ADI_FAT_DIR_ENTRY       **pEntry,
    ADI_FAT_CLUSTER_TYPE    Type
);

static u32 ReleaseDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FAT_DIR_ENTRY       **pEntry
);

static u32 MatchShortEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     EntryCluster,
    char                    *pShortEntry
);

/* ************************************************
 * DIRECTORY CLUSTER OPERATIONS
 * ************************************************
 */

static u32 GetDirCluster(
    ADI_FAT_VOLUME          *pVolume,
    u32                     FirstCluster,
    u32                     CurrentCluster,
    u32                     *pNextCluster
);

static u32 GetNextDirCluster(
    ADI_FAT_VOLUME          *pVolume,
    u32                     Type,
    u32                     *pCluster,
    u32                     Offset
);

static u32 AddDirCluster(
    ADI_FAT_VOLUME          *pVolume,
    u32                     EntrySector,
    u32                     EntryOffset,
    u32                     *pFirstCluster,
    u32                     *pCluster,
    u32                     Clear
);

static u32 ReadDirCluster(
    ADI_FAT_VOLUME          *pVolume,
    u32                     FirstCluster,
    u32                     *pCluster,
    u32                     *pOffset,
    u8                      **pCacheData
);

static u32 ReleaseDirCluster(
    ADI_FAT_VOLUME          *pVolume,
    u8                      **pCacheData
);

static u32 WriteDirCluster(
    ADI_FAT_VOLUME          *pVolume,
    u8                      **pCacheData
);

static u32 GetClusterSectorNumber(
    ADI_FAT_VOLUME          *pVolume,
    u32                     Cluster,
    u32                     Offset
);

static u32 GetClusterSectorSize(
    ADI_FAT_VOLUME          *pVolume,
    u32                     Cluster,
    u32                     Offset
);



/* ************************************************
 * FAT ENTRY OPERATIONS
 * ************************************************
 */

static u32 GetFatEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ClusterNumber,
    u32                     *pNextCluster
);

static u32 SetFatEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ClusterNumber,
    u32                     Value
);

static u32 AddFatEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ClusterNumber,
    u32                     *pNextCluster,
    u32                     InitFlag
);

static u32 RemoveFatEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ClusterNumber
);


static u32 ReadFatEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ClusterNumber,
    u8                      **pFatEntry
);

static u32 ReleaseFatEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ClusterNumber,
    u8                      **pFatEntry
);

static u32 WriteFatEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ClusterNumber,
    u8                      **pFatEntry
);



/* ************************************************
 * MISCELLANEOUS OPERATIONS
 * ************************************************
 */
static u32 ProcessBufferChain(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_SUPER_BUFFER    *pBuffer,
    u32                     ReadFlag
);

static u32 Seek(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ReadFlag,
    ADI_FSS_FILE_DESCRIPTOR *pFile
);

static void Callback(
    void                    *pHandle,
    u32                     Event,
    void                    *pArg
);

static ADI_FAT_NAMETYPE ValidateName(
    ADI_FSS_WCHAR           *Name
);

static ADI_FAT_NAMETYPE ValidateNameChar(
    ADI_FSS_WCHAR           ch
);

#if defined(__ADI_FAT_LFN_ENABLE)
static u8 ShortEntryCheckSum(
    u8                      *ShortEntry
);

static u32 GetShortEntryFromLFN(
    ADI_FAT_VOLUME          *pVolume,
    u32                     EntryParent,
    ADI_FSS_WCHAR           *pFileName,
    ADI_FSS_WCHAR           *pNameEnd,
    u8                      *pShortEntry);
#endif

static u8 IsValidDirAttribute(
    u8                      Attribute
);

static u8 IsWriteAccess(
    ADI_FSS_FILE_DESCRIPTOR *pFileDef
);


static u32 GetParentName(
    ADI_FSS_FULL_FNAME      *pFullName,
    ADI_FSS_WCHAR           *pParentName
);

static u32 GetFileName(
    ADI_FSS_FULL_FNAME      *pFullName,
    ADI_FSS_WCHAR           *pFileName
);

#if defined(ADI_USE_FAT_FORMAT)
static u32 Format(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_FORMAT_DEF      *pFormatDef
);
#endif

static void CountFreeClusters(
    ADI_FAT_VOLUME          *pVolume
);

static bool IsFileEntry( u8 Attribute );
static bool IsFileOrDirEntry( u8 Attribute );

/********************************************************************

    Function:       ValidatePDDHandle

    Description:    Validates a PDD handle

********************************************************************/
#if defined(ADI_DEV_DEBUG)
__ADI_FAT_SECTION_CODE
int ValidatePDDHandle(ADI_DEV_PDD_HANDLE PDDHandle)
{
    return (PDDHandle==NULL)?ADI_DEV_RESULT_BAD_PDD_HANDLE:ADI_DEV_RESULT_SUCCESS;
}
#else
#define ValidatePDDHandle(X) ADI_DEV_RESULT_SUCCESS
#endif


/*********************************************************************
* non-public FSS functions
*********************************************************************/
extern void *_adi_fss_malloc(
    int                     id,
    size_t                  size
);

extern void _adi_fss_free(
    int                     id,
    void                    *p
);

extern u32 _adi_fss_PIDTransfer(
    ADI_DEV_DEVICE_HANDLE   PIDHandle,
    ADI_FSS_SUPER_BUFFER    *pBuffer,
    u32                     BlockFlag
);




/************************************************************************************
*************************************************************************************

                            DEVICE DRIVER ENTRY POINTS

*************************************************************************************
*************************************************************************************/


/*********************************************************************

    Function:       adi_pdd_Open

    Description:    Opens a FAT Device Driver for use

*********************************************************************/
__ADI_FAT_SECTION_CODE
u32 adi_pdd_Open(                        /* Open a device */
    ADI_DEV_MANAGER_HANDLE  DMHandle,               /* device manager handle                   */
    u32                     DeviceNumber,           /* device number (ignored)                 */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,           /* device handle                           */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,            /* pointer to PDD handle location          */
    ADI_DEV_DIRECTION       Direction,              /* data direction                          */
    void                    *pCriticalRegionArg,    /* critical region imask storage location  */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,              /* handle to the DMA manager               */
    ADI_DCB_HANDLE          DCBHandle,              /* callback handle                         */
    ADI_DCB_CALLBACK_FN     DMCallback)             /* device manager callback function        */
{
    u32 Result;
    ADI_FAT_VOLUME *pVolume;


    Result=ADI_DEV_RESULT_SUCCESS;

    /* Allocate memory for a Volume instance */
    pVolume=(ADI_FAT_VOLUME *)_adi_fss_malloc(PDD_DEFAULT_HEAP,sizeof(*pVolume));
    if (!pVolume)
    {
        Result=ADI_FSS_RESULT_NO_MEMORY;
    }


    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        pVolume->Type                   = ADI_FAT_TYPE_NONE;

        /*initialize the Device Header structure */
        pVolume->DMHandle               = DMHandle;
        pVolume->DCBHandle              = DCBHandle;
        pVolume->DMCallback             = DMCallback;
        pVolume->DeviceHandle           = DeviceHandle;
        pVolume->pCriticalRegionArg     = pCriticalRegionArg;
        pVolume->CacheHeapID            = PDD_DEFAULT_HEAP;

        pVolume->Performance            = __ADI_FAT_DEFAULT_PERFORMANCE;

        /* Disable updating the Access date on reads */
        pVolume->AccessDateFlag         = false;
        
        /* Disable reporting of hidden and system files */
        pVolume->bShowHiddenSystem      = false;

        /* Initialize the Volume Usage Counters */
        pVolume->nFreeClusterCount      = 0xFFFFFFFF;
        pVolume->nTotalClusters         = 0;
        pVolume->bMediaFull             = false;
        
        /* disable usage stats by default for backward compatibility */
        pVolume->bCollectUsageStats     = false;
        pVolume->FSInfoAddress          = 0;
        
        pVolume->FatCacheHandle         = NULL;
        pVolume->FatCacheSize           = __ADI_FAT_DEFAULT_FAT_CACHE_SIZE;

        pVolume->DirCacheHandle         = NULL;
        pVolume->DirCacheSize           = __ADI_FAT_DEFAULT_DIR_CACHE_SIZE;
        
        /* Default MaxTransferCount is set to 0 initially */
        pVolume->MaxTransferCount       = 1;

        /* Default fault tolerance settings */
        pVolume->SemTimeOutArg = 0;

        /* Create Semaphore for Exclusive Access */
        adi_sem_Create(1,&pVolume->LockSemaphoreHandle,NULL);

        /* Create Semaphore for transfer completion */
        adi_sem_Create(0,&pVolume->DataSemaphoreHandle,NULL);

        pVolume->ReadOnly               = FALSE;
        if ((Direction&ADI_DEV_DIRECTION_OUTBOUND)!=ADI_DEV_DIRECTION_OUTBOUND)
        {
            pVolume->ReadOnly           = TRUE;
        }
    }

    /*save the physical device handle in the client supplied location */
    *pPDDHandle=(ADI_DEV_PDD_HANDLE *)pVolume;

    return Result;
}


/*********************************************************************

    Function:       adi_pdd_Close

    Description:    Closes an instance of a FAT driver

*********************************************************************/
__ADI_FAT_SECTION_CODE
u32 adi_pdd_Close(              /* Closes a device */
    ADI_DEV_PDD_HANDLE      PDDHandle)  /* PDD handle               */
{
    u32 Result;

    /* check for errors if required */
#if defined(ADI_DEV_DEBUG)
    Result=ValidatePDDHandle(PDDHandle);
#else
    Result=ADI_DEV_RESULT_SUCCESS;
#endif

    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        ADI_FAT_VOLUME *pVolume;

        /* Assign pointer to the required Volume instance */
        pVolume=(ADI_FAT_VOLUME *)PDDHandle;

        /* Unmount volume if mounted */
        UnmountVolume(pVolume);

        /* Delete Semaphore for Exclusive Access */
        adi_sem_Delete(pVolume->LockSemaphoreHandle);

        /* Delete Semaphore for transfer completion */
        adi_sem_Delete(pVolume->DataSemaphoreHandle);

        /* Free Volume Instance memory */
        _adi_fss_free(PDD_DEFAULT_HEAP,pVolume);
    }

    return(ADI_DEV_RESULT_SUCCESS);
}


/*********************************************************************

    Function:       adi_pdd_Read

    Description:    Read entry point

*********************************************************************/
__ADI_FAT_SECTION_CODE
u32 adi_pdd_Read(        /* Reads data or queues an inbound buffer to a device */
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle               */
    ADI_DEV_BUFFER_TYPE     BufferType, /* buffer type              */
    ADI_DEV_BUFFER          *pBuffer)   /* pointer to buffer        */
{
    u32 Result;
    ADI_FAT_VOLUME *pVolume;


#if 0//defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS)
        return (Result);
    /* Only 1D Buffer allowed */
    if ( BufferType != ADI_DEV_1D )
        return ADI_DEV_RESULT_FAILED;
    /* Only u8 buffers are allowed */
    if ( pBuffer->OneD.ElementWidth!= sizeof(u8) ))
        return ADI_DEV_RESULT_FAILED;
#endif

    /* Assign pointer to the required Volume instance */
    pVolume=(ADI_FAT_VOLUME *)PDDHandle;

    /*  Process buffers, assigning LBA request buffers */
    Result=ProcessBufferChain(
        pVolume,
        (ADI_FSS_SUPER_BUFFER*)pBuffer,
        PDD_READ);

    return (Result);
}


/*********************************************************************

    Function:       adi_pdd_Write

    Description:    Write entry point

*********************************************************************/
__ADI_FAT_SECTION_CODE
u32 adi_pdd_Write(       /* Writes data or queues an outbound buffer to a device */
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle               */
    ADI_DEV_BUFFER_TYPE     BufferType, /* buffer type              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer        */
){
    u32 Result;
    ADI_FAT_VOLUME *pVolume;


#if 0// defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS)
        return (Result);
    /* Only 1D Buffer allowed */
    if ( BufferType != ADI_DEV_1D )
        return ADI_DEV_RESULT_FAILED;
    /* Only u8 buffers are allowed */
    if ( pBuffer->OneD.ElementWidth!= sizeof(u8) ))
        return ADI_DEV_RESULT_FAILED;
#endif

    /* Assign pointer to the required Volume instance */
    pVolume=(ADI_FAT_VOLUME *)PDDHandle;

    Result=ProcessBufferChain(
        pVolume,
        (ADI_FSS_SUPER_BUFFER*)pBuffer,
        PDD_WRITE);

    /* Now update the directory entry with modification time and
        new file size
    */
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        ADI_FAT_FILE_DESCRIPTOR *pFileDesc;

        pFileDesc=(ADI_FAT_FILE_DESCRIPTOR *)((ADI_FSS_SUPER_BUFFER *)pBuffer)->pFileDesc->FSD_data_handle;

        if (((ADI_FSS_SUPER_BUFFER *)pBuffer)->pFileDesc->fsize>=0x0FFF0000)
        {
            Result=ADI_DEV_RESULT_SUCCESS;
        }

        Result=UpdateDirEntry(
            pVolume,
            pFileDesc->Entry.Cluster,
            pFileDesc->Entry.Offset,
            ((ADI_FSS_SUPER_BUFFER *)pBuffer)->pFileDesc->fsize);
    }

    return Result;
}


/*********************************************************************

    Function:       adi_pdd_Control

    Description:    Configures the PPI device

*********************************************************************/
__ADI_FAT_SECTION_CODE
u32 adi_pdd_Control(     /* Sets or senses a device specific parameter */
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle               */
    u32                     Command,    /* command ID               */
    void                    *Value)     /* command specific value   */
{
    u32 Result;
    u32 FSIResult;
    ADI_FAT_VOLUME *pVolume;


    /* Assign pointer to the required Volume instance */
    pVolume=(ADI_FAT_VOLUME *)PDDHandle;

    /* check for errors if required */
#if 0//defined(ADI_DEV_DEBUG)
    if ((Result = ValidatePDDHandle(PDDHandle)) != ADI_DEV_RESULT_SUCCESS) return (Result);
#endif

    /* assume we're going to be successful */
    Result=ADI_DEV_RESULT_SUCCESS;

    /* CASEOF (Command ID) */
    switch (Command)
    {
        /* CASE ELSE */
        default:
            /* we don't understand this command */
            Result=ADI_DEV_RESULT_NOT_SUPPORTED;
            break;

        /* CASE (control dataflow) */
        case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):
            break;

        case (ADI_DEV_CMD_SET_DATAFLOW):
            break;

        /* CASE (query for processor DMA support) */
        case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
            /* No, we do not support it */
            *((u32 *)Value)=FALSE;
            break;

        /* CASE ( Set Index of heap to use for data buffers ) */
        case ADI_FSS_CMD_SET_CACHE_HEAP_ID:
            pVolume->CacheHeapID=(int)Value;
            break;

        /* CASE (Determine if this FSD supports the given File System Type ) */
        case (ADI_FSD_CMD_GET_FILE_SYSTEM_SUPPORT):
            if ((u32)Value!=ADI_FSS_FSD_TYPE_FAT)
            {
                Result=ADI_DEV_RESULT_NOT_SUPPORTED;
            }
            break;

        /* CASE (return string defining File System Type ) */
        case (ADI_FSD_CMD_GET_TYPE_STRING):
            *(ADI_FSS_WCHAR**)Value=&pVolume->VolumeType[0];
            break;

        /* CASE (Mount volume at given LBA start address) */
        case (ADI_FSD_CMD_MOUNT_VOLUME):
            {
                ADI_FSS_VOLUME_DEF *pVolumeDef;

                pVolumeDef=(ADI_FSS_VOLUME_DEF*)Value;

                Result=MountVolume(
                    pVolume,
                    pVolumeDef->DeviceNumber,
                    pVolumeDef->StartAddress,
                    pVolumeDef->SectorSize);

                /* Calculate remaining clusters on volume */
                if (Result == ADI_DEV_RESULT_SUCCESS) {
                    if (pVolume->bCollectUsageStats) 
                    {
                        FSIResult = ReadFSInfoSector(pVolume);
                        if ((FSIResult != ADI_DEV_RESULT_SUCCESS) || pVolume->nFreeClusterCount == 0xFFFFFFFF)
                        {
                            CountFreeClusters( pVolume );
                        }
                    }                   
                    /* Determine the max transfer size from the PID
                     * Backwardly compatible: if PID does not support the request we force once cluster transfers only.
                     */
                    if ( ADI_DEV_RESULT_NOT_SUPPORTED == adi_dev_Control(pVolume->PIDHandle,ADI_PID_CMD_GET_MAX_TFRCOUNT,&pVolume->MaxTransferCount) )
                    {
                        pVolume->MaxTransferCount = pVolume->ClusterSize * pVolume->SectorSize;
                    }

                }
                
                /* If the Volume size is unknown, assign it the value of the
                 * partition size
                */
                if (pVolumeDef->VolumeSize==0)
                {
                    pVolumeDef->VolumeSize=pVolume->VolumeSize;
                }
            }
            break;

        /* CASE (Unmount volume ) */
        case (ADI_FSD_CMD_UNMOUNT_VOLUME):
            Result=UnmountVolume(pVolume);
            break;

        /* CASE (Move to given directory) */
        case (ADI_FSD_CMD_CHANGE_DIR):
            Result=SetDirectory(
                pVolume,
                (ADI_FSS_DIR_DEF*)Value);
            break;

        /* CASE (seek request) */
        case (ADI_FSD_CMD_SEEK_FILE):
            Result=SeekFile(
                pVolume,
                (ADI_FSS_SEEK_REQUEST*)Value);
            break;

        /* CASE (Open File) */
        case (ADI_FSD_CMD_OPEN_FILE):
            Result=OpenFile(
                pVolume,
                (ADI_FSS_FILE_DESCRIPTOR *)Value,
                FILE_TYPE_FILE);
            break;

        /* CASE (Close File) */
        case (ADI_FSD_CMD_CLOSE_FILE):
            Result=CloseFile(
                pVolume,
                (ADI_FSS_FILE_DESCRIPTOR *)Value,
                FILE_TYPE_FILE);
            break;

        /* CASE (Open Directory) */
        case (ADI_FSD_CMD_OPEN_DIR):
            Result=OpenDirectory(
                pVolume,
                (ADI_FSS_DIR_DEF *)Value);
            break;

        /* CASE (Close Directory) */
        case (ADI_FSD_CMD_CLOSE_DIR):
            Result=CloseFile(
                pVolume,
                ((ADI_FSS_DIR_DEF *)Value)->pFileDesc,
                FILE_TYPE_SUBDIR);
            break;

        /* CASE (Read Directory) */
        case (ADI_FSD_CMD_READ_DIR):
            Result=GetDirectory(
                pVolume,
                (ADI_FSS_DIR_DEF *)Value);
            break;

        /* CASE (Add File Attribute) */
        case (ADI_FSD_CMD_ADD_FILE_ATTR):
        /* CASE (Remove File Attribute) */
        case (ADI_FSD_CMD_REMOVE_FILE_ATTR):
        /* CASE (Replace File Attributes) */
        case (ADI_FSD_CMD_REPLACE_FILE_ATTR):
            Result = ChangeFileAttributes(pVolume,(ADI_FSS_DIR_DEF *)Value, Command);
            break;

        /* CASE (Seek in Directory) */
        case (ADI_FSD_CMD_SEEK_DIR):
            Result=SeekDirectory(
                pVolume,
                (ADI_FSS_DIR_DEF *)Value);
            break;

        /* CASE (Rewind Directory) */
        case (ADI_FSD_CMD_REWIND_DIR):
            {
                ADI_FSS_DIR_DEF *pDirDef;

                pDirDef=(ADI_FSS_DIR_DEF *)Value;
                Result=RewindDirectory(pDirDef);

                if (Result==ADI_DEV_RESULT_SUCCESS)
                {
                    pDirDef->tellpos=0;
                }
            }
            break;

        /* CASE (Open Directory) */
        case (ADI_FSD_CMD_MAKE_DIR):
            Result=CreateDirectory(
                pVolume,
                (ADI_FSS_DIR_DEF *)Value);
            break;

        /* CASE (Assign PID Handle) */
        case (ADI_FSD_CMD_SET_PID_HANDLE):
            /* Assign the Handle */
            pVolume->PIDHandle=(ADI_DEV_DEVICE_HANDLE)Value;

            /* Determine whether PID supports backgrounded data transfer - as this will determine whether
             * this FSD will support block or arbitrary mode
            */
#ifdef __ADI_FAT_BLOCK_MODE
            adi_dev_Control(
                pVolume->PIDHandle,
                ADI_FSS_CMD_GET_BACKGRND_XFER_SUPPORT,
                (void*)&pVolume->PID_supports_background_transfer);
#else
            pVolume->PID_supports_background_transfer=FALSE;
#endif
            /* Determine the width of each data element that the PID requires */
            adi_dev_Control(
                pVolume->PIDHandle,
                ADI_FSS_CMD_GET_DATA_ELEMENT_WIDTH,
                &pVolume->DataElementWidth);
            break;

        /* CASE (Determine support for background data transfer) */
        case (ADI_FSS_CMD_GET_BACKGRND_XFER_SUPPORT):
            *(u32*)Value=pVolume->PID_supports_background_transfer;
            break;

        /* CASE (Get device driver parameters) */
        case (ADI_FSD_CMD_GET_LABEL):
            /* Assign Volume Label */
            *(ADI_FSS_WCHAR**)Value=pVolume->VolumeLabel;
            break;

        /* CASE (Get cluster size) */
        case (ADI_FSD_CMD_GET_BLOCK_SIZE):
            *(u32*)Value=pVolume->ClusterSize*pVolume->SectorSize;
            break;

        /* CASE (Get maximum number of contiguous clusters that can be transfered) */
        case (ADI_FSD_CMD_GET_MAX_CONTIG_BLOCKS):
            *(u32*)Value = pVolume->MaxTransferCount / (pVolume->ClusterSize * pVolume->SectorSize);
            break;

        /* CASE (Get status information) */
        case (ADI_FSD_CMD_STAT):
            Result=StatFile(
                pVolume,
                (ADI_FSS_FILE_DESCRIPTOR *)Value);
            break;

        /* CASE (Remove a file) */
        case (ADI_FSD_CMD_REMOVE):
            Result=RemoveFile(
                pVolume,
                (ADI_FSS_FULL_FNAME *)Value,
                FILE_TYPE_FILE);
            break;

        /* CASE (Remove a file) */
        case (ADI_FSD_CMD_REMOVE_DIR):
            Result=RemoveFile(
                pVolume,
                (ADI_FSS_FULL_FNAME *)Value,
                FILE_TYPE_SUBDIR);
            break;

        /* CASE (Rename a file) */
        case (ADI_FSD_CMD_RENAME):
            Result=RenameFile(
                pVolume,
                (ADI_FSS_RENAME_DEF *)Value);
            break;

        case ADI_FAT_CMD_SET_CACHE_PERFORMANCE:
            switch ((u32)Value)
            {
                case ADI_FAT_CACHE_PERFORMANCE_FASTEST:
                case ADI_FAT_CACHE_PERFORMANCE_FAST:
                case ADI_FAT_CACHE_PERFORMANCE_RELIABLE:
                    pVolume->Performance=(u32)Value;
                    break;

                default:
                    Result=ADI_DEV_RESULT_FAILED;
            }
            break;

        case ADI_FAT_CMD_SET_FAT_CACHE_SIZE:
            pVolume->FatCacheSize=(u32)Value;
            break;

        case ADI_FAT_CMD_SET_DIR_CACHE_SIZE:
            pVolume->DirCacheSize=(u32)Value;
            break;

        case ADI_FAT_CMD_ENABLE_ACCESS_DATE:
            pVolume->AccessDateFlag=(u32)Value;
            break;

        /* CASE (Acquire Lock Semaphore for exclusive access) */
        case (ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE):
            adi_sem_Pend(
                pVolume->LockSemaphoreHandle,
                ADI_SEM_TIMEOUT_FOREVER);
            break;

        /* CASE (Release Lock Semaphore for exclusive access) */
        case (ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE):
            adi_sem_Post(pVolume->LockSemaphoreHandle);
            break;

        /* CASE (Get data element width) */
        case (ADI_FSS_CMD_GET_DATA_ELEMENT_WIDTH):
            *(u32*)Value=pVolume->DataElementWidth;
            break;

#if defined(ADI_USE_FAT_FORMAT)
        /* CASE (Format a volume) */
        case (ADI_FSD_CMD_FORMAT_VOLUME):
            Result = Format(pVolume,(ADI_FSS_FORMAT_DEF*)Value);
            break;
#endif
        /* CASE (hidden and system files are viewable) */
        case (ADI_FAT_CMD_SHOW_HIDDEN_SYSTEM):
            pVolume->bShowHiddenSystem = (u32)Value;
            break;
        /* CASE (Set the free cluster count) */
        case (ADI_FAT_CMD_SET_FREE_CLUSTER_COUNT):
            pVolume->nFreeClusterCount = (u32)Value;
            if (pVolume->bCollectUsageStats && pVolume->Type==ADI_FAT_TYPE_FAT32)
            {
                WriteFSInfoSector(pVolume, 1);
            }
            break;
            
        /* CASE (Set the free cluster count) */
        case (ADI_FAT_CMD_ENABLE_USAGE_STATS):
            pVolume->bCollectUsageStats = (u32)Value;
            if (pVolume->bCollectUsageStats && pVolume->Type==ADI_FAT_TYPE_FAT32)
            {
                ReadFSInfoSector(pVolume);
            }
            break;
            
        /* CASE (Set the free cluster count) */
        case (ADI_FSD_CMD_GET_VOLUME_USAGE):
            if (pVolume->bCollectUsageStats) 
            {
                ADI_FSS_VOLUME_USAGE_DEF *pGeometryDef = (ADI_FSS_VOLUME_USAGE_DEF *)Value;
                pGeometryDef->nSectorSize = pVolume->SectorSize;
                pGeometryDef->nClusterSize = pVolume->ClusterSize;
                pGeometryDef->nVolumeSize = pVolume->VolumeSize;
                if (!pVolume->nFreeClusterCount && !pVolume->bMediaFull) {
                    CountFreeClusters( pVolume );
                }
                pGeometryDef->nTotalClusters = pVolume->Clusters-2;
                if (pVolume->bMediaFull) {
                    pGeometryDef->nFreeClusters = 0;
                }
                else if (pVolume->nFreeClusterCount <= pVolume->Clusters) {
                    pGeometryDef->nFreeClusters = pVolume->nFreeClusterCount;
                } else {
                    pGeometryDef->nFreeClusters = pVolume->Clusters;
                    WriteFSInfoSector(pVolume, 1);
                }
            }
            else {
                Result = ADI_DEV_RESULT_NOT_SUPPORTED;
            }
            break;
            
        case ADI_FSS_CMD_SET_DATA_SEMAPHORE_TIMEOUT:
            pVolume->SemTimeOutArg = (u32)Value;
            break;

            

    }
    /* ENDCASE */

    return(Result);
}









/* ************************************************
 * ************************************************
 *          DRIVER HOUSEKEEPING FUNCTIONS
 * ************************************************
 * ************************************************
 */


/********************************************************************

    Function:       MountVolume

    Description:    Mounts a FAT volume,

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 MountVolume(
    ADI_FAT_VOLUME *pVolume,
    u32 DeviceNumber,
    u32 StartSector,
    u32 SectorSize)
{
    u32 Result,DirFlags,FatFlags,FatSectors;
    ADI_FSD_CACHE_HANDLE CacheHandle;


    /* If the device appears to be mounted unmount it */
    if (pVolume->Type!=ADI_FAT_TYPE_NONE)
    {
        UnmountVolume(pVolume);
    }

    pVolume->DeviceNumber=DeviceNumber;
    pVolume->SectorSize=SectorSize;


    /* Read Bios Parameter Block (BPB) */
    /* Create a temporary cache to read the boot sector into */
    Result=adi_fsd_cache_Open(
        pVolume->PIDHandle,
        pVolume->DeviceNumber,
        &CacheHandle,
        0,
        1,
        1,
        pVolume->SectorSize,
        pVolume->DataElementWidth,
        (u32)PDD_DEFAULT_HEAP,
        pVolume->CacheHeapID,
        0,
        0,
        pVolume->DataSemaphoreHandle);

    /* Process the Boot Sector */
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        u8 *pData;

        /* Read in the boot sector */
        Result=adi_fsd_cache_Read(
            CacheHandle,
            StartSector,
            1,
            (u8**)&pData);

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            Result = ReadBootSector(
                pVolume,
                StartSector,
                pData);
        }

        adi_fsd_cache_Release(
            CacheHandle,
            pData);
    }

    /* Close the cache, it should not be modified so don't flush */
    adi_fsd_cache_Close(CacheHandle,TRUE);



    DirFlags=0;
    FatFlags=0;

    /* This value is used for FAT12 where the fat entry crosses a sector
        boundary, to avoid this problem we read 2 FAT sectors at a time so
        that the FAT sectors contain whole entries. This is not a problem for
        FAT16 and FAT32
    */
    FatSectors=pVolume->Type==ADI_FAT_TYPE_FAT12?2:1;

    switch (pVolume->Performance)
    {
        case ADI_FAT_CACHE_PERFORMANCE_FASTEST:
            break;

        case ADI_FAT_CACHE_PERFORMANCE_FAST:
            break;

        default:
            DirFlags|=ADI_FSD_CACHE_FLUSH_ON_WRITE;
            FatFlags|=ADI_FSD_CACHE_FLUSH_ON_WRITE;
    }

    /* Initialize Cache for File Allocation Table */
    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result=adi_fsd_cache_Open(
            pVolume->PIDHandle,
            pVolume->DeviceNumber,
            &pVolume->FatCacheHandle,
            FatFlags,
            (pVolume->FatSize<pVolume->FatCacheSize?pVolume->FatSize:pVolume->FatCacheSize)/FatSectors,
            FatSectors,
            pVolume->SectorSize,
            pVolume->DataElementWidth,
            (u32)PDD_DEFAULT_HEAP,
            pVolume->CacheHeapID,
            0,
            0,
            pVolume->DataSemaphoreHandle);
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result=adi_fsd_cache_CreateHash(
            pVolume->FatCacheHandle,
            ((pVolume->FatSize<pVolume->FatCacheSize?pVolume->FatSize:pVolume->FatCacheSize)/FatSectors)/16);
    }

    /* Initialize Cache for Directory Clusters */
    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        Result=adi_fsd_cache_Open(
            pVolume->PIDHandle,
            pVolume->DeviceNumber,
            &pVolume->DirCacheHandle,
            DirFlags,
            pVolume->DirCacheSize,
            pVolume->ClusterSize,
            pVolume->SectorSize,
            pVolume->DataElementWidth,
            (u32)PDD_DEFAULT_HEAP,
            pVolume->CacheHeapID,
            0,
            0,
            pVolume->DataSemaphoreHandle);
    }

    /* Now locate volume label entry in root directory */
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        u32 Cluster,Offset;
        ADI_FAT_DIR_ENTRY *pEntry;

        Cluster=ADI_FAT_START_OF_FILE;
        Offset=0;
        pEntry=NULL;

        do
        {
            /* Read the directory entry */
            Result=ReadDirEntry(
                pVolume,
                ADI_FAT_START_OF_FILE,
                &Cluster,
                &Offset,
                &pEntry);

            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                if ((pEntry->Attr&0x0F)==0x08)
                {
                    u32 i;

                    for (i=0;
                        i<(sizeof(pVolume->VolumeLabel)/sizeof(pVolume->VolumeLabel[0]))-1 &&
                        i<sizeof(pEntry->Name);
                        i++)
                    {
                        pVolume->VolumeLabel[i]=pEntry->Name[i];
                    }
                    for (pVolume->VolumeLabel[i]=' ';pVolume->VolumeLabel[i]==' ';i--)
                    {
                        pVolume->VolumeLabel[i]='\0';
                    }
                }
                else if (pEntry->Name[0]==0)
                {
                    break;
                }

                /* Read the next directory entry */
                Result=ReadNextDirEntry(
                    pVolume,
                    &Cluster,
                    &Offset,
                    &pEntry,
                    ADI_FAT_CLUSTER_TYPE_READ);
            }
        } while (Result==ADI_DEV_RESULT_SUCCESS);

        /* If we've still got a directory chuck, release it */
        Result=ReleaseDirEntry(
            pVolume,
            &pEntry);
    }


    return Result;
}


/********************************************************************

    Function:       UnmountVolume

    Description:    Unmounts a FAT volume,

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 UnmountVolume(ADI_FAT_VOLUME *pVolume)
{
    /* Close caches, flush them on close */
    adi_fsd_cache_Close(pVolume->FatCacheHandle,TRUE);
    adi_fsd_cache_Close(pVolume->DirCacheHandle,TRUE);
    pVolume->FatCacheHandle=NULL;
    pVolume->DirCacheHandle=NULL;

    /* Clear mounted flag */
    pVolume->Type = ADI_FAT_TYPE_NONE;

    return ADI_DEV_RESULT_SUCCESS;
}





/* ************************************************
 * ************************************************
 *          BOOT SECTOR OPERATIONS
 * ************************************************
 * ************************************************
 */


/********************************************************************

    Function:       ReadBootSector

    Description:    Read the Boot Sector and BIOS Parameter Block
                    from the media and configure driver accordingly

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 ReadBootSector(
    ADI_FAT_VOLUME  *pVolume,
    u32             StartSector,
    u8              *pData)
{
    u32 Result;
    u32 RootSectors;
    ADI_FAT_BOOT_SECTOR_START *pStart;
    ADI_FAT_BOOT_SECTOR_MIDDLE *pMiddle;


    Result=ADI_DEV_RESULT_SUCCESS;

    pVolume->Type=ADI_FAT_TYPE_NONE;

    pStart=(ADI_FAT_BOOT_SECTOR_START *)pData;
    pMiddle=(ADI_FAT_BOOT_SECTOR_MIDDLE *)(((u8*)pStart)+sizeof(*pStart));


    /* Get the volume size in sectors */
    pVolume->VolumeSize=pStart->BPB_TotSec16;
    if (!pVolume->VolumeSize)
    {
        pVolume->VolumeSize=pStart->BPB_TotSec32;
    }

    /* Get the number of sectors for the root */
    RootSectors=(u32)((pStart->BPB_RootEntCnt*sizeof(ADI_FAT_DIR_ENTRY))+pVolume->SectorSize-1)/
        pVolume->SectorSize;

    /* Save FAT sector address */
    pVolume->FatAddress=
        StartSector+
        pStart->BPB_RsvdSecCnt;

    /* Get the fat size */
    pVolume->FatSize=pStart->BPB_FATSz16;
    if (!pVolume->FatSize)
    {
        pVolume->FatSize=pMiddle->BPB_FATSz32;
    }

    /* Store the Sectors per Cluster */
    pVolume->ClusterSize=pStart->BPB_SecPerClus;

    /* Save Data sector address */
    pVolume->DataAddress=
        pVolume->FatAddress+
        (pStart->BPB_NumFATs*pVolume->FatSize)+
        RootSectors;

    /* Calculate the data size, adjust value fall on a cluster
        boundary
    */
    pVolume->DataSize=pVolume->VolumeSize;
    pVolume->DataSize-=pStart->BPB_RsvdSecCnt;
    pVolume->DataSize-=(pStart->BPB_NumFATs*pVolume->FatSize)+RootSectors;

    pVolume->Clusters=pVolume->DataSize/pVolume->ClusterSize;

    /* The FAT type can be worked out from the cluster count */
    if (pVolume->DataSize==0)
    {
        Result=ADI_DEV_RESULT_FAILED;
    }
    else if (pVolume->Clusters<=4078)//85)
    {
        pVolume->Type=ADI_FAT_TYPE_FAT12;
    }
    else if (pVolume->Clusters<=65518)//25)
    {
        pVolume->Type=ADI_FAT_TYPE_FAT16;
    }
    else if (pVolume->Clusters<=268435438)
    {
        pVolume->Type=ADI_FAT_TYPE_FAT32;
    }
    else
    {
        Result=ADI_DEV_RESULT_FAILED;
    }

    /* Save FSINFO sector address */
    if (pVolume->Type == ADI_FAT_TYPE_FAT32)
    {
        pVolume->FSInfoAddress = StartSector + pMiddle->BPB_FSInfo;
    }
    
    /* We have determined the FAT type, so continue decoding Boot Sector */
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        u32 Index;
        ADI_FAT_BOOT_SECTOR_END *pEnd;


        pEnd=(ADI_FAT_BOOT_SECTOR_END *)(((u8*)pStart)+sizeof(*pStart)+
            (pVolume->Type==ADI_FAT_TYPE_FAT32?sizeof(ADI_FAT_BOOT_SECTOR_MIDDLE):0));

        /* Initialise some FAT type specific variables */
        switch (pVolume->Type)
        {
            case ADI_FAT_TYPE_FAT12:
                pVolume->ClusterMask        = 0x00000FFF;
                pVolume->RootDirAddress     = pVolume->DataAddress-RootSectors;
                break;

            case ADI_FAT_TYPE_FAT16:
                pVolume->ClusterMask        = 0x0000FFFF;
                pVolume->RootDirAddress     = pVolume->DataAddress-RootSectors;
                break;

            default:
                pVolume->ClusterMask        = 0x0FFFFFFF;
                pVolume->RootDirAddress     = pMiddle->BPB_RootClus;
        }

        pVolume->FreeCluster=ADI_FAT_FIRST_CLUSTER;

        ResetDirectory(pVolume);

        /* Set the Volume Label */
        memset(
            pVolume->VolumeLabel,' ',sizeof(pVolume->VolumeLabel));
        for (Index=0;
            Index<(sizeof(pVolume->VolumeLabel)/sizeof(pVolume->VolumeLabel[0]))-1;
            Index++)
        {
            pVolume->VolumeLabel[Index]=pEnd->BS_VolLab[Index];
        }
        for (;Index!=0xFFFFFFFF && pVolume->VolumeLabel[Index]==' ';Index--)
        {
            pVolume->VolumeLabel[Index]=0;
        }

        /* Set the Volume Type */
        memset(pVolume->VolumeType,' ',sizeof(pVolume->VolumeType));
        for (Index=0;
            Index<(sizeof(pVolume->VolumeType)/sizeof(pVolume->VolumeType[0]))-1;
            Index++)
        {
            pVolume->VolumeType[Index]=pEnd->BS_FilSysType[Index];
        }
        for (;Index!=0xFFFFFFFF && pVolume->VolumeType[Index]==' ';Index--)
        {
            pVolume->VolumeType[Index]=0;
        }
    }


    if (Result!=ADI_DEV_RESULT_SUCCESS)
    {
        pVolume->Type=ADI_FAT_TYPE_NONE;
    }

    return Result;
}

/********************************************************************
    FSInfo sector structure definition
********************************************************************/
typedef struct {
    u32 FSI_LeadSig;
    u32 FSI_Reserved1[120];
    u32 FSI_Struct_Sig;
    u32 FSI_Free_Count;
    u32 FSI_NxtFree;
    u32 FSI_Reserved2[3];
    u32 FSI_TrailSig;
} FSInfo_Def;


/********************************************************************
    Macro to determine a valid FSInfo structure
********************************************************************/
#define _IS_VALID_FSINFO(P) \
        ( ((P)->FSI_LeadSig == 0x41615252) && ((P)->FSI_Struct_Sig == 0x61417272) && ((P)->FSI_TrailSig == 0xAA550000) )
    
/********************************************************************

    Function:       ReadFSInfoSector

    Description:    Read FSInfo sector and assign volume instance 
                    values if appropriate

********************************************************************/
static u32 ReadFSInfoSector(
    ADI_FAT_VOLUME          *pVolume
)
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    FSInfo_Def *pFSInfo;
    
    /* Return if FSInfoAddress is not set. 
    */
    if (!pVolume->FSInfoAddress)
    {
        Result = ADI_DEV_RESULT_FAILED;
    }
    
    /* Read the FSInfo sector from the Cache. Will retrieve from 
     * media if not already in cache
    */
    if (Result == ADI_DEV_RESULT_SUCCESS) {
        Result = adi_fsd_cache_Read(
                                    pVolume->FatCacheHandle,
                                    pVolume->FSInfoAddress,
                                    1,
                                    (u8**)&pFSInfo
                                   );
    }
    
    if ((Result == ADI_DEV_RESULT_SUCCESS) && pFSInfo && _IS_VALID_FSINFO(pFSInfo)) 
    {
        if (pFSInfo->FSI_NxtFree != 0xFFFFFFFF) 
        {
            pVolume->FreeCluster = pFSInfo->FSI_NxtFree;
        }
        if (pFSInfo->FSI_Free_Count != 0xFFFFFFFF) 
        {
            if (pFSInfo->FSI_Free_Count <= pVolume->Clusters) 
            {
                pVolume->nFreeClusterCount = pFSInfo->FSI_Free_Count;
            }
            else {
                /* the FSI_Free_Count value is inaccurate, adjust to
                 * the total clusters of the volume
                */
                pVolume->nFreeClusterCount = pVolume->Clusters;
                WriteFSInfoSector(pVolume, 1);
            }
        }
    }
    else {
        Result = ADI_FSS_RESULT_NOT_FOUND;
    }
    
    return Result;
}

/********************************************************************

    Function:       WriteFSInfoSector

    Description:    Modify the FSInfo sector and write back to cache.
                    If it fails we don't care.

********************************************************************/
static void WriteFSInfoSector(
    ADI_FAT_VOLUME          *pVolume,
    bool                    bWriteBack
)
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    FSInfo_Def *pFSInfo;
    
    if (pVolume->bCollectUsageStats) 
    {
        /* Return if FSInfoAddress is not set. 
        */
        if (!pVolume->FSInfoAddress)
        {
            Result = ADI_DEV_RESULT_FAILED;
        }
    
        /* Read the FSInfo sector from the Cache. Will retrieve from 
         * media if not already in cache
        */
        if (Result == ADI_DEV_RESULT_SUCCESS) {
            Result = adi_fsd_cache_Read(
                                        pVolume->FatCacheHandle,
                                        pVolume->FSInfoAddress,
                                        1,
                                        (u8**)&pFSInfo
                                       );
        }
    
        /* Set the values accordingly */
        if ((Result == ADI_DEV_RESULT_SUCCESS) && pFSInfo && _IS_VALID_FSINFO(pFSInfo)) 
        {
            pFSInfo->FSI_NxtFree    = pVolume->FreeCluster;
            pFSInfo->FSI_Free_Count = pVolume->nFreeClusterCount;
        }
        else {
            Result = ADI_FSS_RESULT_NOT_FOUND;
        }
    
        /* Unlock the cache sector so it is flushed as per the 
         * definition of the cache
        */
        if (Result == ADI_DEV_RESULT_SUCCESS) {
            Result = adi_fsd_cache_Write(
                                        pVolume->FatCacheHandle,
                                        (u8*)pFSInfo
                                        );
        }
        /* Overwrite the cache default to immediately 
         * write sector to media if required.
        */     
        if ((Result == ADI_DEV_RESULT_SUCCESS) && bWriteBack) {
            Result = adi_fsd_cache_FlushBlock(
                                        pVolume->FatCacheHandle,
                                        pVolume->FSInfoAddress,
                                        1
                                        );
        }
    
        /* We want to retain the FSInfo sector in the cache so we allocate 
         * the sector again to lock it against being removed 
        */
        if (Result == ADI_DEV_RESULT_SUCCESS) {
            Result = adi_fsd_cache_Allocate(
                                        pVolume->FatCacheHandle,
                                        pVolume->FSInfoAddress,
                                        1,
                                        (u8**)&pFSInfo
                                       );
        }
    }
}

/********************************************************************

    Function:       CountFreeClusters

    Description:    Do some basic checks on the FAT table, count the
                    number of clusters and how many are free, Check
                    number of last cluster in chain. Do check on
                    cluster chains

********************************************************************/
__ADI_FAT_SECTION_CODE
void CountFreeClusters(
    ADI_FAT_VOLUME          *pVolume)
{
    u32 Result,Cluster,Value,Free,Reserved,Bad,Last,Used,FatError;

    if (pVolume->bCollectUsageStats) 
    {
        Free=0;
        Reserved=ADI_FAT_FIRST_CLUSTER;
        Bad=0;
        Last=0;
        Used=0;

        FatError=0;

        for (Cluster=ADI_FAT_FIRST_CLUSTER;
            Cluster<pVolume->Clusters;
            Cluster++)
        {
            Result=GetFatEntry(pVolume,Cluster,&Value);

            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                switch (Value)
                {
                    case ADI_FAT_FREE_CLUSTER:      Free++; break;
                    case ADI_FAT_RESERVED_CLUSTER:  Reserved++; break;
                    case ADI_FAT_BAD_CLUSTER:       Bad++; break;
                    case ADI_FAT_LAST_CLUSTER:      Last++; break;
                    default:                        Used++; break;
                }
            }
        }

        pVolume->nFreeClusterCount = Free;
        if (pVolume->nFreeClusterCount == 0)
        {
            /* Set Media Full flag */
            pVolume->bMediaFull = true;
            /* set Free cluster value to zero to indicate no 
             * clusters are available 
            */
            pVolume->FreeCluster = 0;
        }   
            
        WriteFSInfoSector(pVolume, true);
    }
    else {
            pVolume->nFreeClusterCount = pVolume->Clusters;
    }
    
}




/* ************************************************
 * ************************************************
 *               FILE OPERATIONS
 * ************************************************
 * ************************************************
 */


/********************************************************************

    Function:       OpenFile

    Description:    Opens a File.

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 OpenFile(
    ADI_FAT_VOLUME *pVolume,
    ADI_FSS_FILE_DESCRIPTOR *pFileDef,
    u32 FileType)
{
    u32 Result,FileSize;
    u8 Attribute;
    ADI_FAT_FILE_DESCRIPTOR *pFileDesc;
    bool FileExists = true;


    Result=ADI_DEV_RESULT_SUCCESS;
    pFileDesc=0;
    FileSize=0;

    /* Read-Only filesystem will fail trying to open a file/dir for write */
    if (pVolume->ReadOnly &&
        ((pFileDef->mode&ADI_FSS_MODE_WRITE)==ADI_FSS_MODE_WRITE ||
            (pFileDef->mode&ADI_FSS_MODE_READ_WRITE)==ADI_FSS_MODE_READ_WRITE))
    {
        Result=ADI_DEV_RESULT_FAILED;
    }

    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        /* Create new FAT file descriptor */
        pFileDesc=(ADI_FAT_FILE_DESCRIPTOR *)_adi_fss_malloc(
            PDD_DEFAULT_HEAP,
            sizeof(ADI_FAT_FILE_DESCRIPTOR));

        if (pFileDesc==0)
        {
            Result=ADI_FSS_RESULT_NO_MEMORY;
        }
    }

    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        /* Find the parent directory */
        Result=SeekDirEntry(
            pVolume,
            pFileDef->pFullFileName,
            ADI_FAT_SEEK_TYPE_PARENT,
            &pFileDesc->Entry.Cluster,
            &pFileDesc->Entry.Offset,
            &Attribute,
            &pFileDesc->ParentCluster,
            NULL);
    }

    /* Try to open or create the file */
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        u32 i;
        ADI_FSS_WCHAR FileName[PDD_FAT_FILENAME_SIZE];
        ADI_FSS_FULL_FNAME *pName;

        /* Extract the filename */
        pName=pFileDef->pFullFileName;
        while (pName->pNext)
        {
            pName=pName->pNext;
        }

        for (i=0;i<pName->namelen && i<PDD_FAT_FILENAME_SIZE-1 ;i++)
        {
            FileName[i]=pName->name[i];
        }
        if (i==0)
        {
            FileName[i++]='.';
        }
        FileName[i]='\0';

        /* Try to find the FileName in the Parent Directory */
        if (pFileDesc->ParentCluster==ADI_FAT_START_OF_FILE &&
            FileName[0]=='.' && FileName[1]=='\0')
        {
            pFileDesc->FirstCluster=ADI_FAT_ROOT_CLUSTER;
        }
        else
        {
            Result=FindDirEntry(
                pVolume,
                pFileDesc->ParentCluster,
                FileName,
                &pFileDesc->Entry.Cluster,
                &pFileDesc->Entry.Offset,
                &Attribute,
                &pFileDesc->FirstCluster,
                &FileSize);
                
            if(Result==ADI_DEV_RESULT_SUCCESS) {
                FileExists = true;
            }
        }

        /* On FAIL try to create a new file/dir entry in the parent
            directory
        */
        if (Result!=ADI_DEV_RESULT_SUCCESS && pVolume->ReadOnly==FALSE &&
            (pFileDef->mode&ADI_FSS_MODE_CREATE)==ADI_FSS_MODE_CREATE)
        {
            if ( pVolume->bMediaFull )
            {
                Result = ADI_FSS_RESULT_MEDIA_FULL;
            }
            else {
                /* Create the new file/directory */
                if(FileType == FILE_TYPE_SUBDIR) {
                    
                    Attribute = ADI_FAT_DIR_SUBDIRECTORY;
                } else {
                    Attribute = 0;
                }
                Result=AddDirEntry(
                    pVolume,
                    pFileDesc->ParentCluster,
                    FileName,
                    Attribute,
                    ADI_FAT_LAST_CLUSTER,
                    &pFileDesc->Entry.Cluster,
                    &pFileDesc->Entry.Offset);
                pFileDesc->FirstCluster=ADI_FAT_LAST_CLUSTER;
                FileSize=0;
            }
        }
        else if (Result==ADI_DEV_RESULT_SUCCESS && pVolume->ReadOnly==FALSE &&
            (pFileDef->mode&ADI_FSS_MODE_APPEND)==ADI_FSS_MODE_APPEND && pVolume->bMediaFull )
        {
            Result = ADI_FSS_RESULT_MEDIA_FULL;
        }
        /* Check that the file type of the entry matches the type we are trying
            to open, if it doesn't match then close the file
        */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            /* 
            
            An ordinary file is identified as not a sub-dir or device
            
            IF ((File Exists and its Attribute is either SUBDIRECTORY or DEVICE) AND (the requested type is not a DIR))
            OR ((File Exists and its Attribute is neither SUBDIRECTORY nor DEVICE) AND (the requested type is not a FILE))
            OR ((File Exists and its Attribute is READONLY) AND (File open request is for WRITE)) 
            THEN 
                FAIL
            */
            if ( FileExists && (
                  ((IS_ATTRIBUTE(Attribute,ADI_FAT_DIR_SUBDIRECTORY) || IS_ATTRIBUTE(Attribute,ADI_FAT_DIR_DEVICE)) && FileType != FILE_TYPE_SUBDIR)
               || (!IS_ATTRIBUTE(Attribute,ADI_FAT_DIR_SUBDIRECTORY) && !IS_ATTRIBUTE(Attribute,ADI_FAT_DIR_DEVICE) && FileType != FILE_TYPE_FILE)
               || (IsWriteAccess(pFileDef) == ADI_DEV_RESULT_SUCCESS && IS_ATTRIBUTE(Attribute,ADI_FAT_DIR_READONLY))
                               )
            )
            {
                Result=ADI_FSS_RESULT_FAILED;
            }
        }
    }

        /* If the file exists, and is to opened for append and the media is full 
             * then report error
            */
        if (Result==ADI_FSS_RESULT_SUCCESS && IsWriteAccess(pFileDef)==ADI_DEV_RESULT_SUCCESS)
        {
            if ( ((pFileDef->mode & ADI_FSS_MODE_APPEND) == ADI_FSS_MODE_APPEND)
                 && pVolume->bMediaFull )
            {
                Result=ADI_FSS_RESULT_MEDIA_FULL;
            }
        }

            else if (Result!=ADI_FSS_RESULT_SUCCESS && IsWriteAccess(pFileDef)==ADI_DEV_RESULT_SUCCESS)
            {
                /* If the file doesn't exist, and is to opened for writing and the media is full 
                 * then report error
                */
            }
    
    /* If we got the file entry ok, update it */
    if (Result==ADI_FSS_RESULT_SUCCESS &&
        pFileDesc->Entry.Cluster!=ADI_FAT_START_OF_FILE)
    {
        struct tm Now;
        time_t lt;
        ADI_FAT_DIR_ENTRY *pEntry;
        u16 WriteTime,WriteDate;
        u32 Cluster;


        pEntry=NULL;
        adi_rtc_GetDateTime(&Now);
        Now.tm_isdst=FALSE;
        lt=mktime(&Now);

        /* Get the directory file entry */
        Result=ReadDirEntry(
            pVolume,
            ADI_FAT_START_OF_FILE,
            &pFileDesc->Entry.Cluster,
            &pFileDesc->Entry.Offset,
            &pEntry);

        if (Result==ADI_FSS_RESULT_SUCCESS)
        {
            /* Truncate entry if required */
            if (!IS_ATTRIBUTE(Attribute,ADI_FAT_DIR_DEVICE) &&
                !IS_ATTRIBUTE(Attribute,ADI_FAT_DIR_SUBDIRECTORY) &&
                (pFileDef->mode&ADI_FSS_MODE_TRUNCATE)==ADI_FSS_MODE_TRUNCATE)
            {
                WriteTime       = FAT_TIME(Now);
                WriteDate       = FAT_DATE(Now);
                Cluster         = 0;

                /* Clear cluster chain & set file size to zero */
                Result=RemoveFatEntry(
                    pVolume,
                    pFileDesc->FirstCluster);

                pFileDesc->FirstCluster=ADI_FAT_LAST_CLUSTER;
                FileSize=0;
            }
            else
            {
                WriteTime       = pEntry->WrtTime&0xFFFE;
                WriteDate       = pEntry->WrtDate;
                Cluster         = GetClusterDirEntry(pVolume,pEntry);
            }
        }

        /* If anything in the file entry has changed then re-write the entry,
            otherwise don't do anything */
        if (Result==ADI_FSS_RESULT_SUCCESS &&
            ((pVolume->ReadOnly==FALSE && pVolume->AccessDateFlag==TRUE && pEntry->LastAccDate!=FAT_DATE(Now)) ||
                (pEntry->WrtTime&0xFFFE)!=WriteTime || pEntry->WrtDate!=WriteDate ||
                GetClusterDirEntry(pVolume,pEntry)!=Cluster) ||
                pEntry->FileSize!=FileSize)
        {
            pEntry->LastAccDate = FAT_DATE(Now);

            /* Update File Size to blank */
            pEntry->FileSize    = FileSize;

            /* Update File Date/Time Data */
            pEntry->WrtTime     = WriteTime;
            pEntry->WrtDate     = WriteDate;

            /* Update the first cluster to point at nothing */
            SetClusterDirEntry(pVolume,pEntry,Cluster);

            /* And write the cluster back to the media */
            Result=WriteDirEntry(
                pVolume,
                &pEntry);
        }
        else
        {
            ReleaseDirEntry(
                pVolume,
                &pEntry);
        }
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* Reset FSS file descriptor variables */
        pFileDef->curpos=0;
        pFileDef->fsize=FileSize;


        /* Reset FAT file descriptor variables */
        pFileDesc->Position=0;
        pFileDesc->SeekPosition=0;
        pFileDesc->File.Cluster=ADI_FAT_START_OF_FILE;
        pFileDesc->File.Offset=0;

        pFileDesc->NextCluster=ADI_FAT_START_OF_FILE;

        /* Assign FAT File Descriptor handle for requester */
        pFileDef->FSD_data_handle=(ADI_FSS_FSD_DATA_HANDLE)pFileDesc;
    }
    else if (pFileDesc)
    {
        /* Otherwise if we there was and error but the FAT file descriptor
            had been allocated memory clear it
        */
        _adi_fss_free(PDD_DEFAULT_HEAP,pFileDesc);
    }

    return Result;
}


/********************************************************************

    Function:       CloseFile

    Description:    Closes a File.

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 CloseFile(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_FILE_DESCRIPTOR *pFileDef,
    u32                     FileType
)
{
    u32 Result;
    ADI_FAT_FILE_DESCRIPTOR *pFileDesc;
    u32 ClusterSize;
    u32 bClusterBoundary = false;

    Result=ADI_DEV_RESULT_SUCCESS;

    pFileDesc=(ADI_FAT_FILE_DESCRIPTOR *)pFileDef->FSD_data_handle;

    if (pFileDesc)
    {
        struct tm Now;
        time_t lt;

        adi_rtc_GetDateTime(&Now);
        Now.tm_isdst=FALSE;
        lt=mktime(&Now);

        /* Do a file seek to make sure if we have seeked past the end of the
           file but not witten or read anything that the extra clusters are
           added to the end of the file. We only need to do this if the seek
           position is greater than the current file position and it is open
           for writing.
           
           However, if we are at a cluster boundary, we do not need to do so.
        */
        ClusterSize = pVolume->ClusterSize*pVolume->SectorSize;
        if ( (0 == (pFileDesc->SeekPosition%ClusterSize)) && (pFileDesc->SeekPosition > 0) )
        {
            bClusterBoundary = true;
        }

        if (!bClusterBoundary && pFileDesc->SeekPosition>pFileDesc->Position &&
            IsWriteAccess(pFileDef)==ADI_DEV_RESULT_SUCCESS)
        {
            Result=Seek(
                pVolume,
                1,
                pFileDef);
        }

        /* Update the access date and filesize only if the filesize has
            changed or the accessed date has changed and
            ADI_FAT_ENABLE_ACCESS_DATE is set to TRUE */
        if (Result==ADI_DEV_RESULT_SUCCESS && pVolume->ReadOnly==FALSE)
        {
            ADI_FAT_DIR_ENTRY *pEntry;


            pEntry=NULL;

            Result=ReadDirEntry(
                pVolume,
                pFileDesc->Entry.Cluster,
                &pFileDesc->Entry.Cluster,
                &pFileDesc->Entry.Offset,
                &pEntry);

            if (Result==ADI_DEV_RESULT_SUCCESS  &&
                    (IsFileEntry(pEntry->Attr) && FileType==FILE_TYPE_FILE) &&
                ((pVolume->AccessDateFlag==TRUE  && pEntry->LastAccDate!=FAT_DATE(Now) )  ||
                    pEntry->FileSize!=pFileDef->fsize))
            {
                /* Update File Size */
                pEntry->FileSize=pFileDef->fsize;

                /* Update File Date/Time Data */
                pEntry->LastAccDate = FAT_DATE(Now);

                /* And write the cluster back to the media */
                Result=WriteDirEntry(
                    pVolume,
                    &pEntry);
            }
            else
            {
                ReleaseDirEntry(
                    pVolume,
                    &pEntry);
            }
        }

        /* If set to FAST performance setting the directory cache has to be
            flushed for the file entry. For now we just flush the entire FAT
            and directory cache.

            XXX: Look into updating so se only flush the directory/FAT
                clusters that are related to this file. It could be too much
                more work, and cost too much processor time to do effectivily
                anyway
        */
        if ((Result == ADI_DEV_RESULT_SUCCESS || Result == ADI_FSS_RESULT_MEDIA_FULL) &&
            pVolume->Performance==ADI_FAT_CACHE_PERFORMANCE_FAST)
        {
            adi_fsd_cache_Flush(pVolume->FatCacheHandle);
            adi_fsd_cache_Flush(pVolume->DirCacheHandle);
            Result = ADI_DEV_RESULT_SUCCESS;
        }
        /* Free the file descriptor */
        _adi_fss_free(PDD_DEFAULT_HEAP,pFileDesc);
    }

    pFileDef->FSD_data_handle=NULL;

    return Result;
}


/********************************************************************

    Function:       RemoveFile

    Description:    Removes a file from the volume.

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 RemoveFile(
    ADI_FAT_VOLUME      *pVolume,
    ADI_FSS_FULL_FNAME  *pFileName,
    u32                 FileType)
{
    u32 Result,Cluster,Offset,FirstCluster;
    u8 Attribute;


    /* Assume success */
    Result=ADI_DEV_RESULT_SUCCESS;


    if (pVolume->ReadOnly)
    {
        Result=ADI_DEV_RESULT_FAILED;
    }

    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        /* locate entry */
        Result=SeekDirEntry(
            pVolume,
            pFileName,
            ADI_FAT_SEEK_TYPE_CHILD,
            &Cluster,
            &Offset,
            &Attribute,
            &FirstCluster,
            NULL);
    }
    if (Result==ADI_DEV_RESULT_SUCCESS && 
        (  (IsFileEntry(Attribute) && FileType != FILE_TYPE_FILE ) 
        || (IS_ATTRIBUTE(Attribute,ADI_FAT_DIR_SUBDIRECTORY) && FileType != FILE_TYPE_SUBDIR ))
    )
    {
        Result=ADI_DEV_RESULT_FAILED;
    }

    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        /* Directory Entry to be deleted, so check that it is empty.
            Note: the DirEntry.pShortEntry is not on the heap but points to a
            position within a cluster buffer the contents of which will change
            if this next block is executed
        */
        if (FileType==FILE_TYPE_SUBDIR)
        {
            u32 Count,SubDirCluster,SubDirOffset;

            SubDirCluster=ADI_FAT_START_OF_FILE;
            SubDirOffset=0;

            /* Count the directory entries */
            for (Count=0;
                Count<=2 && GetDirEntry(
                    pVolume,
                    FirstCluster,
                    &SubDirCluster,
                    &SubDirOffset,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL)==ADI_DEV_RESULT_SUCCESS;
                Count++);

            /* If more than the . and .. entries then abort as directory is
                not empty
            */
            if (Count>2)
            {
                Result=ADI_DEV_RESULT_FAILED;
            }
        }
    }

    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        /* Remove directory entry */
        Result=RemoveDirEntry(
            pVolume,
            Cluster,
            Offset);
    }

    if (Result==ADI_DEV_RESULT_SUCCESS &&
        pVolume->Performance==ADI_FAT_CACHE_PERFORMANCE_FAST)
    {
        adi_fsd_cache_Flush(pVolume->FatCacheHandle);
        adi_fsd_cache_Flush(pVolume->DirCacheHandle);
    }

    return Result;

}


/********************************************************************

    Function:       RenameFile

    Description:    Renames a file/directory within the volume,

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 RenameFile(
    ADI_FAT_VOLUME      *pVolume,
    ADI_FSS_RENAME_DEF  *pRenameDef)
{
    u32 Result,SrcCluster,SrcOffset,TgtCluster,TgtOffset;
    ADI_FAT_DIR_ENTRY Entry,*pEntry;


    Result=ADI_FSS_RESULT_FAILED;
    pEntry=NULL;

    /* If it isn't a read-only filesystem find the source */
    if (!pVolume->ReadOnly)
    {
        /* Look up Source entry */
        Result=SeekDirEntry(
            pVolume,
            pRenameDef->pSource,
            ADI_FAT_SEEK_TYPE_CHILD,
            &SrcCluster,
            &SrcOffset,
            NULL,
            NULL,
            NULL);
    }

    /* We've found the source, now check if the target exists */
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        /* Lookup the Target entry */
        Result=SeekDirEntry(
            pVolume,
            pRenameDef->pTarget,
            ADI_FAT_SEEK_TYPE_CHILD,
            &TgtCluster,
            &TgtOffset,
            NULL,
            NULL,
            NULL);

        /* It exists! */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            /* Check if the source and target entries are the same if so
                then no need to rename, we return success, otherwise both
                file exist as seperate files, it means that we are unable
                to rename so return error.
            */
            if (SrcCluster!=TgtCluster ||
                SrcOffset!=TgtOffset)
            {
                Result==ADI_DEV_RESULT_FAILED;
            }
        }

        /* Otherwise target does not exist, so now we create it, and copy the
            source first cluster info to it and remove the source file
        */
        else
        {
            u32 Cluster;
            ADI_FSS_WCHAR FileName[PDD_FAT_FILENAME_SIZE];


            /* Find Target parent directory */
            Result=SeekDirEntry(
                pVolume,
                pRenameDef->pTarget,
                ADI_FAT_SEEK_TYPE_PARENT,
                &TgtCluster,
                &TgtOffset,
                NULL,
                &Cluster,
                NULL);

            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                u32 i;
                ADI_FSS_FULL_FNAME *pName;

                /* Extract the filename from the path */
                pName=pRenameDef->pTarget;
                while (pName->pNext)
                {
                    pName=pName->pNext;
                }
                for (i=0;i<pName->namelen && i<PDD_FAT_FILENAME_SIZE-1 ;i++)
                {
                    FileName[i]=pName->name[i];
                }
                FileName[i]='\0';

                // NOTE: The validity of the name is checked by AddDirEntry()
                Result=AddDirEntry(
                    pVolume,
                    Cluster,
                    FileName,
                    0,
                    ADI_FAT_LAST_CLUSTER,
                    &TgtCluster,
                    &TgtOffset);
            }

            /* Read the Source file entry, copy it and point the first cluster
                to nothing, so when it is removed it does not remove the
                file/dir data cluster chain
            */
            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                pEntry=NULL;

                Result=ReadDirEntry(
                    pVolume,
                    ADI_FAT_LAST_CLUSTER,
                    &SrcCluster,
                    &SrcOffset,
                    &pEntry);

                if (Result==ADI_DEV_RESULT_SUCCESS)
                {
                    /* Make a copy of the entry */
                    memcpy(&Entry,pEntry,sizeof(Entry));

                    /* Reset the first cluster to point to nothing */
                    /*pEntry->FstClusLo=ADI_FAT_LAST_CLUSTER&pVolume->ClusterMask;
                    pEntry->FstClusHi=(ADI_FAT_LAST_CLUSTER&pVolume->ClusterMask)>>16;*/
                    SetClusterDirEntry(pVolume,pEntry,ADI_FAT_LAST_CLUSTER);

                    Result=WriteDirEntry(
                        pVolume,
                        &pEntry);
                }
            }

            /* Now copy the Source entry info into the new Target file entry */
            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                /* Open the new file entry, copy the relevent old entry info
                    into the new one, including making the first cluster
                    pointer point to the file/dir data
                */
                pEntry=NULL;

                Result=ReadDirEntry(
                    pVolume,
                    ADI_FAT_LAST_CLUSTER,
                    &TgtCluster,
                    &TgtOffset,
                    &pEntry);

                if (Result==ADI_DEV_RESULT_SUCCESS)
                {
                    /* Copy all the relevent old entry stuff to the new one */
                    pEntry->Attr        = Entry.Attr;
                    pEntry->NTRes       = Entry.NTRes;
                    pEntry->LastAccDate = Entry.LastAccDate;
                    pEntry->FstClusHi   = Entry.FstClusHi;
                    pEntry->FstClusLo   = Entry.FstClusLo;
                    pEntry->FileSize    = Entry.FileSize;

                    /* Write the data back */
                    Result=WriteDirEntry(
                        pVolume,
                        &pEntry);
                }
            }

            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                /* Remove old entry fully */
                Result=RemoveDirEntry(
                    pVolume,
                    SrcCluster,
                    SrcOffset);
            }

            /* On success and depending on the performance settings flush the
                FSD caches
            */
            if (Result==ADI_DEV_RESULT_SUCCESS &&
                pVolume->Performance==ADI_FAT_CACHE_PERFORMANCE_FAST)
            {
                adi_fsd_cache_Flush(pVolume->FatCacheHandle);
                adi_fsd_cache_Flush(pVolume->DirCacheHandle);
            }
        }
    }


    return Result;
}


/********************************************************************

    Function:       StatFile

    Description:    Retrieves File status information.

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 StatFile(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_FILE_DESCRIPTOR *pFileDef)
{
    u32 Result,Cluster,Offset;

    struct stat *pStat = (struct stat *)pFileDef->FSD_data_handle;


    Result=ADI_FSS_RESULT_SUCCESS;


    /* Look up name */
    Result=SeekDirEntry(
        pVolume,
        pFileDef->pFullFileName,
        ADI_FAT_SEEK_TYPE_CHILD,
        &Cluster,
        &Offset,
        NULL,
        NULL,
        NULL);

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        ADI_FAT_DIR_ENTRY *pEntry;

        pEntry=NULL;

        Result=ReadDirEntry(
            pVolume,
            ADI_FAT_LAST_CLUSTER,
            &Cluster,
            &Offset,
            &pEntry);

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            struct tm TimeModified;

            /* Assign Size of File/Directory */
            pStat->st_size=(size_t)pEntry->FileSize;

            /* Assign mode (type) of file */
            pStat->st_mode=S_IFREG;
            if (pEntry->Attr&ADI_FAT_DIR_SUBDIRECTORY)
            {
                pStat->st_mode=S_IFDIR;
            }

            /* Assign time last modified */
            TimeModified.tm_sec     = FAT_SECOND(pEntry->WrtTime);
            TimeModified.tm_min     = FAT_MINUTE(pEntry->WrtTime);
            TimeModified.tm_hour    = FAT_HOUR(pEntry->WrtTime);
            TimeModified.tm_mday    = FAT_DAY(pEntry->WrtDate);
            TimeModified.tm_mon     = FAT_MONTH(pEntry->WrtDate);
            TimeModified.tm_year    = FAT_YEAR(pEntry->WrtDate);

            pStat->st_mtime=mktime(&TimeModified);

            /* Now release the locked memory */
            Result=WriteDirEntry(
                pVolume,
                &pEntry);

            if (Result==ADI_DEV_RESULT_SUCCESS &&
                pVolume->Performance==ADI_FAT_CACHE_PERFORMANCE_FAST)
            {
                Result=adi_fsd_cache_FlushBlock(
                    pVolume->DirCacheHandle,
                    GetClusterSectorNumber(pVolume,Cluster,Offset),
                    pVolume->ClusterSize);
            }
        }
    }

    return Result;
}


/********************************************************************

    Function:    SeekFile

    Description: Seeks to given position within a file. Position can
                 either be a relative or absolute value.
                 Return value is ADI_DEV_RESULT_SUCCESS for success

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 SeekFile(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_SEEK_REQUEST    *pSeek)
{
    u32 Result;
    s32 SeekPosition;
    u8 Mode;
    ADI_FSS_FILE_DESCRIPTOR *pFSSFile;
    ADI_FAT_FILE_DESCRIPTOR *pFATFile;


    pFSSFile=(ADI_FSS_FILE_DESCRIPTOR *)pSeek->pFileDesc;
    pFATFile=(ADI_FAT_FILE_DESCRIPTOR *)pFSSFile->FSD_data_handle;


    Mode=(u8)(pFSSFile->mode&0x00000007);

    SeekPosition=pSeek->offset;

    /* Adjust relative seeks to absolute */
    switch (pSeek->whence)
    {
    case 1: /* Relative to current location within the file */
        SeekPosition+=pFATFile->SeekPosition;
        break;

    case 2: /* Relative to end of file */
        SeekPosition+=pFSSFile->fsize;
        break;
    }


    Result=ADI_DEV_RESULT_FAILED;

    /* Check that seek is within file boundaries */
    if (SeekPosition>=0 &&
        (SeekPosition<=pFSSFile->fsize ||
        Mode==ADI_FSS_MODE_WRITE ||
        Mode==ADI_FSS_MODE_READ_WRITE))
    {
        pFATFile->SeekPosition=SeekPosition;
        Result=ADI_DEV_RESULT_SUCCESS;
    }

    return Result;
}











/* ************************************************
 * ************************************************
 *                DIRECTORY OPERATIONS
 * ************************************************
 * ************************************************
 */


/********************************************************************

    Function:       OpenDirectory

    Description:    Opens a directory for reading

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 OpenDirectory(
    ADI_FAT_VOLUME *pVolume,
    ADI_FSS_DIR_DEF *pDirDef)
{
    u32 Result;


    /* Open directory as if a file */
    Result=OpenFile(
        pVolume,
        pDirDef->pFileDesc,
        FILE_TYPE_SUBDIR);

    pDirDef->pFileDesc->curpos=0;
    pDirDef->tellpos=0;

    return Result;
}


/********************************************************************

    Function:       CreateDirectory

    Description:    Opens a directory for writing

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 CreateDirectory(
    ADI_FAT_VOLUME      *pVolume,
    ADI_FSS_DIR_DEF     *pDirDef)
{
    u32 Result;
    ADI_FAT_FILE_DESCRIPTOR *pFileDesc;


    /* Try opening the directory */
    Result=OpenDirectory(
        pVolume,
        pDirDef);

    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        pFileDesc=(ADI_FAT_FILE_DESCRIPTOR *)pDirDef->pFileDesc->FSD_data_handle;

        /* If the directory was opened successfully check that it is new, This
            can be done by checking that the first cluster is pointing to
            ADI_FAT_LAST_CLUSTER. If it is then add the '.' and '..' entry.
        */
        /* Check if directory is empty, if it not then return error */
        if (pFileDesc->FirstCluster==ADI_FAT_LAST_CLUSTER)
        {
            /* Add a new Cluster to the directory */
            Result=AddDirCluster(
                pVolume,
                pFileDesc->Entry.Cluster,
                pFileDesc->Entry.Offset,
                &pFileDesc->FirstCluster,
                NULL,
                TRUE);

            /* Add a '.' and '..' entry */
            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                ADI_FAT_DIR_ENTRY *pEntry;

                pEntry=NULL;

                /* Get the first entry */
                Result=ReadDirEntry(
                    pVolume,
                    pFileDesc->FirstCluster,
                    &pFileDesc->File.Cluster,
                    &pFileDesc->File.Offset,
                    &pEntry);

                /* Create the '.' entry which points to itself */
                if (Result==ADI_DEV_RESULT_SUCCESS)
                {
                    Result=CreateDirEntry(
                        pVolume,
                        pEntry,
                        ".",
                        ADI_FAT_DIR_SUBDIRECTORY,
                        pFileDesc->FirstCluster,
                        0);
                }

                /* Write the '.' entry back to disk and read the next entry */
                if (Result==ADI_DEV_RESULT_SUCCESS)
                {
                    Result=WriteNextDirEntry(
                        pVolume,
                        &pFileDesc->File.Cluster,
                        &pFileDesc->File.Offset,
                        &pEntry,
                        ADI_FAT_CLUSTER_TYPE_ADD);
                }

                /* Create the '..' entry which points to the parent cluster */
                if (Result==ADI_DEV_RESULT_SUCCESS)
                {
                    Result=CreateDirEntry(
                        pVolume,
                        pEntry,
                        "..",
                        ADI_FAT_DIR_SUBDIRECTORY,
                        pFileDesc->ParentCluster,
                        0);
                }

                /* Write it back to disk */
                if (Result==ADI_DEV_RESULT_SUCCESS)
                {
                    Result=WriteDirEntry(
                        pVolume,
                        &pEntry);
                }

                /* If there was an error then we still need to release the
                    directory entry
                */
                ReleaseDirEntry(
                    pVolume,
                    &pEntry);
            }
        }

        /* Close directory */
        CloseFile(
            pVolume,
            pDirDef->pFileDesc,
            FILE_TYPE_SUBDIR);
    }

    return Result;
}


/********************************************************************

    Function:       GetDirectory

    Description:    Reads the next valid directory entry

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 GetDirectory(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_DIR_DEF         *pDirDef)
{
    u32 Result;
    u32 bSearching = true;
    u32 bShowFile = true;
    ADI_FSS_WCHAR LongName[PDD_FAT_FILENAME_SIZE];
    ADI_FAT_DIR_ENTRY *pEntry;
    ADI_FAT_FILE_DESCRIPTOR *pFileDesc;


    do {
        pEntry=NULL;
        pFileDesc=(ADI_FAT_FILE_DESCRIPTOR *)pDirDef->pFileDesc->FSD_data_handle;

        /* Look for the next valid file or subdirectory entry */
        Result=GetDirEntry(
            pVolume,
            pFileDesc->FirstCluster,
            &(pFileDesc->File.Cluster),
            &(pFileDesc->File.Offset),
            NULL,
            LongName,
            NULL,
            NULL,
            NULL);

        /* Read in the 8.3 directory entry */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            pDirDef->tellpos=pFileDesc->File.Offset;

            Result=ReadDirEntry(
                pVolume,
                pFileDesc->FirstCluster,
                &pFileDesc->File.Cluster,
                &pFileDesc->File.Offset,
                &pEntry);
        }

        /* We want to halt the searching for a valid entry once the EOD is reached
        */
        if (Result==ADI_FSS_RESULT_EOD) {
            bSearching = false;
        }
        
        /* A hidden or system file is a valid entry but we do not want to 
         * expose them to the application by default.
        */
        bShowFile = pVolume->bShowHiddenSystem || (!IS_ATTRIBUTE(pEntry->Attr,ADI_FAT_DIR_HIDDEN) && !IS_ATTRIBUTE(pEntry->Attr,ADI_FAT_DIR_SYSTEM));
                
        if ( Result==ADI_DEV_RESULT_SUCCESS && bShowFile )
        {
            u16 i;

            /* Copy the file/dir name to the file/dir return structure */
            for (i=0;LongName[i];i++)
            {
                pDirDef->entry.d_name[i]=LongName[i];
            }
            pDirDef->entry.d_name[i]='\0';
            pDirDef->entry.d_namlen=i;

            /* Assign entry type */
            if ((pEntry->Attr&ADI_FAT_DIR_SUBDIRECTORY) == ADI_FAT_DIR_SUBDIRECTORY)
            {
                pDirDef->entry.d_type               = DT_DIR;
            }
            else if (IsFileEntry(pEntry->Attr))
            {
                pDirDef->entry.d_type               = DT_REG;
            }

            /* Synchronize File curpos with Directory tellpos */
            pDirDef->pFileDesc->curpos              = pDirDef->tellpos;
            pDirDef->entry.d_off                    = pDirDef->tellpos - sizeof(ADI_FAT_DIR_ENTRY);

            /* Assign Created Date and Time */
            pDirDef->entry.DateCreated.tm_mday      = FAT_DAY(pEntry->CrtDate);
            pDirDef->entry.DateCreated.tm_mon       = FAT_MONTH(pEntry->CrtDate);
            pDirDef->entry.DateCreated.tm_year      = FAT_YEAR(pEntry->CrtDate);
            pDirDef->entry.DateCreated.tm_sec       = FAT_SECOND(pEntry->CrtTime);
            pDirDef->entry.DateCreated.tm_min       = FAT_MINUTE(pEntry->CrtTime);
            pDirDef->entry.DateCreated.tm_hour      = FAT_HOUR(pEntry->CrtTime);

            /* Assign Modified Date and Time */
            pDirDef->entry.DateModified.tm_mday     = FAT_DAY(pEntry->WrtDate);
            pDirDef->entry.DateModified.tm_mon      = FAT_MONTH(pEntry->WrtDate);
            pDirDef->entry.DateModified.tm_year     = FAT_YEAR(pEntry->WrtDate);
            pDirDef->entry.DateModified.tm_sec      = FAT_SECOND(pEntry->WrtTime);
            pDirDef->entry.DateModified.tm_min      = FAT_MINUTE(pEntry->WrtTime);
            pDirDef->entry.DateModified.tm_hour     = FAT_HOUR(pEntry->WrtTime);

            /* Assign Last Access Date*/
            pDirDef->entry.DateLastAccess.tm_mday   = FAT_DAY(pEntry->LastAccDate);
            pDirDef->entry.DateLastAccess.tm_mon    = FAT_MONTH(pEntry->LastAccDate);
            pDirDef->entry.DateLastAccess.tm_year   = FAT_YEAR(pEntry->LastAccDate);

            /* Assign File Size Information */
            pDirDef->entry.d_size                   = pEntry->FileSize;
            
            /* Assign attributes */
            pDirDef->entry.Attributes               = pEntry->Attr;

            /* Release the directory entry */
            Result=ReleaseDirEntry(
                pVolume,
                &pEntry);
        
            bSearching = false;
        }
        
    } while (bSearching);

    return Result;
}

/********************************************************************

    Function:       ChangeFileAttributes

    Description:    Changes the File Attributes in the directory entry
                    Time stamps are not changed

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 ChangeFileAttributes(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_DIR_DEF         *pDirDef,
    u32                     Action
)
{
    u32 Result;
    u32 DirEntryCluster;
    u32 DirEntryOffset;
    ADI_FAT_DIR_ENTRY *pDirEntry = NULL;
    u8 Attributes;
    
    /* only allow the file-appropriate attributes */
    Attributes = (u8)(pDirDef->entry.Attributes & 0xFF);
    if (Attributes & ~(ADI_FAT_DIR_READONLY|ADI_FAT_DIR_HIDDEN|ADI_FAT_DIR_SYSTEM|ADI_FAT_DIR_ARCHIVE) )
    {
        return ADI_FAT_RESULT_INVALID_ATTRIBUTE;
    }
    
    /* Locate the file directory entry */
    Result = SeekDirEntry(
                            pVolume,
                            pDirDef->pFileDesc->pFullFileName,
                            ADI_FAT_SEEK_TYPE_CHILD,
                            &DirEntryCluster,
                            &DirEntryOffset,
                            NULL,
                            NULL,
                            NULL
                          );
                          
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        /* Read the directory entry */
        Result = ReadDirEntry(
                                pVolume,
                                DirEntryCluster,
                                &DirEntryCluster,
                                &DirEntryOffset,
                                &pDirEntry);
                                
        /* Change the file attributes */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            if (Action == ADI_FSD_CMD_ADD_FILE_ATTR) {
                pDirEntry->Attr |= (u8)(pDirDef->entry.Attributes & 0xFF);
            } 
            else if (Action == ADI_FSD_CMD_REMOVE_FILE_ATTR) {
                pDirEntry->Attr &= ~((u8)(pDirDef->entry.Attributes & 0xFF));
            } 
            else if (Action == ADI_FSD_CMD_REPLACE_FILE_ATTR) {
                pDirEntry->Attr = (u8)(pDirDef->entry.Attributes & 0xFF);
            } 

            /* And write the cluster back to the media */
            Result = WriteDirEntry(pVolume,&pDirEntry);
        }
    }
    else {
        Result = ADI_FSS_RESULT_NOT_FOUND;
    }
        
    return Result;
}


/********************************************************************

    Function:       SeekDirectory

    Description:    Seeks to a location in a directory

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 SeekDirectory(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_DIR_DEF         *pDirDef)
{
    u32 Result,TellPos;
    ADI_FAT_FILE_DESCRIPTOR *pFileDesc;


    Result=ADI_DEV_RESULT_SUCCESS;
    TellPos=0;
    pFileDesc=(ADI_FAT_FILE_DESCRIPTOR *)pDirDef->pFileDesc->FSD_data_handle;

    /* Reset directory pointer to start of directory */
    RewindDirectory(pDirDef);

    do
    {
        /* Go through the directory listing */
        Result=GetDirEntry(
            pVolume,
            pFileDesc->FirstCluster,
            &pFileDesc->File.Cluster,
            &pFileDesc->File.Offset,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL);
    } while (Result==ADI_DEV_RESULT_SUCCESS &&
        pFileDesc->File.Offset<pDirDef->tellpos);

    return Result;
}


/********************************************************************

    Function:       RewindDirectory

    Description:    Rewinds to start of directory

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 RewindDirectory(
    ADI_FSS_DIR_DEF         *pDirDef)
{
    ADI_FAT_FILE_DESCRIPTOR *pFileDesc;

    pFileDesc=(ADI_FAT_FILE_DESCRIPTOR *)pDirDef->pFileDesc->FSD_data_handle;

    /* Reset to start of directory */
    pFileDesc->File.Cluster=ADI_FAT_START_OF_FILE;
    pFileDesc->File.Offset=0;

    return ADI_FSS_RESULT_SUCCESS;
}


/********************************************************************

    Function:       ResetDirectory

    Description:    Set CWD to the root directory

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 ResetDirectory(
    ADI_FAT_VOLUME *pVolume)
{
    pVolume->CWDAddress=ADI_FAT_START_OF_FILE;

    return ADI_DEV_RESULT_SUCCESS;
}


/********************************************************************

    Function:       SetDirectory

    Description:    Move the CWD

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 SetDirectory(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_DIR_DEF         *pDirDef)
{
    u32 Result;
    ADI_FAT_DIR_ENTRY DirEntry;


    if (pDirDef==NULL)
    {
        Result=ResetDirectory(pVolume);
    }

    /* Open the directory first, if it fails then the directory posibly
        doesn't exist */
    else
    {
        Result=OpenDirectory(
            pVolume,
            pDirDef);

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            u32 Cluster;
            ADI_FAT_FILE_DESCRIPTOR *pDirDesc;

            /* Point CWDAddress to the first data cluster of the directory */
            pDirDesc=(ADI_FAT_FILE_DESCRIPTOR *)pDirDef->pFileDesc->FSD_data_handle;

            /* A Cluster address of 0 (ADI_FAT_FREE_CLUSTER) is actually pointing
                to the root directory, which can be pointed to with
                ADI_FAT_ROOT_CLUSTER
            */
            Cluster=(pDirDesc->FirstCluster==ADI_FAT_ROOT_CLUSTER)?
                ADI_FAT_START_OF_FILE:pDirDesc->FirstCluster;

            /* Check that the cluster address is not invalid */
            if (Cluster!=ADI_FAT_FREE_CLUSTER &&
                Cluster!=ADI_FAT_RESERVED_CLUSTER &&
                Cluster!=ADI_FAT_BAD_CLUSTER &&
                Cluster!=ADI_FAT_LAST_CLUSTER)
            {
                pVolume->CWDAddress=Cluster;
            }
            else
            {
                Result=ADI_DEV_RESULT_FAILED;
            }
            /* Close the directory */
            CloseFile(
                pVolume,
                pDirDef->pFileDesc,
                FILE_TYPE_SUBDIR);
        }
    }

    return Result;
}









/* ************************************************
 * ************************************************
 *            DIRECTORY ENTRY OPERATIONS
 * ************************************************
 * ************************************************
 */


/********************************************************************

    Function:       SeekDirEntry

    Description:    Determine whether the given path name exists in
                    the file system, Returns the address of the
                    entry in pCluster:pOffset

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 SeekDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_FULL_FNAME      *pFileName,
    ADI_FAT_SEEK_TYPE       Type,
    u32                     *pCluster,
    u32                     *pOffset,
    u8                      *pAttribute,    /* File attributes */
    u32                     *pFirstCluster, /* First data cluster of file/dir */
    u32                     *pFileSize)     /* Size of file/dir data area */
{
    u32 Result,Cluster,Offset,NewCluster;
    ADI_FSS_FULL_FNAME *pName;


    Result=ADI_DEV_RESULT_FAILED;

    /* Initialise some of the arguments */
    if (pAttribute)
    {
        *pAttribute=ADI_FAT_DIR_DEVICE;
    }

    if (pFirstCluster)
    {
        *pFirstCluster=ADI_FAT_ROOT_CLUSTER;
    }

    if (pFileSize)
    {
        pFileSize=0;
    }

    if (pFileName)
    {
        Result=ADI_DEV_RESULT_SUCCESS;
        pName=pFileName;

        /* If name is NULL then goto the root directory */
        if (pName->name==NULL)
        {
            /* Point to root directory */
            NewCluster=ADI_FAT_START_OF_FILE;
        }
        else
        {
            /* Start looking in the Current Working Directory */
            NewCluster=pVolume->CWDAddress;
        }
        Cluster=NewCluster;
        Offset=0;

        if (pFirstCluster)
        {
            *pFirstCluster=Cluster;
        }
    }

    /* Iterate the Full file name */
    while (Result==ADI_DEV_RESULT_SUCCESS && pName &&
        (Type!=ADI_FAT_SEEK_TYPE_PARENT || pName->pNext!=NULL))
    {
        u32 i;
        ADI_FSS_WCHAR FileName[PDD_FAT_FILENAME_SIZE];

        /* Copy the name into a ADI_FSS_WCHAR string */
        for (i=0;i<pName->namelen && i<PDD_FAT_FILENAME_SIZE-1 ;i++)
        {
            FileName[i]=pName->name[i];
        }
        FileName[i]='\0';

        /* If name is NULL then goto the root directory */
        if (FileName[0]=='\0')
        {
            /* Point to root directory */
            NewCluster=ADI_FAT_START_OF_FILE;
        }

        /* If file/directory name is not '.' then find the next directory,
            otherwise do nothing
        */
        else if (FileName[0]!='.' || FileName[1]!='\0')
        {
            u8 Attribute;

            Cluster=NewCluster;

            /* Locate FileName  entry */
            Result=FindDirEntry(
                pVolume,
                Cluster,
                FileName,
                &Cluster,
                &Offset,
                &Attribute,
                &NewCluster,
                pFileSize);

            /* Found the file/directory, now get the Attribute and First Data
                Cluster
            */
            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                if (pAttribute)
                {
                    *pAttribute=Attribute;
                }

                if (pFirstCluster)
                {
                    *pFirstCluster=NewCluster;
                }

                /* Not at the end of the path and not a directory, then file/
                    directory path is not valid */
                if (pName->pNext && !(Attribute&ADI_FAT_DIR_SUBDIRECTORY))
                {
                    /* Bad file name */
                    Result=ADI_FSS_RESULT_BAD_NAME;
                }
            }
        }

        /* Move to next name in the path list */
        pName=pName->pNext;
    }

    /* On success update the Cluster:Offset values */
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        if (pCluster)
        {
            *pCluster=Cluster;
        }

        if (pOffset)
        {
            *pOffset=Offset;
        }
    }

    return Result;
}


/********************************************************************

    Function:       FindDirEntry

    Description:    Locates an entry in the directory

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 FindDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ParentCluster,
    ADI_FSS_WCHAR           *pFileName,
    u32                     *pCluster,
    u32                     *pOffset,
    u8                      *pAttribute,    /* File attributes */
    u32                     *pFirstCluster, /* First data cluster of file/dir */
    u32                     *pFileSize)     /* Size of file/dir data area */
{
    u32 Result,Cluster,Offset;
    ADI_FSS_WCHAR ShortName[ADI_FAT_SHORT_NAME_SIZE],LongName[PDD_FAT_FILENAME_SIZE];
    u8 Attribute;


    Cluster=ADI_FAT_START_OF_FILE;
    Offset=0;

    /* Loop while there is no error and the file entry has not been located */
    do
    {
        /* Get Next valid entry */
        Result=GetDirEntry(
            pVolume,
            ParentCluster,
            &Cluster,
            &Offset,
            ShortName,
            LongName,
            &Attribute,
            pFirstCluster,
            pFileSize);

        if (Result == ADI_DEV_RESULT_SUCCESS 
            && (IS_ATTRIBUTE(Attribute,ADI_FAT_DIR_SUBDIRECTORY) || IsFileEntry(Attribute) )
        )
        {
            u16 i,j;
            ADI_FSS_WCHAR *pChar;

            /* Try to match the FileName with both the ShortName and LongName,
                the results of both checks are used to determine if there is
                a match
            */

            // find end of string & strip trailing dots...
            for (pChar = pFileName; *pChar != '\0'; pChar++);

            while (*(--pChar) == '.' && pChar != pFileName);  // ignore all trailing dots

            // avoid stomping on the '.' and '..' names...
            if (pChar != pFileName)
                *(++pChar) = '\0';                      // truncate at the first dot

            for (i=0;pFileName[i] && toupper(pFileName[i])==toupper(ShortName[i]);i++);
            for (j=0;pFileName[j] && toupper(pFileName[j])==toupper(LongName[j]);j++);

            if ((pFileName[i]==0 && ShortName[i]==0) ||
                (pFileName[j]==0 && LongName[j]==0))
            {
                /* Found a filename match, so return the Cluster:Offset
                    address of the 8.3 entry name
                */
                if (pCluster)
                {
                    *pCluster=Cluster;
                }
                if (pOffset)
                {
                    *pOffset=Offset;
                }
                if (pAttribute)
                {
                    *pAttribute=Attribute;
                }
                break;
            }
        }
    } while (Result==ADI_DEV_RESULT_SUCCESS);

    return Result;
}


/********************************************************************

    Function:       GetDirEntry

    Description:    Get the next valid directory entry in a given
                    directory, It starts at EntryCluster:EntryOffset,
                    and returns the address of the entry in
                    pCluster:pOffset. If it is a LFN entry then the
                    return address will point to the first LFN entry

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 GetDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ParentCluster,
    u32                     *pCluster,
    u32                     *pOffset,
    ADI_FSS_WCHAR           *pShortName,    /* Short 8.3 filename, this
                                                should be a 13 element buffer */
    ADI_FSS_WCHAR           *pLongName,     /* Long filename, max 255 bytes
                                                should be a 256 element buffer */
    u8                      *pAttribute,    /* File attributes */
    u32                     *pFirstCluster, /* First data cluster of file/dir */
    u32                     *pFileSize)     /* Size of file/dir data area */
{
    u32 Result,Index,Cluster,Offset,Count,CountCapture;
    u8 CheckSum;
    ADI_FAT_DIR_ENTRY *pEntry;
    ADI_FSS_WCHAR NameSegment[13];
    bool LFN;


    /* If the EntryCLuster points to the start of the file, then we grab
        the first valid entry we find, otherwise we grab the second
    */
    pEntry=NULL;
    LFN=FALSE;

    Cluster=*pCluster;
    Offset=*pOffset;

    if (Cluster!=ADI_FAT_START_OF_FILE)
    {
        /* Read the directory entry */
        Result=ReadDirEntry(
            pVolume,
            ParentCluster,
            &Cluster,
            &Offset,
            &pEntry);

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            /* Get the next directory entry */
            Result=ReadNextDirEntry(
                pVolume,
                &Cluster,
                &Offset,
                &pEntry,
                ADI_FAT_CLUSTER_TYPE_READ);
        }
    }

    /* Iterate directory until a match is found, or we run out of directory */
    do
    {
        /* Read the directory entry */
        Result=ReadDirEntry(
            pVolume,
            ParentCluster,
            &Cluster,
            &Offset,
            &pEntry);

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            /* (Free entry and end of directory listing) */
            if ((u8)(pEntry->Name[0])==0x00)
            {
                /* On EOD do not continue to the next entry */
                Result=ADI_FSS_RESULT_EOD;
                break;
            }

            /* ( Free Entry ) */
            else if ((u8)(pEntry->Name[0])==0xE5)
            {
                /* Erased directory entry, do nothing */
                LFN=FALSE;
            }

            /* ( Actual First Character = 0x05 ( valid Kanji ) ) */
            else
            {
#if defined(__ADI_FAT_LFN_ENABLE)
                if (IsLongDirEntry(pEntry->Attr))
                {
                    ADI_FAT_LFN_ENTRY *pLongEntry;

                    pLongEntry=(ADI_FAT_LFN_ENTRY *)pEntry;

                    if (IsLastLongEntry(pEntry->Name[0]))
                    {
                        /* If we're already processing a long name then the current
                            long name is orphaned and we can ignore it and restart
                            the process
                        */
                        LFN=TRUE;

                        /* Store checksum to compare against 8.3 name checksum */
                        CheckSum=pLongEntry->Chksum;
                    }

                    /* Process subsequent long entry first, validate
                        checksum, long file name attribute & FirstCluster
                        number
                    */
                    else if (pLongEntry->Chksum!=CheckSum ||
                        !(pLongEntry->Attr&ADI_FAT_DIR_LONG_FILE_NAME) ||
                        pLongEntry->FstClusLo!=0 )
                    {
                        LFN=FALSE;
                    }

                    /* Copy the long segment name into the pLongName field */
                    if (LFN && pLongName)
                    {
                        unsigned short i,j;
                        u16 ch;
                        bool Flag;

                        /* Initialise the long filename offset using the
                            ordinal value in the LFN entry
                        */
                        j=((pLongEntry->Ord-1)&0x3F)*13;
                        Flag=TRUE;

                        /* Copy the first segment of the filename */
                        for (i=0; i<10 && j<255 && Flag; i+=2)
                        {
                            ch=pLongEntry->Name1[i]|(pLongEntry->Name1[i+1]<<8);
                            pLongName[j++]=ch;
                            if (ch==0x0000 || ch==0xFFFF)
                            {
                                Flag=FALSE;
                            }
                        }

                        /* Copy the second segment of the filename */
                        for (i=0; i<12 && j<255 && Flag; i+=2)
                        {
                            ch=pLongEntry->Name2[i]|(pLongEntry->Name2[i+1]<<8);
                            pLongName[j++]=ch;
                            if (ch==0x0000 || ch==0xFFFF)
                            {
                                Flag=FALSE;
                            }
                        }

                        /* Copy the third segment of the filename */
                        for (i=0; i<4 && j<255 && Flag; i+=2)
                        {
                            ch=pLongEntry->Name3[i]|(pLongEntry->Name3[i+1]<<8);
                            pLongName[j++]=ch;
                            if (ch==0x0000 || ch==0xFFFF)
                            {
                                Flag=FALSE;
                            }
                        }

                        /* If we're at the end of the filename then append NULL */
                        if (pLongEntry->Ord&0x40)
                        {
                            pLongName[j++]='\0';
                        }
                    }
                }
#endif
                if (IsValidDirAttribute(pEntry->Attr))
                {
#if defined(__ADI_FAT_LFN_ENABLE)
                    if (LFN && ShortEntryCheckSum((u8 *)pEntry->Name)!=CheckSum)
                    {
                        LFN=FALSE;
                    }
#endif

                    if (pShortName)
                    {
                        unsigned char i,j;

                        /* Copy the first part of the 8.3 filename */
                        for (i=0,j=0;i<8 && pEntry->Name[i]!=' ';i++)
                        {
                            pShortName[j++]=pEntry->Name[i];
                        }

                        /* Copy the Extention of the 8.3 filename */
                        for (i=8;i<11 && pEntry->Name[i]!=' ';i++)
                        {
                            /* Insert a '.' character */
                            if (i==8)
                            {
                                pShortName[j++]='.';
                            }
                            pShortName[j++]=pEntry->Name[i];
                        }
                        pShortName[j++]='\0';
                    }

                    if (pLongName && !LFN)
                    {
                        unsigned char i,j;

                        /* Copy the first part of the 8.3 filename */
                        for (i=0,j=0;i<8 && pEntry->Name[i]!=' ';i++)
                        {
                            pLongName[j++]=pEntry->Name[i];
                        }

                        /* Copy the Extention of the 8.3 filename */
                        for (i=8;i<11 && pEntry->Name[i]!=' ';i++)
                        {
                            /* Insert a '.' character */
                            if (i==8)
                            {
                                pLongName[j++]='.';
                            }
                            pLongName[j++]=pEntry->Name[i];
                        }
                        pLongName[j++]='\0';
                    }

                    if (pAttribute)
                    {
                        *pAttribute=pEntry->Attr;
                    }

                    if (pFirstCluster)
                    {
                        u32 Cluster;

                        /* *pFirstCluster=Cluster=(pEntry->FstClusHi<<16)|pEntry->FstClusLo;*/
                        *pFirstCluster=Cluster=GetClusterDirEntry(pVolume,pEntry);

                        /* Convert cluster entry values to the FAT32
                            values, this just makes it easy to deal
                            with the special cases through out the code
                        */
                        switch (pVolume->Type)
                        {
                            case ADI_FAT_TYPE_FAT12:
                                Cluster|=ADI_FAT_LAST_CLUSTER&0x0FFFF000;
                                break;

                            case ADI_FAT_TYPE_FAT16:
                                Cluster|=ADI_FAT_LAST_CLUSTER&0x0FFF0000;
                                break;
                        }

                        /* Only update the *pCluster value if it is
                            one of the special cases
                        */
                        switch (Cluster)
                        {
                            case ADI_FAT_RESERVED_CLUSTER:
                            case ADI_FAT_BAD_CLUSTER:
                            case ADI_FAT_LAST_CLUSTER:
                                *pFirstCluster=Cluster;
                        }
                    }

                    if (pCluster)
                    {
                        *pCluster=Cluster;
                    }

                    if (pOffset)
                    {
                        *pOffset=Offset;
                    }

                    if (pFileSize)
                    {
                        *pFileSize=pEntry->FileSize;
                    }
                    break;
                }
            }
        }

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            /* Get the next directory entry */
            Result=ReadNextDirEntry(
                pVolume,
                &Cluster,
                &Offset,
                &pEntry,
                ADI_FAT_CLUSTER_TYPE_READ);
        }
    } while (Result==ADI_DEV_RESULT_SUCCESS);

    /* Release the directory entry */
    ReleaseDirEntry(
        pVolume,
        &pEntry);

    /* On success update the function outputs */
    return Result;
}


/********************************************************************

    Function:       AddDirEntry

    Description:    Add a new file/directory to the directory pointed
                    to by DirSector:DirOffset. If the directory is
                    empty then we add a new cluster and update the
                    directory entry to point to the new cluster

                    If pFileName does not fit into the standard 8.3
                    FAT filename, then add the LFN entries and the
                    8.3 entry

                    The sector address and size of entry are passed
                    to the function and if ADI_DEV_RESULT_SUCCESS is
                    returned then pSector and pOffset will contain
                    the location of the first entry.

                    On succes pCluster:pOffset are loaded with the

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 AddDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     Cluster,        /* Parent data cluster */
    ADI_FSS_WCHAR           *pFileName,     /* Filename of new Dir entry */
    u8                      FileType,       /* Type of Dir entry to add */
    u32                     FirstCluster,   /* Initial cluster value */
    u32                     *pCluster,
    u32                     *pOffset)
{
    u32 i, Result = ADI_DEV_RESULT_FAILED;
    u32 nEntriesReq=1; /* minimum of 1 entry required for the 8.3 name */
    u32 EOD,Offset,NewCluster,NewOffset,LFN,Chksum;
    s32 Count;
    u8 ShortEntry[ADI_FAT_SHORT_ENTRY_SIZE];
    ADI_FAT_DIR_ENTRY *pEntry;
    ADI_FAT_NAMETYPE eNameType;

    pEntry=NULL;
    Offset=0;
    EOD=0;
    LFN=0;
    Chksum=0;

    // always minimum of 1 entry required (if short name only 1 entry required)
    // if long name and LFN support enabled, get number of entries required
    // if long name and LFN support not enabled, error
    eNameType = ValidateName(pFileName);                // determine the type of filename
    if (eNameType == ADI_FAT_BAD_FILENAME)
        return ADI_DEV_RESULT_FAILED;                   // invalid filename
    if (eNameType == ADI_FAT_LFN)
    {
        // long filename detected ...
#if defined(__ADI_FAT_LFN_ENABLE)
        /* pFileName is a long file name, so determine the number of 13 character
           unicode entries required to store the name (excluding the null, since if
           the string exactly fits in an entry it is NOT null terminated) */
        ADI_FSS_WCHAR *pNameEnd = pFileName;

        while (*pNameEnd) pNameEnd++;
        /* round up number of entries required and add it to the one required for the 8.3 name */
        nEntriesReq += ((pNameEnd-pFileName)+ADI_FAT_SHORT_NAME_SIZE-1)/ADI_FAT_SHORT_NAME_SIZE;
        /* Create a short name (8.3) entry from the supplied file name */
        Result=GetShortEntryFromLFN(pVolume, Cluster, pFileName, pNameEnd, ShortEntry);
#else
        return Result;                                  // long filename not supported
#endif
    }
    else
    {
        // valid short filename detected ...
        // create an uppercased entry format string ...
        u32 i,j;

        // copy base
        for (i=0,j=0;pFileName[i] && pFileName[i] != '.';i++,j++)
        {
            ShortEntry[j]=toupper(pFileName[i]);
        }
        // pad rest of base (if required)
        for (;j < 8; j++)
            ShortEntry[j] = ' ';

        // check for an extension ...
        if (pFileName[i] == '.')
        {
            // copy the extension (skip the dot) ....
            for (i++; i<ADI_FAT_SHORT_NAME_SIZE && j<ADI_FAT_SHORT_ENTRY_SIZE; j++,i++)
                ShortEntry[j]=toupper(pFileName[i]);
        }
        // pad the extension (if necessary)...
        for (;j<ADI_FAT_SHORT_ENTRY_SIZE;j++)
            ShortEntry[j] = ' ';
        Result = ADI_DEV_RESULT_SUCCESS;
    }

    /* Go through the directory and decrement Count if a free entry is found
       if a valid used entry is found then reset Count to nEntriesReq
    */
    Count=nEntriesReq;
    while (Result==ADI_DEV_RESULT_SUCCESS && Count>0)
    {
        Result=ReadDirEntry(
            pVolume,
            ADI_FAT_ROOT_CLUSTER,
            &Cluster,
            &Offset,
            &pEntry);

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            /* Check for End Of Directory */
            if ((u8)(pEntry->Name[0])==0x00 && EOD==0)
            {
                EOD=1;

                /* We need an extra entry for the EOD */
                Count++;
                nEntriesReq++;
            }

            /* Free Entry */
            if ((u8)(pEntry->Name[0])==0xE5 || EOD==1)
            {
                /* Erased directory entry, decrement counter, include LFN
                    entry
                */
                if (Count==nEntriesReq && LFN==0)
                {
                    NewCluster=Cluster;
                    NewOffset=Offset;
                }
                Count-=1+LFN;
                LFN=0;
            }

#if defined(__ADI_FAT_LFN_ENABLE)
            /* LFN entry */
            else if (IsLongDirEntry(pEntry->Attr))
            {
                ADI_FAT_LFN_ENTRY *pLFN;

                pLFN=(ADI_FAT_LFN_ENTRY *)pEntry;

                /* Could potentially be a source of free entries */
                if (Count==nEntriesReq && LFN==0)
                {
                    NewCluster=Cluster;
                    NewOffset=Offset;
                }

                /* If checksum doesnt match old one then update count and
                    reset LFN count
                */
                if (Chksum!=pLFN->Chksum)
                {
                    Count-=LFN;
                    LFN=0;
                }

                /* Update the chksum */
                Chksum=pLFN->Chksum;

                /* Increment LFN count */
                LFN++;
            }
#endif

            /* Valid directory entry */
            else if (IsValidDirAttribute(pEntry->Attr))
            {
                /* If LFN entry check that LFN entry matches entry chksum, if
                    it doesn't then the previous LFN entries are not valid so
                    decrement the count and see if we have enough space
                */
#if defined(__ADI_FAT_LFN_ENABLE)
                if (LFN)
                {
                    if (ShortEntryCheckSum((u8 *)pEntry->Name)!=Chksum)
                    {
                        Count-=LFN;
                    }
                    LFN=0;
                }
#endif

                /* Reset counter if we didnt get enough space */
                if (Count>0)
                {
                    Count=nEntriesReq;
                }
            }
        }

        /* Increment relative position within chunk. A chunk will always
            contain an integral number of directory entries, thus when it
            reaches the end of the chunk, we reset the Offset to zero to
            trigger the fetching of the next chunk of data.
        */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            Result=ReadNextDirEntry(
                pVolume,
                &Cluster,
                &Offset,
                &pEntry,
                ADI_FAT_CLUSTER_TYPE_ADD);
        }
    }

    /* If we have found enough entries add the new entry */
    if (Count<=0)
    {
        /* If we've still got a directory chunk release it */
        Result=ReleaseDirEntry(
            pVolume,
            &pEntry);

        Cluster=NewCluster;
        Offset=NewOffset;

        Count=nEntriesReq;
        while (Result==ADI_DEV_RESULT_SUCCESS && Count--)
        {
            /* If pEntry pointer is NULL read current chunk */
            Result=ReadDirEntry(
                pVolume,
                ADI_FAT_FREE_CLUSTER,
                &Cluster,
                &Offset,
                &pEntry);

            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
#if defined(__ADI_FAT_LFN_ENABLE)
                /* Write a LFN entry */
                if (Count>EOD)
                {
                    u32 i,j,k,Inc;
                    ADI_FAT_LFN_ENTRY *pLFN;

                    pLFN=(ADI_FAT_LFN_ENTRY *)pEntry;

                    pLFN->Ord       = ((Count==nEntriesReq-1)?0x40:0x00)|(Count-EOD);
                    pLFN->Attr      = 0x0F;
                    pLFN->Type      = 0;
                    pLFN->Chksum    = ShortEntryCheckSum(ShortEntry);
                    pLFN->FstClusLo = 0;

                    for (k=0,j=((pLFN->Ord-1)&0x3F)*13,Inc=1;k<3;k++)
                    {
                        u8 *pStr;
                        u16 Len;

                        switch (k)
                        {
                            case 0:
                                pStr=pLFN->Name1;
                                Len=sizeof(pLFN->Name1);
                                break;

                            case 1:
                                pStr=pLFN->Name2;
                                Len=sizeof(pLFN->Name2);
                                break;

                            case 2:
                                pStr=pLFN->Name3;
                                Len=sizeof(pLFN->Name3);
                                break;
                        }
                        for (i=0;i<Len;i+=2,j+=Inc)
                        {
                            pStr[i]=Inc?pFileName[j]:0xFF;
                            pStr[i+1]=Inc?(pFileName[j]>>8):0xFF;
                            if (pFileName[j]=='\0')
                            {
                                Inc=0;
                            }
                        }
                    }
                }
                else
#endif

                /* Write the 8.3 dir/file entry */
                if (Count==EOD)
                {
                    CreateDirEntry(
                        pVolume,
                        pEntry,
                        ShortEntry,
                        FileType,
                        FirstCluster,
                        0);


                    if (pCluster)
                    {
                        *pCluster=Cluster;
                    }

                    if (pOffset)
                    {
                        *pOffset=Offset;
                    }
                }
                /* Write a new EOD if we have to, simply clear the entry */
                else if (!Count && EOD)
                {
                    memset(pEntry,0,sizeof(*pEntry));
                }
            }

            /* Increment relative position within chunk. A chunk will always
                contain an integral number of directory entries, thus when it
                reaches the end of the chunk, we reset the Offset to zero to
                trigger the fetching of the next chunk of data.
            */
            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                Result=WriteNextDirEntry(
                    pVolume,
                    &Cluster,
                    &Offset,
                    &pEntry,
                    ADI_FAT_CLUSTER_TYPE_ADD);
            }
        }

        /* If we've still got a directory chunk, write it */
        Result=WriteDirEntry(
            pVolume,
            &pEntry);
    }

    /* If we've still got a directory cluster, release it */
    ReleaseDirEntry(
        pVolume,
        &pEntry);

    return Result;
}


/********************************************************************

    Function:       CreateDirEntry

    Description:    Create a new directory entry

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 CreateDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FAT_DIR_ENTRY       *pEntry,
    u8                      *Name,
    u8                      Type,
    u32                     Cluster,
    u32                     Size)
{
    u8 i;
    struct tm Now;
    time_t lt;

    adi_rtc_GetDateTime(&Now);
    Now.tm_isdst=FALSE;
    lt=mktime(&Now);

    memset(pEntry->Name,' ',sizeof(pEntry->Name));
    for (i=0;i<sizeof(pEntry->Name) && Name[i];i++)
    {
        pEntry->Name[i]=Name[i];
    }

    if (Cluster==ADI_FAT_START_OF_FILE)
    {
        Cluster=ADI_FAT_ROOT_CLUSTER;
    }

    pEntry->Attr            = Type;
    pEntry->NTRes           = 0;
    pEntry->CrtTimeTenth    = 0;
    pEntry->CrtTime         = FAT_TIME(Now);
    pEntry->CrtDate         = FAT_DATE(Now);
    pEntry->LastAccDate     = pEntry->CrtDate;
    pEntry->WrtTime         = pEntry->CrtTime;
    pEntry->WrtDate         = pEntry->CrtDate;
    SetClusterDirEntry(pVolume,pEntry,Cluster);
    pEntry->FileSize        = Size;

    return ADI_DEV_RESULT_SUCCESS;
}


/********************************************************************

    Function:       UpdateDirEntry

    Description:    Amends the directory entry for a modified file

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 UpdateDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     EntryCluster,
    u32                     EntryOffset,
    u32                     FileSize)
{
    u32 Result;


    /* Don't update the root directory entry */
    //if (EntryCluster!=ADI_FAT_ROOT_CLUSTER)// &&
    if (EntryCluster!=ADI_FAT_START_OF_FILE)
    {
        ADI_FAT_DIR_ENTRY *pEntry;

        pEntry=NULL;

        Result=ReadDirEntry(
            pVolume,
            EntryCluster,
            &EntryCluster,
            &EntryOffset,
            &pEntry);

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            struct tm Now;
            time_t lt;

            adi_rtc_GetDateTime(&Now);
            Now.tm_isdst=FALSE;
            lt=mktime(&Now);

            /* Update File Size */
            pEntry->FileSize=FileSize;

            /* Update File Date/Time Data */
            pEntry->WrtTime     = FAT_TIME(Now);
            pEntry->WrtDate     = FAT_DATE(Now);
            pEntry->LastAccDate = pEntry->WrtDate;
            
            /* And write the cluster back to the media */
            Result=WriteDirEntry(
                pVolume,
                &pEntry);
        }
    }
    else
    {
        /* */
        Result=ADI_DEV_RESULT_SUCCESS;
    }

    return Result;
}

/********************************************************************

    Function:       RemoveDirEntry

    Description:    Invalidates a directory entry

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 RemoveDirEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     EntryCluster,
    u32                     EntryOffset)
{
    u32 Result;
    ADI_FAT_DIR_ENTRY *pEntry;


    pEntry=NULL;

    /* Clear long file name entries if required */
    do
    {
        Result=ReadDirEntry(
            pVolume,
            ADI_FAT_START_OF_FILE,
            &EntryCluster,
            &EntryOffset,
            &pEntry);

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            /* Remove File or Directory, as long as it is not the .. special
                directory entry
            */
            if (!memcmp(pEntry->Name,"..         ",sizeof(pEntry->Name)))
            {
                Result=ADI_DEV_RESULT_FAILED;
            }
            else if ( IsFileOrDirEntry(pEntry->Attr) )
            {
                u32 Cluster;

                /* Clear Directory Entry by setting first character of name ot 0xE5 */
                pEntry->Name[0]=0xE5;

                /* Free up cluster chain */
                /*Cluster=pEntry->FstClusLo|(pEntry->FstClusHi<<16);*/
                Cluster=GetClusterDirEntry(pVolume,pEntry);
                if (Cluster!=0 && Cluster!=(ADI_FAT_LAST_CLUSTER&(pVolume->ClusterMask)))
                {
                    RemoveFatEntry(
                        pVolume,
                        Cluster);

                    /*pEntry->FstClusLo=ADI_FAT_LAST_CLUSTER&pVolume->ClusterMask;
                    pEntry->FstClusHi=(ADI_FAT_LAST_CLUSTER&pVolume->ClusterMask)>>16;*/
                    SetClusterDirEntry(pVolume,pEntry,ADI_FAT_LAST_CLUSTER);
                }
                break;
            }
            else if (pEntry->Attr!=ADI_FAT_DIR_LONG_FILE_NAME)
            {
                Result=ADI_DEV_RESULT_FAILED;
            }
        }

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            Result=WriteNextDirEntry(
                pVolume,
                &EntryCluster,
                &EntryOffset,
                &pEntry,
                ADI_FAT_CLUSTER_TYPE_READ);
        }
    } while (Result==ADI_DEV_RESULT_SUCCESS);

    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        Result=WriteDirEntry(
            pVolume,
            &pEntry);
    }
    else
    {
        ReleaseDirEntry(
            pVolume,
            &pEntry);
    }

    return Result;
}

/********************************************************************

    Function:       GetClusterDirEntry

    Description:    Get the first cluster number from a directory
                    entry. The function also converts a file entry
                    cluster value of 0 to 0x0FFFFFFF for files so the
                    driver doesn't have to work with 2 different last
                    cluster values. For a directory a value of 0
                    means the root cluster.

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 GetClusterDirEntry(
    ADI_FAT_VOLUME      *pVolume,
    ADI_FAT_DIR_ENTRY   *pEntry)
{
    u32 Cluster;

    Cluster=ADI_FAT_FREE_CLUSTER;

    if (pEntry)
    {
        Cluster=(pEntry->FstClusLo|(pEntry->FstClusHi<<16));
        if (IsFileEntry(pEntry->Attr) && Cluster==ADI_FAT_FREE_CLUSTER)
        {
            Cluster=ADI_FAT_LAST_CLUSTER;
        }
    }

    return Cluster;
}


/********************************************************************

    Function:       SetClusterDirEntry

    Description:    Set the first cluster value of a directory entry.
                    This function converts a first cluster value of
                    0x0FFFFFFF to 0 for file for compatibility with
                    other OS's

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 SetClusterDirEntry(
    ADI_FAT_VOLUME      *pVolume,
    ADI_FAT_DIR_ENTRY   *pEntry,
    u32                 Cluster)
{
    if (pEntry)
    {
        if (IsFileEntry(pEntry->Attr) && Cluster==ADI_FAT_LAST_CLUSTER)
        {
            Cluster=ADI_FAT_FREE_CLUSTER;
        }
        Cluster&=pVolume->ClusterMask;

        pEntry->FstClusLo=Cluster;
        pEntry->FstClusHi=Cluster>>16;
    }

    return ADI_DEV_RESULT_SUCCESS;
}


/********************************************************************

    Function:       ReadDirEntry

    Description:    Read from the directory cache data cluster
                    *pCluster, and return the address at *pOffset.
                    If the cluster address is ADI_FAT_START_OF_FILE
                    then use ParentCluster. If Cluster = 0 then
                    we are reading the Root Directory. For
                    FAT12/FAT16 *pOffset is used to calculate a
                    sector offset since it is not in the cluster
                    data area.

                    You need to call either WriteDirEntry or
                    ReleaseDirEntry after calling this function to
                    release the cached memory block

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 ReadDirEntry(
    ADI_FAT_VOLUME      *pVolume,
    u32                 FirstCluster,
    u32                 *pCluster,
    u32                 *pOffset,
    ADI_FAT_DIR_ENTRY   **pEntry)
{
    if (*pEntry==NULL)
    {
        /* Adjust offset so that it aligned to ADI_FAT_DIR_ENTRY sturcture */
        *pOffset-=*pOffset%sizeof(**pEntry);
    }

    return ReadDirCluster(
            pVolume,
            FirstCluster,
            pCluster,
            pOffset,
            (u8 **)pEntry);
}


/********************************************************************

    Function:       ReadNextDirEntry

    Description:    Read the next directory entry, first we
                    increment the Offset by one directory entry,
                    if we overun into a new cluster then release the
                    current cluster and load the next one

                    If flag is set to ADI_FAT_CLUSTER_ADD then if we
                    are at the last cluster a new one will be added,
                    otherwise if it is ADI_FAT_CLUSTER_READ then we
                    have reached the end of file

                    You need to call either WriteDirEntry or
                    ReleaseDirEntry after calling this function to
                    release the cached memory block

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 ReadNextDirEntry(
    ADI_FAT_VOLUME      *pVolume,
    u32                 *pCluster,
    u32                 *pOffset,
    ADI_FAT_DIR_ENTRY   **pEntry,
    ADI_FAT_CLUSTER_TYPE Type)
{
    u32 Result,Offset;

    Result=ADI_DEV_RESULT_SUCCESS;

    Offset=*pOffset+sizeof(ADI_FAT_DIR_ENTRY);
    /* If we have crossed a cluster boundary load the next cluster */
    if ((Offset/(pVolume->ClusterSize*pVolume->SectorSize))!=
        ((*pOffset)/(pVolume->ClusterSize*pVolume->SectorSize)))
    {
        /* Release directory cluster before moving to the next */
        Result=ReleaseDirCluster(
            pVolume,
            (u8 **)pEntry);

        /* Get the next cluster */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            Result=GetNextDirCluster(
                pVolume,
                Type,
                pCluster,
                Offset);
        }
    }
    else if (*pEntry)
    {
        (*pEntry)++;
    }


    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        *pOffset=Offset;
    }

    return Result;
}


/********************************************************************

    Function:       WriteDirEntry

    Description:    Write a directory cluster back to the cluster
                    cache. Called after a ReadDirEntry

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 WriteDirEntry(
    ADI_FAT_VOLUME      *pVolume,
    ADI_FAT_DIR_ENTRY   **pEntry)
{
    return WriteDirCluster(
        pVolume,
        (u8**)pEntry);
}


/********************************************************************

    Function:       WriteNextDirEntry

    Description:    Read the next directory entry, first we
                    increment the Offset by one directory entry,
                    if we overun into a new cluster then write the
                    current cluster and load the next one

                    If flag is set to ADI_FAT_CLUSTER_ADD then if we
                    are at the last cluster a new one will be added,
                    otherwise if it is ADI_FAT_CLUSTER_READ then we
                    have reached the end of file

                    You need to call either WriteDirEntry or
                    ReleaseDirEntry after calling this function to
                    release the cached memory block

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 WriteNextDirEntry(
    ADI_FAT_VOLUME      *pVolume,
    u32                 *pCluster,
    u32                 *pOffset,
    ADI_FAT_DIR_ENTRY   **pEntry,
    ADI_FAT_CLUSTER_TYPE Type)
{
    u32 Result,Offset;

    Result=ADI_DEV_RESULT_SUCCESS;

    Offset=*pOffset+sizeof(ADI_FAT_DIR_ENTRY);

    /* If we have crossed a cluster boundary load the next cluster */
    if ((Offset/(pVolume->ClusterSize*pVolume->SectorSize))!=
        ((*pOffset)/(pVolume->ClusterSize*pVolume->SectorSize)))
    {
        /* Write directory cluster before moving to the next */
        Result=WriteDirCluster(
            pVolume,
            (u8 **)pEntry);

        /* Get the next cluster */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            Result=GetNextDirCluster(
                pVolume,
                Type,
                pCluster,
                Offset);
        }
    }
    else if (*pEntry)
    {
        (*pEntry)++;
    }

    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        *pOffset=Offset;
    }

    return Result;
}


/********************************************************************

    Function:       ReleaseDirEntry

    Description:    Release the current directory cached cluster.
                    Called after a ReadDirEntry.

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 ReleaseDirEntry(
    ADI_FAT_VOLUME      *pVolume,
    ADI_FAT_DIR_ENTRY   **pEntry)
{
    return ReleaseDirCluster(
        pVolume,
        (u8**)pEntry);
}










/* ************************************************
 * ************************************************
 *           DIRECTORY CLUSTER OPERATIONS
 * ************************************************
 * ************************************************
 */


/********************************************************************

    Function:       GetDirCluster

    Description:

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 GetDirCluster(
    ADI_FAT_VOLUME          *pVolume,
    u32                     FirstCluster,
    u32                     CurrentCluster,
    u32                     *pNextCluster)
{
    u32 Result,Cluster;


    Result=ADI_DEV_RESULT_SUCCESS;

    /* Check that Sector is not pointing to the Start Of File, if it is then
        we need to get the first cluster address from the Dir Entry.
    */
    if (CurrentCluster==ADI_FAT_START_OF_FILE)
    {
        Cluster=FirstCluster;
        if (Cluster==ADI_FAT_START_OF_FILE)
        {
            Cluster=ADI_FAT_LAST_CLUSTER;
        }
    }
    else
    {
        /* Get the next cluster from the FAT table, on success update the
            pCluster sector value
        */
        Result=GetFatEntry(
            pVolume,
            CurrentCluster,
            &Cluster);

        /* If the next cluster is one of the special cases return an error */
        if (Result==ADI_DEV_RESULT_SUCCESS &&
            (Cluster==ADI_FAT_FREE_CLUSTER ||
            Cluster==ADI_FAT_RESERVED_CLUSTER ||
            Cluster==ADI_FAT_BAD_CLUSTER))
        {
            Result=ADI_DEV_RESULT_FAILED;
        }
    }

    if (Result==ADI_DEV_RESULT_SUCCESS && pNextCluster)
    {
        *pNextCluster=Cluster;
    }

    return Result;
}


/********************************************************************

    Function:       GetNextDirCluster

    Description:    Get the next cluster

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 GetNextDirCluster(
    ADI_FAT_VOLUME      *pVolume,
    u32                 Type,
    u32                 *pCluster,
    u32                 Offset)
{
    u32 Result;

    /* If we are a FAT32 partition or we are not in the root directory
        of a FAT12 or FAT16 directory then get/add the next cluster
    */
    if (pVolume->Type==ADI_FAT_TYPE_FAT32 || *pCluster)
    {
        switch (Type)
        {
            case ADI_FAT_CLUSTER_TYPE_READ:
                Result=GetFatEntry(
                    pVolume,
                    *pCluster,
                    pCluster);
                break;

            case ADI_FAT_CLUSTER_TYPE_ADD:
                Result=AddFatEntry(
                    pVolume,
                    *pCluster,
                    pCluster,
                    1);
                break;
        }

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            switch (*pCluster)
            {
                case ADI_FAT_FREE_CLUSTER:
                case ADI_FAT_RESERVED_CLUSTER:
                case ADI_FAT_BAD_CLUSTER:
                    Result=ADI_DEV_RESULT_FAILED;
                    break;

                case ADI_FAT_LAST_CLUSTER:
                    Result=ADI_FSS_RESULT_EOD;
            }
        }
    }

    /* Otherwise we are in the FAT12/FAT16 root directory which is a fixed
        size, we need to do some calculation in this case
    */
    else
    {
        if (pVolume->RootDirAddress+(Offset/pVolume->SectorSize)>=
            pVolume->DataAddress)
        {
            Result=ADI_FSS_RESULT_EOD;
        }
        else
        {
            Result=ADI_DEV_RESULT_SUCCESS;
        }
    }

    return Result;
}


/********************************************************************

    Function:       AddDirCluster

    Description:    Add a cluster entry to a file entry, if a new
                    cluster is allocated to a file that currently
                    does not have any cluster, then the directory
                    file entry will be updated

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 AddDirCluster(
    ADI_FAT_VOLUME          *pVolume,
    u32                     EntryCluster,
    u32                     EntryOffset,
    u32                     *pFirstCluster,
    u32                     *pNextCluster,
    u32                     Clear)
{
    u32 Result,NewCluster,CurrentCluster;

    NewCluster=ADI_FAT_LAST_CLUSTER;
    CurrentCluster=*pFirstCluster;

    if (pNextCluster)
    {
        CurrentCluster=*pNextCluster;
    }

    /* First try to get the next sector */
    Result=GetDirCluster(
        pVolume,
        *pFirstCluster,
        CurrentCluster,
        &NewCluster);

    /* If we are at the end of the file then try to allocate a new cluster */
    if (Result==ADI_DEV_RESULT_SUCCESS &&
        NewCluster==ADI_FAT_LAST_CLUSTER)
    {
        Result=AddFatEntry(
            pVolume,
            CurrentCluster,
            &NewCluster,
            Clear);
    }

    /* If we have allocated a new cluster and pFirstCluster is equal to
        ADI_FAT_LAST_CLUSTER then update the File Entry to point to the new
        cluster
    */
    if (Result==ADI_DEV_RESULT_SUCCESS &&
        *pFirstCluster==ADI_FAT_LAST_CLUSTER)
    {
        ADI_FAT_DIR_ENTRY *pEntry;

        pEntry=NULL;

        /* Open directory entry */
        Result=ReadDirEntry(
            pVolume,
            ADI_FAT_LAST_CLUSTER,
            &EntryCluster,
            &EntryOffset,
            &pEntry);

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            /* Update the first cluster address */
            /*pEntry->FstClusLo=NewCluster&pVolume->ClusterMask;
            pEntry->FstClusHi=(NewCluster&pVolume->ClusterMask)>>16;*/
            SetClusterDirEntry(pVolume,pEntry,NewCluster);

            /* If we successfully updated directory entry then write cluster
                back to disk
            */
            Result=WriteDirEntry(
                pVolume,
                &pEntry);
        }
    }

    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        if (pNextCluster)
        {
            *pNextCluster=NewCluster;
        }

        if (*pFirstCluster==ADI_FAT_LAST_CLUSTER)
        {
            *pFirstCluster=NewCluster;
        }
    }

    return Result;
}


/********************************************************************

    Function:       ReadDirCluster

    Description:    Read directory cluster,

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 ReadDirCluster(
    ADI_FAT_VOLUME      *pVolume,
    u32                 FirstCluster,
    u32                 *pCluster,
    u32                 *pOffset,
    u8                  **pCacheData)
{
    u32 Result;

    Result=ADI_DEV_RESULT_SUCCESS;

    if (*pCacheData==NULL)
    {
        /* Check if we're at the start of the file, ajust pCluster to point
            to the actual first cluster if we are at the start
        */
        if (*pCluster==ADI_FAT_START_OF_FILE)
        {
            if (FirstCluster==ADI_FAT_START_OF_FILE)
            {
                *pCluster=ADI_FAT_ROOT_CLUSTER;
            }
            else
            {
                *pCluster=FirstCluster;
            }

            *pOffset=0;
        }

        if (pVolume->Type==ADI_FAT_TYPE_FAT32 &&
            *pCluster==ADI_FAT_ROOT_CLUSTER)
        {
            *pCluster=pVolume->RootDirAddress;
        }

        /* Read in cluster data */
        Result=adi_fsd_cache_Read(
            pVolume->DirCacheHandle,
            GetClusterSectorNumber(pVolume,*pCluster,*pOffset),
            GetClusterSectorSize(pVolume,*pCluster,*pOffset),
            pCacheData);

        *pCacheData+=(*pOffset)%(pVolume->ClusterSize*pVolume->SectorSize);
    }

    return Result;
}


/********************************************************************

    Function:       ReleaseDirCluster

    Description:    Release directory cluster,

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 ReleaseDirCluster(
    ADI_FAT_VOLUME      *pVolume,
    u8                  **pCacheData)
{
    u32 Result;

    Result=ADI_DEV_RESULT_SUCCESS;

    if (*pCacheData!=NULL)
    {
        /* Release the cached data */
        Result=adi_fsd_cache_Release(
            pVolume->DirCacheHandle,
            *pCacheData);

        *pCacheData=NULL;
    }

    return Result;
}


/********************************************************************

    Function:       WriteDirCluster

    Description:    Write directory cluster,

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 WriteDirCluster(
    ADI_FAT_VOLUME      *pVolume,
    u8                  **pCacheData)
{
    u32 Result;

    if (pVolume->ReadOnly==TRUE)
    {
        Result=ADI_DEV_RESULT_FAILED;
        ReleaseDirCluster(pVolume,pCacheData);
    }
    else if (*pCacheData!=NULL)
    {
        /* Write the cached data */
        Result=adi_fsd_cache_Write(
            pVolume->DirCacheHandle,
            *pCacheData);

        *pCacheData=NULL;
    }
    else
    {
        Result=ADI_DEV_RESULT_SUCCESS;
    }

    return Result;
}


/********************************************************************

    Function:       GetClusterSectorNumber

    Description:    Convert a cluster number into a sector

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 GetClusterSectorNumber(
    ADI_FAT_VOLUME      *pVolume,
    u32                 Cluster,
    u32                 Offset)
{
    u32 Result;

    if (Cluster==ADI_FAT_ROOT_CLUSTER)
    {
        Result=Offset;
        Result/=pVolume->ClusterSize*pVolume->SectorSize;
        Result*=pVolume->ClusterSize;
        Result+=pVolume->RootDirAddress;
    }
    else
    {
        Result=((Cluster-2)*pVolume->ClusterSize)+pVolume->DataAddress;
    }

    return Result;
}


/********************************************************************

    Function:       GetClusterSectorSize

    Description:    Get the cluster size given the Cluster:Offset of
                    the directory cluster. This checks for a FAT12
                    and FAT16 partition that the root directory
                    cluster does not overflow into the data area

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 GetClusterSectorSize(
    ADI_FAT_VOLUME      *pVolume,
    u32                 Cluster,
    u32                 Offset)
{
    u32 Result;

    Result=pVolume->ClusterSize;

    switch (pVolume->Type)
    {
        case ADI_FAT_TYPE_FAT12:
        case ADI_FAT_TYPE_FAT16:
            if (Cluster==ADI_FAT_ROOT_CLUSTER)
            {
                u32 Sector;

                Sector=GetClusterSectorNumber(pVolume,Cluster,Offset);

                if (Sector>pVolume->DataAddress)
                {
                    Result=0;
                }
                else if (Sector+Result>pVolume->DataAddress)
                {
                    Result=pVolume->DataAddress-Sector;
                }
            }
    }

    return Result;
}










/* ************************************************
 * ************************************************
 *                   FAT OPERATIONS
 * ************************************************
 * ************************************************
 */


/********************************************************************

    Function:       GetFatEntry

    Description:    Given a cluster number, return the next one

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 GetFatEntry(
    ADI_FAT_VOLUME      *pVolume,
    u32                 CurrentCluster,
    u32                 *pNextCluster)
{
    u32 Result;

    Result=ADI_DEV_RESULT_SUCCESS;

    if (CurrentCluster!=ADI_FAT_LAST_CLUSTER &&
        CurrentCluster!=ADI_FAT_START_OF_FILE)
    {
        u32 Cluster;
        u8 *pFatEntry;

        pFatEntry=NULL;

        /* Get FAT entry */
        Result=ReadFatEntry(
            pVolume,
            CurrentCluster,
            &pFatEntry);

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            switch (pVolume->Type)
            {
                case ADI_FAT_TYPE_FAT12:
                    Cluster=((pFatEntry[0]+(pFatEntry[1]<<8))>>((CurrentCluster&1)?4:0))&0xFFF;
                    break;

                case ADI_FAT_TYPE_FAT16:
                    Cluster=*((u16 *)pFatEntry);
                    break;

                default:
                    Cluster=*((u32 *)pFatEntry);
            }

            /* Release the FAT sector */
            Result=ReleaseFatEntry(
                pVolume,
                CurrentCluster,
                &pFatEntry);
        }

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            /* If everything went well return the Cluster number of the next
                cluster
            */
            if (pNextCluster)
            {
                /* Check the cluster value */
                if (((Cluster|0x0F)&pVolume->ClusterMask)==pVolume->ClusterMask)
                {
                    /* This is a special cluster value, now to work out what
                        type it is, we also convert it to the FAT32 version
                        so we dont have to worry about the conversion anywhere
                        else
                    */
                    if ((Cluster&0x0F)<0x07)
                    {
                        Cluster=ADI_FAT_RESERVED_CLUSTER;
                    }
                    else if ((Cluster&0x0F)==0x07)
                    {
                        Cluster=ADI_FAT_BAD_CLUSTER;
                    }
                    else
                    {
                        Cluster=ADI_FAT_LAST_CLUSTER;
                    }
                }
                else if (Cluster==0)
                {
                    Cluster=ADI_FAT_FREE_CLUSTER;
                }
                else if (Cluster==1)
                {
                    Cluster=ADI_FAT_RESERVED_CLUSTER;
                }

                *pNextCluster=Cluster;
            }
        }
    }
    else if (pNextCluster)
    {
        *pNextCluster=ADI_FAT_LAST_CLUSTER;
    }

    return Result;
}


/********************************************************************

    Function:       SetFatEntry

    Description:    Set the FAT entry to Value. You have to be
                    careful with calls to this function as a wrong
                    call or arguments can cause loss of information.
                    AddFatEntry and RemoveFatEntry are used to add
                    and remove FAT cluster entries.

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 SetFatEntry(
    ADI_FAT_VOLUME      *pVolume,
    u32                 Cluster,
    u32                 Value)
{
    u32 Result;


    Result=ADI_DEV_RESULT_FAILED;

    /* Do not allow setting a FAT entry to point to an invalid cluster */
    if (Cluster!=ADI_FAT_FREE_CLUSTER &&
        Cluster!=ADI_FAT_RESERVED_CLUSTER &&
        Cluster!=ADI_FAT_BAD_CLUSTER &&
        Cluster!=ADI_FAT_LAST_CLUSTER)
    {
        u8 *pFatEntry;

        pFatEntry=NULL;

        Result=ReadFatEntry(
            pVolume,
            Cluster,
            &pFatEntry);

        /* Update the fat entry to the new Value */
        switch (pVolume->Type)
        {
            case ADI_FAT_TYPE_FAT12:
                if (Value==ADI_FAT_RESERVED_CLUSTER ||
                    Value==ADI_FAT_BAD_CLUSTER ||
                    Value==ADI_FAT_LAST_CLUSTER ||
                    !(Value&0xFFFFF000))
                {
                    /* Adjust the cluster value */
                    Value=((pFatEntry[0]+(pFatEntry[1]<<8))&((Cluster&1)?0x000F:0xF000))|
                        ((Value&0xFFF)<<((Cluster&1)?4:0));

                    /* Write it back in 2 byte operations to avoid odd byte
                        boundard word transfers
                    */
                    pFatEntry[0]=Value;
                    pFatEntry[1]=Value>>8;
                }
                else
                {
                    Result=ADI_DEV_RESULT_FAILED;
                }
                break;

            case ADI_FAT_TYPE_FAT16:
                if (Value==ADI_FAT_RESERVED_CLUSTER ||
                    Value==ADI_FAT_BAD_CLUSTER ||
                    Value==ADI_FAT_LAST_CLUSTER ||
                    !(Value&0xFFFF0000))
                {
                    *(u16 *)pFatEntry=Value;
                }
                else
                {
                    Result=ADI_DEV_RESULT_FAILED;
                }
                break;

            default:
                *(u32 *)pFatEntry=Value;
        }

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            /* Write Fat Entry back */
            Result=WriteFatEntry(
                pVolume,
                Cluster,
                &pFatEntry);
        }
        else
        {
            ReleaseFatEntry(
                pVolume,
                Cluster,
                &pFatEntry);
        }
    }

    /* If successfully set cluster to FREE then update FreeCluster value if
        the freed cluster is less than the current free cluster or if
        FreeCluster is 0 (ie no free clusters availiable)
    */
    if (Result==ADI_DEV_RESULT_SUCCESS && Value==ADI_FAT_FREE_CLUSTER)
    {
        if (!pVolume->FreeCluster||Cluster<pVolume->FreeCluster)
        {
            pVolume->FreeCluster=Cluster;
        }
    }

    return Result;
}


/********************************************************************

    Function:       AddFatEntry

    Description:    Get the next cluster sector address, if we have
                    the end of the cluster chain then this function
                    will try to add a new cluster and move the end
                    chain value to the new cluster FAT entry. If a
                    new cluster is found the cluster and InitFlag is
                    is set the reset cluster to all zeros. This
                    function uses the GetFatEntry and SetFatEntry
                    functions to manipulate the FAT

    Todo: This function could possibly be sped up if we first looked
        for free clusters in the FAT sectors already loaded into
        cache before searching the rest of the disk

        The other speedup could be done by adding a function to the
        FSD cache to create an entry without loading it from disk.
        This would be used where the cluster has to be initialised
        to all zeros. The actual cluster contents on the disk are
        not important, so we don't actually need to read it.

********************************************************************/

__ADI_FAT_SECTION_CODE
u32 AddFatEntry(
    ADI_FAT_VOLUME      *pVolume,
    u32                 CurrentCluster,
    u32                 *pNextCluster,
    u32                 InitFlag)
{
    u32 Result,NextCluster;

    Result=ADI_DEV_RESULT_SUCCESS;
    
    /* Try to get the next cluster number in the chain */
    Result=GetFatEntry(
        pVolume,
        CurrentCluster,
        &NextCluster);

    /* If we fail to get the next cluster pointed to by SectorNumber  and it
        returns that we are at the end to the chain then use
        pVolume->LastFreeSector as a starting point to find the next free
        cluster, if one is found then update pVolume->LastFreeSector to point
        to the next cluster.

        If we are trying to add to the root directory of a FAT12 or FAT16
        partition and GetFatEnry failed then there is no room left in the
        root directory and we cannot add more, since SectorNumber will point
        to pVolume->DataAddress then next if statement will fail and this
        fuction will pass fail back to the calling function.

        If pVolume->FreeCluster is zero then the partition is full. This value
        can only be set to a cluster value by setting a FAT cluster entry to
        free.
    */
    if (Result==ADI_DEV_RESULT_SUCCESS &&
        NextCluster==ADI_FAT_LAST_CLUSTER &&
        pVolume->ReadOnly==FALSE &&
        pVolume->FreeCluster!=0)
    {
        u32 Value;

        NextCluster=pVolume->FreeCluster;

        /* Loop while we haven't checked every cluster entry or until we find
            a free cluster
        */
        do
        {
            /* Get the next entry */
            Result=GetFatEntry(
                pVolume,
                NextCluster,
                &Value);

            if (Value==ADI_FAT_FREE_CLUSTER)
            {
                break;
            }

            /* Get the next cluster */
            NextCluster++;

            /* If we are at the end of the partition */
            if (NextCluster>=pVolume->Clusters)
            {
                /* Start back at the start */
                NextCluster=ADI_FAT_FIRST_CLUSTER;
            }

            if (NextCluster==pVolume->FreeCluster)
            {
                /* Unable to find a new cluster, volume is full */
                Result = ADI_FSS_RESULT_MEDIA_FULL;
                pVolume->bMediaFull = true;
                pVolume->FreeCluster = 0;
            }
        } while (Result==ADI_DEV_RESULT_SUCCESS);

        /* If we didnt find a free cluster then set pVolume->FreeCluster to
            zero and return failure
        */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            /* Found a free cluster, set the FreeCluster value */
            pVolume->FreeCluster=NextCluster;

            if (CurrentCluster!=ADI_FAT_START_OF_FILE &&
                CurrentCluster!=ADI_FAT_LAST_CLUSTER)
            {
                Result=SetFatEntry(
                    pVolume,
                    CurrentCluster,
                    NextCluster);
            }

            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                Result=SetFatEntry(
                    pVolume,
                    NextCluster,
                    ADI_FAT_LAST_CLUSTER);
            }
        }

        /* We've found a free cluster, if the InitFlag is set then the cluster
            has to be read in and cleared,
        */
        if (Result==ADI_DEV_RESULT_SUCCESS && InitFlag)
        {
            u8 *pCacheData;


            pCacheData=NULL;

            /* Allocate buffer in cache, we dont care what the media
                cluster contains because we're going to clear it straight
                away
            */
            Result=adi_fsd_cache_Allocate(
                pVolume->DirCacheHandle,
                GetClusterSectorNumber(pVolume,NextCluster,0),
                pVolume->ClusterSize,
                &pCacheData);

            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                /* Clear the data block */
                memset(pCacheData,0,pVolume->ClusterSize*pVolume->SectorSize);

                /* Write the initialised cluster back to the drive */
                Result=adi_fsd_cache_Write(
                    pVolume->DirCacheHandle,
                    pCacheData);
            }
        }
    }
    else if (pVolume->FreeCluster==0)
    {
        Result=ADI_FSS_RESULT_MEDIA_FULL;
        pVolume->bMediaFull = true;
    }
    else if (NextCluster==ADI_FAT_LAST_CLUSTER &&
        pVolume->ReadOnly==TRUE)
    {
        Result=ADI_DEV_RESULT_FAILED;
    }


    /* On success return the Sector number of the next cluster */
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        *pNextCluster=NextCluster;
        pVolume->nFreeClusterCount--;
        WriteFSInfoSector(pVolume, false);
    }

    return Result;
}


/********************************************************************

    Function:       RemoveFatEntry

    Description:    Remove a FAT entry cluster chain

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 RemoveFatEntry(
    ADI_FAT_VOLUME      *pVolume,
    u32                 Cluster)
{
    u32 Result;

    Result=ADI_DEV_RESULT_SUCCESS;

    if (pVolume->ReadOnly==TRUE)
    {
        Result=ADI_DEV_RESULT_FAILED;
    }

    /* Check for special cluster values */
    while (Result==ADI_DEV_RESULT_SUCCESS &&
        Cluster!=ADI_FAT_LAST_CLUSTER)
    {
        u32 OldCluster;

        /* Check if the cluster value is valid */
        if (Cluster==ADI_FAT_FREE_CLUSTER ||
            Cluster==ADI_FAT_RESERVED_CLUSTER ||
            Cluster==ADI_FAT_BAD_CLUSTER)
        {
            /* Nope! so fail */
            Result=ADI_DEV_RESULT_FAILED;
            break;
        }

        /* Make a backup of the current cluster */
        OldCluster=Cluster;

        /* Get the next sector */
        Result=GetFatEntry(
            pVolume,
            Cluster,
            &Cluster);

        /* We successfully found the next cluster, set the current one to
            free
        */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            Result=SetFatEntry(
                pVolume,
                OldCluster,
                ADI_FAT_FREE_CLUSTER);
            pVolume->nFreeClusterCount++;
            WriteFSInfoSector(pVolume, false);
        }
    }
    
    if (pVolume->nFreeClusterCount>2)
    {
        pVolume->bMediaFull = false;
    }

    return Result;
}


/********************************************************************

    Function:       ReadFatEntry

    Description:    Load fat sector where ClusterNumber is located
                    and returns a pointer to pFatEntry value

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 ReadFatEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ClusterNumber,
    u8                      **pFatEntry)
{
    u32 Result;

    Result=ADI_DEV_RESULT_SUCCESS;

    /* If pFatEntry is not set the read in the Fat Sector and point pFatEntry
        to the correct address where ClusterNumber is pointing to
    */
    if (*pFatEntry==NULL)
    {
        u32 Offset,Size;

        switch (pVolume->Type)
        {
            case ADI_FAT_TYPE_FAT32:
                Offset=4*ClusterNumber;
                Size=1;
                break;

            case ADI_FAT_TYPE_FAT16:
                Offset=2*ClusterNumber;
                Size=1;
                break;

            case ADI_FAT_TYPE_FAT12:
                Offset=(3*ClusterNumber)/2;
                Size=2;
        }

        Result=adi_fsd_cache_Read(
            pVolume->FatCacheHandle,
            pVolume->FatAddress+(Offset/(pVolume->SectorSize*Size)),
            Size,
            pFatEntry);

        *pFatEntry+=Offset%(pVolume->SectorSize*Size);
    }

    return Result;
}


/********************************************************************

    Function:       ReleaseFatEntry

    Description:    Release the cluster ClusterNumber, pointed to by
                    pFatEntry

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 ReleaseFatEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ClusterNumber,
    u8                      **pFatEntry)
{
    u32 Result;

    Result=ADI_DEV_RESULT_SUCCESS;

    /* If pFatEntry is set then release the Fat Sector */
    if (*pFatEntry!=NULL)
    {
        Result=adi_fsd_cache_Release(
            pVolume->FatCacheHandle,
            *pFatEntry);

        *pFatEntry=NULL;
    }

    return Result;
}


/********************************************************************

    Function:       WriteFatEntry

    Description:    Write the Fat sector that ClusterNumber is in
                    back. pCluster is reset to NULL

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 WriteFatEntry(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ClusterNumber,
    u8                      **pFatEntry)
{
    u32 Result;

    Result=ADI_DEV_RESULT_SUCCESS;

    if (pVolume->ReadOnly==TRUE)
    {
        Result=ADI_DEV_RESULT_FAILED;
        ReleaseFatEntry(pVolume,ClusterNumber,pFatEntry);
    }

    /* If pFatEntry is set then Write the Fat Sector back */
    if (*pFatEntry!=NULL)
    {
        Result=adi_fsd_cache_Write(
            pVolume->FatCacheHandle,
            *pFatEntry);

        *pFatEntry=NULL;
    }

    return Result;
}













/* ************************************************
 * ************************************************
 *            MISCELLANEOUS OPERATIONS
 * ************************************************
 * ************************************************
 */
/********************************************************************

    Function:       ProcessBufferChain

    Description:    Process a chain of ADI_FSS_SUPER_BUFFERs,
                    assigning LBA requests to each buffer

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 ProcessBufferChain(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_SUPER_BUFFER    *pBuffer,
    u32                     ReadFlag)
{
    u32 Result;
    ADI_FSS_LBA_REQUEST     *pChain;
    ADI_FSS_SUPER_BUFFER    *pBuf;
    ADI_FSS_SUPER_BUFFER    *pPrevBuf;
    ADI_FSS_SUPER_BUFFER    *pFirstBuffer;

    u32 Block,Cluster,SeekPosition;
    ADI_FAT_FILE_DESCRIPTOR *pFile;


    Result=pBuffer?ADI_DEV_RESULT_SUCCESS:ADI_DEV_RESULT_FAILED;

    pChain=NULL;

    /* If the PID supports caching then we try to do a cache transfer, if it
        doesn't, or if the transfer blocks are not aligned to clusters then
        we use the FSD directory cache to fetch file clusters and manually
        copy the data across
    */
    Block=pVolume->PID_supports_background_transfer;

    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        Cluster=ADI_FAT_START_OF_FILE;
        pFile=(ADI_FAT_FILE_DESCRIPTOR *)((ADI_FSS_FILE_DESCRIPTOR *)(pBuffer->pFileDesc))->FSD_data_handle;

        if (pFile)
        {
            /* Save the seek position */
            SeekPosition=pFile->SeekPosition;
        }
        else
        {
            Result=ADI_DEV_RESULT_FAILED;
        }
    }

    /* Initialise the buffer chain, this includes reseting the LBA request
        structure and the FSD callpack parameters
    */
    pBuf = pBuffer;
    pFirstBuffer = pBuf;
    
    while (Result==ADI_DEV_RESULT_SUCCESS && pBuf)
    {
        /* Initialise the sector count */
        pBuf->LBARequest.StartSector    = 0;
        pBuf->LBARequest.SectorCount    = 0;
        pBuf->LBARequest.DeviceNumber   = pVolume->DeviceNumber;
        pBuf->LBARequest.ReadFlag       = ReadFlag;
        pBuf->LastInProcessFlag         = false;

        /* Set callback function field */
        pBuf->FSDCallbackFunction       = NULL;
        pBuf->FSDCallbackHandle         = NULL;
        pBuf->SemaphoreHandle           = pFirstBuffer->SemaphoreHandle;
        
        /* copy pNext to outer structure in case the chain is
         * split into separate DRQ units
        */
        SET_NEXT_LBA_BUFFER(pBuf, NULL);
        
        /* Move to next in Chain */
        pBuf = (ADI_FSS_SUPER_BUFFER*)pBuf->Buffer.pNext;
    }


    /* Iterate Cluster chain adding StartSector and SectorCount, this loop
        also checks for adjoining clusters and joins them into one single
        transfer to reduce the reads/write calls.
    */
    pBuf = pBuffer;
    pPrevBuf = NULL;
    while (Result==ADI_DEV_RESULT_SUCCESS && pBuf)
    {
        /* Do a file seek to make sure the file position is set to the seek
            position
        */
        Result=Seek(
            pVolume,
            ReadFlag,
            pBuffer->pFileDesc);

        if (Result!=ADI_DEV_RESULT_SUCCESS)
        {
            break;
        }

        /* If this is a FSS Cache transfer then setup the LBA request only */
        if (Block)
        {
            /* Check that the transfer is a valid FSS cache transfer, if
                is not then we need to switch over to a non block transfer
            */
            if ((pBuf->Buffer.ElementWidth*pBuf->Buffer.ElementCount)%
                (pVolume->ClusterSize*pVolume->SectorSize)==0)
            {
                /* If we get to the end of file abort transfer */
                if (pFile->File.Cluster==ADI_FAT_LAST_CLUSTER)
                {
                    Result = ADI_DEV_RESULT_FAILED;
                }

                /* Assign LBA request structure */
                pBuf->LBARequest.StartSector = GetClusterSectorNumber(pVolume,pFile->File.Cluster,0);

                if (pBuf->LBARequest.StartSector == (pFirstBuffer->LBARequest.StartSector + pFirstBuffer->LBARequest.SectorCount))
                {
                    /* Combine request with the current contiguous request */
                    pFirstBuffer->LBARequest.SectorCount += (pBuf->Buffer.ElementWidth*pBuf->Buffer.ElementCount)/pVolume->SectorSize;
                    
                    if (pBuf!=pFirstBuffer) {
                        pFirstBuffer->Buffer.ElementCount += pBuf->Buffer.ElementCount;
                    }        
                }
                else
                {
                    /* Start a new DRQ unit 
                    */
                    SET_NEXT_LBA_BUFFER(pFirstBuffer, pBuf);

                    pPrevBuf = pFirstBuffer;
                    
                    /* set new top buffer for the DRQ unit */
                    pFirstBuffer = pBuf;
                    
                    pFirstBuffer->LBARequest.SectorCount = (pBuf->Buffer.ElementWidth*pBuf->Buffer.ElementCount)/pVolume->SectorSize;
                    
                    /* point the last DRQ unit to the next one */
                    
                    /* set previous buffer as last in DRQ unit */
                    pPrevBuf->Buffer.pNext          = NULL;
                    pPrevBuf->FSDCallbackFunction   = Callback;
                    pPrevBuf->FSDCallbackHandle     = (void*)pVolume;
                    pPrevBuf->pFileDesc             = pBuffer->pFileDesc;
                    
                }

                /* set BARequest.pBuffer to first buffer for LBA request */
                pBuf->LBARequest.pBuffer = pFirstBuffer;
                
                /* Offset seek position by buffer size */
                pFile->SeekPosition+=pBuf->Buffer.ElementWidth*pBuf->Buffer.ElementCount;

                /* Move to next buffer in Chain */
                pPrevBuf = pBuf;
                pBuf=(ADI_FSS_SUPER_BUFFER*)pBuf->Buffer.pNext;
            }
            /* We cannot transfer to/from the buffers directly, so we have
                to switch over to the slower non-block transfer
            */
            else
            {
                /* Reset the buffer pointer back to the start of the chain
                    and set the block transfer flag to false
                */
                pBuf=pBuffer;
                Block=FALSE;
                pFile->SeekPosition=SeekPosition;
            }
        }

        /* Otherwise this is a manual transfer */
        else
        {
            u8 *pData;
            u32 Size,BytesLeft;

            BytesLeft=(pBuf->Buffer.ElementCount-pBuf->Buffer.ProcessedElementCount)*pBuf->Buffer.ElementWidth;
            Size=pVolume->ClusterSize*pVolume->SectorSize-pFile->File.Offset;

            if (Size>BytesLeft)
            {
                Size=BytesLeft;
            }


            /* We use the directory cluster functions to read the file in
                a non-block transfer mode
            */
            if (pFile->File.Cluster==ADI_FAT_START_OF_FILE)
            {
                if (pFile->FirstCluster==ADI_FAT_START_OF_FILE)
                {
                    pFile->File.Cluster=ADI_FAT_ROOT_CLUSTER;
                }
                else
                {
                    pFile->File.Cluster=pFile->FirstCluster;
                }

                pFile->File.Offset=0;
            }

            if (pVolume->Type==ADI_FAT_TYPE_FAT32 &&
                pFile->File.Cluster==ADI_FAT_ROOT_CLUSTER)
            {
                pFile->File.Cluster=pVolume->RootDirAddress;
            }

            /* Read in cluster data */
            if (ReadFlag)
            {
                Result=adi_fsd_cache_Read(
                        pVolume->DirCacheHandle,
                        GetClusterSectorNumber(
                            pVolume,
                            pFile->File.Cluster,
                            pFile->File.Offset),
                        pVolume->ClusterSize,
                        &pData);
            } else {
                Result=adi_fsd_cache_Allocate(
                        pVolume->DirCacheHandle,
                        GetClusterSectorNumber(
                            pVolume,
                            pFile->File.Cluster,
                            pFile->File.Offset),
                        pVolume->ClusterSize,
                        &pData);
            }
            
            pData+=(pFile->File.Offset)%(pVolume->ClusterSize*pVolume->SectorSize);


            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                if (ReadFlag)
                {
                    /* Copy the cluster data into the buffer chain */
                    memcpy(
                        (u8 *)pBuf->Buffer.Data+(pBuf->Buffer.ProcessedElementCount*pBuf->Buffer.ElementWidth),
                        pData,
                        Size);

                    /* Release the cluster */
                    Result=adi_fsd_cache_Release(
                        pVolume->DirCacheHandle,
                        pData);
                }
                else
                {
                    /* Copy the buffer data into the cluster and write the
                        data back to the cluster.
                    */
                    memcpy(
                        pData,
                        (u8 *)pBuf->Buffer.Data+(pBuf->Buffer.ProcessedElementCount*pBuf->Buffer.ElementWidth),
                        Size);

                    /* Write the initialised cluster back to the drive */
                    Result=adi_fsd_cache_Write(
                        pVolume->DirCacheHandle,
                        pData);
                }
            }

            /* If read/write was successful then update the buffer
                arguments to reflect the change. Also update the
                file offset position. If the Offset overflows the
                cluster size offset is reset to zero. This causes
                the next cluster address to be read next time around
                the loop
            */
            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                pFile->SeekPosition+=Size;

                pBuf->Buffer.ProcessedElementCount+=Size/pBuf->Buffer.ElementWidth;
                if (pBuf->Buffer.ProcessedElementCount==pBuf->Buffer.ElementCount)
                {
                    pBuf->Buffer.ProcessedFlag=TRUE;

                    /* Move to next buffer in Chain */
                    pBuf=(ADI_FSS_SUPER_BUFFER*)pBuf->Buffer.pNext;
                }
            }
        }
    }

    /* If we have successfully setup a block mode transfer then submit the
        block mode buffer chain to the PID
    */
    if (Result==ADI_DEV_RESULT_SUCCESS && Block)
    {
        pFirstBuffer->LastInProcessFlag = true;

        /* the last one in the chain has to be last in DRQ, so set values accordingly */
        pFirstBuffer->Buffer.pNext          = NULL;
        pFirstBuffer->FSDCallbackFunction   = Callback;
        pFirstBuffer->FSDCallbackHandle     = (void*)pVolume;
        pFirstBuffer->pFileDesc             = pBuffer->pFileDesc;

        /* Terminate the LBA chain */
        SET_NEXT_LBA_BUFFER(pFirstBuffer, NULL);
        
        /* Submit buffers to the PID one at a time 
         * (pending on completion each time is more robust and does not impact
         * performance.
        */
        for (pBuf = pBuffer; (Result == ADI_DEV_RESULT_SUCCESS && pBuf); pBuf = GET_NEXT_LBA_BUFFER(pBuf))
        {
            _adi_fss_PIDTransfer(pVolume->PIDHandle, pBuf, PDD_NOBLOCK);
            Result = adi_sem_Pend(pVolume->DataSemaphoreHandle, pVolume->SemTimeOutArg);
            adi_dev_Control( pVolume->PIDHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
        }        
    }

    return Result;
}

/********************************************************************

    Function:       Seek

    Description:    Seek to a file cluster:offset

********************************************************************/
__ADI_FAT_SECTION_CODE
u32 Seek(
    ADI_FAT_VOLUME          *pVolume,
    u32                     ReadFlag,
    ADI_FSS_FILE_DESCRIPTOR *pFileDesc)
{
    u32 Result,ClusterSize,SeekCluster,CurrentCluster;
    ADI_FAT_FILE_DESCRIPTOR *pFile;


    Result=ADI_DEV_RESULT_SUCCESS;
    pFile=(ADI_FAT_FILE_DESCRIPTOR*)pFileDesc->FSD_data_handle;


    if (pFile)
    {
        ClusterSize=pVolume->ClusterSize*pVolume->SectorSize;
        SeekCluster=pFile->SeekPosition/ClusterSize;
        CurrentCluster=pFile->Position/ClusterSize;

        /* If the seek cluster doesnt equal the current cluster */
        if (CurrentCluster!=SeekCluster || pFile->File.Cluster==ADI_FAT_START_OF_FILE)
        {
            u32 Index,Offset,Cluster;


            /* Do an absolute seek if the seek position cluster is less than current
                position
            */
            if (SeekCluster<CurrentCluster)
            {
                /* Reset file position */
                Index=0;
                Cluster=ADI_FAT_START_OF_FILE;
            }

            /* Do a relative seek if the seek position cluster is greater than current
                position
            */
            else
            {
                Index=pFile->Position;
                Cluster=pFile->File.Cluster;
            }

            /* Offset in file = NumClusters*NumBytesPerCluster + pSeekDef->offset.
               Calculate the cluster offset within the file
            */

            /* Recurse the FAT table for the number of full clusters */
            while (Result==ADI_DEV_RESULT_SUCCESS &&
                (Index<SeekCluster*ClusterSize || Cluster==ADI_FAT_START_OF_FILE))
            {
                if (Cluster!=ADI_FAT_START_OF_FILE)
                {
                    Index+=ClusterSize;
                }

                if (IsWriteAccess(pFileDesc)==ADI_DEV_RESULT_SUCCESS)
                {
                    Result=AddDirCluster(
                        pVolume,
                        pFile->Entry.Cluster,
                        pFile->Entry.Offset,
                        &pFile->FirstCluster,
                        &Cluster,
                        pVolume->PID_supports_background_transfer?FALSE:TRUE);
                }
                else
                {
                    Result=GetDirCluster(
                        pVolume,
                        pFile->FirstCluster,
                        Cluster,
                        &Cluster);
                }
            }

            if (Result==ADI_DEV_RESULT_SUCCESS && Cluster==ADI_FAT_LAST_CLUSTER)
            {
                Result=ADI_DEV_RESULT_FAILED;
            }

            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                /* Update the file cluster */
                pFile->File.Cluster=Cluster;
            }
        }


        /* Update the file position on a successful seek and file offset */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            pFile->Position=pFile->SeekPosition;
            pFile->File.Offset=pFile->Position%ClusterSize;
        }
    }

    return Result;
}

/********************************************************************

    Function:       Callback

    Description:    The callback to be taken upon buffer completion
                    and device interrupt callbacks.
                    (It is only used for File Cache block transfers).

********************************************************************/
__ADI_FAT_SECTION_CODE
void Callback(void *pHandle, u32 Event, void *pArg)
{
    ADI_FAT_VOLUME *pVolume;
    ADI_FSS_SUPER_BUFFER *pBuffer;
    ADI_FSS_SUPER_BUFFER *pFirstBuffer;
    int i;

    pVolume      = (ADI_FAT_VOLUME *)pHandle;
    pBuffer      = (ADI_FSS_SUPER_BUFFER *)pArg;
    pFirstBuffer = (ADI_FSS_SUPER_BUFFER *)pBuffer->LBARequest.pBuffer;

    if ((pFirstBuffer->CompletionBitMask&(~0xADF50000)) == 3)
    {
        /* Post the semaphore */
        adi_sem_Post ( pVolume->DataSemaphoreHandle );
    }
}

/********************************************************************

    Function:       ValidateNameChar

    Description:    Determines whether the given unicode character is
                    a valid character for either a short file name
                    or a long filename, and indicates which it detected.
                    A char that is valid for a short filename is also
                    valid for a long filename.  The converse is not
                    true.

                    Chars not allowed (either LFN or SFN):
                    * control codes 0 - 1f
                    * del (0x7f)
                    * any of " * / : < > ? \ |

                    Chars not allowed in a short name are the above +
                    * space (? - contentious)
                    * any of + , ; = [ ]
                    * unicode char with code point > 255

********************************************************************/
// LFN disallowed chars start at offset 7.
// Space must always be at the start since disallowing it is contentious
// (XP assumes the file is LFN if a space is present, so for maximum
// compatibility, this does not allow spaces in a short name)
// ... we can skip it (and allow spaces) by offsetting by 1
u8 InvalidNameChars[] = " +,;=[]\"*/:<>?\\|";

__ADI_FAT_SECTION_CODE
ADI_FAT_NAMETYPE ValidateNameChar(ADI_FSS_WCHAR ch)
{
    u8 *pBadCh = InvalidNameChars;
    ADI_FAT_NAMETYPE eNameType = ADI_FAT_SFN;           // assume valid SFN until determined otherwise

    // check for a unicode char (top 8-bits in use)...
    if (ch & 0xFF00)
        return ADI_FAT_LFN;                             // Done: must be a long filename

    if (iscntrl(ch))
        return ADI_FAT_BAD_FILENAME;                    // Done: control chars not allowed

    // check if character is any of the disallowed chars...
    for (;*pBadCh; pBadCh++)
        if (ch == *pBadCh)                              // is this a restricted char?
            // if so determine whether invalid for either LFN or SFN, or only invalid for SFN
            if (pBadCh >= &InvalidNameChars[7])
                return ADI_FAT_BAD_FILENAME;            // Done: char invalid for both SFN and LFN
            else
                eNameType = ADI_FAT_LFN;                // char is not valid for SFN, so change type to LFN

    return eNameType;                                   // Done: valid short or long name
}

/********************************************************************

    Function:       ValidateName

    Description:    Returns ADI_DEV_RESULT_SUCCESS if name array is
                    valid for either a short file name (8.3) or a
                    long filename

                    Returns ADI_FAT_SFN if valid for a short filename
                    Returns ADI_FAT_LFN if valid for a long filename
                    Returns ADI_FAT_BAD_FILENAME if not valid

                    A filename is a valid short name if:
                    * there is only one '.'
                    * there are 0 < x <=8 chars before the '.' (basename)
                    * there are <=3 chars after the '.'(extension)
                    * case is not mixed within the basename or extension
                    * there are no unicode chars present
                    * no chars that are not allowed in 8.3 name

                    A valid long filename if:
                    * no chars that are not allowed in an LFN
                    * length is <= 255 unicode chars

    This function does not modify the source string

********************************************************************/
__ADI_FAT_SECTION_CODE
ADI_FAT_NAMETYPE ValidateName(ADI_FSS_WCHAR *pName)
{
    ADI_FSS_WCHAR ch;
    u32 i, nBaseLen = 0, nNumUpperCase = 0, nNumLowerCase = 0;
    ADI_FAT_NAMETYPE eType = ADI_FAT_SFN;   // assume name is short name unless determined otherwise

    // while name is valid and we have chars remaining ...
    for (i = 0; (ch = *(pName+i)) != '\0' && eType != ADI_FAT_BAD_FILENAME; i++)
    {
        // look for a dot if this is still a valid short name
        // (if this is a long name, then the dot is treated like any other char)
        if (ch == '.' && eType == ADI_FAT_SFN)
        {
            // dot found, so check the validity of the basename
            if (nBaseLen ||                             // more than one dot
                (nNumUpperCase && nNumLowerCase) ||     // mixed case
                (i == 0 || i > 8))                      // base missing or too long
#if defined(__ADI_FAT_LFN_ENABLE)
                    eType = ADI_FAT_LFN;                // must be a long filename
#else
                    // LFN not supported, so must be a bad filename
                    return ADI_FAT_BAD_FILENAME;        // Done: invalid filename for SFN
#endif
            nNumUpperCase = nNumLowerCase = 0;          // reset case markers for extension
            nBaseLen = i;                               // current count is the base length
        }
        else
        {
            // validate the character in the name...
            ADI_FAT_NAMETYPE eCharType = ValidateNameChar(ch);
            if ((eCharType == ADI_FAT_BAD_FILENAME))
               return eCharType;                        // Done: invalid char for both LFN and SFN
            // character is invalid for an SFN, so indicate LFN name detected
            if (eCharType == ADI_FAT_LFN && eType == ADI_FAT_SFN)
#if defined(__ADI_FAT_LFN_ENABLE)
                eType = ADI_FAT_LFN;
#else
                // LFN not supported, so invalid filename
                return ADI_FAT_BAD_FILENAME;            // Done: Invalid filename for SFN
#endif

            // count case to detect mixed case usage...
            nNumUpperCase += isupper(ch);
            nNumLowerCase += islower(ch);
        }
    }

    if (eType == ADI_FAT_BAD_FILENAME || !i)
        return ADI_FAT_BAD_FILENAME;                    // Done: invalid name detected

    if (eType == ADI_FAT_SFN)
    {
        // file is possibly Short Name ...
        // this is checking the extension, or the basename in the case when no
        // extension is present.
        // NOTE: missing base is checked in the loop above.  If nBaseLen is 0
        // here, then that means there is no extension, and i is the baselength
        if (((nBaseLen && (i-nBaseLen-1) <= 3) ||       // extension length OK (base present)
             (!nBaseLen && (i <= 8))) &&                // base length OK (no extension)
             !(nNumUpperCase && nNumLowerCase))         // not mixed case
            return ADI_FAT_SFN;                         // Done: valid 8.3 filename...
    }

    // if we get here, then the name must be a possible LFN...
#if defined(__ADI_FAT_LFN_ENABLE)
    if (i <= 255)
        return ADI_FAT_LFN;                             // Done: length is good: valid LFN
#endif

    return ADI_FAT_BAD_FILENAME;                        // Done: LFN not supported or name is too long
}

#if defined(__ADI_FAT_LFN_ENABLE)
/* Functions only needed when LFN support is enabled */

/********************************************************************

    Function:       GetShortEntryFromLFN

    Description:    Generates a short file entry from the supplied
                    unicode filename.

                    pShortEntry must point to an array of
                    ADI_FAT_SHORT_ENTRY_SIZE characters

    NOTE: The string returned will always be uppercased
          In addition, trailing dots are removed from the supplied
          long filename

********************************************************************/
u8 HexLookup[] = "0123456789ABCDEF";

__ADI_FAT_SECTION_CODE
u32 GetShortEntryFromLFN(
    ADI_FAT_VOLUME          *pVolume,
    u32                     EntryCluster,
    ADI_FSS_WCHAR           *pFileName,
    ADI_FSS_WCHAR           *pNameEnd,
    u8                      *pShortEntry)
{
    u8 i;
    u8 j = 0;               // index into basis portion of short entry
    u8 k = 8;               // index into extension portion of short entry
    u8 bTailRequired = 0;   // tail required if chars lost, or name doesn't fit into 8.3 format
    u8 bExtFound = 0;       // indicates an extension (and hence an embedded dot) has been found
    u32 nTailValue;         // numeric tail value
    u32 nChars;             // number of chars required for the numeric tail

    ADI_FSS_WCHAR *pLFN = pFileName;
    // location of the extension (if any) within the LFN (start the search from the end of the filename)
    ADI_FSS_WCHAR *pExt = pNameEnd;

    while (*pLFN == '.') pLFN++;                        // strip leading periods
    bTailRequired = (pLFN != pFileName);                // indicate characters were lost (tail required)

    // find the location of the last embedded '.' (the position of the extension is the next char)
    while (pExt != pLFN && *pExt != '.')
        pExt--;

    if (pExt == pLFN) pExt = pNameEnd;                  // no extension found, so set marker to end
    else
    {
        // dot found ...
        if (pExt == pNameEnd-1)
        {
            // trailing . found ...
            // NOTE: in the following loop, pExt cannot become pLFN since that would imply the
            // name is all dots and the strip leading periods check above would account fot this case.
            // so there has to be at least one non-dot char between pLFN and pExt
            while (*pExt == '.') pExt--;                // ignore all trailing dots
            *(++pExt) = '\0';                           // truncate at the last dot
            pNameEnd = pExt;                            // update the end of string marker
        }
        else
        {
            bExtFound = 1;                              // embedded dot / extension found
        }
    }

    // determine if the long name fits within the 8.3 allocation (i.e. base is <=8 chars and ext <= 3 chars)
    // (bExtFound also indicates the presence of a . that must be accounted for)
    if (pExt - pLFN > 8 || pNameEnd - pExt - bExtFound > 3)
        bTailRequired = 1;

    // create a basis name from the LFN...
    // NOTE: at this point pExt must point to either the '.' before the extension, or the end of the string
    for (j = 0; *pLFN != '\0' && pLFN != pExt && j < 8; pLFN++)
    {
        ADI_FSS_WCHAR ch = *pLFN;
        if (ch == ' ' || ch == '.')
        {
            bTailRequired = 1;
            continue;                                   // skip space or dot (not allowed in basis name)
        }
        ch = toupper(ch);                               // uppercase the character
        if (ValidateNameChar(ch) == ADI_FAT_LFN)
        {
            // character is not valid for an 8.3 format name, so replace with a '_'
            pShortEntry[j++] = '_';
            bTailRequired = 1;                          // indicate original character was lost
        }
        else
            // at this point the char is an 8-bit (OEM) entity ...
            pShortEntry[j++] = ch;                      // copy the char into the basis name
    }

    if (bExtFound)
    {
        // extension is present, so copy at most 3 chars of it into the short entry
        pExt++;                                         // skip the dot to point to the first extension char
        for (;*pExt != '\0' && k < ADI_FAT_SHORT_ENTRY_SIZE; pExt++)
        {
            ADI_FSS_WCHAR ch = *pExt;

            if (ch == ' ')
            {
                bTailRequired = 1;
                continue;                               // skip space
            }
            ch = toupper(ch);                           // uppercase the character
            if (ValidateNameChar(ch) == ADI_FAT_LFN)
            {
                // character is not valid for an 8.3 format name, so replace with a '_'
                pShortEntry[k++] = '_';
                bTailRequired = 1;                      // indicate original character was lost
            }
            else
                // at this point the ch is an 8-bit (OEM) entity...
                pShortEntry[k++] = ch;                  // copy the char into the extension name
        }
    }

    for (; k < ADI_FAT_SHORT_ENTRY_SIZE; k++)
        pShortEntry[k] = ' ';                           // pad rest of extension with spaces

    // Note: do not modify j here since we need it later to indicate the start of any numeric tail
    for (k = j;k < 8; k++)
        pShortEntry[k] = ' ';                           // pad rest of base with spaces

    // determine if a numeric tail is required ...
    if (!bTailRequired)
    {
        // NOTE: this would occur if mixed case was used on an otherwise valid 8.3 name, or an invalid
        // char was found in an otherwise valid 8.3 name

        // see if the proposed short name collides with any existing name
        if (!MatchShortEntry(pVolume, EntryCluster, (char *)pShortEntry))
            return ADI_DEV_RESULT_SUCCESS;

        // else we need a tail due to the collision...
        bTailRequired = 1;
    }

    // else, a tail is required....
    // NOTES: here bTailRequired indicates a collision with an existing name
    // also, j is the index into the short entry where the tail should be written
    // minimum number of chars required for the tail is 2 (1 for the '~')
    for (nTailValue = 1, nChars = 2; bTailRequired; nTailValue++)
    {
        int nShift;
        u8 nChars;
        u8 nBits;

        // find the number of chars needed by the tail...
        // NOTE: for the following nTailValue can be guaranteed to not be either 0 or all ones
        // (since the loop starts at 1 and we run out of chars before it reaches anywhere near
        // the upper bound).
        asm("%0 = signbits %1;": "=l" (nBits): "d" (nTailValue));
        // +1 for the '~'
        nChars = 1 + (((31 - nBits) + 3) >> 2);

        // if total chars required for the tail including the ~, and the minimum 1 char for the name
        // doesn't fit in 8 chars, then we are done...
        if (nChars + 1 > 8)
            return ADI_DEV_RESULT_FAILED;               // it will never fit (1 char min required in the name)

        if (nChars + j > 8)
            j = 8 - nChars;                             // shrink j to fit

        // Note: writing the tail should not modify j, since if we need to go back to write a new tail,
        // we want to start at the same location...
        k = j;
        pShortEntry[k++] = '~';
        // convert index into hex character string
        for (nShift = (nChars - 2) << 2; nShift >= 0; nShift -= 4)
        {
            // lookup nchar and write at j location
            pShortEntry[k++] = HexLookup[(nTailValue >> nShift) & 0xF];
        }

        // see if the proposed short name collides with any existing name
        bTailRequired = MatchShortEntry(pVolume, EntryCluster, (char *)pShortEntry);
    }
    // NOTE: we get here if a valid filename is found
    return ADI_DEV_RESULT_SUCCESS;

}

/********************************************************************

    Function:       MatchShortEntry
    
    Description:    Scans thru directory entries to find a match against
                    the specified short entry.
                    Returns true if a match found, else false.

    Note:           This is more efficient than doing a FindDirEntry, which 
                    accesses and returns the long entries, which are not 
                    required for a short entry match

********************************************************************/   
u32 MatchShortEntry(
    ADI_FAT_VOLUME *pVolume,
    u32             ParentCluster,
    char           *pShortEntry)
{
    u32 bFound = 0;                             // matching entry not found yet
    u32 Result = ADI_DEV_RESULT_FAILED;
    ADI_FAT_DIR_ENTRY *pEntry = NULL;           // the current entry
    u32 Cluster = ADI_FAT_START_OF_FILE;        // start searching from start of file
    u32 Offset = 0;

    do
    {
        Result = ReadDirEntry(pVolume, ParentCluster, &Cluster, &Offset, &pEntry);
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            // process the entry ...
            if (pEntry->Name[0] == 0x00)
                break;
            if (pEntry->Name[0] != 0xE5 && IsValidDirAttribute(pEntry->Attr) && (!(IsLongDirEntry(pEntry->Attr))))
            {
                // valid short entry, so compare the name
                bFound = (strncmp(pEntry->Name, pShortEntry, ADI_FAT_SHORT_ENTRY_SIZE) == 0);
            }
            Result = ReadNextDirEntry(pVolume, &Cluster, &Offset, &pEntry, ADI_FAT_CLUSTER_TYPE_READ);
        }
    } while (!bFound && Result == ADI_DEV_RESULT_SUCCESS);

    ReleaseDirEntry(pVolume, &pEntry);

    return bFound;
}

/********************************************************************

    Function:


    Description:    Calculate Check Sum from Short Directory Entry
                    Name.

********************************************************************/
__ADI_FAT_SECTION_CODE
u8 ShortEntryCheckSum(u8 *pName)
{
    u8 Index,Sum;

    for (Index=0,Sum=0;Index<ADI_FAT_SHORT_ENTRY_SIZE;Index++)
    {
        Sum=(((Sum&1)?0x80:0)|(Sum>>1))+pName[Index];
    }

    return Sum;
}

#endif


/********************************************************************

    Function:       IsValidDirAttribute

    Description:    Returns TRUE is directory attribute is valid.

********************************************************************/
__ADI_FAT_SECTION_CODE
u8 IsValidDirAttribute(u8 Attribute)
{
    return  IsFileOrDirEntry(Attribute);
}


/********************************************************************

    Function:       IsWriteAccess

    Description:

********************************************************************/
__ADI_FAT_SECTION_CODE
u8 IsWriteAccess(ADI_FSS_FILE_DESCRIPTOR *pFileDef)
{
    return (
        (pFileDef->mode & ADI_FSS_MODE_WRITE)==ADI_FSS_MODE_WRITE ||
        (pFileDef->mode & ADI_FSS_MODE_READ_WRITE)==ADI_FSS_MODE_READ_WRITE ||
        (pFileDef->mode & ADI_FSS_MODE_APPEND)==ADI_FSS_MODE_APPEND ||
        (pFileDef->mode & ADI_FSS_MODE_CREATE)==ADI_FSS_MODE_CREATE)?
        ADI_DEV_RESULT_SUCCESS:ADI_DEV_RESULT_FAILED;
}















/******************************************************************************

    FAT Format Functions

******************************************************************************/


#if defined(ADI_USE_FAT_FORMAT)

#include <drivers/pid/adi_ata.h>

#define HPC         256
#define SPT         63
#define OEMNAME     "MSWIN4.1"

/* ************************************************
 * Tables for FAT formatting
 * ************************************************
 */

typedef struct {
    u32 VolumeSize;
    u32 ClusterSize;
} ADI_FAT_SIZE_DEF;

static ADI_FAT_SIZE_DEF FatClusterSize16[] = {
    /* < 4.1MB */ {    8400,    0 },      /* zero SecsPerClus indicates error */
    /* - 16MB  */ {   32680,    2 },
    /* - 128MB */ {  262144,    4 },
    /* - 256MB */ {  524288,    8 },
    /* - 512MB */ { 1048576,   16 },
    /* The next two entries are only to be used if the user forces FAT16 */
    /* - 1GB   */ { 2097152,   32 },
    /* - 2GB   */ { 4194304,   64 },
                  { 0xFFFFFFFF, 0 }    /* zero SecsPerClus indicates error */
};

static ADI_FAT_SIZE_DEF FatClusterSize32[] = {
    /* < 32.5MB */ {    66600,    0 },      /* zero SecsPerClus indicates error */
    /* - 260MB  */ {   532480,    1 },
    /* - 8GB    */ { 16777216,    8 },
    /* - 16GB   */ { 33554432,   16 },
    /* - 2TB    */ { 0xFFFFFFFF, 32 }
};


static u32 FormatVolume(
    ADI_FAT_VOLUME          *pVolume,
    u32                     DeviceNumber,
    u32                     Type,
    ADI_FSS_WCHAR           *Label,
    u32                     StartAddress,
    u32                     VolumeSize,
    u32                     SectorSize,
    u32                     SinglePartitionFlag
);

static u32 CreateBPB(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSD_CACHE_HANDLE    CacheHandle,
    u32                     StartAddress,
    ADI_FSS_WCHAR           *Label
);

static u32 UpdatePartitionTable(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSD_CACHE_HANDLE    CacheHandle,
    u32                     StartAddress,
    u32                     SinglePartitionFlag
);


/********************************************************************

    Function:       Format

    Description:    Generic format command

********************************************************************/
__ADI_FAT_FORMAT_SECTION_CODE
u32 Format(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSS_FORMAT_DEF      *pFormatDef)
{
    u32 Result;
    ADI_FSS_VOLUME_DEF VolumeDef;
    u32 SinglePartitionFlag;


    SinglePartitionFlag=FALSE;

    /* Get the overall volume sizes from the PID */
    VolumeDef=pFormatDef->VolumeDef;

    Result=adi_dev_Control(
        pVolume->PIDHandle,
        ADI_PID_CMD_GET_GLOBAL_MEDIA_DEF,
        &VolumeDef);

    /* If this call fails (media may need resettting?!) abort */

    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        u32 i;
        ADI_FSS_WCHAR Label[12];

        /* Set the format request size to max volume size if not set or too
            large
        */
        if (pFormatDef->VolumeDef.VolumeSize==0 ||
            pFormatDef->VolumeDef.VolumeSize>VolumeDef.VolumeSize)
        {
            pFormatDef->VolumeDef.VolumeSize=VolumeDef.VolumeSize;
            SinglePartitionFlag=TRUE;
        }

        /* Initialise the volume label to a NULL terminated ADI_FSS_WCHAR */
        memset(Label,0,sizeof(Label));
        for (i=0;i<pFormatDef->label_len && i<(sizeof(Label)/sizeof(Label[0]))-1;i++)
        {
            Label[i]=pFormatDef->label[i];
        }

        /* Now format the volume */
        Result=FormatVolume(
            pVolume,
            pFormatDef->VolumeDef.DeviceNumber,
            ADI_FSS_FMT_OPTION_GET_TYPE(pFormatDef->OptionMask),
            Label,
            pFormatDef->VolumeDef.StartAddress,
            pFormatDef->VolumeDef.VolumeSize,
            pFormatDef->VolumeDef.SectorSize,
            SinglePartitionFlag);
    }

    return Result;
}


/*********************************************************************

    Function:       FormatVolume

    Description:    Formats the volume

*********************************************************************/
__ADI_FAT_FORMAT_SECTION_CODE
u32 FormatVolume(
    ADI_FAT_VOLUME          *pVolume,
    u32                     DeviceNumber,
    u32                     Type,
    ADI_FSS_WCHAR           *pLabel,
    u32                     StartAddress,
    u32                     VolumeSize,
    u32                     SectorSize,
    u32                     SinglePartitionFlag)
{
    u32 Result,Index;
    ADI_FSD_CACHE_HANDLE CacheHandle;


    /* Initialise the Volume */
    pVolume->DeviceNumber=DeviceNumber;
    pVolume->Type=Type;
    pVolume->VolumeSize=VolumeSize;
    pVolume->SectorSize=SectorSize;

    /* Initialise the format sector cache */
    Result=adi_fsd_cache_Open(
        pVolume->PIDHandle,
        pVolume->DeviceNumber,
        &CacheHandle,
        0,
        pVolume->FatCacheSize,
        1,
        pVolume->SectorSize,
        pVolume->DataElementWidth,
        (u32)PDD_DEFAULT_HEAP,
        pVolume->CacheHeapID,
        0,
        0,
        pVolume->DataSemaphoreHandle);

    /* Create a BPB based on the information in pVolume and the extra info
        supplied
    */
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        Result=CreateBPB(
            pVolume,
            CacheHandle,
            StartAddress,
            pLabel);
    }

    /* Write FS Info sector and the first FAT sector */
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        ADI_FAT_BOOT_SECTOR_FAT32 *pBPB;


        /* Read BPB into cache */
        Result=adi_fsd_cache_Read(
            CacheHandle,
            StartAddress,
            1,
            (u8 **)&pBPB);

        /* Now setup pVolume by reading the Boot Sector and BPB, this sector
            should have been setup correctly by the CreateBPB function
        */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            Result=ReadBootSector(
                pVolume,
                StartAddress,
                (u8 *)pBPB);
        }

        /* Write FSINFO sector to BPB Sector + BPB_FSInfo */
        if (Result==ADI_DEV_RESULT_SUCCESS && pVolume->Type==ADI_FAT_TYPE_FAT32 && pBPB->BPB_FSInfo)
        {
            ADI_FAT_FSINFO_SECTOR_FAT32 *pInfo;

            /* Read BPB into cache */
            Result=adi_fsd_cache_Allocate(
                CacheHandle,
                StartAddress+pBPB->BPB_FSInfo,
                1,
                (u8 **)&pInfo);

            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                memset(pInfo,0,pVolume->SectorSize);

                pInfo->ExtendedBootSignature    = 0x41615252;   /* Lead signature     */
                pInfo->FSInfoSignature          = 0x61417272;   /* Struc Sig          */
                pInfo->FreeClusterCount         = 0xFFFFFFFF;   /* Free cluster count */
                pInfo->NextFreeCluster          = 2;            /* Free cluster hint  */
                pInfo->FSInfoEndSignature       = 0xAA550000;   /* Trailing Signature */

                Result=adi_fsd_cache_Write(
                    CacheHandle,
                    (u8 *)pInfo);
            }
            else
            {
                adi_fsd_cache_Release(
                    CacheHandle,
                    (u8 *)pInfo);
            }
        }

        /* Write FSINFO sector to BPB Sector + BPB_BkBootSec + 1 */
        if (Result==ADI_DEV_RESULT_SUCCESS && pVolume->Type==ADI_FAT_TYPE_FAT32 && pBPB->BPB_FSInfo)
        {
            ADI_FAT_FSINFO_SECTOR_FAT32 *pInfo;

            /* Read BPB into cache */
            Result=adi_fsd_cache_Allocate(
                CacheHandle,
                StartAddress+pBPB->BPB_BkBootSec+1,
                1,
                (u8 **)&pInfo);

            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                memset(pInfo,0,pVolume->SectorSize);

                pInfo->ExtendedBootSignature    = 0x41615252;   /* Lead signature     */
                pInfo->FSInfoSignature          = 0x61417272;   /* Struc Sig          */
                pInfo->FreeClusterCount         = 0xFFFFFFFF;   /* Free cluster count */
                pInfo->NextFreeCluster          = 2;            /* Free cluster hint  */
                pInfo->FSInfoEndSignature       = 0xAA550000;   /* Trailing Signature */

                Result=adi_fsd_cache_Write(
                    CacheHandle,
                    (u8 *)pInfo);
            }
            else
            {
                adi_fsd_cache_Release(
                    CacheHandle,
                    (u8 *)pInfo);
            }
        }

        /* Create and write the first FAT sector */
        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            u8 *pFat;

            /* Read BPB into cache */
            Result=adi_fsd_cache_Allocate(
                CacheHandle,
                pVolume->FatAddress,
                1,
                &pFat);

            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                memset(pFat,0,pVolume->SectorSize);

                switch (pVolume->Type)
                {
                    case ADI_FAT_TYPE_FAT16:
                        ((u16 *)pFat)[0]=0xFF00 | pBPB->BPB_Media;
                        ((u16 *)pFat)[1]=0xBFFF;
                        break;

                    case ADI_FAT_TYPE_FAT32:
                        ((u32 *)pFat)[0]=0xFFFFFF00 | pBPB->BPB_Media;
                        ((u32 *)pFat)[1]=0xFBFFFFFF;
                        ((u32 *)pFat)[2]=ADI_FAT_LAST_CLUSTER;
                        break;
                }

                Result=adi_fsd_cache_Write(
                    CacheHandle,
                    (u8 *)pFat);
            }
            else
            {
                adi_fsd_cache_Release(
                    CacheHandle,
                    (u8 *)pFat);
            }
        }

        /* Clear all FAT entries except the first */
        for (Index=1;
            Result==ADI_DEV_RESULT_SUCCESS && Index<pVolume->FatSize;
            Index++)
        {
            u8 *pFat;

            Result=adi_fsd_cache_Allocate(
                CacheHandle,
                pVolume->FatAddress+Index,
                1,
                &pFat);

            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                memset(pFat,0,pVolume->SectorSize);

                Result=adi_fsd_cache_Write(
                    CacheHandle,
                    pFat);
            }
            else
            {
                adi_fsd_cache_Release(
                    CacheHandle,
                    pFat);
            }
        }

        /* Duplicate the FAT into the successive copy location, the number of
            copies is founf in the BPB as NumFATs */
        for (Index=0;
            Result==ADI_DEV_RESULT_SUCCESS && Index<pVolume->FatSize;
            Index++)
        {
            u8 *pFat;
            u32 Fat;

            Result=adi_fsd_cache_Read(
                CacheHandle,
                pVolume->FatAddress+Index,
                1,
                &pFat);

            for (Fat=1;Result==ADI_DEV_RESULT_SUCCESS && Fat<pBPB->BPB_NumFATs;Fat++)
            {
                u8 *pFatCopy;

                Result=adi_fsd_cache_Allocate(
                    CacheHandle,
                    pVolume->FatAddress+(Fat*pVolume->FatSize)+Index,
                    1,
                    &pFatCopy);

                if (Result==ADI_DEV_RESULT_SUCCESS)
                {
                    memcpy(pFatCopy,pFat,pVolume->SectorSize);

                    adi_fsd_cache_Write(
                        CacheHandle,
                        pFatCopy);
                }
                else
                {
                    adi_fsd_cache_Release(
                        CacheHandle,
                        pFatCopy);
                }
            }

            adi_fsd_cache_Release(
                CacheHandle,
                pFat);
        }
        Result=adi_fsd_cache_Release(
            CacheHandle,
            (u8 *)pBPB);
    }

    /* Clear the root directory and add the volume label root directory
        entry. For FAT32 media the root directory is one cluster, for FAT12
        and FAT16 the root directory is a fixed size and is located just
        before the Data clusters
    */
    for (Index=0;
        Result==ADI_DEV_RESULT_SUCCESS &&
        Index<((pVolume->Type==ADI_FAT_TYPE_FAT32)?pVolume->ClusterSize:pVolume->DataAddress-pVolume->RootDirAddress);
        Index++)
    {
        u8 *pData;

        Result=adi_fsd_cache_Allocate(
            CacheHandle,
            pVolume->RootDirAddress+Index+((pVolume->Type==ADI_FAT_TYPE_FAT32)?pVolume->DataAddress-2:0),
            1,
            &pData);

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            memset(pData,0,pVolume->SectorSize);

            /* Add the volume lable entry to the first sector only */
            if (Index==0)
            {
                u8 i,Name[12];

                for (i=0;i<sizeof(Name)-1 && pLabel[i];i++)
                {
                    Name[i]=pLabel[i];
                }
                Name[i]='\0';

                Result=CreateDirEntry(
                    pVolume,
                    (ADI_FAT_DIR_ENTRY *)pData,
                    Name,
                    ADI_FAT_DIR_VOLUME_LABEL,
                    ADI_FAT_ROOT_CLUSTER,
                    0);
            }
        }

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            Result=adi_fsd_cache_Write(
                CacheHandle,
                pData);
        }
        else
        {
            adi_fsd_cache_Release(
                CacheHandle,
                pData);
        }
    }

    /* Adjust partition table entry */
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        Result=UpdatePartitionTable(
            pVolume,
            CacheHandle,
            StartAddress,
            SinglePartitionFlag);
    }

    /* We are finished, if everything worked then Result will equal
        ADI_DEV_RESULT_SUCCESS. We close the cache with TRUE in the flush
        flag so that the changes will be written to the PID, if something
        went wrong then we close the PID with FALSE so that nothing is
        flushed to the PID and if enough cache was allocated the media will
        not have been modified.
    */
    adi_fsd_cache_Close(CacheHandle,Result==ADI_DEV_RESULT_SUCCESS?TRUE:FALSE);

    return Result;
}


/********************************************************************

    Function:       CreateBPB

    Description:    Calculates the fat size and initialises the
                    BPB for a volume

********************************************************************/
__ADI_FAT_FORMAT_SECTION_CODE
u32 CreateBPB(
    ADI_FAT_VOLUME          *pVolume,       /* pVolume pointer */
    ADI_FSD_CACHE_HANDLE    CacheHandle,
    u32                     StartAddress,
    ADI_FSS_WCHAR           *pLabel)
{
    u32 Result,Index,i,j;
    u32 RootEntryCount,ReservedSectorCount,FixedFlag;
    u32 MaxCluster,BlockSize;
    u32 Type,Reserved,FatSize,RootEntry,Clusters;

    Result=ADI_DEV_RESULT_FAILED;
    pVolume->ClusterSize=0;

    BlockSize=pVolume->SectorSize;

    Type=ADI_FAT_TYPE_NONE;
    Reserved=0;
    FatSize=0;
    RootEntry=0;
    Clusters=0;

    /* Volume size calculation algorithm
        We loop through the FAT types/cluster sizes and calculate the best
        option in terms of prefered FAT type and volume size. If a FAT type
        /cluster size is valid for the volume it is then compared with the
        currently selected FAT type/cluster size and the best match for
        prefered FAT type/cluster size is kept. The loop is broken once a
        prefered match is found
    */

    /* Cycle through FAT12, FAT16 and FAT32 */
    for (i=0;i<3;i++)
    {
        /* Cycle through the sectors per cluster varients */
        for (j=1;j<7;j++)
        {
            u32 TmpReserved,TmpFatSize,TmpRootEntry,TmpClusters;

            if (Type!=ADI_FAT_TYPE_NONE)
            {
            }
        }
    }

    /* We can only deal with FAT16 an FAT32 at the moment */

    if (pVolume->Type==ADI_FAT_TYPE_FAT12) {
        /* FAT12 Volumes are not supported */
        Result = ADI_FSS_RESULT_BAD_VOLUME;
    }
    else
    {
        /* Look for match in FAT16 table */
        for (Index=0;
            Index<8 && pVolume->VolumeSize>FatClusterSize16[Index].VolumeSize;
            Index++);

        RootEntryCount=512;
        ReservedSectorCount=1;

        if (Index==0 && FatClusterSize16[Index].ClusterSize==0 )
        {
            /* Volumes less than 4.1MB are not supported */
            Result = ADI_FSS_RESULT_BAD_VOLUME;
        }
        else if (Index<2)
        {
            /* FAT 16 is only option */
            pVolume->ClusterSize=FatClusterSize16[Index].ClusterSize;

            Result=ADI_DEV_RESULT_SUCCESS;
            pVolume->Type=ADI_FAT_TYPE_FAT16;
        }
        else if (Index<5)
        {
            pVolume->ClusterSize=FatClusterSize16[Index].ClusterSize;

            /* FAT 16 should be used if not forced */
            if (pVolume->Type==ADI_FAT_TYPE_FAT32)
            {
                Index=7;
            }
            else
            {
                Result=ADI_DEV_RESULT_SUCCESS;
                pVolume->Type=ADI_FAT_TYPE_FAT16;
            }
        }
        else if (Index<7 || pVolume->Type==ADI_FAT_TYPE_FAT16)
        {
            Index=7;

            /* FAT 32 should be used if not forced */
            if (pVolume->Type==ADI_FAT_TYPE_FAT16)
            {
                Index=6;

                Result=ADI_DEV_RESULT_SUCCESS;
                if (pVolume->VolumeSize>FatClusterSize16[Index].VolumeSize)
                {
                    pVolume->VolumeSize=FatClusterSize16[Index].VolumeSize;
                }
                pVolume->ClusterSize=FatClusterSize16[Index].ClusterSize;
            }
        }

        /* Disk Size or User Selection indicates FAT32 */
        if (Index==7 && pVolume->Type!=ADI_FAT_TYPE_FAT16)
        {
            RootEntryCount=0;
            ReservedSectorCount=32;

            if (pVolume->VolumeSize>=FatClusterSize32[4].VolumeSize)
            {
                pVolume->VolumeSize=FatClusterSize32[4].VolumeSize;
                pVolume->ClusterSize=FatClusterSize32[4].ClusterSize;
            }
            else
            {
                for (Index=0;Index<5;Index++)
                {
                    if (pVolume->VolumeSize<=FatClusterSize32[Index].VolumeSize)
                    {
                        pVolume->ClusterSize=FatClusterSize32[Index].ClusterSize;
                        break;
                    }
                }
            }

            if (pVolume->ClusterSize)
            {
                Result=ADI_DEV_RESULT_SUCCESS;
                pVolume->Type=ADI_FAT_TYPE_FAT32;
            }
        }
    }


    /* Check if the media is fixed */
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        Result=adi_dev_Control(
            pVolume->PIDHandle,
            ADI_PID_CMD_GET_FIXED,
            (void*)&FixedFlag);
    }

    /* Read in the BPB for the volume */
    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        ADI_FAT_BOOT_SECTOR_START *pStart;
        ADI_FAT_BOOT_SECTOR_MIDDLE *pMiddle;
        ADI_FAT_BOOT_SECTOR_END *pEnd;


        /* Read BPB into cache */
        Result=adi_fsd_cache_Read(
            CacheHandle,
            StartAddress,
            1,
            (u8 **)&pStart);

        if (Result==ADI_DEV_RESULT_SUCCESS)
        {
            char *pString,i;
            u32 TmpVal1,TmpVal2,FatSize;

            /* Setup pointers to the middle and end of the BS/BPB depending
                on the FAT type
            */
            if (pVolume->Type==ADI_FAT_TYPE_FAT16)
            {
                pMiddle=NULL;
                pEnd=(ADI_FAT_BOOT_SECTOR_END *)(((u8*)pStart)+sizeof(*pStart));
            }
            else
            {
                pMiddle=(ADI_FAT_BOOT_SECTOR_MIDDLE *)(((u8*)pStart)+sizeof(*pStart));
                pEnd=(ADI_FAT_BOOT_SECTOR_END *)(((u8*)pMiddle)+sizeof(*pMiddle));
            }

            /* Now apply the calculation as set out in the Microsoft White Paper */
            TmpVal1=pVolume->VolumeSize-
                ReservedSectorCount-
                (((RootEntryCount*32)+(pVolume->SectorSize-1))/pVolume->SectorSize);
            TmpVal2=((pVolume->SectorSize/sizeof(u16))*pVolume->ClusterSize)+2;

            if (pVolume->Type==ADI_FAT_TYPE_FAT32)
            {
                TmpVal2/=2;
            }

            FatSize=(TmpVal1+(TmpVal2-1))/TmpVal2;

            /* For MS Windows to recognize the media, this must be set to either
             * 0xEB or 0xE9. 0xEB is more common. It does not matter about the jump
             * address
            */
            pStart->BS_jmpBoot.instr = 0xEB;

            pString=OEMNAME;
            memset(pStart->BS_OEMName,' ',sizeof(pStart->BS_OEMName));
            for (i=0;i<sizeof(pStart->BS_OEMName) && pString[i];i++)
            {
                pStart->BS_OEMName[i]=pString[i];
            }

            pStart->BPB_BytsPerSec  = pVolume->SectorSize;
            pStart->BPB_SecPerClus  = pVolume->ClusterSize;
            pStart->BPB_RsvdSecCnt  = ReservedSectorCount;
            pStart->BPB_NumFATs     = 2;
            pStart->BPB_RootEntCnt  = RootEntryCount;

            pStart->BPB_TotSec16    = pVolume->VolumeSize<0x10000?pVolume->VolumeSize:0;
            pStart->BPB_TotSec32    = pVolume->VolumeSize>=0x10000?pVolume->VolumeSize:0;

            /* Fixed or removable media */
            pStart->BPB_Media       = FixedFlag?0xF8:0xF0;

            pStart->BPB_FATSz16     = 0;

            /* Geometry settings - set to zero as IRQ 13 not used */
            pStart->BPB_SecPerTrk   = SPT;
            pStart->BPB_NumHeads    = HPC-1;

            /* Hidden sectors (same as start sector) */
            pStart->BPB_HiddenSec   = StartAddress;

            switch (pVolume->Type)
            {
                case ADI_FAT_TYPE_FAT12:
                    pString="FAT12";
                    break;

                case ADI_FAT_TYPE_FAT16:
                    pString="FAT16";
                    pStart->BPB_FATSz16     = FatSize;
                    break;

                case ADI_FAT_TYPE_FAT32:
                    pString="FAT32";
                    pMiddle->BPB_FATSz32    = FatSize;
                    pMiddle->BPB_ExtFlags   = 0;
                    pMiddle->BPB_FSVer      = 0;
                    pMiddle->BPB_RootClus   = 2;
                    pMiddle->BPB_FSInfo     = 1;
                    pMiddle->BPB_BkBootSec  = 6;
            }

            pEnd->BS_DrvNum         = 0x80;
            pEnd->BS_Reserved1      = 0;
            pEnd->BS_BootSig        = 0x29;
            pEnd->BS_VolID          = 0;

            memset(pEnd->BS_VolLab,' ',sizeof(pEnd->BS_VolLab));
            for (i=0;i<sizeof(pEnd->BS_VolLab) && pLabel[i];i++)
            {
                pEnd->BS_VolLab[i]=pLabel[i];
            }

            memset(pEnd->BS_FilSysType,' ',sizeof(pEnd->BS_FilSysType));
            for (i=0;i<sizeof(pEnd->BS_FilSysType) && pString[i];i++)
            {
                pEnd->BS_FilSysType[i]=pString[i];
            }

            ((u16 *)pStart)[255]=0xAA55;

            /* Write updated BPB sector back to media */
            Result=adi_fsd_cache_Write(
                CacheHandle,
                (u8 *)pStart);
        }
        else
        {
            /* There was an error so release BPB sector */
            adi_fsd_cache_Release(
                CacheHandle,
                (u8 *)pStart);
        }
    }

    return Result;
}


/********************************************************************

    Function:       UpdatePartitionTable

    Description:    Updates the primary partition table. If
                    SinglePartitionFlag is set then the partition
                    table is cleared and one entry is added.

********************************************************************/
__ADI_FAT_FORMAT_SECTION_CODE
u32 UpdatePartitionTable(
    ADI_FAT_VOLUME          *pVolume,
    ADI_FSD_CACHE_HANDLE    CacheHandle,
    u32                     StartAddress,
    u32                     SinglePartitionFlag)
{
    u32 Result;
    u8 *pMBR;


    Result=adi_fsd_cache_Read(
        CacheHandle,
        0,
        1,
        &pMBR);

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        u32 Index;
        ADI_ATA_PARTITION_ENTRY Table[4];
        u32 MaxCylinder;


        memcpy(Table,pMBR+446,sizeof(Table));
        for (Index=SinglePartitionFlag?4:0;
            Index<4 && Table[Index].lba_start!=StartAddress;
            Index++);

        if (Index==4)
        {
            memset(Table,0,sizeof(Table));
            Index=0;
        }

        /*  LBA = ((Cylinder * HPC + Head) * SPT) + Sector - 1

            Cylinder        = LBA / (HPC * SPT)
            Head            = (LBA % (HPC * SPT)) / SPT
            Sector          = ((LBA % (HPC * SPT)) % SPT) + 1

            where
            HPC             = Heads per cylinder (assume 256)
            SPT             = Sectors per track (assume 63)

            dh              = Head
            cl(0:5)         = Sector
            ch(0:7):cl(6:7) = Cylinder
        */

        /* Starting CHS coordinates */
        Table[Index].dh_start  = (StartAddress % (HPC * SPT)) / SPT;
        Table[Index].cl_start  = ((((StartAddress % (HPC * SPT)) % SPT) + 1)&0x3F) |
                                    (((StartAddress / (HPC * SPT))>>2)&0xC0);
        Table[Index].ch_start  = StartAddress / (HPC * SPT);

        /* Ending CHS coordinates */
        Table[Index].dh_end    = ((StartAddress+pVolume->VolumeSize) % (HPC * SPT)) / SPT;
        Table[Index].cl_end    = (((((StartAddress+pVolume->VolumeSize) % (HPC * SPT)) % SPT) + 1)&0x3F) |
                                    ((((StartAddress+pVolume->VolumeSize) / (HPC * SPT))>>2)&0xC0);
        Table[Index].ch_end    = (StartAddress+pVolume->VolumeSize) / (HPC * SPT);

        /* Partition Type */
        Table[Index].type      = ADI_ATA_VOLUME_TYPE_PRI_FAT32_LBA;

        /* Partition LBA Start Address */
        Table[Index].lba_start = StartAddress;

        /* Partition size in LBA sectors */
        Table[Index].size      = pVolume->VolumeSize;

        memcpy(pMBR+446,Table,sizeof(Table));
    }

    /* Add signature to end of MBR */
    pMBR[510] = 0x55;
    pMBR[511] = 0xAA;

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {
        /* XXX - Write backup boot sector for FAT32 */
        /*if ( pVolume->Type==ADI_FAT_TYPE_FAT32 )
        {
            char *pBackup;

            Result=adi_fsd_cache_Allocate(
                pVolume->FatCacheHandle,
                pVolumeDef->StartAddress+pVolume->bpb.large.BPB_BkBootSec,
                1,
                &pBackup)

            if (Result==ADI_DEV_RESULT_SUCCESS)
            {
                memcpy(pBackup,pMBR,512);

                Result=adi_fsd_cache_Write(
                    pVolume->FatCacheHandle,
                    pBackup);
            }
        }*/
    }

    if (Result==ADI_DEV_RESULT_SUCCESS)
    {
        /* Write MBR sector back to media */
        Result=adi_fsd_cache_Write(
            CacheHandle,
            pMBR);
    }
    else
    {
        /* There was an error so do not update MBR */
        adi_fsd_cache_Release(
            CacheHandle,
            pMBR);
    }

    return Result;
}


#endif

static bool IsFileEntry( u8 Attribute )
{
    /* A file entry is identified by what it is not: not a subdir, LFN or Volume label. So we create
       a mask of the possible bits that represent a file and clear these bits in the attribute value;
       then if no other bits are set in the atrtribute we can assume it's a file entry
    */
    u8 FileEntryMask = ADI_FAT_DIR_READONLY | ADI_FAT_DIR_HIDDEN | ADI_FAT_DIR_SYSTEM | ADI_FAT_DIR_ARCHIVE;
    bool IsFile = (Attribute & ~FileEntryMask) == 0;
    
    return IsFile;
}

static bool IsFileOrDirEntry( u8 Attribute )
{
    return (IsFileEntry(Attribute) || IS_ATTRIBUTE(Attribute,ADI_FAT_DIR_SUBDIRECTORY));
}

/* End Of File ***************************************************************/


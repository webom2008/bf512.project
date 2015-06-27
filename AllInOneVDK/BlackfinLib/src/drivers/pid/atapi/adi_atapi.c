/*********************************************************************************

Copyright(c) 2007 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_atapi.c,v $
$Revision: 3578 $
$Date: 2010-10-19 09:36:04 -0400 (Tue, 19 Oct 2010) $

Description:
            PID Device driver for ATA/ATAPI Interface on ADSP-BF54x.

********************************************************************************
*/

#ifndef __ADI_ATAPI_C__
#define __ADI_ATAPI_C__

#if !defined(_DEV_PURPOSES_ONLY)
#define _DEV_PURPOSES_ONLY
#endif /* _DEV_PURPOSES_ONLY */
#define __DEPRECATED__ 0

/*********************************************************************

Include files

*********************************************************************/
#include <services/services.h>      /* system services                     */
#include <services/fss/adi_fss.h>   /* File System Service                 */
#include <drivers/pid/adi_ata.h>    /* ATA structures, comamnds and macros */
#include <string.h>                 /* Memory Copy etc.                    */

/* ***************************************************************************
 * Register MMR definititons and bitmask access macros
 * ***************************************************************************
 */
#include "adi_atapi_reg.h"

/* ***************************************************************************
 * Device Driver public header
 * ***************************************************************************
 */
#include <drivers/pid/atapi/adi_atapi.h>

/* the following can be extern'd by another file if required for debug purpose */
//u16 ata_device_status = 0;

/* For clarity */

#ifdef ENABLE
#undef ENABLE
#endif
#define ENABLE 1
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

// For convenience
#ifdef Debug
#undef Debug
#endif
#ifdef ADI_DEV_DEBUG
#define Debug(V)  if ((V)) return (u32)(V)
#else
#define Debug(V)  (V)
#endif

#if 0
#define ADI_ATAPI_STATE_XFER_IN_PROGESS     (1)
#define ADI_ATAPI_STATE_DEV_INT     (2)
#define ADI_ATAPI_STATE_DMA_INT     (4)
#define ADI_ATAPI_STATE_XFER_DONE   (ADI_ATAPI_STATE_DEV_INT|ADI_ATAPI_STATE_DMA_INT|ADI_ATAPI_STATE_XFER_IN_PROGESS)
#endif

enum state_flag_e {
    ADI_ATAPI_STATE_XFER_IDLE       = 0,
    ADI_ATAPI_STATE_XFER_IN_PROGESS = 1,
    ADI_ATAPI_STATE_DEV_INT         = 2,
    ADI_ATAPI_STATE_DMA_INT         = 4,
    ADI_ATAPI_STATE_HDMA_INT        = 8,
    ADI_ATAPI_STATE_XFER_DONE       =   ADI_ATAPI_STATE_HDMA_INT 
                                      | ADI_ATAPI_STATE_DEV_INT
                                      | ADI_ATAPI_STATE_XFER_IN_PROGESS
};



ADI_DCB_CALLBACK_FN FSSCallbackFunction = NULL;

static u32 g_ActivateOncePerOpen = 0;


/* *************************************************************
 * Device Driver instance structure
 * *************************************************************
 */

typedef struct {
    u32 Type;                   /* Type of device detected                                */
    u8  MediaPresent;           /* flag determining whether media is present in drive     */
    u32 TransferSize;           /* Indicates the ata/atapi buffer size                    */
    u16 SectorMax;              /* Number of sectors that can be requested at a time      */
    s32 TransferMode;           /* Transfer mode to use: PIO or DMA (Multi-word or Ultra) */
    u16 max_rw_multiples;       /* Number of sectors that can be requested at a time      */
    u8  PacketInterfaceRequired;/* Flag indicating whether to use ATAPI commands          */
    u8  media_active;           /* flag determining whether media is ready to use         */
    u16 SupportsRemovableMediaFeatureSet;
    u16 SupportsDeviceReset;
    u16 RequiresFeaturesToSpinUp;
    u16 SupportsIORDY;
    u16 CanDisableIORDY;
    u16 CommandSetNotificationSupported;
    u16 FeaturesSupported;
    u16 CommandSetsSupported;
    u16 FeaturesEnabled;
    u16 reg_t2;
    u16 reg_teoc;
    u16 pio_t1;
    u16 pio_t2;
    u16 pio_t4;
    u16 pio_teoc;
    u16 mdma_td;
    u16 mdma_tm;
    u16 mdma_tkw;
    u16 mdma_tkr;
    u16 mdma_th;
    u16 mdma_teoc;
    u16 udma_tack;
    u16 udma_tenv;
    u16 udma_tdvs;
    u16 udma_tcyc_minus_tdvs;
    u16 udma_tss;
    u16 udma_tmli;
    u16 udma_tzah;
    u16 udma_trp;
    u32 FirstPartitionSector;
    u32 LastSectorOnMedia;

} ADI_ATAPI_DEVICE_DEF;


#pragma pack(4)
typedef struct {
    ADI_DEV_DEVICE_HANDLE   DeviceHandle;           /* device manager handle                                  */
    u32                     DeviceNumber;           /* Device Number                                          */
    ADI_DEV_DIRECTION       Direction;              /* Direction of Device Driver                             */
    ADI_DCB_HANDLE          DCBHandle;              /* callback manager handle                                */
    ADI_DCB_CALLBACK_FN     DMCallback;             /* the callback function supplied by the Device Manager   */
    void                    *pEnterCriticalArg;     /* critical region argument                               */
    ADI_FSS_LBA_REQUEST     CurrentLBARequest;      /* The current LBA request being processed                */
    u32                     *pVolumePriority;       /* Pointer to table of volume priorities                  */
    u32                     fcclk;                  /* Core Clock Frequency                                   */
    u32                     fsclk;                  /* System Clock Frequency                                 */
    u32                     NumDrives;              /* Number of drives on chain                              */
    ADI_DMA_PMAP            InboundMappingID;       /* Inbound DMA peripheral mapping ID                      */
    ADI_DMA_PMAP            OutboundMappingID;      /* Outbound DMA peripheral mapping ID                     */
    enum state_flag_e                     StateFlag;              /* State Flag: 2=command started, 1=awaiting completion   */
                                                    /*             0=command completed.                       */
    u32                     ErrorReportingFlag;
    s32                     TimeOutFlag;
    u32                     TimeOutTimerID;
    ADI_INT_PERIPHERAL_ID   DevIntPeripheralID;

    int                     CacheHeapID;            /* Heap Index for Device transfer buffers                 */
    ADI_SEM_HANDLE          DataSemaphoreHandle;    /* Semaphore for Internal data transfers                  */
    ADI_SEM_HANDLE          LockSemaphoreHandle;   /* Semaphore for Lock Semaphore operation                          */
    u16 InterruptMask;                              /* Current ATAPI controller Interrupt Mask                */
    u32 ForceMDMA;
    u32 ForcePIO;
    ADI_ATAPI_DEVICE_DEF Device[2];
    u32 CurrentDrive;
    
} ADI_ATAPI_DEF;

#pragma pack()

/* internal prototypes */

static u32 WaitOnInterrupt(ADI_ATAPI_DEF *pDevice, u16 IntMask, u16 BitPos );
static u32 WaitOnBusy( ADI_ATAPI_DEF *pDevice, u8 bsy );
static u32 PowerOnReset(ADI_ATAPI_DEF *pDevice);
static void SendResetSignal(u32 sense);
static void ProcessInterrupt(ADI_ATAPI_DEF *pDevice, u32 Event, ADI_FSS_SUPER_BUFFER *pBuffer);

static u32 Identify(ADI_ATAPI_DEF *pDevice, u32 Drive);
static u32 DetermineOperationModes( ADI_ATAPI_DEF *pDevice, u32 Drive, u16 *word );
static u32 CalculatePIOTimings(u32 fsclk, u16 mode, ADI_ATAPI_DEVICE_DEF *pDevice);
static u32 CalculateRegTimings(u32 fsclk, u16 mode, ADI_ATAPI_DEVICE_DEF *pDevice);
static u32 CalculateMDMATimings(u32 fsclk, u16 mode, ADI_ATAPI_DEVICE_DEF *pDevice);
static u32 CalculateUDMATimings(u32 fsclk, u16 mode, ADI_ATAPI_DEVICE_DEF *pDevice);
static u32 num_clocks_min(long long tmin, long long fsclk);
static u32 num_clocks_max(long long tmax, long long fsclk);

static void SetPIOTimingRegs( ADI_ATAPI_DEVICE_DEF *pDevice );
static void SetRegXferTimingRegs( ADI_ATAPI_DEVICE_DEF *pDevice );
static void SetMDMATimingRegs( ADI_ATAPI_DEVICE_DEF *pDevice );
static void SetUDMATimingRegs( ADI_ATAPI_DEVICE_DEF *pDevice );
static u32 POSTFunction(ADI_ATAPI_DEF *pDevice, u32 Drive);

static u32 SelectPortPins(u32 EnableFlag);
static void PrepareForCommand(ADI_ATAPI_DEF *pDevice);
static void PrepareForTransfer(ADI_ATAPI_DEF *pDevice);

static void WaitOnTransferComplete(u16 TransferBitMask);

static u32 SelectDrive(ADI_ATAPI_DEF *pDevice, u32 Drive);
static u32 WriteTransferMode(ADI_ATAPI_DEF *pDevice, u32 Drive);
static void SetTimingRegs ( ADI_ATAPI_DEF *pDevice, u32 Drive );
static void ConfigureRegistersForTransfer( ADI_ATAPI_DEF *pDevice, u32 Drive, u16 xlen, u32 ReadFlag);

static u32 TransferSectors(ADI_ATAPI_DEF *pDevice, u32 Drive, u32 SectorNumber, u32 SectorCount, u16 *buf, u32 ReadFlag);
static u32 Transfer(ADI_ATAPI_DEF *pDevice, u32 Drive, u32 ReadFlag, u32 Size, u8 *Data);
static u32 DetectVolumes(ADI_ATAPI_DEF *pDevice, u32 Drive);
static u32 ProcessPartitionTable(ADI_ATAPI_DEF *pDevice, u32 Drive, u32 *Count);
static u32 ProcessExtendedPartitions(ADI_ATAPI_DEF *pDevice, u32 Drive, ADI_ATA_PARTITION_ENTRY *pPartTabEntry, u32 *Count);
static void NewVolumeDef(ADI_ATAPI_DEF *pDevice, u32 Drive, ADI_ATA_PARTITION_ENTRY *pPartTab);
static u32 AssignFileSystemType(u32 type);

static u32
WriteDeviceCommand(
            ADI_ATAPI_DEF *pDevice,         /* ATA/ATAPI Device Instance        */
            u16            Command,         /* ATA command                      */
            u16            DriveNumber,     /* Drive Number 0=Master, 1=Slave   */
            u16            SectorCount,     /* Sector Count register contents   */
            u16            Feature,         /* Feature register comtents        */
            u32            LBA              /* Logical Block Address value      */
);
static void GetGlobalVolumeDef(ADI_ATAPI_DEF *pDevice, ADI_FSS_VOLUME_DEF *pVolumeDef, u32 Drive);

static u32 Activate(ADI_ATAPI_DEF *pDevice,u32 enable);
static u32 SendATACommand ( ADI_ATAPI_DEF *pDevice, ADI_FSS_LBA_REQUEST *pLBARequest );
static u32 DevicePollMedia(ADI_ATAPI_DEF *pDevice);
//static u32 DevicePollMedia(ADI_ATAPI_DEF *pDevice, u32 Drive);
static void SetDataFlow(ADI_ATAPI_DEF *pDevice, u32 EnableFlag);

static u32 WriteDeviceRegister( ADI_ATAPI_DEF *pDevice, u16 RegisterOffset, u16 Value );
static u32 ReadDeviceRegister( ADI_ATAPI_DEF *pDevice, u16 RegisterOffset, u16 *Value );

/* interrupt handler for status/errors - normally only used with PIO (true and with DMA)
 * and Device register access.
*/
static ADI_INT_HANDLER(DeviceInterruptHandler);

/* Data transfer time out callback
*/
static void TimeOutCallback(void *hArg, u32 Event, void *pArg);

static bool WriteAndCheckPattern( ADI_ATAPI_DEF *pDevice, u8 *pattern );
static bool InitializeDevice(ADI_ATAPI_DEF *pDevice, u16 Drive);

/* Enumerators for ATA devices for Master (0), Slave (1) devices on chain */
enum {
    PDD_DEVICE_TYPE_NONE=0,
    PDD_DEVICE_TYPE_UNKNOWN,
    PDD_DEVICE_TYPE_ATA,
    PDD_DEVICE_TYPE_ATAPI,
};

#define PDD_DEFAULT_HEAP            -1

#define PDD_MASTER                  0
#define PDD_SLAVE                   1

#define PDD_DEVICES                 2

#define PDD_DEFAULT_SECTOR_SIZE     512     /* in bytes */

enum {
    PDD_DEVICE_STATE_IDLE=0,                /* device is idle */
    PDD_DEVICE_STATE_ATA_CMD,               /* execute ata command */
    PDD_DEVICE_STATE_ATA_READ,              /* finish reading data from ata device */
    PDD_DEVICE_STATE_ATA_READ_PARTIAL,      /* read in part of the data from an ata device */
    PDD_DEVICE_STATE_ATA_WRITE,             /* finish writing data to ata device */
    PDD_DEVICE_STATE_ATA_WRITE_PARTIAL,     /* write out part of the data to an ata device */
    PDD_DEVICE_STATE_SCSI_CMD,              /* execute a scsi command */
    PDD_DEVICE_STATE_SCSI_READ_CMD,         /* execute a scsi read command */
    PDD_DEVICE_STATE_SCSI_READ,             /* finish reading in data from scsi read command */
    PDD_DEVICE_STATE_SCSI_READ_PARTIAL,     /* read in part of the data from a scsi read command */
    PDD_DEVICE_STATE_SCSI_WRITE_CMD,        /* execute a scsi write command */
    PDD_DEVICE_STATE_SCSI_WRITE,            /* finish writing out scsi data from scsi write command */
    PDD_DEVICE_STATE_SCSI_WRITE_PARTIAL,    /* write out part of the data from a scsi write command */
};

static volatile u32 PDD_State=PDD_DEVICE_STATE_IDLE;

#define ADI_ATAPI_IS_XFER_IN_PROGESS(V)     ( ((V)&ADI_ATAPI_STATE_XFER_IN_PROGESS)==ADI_ATAPI_STATE_XFER_IN_PROGESS )
#define ADI_ATAPI_IS_DEV_INT(V)     ( ((V)&ADI_ATAPI_STATE_DEV_INT)==ADI_ATAPI_STATE_DEV_INT )
#define ADI_ATAPI_IS_DMA_INT(V)     ( ((V)&ADI_ATAPI_STATE_DMA_INT)==ADI_ATAPI_STATE_DMA_INT )
#define ADI_ATAPI_IS_XFER_DONE(V)   ( ((V)&ADI_ATAPI_STATE_XFER_DONE)==ADI_ATAPI_STATE_XFER_DONE )

// temporary global state flag
enum state_flag_e StateFlag = ADI_ATAPI_STATE_XFER_IDLE;
u32 DevIntCount = 0;
u16 DevIntStatus[4] = { 0, 0, 0, 0 };


/* ***************************************************************************
 * LBA Request Queue Management
 * ***************************************************************************
 */

typedef struct ADI_PID_LBA_REQUEST_ENTRY{
    u32 ReadFlag;
    ADI_FSS_LBA_REQUEST LBARequest;
    struct ADI_PID_LBA_REQUEST_ENTRY *pNext;
} ADI_PID_LBA_REQUEST_ENTRY;

static ADI_PID_LBA_REQUEST_ENTRY *pHeadQueuedRequest = NULL;
static ADI_PID_LBA_REQUEST_ENTRY *pTailQueuedRequest = NULL;
static ADI_PID_LBA_REQUEST_ENTRY *pTopQueuedRequest = NULL;
static ADI_PID_LBA_REQUEST_ENTRY *pBottomQueuedRequest = NULL;

static void CallbackFromFSS(void *pHandle, u32 Event, void *pArg);

static u32 pddQueueLBARequest ( ADI_ATAPI_DEF *pDevice, ADI_FSS_LBA_REQUEST *pLBARequest, u32 ReadFlag );
static u32 pddRetrieveLBARequest ( ADI_ATAPI_DEF *pDevice, ADI_PID_LBA_REQUEST_ENTRY *pRequest );


/* ***************************************************************************
 * FSS housekeeping functions
 * ***************************************************************************
 */
extern void *_adi_fss_malloc( int id, size_t size );
extern void _adi_fss_free( int id, void *p );
extern void *_adi_fss_MallocAligned( int id, size_t size );
extern void _adi_fss_FreeAligned( int id, void *p );
extern u32 _adi_fss_PIDTransfer( ADI_DEV_DEVICE_HANDLE PIDHandle, ADI_FSS_SUPER_BUFFER *pBuffer, u32 BlockFlag );
extern void _adi_fss_WaitMicroSec(unsigned int msec);
extern void _adi_fss_WaitMilliSec(unsigned int msec);

extern u32 _adi_fss_DetectVolumes( 
                            ADI_DEV_DEVICE_HANDLE   hDevice, 
                            u32                     Drive,
                            int                     HeapID,
                            u32                     SectorSize,
                            u32                     DataElementWidth,
                            ADI_SEM_HANDLE          hSemaphore
);

extern void _adi_fss_FlushMemory( char *StartAddress, u32 NumBytes, ADI_DMA_WNR Direction);

#define PDD_GEN_HEAPID (-1)

/* ******************************************************************
 * PIO Mode - Frequency compatibility
 * ******************************************************************
 */
/*               mode:       0          1           2           3           4    */
static u32 pio_fsclk[]   = { 33333333,  33333333,   33333333,   33333333,   33333333 };

/* ******************************************************************
 * MDMA Mode - Frequency compatibility
 * ******************************************************************
 */
/*               mode:       0          1           2        */
static u32 mdma_fsclk[]  = { 33333333,  33333333,   33333333 };

/* ******************************************************************
 * UDMA Mode - Frequency compatibility

        UDMA 5 - 100 MB/s  - SCLK   = 133 MHz
        UDMA 4 - 66 MB/s    - SCLK >=  80 MHz
        UDMA3 - 44.4 MB/s  - SCLK >=  50 MHz
        UDMA2 - 33 MB/s     - SCLK >=  40 MHz

 * ******************************************************************
 */
/*               mode:       0          1           2           3           4           5       */
static u32 udma_fsclk[]  = { 33333333,  33333333,   40000000,   50000000,  80000000,  133333333 };

/* ******************************************************************
 * Register transfer timing table
 * ******************************************************************
 */
/*               mode:       0    1    2    3    4    */
static u32 reg_t0min[]   = { 600, 383, 330, 180, 120 };      /* Cycle Time                     */
static u32 reg_t2min[]   = { 290, 290, 290, 70,  25  };      /* DIOR/DIOW to end cycle         */
static u32 reg_teocmin[] = { 290, 290, 290, 80,  70  };      /* DIOR/DIOW asserted pulse width */

/* ******************************************************************
 * PIO timing table
 * ******************************************************************
 */
/*               mode:       0    1    2    3    4    */
static u32 pio_t0min[]   = { 600, 383, 240, 180, 120 };      /* Cycle Time                     */
static u32 pio_t1min[]   = { 70,  50,  30,  30,  25  };      /* Address valid to DIOR/DIORW    */
static u32 pio_t2min[]   = { 165, 125, 100, 80,  70  };      /* DIOR/DIOW to end cycle         */
static u32 pio_teocmin[] = { 165, 125, 100, 70,  25  };      /* DIOR/DIOW asserted pulse width */
static u32 pio_t4min[]   = { 30,  20,  15,  10,  10  };      /* DIOW data hold                 */

/* ******************************************************************
 * Multiword DMA timing table
 * ******************************************************************
 */
/*               mode:       0   1    2        */
static u32 mdma_t0min[]  = { 480, 150, 120 };        /* Cycle Time                     */
static u32 mdma_tdmin[]  = { 215, 80,  70  };        /* DIOR/DIOW asserted pulse width */
static u32 mdma_thmin[]  = { 20,  15,  10  };        /* DMACK to read data released    */
static u32 mdma_tjmin[]  = { 20,  5,   5   };        /* DIOR/DIOW to DMACK hold        */
static u32 mdma_tkrmin[] = { 50,  50,  25  };        /* DIOR negated pulse width       */
static u32 mdma_tkwmin[] = { 215, 50,  25  };        /* DIOR negated pulse width       */
static u32 mdma_tmmin[]  = { 50,  30,  25  };        /* CS[1:0] valid to DIOR/DIOW     */
static u32 mdma_tzmax[]  = { 20,  25,  25  };        /* DMACK to read data released    */

/* ******************************************************************
 * Ultra DMA timing table
 * ******************************************************************
 */
/*               mode:         0    1    2    3    4    5       */
static u32 udma_tcycmin[]  = { 112, 73,  54,  39,  25,  17 };      /* Cycle Time                     */
static u32 udma_tdvsmin[]  = { 70,  48,  31,  20,  7,   5  };      /* */
static u32 udma_tenvmax[]  = { 70,  70,  70,  55,  55,  50 };      /* */
static u32 udma_trpmin[]   = { 160, 125, 100, 100, 100, 85 };      /* */
static u32 udma_tmin[]     = { 5,   5,   5,   5,   3,   3  };      /* */


static u32 udma_tmlimin = 20;
static u32 udma_tzahmin = 20;
static u32 udma_tenvmin = 20;
static u32 udma_tackmin = 20;
static u32 udma_tssmin = 50;

#ifdef _DEV_PURPOSES_ONLY /* For development purposes only */
u32 CurrentTransferMode;
#endif


/* ***************************************************************************
 * Device Driver Model entry point functions
 * ***************************************************************************
 */
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

static u32 adi_pdd_Close
(       // Closes a device
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

/* ***************************************************************************
 * Device Driver Model entry point structure
 * ***************************************************************************
 */
ADI_DEV_PDD_ENTRY_POINT ADI_ATAPI_EntryPoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control
};

/* ***************************************************************************
 * Bit field access macros for packing mode data into 32 bit Device[Drive].TransferMode field
 * ***************************************************************************
 */

#define ADI_ATA_TRANSFER_MODE_MASK                  0x0000000F
#define ADI_ATA_TRANSFER_MODE_SHIFT                 0
#define ADI_ATA_TRANSFER_MODE_SET(R,V)              ( (R) = ( (V) << ADI_ATA_TRANSFER_MODE_SHIFT ) | ( (R) & ~ADI_ATA_TRANSFER_MODE_MASK ) )
#define ADI_ATA_TRANSFER_MODE_GET(V)                ( ( (V) & ADI_ATA_TRANSFER_MODE_MASK ) >> ADI_ATA_TRANSFER_MODE_SHIFT )

#define ADI_ATA_TRANSFER_TYPE_MASK                  0x000000F0
#define ADI_ATA_TRANSFER_TYPE_SHIFT                 4
#define ADI_ATA_TRANSFER_TYPE_SET(R,V)              ( (R) = ( (V) << ADI_ATA_TRANSFER_TYPE_SHIFT ) | ( (R) & ~ADI_ATA_TRANSFER_TYPE_MASK ) )
#define ADI_ATA_TRANSFER_TYPE_GET(V)                ( ( (V) & ADI_ATA_TRANSFER_TYPE_MASK ) >> ADI_ATA_TRANSFER_TYPE_SHIFT )

#define ADI_ATA_MAX_DEV_PIO_MODE_MASK               0x00000F00
#define ADI_ATA_MAX_DEV_PIO_MODE_SHIFT              8
#define ADI_ATA_MAX_DEV_PIO_MODE_SET(R,V)           ( (R) = ( (V) << ADI_ATA_MAX_DEV_PIO_MODE_SHIFT ) | ( (R) & ~ADI_ATA_MAX_DEV_PIO_MODE_MASK ) )
#define ADI_ATA_MAX_DEV_PIO_MODE_GET(V)             ( ( (V) & ADI_ATA_MAX_DEV_PIO_MODE_MASK ) >> ADI_ATA_MAX_DEV_PIO_MODE_SHIFT )

#define ADI_ATA_MAX_DEV_MDMA_MODE_MASK              0x0000F000
#define ADI_ATA_MAX_DEV_MDMA_MODE_SHIFT             12
#define ADI_ATA_MAX_DEV_MDMA_MODE_SET(R,V)          ( (R) = ( (V) << ADI_ATA_MAX_DEV_MDMA_MODE_SHIFT ) | ( (R) & ~ADI_ATA_MAX_DEV_MDMA_MODE_MASK ) )
#define ADI_ATA_MAX_DEV_MDMA_MODE_GET(V)            ( ( (V) & ADI_ATA_MAX_DEV_MDMA_MODE_MASK ) >> ADI_ATA_MAX_DEV_MDMA_MODE_SHIFT )

#define ADI_ATA_MAX_DEV_UDMA_MODE_MASK              0x000F0000
#define ADI_ATA_MAX_DEV_UDMA_MODE_SHIFT             16
#define ADI_ATA_MAX_DEV_UDMA_MODE_SET(R,V)          ( (R) = ( (V) << ADI_ATA_MAX_DEV_UDMA_MODE_SHIFT ) | ( (R) & ~ADI_ATA_MAX_DEV_UDMA_MODE_MASK ) )
#define ADI_ATA_MAX_DEV_UDMA_MODE_GET(V)            ( ( (V) & ADI_ATA_MAX_DEV_UDMA_MODE_MASK ) >> ADI_ATA_MAX_DEV_UDMA_MODE_SHIFT )

#define ADI_ATA_MAX_CUR_PIO_MODE_MASK               0x00F00000
#define ADI_ATA_MAX_CUR_PIO_MODE_SHIFT              20
#define ADI_ATA_MAX_CUR_PIO_MODE_SET(R,V)           ( (R) = ( (V) << ADI_ATA_MAX_CUR_PIO_MODE_SHIFT ) | ( (R) & ~ADI_ATA_MAX_CUR_PIO_MODE_MASK ) )
#define ADI_ATA_MAX_CUR_PIO_MODE_GET(V)             ( ( (V) & ADI_ATA_MAX_CUR_PIO_MODE_MASK ) >> ADI_ATA_MAX_CUR_PIO_MODE_SHIFT )

#define ADI_ATA_MAX_CUR_MDMA_MODE_MASK              0x0F000000
#define ADI_ATA_MAX_CUR_MDMA_MODE_SHIFT             24
#define ADI_ATA_MAX_CUR_MDMA_MODE_SET(R,V)          ( (R) = ( (V) << ADI_ATA_MAX_CUR_MDMA_MODE_SHIFT ) | ( (R) & ~ADI_ATA_MAX_CUR_MDMA_MODE_MASK ) )
#define ADI_ATA_MAX_CUR_MDMA_MODE_GET(V)            ( ( (V) & ADI_ATA_MAX_CUR_MDMA_MODE_MASK ) >> ADI_ATA_MAX_CUR_MDMA_MODE_SHIFT )

#define ADI_ATA_MAX_CUR_UDMA_MODE_MASK              0xF0000000
#define ADI_ATA_MAX_CUR_UDMA_MODE_SHIFT             28
#define ADI_ATA_MAX_CUR_UDMA_MODE_SET(R,V)          ( (R) = ( (V) << ADI_ATA_MAX_CUR_UDMA_MODE_SHIFT ) | ( (R) & ~ADI_ATA_MAX_CUR_UDMA_MODE_MASK ) )
#define ADI_ATA_MAX_CUR_UDMA_MODE_GET(V)            ( ( (V) & ADI_ATA_MAX_CUR_UDMA_MODE_MASK ) >> ADI_ATA_MAX_CUR_UDMA_MODE_SHIFT )

#define ADI_ATA_MODE_PIO     0x1
#define ADI_ATA_MODE_MDMA    0x4
#define ADI_ATA_MODE_UDMA    0x8

#define ADI_ATA_MODE_UNSET   0x7

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
*   Description:    Opens an ATAPI Physical Interface Device driver for use
*
*********************************************************************/

static u32 adi_pdd_Open(                    /* Open a device */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,          /* device manager handle            */
    u32                     DeviceNumber,           /* device number                    */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,           /* Device Manager device handle     */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,            /* pointer to PDD handle location   */
    ADI_DEV_DIRECTION       Direction,              /* data direction                   */
    void                    *pEnterCriticalArg,     /* enter critical region parameter  */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,              /* handle to the DMA manager        */
    ADI_DCB_HANDLE          DCBHandle,              /* callback handle                  */
    ADI_DCB_CALLBACK_FN     DMCallback              /* client callback function         */
)
{
    u16 i;
    u32 fvco;
    u32 Result= ADI_DEV_RESULT_SUCCESS;
    u32 DevIntIVG;

    /* Create an instance of the device driver */
    ADI_ATAPI_DEF *pDevice = (ADI_ATAPI_DEF*)_adi_fss_malloc(PDD_GEN_HEAPID,sizeof(ADI_ATAPI_DEF));
    if (!pDevice) {
        Result = ADI_FSS_RESULT_NO_MEMORY;
    }
    else
    {
        /* initialize the Device Header structure */
        pDevice->Direction = Direction;
        pDevice->DCBHandle = DCBHandle;
        pDevice->DMCallback = DMCallback;
        pDevice->DeviceHandle = DeviceHandle;
        pDevice->DeviceNumber = DeviceNumber;
        pDevice->pEnterCriticalArg = pEnterCriticalArg;

        pDevice->InterruptMask = 0;
        pDevice->InboundMappingID = ADI_DMA_PMAP_ATAPI_RX;
        pDevice->OutboundMappingID = ADI_DMA_PMAP_ATAPI_TX;
        pDevice->ErrorReportingFlag = FALSE;
        pDevice->DevIntPeripheralID = ADI_INT_ATAPI_ERROR;

        pDevice->DataSemaphoreHandle = NULL;
        pDevice->LockSemaphoreHandle = NULL;

        /* Media is inactive on initialization */
        for (i=0;i<sizeof(pDevice->Device)/sizeof(pDevice->Device[0]);i++)
        {
            memset(&pDevice->Device[i],0, sizeof(ADI_ATAPI_DEVICE_DEF));
            pDevice->Device[i].Type         = PDD_DEVICE_TYPE_NONE;
            pDevice->Device[i].media_active = false;
            pDevice->Device[i].MediaPresent = 0;
            pDevice->Device[i].TransferSize = PDD_DEFAULT_SECTOR_SIZE;
            pDevice->Device[i].SectorMax    = 1;
        }
        pDevice->CurrentDrive = 0;

        /* Default condition is to adopt the fastest mode possible */
        pDevice->ForcePIO = FALSE;
        pDevice->ForceMDMA = FALSE;

        /* Use the FSS General Heap for data buffers by default */
        pDevice->CacheHeapID = -1;

        /* The default is for one Device attached to chain */
        pDevice->NumDrives = 1;

        /* Initialize State Flag */
        pDevice->StateFlag = ADI_ATAPI_STATE_XFER_IDLE;

        /* save the physical device handle in the client supplied location */
        *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;

        /* initialise the volume prioritisation table */
        pDevice->pVolumePriority=
            (u32*)_adi_fss_malloc(PDD_GEN_HEAPID,sizeof(u32)*ADI_FSS_PID_VOLUME_END);

        for (i=0;
            i<ADI_FSS_PID_VOLUME_END&&pDevice->pVolumePriority!=NULL;
            i++)
        {
            pDevice->pVolumePriority[i]=ADI_FSS_PID_VOLUME_END;
        }

        /* Find the current SCLK frequency */
        /* default to slowest (50MHz) if call fails */
        if ( adi_pwr_GetFreq(&pDevice->fcclk,&pDevice->fsclk,&fvco)!= ADI_PWR_RESULT_SUCCESS )
        {
            pDevice->fsclk = 50000000;
        }
        /* factor value bu 1,000,000 if value is expressed in MHz */
        if (pDevice->fsclk<1000000) {
            pDevice->fsclk *= 1000000;
        }

        adi_int_SICGetIVG(pDevice->DevIntPeripheralID, &DevIntIVG);
        /* hook the DEV_INT interrupt handler into the system */
        if(adi_int_CECHook(DevIntIVG, DeviceInterruptHandler, pDevice, TRUE) != ADI_INT_RESULT_SUCCESS)
        {
            Result = ADI_DEV_RESULT_CANT_HOOK_INTERRUPT;
        }

        /* We only allow the ATAPI PID to be activated once per open-close session, so that the
         * power-on-reset is not done repeatedly.
        */
        g_ActivateOncePerOpen = 0;

        /* Disable for now */
        adi_int_SICDisable(pDevice->DevIntPeripheralID);

    }

    /* and return */
    return Result;
}

/*********************************************************************
*
*   Function:       adi_pdd_Close
*
*   Description:    Closes down the ATAPI PID driver
*
*********************************************************************/

static u32 adi_pdd_Close(       // Closes a device
    ADI_DEV_PDD_HANDLE PDDHandle    // PDD handle
)
{
    ADI_ATAPI_DEF *pDevice = (ADI_ATAPI_DEF *)PDDHandle; /* Device Instance */
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    u32 DevIntIVG, Drive;

    /* Deactivate device and reset media availability flags */
    Activate( pDevice, DISABLE );
    for (Drive=0; Drive<(pDevice->NumDrives); Drive++ )
    {
        pDevice->Device[Drive].media_active = false;
    }

    /* Delete semaphores.
     */
     if (pDevice->DataSemaphoreHandle)
     {
        adi_sem_Delete(pDevice->DataSemaphoreHandle);
        adi_sem_Delete(pDevice->LockSemaphoreHandle);
     }

    /* free priority list */
    if (pDevice->pVolumePriority)
    {
        _adi_fss_free(PDD_GEN_HEAPID,pDevice->pVolumePriority);
    }

    /* free the device instance */
    _adi_fss_free(PDD_GEN_HEAPID,pDevice);

    /* unhook the DEV_INT interrupt handler from the system */
    adi_int_SICGetIVG(pDevice->DevIntPeripheralID, &DevIntIVG);
    adi_int_SICDisable(pDevice->DevIntPeripheralID);
    if (adi_int_CECUnhook(DevIntIVG, DeviceInterruptHandler, pDevice) != ADI_INT_RESULT_SUCCESS)
    {
        Result = ADI_DEV_RESULT_CANT_UNHOOK_INTERRUPT;
    }

    /* Uninstall Timer */
#if defined(_USE_TIMEOUT_TIMER)
    adi_tmr_RemoveCallback( pDevice->TimeOutTimerID );
    adi_tmr_Close( pDevice->TimeOutTimerID );
#endif

    /* return */
    return Result;
}

/*********************************************************************
*
*   Function:       adi_pdd_Read
*
*   Description:    PIO mode read
*
*********************************************************************/

static u32 adi_pdd_Read(        // Reads data or queues an inbound buffer to a device
    ADI_DEV_PDD_HANDLE  PDDHandle,  // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER      *pBuffer    // pointer to buffer
){

/* This function is only required for TRUE PIO mode
*/
    return (ADI_FSS_RESULT_FAILED);
}


/*********************************************************************
*
*   Function:       adi_pdd_Write
*
*   Description:    PIO mode write
*
*********************************************************************/

static u32 adi_pdd_Write(       // Writes data or queues an outbound buffer to a device
    ADI_DEV_PDD_HANDLE  PDDHandle,  // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER      *pBuffer    // pointer to buffer
){
    return (ADI_FSS_RESULT_FAILED);
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

    ADI_ATAPI_DEF   *pDevice;   /* Device Instance */
    u32             Result;     /* return value    */
    u32             Drive;      /* Drive Number    */

    pDevice = (ADI_ATAPI_DEF *)PDDHandle;

    /* assume we're going to be successful */
    Result = ADI_DEV_RESULT_SUCCESS;

    /* CASEOF (Command ID) */
    switch (Command) {

        case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):
            break;

        case (ADI_DEV_CMD_SET_DATAFLOW):
        case (ADI_PID_CMD_ENABLE_DATAFLOW):
            SetDataFlow( pDevice, (u32)Value );
            break;

        /* CASE (query for processor DMA support) */
        case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):
            *((u32 *)Value) = TRUE;
            break;

        case ADI_FSS_CMD_SET_CACHE_HEAP_ID:
            pDevice->CacheHeapID = (int)Value;
            break;

        case ADI_PID_CMD_GET_MAX_TFRCOUNT:
            /* Due to limitation of transfer length registers, the driver is limited to
             * one less than 64K words
            */
            *((u32 *)Value) = (64*1024 - 1)*2;
            break;

        /* CASE ( error reporting) */
        case (ADI_DEV_CMD_SET_ERROR_REPORTING):
            break;

        /* CASE (query for whether media is termed FIXED or removable)
           Used by FSD for formatting purposes, so does not apply to
           CD-ROM access
        */
        /* CASE (query for fixed status - format purposes only ) */
        case (ADI_PID_CMD_GET_FIXED):
            *((u32 *)Value) = TRUE;
            break;

        /* CASE ( Set number of devices on chain) */
        case (ADI_PID_CMD_SET_NUMBER_DEVICES):
            pDevice->NumDrives = (u32)Value;
            break;


        /* CASE ( Activate/Deactivate timers and flags) */
        case (ADI_PID_CMD_MEDIA_ACTIVATE):
            Result=Activate(pDevice,(u32)Value);
            break;

        /* CASE ( Send ATA Read Command ) */
        case (ADI_PID_CMD_SEND_LBA_REQUEST):
            Result = SendATACommand ( pDevice, (ADI_FSS_LBA_REQUEST*)Value );
            break;

        /* CASE ( Update volume priorities ) */
        case (ADI_PID_CMD_SET_VOLUME_PRIORITY):
            if (pDevice->pVolumePriority!=NULL)
            {
                u32 i,Val,Tmp;

                /* Update Volume Priority Table, This command takes one     */
                /* volume type and puts that value at the top of the volume */
                /* table list. If the value is ADI_FSS_PID_VOLUME_END then  */
                /* it is reset to the default, otherwise if the Value       */
                /* already exists in the table it is moved to the top */
                for (i=0,Val=(u32)Value;
                    i<ADI_FSS_PID_VOLUME_END;//&&
                        //(Val==ADI_FSS_PID_VOLUME_END||Val!=(u32)Value);
                    i++)
                {
                    Tmp=pDevice->pVolumePriority[i];
                    pDevice->pVolumePriority[i]=Val;
                    if (Tmp==(u32)Value)
                    {
                        break;
                    }
                    Val=(Val==ADI_FSS_PID_VOLUME_END)?Val:Tmp;
                }
            }
            else
            {
                Result=ADI_DEV_RESULT_NOT_SUPPORTED;
            }
            break;

        /* CASE ( Poll for media ) */
        case ADI_PID_CMD_POLL_MEDIA_CHANGE:
            Result = DevicePollMedia(pDevice);
            break;

        /* CASE ( Set callback function for direct access! ) */
        case (ADI_PID_CMD_SET_DIRECT_CALLBACK):
            FSSCallbackFunction = (ADI_DCB_CALLBACK_FN)Value;
            break;

        /* CASE ( Detect volumes ) */
        case ADI_PID_CMD_DETECT_VOLUMES:
            Result = DetectVolumes(pDevice,(u32)Value);
            break;

        /* CASE ( Get overall sizes of teh media (as if one partition) ) */
        case (ADI_PID_CMD_GET_GLOBAL_MEDIA_DEF):
            GetGlobalVolumeDef(pDevice,(ADI_FSS_VOLUME_DEF*)Value, pDevice->CurrentDrive);
            break;

        /* CASE (query for the PID's inbound DMA peripheral ID) */
        case (ADI_DEV_CMD_GET_INBOUND_DMA_PMAP_ID):
            *((ADI_DMA_PMAP *)Value) = pDevice->InboundMappingID;
            break;

        /* CASE (query for the PID's Outbound DMA peripheral ID) */
        case (ADI_DEV_CMD_GET_OUTBOUND_DMA_PMAP_ID):
            *((ADI_DMA_PMAP *)Value) = pDevice->OutboundMappingID;
            break;

        /* CASE ( Does PID support background data transfer ) */
        case (ADI_FSS_CMD_GET_BACKGRND_XFER_SUPPORT):
            *(u32*)Value = TRUE;
            //*(u32*)Value = FALSE;
            break;

        /* CASE ( Clear Interrupt ) */
        case (ADI_FSS_CMD_GET_DATA_ELEMENT_WIDTH):
            *(u32*)Value = sizeof(u16);
            break;

        /* CASE ( Force MDMA - over-ride UDMA  ) */
        case (ADI_ATAPI_CMD_FORCE_MDMA):
            pDevice->ForceMDMA = (u32)Value;
            break;

        /* CASE ( Force MDMA - over-ride UDMA  ) */
        case (ADI_ATAPI_CMD_FORCE_PIO):
            pDevice->ForcePIO = (u32)Value;

        case ( ADI_DEV_CMD_FREQUENCY_CHANGE_PROLOG ):
            {
                ADI_DEV_FREQUENCIES *pFreqs = (ADI_DEV_FREQUENCIES *)Value;
                /* if frequecny is increasing we need to adjust the timings
                 * ahead of frequency change. */
                if (pFreqs->SystemClock > pDevice->fsclk) {
                    pDevice->fcclk = pFreqs->CoreClock;
                    pDevice->fsclk = pFreqs->SystemClock;
                }
            }
            break;

        case ( ADI_DEV_CMD_FREQUENCY_CHANGE_EPILOG ):
            {
                ADI_DEV_FREQUENCIES *pFreqs = (ADI_DEV_FREQUENCIES *)Value;
                /* if frequecny is increasing we need to adjust the timings
                 * after frequency change. */
                if (pFreqs->SystemClock < pDevice->fsclk) {
                    pDevice->fcclk = pFreqs->CoreClock;
                    pDevice->fsclk = pFreqs->SystemClock;
                }
            }
            break;

        case (ADI_ATAPI_CMD_RUN_POST):
            Result = POSTFunction(pDevice, (u32)Value );
            break;

        /* CASE (Acquire Lock Semaphore for exclusive access) */
        case (ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE):
            adi_sem_Pend (pDevice->LockSemaphoreHandle, ADI_SEM_TIMEOUT_FOREVER);
            break;

        /* CASE (Release Lock Semaphore for exclusive access) */
        case (ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE):
            adi_sem_Post (pDevice->LockSemaphoreHandle);
            break;

        /* CASE (Disable Device Interrupt) */
        case (ADI_PID_CMD_DISABLE_INTERRUPT):
            adi_int_SICDisable(pDevice->DevIntPeripheralID);
            break;
        
        /* CASE (Enable Device Interrupt) */
        case (ADI_PID_CMD_ENABLE_INTERRUPT):
            adi_int_SICEnable(pDevice->DevIntPeripheralID);
            break;
        
        /* CASE ELSE */
        default:

            /* we don't understand this command */
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;

    /* ENDCASE */
    }

    /* return */
    return Result;
}

/*********************************************************************

    Function:       GetGlobalVolumeDef

    Description:    Assign and return pointer to index'd volume definition

*********************************************************************/
static void GetGlobalVolumeDef(ADI_ATAPI_DEF *pDevice, ADI_FSS_VOLUME_DEF *pVolumeDef, u32 Drive)
{
    pVolumeDef->StartAddress = pDevice->Device[Drive].FirstPartitionSector;
    pVolumeDef->VolumeSize   = pDevice->Device[Drive].LastSectorOnMedia
                               - pDevice->Device[Drive].FirstPartitionSector;
    if (!pDevice->Device[Drive].PacketInterfaceRequired)
    {
        pVolumeDef->SectorSize = 512;
    }
}

/* ***************************************************************************
 * ***************************************************************************
 * General ATA PID driver functions
 * ***************************************************************************
 * ***************************************************************************
 */



/*********************************************************************

    Function:       TransferSectors

    Description:    Read/Write a sector from the media

*********************************************************************/
static u32 TransferSectors(
                            ADI_ATAPI_DEF  *pDevice,
                            u32            Drive,
                            u32            SectorNumber,
                            u32            SectorCount,
                            u16            *buf,
                            u32            ReadFlag
)
{
    ADI_FSS_SUPER_BUFFER FSSBuffer;
    u32 Result;

    /* set up LBA request */
    FSSBuffer.LBARequest.StartSector    = SectorNumber;
    FSSBuffer.LBARequest.SectorCount    = SectorCount;
    FSSBuffer.LBARequest.DeviceNumber   = Drive;
    FSSBuffer.LBARequest.ReadFlag       = ReadFlag;
    FSSBuffer.LBARequest.pBuffer        = &FSSBuffer;

    /* Set up One D Buffer */
    FSSBuffer.Buffer.Data               = (void*)buf;
    FSSBuffer.Buffer.ElementCount       = SectorCount*512/sizeof(u16);
    FSSBuffer.Buffer.ElementWidth       = sizeof(u16);
    FSSBuffer.Buffer.CallbackParameter  = &FSSBuffer;
    FSSBuffer.Buffer.pAdditionalInfo    = NULL;
    FSSBuffer.Buffer.ProcessedFlag      = FALSE;
    FSSBuffer.Buffer.pNext              = NULL;

    /* Assign Semaphore */
    FSSBuffer.SemaphoreHandle           = pDevice->DataSemaphoreHandle;

    /* PID callback function and handle will be assigned when the LBA request is sent
    FSSBuffer.PIDCallbackFunction       = CallbackFromFSS;
    FSSBuffer.PIDCallbackHandle         = (void*)pDevice;
    */

    /* Assign Other fields to zero/false */
    FSSBuffer.FSDCallbackFunction       = NULL;
    FSSBuffer.FSDCallbackHandle         = NULL;
    FSSBuffer.LastInProcessFlag         = FALSE;
    FSSBuffer.pBlock                    = NULL;
    FSSBuffer.pFileDesc                 = NULL;

    Result = _adi_fss_PIDTransfer ( pDevice->DeviceHandle, &FSSBuffer, BLOCK );

    return Result;
}

/*********************************************************************

    Function:       Transfer

    Description:    Read/Write a data from the media

*********************************************************************/

static u32 Transfer(ADI_ATAPI_DEF *pDevice, u32 Drive, u32 ReadFlag, u32 Size, u8 *Data)
{
    u32 i, Result = ADI_FSS_RESULT_SUCCESS;
    u32 ndata = Size/2;
    u16 *pData = (u16*)Data;
    ADI_FSS_SUPER_BUFFER FSSBuffer;

    /* clear the interrupt status register */
    *pADI_ATA_INT_STATUS = 1;
    /* Unmask the Device Interrupt */
    //ADI_ATA_INT_MASK_ATA_DEV_INT_MASK_SET( *pADI_ATA_INT_MASK, 1 );
    
    /* set up Transfer Buffer */
    FSSBuffer.Buffer.Data = (void*)pData;
    FSSBuffer.Buffer.ElementWidth = sizeof(u16);
    FSSBuffer.Buffer.ElementCount = ndata;
    FSSBuffer.Buffer.CallbackParameter = &FSSBuffer;
    FSSBuffer.Buffer.pAdditionalInfo = NULL;
    FSSBuffer.Buffer.ProcessedFlag = FALSE;
    FSSBuffer.Buffer.pNext = NULL;
    
    /* On transfer completion the LBA RQ & FSS File descriptor are not required;
     * and for SCSI packets they're meaningless
    */
    FSSBuffer.LBARequest.SectorCount = 0;
    FSSBuffer.pFileDesc = NULL;

    /* Attach completion semaphore */
    FSSBuffer.SemaphoreHandle = pDevice->DataSemaphoreHandle;

    /* Set callback function and PID handle*/
    FSSBuffer.PIDCallbackFunction = CallbackFromFSS;
    FSSBuffer.PIDCallbackHandle = (void*)pDevice;

    /* Assign Other fields to zero/false */
    FSSBuffer.LastInProcessFlag         = FALSE;
    FSSBuffer.pBlock                    = NULL;
    FSSBuffer.pFileDesc                 = NULL;
    FSSBuffer.FSDCallbackFunction       = NULL;
    FSSBuffer.FSDCallbackHandle         = NULL;
    FSSBuffer.LBARequest.SectorCount    = 0;
    FSSBuffer.LBARequest.pBuffer        = &FSSBuffer;

    /* Only two fields need to be assigned in the stored LBA request */
    pDevice->CurrentLBARequest.SectorCount = 0;
    pDevice->CurrentLBARequest.pBuffer     = &FSSBuffer;

    /* Queue Buffer */
    if (ReadFlag) {
        Result = adi_dev_Read(pDevice->DeviceHandle,ADI_DEV_1D,(ADI_DEV_BUFFER*)&FSSBuffer);
    }
    else {
        Result = adi_dev_Write(pDevice->DeviceHandle,ADI_DEV_1D,(ADI_DEV_BUFFER*)&FSSBuffer);
    }

    /* start dataflow */
    Result = adi_dev_Control(pDevice->DeviceHandle, ADI_PID_CMD_ENABLE_DATAFLOW, (void*)TRUE );

    if (Result==ADI_FSS_RESULT_SUCCESS){
        /* Await transfer completion */
        Result = adi_sem_Pend( FSSBuffer.SemaphoreHandle, ADI_SEM_TIMEOUT_FOREVER );
    }

    return Result;
}

/*********************************************************************

    Function:       CallbackFromFSS

    Description:    The callback to be taken upon buffer completion and
                    device interrupt callbacks.
*********************************************************************/
static void
CallbackFromFSS(void *pHandle, u32 Event, void *pArg)
{
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pArg;
    ADI_ATAPI_DEF *pDevice             = (ADI_ATAPI_DEF *)pHandle;

    switch(Event)
    {
        case ADI_DEV_EVENT_BUFFER_PROCESSED:
            pDevice->StateFlag |= ADI_ATAPI_STATE_HDMA_INT;
            StateFlag = pDevice->StateFlag;
            break;

        case ADI_DEV_EVENT_DMA_ERROR_INTERRUPT:
        default:
            break;
    }
    ProcessInterrupt(pDevice, Event, (ADI_FSS_SUPER_BUFFER*)pArg);

}


/*********************************************************************

    Function:       DetectVolumes

    Description:    The device is checked for partitions dependin on the device
                    type. ATA devices are checked for FAT partitions and ATAPI
                    devices are checked for ISO9660 volumes and Audio tracks.

*********************************************************************/
static u32 DetectVolumes(
    ADI_ATAPI_DEF   *pDevice,
    u32             Drive)
{
    u32 Result= ADI_FSS_RESULT_SUCCESS;
    u32 Count = 0;

    /* The device is checked for partitions dependin on the device
       type. ATA devices are checked for FAT partitions. ATAPI
       devices are currently not implemented.
    */
    switch (pDevice->Device[Drive].Type)
    {
        case PDD_DEVICE_TYPE_ATA:
            Result = _adi_fss_DetectVolumes( 
                            pDevice->DeviceHandle, 
                            Drive,
                            pDevice->CacheHeapID,
                            512,
                            sizeof(u16),
                            pDevice->DataSemaphoreHandle
                    );
            break;
         default:
            Result = ADI_FSS_RESULT_FAILED;
    }
     
    return Result;
}

/*********************************************************************

    Function:       Activate

    Description:    performs hardware reset on the Device Chain

*********************************************************************/
static u32 Activate(ADI_ATAPI_DEF *pDevice, u32 EnableFlag)
{
    u32 Result = ADI_FSS_RESULT_FAILED;
    /* The reset is only performed once for all PID's in chain */
    static u32 PowerOnDone = 0;
    u16 Status=0;
    u16 sc, sn, cl, ch;
    u32 Drive, Count;
    int i;

    if (!g_ActivateOncePerOpen && EnableFlag)
    {
        /* Configure the PF flag and Mux registers to enable the ATAPI interface */
        SelectPortPins( EnableFlag );

        /* perform Power on / Reset sequence */
        Result = PowerOnReset(pDevice);
        g_ActivateOncePerOpen++;

        if (Result == ADI_FSS_RESULT_SUCCESS)
        {

            /* Create semaphores. Done here as all config commands will have been receieved by this point
             */
            if (Result == (u32)ADI_SEM_RESULT_SUCCESS)
            {
                Result = (u32)adi_sem_Create(0,&pDevice->DataSemaphoreHandle, NULL);
            }
            if (Result == (u32)ADI_SEM_RESULT_SUCCESS)
            {
                Result = (u32)adi_sem_Create(1,&pDevice->LockSemaphoreHandle, NULL);
            }
                
            /* Enable DMA Dataflow via the Device Manager. This will not affect the status
             * of the ATAPI Controller
            */
            if (Result == ADI_FSS_RESULT_SUCCESS)
            {
                Result = adi_dev_Control(pDevice->DeviceHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)TRUE );
            }
            
            /* Now read the signature (section 9.12 of ATA/ATAPI-5 standard) 
            */
            //for (Drive=0; Drive<(sizeof(pDevice->Device)/sizeof(pDevice->Device[0]));Drive++)
            for (Drive=0; Drive<(pDevice->NumDrives);Drive++)
            {
                SelectDrive ( pDevice, Drive );

                /* 10 ms delay required */
                _adi_fss_WaitMilliSec(10); 

                ReadDeviceRegister ( pDevice, ADI_ATA_SCNT_OFFSET, &sc );
                ReadDeviceRegister ( pDevice, ADI_ATA_SNUM_OFFSET, &sn );

                if ( (sc&0x00FF)!=1 || (sn&0x00FF)!=1 ) {
                    Result = ADI_FSS_RESULT_FAILED;
                }
                else 
                {
                    ReadDeviceRegister ( pDevice, ADI_ATA_CYLL_OFFSET, &cl );
                    ReadDeviceRegister ( pDevice, ADI_ATA_CYLH_OFFSET, &ch );

                    if ((cl&0x00FF)==0x14 && (ch&0x00FF)==0xEB ) {
                        /* ATAPI drive has sig 14h, EBh */
                        pDevice->Device[Drive].PacketInterfaceRequired = TRUE;
                        pDevice->Device[Drive].Type = PDD_DEVICE_TYPE_ATAPI;
                        pDevice->Device[Drive].media_active = TRUE;
                    }
                    else if ( (cl&0x00FF)==0 && (ch&0x00FF)==0 ) {
                        /* ATAPI drive has sig 0, 0 */
                        pDevice->Device[Drive].PacketInterfaceRequired = FALSE;
                        pDevice->Device[Drive].Type = PDD_DEVICE_TYPE_ATA;
                        pDevice->Device[Drive].media_active = TRUE;
                    }
                    else {
                        /* incorrect signature, assume failure */
                        Result = ADI_FSS_RESULT_FAILED;
                    }
                }
            } 

            /* We now initialize each device in turn and read its IDENTIFY structure 
            */
            
            /* FOR each Device on chain */
            //for (Drive=0; Drive<(sizeof(pDevice->Device)/sizeof(pDevice->Device[0]));Drive++)
            for (Drive=0; Drive<(pDevice->NumDrives);Drive++)
            { 
                /* IF (detect valid device) */
                if (InitializeDevice(pDevice, Drive))
                {
                    /* Set up Device for PIO Mode 1 initially and read the IDENTIFY data */
                    ADI_ATA_TRANSFER_TYPE_SET(pDevice->Device[Drive].TransferMode,ADI_ATA_MODE_PIO);
                    ADI_ATA_TRANSFER_MODE_SET(pDevice->Device[Drive].TransferMode,1);
                    
                    Result = ADI_FSS_RESULT_NO_MEDIA;
                    /* IF (set mode to PIO) */
                    if (WriteTransferMode( pDevice, Drive ) == ADI_FSS_RESULT_SUCCESS)
                    {
                        /* IF (IDENTIFY structure read OK) */
                        if (Identify(pDevice,Drive) == ADI_FSS_RESULT_SUCCESS)
                        {
                            /* IF (set mode to device's capability) */
                            if (WriteTransferMode( pDevice, Drive ) == ADI_FSS_RESULT_SUCCESS)
                            {
                                Result = ADI_FSS_RESULT_SUCCESS;
                                
                            /* END IF (WriteTransferMode()) */
                            }
                        /* END IF (Identify) */
                        }
                    /* END IF (WriteTransferMode(PIO)) */
                    }
                /* END IF (InitializeDevice) */
                }
            /* END FOR (Drive) */
            }
        }

    }
    /* On Disable, we simply mark the media as not present, to force the volume
     * detection process to be started.
    */
    else if (EnableFlag==false) {
        /* Deactivate */
        for (Drive=0;Drive<pDevice->NumDrives;Drive++)
        {
            pDevice->Device[Drive].MediaPresent = false;
        }
        Result = ADI_FSS_RESULT_SUCCESS;
    }
    /* On subsequent calls to enable activation we simply report success
    */
    else if (g_ActivateOncePerOpen)
    {
        Result = ADI_FSS_RESULT_SUCCESS;
    }

    return Result;
}


/*********************************************************************

    Function:       SelectPortPins

    Description:    Configures the ATAPI pins for use

*********************************************************************/
static u32 SelectPortPins(u32 EnableFlag)
{
    u32 Result;

    if (EnableFlag)
    {
        ADI_PORTS_DIRECTIVE ATAPI_PortConfig[] = {
#if defined( _USE_ATAPI_ON_GPIO_ )
                ADI_PORTS_DIRECTIVE_ATAPI_DA,
#endif
                ADI_PORTS_DIRECTIVE_ATAPI_RESET,
                ADI_PORTS_DIRECTIVE_ATAPI_DIOR,
                ADI_PORTS_DIRECTIVE_ATAPI_DIOW,
                ADI_PORTS_DIRECTIVE_ATAPI_CS0,
                ADI_PORTS_DIRECTIVE_ATAPI_CS1,
                ADI_PORTS_DIRECTIVE_ATAPI_DMACK,
                ADI_PORTS_DIRECTIVE_ATAPI_DMARQ,
                ADI_PORTS_DIRECTIVE_ATAPI_INTRQ,
                ADI_PORTS_DIRECTIVE_ATAPI_IORDY
                };
        Result = adi_ports_Configure( ATAPI_PortConfig, sizeof(ATAPI_PortConfig)/sizeof(ADI_PORTS_DIRECTIVE) );
    }

    return Result;
}

/*********************************************************************

    Function:       PrepareForCommand

    Description:    Set up the controller for issuing commands

*********************************************************************/
static void PrepareForCommand(ADI_ATAPI_DEF *pDevice)
{
    /* At this point we need to
       a) Disable Device Interrupt in SIC
       b) Set DEV_INT_MASK bit in ATAPI_INT_MASK register
       c) Ensure nIEN = 0
    */
    adi_int_SICDisable(pDevice->DevIntPeripheralID);
    ADI_ATA_INT_MASK_ATA_DEV_INT_MASK_SET ( *pADI_ATA_INT_MASK, 1);
    WriteDeviceRegister( pDevice,  ADI_ATA_CTRL_OFFSET, 0 );

    /* set transfer len to one word for device register access */
    ADI_ATA_XFER_LEN_SET  ( *pADI_ATA_XFER_LEN, 1 );
    /* clear any control start bits */
    *pADI_ATA_CTRL &= ~0x0007;
    /* Reset Transfer counts */
    ADI_ATA_CTRL_TFRCNT_RST_SET ( *pADI_ATA_CTRL, 1 );

}

/*********************************************************************

    Function:       PrepareForTransfer

    Description:    Set up the controller for data transfer

*********************************************************************/
static void PrepareForTransfer(ADI_ATAPI_DEF *pDevice)
{
    /* At this point we need to
       a) Enable Device Interrupt in SIC
       b) Set DEV_INT_MASK bit in ATAPI_INT_MASK register
       c) Ensure nIEN = 0
    */
    /* Reset Transfer counts */
    ADI_ATA_CTRL_TFRCNT_RST_SET ( *pADI_ATA_CTRL, 1 );

    /* Enable interrupts */
    adi_int_SICEnable(pDevice->DevIntPeripheralID);
    ADI_ATA_INT_MASK_ATA_DEV_INT_MASK_SET ( *pADI_ATA_INT_MASK, 1);

}

/*********************************************************************

    Function:       SendATACommand

    Description:    Sends an ATA read request to the device

*********************************************************************/
static u32
SendATACommand ( ADI_ATAPI_DEF *pDevice, ADI_FSS_LBA_REQUEST *pLBARequest )
{
    u32 Result = ADI_FSS_RESULT_SUCCESS;
    u16 ATA_Command;
    u32 SectorCount;
    u16 Status=0;
    u32 ReadFlag = pLBARequest->ReadFlag;
    /* Avoid structure reference for Drive number */
    u32 Drive = pLBARequest->DeviceNumber;
    
    void *pExitCritRegionArg;
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pLBARequest->pBuffer;

    if ( Result==ADI_FSS_RESULT_SUCCESS )
    {
        /* Cache current LBA request */
        pDevice->CurrentLBARequest = *pLBARequest;
        pSuperBuffer->PIDCallbackFunction = CallbackFromFSS;
        pSuperBuffer->PIDCallbackHandle   = (void*)pDevice;

        /* Set the state flag to indicate start of request */
        pDevice->StateFlag = ADI_ATAPI_STATE_XFER_IN_PROGESS;
        StateFlag = ADI_ATAPI_STATE_XFER_IN_PROGESS;
        DevIntCount=0;
        {
            int i;
            for (i=0;i<3;i++)
                DevIntStatus[i] = 0;
        }

        /* Ensure timing regs are set for the appropriate device */

        if ( Result==ADI_FSS_RESULT_SUCCESS && pDevice->CurrentDrive != Drive )
        {
            pDevice->CurrentDrive = Drive;
            SetTimingRegs(pDevice, pLBARequest->DeviceNumber);
        }

        /* Disable SIC interrupt, enable ATAPI_DEV_Int, nIEN=0 */
        PrepareForCommand(pDevice);

        /* Send the command to the device  */
        if (Result==ADI_FSS_RESULT_SUCCESS && pDevice->Device[Drive].Type==PDD_DEVICE_TYPE_ATA )
        {


            if ( pDevice->ForcePIO || ADI_ATA_TRANSFER_TYPE_GET(pDevice->Device[Drive].TransferMode)==ADI_ATA_MODE_PIO )
            {
                if (pDevice->Device[Drive].max_rw_multiples>1) {
                    if (IS_48BIT_ADDRESSABLE(Drive) ) {
                        ATA_Command = ( ReadFlag? ADI_ATA_CMD_READ_MULTIPLE_EXT : ADI_ATA_CMD_WRITE_MULTIPLE_EXT );
                    } else {
                        ATA_Command = ( ReadFlag? ADI_ATA_CMD_READ_MULTIPLE : ADI_ATA_CMD_WRITE_MULTIPLE );
                    }
                } else {
                    if (IS_48BIT_ADDRESSABLE(Drive) ) {
                        ATA_Command = ( ReadFlag? ADI_ATA_CMD_READ_SECTORS_EXT : ADI_ATA_CMD_WRITE_SECTORS_EXT );
                    } else {
                        ATA_Command = ( ReadFlag? ADI_ATA_CMD_READ_SECTORS_RETRY : ADI_ATA_CMD_WRITE_SECTORS );
                    }
                }
            }
            else /* if Multi word DMA or Ultra DMA */
            {
                if (IS_48BIT_ADDRESSABLE(Drive) ) {
                    ATA_Command = ( ReadFlag? ADI_ATA_CMD_READ_DMA_EXT : ADI_ATA_CMD_WRITE_DMA_EXT );
                } else {
                    ATA_Command = ( ReadFlag? ADI_ATA_CMD_READ_DMA : ADI_ATA_CMD_WRITE_DMA );
                }
            }

            /* Write the command to the device */
            Result = WriteDeviceCommand(
                        pDevice,
                        ATA_Command,
                        pLBARequest->DeviceNumber,
                        pLBARequest->SectorCount,
                        0,
                        pLBARequest->StartSector);


        }

        /* For all DMA modes set up the ATAPI interface registers */
        ConfigureRegistersForTransfer( pDevice, Drive, pLBARequest->SectorCount*256, ReadFlag );
    }

    return Result;
}

/*********************************************************************

    Function:       ConfigureRegistersForTransfer

    Description:    Configures teh ATAPI Controller registers for the transfer

*********************************************************************/
static void
ConfigureRegistersForTransfer( ADI_ATAPI_DEF *pDevice, u32 Drive, u16 xlen, u32 ReadFlag)
{

    if ( !pDevice->ForcePIO &&
         (  ADI_ATA_TRANSFER_TYPE_GET(pDevice->Device[Drive].TransferMode)==ADI_ATA_MODE_MDMA
         || ADI_ATA_TRANSFER_TYPE_GET(pDevice->Device[Drive].TransferMode)==ADI_ATA_MODE_UDMA )
    )
    {

        ADI_ATA_CTRL_PIO_USE_DMA_SET  ( *pADI_ATA_CTRL, 0 );
        ADI_ATA_XFER_LEN_SET          ( *pADI_ATA_XFER_LEN, xlen  );
        ADI_ATA_DEV_ADDR_DEV_ADDR_SET ( *pADI_ATA_DEV_ADDR, ADI_ATA_RDD_OFFSET);

        *pADI_ATA_INT_MASK = 0;
        pDevice->InterruptMask = 0;

        /* Give this driver control on premature termination
        */
        ADI_ATA_CTRL_END_ON_TERM_SET ( *pADI_ATA_CTRL, 0 );
        
        if (ReadFlag)
        {

            /* Set the transfer direction
            */
            ADI_ATA_CTRL_XFER_DIR_SET ( *pADI_ATA_CTRL, 0 );
            if (ADI_ATA_TRANSFER_TYPE_GET(pDevice->Device[Drive].TransferMode)==ADI_ATA_MODE_MDMA)
            {
                /* Set MDMA_DONE bit in status query mask */
                ADI_ATA_INT_STATUS_MDMA_DONE_SET(pDevice->InterruptMask, 1);
                /* Enable interrupts for DMA TERM And DONE events */
                //ADI_ATA_INT_MASK_MDMA_DONE_MASK_SET ( *pADI_ATA_INT_MASK, 1);
                ADI_ATA_INT_MASK_MDMA_TERM_MASK_SET ( *pADI_ATA_INT_MASK, 1);
            }
            else {
                /* SET UDMAIN_DONE bit in status query mask */
                ADI_ATA_INT_STATUS_UDMAIN_DONE_SET(pDevice->InterruptMask, 1);
                /* Enable interrupts for DMA TERM And DONE events */
                ADI_ATA_INT_MASK_UDMAIN_TERM_MASK_SET ( *pADI_ATA_INT_MASK, 1);
                //ADI_ATA_INT_MASK_UDMAIN_DONE_MASK_SET ( *pADI_ATA_INT_MASK, 1);

            }
        }
        else    /* Write Access */
        {
            /* Set the transfer direction
            */
            ADI_ATA_CTRL_XFER_DIR_SET ( *pADI_ATA_CTRL, 1 );
            if (ADI_ATA_TRANSFER_TYPE_GET(pDevice->Device[Drive].TransferMode)==ADI_ATA_MODE_MDMA)
            {
                /* SET MDMA_DONE bit in status query mask */
                ADI_ATA_INT_STATUS_MDMA_DONE_SET(pDevice->InterruptMask, 1);
                /* Enable interrupts for DMA TERM And DONE events */
                //ADI_ATA_INT_MASK_MDMA_DONE_MASK_SET ( *pADI_ATA_INT_MASK, 1);
                ADI_ATA_INT_MASK_MDMA_TERM_MASK_SET ( *pADI_ATA_INT_MASK, 1);
            } else {
                /* SET UDMAOUT_DONE bit in status query mask */
                ADI_ATA_INT_STATUS_UDMAOUT_DONE_SET(pDevice->InterruptMask, 1);
                /* Enable interrupts for DMA TERM And DONE events */
                ADI_ATA_INT_MASK_UDMAOUT_TERM_MASK_SET ( *pADI_ATA_INT_MASK, 1);
                //ADI_ATA_INT_MASK_UDMAOUT_DONE_MASK_SET ( *pADI_ATA_INT_MASK, 1);
            }
        }
    }
    else
    {
        /* PIO with DMA
        */

        ADI_ATA_XFER_LEN_SET          ( *pADI_ATA_XFER_LEN, xlen  );
        ADI_ATA_DEV_ADDR_DEV_ADDR_SET ( *pADI_ATA_DEV_ADDR, ADI_ATA_RDD_OFFSET);
        ADI_ATA_INT_MASK_PIO_DONE_MASK_SET ( *pADI_ATA_INT_MASK, 1);

        if (ReadFlag)
        {
            ADI_ATA_CTRL_XFER_DIR_SET ( *pADI_ATA_CTRL, 0 );
            pDevice->InterruptMask = ADI_ATA_INT_STATUS_PIO_DONE_MASK;
        }
        else
        {
            ADI_ATA_CTRL_XFER_DIR_SET ( *pADI_ATA_CTRL, 1 );
            pDevice->InterruptMask = ADI_ATA_INT_STATUS_PIO_DONE_MASK;
        }

        ADI_ATA_CTRL_PIO_USE_DMA_SET  ( *pADI_ATA_CTRL, 1 );
    }

}


/*********************************************************************

    Function:       Identify

    Description:    Identifies the IDE device and determines capabilities

*********************************************************************/

static u32 Identify(ADI_ATAPI_DEF *pDevice, u32 Drive)
{
    u32 Result = ADI_FSS_RESULT_SUCCESS;
    u16 multiple_sector_setting=0;
    u16 *word;
    u16 Status, Error, i;
    u64 nLBA48Size;

    /* Acquire Lock Semaphore for exclusive use of ATA bus. Will be released in callback */
    adi_dev_Control(pDevice->DeviceHandle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );

    /* Disable SIC interrupt, enable ATAPI_DEV_Int, nIEN=0 */
    PrepareForCommand(pDevice);

    if (pDevice->Device[Drive].Type==PDD_DEVICE_TYPE_ATA) {
        Result = WriteDeviceCommand(pDevice,ADI_ATA_CMD_IDENTIFY,Drive,0,0,0);
    }

    if (Result==ADI_FSS_RESULT_SUCCESS)
    {

        /* Wait for Device Interrupt before continuing  */
        WaitOnInterrupt(pDevice,ADI_ATA_INT_STATUS_ATA_DEV_INT_MASK, ADI_ATA_INT_STATUS_ATA_DEV_INT_SHIFT);

        /* Set up the ATAPI controller for the write transfer */
        ConfigureRegistersForTransfer( pDevice, Drive, 256, READ );

        if ( !pDevice->ForcePIO &&
             (  ADI_ATA_TRANSFER_TYPE_GET(pDevice->Device[Drive].TransferMode)==ADI_ATA_MODE_MDMA
             || ADI_ATA_TRANSFER_TYPE_GET(pDevice->Device[Drive].TransferMode)==ADI_ATA_MODE_UDMA )
             || ADI_ATA_CTRL_PIO_USE_DMA_GET(*pADI_ATA_CTRL)
        )
        {
            void *pExitCritRegionArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);
            pDevice->StateFlag = ADI_ATAPI_STATE_XFER_IN_PROGESS;
            StateFlag=pDevice->StateFlag;
            DevIntCount=0;
        {
            int i;
            for (i=0;i<3;i++)
                DevIntStatus[i] = 0;
        }
            adi_int_ExitCriticalRegion(pExitCritRegionArg);
        }

        /* Allocate the buffer for the identify data - we do this to avoid using
         * cached stack space in L3 for ATAPI DMA transfer
        */

        word = (u16*)_adi_fss_MallocAligned( pDevice->CacheHeapID, 256*sizeof(u16) );

        if (word) {
            memset(word, 0, 512);
            _adi_fss_FlushMemory( (char*)word, 512, ADI_DMA_WNR_WRITE);
            Result = Transfer (pDevice, Drive, READ, 512, (u8*)word);
        } else {
            Result = ADI_FSS_RESULT_NO_MEMORY;
        }


        /* Can only support ATAPI devices in LBA mode */
        if ( !Result && !pDevice->Device[Drive].PacketInterfaceRequired || BIT(9,word[49])==1 )
        {
            /* Disable SIC interrupt, enable ATAPI_DEV_Int, nIEN=0 */
            PrepareForCommand(pDevice);

            /* Retrieve Size of Media from First to last usable sector for
             * partitions.
            */
            pDevice->Device[Drive].FirstPartitionSector = word[6];
            /* 48 bit addressing is supported up to 2TB (limits of current 32 bit implementation);
             * bits 47:32 can thus be ignored.
            */
            nLBA48Size = *(u64*)&word[100];
            if (nLBA48Size > (u64)ADI_ATAPI_MAX_SECTOR_28BIT ) {
                /* use 48 bit max size (for media up to 2TB) */
                if ( *(u32*)&word[102] == 0) {
                    pDevice->Device[Drive].LastSectorOnMedia = *(u32*)&word[100] - 1;
                }
                else {
                    /* we do not support > 2TB */
                    Result = ADI_FSS_RESULT_NOT_SUPPORTED;
                }
            } else {
                /* use 28 bit max size */
                pDevice->Device[Drive].LastSectorOnMedia = *(u32*)&word[60];
            }

            if (Result==ADI_FSS_RESULT_SUCCESS)
            {
                /* determine operation mode timings and set ATAPI registers */
                Result = DetermineOperationModes(pDevice, Drive, &word[0]);
            }

            if (Result==ADI_FSS_RESULT_SUCCESS)
            {
                /* if PIO mode only Determine the Read/Write multiple capability and set accordingly */
                if ( pDevice->ForcePIO || ADI_ATA_TRANSFER_TYPE_GET(pDevice->Device[Drive].TransferMode)==ADI_ATA_MODE_PIO )
                {
                    pDevice->Device[Drive].max_rw_multiples = FIELD(0x00FF,0,word[47]);
                    if ( BIT(8,word[59])==1 )
                    {
                        multiple_sector_setting = FIELD(0x00FF,0,word[59]);
                    }

                    if (multiple_sector_setting < pDevice->Device[Drive].max_rw_multiples)
                    {
                        /* Acquire Lock Semaphore for exclusive use of ATA bus */
                        adi_dev_Control(pDevice->DeviceHandle, ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE, NULL );
                        Result = WriteDeviceCommand(
                                        pDevice,
                                        ADI_ATA_CMD_SET_MULTIPLE_MODE,
                                        Drive,
                                        pDevice->Device[Drive].max_rw_multiples,
                                        0,
                                        0
                                 );
                        /* Release Lock Semaphore  */
                        adi_dev_Control(pDevice->DeviceHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
                    }
                }
            }
            _adi_fss_FreeAligned( pDevice->CacheHeapID, word );
        }
        else {
            if (!Result) {
                Result = ADI_FSS_RESULT_NOT_SUPPORTED;
            }
        }
    }

    return Result;
}


/*********************************************************************
*
*   Function:       DetermineOperationModes
*
*   Description:    Determines the transfer modes available and which
*                   ones can be supported by the SCLK frequency.
*
*********************************************************************/
static u32 DetermineOperationModes( ADI_ATAPI_DEF *pDevice, u32 Drive, u16 *word )
{
    u16 iordy_can_be_set;
    u16 min_pio_cycle_time;
    u16 min_mdma_xfer_cycle_time_per_word;
    u16 b, mode, setFlag;
    u32 Result=ADI_FSS_RESULT_SUCCESS;

    /* Clear the Device[Drive].TransferMode Word */
    pDevice->Device[Drive].TransferMode = 0;


    pDevice->Device[Drive].SupportsIORDY                   = BIT(11,word[49]);
    pDevice->Device[Drive].CanDisableIORDY                 = BIT(10,word[49]);
    pDevice->Device[Drive].CommandSetNotificationSupported =
                                    ( word[82]!=0x0000 && word[82]!=0xFFFF
                                   && word[83]!=0x0000 && word[82]!=0xFFFF );

    if (pDevice->Device[Drive].CommandSetNotificationSupported)
    {
        pDevice->Device[Drive].FeaturesSupported        = word[82];
        pDevice->Device[Drive].CommandSetsSupported     = word[83];
        pDevice->Device[Drive].FeaturesEnabled          = (((u32)word[85])&0x0000FFFF) | ((((u32)word[85])<<16)&0xFFFF0000);
    }
    else
    {
        pDevice->Device[Drive].FeaturesSupported        = 0;
        pDevice->Device[Drive].CommandSetsSupported     = 0;
    }

    /* ****************************************
     *           PIO Mode timings             *
     * ****************************************
    */
    /* find max PIO mode supported by the device */
    if ( BIT(1,word[53])==1 )
    {
        ADI_ATA_MAX_DEV_PIO_MODE_SET( pDevice->Device[Drive].TransferMode, FIELD(0x0003,0,word[64])+2 );
        /* bit 11, word 49 determines whether IORDY is supported */
        /* bit 10, word 49 determines whether use of IORDY can be set */
        if ( pDevice->Device[Drive].SupportsIORDY &&  pDevice->Device[Drive].CanDisableIORDY )
        {
            min_pio_cycle_time = word[68];
        } else {
            min_pio_cycle_time = word[67];
        }
    }
    else {
        ADI_ATA_MAX_DEV_PIO_MODE_SET(pDevice->Device[Drive].TransferMode,2);
    }

    /* Determine the highest PIO mode that can be supported at current SCLK */
    if ( ADI_ATA_MAX_DEV_PIO_MODE_GET(pDevice->Device[Drive].TransferMode) >=0 )
    {
        setFlag = FALSE;
        for (mode=0; mode<=ADI_ATA_MAX_DEV_PIO_MODE_GET(pDevice->Device[Drive].TransferMode); mode++) {
            if ( (!min_pio_cycle_time || pio_t0min[mode] >= min_pio_cycle_time)
                 && !CalculatePIOTimings(pDevice->fsclk, mode, &pDevice->Device[Drive]) )
            {
                ADI_ATA_MAX_CUR_PIO_MODE_SET(pDevice->Device[Drive].TransferMode, mode);
                setFlag = TRUE;
            }
        }
        if (!setFlag) {
            ADI_ATA_MAX_CUR_PIO_MODE_SET(pDevice->Device[Drive].TransferMode, ADI_ATA_MODE_UNSET);
        }

    }
    else
    {
        ADI_ATA_MAX_CUR_PIO_MODE_SET(pDevice->Device[Drive].TransferMode, ADI_ATA_MODE_UNSET);
    }

    /* ****************************************
     *       Multi-word DMA Mode timings      *
     * ****************************************
    */

    if ( !pDevice->Device[Drive].PacketInterfaceRequired || BIT(8,word[49])==1 )
    {
        /* find max multiword DMA mode supported */
        for (b=0;b<3;b++) {
            if ( BIT(b,word[63])==1 ) {
                ADI_ATA_MAX_DEV_MDMA_MODE_SET (pDevice->Device[Drive].TransferMode, b );
            }
        }

        min_mdma_xfer_cycle_time_per_word = word[65];

        /* Determine the highest MDMA mode that can be supported at current SCLK */
        if ( !pDevice->ForcePIO && ADI_ATA_MAX_DEV_MDMA_MODE_GET(pDevice->Device[Drive].TransferMode) >=0 )
        {
            setFlag = FALSE;
            for (mode=0; mode<=ADI_ATA_MAX_DEV_MDMA_MODE_GET(pDevice->Device[Drive].TransferMode); mode++) {
                if ( mdma_t0min[mode] >= min_mdma_xfer_cycle_time_per_word
                     && !CalculateMDMATimings(pDevice->fsclk, mode, &pDevice->Device[Drive] ) )
                {
                    ADI_ATA_MAX_CUR_MDMA_MODE_SET(pDevice->Device[Drive].TransferMode, mode);
                    setFlag = TRUE;
                }
            }
            if (!setFlag) {
                ADI_ATA_MAX_CUR_MDMA_MODE_SET(pDevice->Device[Drive].TransferMode, ADI_ATA_MODE_UNSET);
            }
        }
        else
        {
            ADI_ATA_MAX_CUR_MDMA_MODE_SET(pDevice->Device[Drive].TransferMode, ADI_ATA_MODE_UNSET);
        }


        /* ****************************************
         *       Ultra DMA Mode timings           *
         * ****************************************
        */

        if ( BIT(2,word[53])==1 ) {

            /* find max ultra DMA mode supported */
            for (b=0;b<6;b++) {
                if ( BIT(b,word[88])==1 ) {
                    ADI_ATA_MAX_DEV_UDMA_MODE_SET(pDevice->Device[Drive].TransferMode, b);
                }
            }

            /* Determine the highest UDMA mode that can be supported at this frequency */

            if ( !pDevice->ForceMDMA && !pDevice->ForcePIO
                 && ADI_ATA_MAX_DEV_UDMA_MODE_GET(pDevice->Device[Drive].TransferMode) >=0 )
            {
                setFlag = FALSE;
                for (mode=0; mode<=ADI_ATA_MAX_DEV_UDMA_MODE_GET(pDevice->Device[Drive].TransferMode); mode++) {
                    if ( !CalculateUDMATimings(pDevice->fsclk, mode, &pDevice->Device[Drive]) )
                    {
                        /* assign the max mode available for the given frequency */
                        ADI_ATA_MAX_CUR_UDMA_MODE_SET( pDevice->Device[Drive].TransferMode, mode );
                        setFlag = TRUE;

                    }
                }
                if (!setFlag) {
                    ADI_ATA_MAX_CUR_UDMA_MODE_SET(pDevice->Device[Drive].TransferMode, ADI_ATA_MODE_UNSET);
                }
            }
            else
            {
                ADI_ATA_MAX_CUR_UDMA_MODE_SET(pDevice->Device[Drive].TransferMode, ADI_ATA_MODE_UNSET);
            }
        }
        else
        {
            ADI_ATA_MAX_CUR_UDMA_MODE_SET(pDevice->Device[Drive].TransferMode, ADI_ATA_MODE_UNSET);
        }
    }
    else
    {
        ADI_ATA_MAX_CUR_UDMA_MODE_SET(pDevice->Device[Drive].TransferMode, ADI_ATA_MODE_UNSET);
        ADI_ATA_MAX_CUR_MDMA_MODE_SET(pDevice->Device[Drive].TransferMode, ADI_ATA_MODE_UNSET);
    }


    /* Set Register timings for the max PIO mode */
    CalculateRegTimings(pDevice->fsclk, ADI_ATA_MAX_CUR_PIO_MODE_GET(pDevice->Device[Drive].TransferMode), &pDevice->Device[Drive] );

    /* Program the register transfer timing registers */
    SetRegXferTimingRegs( &pDevice->Device[Drive] );

    /* Set Driver & Interface to use the best available mode */

    if ( !pDevice->ForceMDMA && !pDevice->ForcePIO
        && ADI_ATA_MAX_CUR_UDMA_MODE_GET(pDevice->Device[Drive].TransferMode)!=ADI_ATA_MODE_UNSET )
    {
        ADI_ATA_TRANSFER_TYPE_SET( pDevice->Device[Drive].TransferMode, ADI_ATA_MODE_UDMA );
        ADI_ATA_TRANSFER_MODE_SET( pDevice->Device[Drive].TransferMode,
                                   ADI_ATA_MAX_CUR_UDMA_MODE_GET(pDevice->Device[Drive].TransferMode) );
        /* Program the UDMA timing registers accordingly */
        SetUDMATimingRegs( &pDevice->Device[Drive] );
    }
    else if ( !pDevice->ForcePIO && ADI_ATA_MAX_CUR_MDMA_MODE_GET(pDevice->Device[Drive].TransferMode)!=ADI_ATA_MODE_UNSET )
    {
        ADI_ATA_TRANSFER_TYPE_SET( pDevice->Device[Drive].TransferMode, ADI_ATA_MODE_MDMA );
        ADI_ATA_TRANSFER_MODE_SET( pDevice->Device[Drive].TransferMode, ADI_ATA_MAX_CUR_MDMA_MODE_GET(pDevice->Device[Drive].TransferMode) );
        /* Program the MDMA timing registers accordingly */
        SetMDMATimingRegs( &pDevice->Device[Drive] );
    }
    else if ( ADI_ATA_MAX_CUR_PIO_MODE_GET(pDevice->Device[Drive].TransferMode)!=ADI_ATA_MODE_UNSET )
    {
        ADI_ATA_TRANSFER_TYPE_SET( pDevice->Device[Drive].TransferMode, ADI_ATA_MODE_PIO );
        ADI_ATA_TRANSFER_MODE_SET( pDevice->Device[Drive].TransferMode, ADI_ATA_MAX_CUR_PIO_MODE_GET(pDevice->Device[Drive].TransferMode));
        /* Program the PIO timing registers */
        SetPIOTimingRegs( &pDevice->Device[Drive] );
    }
    else
    {
        Result = ADI_FSS_RESULT_FAILED;
    }

#ifdef _DEV_PURPOSES_ONLY /* For development purposes only */
    CurrentTransferMode = pDevice->Device[Drive].TransferMode & (ADI_ATA_TRANSFER_MODE_MASK | ADI_ATA_TRANSFER_TYPE_MASK);
#endif

    return Result;
}


/*********************************************************************

    Function:       CalculateUDMATimings

    Description:    calculate the timing values for Ultra DMA.
                    returns failure code if mode not supported.

*********************************************************************/

static u32 CalculateUDMATimings(u32 fsclk, u16 mode, ADI_ATAPI_DEVICE_DEF *pDevice)
{
    u32 ndvs, ncyc, nt, nmin;
    u32 Result=ADI_FSS_RESULT_SUCCESS;

    /* the most restrictive timing value is t6 and tc, the DIOW - data hold
     * If one SCLK pulse is longer than this minimum value then register
     * transfers cannot be supported at this frequency.
    */

    nmin = num_clocks_min((long long)udma_tmin[mode],(long long)fsclk);
    if ( (udma_fsclk[mode]<=fsclk) && nmin>=1)
    {
        /* calculate the SCLK pulses required for each relevant timing value */

        /* */
        ndvs = num_clocks_min((long long)udma_tdvsmin[mode],(long long)fsclk);
        ncyc = num_clocks_min((long long)udma_tcycmin[mode],(long long)fsclk);
        nt = 2;
        /* increase tcyc - tdvs (nt) until we meed the minimum cycle length */
        while ( (ndvs+nt) < ncyc ) {
            nt++;
        }

        /* Mow assign the values required for the timing registers */

        /*  */

        if (nt<2) {
            nt = 2;
        }
        pDevice->udma_tcyc_minus_tdvs = nt;
        /*  */
        if (ndvs<2) {
            ndvs = 2;
        }
        pDevice->udma_tdvs = ndvs;
        /*  */
        pDevice->udma_tack = num_clocks_min((long long)udma_tackmin,(long long)fsclk);
        /*  */
        pDevice->udma_tss = num_clocks_min((long long)udma_tssmin,(long long)fsclk);
        /*  */
        pDevice->udma_tmli = num_clocks_min((long long)udma_tmlimin,(long long)fsclk);
        /*  */
        pDevice->udma_tzah = num_clocks_min((long long)udma_tzahmin,(long long)fsclk);
        /*  */
        pDevice->udma_trp = num_clocks_min((long long)udma_trpmin[mode],(long long)fsclk);
        /*  */
        pDevice->udma_tenv = num_clocks_min((long long)udma_tenvmin,(long long)fsclk);
        if (pDevice->udma_tenv>udma_tenvmax[mode])
        {
            Result = ADI_FSS_RESULT_FAILED;
        }

    } else {
        Result = ADI_FSS_RESULT_FAILED;
    }

    return Result;
}

/*********************************************************************

    Function:       CalculateRegTimings

    Description:    calculate the timing values for register transfers.
                    returns failure code if mode not supported.

*********************************************************************/
static u32 CalculateRegTimings(u32 fsclk, u16 mode, ADI_ATAPI_DEVICE_DEF *pDevice)
{
    u32 n0, n2, neoc;
    u32 n6,t6min = 5;
    u32 Result=ADI_FSS_RESULT_SUCCESS;

    /* the most restrictive timing value is t6 and tc, the DIOW - data hold
     * If one SCLK pulse is longer than this minimum value then register
     * transfers cannot be supported at this frequency.
    */
    n6 = num_clocks_min((long long)t6min,(long long)fsclk);
    if ( (pio_fsclk[mode]<=fsclk) && n6>=1)
    {
        /* calculate the SCLK pulses required for each relevant timing value */

        /* DIOR/DIOW to end cycle time */
        n2 = num_clocks_min((long long)reg_t2min[mode],(long long)fsclk);
        /* DIOR/DIOW asserted pulse width */
        neoc = num_clocks_min((long long)reg_teocmin[mode],(long long)fsclk);
        /* Cycle Time */
        n0  = num_clocks_min((long long)reg_t0min[mode],(long long)fsclk);

        /* increase t2 until we meed the minimum cycle length */
        while ( (n2+neoc) < n0 ) {
            n2++;
        }

        /* Mow assign the values required for the timing registers */

        /* DIOR/DIOW to end cycle time */
        pDevice->reg_t2 = n2;
        /* DIOR/DIOW asserted pulse width */
        pDevice->reg_teoc = neoc;

    } else {
        Result = ADI_FSS_RESULT_FAILED;
    }

    return Result;
}

/*********************************************************************

    Function:       CalculatePIOTimings

    Description:    calculate the timing values for PIO transfers
                    returns failure code if mode not supported.

*********************************************************************/

static u32 CalculatePIOTimings(u32 fsclk, u16 mode, ADI_ATAPI_DEVICE_DEF *pDevice)
{
    u32 n0, n2, neoc;
    u32 n6,t6min = 5;
    u32 Result=ADI_FSS_RESULT_SUCCESS;

    /* the most restrictive timing value is t6 and tc, the DIOW - data hold
     * If one SCLK pulse is longer than this minimum value then register
     * transfers cannot be supported at this frequency.
    */
    n6 = num_clocks_min((long long)t6min,(long long)fsclk);
    if ( (pio_fsclk[mode]<=fsclk) && n6>=1)
    {
        /* calculate the SCLK pulses required for each relevant timing value */

        /* DIOR/DIOW to end cycle time */
        n2 = num_clocks_min((long long)pio_t2min[mode],(long long)fsclk);
        /* DIOR/DIOW asserted pulse width */
        neoc = num_clocks_min((long long)pio_teocmin[mode],(long long)fsclk);
        /* Cycle Time */
        n0  = num_clocks_min((long long)pio_t0min[mode],(long long)fsclk);

        /* increase t2 until we meed the minimum cycle length */
        while ( (n2+neoc) < n0 ) {
            n2++;
        }

        /* Mow assign the values required for the timing registers */

        /* Address valid to DIOR/DIORW */
        pDevice->pio_t1 = num_clocks_min((long long)pio_t1min[mode],(long long)fsclk);
        /* DIOR/DIOW to end cycle time */
        pDevice->pio_t2 = n2;
        /* DIOR/DIOW asserted pulse width */
        pDevice->pio_teoc = neoc;
        /* DIOW data hold */
        pDevice->pio_t4 = num_clocks_min((long long)pio_t4min[mode],(long long)fsclk);

    } else {
        Result = ADI_FSS_RESULT_FAILED;
    }

    return Result;
}

/*********************************************************************

    Function:       CalculateMDMATimings

    Description:    calculate the timing values for Multi-word DMA.
                    returns failure code if mode not supported.

*********************************************************************/

static u32 CalculateMDMATimings(u32 fsclk, u16 mode, ADI_ATAPI_DEVICE_DEF *pDevice)
{
    u32 nkw, nd, n0, nz, ne;
    u32 nf,tfmin = 5;
    u32 Result=ADI_FSS_RESULT_SUCCESS;

    /* the most restrictive timing value is tf, the DMACK to read data released
     * If one SCLK pulse is longer than this maximum value then the MDMA mode
     * cannot be supported at this frequency.
    */
    nf = num_clocks_min((long long)tfmin,(long long)fsclk);
    //nz = num_clocks_max((long long)mdma_tzmax[mode],(long long)fsclk);
    if ( (mdma_fsclk[mode]<=fsclk) && nf>=1)
    {
        /* calculate the SCLK pulses required for each relevant timing value */

        /* DIOR/DIOW asserted pulse width */
        nd = num_clocks_min((long long)mdma_tdmin[mode],(long long)fsclk);

        /* DIOR negated pulse width */
        nkw = num_clocks_min((long long)mdma_tkwmin[mode],(long long)fsclk);

        /* Cycle Time */
        n0  = num_clocks_min((long long)mdma_t0min[mode],(long long)fsclk);

        /* increase tk until we meed the minimum cycle length */
        while ( (nkw+nd) < n0 ) {
            nkw++;
        }

        /* Mow assign the values required for the timing registers */

        /* DIOR negated pulse width - write */
        pDevice->mdma_tkw = nkw;
        /* DIOR negated pulse width - read */
        pDevice->mdma_tkr = num_clocks_min((long long)mdma_tkrmin[mode],(long long)fsclk);
        /* CS{1:0] valid to DIOR/DIOW */
        pDevice->mdma_tm = num_clocks_min((long long)mdma_tmmin[mode],(long long)fsclk);
        /* DIOR/DIOW to DMACK hold */
        pDevice->mdma_teoc = num_clocks_min((long long)mdma_tjmin[mode],(long long)fsclk);
        /* DIOR/DIOW asserted pulse width */
        pDevice->mdma_td = nd;
        /* DIOW Data hold */
        pDevice->mdma_th = num_clocks_min((long long)mdma_thmin[mode],(long long)fsclk);


    } else {
        Result = ADI_FSS_RESULT_FAILED;
    }
    return Result;
}

/*********************************************************************

    Function:       num_clocks_min

    Description:    calculate number of SCLK cycles to meet minimum timing

*********************************************************************/
static u32 num_clocks_min(long long tmin, long long fsclk)
{
    long long tmp ;
    u32 Result;

    tmp = tmin*fsclk/1000000000;
    Result = (u32)tmp;
    if ( tmp*1000 < (tmin*fsclk) )
    {
        Result++;
    }

    return Result;
}

/*********************************************************************

    Function:       num_clocks_max

    Description:    calculate number of SCLK cycles to meet maximum timing

*********************************************************************/
static u32 num_clocks_max(long long tmax, long long fsclk)
{
    u32 Result;
    Result = (u32)(tmax*fsclk/1000000000);
    return Result;
}

/*********************************************************************

    Function:       SetPIOTimingRegs

    Description:    Programs the PIO timing registers

*********************************************************************/
static void SetPIOTimingRegs( ADI_ATAPI_DEVICE_DEF *pDevice )
{
    ADI_ATA_PIO_TIM_0_T1_REG_SET   ( *pADI_ATA_PIO_TIM_0, pDevice->pio_t1   );
    ADI_ATA_PIO_TIM_0_T2_REG_SET   ( *pADI_ATA_PIO_TIM_0, pDevice->pio_t2   );
    ADI_ATA_PIO_TIM_0_T4_REG_SET   ( *pADI_ATA_PIO_TIM_0, pDevice->pio_t4   );
    ADI_ATA_PIO_TIM_1_TEOC_REG_SET ( *pADI_ATA_PIO_TIM_1, pDevice->pio_teoc );
}

/*********************************************************************

    Function:       SetRegXferTimingRegs

    Description:    Programs the register transfer timing registers

*********************************************************************/
static void SetRegXferTimingRegs( ADI_ATAPI_DEVICE_DEF *pDevice )
{
    ADI_ATA_REG_TIM_0_T2_REG_SET   (*pADI_ATA_REG_TIM_0,pDevice->reg_t2   );
    ADI_ATA_REG_TIM_0_TEOC_REG_SET (*pADI_ATA_REG_TIM_0,pDevice->reg_teoc );
}

/*********************************************************************

    Function:       SetMDMATimingRegs

    Description:    Programs the multi-word DMA timing registers

*********************************************************************/
static void SetMDMATimingRegs( ADI_ATAPI_DEVICE_DEF *pDevice )
{
    ADI_ATA_MDMA_TIM_0_TD_SET   ( *pADI_ATA_MDMA_TIM_0, pDevice->mdma_td   );
    ADI_ATA_MDMA_TIM_0_TM_SET   ( *pADI_ATA_MDMA_TIM_0, pDevice->mdma_tm   );
    ADI_ATA_MDMA_TIM_1_TKW_SET  ( *pADI_ATA_MDMA_TIM_1, pDevice->mdma_tkw  );
    ADI_ATA_MDMA_TIM_1_TKR_SET  ( *pADI_ATA_MDMA_TIM_1, pDevice->mdma_tkr  );
    ADI_ATA_MDMA_TIM_2_TH_SET   ( *pADI_ATA_MDMA_TIM_2, pDevice->mdma_th   );
    ADI_ATA_MDMA_TIM_2_TEOC_SET ( *pADI_ATA_MDMA_TIM_2, pDevice->mdma_teoc );
}

/*********************************************************************

    Function:       SetUDMATimingRegs

    Description:    Programs the register transfer timing registers

*********************************************************************/
static void SetUDMATimingRegs( ADI_ATAPI_DEVICE_DEF *pDevice )
{
    ADI_ATA_UDMA_TIM_0_TACK_SET     ( *pADI_ATA_UDMA_TIM_0, pDevice->udma_tack              );
    ADI_ATA_UDMA_TIM_0_TENV_SET     ( *pADI_ATA_UDMA_TIM_0, pDevice->udma_tenv              );
    ADI_ATA_UDMA_TIM_1_TDVS_SET     ( *pADI_ATA_UDMA_TIM_1, pDevice->udma_tdvs              );
    ADI_ATA_UDMA_TIM_1_TCYC_TDVS_SET( *pADI_ATA_UDMA_TIM_1, pDevice->udma_tcyc_minus_tdvs   );
    ADI_ATA_UDMA_TIM_2_TSS_SET      ( *pADI_ATA_UDMA_TIM_2, pDevice->udma_tss               );
    ADI_ATA_UDMA_TIM_2_TMLI_SET     ( *pADI_ATA_UDMA_TIM_2, pDevice->udma_tmli              );
    ADI_ATA_UDMA_TIM_3_TZAH_SET     ( *pADI_ATA_UDMA_TIM_3, pDevice->udma_tzah              );
    ADI_ATA_UDMA_TIM_3_TRP_SET      ( *pADI_ATA_UDMA_TIM_3, pDevice->udma_trp               );
}

/*********************************************************************
 * Function:    WriteDeviceCommand
 *
 * Description: Execute an ata command, either ADI_FSS_RESULT_SUCCESS
 *      if the command was accepted, or ADI_FSS_RESULT_FALIED if there
 *      was an Error or if there is command time-out waiting for the
 *      BUSY or DRQ flags.
 *********************************************************************/
static u32
WriteDeviceCommand(
            ADI_ATAPI_DEF *pDevice,         /* ATA/ATAPI Device Instance        */
            u16            Command,         /* ATA command                      */
            u16            DriveNumber,     /* Drive Number 0=Master, 1=Slave   */
            u16            SectorCount,     /* Sector Count register contents   */
            u16            Feature,         /* Feature register comtents        */
            u32            LBA              /* Logical Block Address value      */
)
{
    u32 Result = ADI_FSS_RESULT_SUCCESS;
    u8 DriveHead = 0xA0;

    /* Output command */
    WriteDeviceRegister( pDevice,  ADI_ATA_FEAT_OFFSET, Feature          );

    /* For extended (48 bit support we write the higher order bytes into the 2 deep FIFO first  */
    if ( IS_48BIT_COMMAND(Command) ) {

        WriteDeviceRegister( pDevice,  ADI_ATA_SCNT_OFFSET, (SectorCount>>8)&0xFF  );        /* Sector Count (15:8)  */
        WriteDeviceRegister( pDevice,  ADI_ATA_SNUM_OFFSET, ((LBA>>24)&0xFF)       );        /* LBA (31:24)     */
        WriteDeviceRegister( pDevice,  ADI_ATA_CYLL_OFFSET, 0                      );        /* LBA (39:32)      */
        WriteDeviceRegister( pDevice,  ADI_ATA_CYLH_OFFSET, 0                      );        /* LBA (47:40)     */

        /* Set up the CDH value (does not have LBA(27:24) for 48 bit command) */
        DriveHead = 0;
        ADI_ATA_DEV_CDH_DRIVE_SET    ( DriveHead, DriveNumber            );
        ADI_ATA_DEV_CDH_LBA_SET      ( DriveHead, 1                      );
    }
    else
    {
        /* Set up the CDH value - mix of Device number and LBA MSB */
        ADI_ATA_DEV_CDH_DRIVE_SET    ( DriveHead, DriveNumber            );
        ADI_ATA_DEV_CDH_LBA_SET      ( DriveHead, 1                      );
        ADI_ATA_DEV_CDH_LBA_ADDR_SET ( DriveHead, ((LBA&0x0f000000)>>24) );
    }

    /* Now write the lower order bytes as before */
    WriteDeviceRegister( pDevice,  ADI_ATA_SCNT_OFFSET, SectorCount&0xFF );        /* Sector Count (7:0)   */
    WriteDeviceRegister( pDevice,  ADI_ATA_SNUM_OFFSET, (LBA&0xFF)       );        /* LBA (7:0)     */
    WriteDeviceRegister( pDevice,  ADI_ATA_CYLL_OFFSET, ((LBA>>8)&0xFF)  );        /* LBA (15:8)      */
    WriteDeviceRegister( pDevice,  ADI_ATA_CYLH_OFFSET, ((LBA>>16)&0xFF) );        /* LBA (23:16)     */
    WriteDeviceRegister( pDevice,  ADI_ATA_CDH_OFFSET,  DriveHead    );        /* Drive/head  */
    WriteDeviceRegister( pDevice,  ADI_ATA_CMD_OFFSET,  Command          );        /* Command Register  */

    return Result;
}


/*********************************************************************

    Function:       PowerOnReset

    Description:    performs hardware reset on the Device Chain

*********************************************************************/
static u32 PowerOnReset(ADI_ATAPI_DEF *pDevice)
{
    u32 Result=ADI_FSS_RESULT_SUCCESS;
    static u32 PowerOnDone = 0, Count;
    u16 Status;

    /* Disable the interrupt handler as it is best to poll for completion */
    adi_int_SICDisable(pDevice->DevIntPeripheralID);

    /* Assert the RESET signal 25us*/
    SendResetSignal(1);
    _adi_fss_WaitMicroSec(25);

    /* Negate the RESET signal for 2ms*/
    SendResetSignal(0);
    _adi_fss_WaitMilliSec(2) ;

    /* Wait on Busy flag to clear */
    Count = 10000000;
    do {
        ReadDeviceRegister( pDevice,  ADI_ATA_STAT_OFFSET, &Status );
        Count--;
    } while( Count && ADI_ATA_DEV_STATUS_BSY_GET(Status) );

    if (!Count) {
        Result = ADI_FSS_RESULT_NO_MEDIA;
    }

    return Result;


}

/*********************************************************************

  Function:    DevicePollMedia

  Description: Detect media change, this is used in particular to detect
               when a CD is removed.or inserted.

*********************************************************************/
static u32 DevicePollMedia(ADI_ATAPI_DEF *pDevice)
{
    u32 Result,Drive;
    u16 Status;

    Result=ADI_FSS_RESULT_SUCCESS;

    /* Poll all devices in PID */
    for (Drive=0;Drive<pDevice->NumDrives;Drive++)
    {
        /* Check if media is not active, if it is not then
           throw media change event
        */
        if (!pDevice->Device[Drive].MediaPresent)
        {
            /* Use Result to pass the device number */
            Result=Drive;
            if ( FSSCallbackFunction ){
                /* tbis can be used if we do not wish to use DCB for Poll Media commands */
                (FSSCallbackFunction)( &pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_INSERTED, &Result );
            } else {
                (pDevice->DMCallback) ( pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_INSERTED, &Result );
            }

            if ( Result==ADI_FSS_RESULT_SUCCESS ) {
                pDevice->Device[Drive].MediaPresent = TRUE;
            } else {
                pDevice->Device[Drive].MediaPresent = FALSE;
            }
        }
    }

    return Result;
}

/*********************************************************************

    Function:       SendResetSignal

    Description:    performs hardware reset on the Device Chain

*********************************************************************/
static void SendResetSignal(u32 sense)
{
    ADI_ATA_CTRL_DEV_RST_SET(*pADI_ATA_CTRL,sense);
}

/*********************************************************************

    Function:       SelectDrive

    Description:    Select the Appropriate Drive

*********************************************************************/
static u32 SelectDrive(ADI_ATAPI_DEF *pDevice, u32 Drive)
{
    u16 Status, Error, cdh = 0x00;
    u32 Result = ADI_FSS_RESULT_SUCCESS;

   /* pogram CDH for the required drive number */
    ADI_ATA_DEV_CDH_DRIVE_SET ( cdh, Drive );
    WriteDeviceRegister( pDevice,  ADI_ATA_CDH_OFFSET, cdh );
    return Result;
}

/*********************************************************************

    Function:       SetDataFlow

    Description:    Starts/Stops dataflow

*********************************************************************/
static void SetDataFlow(ADI_ATAPI_DEF *pDevice, u32 EnableFlag)
{
    /* Determine the actual number of words transfered */
    u16 type = ADI_ATA_TRANSFER_TYPE_GET(pDevice->Device[pDevice->CurrentDrive].TransferMode);

    if (EnableFlag && ADI_ATAPI_IS_XFER_IN_PROGESS(pDevice->StateFlag))
    {
        PrepareForTransfer(pDevice);

        switch (type) {
            case (ADI_ATA_MODE_UDMA):
                ADI_ATA_CTRL_UDMA_START_SET ( *pADI_ATA_CTRL, 1 );
                break;
            case (ADI_ATA_MODE_MDMA):
                ADI_ATA_CTRL_MDMA_START_SET ( *pADI_ATA_CTRL, 1 );
                break;
            case (ADI_ATA_MODE_PIO):
                ADI_ATA_CTRL_PIO_START_SET  ( *pADI_ATA_CTRL, 1 );
                break;
        }
    }
}

/*********************************************************************

    Function:       ProcessInterrupt

    Description:    This function is called via the ADI_FSS_CMD_PROCESS_CALLBACK
                    command from the FSS callback function

*********************************************************************/
static void ProcessInterrupt(ADI_ATAPI_DEF *pDevice, u32 Event, ADI_FSS_SUPER_BUFFER *pSuperBuffer)
{
    u16                         type;
    ADI_PID_LBA_REQUEST_ENTRY   NextRequest;
    u32                         Result;
    void                        *pExitCritRegionArg;
    ADI_DEV_1D_BUFFER           *pBuffer = (ADI_DEV_1D_BUFFER*)pSuperBuffer;

    type = ADI_ATA_TRANSFER_TYPE_GET(pDevice->Device[pDevice->CurrentDrive].TransferMode);
    
    if ( ADI_ATAPI_IS_XFER_DONE(pDevice->StateFlag) )
    {
        /* Determine the actual number of words transfered */
        switch (type) {
            case (ADI_ATA_MODE_UDMA):
                if ( ADI_ATA_INT_STATUS_UDMAIN_DONE_MASK & pDevice->InterruptMask )
                {
                    pBuffer->ProcessedElementCount = ADI_ATA_UDMAIN_TFRCNT_GET(*pADI_ATA_UDMAIN_TFRCNT);
                }
                else
                {
                    pBuffer->ProcessedElementCount = ADI_ATA_UDMAOUT_TFRCNT_GET(*pADI_ATA_UDMAOUT_TFRCNT);
                }
                break;
            case (ADI_ATA_MODE_MDMA):
                pBuffer->ProcessedElementCount = ADI_ATA_DMA_TFRCNT_GET(*pADI_ATA_DMA_TFRCNT);
                if (ADI_ATA_INT_STATUS_MDMA_TERM_GET(*pADI_ATA_INT_STATUS) )
                {
                    ADI_ATA_INT_STATUS_MDMA_TERM_SET( *pADI_ATA_INT_STATUS, 1 );
                }
                break;
            case (ADI_ATA_MODE_PIO):
                pBuffer->ProcessedElementCount = ADI_ATA_PIO_TFRCNT_GET(*pADI_ATA_PIO_TFRCNT);
                break;
                
                
        }

        pDevice->StateFlag = ADI_ATAPI_STATE_XFER_IDLE;
#ifdef __DEBUG_SM__
        StateFlag = pDevice->StateFlag;
#endif

        if (pSuperBuffer->SemaphoreHandle == pDevice->DataSemaphoreHandle )
        {
            adi_dev_Control( pDevice->DeviceHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL );
            adi_sem_Post( pSuperBuffer->SemaphoreHandle );
        }
        
    }

}

/*********************************************************************

    Function:       WaitOnTransferComplete

    Description:    Waits on the requested bit in the ATA_STATUS
                    register to clear.

*********************************************************************/
static void WaitOnTransferComplete(u16 TransferBitMask)
{
    u16 Status;
    do {
        Status = *pADI_ATA_STATUS;
    } while ( (Status&TransferBitMask)==TransferBitMask );
}



/*********************************************************************

    Function:       WriteDeviceRegister

    Description:    Writes to ATA Device Resgister

*********************************************************************/

static u32 WriteDeviceRegister( ADI_ATAPI_DEF *pDevice, u16 RegisterOffset, u16 Value)
{
    u32 Result = ADI_FSS_RESULT_SUCCESS;

    /* Program the ATA_DEV_TXBUF register with write data (to be
     * written into the device).
    */
    *pADI_ATA_DEV_TXBUF = Value;

    /* Program the ATA_DEV_ADDR register with address of the
     * device register (0x01 to 0x0F).
    */
    ADI_ATA_DEV_ADDR_DEV_ADDR_SET(*pADI_ATA_DEV_ADDR,RegisterOffset);

    //ADI_ATA_XFER_LEN_SET  ( *pADI_ATA_XFER_LEN, 1 );
    /* Program the ATA_CTRL register with dir set to write (1)
    */
    ADI_ATA_CTRL_XFER_DIR_SET  ( *pADI_ATA_CTRL, 1 );

    /* ensure PIO DMA is not set */
    ADI_ATA_CTRL_PIO_USE_DMA_SET  ( *pADI_ATA_CTRL, 0 );

    /* and start the transfer */
    ADI_ATA_CTRL_PIO_START_SET ( *pADI_ATA_CTRL, 1 );

    /* Wait for the interrupt to indicate the end of the transfer.
    * (We need to wait on and clear rhe ATA_DEV_INT interrupt status)
    */
    //while ( ADI_ATA_STATUS_PIO_XFER_ON_GET(*pADI_ATA_STATUS) );

    Result = WaitOnInterrupt(pDevice,ADI_ATA_INT_STATUS_PIO_DONE_MASK, ADI_ATA_INT_STATUS_PIO_DONE_SHIFT);

    return Result;

}

/*********************************************************************

    Function:       ReadDeviceRegister

    Description:    Reads from ATA Device Resgister

*********************************************************************/

static u32 ReadDeviceRegister( ADI_ATAPI_DEF *pDevice, u16 RegisterOffset, u16 *Value)
{
    u32 Result = ADI_FSS_RESULT_SUCCESS;

    /* Program the ATA_DEV_ADDR register with address of the
     * device register (0x01 to 0x0F).
    */
    ADI_ATA_DEV_ADDR_DEV_ADDR_SET(*pADI_ATA_DEV_ADDR,RegisterOffset);

    /* Program the ATA_CTRL register with dir set to read (1) and
     * pio_start set to 1.
    */
    ADI_ATA_CTRL_XFER_DIR_SET  ( *pADI_ATA_CTRL, 0 );

    /* ensure PIO DMA is not set */
    ADI_ATA_CTRL_PIO_USE_DMA_SET  ( *pADI_ATA_CTRL, 0 );

    /* and start the transfer */
    ADI_ATA_CTRL_PIO_START_SET ( *pADI_ATA_CTRL, 1 );

    /* Wait for the interrupt to indicate the end of the transfer.
     * (PIO_DONE interrupt is set and it doesn't seem to matter that we don't clear it)
    */
    while ( ADI_ATA_STATUS_PIO_XFER_ON_GET(*pADI_ATA_STATUS) );
    Result = WaitOnInterrupt(pDevice,ADI_ATA_INT_STATUS_PIO_DONE_MASK, ADI_ATA_INT_STATUS_PIO_DONE_SHIFT);

    /* Read the ATA_DEV_RXBUF register with write data (to be
     * written into the device).
    */
    *Value = *pADI_ATA_DEV_RXBUF;

    return Result;

}

/*********************************************************************

    Function:       SetTimingRegs

    Description:    Set up the timing regs

*********************************************************************/
static void SetTimingRegs ( ADI_ATAPI_DEF *pDevice, u32 Drive )
{
    if ( !pDevice->ForceMDMA && !pDevice->ForcePIO
        && ADI_ATA_MAX_CUR_UDMA_MODE_GET(pDevice->Device[Drive].TransferMode)!=ADI_ATA_MODE_UNSET )
    {
        /* Program the UDMA timing registers accordingly */
        SetUDMATimingRegs( &pDevice->Device[Drive] );
    }
    else if ( !pDevice->ForcePIO
             && ADI_ATA_MAX_CUR_MDMA_MODE_GET(pDevice->Device[Drive].TransferMode)!=ADI_ATA_MODE_UNSET )
    {
        /* Program the MDMA timing registers accordingly */
        SetMDMATimingRegs( &pDevice->Device[Drive] );
    }
    else if ( ADI_ATA_MAX_CUR_PIO_MODE_GET(pDevice->Device[Drive].TransferMode)!=ADI_ATA_MODE_UNSET )
    {
        /* Program the PIO timing registers */
        SetPIOTimingRegs( &pDevice->Device[Drive] );
    }
}

/*********************************************************************

    Function:       WriteTransferMode

    Description:    This function is called via the ADI_PID_CMD_CLEAR_INT
                    command from the FSS callback function

*********************************************************************/

static u32 WriteTransferMode(ADI_ATAPI_DEF *pDevice, u32 Drive )
{
    u32 Result;

    /* Extract type and mode level */
    u16 type = ADI_ATA_TRANSFER_TYPE_GET(pDevice->Device[Drive].TransferMode);
    u16 mode = ADI_ATA_TRANSFER_MODE_GET(pDevice->Device[Drive].TransferMode);

    /* for PIO Modes 3 & above set IORDY */
    if ( pDevice->Device[Drive].SupportsIORDY && (type==ADI_ATA_MODE_PIO) && (mode >= 3)  )
    {
        ADI_ATA_CTRL_IORDY_EN_SET( *pADI_ATA_CTRL, 1 );
    }

    /* Write SET FEATURES/transfer mode command */
    Result = WriteDeviceCommand (
                            pDevice,
                            ADI_ATA_CMD_SET_FEATURES,
                            Drive,
                            ( (type<<3) | mode ),
                            ADI_ATA_FEAT_TRANFER_MODE,
                            0
                        );

    /* unmask device interrupt and wait for interrupt to be taken */
    ADI_ATA_INT_MASK_ATA_DEV_INT_MASK_SET( *pADI_ATA_INT_MASK, 1 );
    WaitOnInterrupt(pDevice,ADI_ATA_INT_STATUS_ATA_DEV_INT_MASK, ADI_ATA_INT_STATUS_ATA_DEV_INT_SHIFT);

    return Result;

}

/*********************************************************************

    Function:       WaitOnInterrupt

    Description:    Waits on the appropriate interrupt from the Device

*********************************************************************/
static u32 WaitOnInterrupt(ADI_ATAPI_DEF *pDevice, u16 IntMask, u16 BitPos)
{
    u32 Result = ADI_FSS_RESULT_SUCCESS;
    u16 Status;

    do {
        Status = *pADI_ATA_INT_STATUS;
        Status = (Status&IntMask) >> BitPos ;
    } while( !Status );

    /* clear interrupt */
    *pADI_ATA_INT_STATUS |= (1<<BitPos);

    return Result;
}

/*********************************************************************

    Function:       WaitOnBusy

    Description:    Waits on the required conditions being set in the
                    Device Status register.

*********************************************************************/
static u32 WaitOnBusyWithTimeOut( ADI_ATAPI_DEF *pDevice, u8 bsy, u32 Count )
{
    u32 Result = ADI_FSS_RESULT_SUCCESS;
    u16 Status;

    /* We don't want to handle the device interrupt here */
    adi_int_SICDisable(pDevice->DevIntPeripheralID);

    do {
        ReadDeviceRegister( pDevice,  ADI_ATA_STAT_OFFSET, &Status );
    } while( ADI_ATA_DEV_STATUS_BSY_GET(Status)!=bsy );

    return Result;
}


/*********************************************************************

    Function:       WaitOnBusy

    Description:    Waits on the required conditions being set in the
                    Device Status register.

*********************************************************************/
static u32 WaitOnBusy( ADI_ATAPI_DEF *pDevice, u8 bsy )
{
    u32 Result = ADI_FSS_RESULT_SUCCESS;
    u16 Status;

    /* We don't want to handle the device interrupt here */
    adi_int_SICDisable(pDevice->DevIntPeripheralID);

    do {
        ReadDeviceRegister( pDevice,  ADI_ATA_STAT_OFFSET, &Status );
    } while( ADI_ATA_DEV_STATUS_BSY_GET(Status)!=bsy );

    return Result;
}

#define ADI_ATAPI_IS_TERM_INT(V) ( (V)&(ADI_ATA_INT_STATUS_MDMA_TERM_MASK|ADI_ATA_INT_STATUS_UDMAIN_TERM_MASK|ADI_ATA_INT_STATUS_UDMAOUT_TERM_MASK) )

/*********************************************************************

    Function:       DeviceInterruptHandler

    Description:    Handles the ATA_DEV_INT interrupt

*********************************************************************/
static ADI_INT_HANDLER(DeviceInterruptHandler)    // DMA error handler
{

    u32                     i;
    u16                     Status, Error;
    ADI_INT_HANDLER_RESULT  Result;
    u32                     TransferredElementCount;
    u32                     RequestedElementCount;
    ADI_ATAPI_DEF           *pDevice = (ADI_ATAPI_DEF*)ClientArg;
    u16                     IntrStatus;
    u16                     ProcessedIntr = 0;
    u16                     PendingIntr;
    u16                     IntrMask;
    u16                     IntrShift;
    bool                    bPIODone = false;
    bool                    bDmaTermInt = false;

    Result = ADI_INT_RESULT_NOT_PROCESSED;
    
    /* Read the current interrupt status */
    PendingIntr = IntrStatus = *pADI_ATA_INT_STATUS;

    /* clear the interrupt immediately, so that any subsequent unhandled 
     * interrupt is not inadvertently cleared
    */
    //*pADI_ATA_INT_STATUS |= IntrStatus;
        
#ifdef __DEBUG_SM__
        SET_DEV_INT_STATUS(IntrStatus)     
#endif

    /* Process all currently asserted interrupts 
    */
    while( PendingIntr )
    {
        
        RequestedElementCount = *pADI_ATA_XFER_LEN;
        
        /* *************************************************
         * MDMA TERM
         * *************************************************
        */
        if ( ADI_ATA_INT_STATUS_MDMA_TERM_GET(PendingIntr) )
        {
            TransferredElementCount = *pADI_ATA_DMA_TFRCNT;
            Result = ADI_INT_RESULT_PROCESSED;
            IntrMask = ADI_ATA_INT_STATUS_MDMA_TERM_MASK;
            IntrShift = ADI_ATA_INT_STATUS_MDMA_TERM_SHIFT;
        }
        
            
        /* *************************************************
         * MDMA DONE
         * *************************************************
        */
        else if ( ADI_ATA_INT_STATUS_MDMA_DONE_GET(PendingIntr) )
        {
            TransferredElementCount = *pADI_ATA_DMA_TFRCNT;
            pDevice->StateFlag |= ADI_ATAPI_STATE_DMA_INT;
#ifdef __DEBUG_SM__
            StateFlag = pDevice->StateFlag;
#endif
            IntrMask = ADI_ATA_INT_STATUS_MDMA_DONE_MASK;
            IntrShift = ADI_ATA_INT_STATUS_MDMA_DONE_SHIFT;
            Result = ADI_INT_RESULT_PROCESSED;
        }
        
        /* *************************************************
         * UDMA IN TERM
         * *************************************************
        */
        else if ( ADI_ATA_INT_STATUS_UDMAIN_TERM_GET(PendingIntr))
        {
            TransferredElementCount = *pADI_ATA_UDMAIN_TFRCNT;
            IntrMask = ADI_ATA_INT_STATUS_UDMAIN_TERM_MASK;
            IntrShift = ADI_ATA_INT_STATUS_UDMAIN_TERM_SHIFT;
            Result = ADI_INT_RESULT_PROCESSED;
        }
        /* *************************************************
         * UDMA IN DONE
         * *************************************************
        */
        else if ( ADI_ATA_INT_STATUS_UDMAIN_DONE_GET(PendingIntr) )
        {
            TransferredElementCount = *pADI_ATA_UDMAIN_TFRCNT;
#if 0            
            pDevice->StateFlag |= ADI_ATAPI_STATE_DMA_INT;
#ifdef __DEBUG_SM__
            StateFlag = pDevice->StateFlag;
#endif
#endif
            IntrMask = ADI_ATA_INT_STATUS_UDMAIN_DONE_MASK;
            IntrShift = ADI_ATA_INT_STATUS_UDMAIN_DONE_SHIFT;
            Result = ADI_INT_RESULT_PROCESSED;
        }
        
        /* *************************************************
         * UDMA OUT TERM
         * *************************************************
        */
        else if ( ADI_ATA_INT_STATUS_UDMAOUT_TERM_GET(PendingIntr)  )
        {
            bDmaTermInt = true;
            TransferredElementCount = *pADI_ATA_UDMAOUT_TFRCNT;
            IntrMask = ADI_ATA_INT_STATUS_UDMAOUT_TERM_MASK;
            IntrShift = ADI_ATA_INT_STATUS_UDMAOUT_TERM_SHIFT;
            Result = ADI_INT_RESULT_PROCESSED;
        }
        
        /* *************************************************
         * UDMA OUT DONE
         * *************************************************
        */
        else if ( ADI_ATA_INT_STATUS_UDMAOUT_DONE_GET(PendingIntr) )
        {
#if 0            
            pDevice->StateFlag |= ADI_ATAPI_STATE_DMA_INT;
#ifdef __DEBUG_SM__
            StateFlag = pDevice->StateFlag;
#endif
#endif
            TransferredElementCount = *pADI_ATA_UDMAOUT_TFRCNT;
            IntrMask = ADI_ATA_INT_STATUS_UDMAOUT_DONE_MASK;
            IntrShift = ADI_ATA_INT_STATUS_UDMAOUT_DONE_SHIFT;
            Result = ADI_INT_RESULT_PROCESSED;
        }

        /* *************************************************
         * HOST TERM 
         * *************************************************
        */
        else if ( ADI_ATA_INT_STATUS_HOST_TERM_XFER_GET(PendingIntr) )
        {
            Result = ADI_INT_RESULT_PROCESSED;
            /* Not sure whether anything is needed here yet */
        }
            
        /* *************************************************
         * UDMA OUT DONE
         * *************************************************
        */
        else if ( ADI_ATA_INT_STATUS_PIO_DONE_GET(PendingIntr) )
        {
            bPIODone = true;
            pDevice->StateFlag |= ADI_ATAPI_STATE_DMA_INT;
            pDevice->StateFlag |= ADI_ATAPI_STATE_DEV_INT;
#ifdef __DEBUG_SM__
            StateFlag = pDevice->StateFlag;
#endif
            TransferredElementCount = *pADI_ATA_PIO_TFRCNT;
            IntrMask = ADI_ATA_INT_STATUS_PIO_DONE_MASK;
            IntrShift = ADI_ATA_INT_STATUS_PIO_DONE_SHIFT;
            Result = ADI_INT_RESULT_PROCESSED;
        }
            
        /* *************************************************
         * DEVICE INTERRUPT
         * *************************************************
        */
        else if ( ADI_ATA_INT_STATUS_ATA_DEV_INT_GET(PendingIntr) )
        {
            TransferredElementCount = RequestedElementCount;
            pDevice->StateFlag |= ADI_ATAPI_STATE_DEV_INT;
#ifdef __DEBUG_SM__
            StateFlag = pDevice->StateFlag;
#endif
            IntrMask = ADI_ATA_INT_STATUS_ATA_DEV_INT_MASK;
            IntrShift = ADI_ATA_INT_STATUS_ATA_DEV_INT_SHIFT;
            ADI_ATA_INT_STATUS_ATA_DEV_INT_CLR(IntrStatus,0); 
            Result = ADI_INT_RESULT_PROCESSED;
        }
                
        /* clear from pending set */
        ADI_ATA_INT_STATUS_CLR(PendingIntr,IntrShift);              
        /* add to processed set */
        ADI_ATA_INT_STATUS_SET(ProcessedIntr,IntrMask,IntrShift);              
        /* get current status */
        IntrStatus = *pADI_ATA_INT_STATUS;
        /* remove processed from pending set */
        PendingIntr = IntrStatus & ~ProcessedIntr;
        
    /* END WHILE (Interrupt asserted) */
    }
    
    /* Clear interrupt */
    *pADI_ATA_INT_STATUS |= ProcessedIntr;
    
    if ( Result == ADI_INT_RESULT_PROCESSED )
    {
        /* If the transferred count is different to that requested,
         * adjust the XFER LEN register to the remainig count 
         * and restart
        */
        if ( TransferredElementCount < RequestedElementCount )
        {
            RequestedElementCount -= TransferredElementCount;

            ADI_ATA_XFER_LEN_SET( *pADI_ATA_XFER_LEN, RequestedElementCount  );
            SetDataFlow(pDevice, 1);
        }            
        else
        {
            /* Transfer is complete so process the interrupt 
            */
           (pDevice->DMCallback)( pDevice->DeviceHandle, ADI_PID_EVENT_DEVICE_INTERRUPT, (void*)pDevice->CurrentLBARequest.pBuffer );
        }
        
    }

    return Result;

}    


/*********************************************************************

    Function:       POSTFunction

    Description:    Performs a Power On Self Test procedure to determine
                    whether the basic read/write funcitonality works.
                    Sector 2 is written to, which should not affect the
                    format on a Hard disk drive.

*********************************************************************/
static u32 POSTFunction(ADI_ATAPI_DEF *pDevice, u32 Drive)
{
    u32 Result;
    u16 *pSectorData = (u16*)_adi_fss_MallocAligned( pDevice->CacheHeapID, 512);
    if (!pSectorData)
    {
        Result = ADI_FSS_RESULT_NO_MEMORY;
    }
    else {
        u32 i;
        /* initialize sector buffer */
        for (i=0;i<256;i++) {
            pSectorData[i] = i;
        }
        /* write to sector 2 on the media */
        Result = TransferSectors(pDevice, Drive, 2, 1, pSectorData, WRITE);
        if (Result == ADI_FSS_RESULT_SUCCESS) {
            /* clear sector buffer */
            for (i=0;i<256;i++) {
                pSectorData[i] = 0;
            }
            /* read back sector 2 from the media */
            Result = TransferSectors(pDevice, Drive, 2, 1, pSectorData, READ);
            if (Result == ADI_FSS_RESULT_SUCCESS) {
                /* verify sector buffer */
                for (i=0; (i<256) && (Result==ADI_FSS_RESULT_SUCCESS); i++)
                {
                    if ( pSectorData[i] != i ) {
                        Result = ADI_FSS_RESULT_FAILED;
                    }
                }
            }
        }

    }

    _adi_fss_FreeAligned( pDevice->CacheHeapID, pSectorData );

    return Result;
}

#ifdef _DEV_PURPOSES_ONLY /* For development purposes only */

#include <stdio.h>
char *mode_str[] = { "Programmed I/O", "Multiword DMA", "Ultra DMA" };

void ATAPI_DisplayTransferMode(FILE *fp)
{
    u16 type = ADI_ATA_TRANSFER_TYPE_GET(CurrentTransferMode);
    u16 mode = ADI_ATA_TRANSFER_MODE_GET(CurrentTransferMode);

    fprintf( fp, "%s mode %d",  mode_str[type>>2], mode );

}
#endif

/********************************************************************

    Function:       WriteAndCheckPattern

    Description:    Send pattern to device and read back to check 

********************************************************************/

static bool WriteAndCheckPattern( ADI_ATAPI_DEF *pDevice, u8 *pattern )
{
    u16 Status;
    /* Write to the device registers with ascending value */
    WriteDeviceRegister( pDevice, ADI_ATA_SCNT_OFFSET, pattern[0] );
    WriteDeviceRegister( pDevice, ADI_ATA_SNUM_OFFSET, pattern[1] );
    WriteDeviceRegister( pDevice, ADI_ATA_CYLL_OFFSET, pattern[2] );
    WriteDeviceRegister( pDevice, ADI_ATA_CYLH_OFFSET, pattern[3] );
    
    /* read them back */
    ReadDeviceRegister( pDevice, ADI_ATA_SCNT_OFFSET, &Status );
    if ((Status&0x00FF) != pattern[0]) 
        return false;
    ReadDeviceRegister( pDevice, ADI_ATA_SNUM_OFFSET, &Status );
    if ((Status&0x00FF) != pattern[1]) 
        return false;
    ReadDeviceRegister( pDevice, ADI_ATA_CYLL_OFFSET, &Status );
    if ((Status&0x00FF) != pattern[2]) 
        return false;
    ReadDeviceRegister( pDevice, ADI_ATA_CYLH_OFFSET, &Status );
    if ((Status&0x00FF) != pattern[3]) 
        return false;
        
    return true;
}
/********************************************************************

    Function:       InitializeDevice

    Description:    Send pattern to device to initialize 

********************************************************************/
static bool InitializeDevice(ADI_ATAPI_DEF *pDevice, u16 Drive)
{
    bool    Result        = false;
    u8      patternInc[4] = { 0x01, 0x02, 0x03, 0x04};
    u8      pattern55[4]  = { 0x55, 0x55, 0x55, 0x55};
    u8      patternAA[4]  = { 0xAA, 0xAA, 0xAA, 0xAA};
    
    /* Change the active drive and set timing registers */
    pDevice->CurrentDrive = Drive;

    /* Set Drive Number */
    WriteDeviceRegister( pDevice, ADI_ATA_CDH_OFFSET, Drive?0x10:0x00 );

    /* Wait 10 ms */
    _adi_fss_WaitMilliSec(10);

    /* Write & read back incrementing pattern */
    if (WriteAndCheckPattern(pDevice, patternInc) )
    {
        /* Write & read back 0x55 to all registers */
        if (WriteAndCheckPattern(pDevice, pattern55) )
        {
            /* Write & read back 0xAA to all registers */
            if (WriteAndCheckPattern(pDevice, patternAA) )
            {
                Result = true;
            }      
        }
    }

    return Result;
}


#endif /* __ADI_ATAPI_C__ */


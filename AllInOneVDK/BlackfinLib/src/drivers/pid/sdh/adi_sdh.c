#define _MMC_STOP_TRANSMISSION_WORKAROUND__
/**********************************************************************

Copyright(c) 2009 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_sdh.c,v $
$Revision: 3578 $
$Date: 2010-10-19 09:36:04 -0400 (Tue, 19 Oct 2010) $

Description:
    This is the driver source code for ADI Secure Digital Host (SDH)

***********************************************************************/

/* system service header  */
#include <services/services.h>

/* SDH is available only on BF54x & BF51x family */
#if !defined(__ADSP_MOAB__) && !defined(__ADSP_BRODIE__) && !defined(__ADSP_MOY__)
#error "Removable Storage Interface (SDH) is available only on ADSP-BF54x, ADSP-BF51x & ADSP-BF50x processors"
#endif

/* potential code and data mappings */
#if 0
#define __ADI_SDH_SECTION_CODE          section("adi_sdh_code")
#define __ADI_SDH_SECTION_DATA          section("adi_sdh_data")
#else
#define __ADI_SDH_SECTION_CODE
#define __ADI_SDH_SECTION_DATA
#endif

/* common code for guarding code against interrupts */
#define BEGIN_CRITICAL_REGION \
    { void *pExitCriticalArg = adi_int_EnterCriticalRegion(pDevice->pEnterCriticalArg);
#define END_CRITICAL_REGION   \
    adi_int_ExitCriticalRegion(pExitCriticalArg); }

/*********************************************************************

Include files

*********************************************************************/

/* SDH driver includes */
#include <drivers/pid/sdh/adi_sdh.h>
/* ATA Partition Table Structure & Partition type */
#include <drivers/pid/adi_ata.h>
/* Memory Copy etc. */
#include <string.h>
/* SDH MMR definitions and bitmask access macros */
#include "adi_sdh_reg.h"

/*********************************************************************

Macros specific to SDH command configuration register for selected
SD/SDIO/Multimedia card commands

*********************************************************************/

/* For commands that require short responses    */
#define     ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE           0x0040
/* For commands that require long responses     */
#define     ADI_SDH_SD_MMC_CMD_LONG_RESPONSE            0x00C0
/* Use interrupt to monitor command/response transfer   */
#define     ADI_SDH_SD_MMC_CMD_USE_INTERRUPT            0x0100
/* Wait for CmdPend before sending a command    */
#define     ADI_SDH_SD_MMC_CMD_WAIT_ON_CMD_PEND         0x0200
/* Enable Commands */
#define     ADI_SDH_SD_MMC_CMD_ENABLE_COMMANDS          0x0400

/*********************************************************************

Macros specific to SDH data control register related operations

*********************************************************************/

/* Data transfer direction - from card to controller, block length = 512 bytes,
                             enable DMA, block transfer mode, Enable data transfer */
#define     ADI_SDH_READ_DATA_FROM_SD_MMC               0x009B
/* Data transfer direction - from controller to card, block length = 512 bytes,
                             enable DMA, block transfer mode, Enable data transfer */
#define     ADI_SDH_WRITE_DATA_TO_SD_MMC                0x0099

/*********************************************************************

Macros specific to SD/SDIO/Multimedia card operation

*********************************************************************/

/* SDH Clock frequency for card identification mode (300kHz) */
#define     ADI_SDH_SD_MMC_SDCLK_ID_MODE                300000
/* #times to retry card identification process before exiting on Card Operating Condition - busy flag */
#define     ADI_SDH_SD_MMC_IDENTIFY_RETRY_COUNT         0xFFFF
/* Retry count for Memory Device Access Ready status */
#define     ADI_SDH_SD_MMC_ACCESS_RETRY_COUNT           0xFFFF
/* Stuff bits for SD command argument   */
#define     ADI_SDH_SD_MMC_STUFF_BITS                   0
/* Send Card Interface Condition (only for SDHC) - Voltage Host supplied (VHS) ID for 2.7 to 3.6V   */
#define     ADI_SDH_SDHC_IFACE_COND_VHS1                0x00000100
/* Send Card Interface Condition (only for SDHC) - recommended check pattern value */
#define     ADI_SDH_SDHC_IFACE_COND_CHECK_PATTERN       0x000000AA
/* Required Voltage range to support SDHC type cards (for 2.7 to 3.6V) */
#define     ADI_SDH_SDHC_REQUIRED_VOLTAGE_RANGE1        0x00FF8000
/* Card Operating Condition - Busy flag mask for SD/MMC */
#define     ADI_SDH_SD_MMC_OCR_BUSY_MASK                0x80000000
/* Card Operating Condition - Voltage Window mask for SD/SDIO/SDHC/MMC */
#define     ADI_SDH_SD_MMC_OCR_VOLTAGE_WINDOW_MASK      0x00FFFFFF
/* Card Operating Condition - Host Capacity Status/Card Capacity Status mask for SDHC & MMCHC*/
#define     ADI_SDH_SDHC_MMCHC_OCR_HCS_CCS_MASK         0x40000000
/* Card Operating Condition - Busy flag mask for SDIO */
#define     ADI_SDH_SDIO_OCR_BUSY_MASK                  0x08000000
/* Card Operating Condition - Operating voltage mask   */
#define     ADI_SDH_SD_MMC_OCR_VOLTAGE_MASK             0xFF000000
/* card identification mode - no more id command required */
#define     ADI_SDH_SD_MMC_ID_MODE_NO_CMD               0xFF
/* Relative Card Address for Multimedia type card (bits 31 to 16 - vaild RCA, bits 15 to 0 - stuff bits */
#define     ADI_SDH_MMC_RCA_VALUE                       0xADAD0000
/* Mask value to extract Relative Card Address */
#define     ADI_SDH_MMC_RCA_MASK                        0xFFFF0000
/* Mask value to Identify SDIO Combo card */
#define     ADI_SDH_SDIO_MEMORY_SUPPORT                 0x00080000
/* Check if the SDIO card provides atleast 1 IO support */
#define     ADI_SDH_SDIO_IO_SUPPORT                     0x00700000
/* Extract TAAC access time - time exponent information from CSD (Version 1.X or 2.0) */
#define     ADI_SDH_EXTRACT_TAAC_TIME_EXPONENT          ((*pADI_SDH_RESPONSE_0 >> 16) & 0x7)
/* Extract TAAC access time - time value information from CSD (Version 1.X) */
#define     ADI_SDH_EXTRACT_TAAC_TIME_VALUE             ((*pADI_SDH_RESPONSE_0 >> 19) & 0xF)
/* Extract NSAC access time from CSD (Version 1.0 or 2.0) */
#define     ADI_SDH_EXTRACT_NSAC_VALUE                  ((*pADI_SDH_RESPONSE_0 >> 8) & 0xFF)
/* Extract Transfer speed - Transfer Rate exponent information from CSD (Version 1.X) */
#define     ADI_SDH_CARD_TRANSFER_RATE_EXPONENT         (*pADI_SDH_RESPONSE_0 & 0x07)
/* Extract Transfer speed - Time mantissa information from CSD (Version 1.X) */
#define     ADI_SDH_CARD_TRANSFER_TIME_MANTISSA         ((*pADI_SDH_RESPONSE_0 & 0x78) >> 3)
/* Extract C_SIZE info from CSD Version 1.0*/
#define     ADI_SDH_CARD_C_SIZE_VER_1_X                 (((*pADI_SDH_RESPONSE_1 & 0x3FF) << 2) | (*pADI_SDH_RESPONSE_2 >> 30))
/* Extract C_SIZE info from CSD Version 2.0*/
#define     ADI_SDH_CARD_C_SIZE_VER_2_0                 (((*pADI_SDH_RESPONSE_1 & 0x3F) << 16) | (*pADI_SDH_RESPONSE_2 >> 16))
/* Extract Card Command Classes (CCC) information from CSD (Version 1.X or 2.0) */
#define     ADI_SDH_EXTRACT_CCC                         (*pADI_SDH_RESPONSE_1 >> 20)
/* Extract Read Block Length info from CSD (Version 1.0 & 2.0) */
#define     ADI_SDH_CARD_READ_BLOCK_LEN                 ((*pADI_SDH_RESPONSE_1 >> 16) & 0xF)

/* Extract Read-to-Write factor info from CSD (Version 1.0 & 2.0) */
#if defined(__ADSP_MOAB__)
#define     ADI_SDH_EXTRACT_R2W_FACTOR                  ((*pADI_SDH_RESPONSE_3 >> 26) & 0x7)
#elif defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
/* Brodie shifts RESPONSE3 right by one bit. */
#define     ADI_SDH_EXTRACT_R2W_FACTOR                  ((*pADI_SDH_RESPONSE_3 >> 25) & 0x7)
#endif

/* Extract Write Block Length info from CSD (Version 1.0 & 2.0) */
#if defined(__ADSP_MOAB__)
#define     ADI_SDH_CARD_WRITE_BLOCK_LEN                ((*pADI_SDH_RESPONSE_3 >> 22) & 0xF)
#elif defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
/* Brodie shifts RESPONSE3 right by one bit. */
#define     ADI_SDH_CARD_WRITE_BLOCK_LEN                ((*pADI_SDH_RESPONSE_3 >> 21) & 0xF)
#endif
/* Extract C_SIZE_MULT info from CSD (Version 1.0) */
#define     ADI_SDH_CARD_C_SIZE_MULT_VER_1_X            ((*pADI_SDH_RESPONSE_2 >> 15) & 0x7)

/* Extract Card Write Protect information from CSD */
#if defined(__ADSP_MOAB__)
#define     ADI_SDH_IS_CARD_WRITE_PROTECTED             (*pADI_SDH_RESPONSE_3 & 0x3000)
#elif defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
#define     ADI_SDH_IS_CARD_WRITE_PROTECTED             (*pADI_SDH_RESPONSE_3 & 0x1800)
#endif
/*
   Card Status bit that indicates whether the
   Memory Device is ready to Tx/Rx next chunk of data
   Make sure the card is in transfer state & ready for data before
   issuing next data transfer command
*/
/* MMC/SD status must match this value before issuing next data transfer */
#define     ADI_SDH_SD_MMC_READY_FOR_DATA               (0x900U)
/* Extract ready for data status */
#define     ADI_SDH_SD_MMC_READY_FOR_DATA_STATUS        (*pADI_SDH_RESPONSE_0 & 0x00001F00U)

/*********************************************************************

Enums to identify SDH Bus state

*********************************************************************/
__ADI_SDH_SECTION_DATA
typedef enum {
    /* SDH Bus is idle                                      */
    ADI_SDH_BUS_IDLE                = 0x00,
    /* SDH Bus is transfering data                          */
    ADI_SDH_BUS_XFER_IN_PROGRESS    = 0x01,
    /* SDH Data transfer complete - Device interrupt occured*/
    ADI_SDH_BUS_DEV_INT_XFER_DONE   = 0x02,
    /* SDH Data transfer complete - DMA interrupt occured   */
    ADI_SDH_BUS_DMA_INT_XFER_DONE   = 0x04,
    /* SDH device and SDH DMA data transfer complete        */
    ADI_SDH_BUS_XFER_COMPLETE       = ( ADI_SDH_BUS_XFER_IN_PROGRESS    |
                                        ADI_SDH_BUS_DEV_INT_XFER_DONE   |
                                        ADI_SDH_BUS_DMA_INT_XFER_DONE),
    /* SDH Data transfer fail - Device interrupt occured    */
    ADI_SDH_BUS_DEV_INT_XFER_FAILED = 0x08,
    /* SDH Data transfer fail - DMA interrupt occured       */
    ADI_SDH_BUS_DMA_INT_XFER_FAILED = 0x10,

}ADI_SDH_BUS_STATE;


/*********************************************************************

Structure to hold SD/SDIO/Multimedia card specific information

*********************************************************************/

unsigned char tempBuffer[512];
unsigned long AndyVar = 0;
typedef struct
{
    u8      CardLocked;         /* TRUE when the mechanical switch in SD/MMC/SDIO card is in Lock state */
    u8      WriteProtect;       /* Card Write protected when TRUE - value read from CSD register        */
    u8      EnableWideBus;      /* TRUE = 4-bit bus(if supported), FALSE = 1-bit bus                    */
#if defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
    u8      EnableByteBus;      /* TRUE = 8-bit bus(if supported), FALSE = 1-bit bus                    */
#endif
    u8      BlockLength;        /* Max Read/Write block length (2^N bytes)                              */
    u16     CardCmdClasses;     /* Command Classes supported by this card                               */
    u32     TotalBlocks;        /* Total number of blocks in this device                                */
    u32     OperatingFreq;      /* Present SDH Clock frequency                                          */
    u32     OperatingCondition; /* Supported operating conditions                                       */
    u32     CardIfaceCondition; /* Card Interface condition                                             */
    u32     RCA;                /* Relative Card Address (bits 31:16), stuff bits (15:0)                */
    u32     CardTxTimeOut;      /* Maximum SD CLKs for SD Data transfer timeout                         */
    u32     TxTimeOutMultiplier;/* Timeout multiply factor for DMA data transfer                        */
    u32     MaxCardTxTimeOut;   /* Maximum Card Timeout for DMA data transfer                           */
    u32     LatestStatus;       /* Value of ADI_SDH_STATUS register on most recent interrupt callback   */
    u32     HalvingCount;       /* Count of card operating frequency halvings - for diagnostic purposes */
    ADI_RSI_CARD_REGISTERS CardRegisters;
}ADI_SDH_CARD_INFO;

/*********************************************************************

Device Driver instance structure

*********************************************************************/

#pragma pack(4)

typedef struct {                                        /* SDH device structure                                 */
    ADI_DEV_DEVICE_HANDLE       DeviceHandle;           /* device manager handle                                */
    ADI_DEV_DIRECTION           Direction;              /* Direction of Device Driver                           */
    ADI_DCB_HANDLE              DCBHandle;              /* callback manager handle                              */
    ADI_DCB_CALLBACK_FN         DMCallback;             /* the callback function supplied by the Device Manager */
    void                        *pEnterCriticalArg;     /* critical region argument                             */
    ADI_DEV_DMA_INFO            SdhDmaInfo;             /* SDH DMA channel information table(SDH is half-duplex)*/
    ADI_INT_PERIPHERAL_ID       SdhMask0IntID;          /* SDH Mask 0 Interrupt peripheral ID                   */
    ADI_SEM_HANDLE              DataSemaphoreHandle;    /* Semaphore for Internal data transfers                */
    ADI_SEM_HANDLE              LockSemaphoreHandle;    /* Semaphore for Lock Semaphore operation               */
    int                         CacheHeapID;            /* Heap Index for Device transfer buffers               */
    u32                         fcclk;                  /* CCLK frequency                                       */
    u32                         fsclk;                  /* SCLK frequency                                       */
    u8                          ClkDiv;                 /* CLKDIV override value                                */
    u32                         BlockModeSupport;       /* Flag that dictates use of file cache or otherwise    */
    ADI_SDH_SD_SLOT_STATUS      SlotStatus;             /* SD Slot status                                       */
    ADI_SDH_CARD_INFO           CardInfo;               /* Structure to hold Card specific info                 */
    ADI_FSS_LBA_REQUEST         CurrentLBARequest;      /* The current LBA request being processed              */
    ADI_SDH_BUS_STATE           BusState;               /* SDH Bus state                                        */
    u32                         SavedCdprio;            /* save location for EBIU_AMGCTL's CDPRIO setting       */
    u32                         CdprioModeSupport;      /* flag to indicate whether to use CDPRIO for DMAs      */
    u32                         InErrorRecovery;        /* flag to indicate whether recovering from I/O error   */
} ADI_SDH_DEF;

#pragma pack()

/*********************************************************************

Global Data

*********************************************************************/

/* Flag to monitor SDH Hardware usage status-
   assures only one SDH driver instance has control over SDH hardware at any given time */
__ADI_SDH_SECTION_DATA
static u8 adi_sdh_HardwareInUse = FALSE;

/* FSS callback function for direct access */
__ADI_SDH_SECTION_DATA
ADI_DCB_CALLBACK_FN ADI_SDH_FSSCallbackFunction = NULL;

/* copy of pointer to last-created SDH device structure - for debug */
__ADI_SDH_SECTION_DATA
static ADI_SDH_DEF *gpDevice = NULL;

/*********************************************************************

FSS defines and housekeeping functions

*********************************************************************/

#define PDD_GEN_HEAPID (-1)     /* Generic Heap ID for this PID     */

extern void *_adi_fss_malloc(int id, size_t size);
extern void _adi_fss_free(int id, void *p);
extern void *_adi_fss_realloc(int id, void *p, size_t size);
extern u32 _adi_fss_PIDTransfer(ADI_DEV_DEVICE_HANDLE PIDHandle, ADI_FSS_SUPER_BUFFER *pBuffer, u32 BlockFlag);
extern u32 _adi_fss_DetectVolumes(
                            ADI_DEV_DEVICE_HANDLE   hDevice,
                            u32                     Drive,
                            int                     HeapID,
                            u32                     SectorSize,
                            u32                     DataElementWidth,
                            ADI_SEM_HANDLE          hSemaphore
);
extern void _adi_fss_FlushMemory( char *StartAddress, u32 NumBytes, ADI_DMA_WNR Direction);

/*********************************************************************

Internal Function prototypes

*********************************************************************/
/* Sends SD/SDIO/MMC commands from SDH */
static u32  adi_sdh_SendCommand(
    ADI_SDH_DEF     *pDevice,                   /* pointer to SDH device we're working on       */
    u16             CmdRegVal,                  /* SDH Command register value                   */
    u32             CmdArgument,                /* SDH Command Argument                         */
    u32             ErrorStatus,                /* Status bits to monitor for error             */
    u32             SuccessStatus,              /* Status bits to monitor for command success   */
    u32             TimeOut                     /* Command/response timeout period (in SD CLKs) */
);

/* Resets SDH registers to default/power-up values */
static void adi_sdh_Reset (void);

/* Enables SDH Bus (Data, Clock and command lines) */
static void adi_sdh_EnableHostBus (
    ADI_SDH_DEF     *pDevice                    /* pointer to SDH device we're working on       */
);

/* Reset SD/MMC/SDIO Card Information Table */
static void adi_sdh_ResetCardInfo (
    ADI_SDH_DEF     *pDevice                    /* pointer to SDH device we're working on       */
);

/* Activates SDH device and configures it for use */
static u32 adi_sdh_Activate(
    ADI_SDH_DEF     *pDevice,                   /* pointer to SDH device we're working on       */
    u32             EnableFlag                  /* Enable/Disable Flag                          */
);

/* Detect media change, particularly used to detect SD/SDIO/MMC card insertion or removal       */
static u32 adi_sdh_PollMedia(
    ADI_SDH_DEF     *pDevice                    /* Pointer to the device we're working on       */
);

/* Identifies SD/SDIO/Multimedia card inserted to the SD slot */
static void adi_sdh_Identify(
    ADI_SDH_DEF     *pDevice                    /* pointer to SDH device we're working on       */
);

/*  Tries to identify if the inserted media is a Multi Media Card */
static void adi_sdh_Identify_MMC (
    ADI_SDH_DEF     *poDevice
);

/* Tries to identify if the inserted media is a
   SDHC or SD Version 2.0 card */
static void adi_sdh_Identify_SDHC_SD_Version_2_0 (
    ADI_SDH_DEF     *poDevice
);

/* Tries to identify if the inserted media is a
   SD Version 1.X card */
static void adi_sdh_Identify_SD_Version_1_X (
    ADI_SDH_DEF     *poDevice,
    u32             nOperatingCondition
);

/* Tries to identify if the inserted media is a
   SDIO / SDIO Combo card */
static void adi_sdh_Identify_SDIO_Combo (
    ADI_SDH_DEF     *poDevice
);

/* Gets SD/SDIO/Multimedia card specific information
   Sets Relative Card Address for Multimedia card   */
static u32 adi_sdh_ObtainCardInfo(
    ADI_SDH_DEF     *pDevice                    /* pointer to SDH device we're working on       */
);

/* Configures SDH data bus width */
static u32 adi_sdh_ConfigDataBus(
    ADI_SDH_DEF     *pDevice                    /* pointer to SDH device we're working on       */
);

/*  Calculates SD/SDIO/Multimedia card size and updates driver card info cache */
static u32 adi_sdh_CalculateCardSize(
    ADI_SDH_DEF     *pDevice                    /* pointer to SDH device we're working on       */
);

/* Calculates SD/SDIO/Multimedia card maximum operating frequency and sets SDH CLK to that value*/
static u32 adi_sdh_UpdateOperatingFreq(
    ADI_SDH_DEF     *pDevice                    /* pointer to SDH device we're working on       */
);

/* Calculates and configures SD CLK divisor for selected SD operating frequency */
static u32 adi_sdh_SetClkDiv(
    ADI_SDH_DEF     *pDevice,                   /* pointer to SDH device we're working on       */
    u32             TargetSDClk                 /* Target SD CLK frequency                      */
);

/* Delay loop in terms of CCLK */
static void adi_sdh_Delay (
    ADI_SDH_DEF     *pDevice,                   /* pointer to SDH device we're working on       */
    u32             BaseFreq,                   /* Base frequency to calculate delay            */
    u32             DelayCycle                  /* # base frequency cycles to delay             */
);

/* Reads MBR and identifies partitions and its file format */
static u32 adi_sdh_DetectVolumes (
    ADI_SDH_DEF     *pDevice                    /* pointer to SDH device we're working on       */
);

/* Read/Write Block(s) from the media */
static u32 adi_sdh_TransferBlocks (
    ADI_SDH_DEF     *pDevice,                   /* pointer to SDH device we're working on           */
    u32             StartBlock,                 /* Block number to start with                       */
    u32             BlockCount,                 /* # memory blocks to access                        */
    u8              ReadFlag,                   /* Read/Write Flag (1=Read, 0=Write)                */
    u8              *pMediaData                 /* Pointer to location holding/to hold media data   */
);

/* Initiates SD/SDIO/MMC Memory access by issuing Card specific commands */
static u32 adi_sdh_InitiateMemAccess (
    ADI_SDH_DEF         *pDevice                /* pointer to SDH device we're working on           */
);

/* Hot plug support function - updates SD slot status */
static void adi_sdh_UpdateSlotStatus (
    ADI_SDH_DEF         *pDevice                /* pointer to SDH device we're working on           */
);

static void adi_sdh_WaitForReady (
    ADI_SDH_DEF         *pDevice                /* pointer to SDH device we're working on           */
);

/**************************************
Functions to handle SDH DMA Buffers
**************************************/
/* Prepares a single or list of buffers and submits it to the DMA manager */
static u32 adi_sdh_SubmitBuffers(
    ADI_SDH_DEF                 *pDevice,       /* pointer to SDH device we're working on           */
    ADI_DEV_BUFFER_TYPE         BufferType,     /* type of Buffer                                   */
    ADI_DEV_BUFFER              *pBuffer,       /* pointer to the start of buffer chain             */
    ADI_DEV_DIRECTION           Direction       /* Receive/Transmit data                            */
);

/**************************************
Functions to handle SDH interrupts
**************************************/

/* SDH Device Interrupt handler */
static ADI_INT_HANDLER(adi_sdh_InterruptHandler);

/* PID Callback from FSS */
static void adi_sdh_CallbackFromFSS (
    void                    *pHandle,           /* Client Handle                                    */
    u32                     Event,              /* Callback Event                                   */
    void                    *pArg               /* Buffer address that caused this callback         */
);

/* process SDH interrupts */
static void adi_sdh_ProcessInterrupt (
    ADI_SDH_DEF         *pDevice                /* pointer to SDH device we're working on           */
);

static void adi_sdh_RestoreCdprio (
    ADI_SDH_DEF         *pDevice                /* pointer to SDH device we're working on           */
);

/*********************************************************************

SDH Driver - Entry point functions

*********************************************************************/

static u32 adi_pdd_Open(                       /* Opens a device                                   */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* Device Manager handle                            */
    u32                     DeviceNumber,       /* Device number to open                            */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* Device handle                                    */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,        /* pointer to PDD handle location                   */
    ADI_DEV_DIRECTION       Direction,          /* data direction                                   */
    void                    *pEnterCriticalArg, /* critical region storage location                 */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager                        */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle                                  */
    ADI_DCB_CALLBACK_FN     DMCallback          /* device manager callback function                 */
);

static u32 adi_pdd_Close(                      /* Closes a device                                  */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of the device to close                */
);

static u32 adi_pdd_Read(                       /* Reads data/queues an inbound buffer to a device  */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                                       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                                      */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                                */
);

static u32 adi_pdd_Write(                      /* Writes data/queues outbound buffer to a device   */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                                       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                                      */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                                */
);

static u32 adi_pdd_SequentialIO(               /* Sequentially read/writes data to a device        */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a EPPI device                      */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                                      */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                                */
);

static u32 adi_pdd_Control(                    /* Sets or senses device specific parameter         */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                                       */
    u32                     Command,            /* Command ID                                       */
    void                    *Value              /* Command specific value                           */
);

/*********************************************************************

Processor specific functions

*********************************************************************/

/* Static functions for Moab/Brodie/Moy class devices  */
#if defined(__ADSP_MOAB__) || defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)

/* configures the port control registers for SDH */
static u32 adi_sdh_SetPortControl(ADI_SDH_DEF *pDevice);

#endif


/*********************************************************************

New Card Register Read Functions

*********************************************************************/

/* Static functions for Moab/Brodie/Moy class devices  */
#if defined(__ADSP_MOAB__) || defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)

/* Gets the CID register and stores the result into structure */
static void adi_sdh_GetCIDRegister(ADI_SDH_DEF *pDevice);

/* Gets the CSD register and stores the result into structure */
static void adi_sdh_GetCSDRegister(ADI_SDH_DEF *pDevice);

static void adi_sdh_GetExtendedCSDRegister(ADI_SDH_DEF *pDevice);

#endif
/*********************************************************************

Entry point for device manager

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT ADI_SDH_EntryPoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control,
    adi_pdd_SequentialIO
};

/*********************************************************************

    Function:       adi_pdd_Open

    Description:    Opens SDH Physical Interface Device driver for use

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_pdd_Open(                       /* Open a device                    */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* device manager handle            */
    u32                     DeviceNumber,       /* device number                    */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* device handle                    */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,        /* pointer to PDD handle location   */
    ADI_DEV_DIRECTION       Direction,          /* data direction                   */
    void                    *pEnterCriticalArg, /* enter critical region parameter  */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager        */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle                  */
    ADI_DCB_CALLBACK_FN     DMCallback          /* client callback function         */
){
    u32 Result= ADI_DEV_RESULT_SUCCESS;
    u32 fvco;
    u32 SdhIntIVG;
    ADI_SDH_DEF *pDevice = NULL;

    /* Create SDH driver instance only if the Hardware is available */
    if (adi_sdh_HardwareInUse == FALSE)
    {
        /* Create an instance of the SDH device driver */
        pDevice = (ADI_SDH_DEF*)_adi_fss_malloc(PDD_GEN_HEAPID,sizeof(ADI_SDH_DEF));

        /* Continue only if create device instance is a success */
        if (!pDevice)
        {
            Result = ADI_FSS_RESULT_NO_MEMORY;
        }
    }
    else
    {
        Result = ADI_DEV_RESULT_DEVICE_IN_USE;
    }

    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
        /* Reserve SDH hardware for this driver instance */
        adi_sdh_HardwareInUse = TRUE;

        /*** initialise the SDH Device driver structure ***/
        pDevice->Direction              = Direction;
        pDevice->DCBHandle              = DCBHandle;
        pDevice->DMCallback             = DMCallback;
        pDevice->DeviceHandle           = DeviceHandle;
        pDevice->pEnterCriticalArg      = pEnterCriticalArg;

        /*** Populate SDH DMA channel information table ***/
        /*
           SDH is a half-duplex device and is mapped to a single DMA channel
           To have a better control over DMA dataflow, the SDH DMA and buffers
           are handled by the driver itself rather than the Device manager
        */
        /* DMA Mapping ID - DMA channel shared btwn SDH and NFC */
        pDevice->SdhDmaInfo.MappingID       = ADI_DMA_PMAP_SDH_ABSTRACTION;
        /* No DMA channel handle to start with */
        pDevice->SdhDmaInfo.ChannelHandle   = NULL;

        /* SDH Mask 0 Interrupt peripheral ID */
        pDevice->SdhMask0IntID      = ADI_INT_SDH_ABSTRACTION;

        /* Default SD Slot status as empty */
        pDevice->SlotStatus = ADI_SDH_SLOT_EMPTY;

        /*** Reset SD/MMC/SDIO card information table ***/
        adi_sdh_ResetCardInfo(pDevice);
        /* Enable 4-bit bus (Only if supported by the card and allowed by the
         * application; fallback is 1-bit)  */
        pDevice->CardInfo.EnableWideBus         = TRUE;
#if defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
        pDevice->CardInfo.EnableByteBus         = FALSE;
#endif
        /* Default Card operating condition */
        pDevice->CardInfo.OperatingCondition    =
            (ADI_SD_MMC_OPERATING_CONDITION_BF548_EZKIT &
             ADI_SDH_SD_MMC_OCR_VOLTAGE_WINDOW_MASK);
        /* supported card interface condition (used for SDHC type cards) */
        pDevice->CardInfo.CardIfaceCondition    =
            (ADI_SDH_SDHC_IFACE_COND_VHS1 |
             ADI_SDH_SDHC_IFACE_COND_CHECK_PATTERN);

        /* Use the FSS General Heap for data buffers by default */
        pDevice->CacheHeapID    = -1;

        /* No LBA request to process */
        pDevice->CurrentLBARequest.SectorCount  = 0;

        /* SDH Bus is idle to start with */
        pDevice->BusState = ADI_SDH_BUS_IDLE ;

        /* Set CLKDIV override to zero to use the default value */
        pDevice->ClkDiv = 0;

        /* Set the default mode to Block mode to use File Cache */
        pDevice->BlockModeSupport = true;

        /* Find the current SCLK frequency */
        /* default to power-up values (CCLK = 250MHz, SCLK = 50MHz) if call fails */
        if (adi_pwr_GetFreq(&pDevice->fcclk,&pDevice->fsclk,&fvco)!= ADI_PWR_RESULT_SUCCESS)
        {
            pDevice->fcclk = 250000000;
            pDevice->fsclk = 50000000;
        }

        /*** Create semaphores ***/
        Result = (u32)adi_sem_Create(0,&pDevice->DataSemaphoreHandle, NULL);
        if (Result == (u32)ADI_SEM_RESULT_SUCCESS)
        {
            Result = (u32)adi_sem_Create(1,&pDevice->LockSemaphoreHandle, NULL);
        }

        /*** Hook SDH Status Interrupt Handler ***/
        /* Disable SDH interrupts */
        adi_int_SICDisable(pDevice->SdhMask0IntID);
        /* Get SDH Mask interrupt line IVG */
        adi_int_SICGetIVG(pDevice->SdhMask0IntID, &SdhIntIVG);
        /* hook the interrupt handler into the system */
        if(adi_int_CECHook(SdhIntIVG, adi_sdh_InterruptHandler, pDevice, TRUE) != ADI_INT_RESULT_SUCCESS)
        {
            Result = ADI_DEV_RESULT_CANT_HOOK_INTERRUPT;
        }
        /* Successfully hooked SDH interrupt handler */
        else
        {
            /* save the physical device handle in the client supplied location */
            *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;
            gpDevice = pDevice;
        }

        /* default is to set CDPRIO during DMA transfers */
        pDevice->CdprioModeSupport = TRUE;
        pDevice->SavedCdprio = 0;
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_pdd_Close

    Description:    Closes down the SDH PID driver

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_pdd_Close(
    ADI_DEV_PDD_HANDLE  PDDHandle       /* PDD handle   */
)
{
    ADI_SDH_DEF *pDevice = (ADI_SDH_DEF *)PDDHandle; /* Device Instance */
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    u32 SdhIntIVG;

    /* De-activate SDH */
    adi_sdh_Activate(pDevice,FALSE);

    /* Delete semaphores */
    adi_sem_Delete(pDevice->DataSemaphoreHandle);
    adi_sem_Delete(pDevice->LockSemaphoreHandle);

    /* Disable SDH interrupts */
    adi_int_SICDisable(pDevice->SdhMask0IntID);
    /* unhook the SDH interrupt handler from the system */
    adi_int_SICGetIVG(pDevice->SdhMask0IntID, &SdhIntIVG);
    if (adi_int_CECUnhook(SdhIntIVG, adi_sdh_InterruptHandler, pDevice) != ADI_INT_RESULT_SUCCESS)
    {
        Result = ADI_DEV_RESULT_CANT_UNHOOK_INTERRUPT;
    }

    if ((Result == ADI_FSS_RESULT_SUCCESS) && (pDevice->SdhDmaInfo.ChannelHandle))
    {
        /* Disable DMA dataflow */
        adi_dma_Control(pDevice->SdhDmaInfo.ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)FALSE);
        /* Close SDH DMA channel */
        Result = adi_dev_Control(pDevice->DeviceHandle, ADI_DEV_CMD_CLOSE_PERIPHERAL_DMA, (void *)&pDevice->SdhDmaInfo);
    }

    if (Result == 0)
    {
        /* free SDH hardware */
        adi_sdh_HardwareInUse = FALSE;
        pDevice->SdhDmaInfo.ChannelHandle = NULL;
        /* free memory allocated to this SDH driver instance */
        _adi_fss_free(PDD_GEN_HEAPID,pDevice);
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_pdd_Read

    Description:    Read from SD/MMC memory

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_pdd_Read(                       /* Reads data/queues an inbound buffer to a device  */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                                       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                                      */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                                */
){
    ADI_SDH_DEF *pDevice = (ADI_SDH_DEF *)PDDHandle; /* Device Instance */
    /* assume we're going to be successful */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* Queue this buffer only if SDH DMA channel is open */
    if (pDevice->SdhDmaInfo.ChannelHandle)
    {
        /* prepare and submit list of buffers to SDH DMA */
        Result = adi_sdh_SubmitBuffers(pDevice,BufferType,pBuffer,ADI_DEV_DIRECTION_INBOUND);
    }
    else
    {
        Result = ADI_DEV_RESULT_DMA_CHANNEL_UNAVAILABLE;
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_pdd_Write

    Description:    Write to SD/MMC memory

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_pdd_Write(                      /* Writes data/queues outbound buffer to a device   */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                                       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                                      */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                                */
){

    ADI_SDH_DEF *pDevice = (ADI_SDH_DEF *)PDDHandle; /* Device Instance */
    /* assume we're going to be successful */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* Queue this buffer only if SDH DMA channel is open */
    if (pDevice->SdhDmaInfo.ChannelHandle)
    {
        /* Return error when the card is write-protected or locked */
        if ((pDevice->CardInfo.WriteProtect) || (pDevice->CardInfo.CardLocked))
        {
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;
        }
        else
        {
            /* prepare and submit list of buffers to SDH DMA */
            Result = adi_sdh_SubmitBuffers(pDevice,BufferType,pBuffer,ADI_DEV_DIRECTION_OUTBOUND);
        }
    }
    else
    {
        Result = ADI_DEV_RESULT_DMA_CHANNEL_UNAVAILABLE;
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_pdd_SequentialIO

    Description:    Function not supported

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_pdd_SequentialIO(               /* Sequentially read/writes data to a device        */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a EPPI device                      */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                                      */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                                */
){
    return (ADI_FSS_RESULT_FAILED);
}

/*********************************************************************

    Function:       adi_sdh_SubmitBuffers

    Description:    Prepares a single or list of buffers and submits it to
                    the DMA manager. All fields within the ADI_DEV_BUFFER
                    structure and the embedded DMA descriptor fields are
                    set up appropriately.

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_sdh_SubmitBuffers(
    ADI_SDH_DEF                 *pDevice,       /* pointer to SDH device we're working on   */
    ADI_DEV_BUFFER_TYPE         BufferType,     /* type of Buffer                           */
    ADI_DEV_BUFFER              *pBuffer,       /* pointer to the start of buffer chain     */
    ADI_DEV_DIRECTION           Direction       /* Receive/Transmit data                    */
){
    u32                         Result;             /* return code */
    ADI_DEV_BUFFER              *pWorkingBuffer;    /* pointer to the buffer we're currently working on */
    ADI_DMA_WNR                 wnr;                /* value of the direction field within the DMA config register */
    /* build for large descriptors */
    ADI_DMA_DESCRIPTOR_LARGE    *pDescriptor;       /* pointer to the descriptor within the buffer */

    /* assume we're going to be successful */
    Result = ADI_DEV_RESULT_SUCCESS;

    /* set the value of wnr for the config register */
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
        /* CASE (1D buffer) */
        case (ADI_DEV_1D):

            /* FOR each buffer in the list */
            for (pWorkingBuffer = pBuffer; pWorkingBuffer != NULL; pWorkingBuffer = (ADI_DEV_BUFFER *)pWorkingBuffer->OneD.pNext)
            {
                /* point to the descriptor  */
                pDescriptor                     = (ADI_DMA_DESCRIPTOR_LARGE *)pWorkingBuffer->OneD.Reserved;
                pDescriptor->Config.b_FLOW      = ADI_DMA_FLOW_LARGE;
                pDescriptor->Config.b_NDSIZE    = ADI_DMA_NDSIZE_LARGE;
                pDescriptor->Config.b_DI_SEL    = ADI_DMA_DI_SEL_OUTER_LOOP;
                /* set the word size, direction and transfer type according to the buffer */
                pDescriptor->Config.b_WDSIZE    = pWorkingBuffer->OneD.ElementWidth >> 1;
                pDescriptor->Config.b_WNR       = wnr;
                pDescriptor->Config.b_DMA2D     = ADI_DMA_DMA2D_LINEAR;
                /* Enable Synchronized transition */
                pDescriptor->Config.b_RESTART   = ADI_DMA_RESTART_DISCARD;

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
                pDescriptor->StartAddress = pWorkingBuffer->OneD.Data;
                pDescriptor->XCount       = pWorkingBuffer->OneD.ElementCount;
                pDescriptor->XModify      = pWorkingBuffer->OneD.ElementWidth;

                /* update the descriptor to point to the next descriptor in the chain */
                if (pWorkingBuffer->OneD.pNext == NULL)
                {
                    pDescriptor->pNext = NULL;
                }
                else
                {
                    pDescriptor->pNext = (ADI_DMA_DESCRIPTOR_LARGE *)pWorkingBuffer->OneD.pNext->Reserved;
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
            /* ENDFOR */
            }

            /* Queue this Buffer list to DMA manager, setting CDPRIO to give DMA priority over
             * the core for bus accesses (if enabled). */
            if (Result == ADI_DEV_RESULT_SUCCESS)
            {
                if (pDevice->CdprioModeSupport != FALSE)
                {
                    u32 DMA_running;

                    BEGIN_CRITICAL_REGION

                        adi_dma_Control(pDevice->SdhDmaInfo.ChannelHandle,
                                        ADI_DMA_CMD_GET_TRANSFER_STATUS,
                                        (void*)&DMA_running);
                        if (DMA_running == FALSE) {
                            /* save the default CDPRIO setting so that it can be restored later */
                            pDevice->SavedCdprio = (*pEBIU_AMGCTL & CDPRIO);
                            /* give DMA priority */
                            *pEBIU_AMGCTL |= CDPRIO;
                        }

                    END_CRITICAL_REGION
                }
                
                /* Ensure that the memory and cache are synchronized before queuing */
                _adi_fss_FlushMemory ( 
                            pDescriptor->StartAddress, 
                            pDescriptor->XCount * pDescriptor->XModify,
                            (ADI_DMA_WNR)pDescriptor->Config.b_WNR
                         );

                Result = adi_dma_Queue(pDevice->SdhDmaInfo.ChannelHandle, (ADI_DMA_DESCRIPTOR_HANDLE)pBuffer->OneD.Reserved);
            }

            break;

        /* other buffer types - not supported */
        default:
            Result = ADI_DEV_RESULT_BUFFER_TYPE_INCOMPATIBLE;
            break;
    }   /* END CASE */

    /* return */
    return (Result);
}
/*********************************************************************

    Function:       adi_pdd_Control

    Description:    Configures the SDH device

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_pdd_Control(                    /* Sets or senses device specific parameter         */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                                       */
    u32                     Command,            /* Command ID                                       */
    void                    *Value              /* Command specific value                           */
){

    ADI_SDH_DEF         *pDevice;   /* Device Instance */
    u32                 Result;     /* return value    */
    ADI_DEV_FREQUENCIES *pFreqs;
    ADI_FSS_LBA_REQUEST *pLBARequest;
    ADI_FSS_VOLUME_DEF  *pVolumeDef;
    u64                 u64Temp;

    pDevice = (ADI_SDH_DEF *)PDDHandle;

    /* assume we're going to be successful */
    Result = ADI_DEV_RESULT_SUCCESS;

    /* CASEOF (Command ID) */
    switch(Command)
    {
        /******************************
        Common Device Driver Commands
        ******************************/
        /* CASE (control dataflow) */
        case (ADI_DEV_CMD_SET_DATAFLOW):
            /* nothing to do here */
            break;

        /* CASE (dataflow method) */
        case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
            /* SDH only supports chained dataflow method */
            if ((ADI_DEV_MODE)Value != ADI_DEV_MODE_CHAINED)
            {
                Result = ADI_DEV_RESULT_NOT_SUPPORTED;
                break;
            }
#endif  /* ADI_DEV_DEBUG */

            /* open SDH DMA channel */
            Result = adi_dev_Control(pDevice->DeviceHandle, ADI_DEV_CMD_OPEN_PERIPHERAL_DMA, (void *)&pDevice->SdhDmaInfo);
            break;

        /* CASE (query for processor DMA support) */
        case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):

            /*
               SDH is a half-duplex device and is mapped to a single DMA channel
               To have a better control over DMA dataflow, the SDH DMA and buffers
               are handled by the driver itself rather than the Device manager
            */
            *((u32 *)Value) = FALSE;
            break;

        /* CASE (Enable SDH Error reporting) */
        case (ADI_DEV_CMD_SET_ERROR_REPORTING):

            break;

        /* CASE (CCLK/SCLK Frequency Change) */
        case (ADI_DEV_CMD_FREQUENCY_CHANGE_PROLOG):
        case (ADI_DEV_CMD_FREQUENCY_CHANGE_EPILOG):

            pFreqs = (ADI_DEV_FREQUENCIES *)Value;
            /* IF (there is any change in operating frequency) */
            if ((pFreqs->SystemClock != pDevice->fsclk) || (pFreqs->CoreClock != pDevice->fcclk))
            {
                /* store the new CCLK/SCLK frequecnies */
                pDevice->fcclk = pFreqs->CoreClock;
                pDevice->fsclk = pFreqs->SystemClock;
                /* update SD Clock divisor */
                Result = adi_sdh_SetClkDiv(pDevice,pDevice->CardInfo.OperatingFreq);
            }
            break;

        /*********************
        Common PID Commands
        *********************/

        /* CASE (determine whether SDH supports background transfers) */
        case (ADI_FSS_CMD_GET_BACKGRND_XFER_SUPPORT):

            /* Yes, we do support background transfers (DMA) */
            *((u32 *)Value) = pDevice->BlockModeSupport;
            break;

        /* CASE (get SDH DMA data element width) */
        case (ADI_FSS_CMD_GET_DATA_ELEMENT_WIDTH):

            /* SDH DMA only support 32-bit transfers */
            *(u32*)Value = sizeof(u32);
            break;
            
        case ADI_PID_CMD_GET_MAX_TFRCOUNT:
            /* due to limitation of data lenght registers, the driver is limited
             * to one less than 64K at a time.
            */
            *((u32 *)Value) = 64*1024 - 1;
            break;

        /* CASE (Acquire Lock Semaphore for exclusive access) */
        case (ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE):

            adi_sem_Pend (pDevice->LockSemaphoreHandle, ADI_SEM_TIMEOUT_FOREVER);
            break;

        /* CASE (Release Lock Semaphore for exclusive access) */
        case (ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE):

            adi_sem_Post (pDevice->LockSemaphoreHandle);
            break;

        /* CASE (Sets Cache Heap ID for data caches) */
        case ADI_FSS_CMD_SET_CACHE_HEAP_ID:

            pDevice->CacheHeapID = (int)Value;
            break;

        /* CASE (query for fixed status) */
        case (ADI_PID_CMD_GET_FIXED):

            /* MMC,SD & SDIO are removeable medias */
            *((u32 *)Value) = FALSE;
            break;

        /* CASE (Activate/Deactivate SDH) */
        case (ADI_PID_CMD_MEDIA_ACTIVATE):
            Result = adi_sdh_Activate(pDevice,(u32)Value);
            break;

        /* CASE (Poll for media) */
        case ADI_PID_CMD_POLL_MEDIA_CHANGE:

            Result = adi_sdh_PollMedia(pDevice);
            break;

        /* CASE (Detect volumes) */
        case ADI_PID_CMD_DETECT_VOLUMES:
            Result = _adi_fss_DetectVolumes(
                            pDevice->DeviceHandle,
                            0,
                            pDevice->CacheHeapID,
                            (1<<pDevice->CardInfo.BlockLength),
                            sizeof(u32),
                            pDevice->DataSemaphoreHandle
                    );

            //Result = adi_sdh_DetectVolumes(pDevice);
            break;


        /* CASE (Send LBA read/write Request) */
        case (ADI_PID_CMD_SEND_LBA_REQUEST):

            pLBARequest = (ADI_FSS_LBA_REQUEST *)Value;

            /* save the LBA request */
            pDevice->CurrentLBARequest.StartSector  = pLBARequest->StartSector;
            pDevice->CurrentLBARequest.SectorCount  = pLBARequest->SectorCount;
            pDevice->CurrentLBARequest.DeviceNumber = pLBARequest->DeviceNumber;
            pDevice->CurrentLBARequest.ReadFlag     = pLBARequest->ReadFlag;
            pDevice->CurrentLBARequest.pBuffer      = pLBARequest->pBuffer;

            /* validate the sector address to be accessed */
            if ((pDevice->CurrentLBARequest.StartSector + pDevice->CurrentLBARequest.SectorCount) > pDevice->CardInfo.TotalBlocks)
            {
                /* Sector address exceeds device size, return failure */
                Result = ADI_FSS_RESULT_FAILED;
                /* invalidate this LBA request */
                pDevice->CurrentLBARequest.SectorCount = 0;
            }

            break;

        /* CASE (Enable PID Dataflow) */
        case (ADI_PID_CMD_ENABLE_DATAFLOW):

            /* Enable dataflow ? */
            if ((u32)Value)
            {
                /* Initiate Memory access with the Current LBA request cached by this device instance */
                Result = adi_sdh_InitiateMemAccess (pDevice);
            }
            /* else, disable SDH dataflow */
            else
            {
                /* Disable SDH dataflow */
                ADI_SDH_DATA_CTRL_DATA_TX_DISABLE;
            }

            break;

        /* CASE (Set FSS callback function for direct access) */
        case (ADI_PID_CMD_SET_DIRECT_CALLBACK):

            ADI_SDH_FSSCallbackFunction = (ADI_DCB_CALLBACK_FN)Value;
            break;

        /* CASE (Return information regarding total geometry of the inserted media) */
        case (ADI_PID_CMD_GET_GLOBAL_MEDIA_DEF):

            pVolumeDef = (ADI_FSS_VOLUME_DEF*)Value;

            /* IF the SDH slot contains a valid media */
            if (pDevice->SlotStatus >= ADI_SDH_SLOT_MMC)
            {
                /* device starts from sector 0 */
                pVolumeDef->StartAddress    = 0;
                /* total blocks available in this media */
                pVolumeDef->VolumeSize      = pDevice->CardInfo.TotalBlocks;
                /* sector (block) size in bytes */
                pVolumeDef->SectorSize      = (1 << pDevice->CardInfo.BlockLength);
            }
            else
            {
                Result = ADI_FSS_RESULT_FAILED;
            }
            break;

        /****************************
        SDH driver Specific Commands
        ****************************/

        /* CASE (Set supported card operating condition) */
        case (ADI_SDH_CMD_SET_SUPPORTED_CARD_OPERATING_CONDITION):

            /* save the operating condition */
            pDevice->CardInfo.OperatingCondition = ((u32)Value & ADI_SDH_SD_MMC_OCR_VOLTAGE_WINDOW_MASK);

            /* update Card Interface condition */

            /* IF (given operating condition matches SDHC required voltage range */
            if (ADI_SDH_SDHC_REQUIRED_VOLTAGE_RANGE1 & pDevice->CardInfo.OperatingCondition)
            {
                pDevice->CardInfo.CardIfaceCondition = (ADI_SDH_SDHC_IFACE_COND_VHS1 | ADI_SDH_SDHC_IFACE_COND_CHECK_PATTERN);
            }
            /* ELSE (Operating condition does not support SDHC cards) */
            else
            {
                pDevice->CardInfo.CardIfaceCondition = 0;
            }
            break;

        /* CASE (Save the card lock (mechanical switch) status) */
        case (ADI_SDH_CMD_SET_CARD_LOCK_ENABLED):

            pDevice->CardInfo.CardLocked = (u8)((u32)Value);
            break;

        /* CASE (Enable/Disable wide-bus mode) */
        case (ADI_SDH_CMD_ENABLE_WIDEBUS):

            pDevice->CardInfo.EnableWideBus = (u8)((u32)Value);
            /* update SDH bus status only when the SD slot contains a valid media */
            if (pDevice->SlotStatus >= ADI_SDH_SLOT_MMC)
            {
                Result = adi_sdh_ConfigDataBus(pDevice);
            }
            break;

#if defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
        /* CASE (Enable/Disable byte-bus mode) */
        case (ADI_SDH_CMD_ENABLE_BYTEBUS):

            pDevice->CardInfo.EnableByteBus = (u8)((u32)Value);
            /* update SDH bus status only when the SD slot contains a valid media */
            if (pDevice->SlotStatus >= ADI_SDH_SLOT_MMC)
            {
                Result = adi_sdh_ConfigDataBus(pDevice);
            }
            break;
#endif

        /* CASE (Set Time-out multiplier) */
        case (ADI_SDH_CMD_SET_TIMEOUT_MULTIPLIER):
            /*
               Maximum Card Transfer Timeout is set to
               "Time out Multiplier * pDevice->CardInfo.CardTxTimeOut" times
               so that the DMA gets enough time to transfer multiple blocks
            */
            /* save the time out value only if it is within the boundary */
            if (((u32)Value != 0) && ((u32)Value <= 300))
            {
                pDevice->CardInfo.TxTimeOutMultiplier = (u32)Value;
            }
            /* else, revert to default value */
            else
            {
                pDevice->CardInfo.TxTimeOutMultiplier = 100U;
            }
            /* Calculate Maximum Card Timeout value */
            u64Temp = (u64)pDevice->CardInfo.CardTxTimeOut;
            u64Temp *= pDevice->CardInfo.TxTimeOutMultiplier;
            pDevice->CardInfo.MaxCardTxTimeOut =
                    (u64Temp > 0xFFFFFFFFull)? 0xFFFFFFFFu : (u32)u64Temp;
            break;

        /* CASE (Get present Status of SDH slot) */
        case (ADI_SDH_CMD_GET_SDH_SLOT_STATUS):

            /* Save the present SDH slot status to application provided location */
            *((ADI_SDH_SD_SLOT_STATUS *)Value) = pDevice->SlotStatus;
            break;

        /* CASE (Set CLKDIV override value) */
        case (ADI_SDH_CMD_OVERRIDE_CLKDIV):

            pDevice->ClkDiv = (u8)((u32)Value);
            break;

        /* CASE (Set block mode support value) */
        case (ADI_SDH_CMD_SET_BLOCK_MODE):

            pDevice->BlockModeSupport = (u32)Value;
            break;

        /* CASE (Set CDPRIO mode support value) */
        case (ADI_SDH_CMD_SET_CDPRIO_MODE):

            pDevice->CdprioModeSupport = (u32)Value;
            break;

        /* CASE (Disable Device Interrupt) */
        case (ADI_PID_CMD_DISABLE_INTERRUPT):
            adi_int_SICDisable(pDevice->SdhMask0IntID);
            break;

        /* CASE (Enable Device Interrupt) */
        case (ADI_PID_CMD_ENABLE_INTERRUPT):
            adi_int_SICEnable(pDevice->SdhMask0IntID);
            break;

        /* Command not supported */
        default:
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;
            break;

    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_sdh_Reset

    Description:    Resets SDH registers to default/power-up values

*********************************************************************/
__ADI_SDH_SECTION_CODE
static void adi_sdh_Reset (void)
{
    /*** Clear any pending SDH interrupts ***/
    /* Clear all SDH status bits */
    ADI_SDH_STATUS_CLEAR_ALL;
    /* Clear all exception status bits */
    ADI_SDH_E_STATUS_CLEAR_ALL;
    /* Mask all SDH status interrupts */
    ADI_SDH_INT_MASK0_ALL;
    /* Mask all exception status bits */
    ADI_SDH_E_STATUS_MASK_ALL;

    /*** Configure SDH registers with default values */
    /* Set SDH in 1-bit bus mode, Disable bus clock */
    ADI_SDH_CLK_CTRL_SET_DEFAULT;
    /* Disable SDH Command Transfer */
    ADI_SDH_COMMAND_SET_DEFAULT;
    /* Disable any data transfer */
    ADI_SDH_DATA_CTRL_SET_DEFAULT;
    /* Disable SDH Clock, Disable pull-up on SDH DAT2-0 and SDH CMD signals, Disable pull-up & Pull-down on DAT3 */
    ADI_SDH_CFG_SET_DEFAULT;

    /* return */
}

/*********************************************************************

    Function:       adi_sdh_EnableHostBus

    Description:    Enables SDH signals (Data, Clock and command lines)

*********************************************************************/
__ADI_SDH_SECTION_CODE
static void adi_sdh_EnableHostBus (
    ADI_SDH_DEF     *pDevice    /* pointer to SDH device we're working on   */
){
    /* Enable SDH DAT2-0 and SDH CMD pin pull-ups */
    ADI_SDH_CFG_PUP_SDCMD_SDDAT2_0_ENABLE;

    /* Disable Pull-down on DAT3 */
    ADI_SDH_CFG_PD_SDDAT3_DISABLE;

    /* Enable Pull-up on DAT3  */
    ADI_SDH_CFG_PUP_SDDAT3_ENABLE;

    /* Enable SDH CLKs */
    ADI_SDH_CFG_CLOCKS_EN_ENABLE;
    ADI_SDH_PWR_CTRL_PWN_ON_ON;

    /* provide at least 5 SCLK delay to update Clock control register */
    adi_sdh_Delay(pDevice,pDevice->fsclk,5);

    /* return */
}

/*********************************************************************

    Function:       adi_sdh_ResetCardInfo

    Description:    Resets SD/MMC/SDIO Card Information Table

*********************************************************************/
__ADI_SDH_SECTION_CODE
static void adi_sdh_ResetCardInfo (
    ADI_SDH_DEF     *pDevice    /* pointer to SDH device we're working on   */
){

    /* Assume the card is not locked */
    pDevice->CardInfo.CardLocked            = FALSE;
    /* Assume the card is write protected until we get real card parameters */
    pDevice->CardInfo.WriteProtect          = TRUE;
    /* 2^9 = 512 bytes per block by default */
    pDevice->CardInfo.BlockLength           = 9;
    /* Command classes - unknown */
    pDevice->CardInfo.CardCmdClasses        = 0;
    /* No memory device has been detected yet */
    pDevice->CardInfo.TotalBlocks           = 0;
    /* Card operating frequency - card ID mode */
    pDevice->CardInfo.OperatingFreq         = ADI_SDH_SD_MMC_SDCLK_ID_MODE;
    /* Relative Card Address - unknown at this point */
    pDevice->CardInfo.RCA                   = 0;
    /* Max Transfer time dealy required - unknown at this point */
    pDevice->CardInfo.CardTxTimeOut         = 0;
    /* Maximum card timeout multiplier set to 100 by default */
    pDevice->CardInfo.TxTimeOutMultiplier   = 100;
    /* Card Time out - unknown */
    pDevice->CardInfo.MaxCardTxTimeOut      = 0;
    /* initialise debug items */
    pDevice->CardInfo.LatestStatus          = 0;
    pDevice->CardInfo.HalvingCount          = 0;

    /* return */
}

/*********************************************************************

    Function:       adi_sdh_Activate

    Description:    Activates SDH device and configures it for use

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_sdh_Activate(
    ADI_SDH_DEF *pDevice,       /* pointer to SDH device we're working on   */
    u32         EnableFlag      /* Enable/Disable Flag                      */
){

    u32 Result = ADI_FSS_RESULT_SUCCESS;    /* default return code */

    /* reset SDH registers */
    adi_sdh_Reset();

    /* Enable SDH device */
    if (EnableFlag)
    {
        /* Enable SDH pins */
        Result = adi_sdh_SetPortControl(pDevice);

        if (Result == ADI_PORTS_RESULT_SUCCESS)
        {
            /* Enable SDH Bus */
            adi_sdh_EnableHostBus(pDevice);

/* For BF54x family */
#if defined(__ADSP_MOAB__)

            /*** Configure DMA Peripheral Mux register for SDH use ***/
            /*### TEMPORARY - this register configuration will be replaced by a
                              DMA manager control command & adi_dma_Control() call ###*/
            *pDMAC1_PERIMUX |= 1;

#endif
            /* Enable SDH interrupts */
            adi_int_SICEnable(pDevice->SdhMask0IntID);

            /* Enable DMA dataflow */
            Result = adi_dma_Control(pDevice->SdhDmaInfo.ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)TRUE);
        }
    }
    else
    {
        /* Mark SD slot status as media removed so that we could unmount any volumes already mounted by SDH */
        pDevice->SlotStatus = ADI_SDH_SLOT_MEDIA_REMOVED;
        /* Reset SDH registers */
        adi_sdh_Reset();
        /* Enable SDH bus */
        adi_sdh_EnableHostBus(pDevice);
        /* Reset SD/MMC/SDIO card information table */
        adi_sdh_ResetCardInfo(pDevice);
        /* enable card detect interrupt */
        ADI_SDH_E_STATUS_MASK_SD_CARD_DETECT_ENABLE;
    }

    /* return */
    return (Result);
}


/*********************************************************************

  Function:    adi_sdh_PollMedia

  Description: Detect media change, this is used in particular to detect
               when a SD/SDIO/MMC card is inserted or removed

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_sdh_PollMedia(
    ADI_SDH_DEF     *pDevice      /* Pointer to the device we're working on */
){

    u32 Result = ADI_FSS_RESULT_SUCCESS;
    u32 FSSValue = 0;   /* initiate the value to be sent to FSS as 0 (SDH Device number in use) */

    /* if the SDH slot contains a valid media */
    if (pDevice->SlotStatus >= ADI_SDH_SLOT_MMC)
    {
        /* get an update on SDH slot status */
        adi_sdh_UpdateSlotStatus(pDevice);
    }

    /* Poll for media change only when the slot is empty or media status has changed */
    if (pDevice->SlotStatus <= ADI_SDH_SLOT_EMPTY)
    {
        /* unmount SDH volume(s) when the Media in SDH slot was removed */
        if (pDevice->SlotStatus == ADI_SDH_SLOT_MEDIA_REMOVED)
        {
            /* indicate FSS that media has been removed from SDH slot */
            if (ADI_SDH_FSSCallbackFunction)
            {
                (ADI_SDH_FSSCallbackFunction)(&pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_REMOVED, &FSSValue);
            }
            else
            {
                (pDevice->DMCallback) (pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_REMOVED, &FSSValue);
            }
        }

        /* Identify if there is any media inserted to SDH slot*/
        adi_sdh_Identify(pDevice);

        /* Detected a valid media in SDH slot */
        if (pDevice->SlotStatus >= ADI_SDH_SLOT_MMC)
        {
            /* indicate FSS that media has been inserted to SDH slot */
            if (ADI_SDH_FSSCallbackFunction)
            {
                (ADI_SDH_FSSCallbackFunction)(&pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_INSERTED, &FSSValue);
            }
            else
            {
                (pDevice->DMCallback) (pDevice->DeviceHandle, ADI_FSS_EVENT_MEDIA_INSERTED, &FSSValue);
            }

            Result = FSSValue;
        }
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_sdh_Identify

        Identifies SD/SDIO/Multimedia card inserted
                    to the SD slot and determines its capabilities

    Parameters:
        pDevice - Pointer to SDH device instance to work on

*********************************************************************/
__ADI_SDH_SECTION_CODE
static void adi_sdh_Identify(
    ADI_SDH_DEF *pDevice
)
{

    /* Mark slot status as empty to start with */
    pDevice->SlotStatus = ADI_SDH_SLOT_EMPTY;

    /* clear card detect status */
    ADI_SDH_E_STATUS_SD_CARD_DETECT_CLEAR;
    /* disable card detect interrupt */
    ADI_SDH_E_STATUS_MASK_SD_CARD_DETECT_DISABLE;

    /* Set SDH Clock frequency to card identification mode */
    if (adi_sdh_SetClkDiv(pDevice, ADI_SDH_SD_MMC_SDCLK_ID_MODE) == ADI_DEV_RESULT_SUCCESS)
    {
        /* Acquire Lock Semaphore for exclusive use of SDH bus */
        adi_dev_Control(pDevice->DeviceHandle,
                        ADI_FSS_CMD_ACQUIRE_LOCK_SEMAPHORE,
                        NULL);

        /* Enable SDH bus clock */
        ADI_SDH_CLK_CTRL_CLK_ENABLE;

        /* provide some delay for SD/SDIO/MMC power-up/initialisation sequence */
        adi_sdh_Delay(pDevice,
                      pDevice->CardInfo.OperatingFreq,
                      ADI_SDH_WAIT_PERIOD_POWER_UP);

        /*** Send SD/SDIO/MMC to Idle state ***/
        /* CMD0,
           no response,
           stuff bits as argument,
           no error/success status bits,
           no timeout */
        adi_sdh_SendCommand (pDevice,
                             ADI_SD_MMC_CMD_GO_IDLE_STATE,
                             ADI_SDH_SD_MMC_STUFF_BITS,
                             0,
                             0,
                             0);

        /* wait for NRC SD CLKs to let the media device return to idle state */
        adi_sdh_Delay(pDevice,
                      pDevice->CardInfo.OperatingFreq,
                      ADI_SDH_WAIT_PERIOD_NRC);

        /* Try to identify if the inserted media is a Multi Media Card */
        adi_sdh_Identify_MMC (pDevice);

        /* IF (Slot status is still empty) */
        if (pDevice->SlotStatus == ADI_SDH_SLOT_EMPTY)
        {
            /* Try to identify if the inserted media is a SDHC or SD Version 2.0 card */
            adi_sdh_Identify_SDHC_SD_Version_2_0 (pDevice);
        }

        /* IF (Slot status is still empty) */
        if (pDevice->SlotStatus == ADI_SDH_SLOT_EMPTY)
        {
            /* Try to identify if the inserted media is a SDIO / SDIO Combo card */
            adi_sdh_Identify_SDIO_Combo(pDevice);
        }

        /* IF (Slot status is still empty) */
        if (pDevice->SlotStatus == ADI_SDH_SLOT_EMPTY)
        {
            /* Try to identify if the inserted media is a SD Version 1.X card */
            adi_sdh_Identify_SD_Version_1_X (pDevice,
                                             pDevice->CardInfo.OperatingCondition);
        }

        /* IF (Media inserted is not supported by the driver) */
        if (pDevice->SlotStatus == ADI_SDH_SLOT_CARD_NOT_SUPPORTED)
        {
            /* Mark slot status as empty */
                pDevice->SlotStatus = ADI_SDH_SLOT_EMPTY;
        }

        /* IF (detected a valid card in the slot) */
        if (pDevice->SlotStatus != ADI_SDH_SLOT_EMPTY)
        {
            /* Obtain card specific information */
            if (adi_sdh_ObtainCardInfo(pDevice) != ADI_DEV_RESULT_SUCCESS)
            {
                /* Failed to obtain card specific info. Mark SDH slot as empty */
                pDevice->SlotStatus = ADI_SDH_SLOT_EMPTY;
            }
        }
        /* Release Lock Semaphore */
        adi_dev_Control(pDevice->DeviceHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL);
    }
    else
    {
        /* Failed to set SDH clock for card identification mode. Mark SDH slot as empty */
        pDevice->SlotStatus = ADI_SDH_SLOT_EMPTY;
    }

    /* IF SD slot is empty */
    if (pDevice->SlotStatus == ADI_SDH_SLOT_EMPTY)
    {
        /* mask all SD status interrupt bits */
        ADI_SDH_INT_MASK0_ALL;
        /* enable card detect interrupt */
        ADI_SDH_E_STATUS_MASK_SD_CARD_DETECT_ENABLE;
    }
    /* on detecting a valid media (card)*/
    else
    {
        /*** Flush SDH DMA to start with ***/
        /* IF (SDH DMA is already open) */
        if (pDevice->SdhDmaInfo.ChannelHandle)
        {
            /* Disable DMA dataflow */
            adi_dma_Control(pDevice->SdhDmaInfo.ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)FALSE);
            /* Flush SDH DMA */
            adi_dma_Control(pDevice->SdhDmaInfo.ChannelHandle, ADI_DMA_CMD_FLUSH, (void *)FALSE);
            /* Enable DMA dataflow */
            adi_dma_Control(pDevice->SdhDmaInfo.ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)TRUE);
        }
        /* Enable SDH data transfer status interrupts */
        ADI_SDH_INT_MASK0_ENABLE_DATA_TX_INT;
    }

    /* return */
}

/*********************************************************************

    Function: adi_sdh_Identify_MMC

        Tries to identify if the inserted media is a Multi Media Card

*********************************************************************/
__ADI_SDH_SECTION_CODE
static void adi_sdh_Identify_MMC (
    ADI_SDH_DEF     *poDevice
)
{
    /* SDH Status register value in response to a command transfer */
    u32     nSdhStatus;
    /* Counter to retry card identification sequence */
    u32     nRetryCount;

    /* Retry card identification until the media is ready to enter data transfer mode
       (or) until the stipulated retry count expires */
    for (nRetryCount = 0;
         nRetryCount < ADI_SDH_SD_MMC_IDENTIFY_RETRY_COUNT;
         nRetryCount++)
    {
        /* Issue Command to Identify the inserted Card,
           Short response,
           Card operating condition as command argument,
           Identification mode time out */
        nSdhStatus = adi_sdh_SendCommand (poDevice,
                                          (ADI_MMC_CMD_GET_OCR_VALUE |
                                           ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE),
                                          (poDevice->CardInfo.OperatingCondition |
                                              ADI_SDH_SDHC_MMCHC_OCR_HCS_CCS_MASK),
                                          (ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD |
                                           ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD),
                                          ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                          ADI_SDH_TIMEOUT_NID);

        /* Neglet Command CRC error and Command response error for for MMC */
        nSdhStatus &= ~(ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD | ADI_SDH_STATUS_CMD_SENT_FIELD);

        /* IF (SDH Status error other than Command CRC Failed (or)
               Card is ready to enter data transfer mode) */
        if ((nSdhStatus != 0) ||
            (*pADI_SDH_RESPONSE_0 & ADI_SDH_SD_MMC_OCR_BUSY_MASK))
        {
            /* Internal Card initialisation process completed */
            poDevice->CardInfo.CardRegisters.mmc.ocr = *pADI_SDH_RESPONSE_0;
            break;
        }

    }

    /* IF (No SDH Status error reported) */
    if (nSdhStatus == 0)
    {
        /* IF (Specified operating condition is not supported by the card (or)
               we've reached our idenification retry limit) */
        if (((*pADI_SDH_RESPONSE_0 & poDevice->CardInfo.OperatingCondition) == 0) ||
            (nRetryCount >= ADI_SDH_SD_MMC_IDENTIFY_RETRY_COUNT))
        {
            /* Hardware does not supported the required operating condition of
               the Media Inserted */
            /* Mark slot status as card not supported */
            poDevice->SlotStatus = ADI_SDH_SLOT_CARD_NOT_SUPPORTED;
        }
        /* ELSE (Inserted media is a Multimedia Card) */
        else
        {
        	if (*pADI_SDH_RESPONSE_0 & ADI_SDH_SDHC_MMCHC_OCR_HCS_CCS_MASK)
        	{
        		/* Update slot status as MMC */
            	poDevice->SlotStatus = ADI_SDH_SLOT_MMC_HC;
        	}
        	else
        	{
            	/* Update slot status as MMC */
            	poDevice->SlotStatus = ADI_SDH_SLOT_MMC;
        	}
        }
    }

}

/*********************************************************************

    Function: adi_sdh_Identify_SDHC_SD_Version_2_0

        Tries to identify if the inserted media is a
        SDHC or SD Version 2.0 card

*********************************************************************/
__ADI_SDH_SECTION_CODE
static void adi_sdh_Identify_SDHC_SD_Version_2_0 (
    ADI_SDH_DEF     *poDevice
)
{
    /* SDH Status register value in response to a command transfer */
    u32     nSdhStatus;

    /* Issue Command to send card interface condition,
       Short response,
       Card operating condition as command argument,
       Identification mode time out */
    nSdhStatus = adi_sdh_SendCommand (poDevice,
                                      (ADI_SDHC_CMD_SEND_IF_COND |
                                       ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE),
                                      poDevice->CardInfo.CardIfaceCondition,
                                      (ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD |
                                       ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD),
                                      ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                      ADI_SDH_TIMEOUT_NID);

    /* IF (Successfully obtained card interface condition) */
    if (nSdhStatus == 0)
    {
        /* IF (Inserted media accepts the given Card Interface Condition) */
        if (poDevice->CardInfo.CardIfaceCondition == *pADI_SDH_RESPONSE_0)
        {
            /* Run SD Version 1.X card identification sequence with
               support to Version 2.0 SD/SDHC cards */
            adi_sdh_Identify_SD_Version_1_X (poDevice,
                                             (poDevice->CardInfo.OperatingCondition |
                                              ADI_SDH_SDHC_MMCHC_OCR_HCS_CCS_MASK));

            /* IF (SD standard Version 1.X identifaction sequence is successful) */
            if (poDevice->SlotStatus == ADI_SDH_SLOT_SD_CARD_VER_1_X)
            {
                /* IF (Media is SDHC & Version 2.0 SD standard compliant) */
                if (*pADI_SDH_RESPONSE_0 & ADI_SDH_SDHC_MMCHC_OCR_HCS_CCS_MASK)
                {
                    /* update slot status as SDHC card */
                    poDevice->SlotStatus = ADI_SDH_SLOT_SDHC_CARD;
                }
                /* ELSE (Media supportes Version 2.0 SD Standards) */
                else
                {
                    /* update slot status as Version 2.0 standard capacity SD Card */
                    poDevice->SlotStatus = ADI_SDH_SLOT_SD_CARD_VER_2_0;
                }
            }
            /* ELSE (SD standard Version 1.X identifaction failed,
                     Media not supported) */
            else
            {
                /* Mark slot status as card not supported */
                poDevice->SlotStatus = ADI_SDH_SLOT_CARD_NOT_SUPPORTED;
            }

        } /* End of if (Inserted media accepts the given Card Interface Condition) */

    } /* End of if (Successfully obtained card interface condition) */

}

/*********************************************************************

    Function: adi_sdh_Identify_SDIO_Combo

        Tries to identify if the inserted media is a
        SDIO / SDIO Combo card

*********************************************************************/
__ADI_SDH_SECTION_CODE
static void adi_sdh_Identify_SDIO_Combo (
    ADI_SDH_DEF     *poDevice
)
{
    /* SDH Status register value in response to a command transfer */
    u32     nSdhStatus;

    /* Issue Command to get command SDIO support,
       Short response,
       Card operating condition as command argument,
       Identification mode time out */
    nSdhStatus = adi_sdh_SendCommand (poDevice,
                                      (ADI_SDIO_CMD_GET_SUPPORT |
                                       ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE),
                                      poDevice->CardInfo.OperatingCondition,
                                      (ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD |
                                       ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD),
                                      ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                      ADI_SDH_TIMEOUT_NID);

    /* IF (SDIO Support command successfully received by the media) */
    if (nSdhStatus == 0)
    {
        /* Inserted card should support SDIO commands */
        poDevice->SlotStatus = ADI_SDH_SLOT_SDIO_CARD;

        /* Run SD Version 1.X card identification sequence to
           find if this is a SDIO Combo card */
        adi_sdh_Identify_SD_Version_1_X (poDevice,
                                         poDevice->CardInfo.OperatingCondition);

        /* IF (SD standard Version 1.X identifaction sequence is successful) */
        if (poDevice->SlotStatus == ADI_SDH_SLOT_SD_CARD_VER_1_X)
        {
            /* update slot status as SDIO Combo Card */
            poDevice->SlotStatus = ADI_SDH_SLOT_SDIO_COMBO_CARD;
        }

    } /* End of if (SDIO Support command successfully received by the media) */

}

/*********************************************************************

    Function: adi_sdh_Identify_SD_Version_1_X

        Tries to identify if the inserted media is a
        SD Version 1.X card

*********************************************************************/
__ADI_SDH_SECTION_CODE
static void adi_sdh_Identify_SD_Version_1_X (
    ADI_SDH_DEF     *poDevice,
    u32             nOperatingCondition
)
{
    /* SDH Status register value in response to a command transfer */
    u32     nSdhStatus;
    /* Counter to retry card identification sequence */
    u32     nRetryCount;

    /* Retry card identification until the media is ready to enter data transfer mode
       (or) until the stipulated retry count expires */
    for (nRetryCount = 0;
         nRetryCount < ADI_SDH_SD_MMC_IDENTIFY_RETRY_COUNT;
         nRetryCount++)
    {

        /* Issue Command to set card ready to accept App specific command,
           Short response,
           Relative Card Address as command argument,
           Identification mode time out */
        nSdhStatus = adi_sdh_SendCommand (poDevice,
                                          (ADI_SD_CMD_SET_FOR_APP_SPECIFIC_CMD |
                                           ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE),
                                          poDevice->CardInfo.RCA,
                                          (ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD |
                                           ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD),
                                          ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                          ADI_SDH_TIMEOUT_NID);

        /* IF (Error reported on command transfer) */
        if (nSdhStatus != 0)
        {
            /* Exit loop as the insterted media might not be a
               Version 1.X SD */
            break;
        }

        /* Issue Command to send Card Operating condition,
           Short response,
           Card operating condition with support to
           Version 2.0 SD/SDHC cards as command argument,
           Identification mode time out */
        nSdhStatus = adi_sdh_SendCommand (poDevice,
                                          (ADI_SD_CMD_GET_OCR_VALUE |
                                           ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE),
                                          nOperatingCondition,
                                          (ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD |
                                           ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD),
                                          ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                          ADI_SDH_TIMEOUT_NID);

        /* Neglet CRC error for 2nd card ID command (ACMD 41) */
        nSdhStatus &= ~ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD;

        /* IF (SDH Status error other than Command CRC Failed ) */
        if (nSdhStatus != 0)
        {
            /* Mark slot status as card not supported */
            poDevice->SlotStatus = ADI_SDH_SLOT_CARD_NOT_SUPPORTED;
            break;
        }

        /* IF (Card is ready to enter data transfer mode) */
        if (*pADI_SDH_RESPONSE_0 & ADI_SDH_SD_MMC_OCR_BUSY_MASK)
        {
            /* Internal Card initialisation process completed */
            break;
        }

    } /* End of for (Retry card identification) */

    /* IF (No SDH Status error reported) */
    if (nSdhStatus == 0)
    {
        /* IF (Specified operating condition is not supported by the card (or)
               we've reached our idenification retry limit */
        if (((*pADI_SDH_RESPONSE_0 & poDevice->CardInfo.OperatingCondition) == 0) ||
            (nRetryCount >= ADI_SDH_SD_MMC_IDENTIFY_RETRY_COUNT))
        {
            /* Hardware does not supported the required operating condition of
               the Media Inserted */
            /* Mark slot status as card not supported */
            poDevice->SlotStatus = ADI_SDH_SLOT_CARD_NOT_SUPPORTED;
        }
        /* ELSE (Operating conditions supported by the inserted media and
                 this supports Version 1.X SD standards) */
        else
        {
            /* update slot status as Version 1.X standard capacity SD Card */
            poDevice->SlotStatus = ADI_SDH_SLOT_SD_CARD_VER_1_X;
        }
    }

}

/*********************************************************************

    Function:       adi_sdh_ObtainCardInfo

    Description:    Gets SD/SDIO/Multimedia card specific information
                    Sets Relative Card Address for Multimedia card

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_sdh_ObtainCardInfo(
    ADI_SDH_DEF *pDevice        /* pointer to SDH device we're working on   */
){
    u32 Result= ADI_DEV_RESULT_SUCCESS;
    /* Get card specific information */
    do
    {
        /* Continue only if a supported card was detected */
        if (pDevice->SlotStatus == ADI_SDH_SLOT_EMPTY)
        {
            break;
        }

        /* a supported card type is available in SD slot */
        /* Get SD I/O related information for SDIO & SDIO-Combo cards */
        if ((pDevice->SlotStatus == ADI_SDH_SLOT_SDIO_CARD) || (pDevice->SlotStatus == ADI_SDH_SLOT_SDIO_COMBO_CARD))
        {
            /*** TBD - acquiring I/O specific information ***/
            /* No more card specific information required for SDIO only cards */
            if (pDevice->SlotStatus == ADI_SDH_SLOT_SDIO_CARD)
            {
                break;
            }
        }

        adi_sdh_GetCIDRegister(pDevice);

        if(pDevice->SlotStatus == ADI_SDH_SLOT_EMPTY)
        {
        	break;
        }

        /*** Set Relative Card Address for MMC/ Get Relative Card Address of SD/SDIO-Combo card ***/
        /* Set relative card address for multimedia type card */
        if ((pDevice->SlotStatus == ADI_SDH_SLOT_MMC) || (pDevice->SlotStatus == ADI_SDH_SLOT_MMC_HC))
        {
            pDevice->CardInfo.RCA = ADI_SDH_MMC_RCA_VALUE;
        }

        /* CMD3, short response, RCA value (for MMC) or stuff bits (for SD/SDIO) as argument, Timeout for RCA get/set mode */
        if(adi_sdh_SendCommand (    pDevice,
                                    ADI_SD_MMC_CMD_GET_SET_RELATIVE_ADDR | ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE,
                                    pDevice->CardInfo.RCA,
                                    ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                                    ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                    ADI_SDH_TIMEOUT_NCR
                               ))
        {
            pDevice->SlotStatus = ADI_SDH_SLOT_EMPTY;
            break;
        }

        /* For SD/SDIO-Combo cards - update Card info cache with the received RCA value */
        if ((pDevice->SlotStatus != ADI_SDH_SLOT_MMC) && (pDevice->SlotStatus != ADI_SDH_SLOT_MMC_HC))
        {
            pDevice->CardInfo.RCA = (*pADI_SDH_RESPONSE_0 & ADI_SDH_MMC_RCA_MASK);
        }


        adi_sdh_GetCSDRegister(pDevice);
        if(pDevice->SlotStatus == ADI_SDH_SLOT_EMPTY)
        {
        	break;
        }

        /* calculate & update card's maximum operating frequency */
        if ((Result = adi_sdh_UpdateOperatingFreq(pDevice)) != ADI_DEV_RESULT_SUCCESS)
        {
            break;
        }




        /*** Activate (Select) this card for data access ***/
        /* CMD7, short response, RCA value as argument, Card identification Timeout */
        if(adi_sdh_SendCommand (  pDevice,
                                    ADI_SD_MMC_CMD_SELECT_CARD | ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE,
                                    pDevice->CardInfo.RCA,
                                    ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                                    ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                    ADI_SDH_TIMEOUT_NID
                               ))
        {
            pDevice->SlotStatus = ADI_SDH_SLOT_EMPTY;
            break;
        }

        if((pDevice->SlotStatus == ADI_SDH_SLOT_MMC) || (pDevice->SlotStatus == ADI_SDH_SLOT_MMC_HC))
        {
        	adi_sdh_GetExtendedCSDRegister(pDevice);
        }

        if ((Result = adi_sdh_CalculateCardSize(pDevice)) != ADI_DEV_RESULT_SUCCESS)
        {
            break;
        }

        if(pDevice->SlotStatus == ADI_SDH_SLOT_EMPTY)
        {
        	break;
        }

        /*** Update SDH Data Bus mode ***/
        Result = adi_sdh_ConfigDataBus(pDevice);

    }while(0);

    return(Result);
}

/*********************************************************************

    Function:       adi_sdh_ConfigDataBus

    Description:    Configures SDH data bus width

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_sdh_ConfigDataBus(
    ADI_SDH_DEF *pDevice       /* pointer to SDH device we're working on   */
){
    u32 Result= ADI_DEV_RESULT_SUCCESS;
    u32 Acmd6Arg;
    u8  BusChangeSuccess;   /* SDH Bus config result */


    ADI_SDH_CFG_PD_SDDAT3_DISABLE;  /* disable pull-down on DAT3 */
    ADI_SDH_CFG_PUP_SDDAT3_ENABLE;  /* enable pull-up on DAT3 */

#if defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
     /* widebus and Byte Bus are supported by MMC */
    if ((pDevice->SlotStatus == ADI_SDH_SLOT_MMC) || (pDevice->SlotStatus == ADI_SDH_SLOT_MMC_HC))
    {
        ADI_SDH_CFG_SDIO_4BIT_DISABLE;

        // If wide bus
        if(pDevice->CardInfo.EnableWideBus)
        {
            Acmd6Arg = 0x03B70100;   /* ACMD6 argument, SWITH command, Write Byte Access, Byte 183, value 1 */
        }
        // If Byte bus
        else if(pDevice->CardInfo.EnableByteBus)
        {
            Acmd6Arg = 0x03B70200;  /* ACMD6 argument, SWITH command, Write Byte Access, Byte 183, value 2 */
        }
        // 1-bit bus
        else
        {
            Acmd6Arg = 0x03B70000;  /* ACMD6 argument, SWITH command, Write Byte Access, Byte 183, value 0 */
        }


        if(!adi_sdh_SendCommand (       pDevice,
                                        ADI_MMC_CMD_SWITCH | ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE,
                                        Acmd6Arg,
                                        ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                                        ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                        ADI_SDH_TIMEOUT_NID
                                   ))
        {   /* If no error during send command */
            /* Check no switch error occurred */
            if(!(*pRSI_RESPONSE0 & 0x00000080))
            {
              if(pDevice->CardInfo.EnableWideBus)
                {
                    /* disable wide-bus mode */
                    ADI_SDH_CLK_CTRL_BYTE_BUS_DISABLE;
                    /* disable wide-bus mode */
                    ADI_SDH_CLK_CTRL_WIDE_BUS_ENABLE;
                }

                else if(pDevice->CardInfo.EnableByteBus)
                {
                    /* disable wide-bus mode */
                    ADI_SDH_CLK_CTRL_WIDE_BUS_DISABLE;
                    /* disable wide-bus mode */
                    ADI_SDH_CLK_CTRL_BYTE_BUS_ENABLE;
                }
                // 1-bit bus
                else
                {
                    /* disable wide-bus mode */
                    ADI_SDH_CLK_CTRL_WIDE_BUS_DISABLE;
                    /* disable wide-bus mode */
                    ADI_SDH_CLK_CTRL_BYTE_BUS_DISABLE;
                }
            }

            adi_sdh_UpdateSlotStatus(pDevice);
        }

    }
#else

    /* widebus is not supported by MMC */
    if (pDevice->SlotStatus == ADI_SDH_SLOT_MMC)
    {
            /* disable wide-bus mode */
            ADI_SDH_CLK_CTRL_WIDE_BUS_DISABLE;
            /* disable pull-up/pull-down on DAT3 */
            ADI_SDH_CFG_SDIO_4BIT_DISABLE;
    }
#endif


    /* ELSE, for SD/SDIO cards */
    else
    {
        /* If allowed, use 4-bit Data Bus for SD/SDIO-Combo cards */
        if (pDevice->CardInfo.EnableWideBus)
        {
            Acmd6Arg = ADI_SDH_SD_MMC_STUFF_BITS | 2;   /* ACMD6 argument */
        }
        /* else, disable wide bus mode */
        else
        {
            Acmd6Arg = ADI_SDH_SD_MMC_STUFF_BITS;       /* ACMD6 argument */
        }

#if defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
        /* If requested, select to use 8-bit byte bus Data Bus for embedded, eMMC cards */
        if (pDevice->CardInfo.EnableByteBus)
        {
            Acmd6Arg = ADI_SDH_SD_MMC_STUFF_BITS;      /* no support for 8-bit in SD */
            /* port bits are eventually set by adi_sdh_SetPortControl() */
        }
#endif
        do
        {
            BusChangeSuccess = FALSE;   /* assume the worst */
            /* CMD55, Short response, RCA value as argument, Card identification Timeout */
            if(adi_sdh_SendCommand (    pDevice,
                                        ADI_SD_CMD_SET_FOR_APP_SPECIFIC_CMD | ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE,
                                        pDevice->CardInfo.RCA,
                                        ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                                        ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                        ADI_SDH_TIMEOUT_NID
                                   ))
            {
                break;
            }
#if 0 // deprecated as we should leave the DAT3 pullup enabled
            /* ACMD42, Short response, Argument (stuff bits (31:1) disable DAT3 pullup (bit 0)), Card identification Timeout */
            if(adi_sdh_SendCommand (    pDevice,
                                        ADI_SD_CMD_DISCONNECT_DAT3_PULLUP | ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE,
                                        ADI_SDH_SD_MMC_STUFF_BITS,
                                        ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                                        ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                        ADI_SDH_TIMEOUT_NID
                                   ))
            {
                break;
            }
#endif
            /* CMD55, Short response, RCA value as argument, Card identification Timeout */
            if(adi_sdh_SendCommand (    pDevice,
                                        ADI_SD_CMD_SET_FOR_APP_SPECIFIC_CMD | ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE,
                                        pDevice->CardInfo.RCA,
                                        ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                                        ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                        ADI_SDH_TIMEOUT_NID
                                   ))
            {
                break;
            }

            /* ACMD6, Short response, Argument (stuff bits [31:2], bits [1:0] = 10 (4-bit bus)), Card identification Timeout */
            if(adi_sdh_SendCommand (    pDevice,
                                        ADI_SD_CMD_SET_BUS_WIDTH | ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE,
                                        Acmd6Arg,
                                        ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                                        ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                        ADI_SDH_TIMEOUT_NID
                                   ))
            {
                break;
            }

            BusChangeSuccess = TRUE;    /* SDH buswidth configuration is done */
        }while (0);

        /* update SDH bus only of SDH card bus change results in success */
        if (BusChangeSuccess)
        {
            /* Enable widebus ? */
            if (pDevice->CardInfo.EnableWideBus)
            {
                ADI_SDH_CLK_CTRL_WIDE_BUS_ENABLE;
            }
            else
            {
                ADI_SDH_CLK_CTRL_WIDE_BUS_DISABLE;
                ADI_SDH_CFG_SDIO_4BIT_DISABLE;
            }

#if defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
            /* Enable bytebus ? */
            if (pDevice->CardInfo.EnableByteBus)
            {
                ADI_SDH_CLK_CTRL_BYTE_BUS_ENABLE;
            }
            else
            {
                ADI_SDH_CLK_CTRL_BYTE_BUS_DISABLE;
            }
#endif
        }
    }

    /* Update SDH port pins */
    Result = adi_sdh_SetPortControl(pDevice);

    /* return */
    return(Result);
}

/*********************************************************************

    Function:       adi_sdh_SendCommand

    Description:    Sends SD/SDIO/MMC commands from SDH

    Arguments:
        pDevice         - pointer to SDH device we're working on
        CmdRegVal       - Command register value
        CmdArgument     - Command Argument
        ErrorStatus     - Status bits to monitor for error
        SuccessStatus   - Status bits to monitor for command success
        TimeOut         - Command/response timeout period (in SD CLKs)

    Return value (u32):
        ADI_DEV_RESULT_SUCCESS on success
        SDH Status register value on failure

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_sdh_SendCommand(
    ADI_SDH_DEF     *pDevice,       /* pointer to SDH device we're working on       */
    u16             CmdRegVal,      /* SDH Command register value                   */
    u32             CmdArgument,    /* SDH Command Argument                         */
    u32             ErrorStatus,    /* Status bits to monitor for error             */
    u32             SuccessStatus,  /* Status bits to monitor for command success   */
    u32             TimeOut         /* Command/response timeout period (in SD CLKs) */
){
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    if (!SuccessStatus) SuccessStatus = ADI_SDH_STATUS_CMD_SENT_FIELD;

    /* set command argument */
    ADI_SDH_ARGUMENT_SET_VALUE(CmdArgument);

    /* configure command register */
    ADI_SDH_COMMAND_SET_VALUE(CmdRegVal | ADI_SDH_SD_MMC_CMD_ENABLE_COMMANDS);

    /* cross-check Error/Success status value with present Status register value */
    if (ErrorStatus|SuccessStatus)
    {
        while (!(*pADI_SDH_STATUS & (ErrorStatus|SuccessStatus)));
    }

    /* On status error, pass the status register value as result */
    if (*pADI_SDH_STATUS & ErrorStatus)
    {
        Result = *pADI_SDH_STATUS;
    }

    /* clear all SDH status bits */
    *pADI_SDH_STATUS_CLEAR = (ErrorStatus|SuccessStatus);

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_sdh_CalculateCardSize

    Description:    Calculates SD/SDIO/Multimedia card size
                    and updates driver card info cache

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_sdh_CalculateCardSize(
    ADI_SDH_DEF *pDevice       /* pointer to SDH device we're working on   */
){

    u32 u32Temp;
    u8  u8Temp;
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* save the card block length */
	if((pDevice->SlotStatus == ADI_SDH_SLOT_MMC) || (pDevice->SlotStatus == ADI_SDH_SLOT_MMC_HC))
	{
    	pDevice->CardInfo.BlockLength = pDevice->CardInfo.CardRegisters.mmc.csd.read_bl_len;
    	u8Temp  = pDevice->CardInfo.CardRegisters.mmc.csd.c_size_mult;
       	u32Temp = (1 << (u8Temp + 2));

       	if(pDevice->SlotStatus == ADI_SDH_SLOT_MMC)
       	{
       		pDevice->CardInfo.TotalBlocks = ((pDevice->CardInfo.CardRegisters.mmc.csd.c_size + 1) * u32Temp);
       	}
       	else
       	{
       		pDevice->CardInfo.TotalBlocks = pDevice->CardInfo.CardRegisters.mmc.extcsd.sec_count;

       	}
	}
	else
	{
		pDevice->CardInfo.BlockLength = pDevice->CardInfo.CardRegisters.sd.csd.read_bl_len;
		u8Temp  = pDevice->CardInfo.CardRegisters.sd.csd.c_size_mult;
       	u32Temp = (1 << (u8Temp + 2));

	    /* IF (SDH slot has Version 2.0 SDHC card) */
    	if (pDevice->SlotStatus == ADI_SDH_SLOT_SDHC_CARD)
    	{
       		/* Memory capacity of version 2.0 SDHC card = (C_SIZE + 1) * 512K bytes */
       		/* Get Total block count (1 block = 512 bytes) */
       		/* so, Total block count = ((C_SIZE + 1) * (512KB / 512))
           		Total block count = ((C_SIZE + 1) * 1KB)
       		*/
       		pDevice->CardInfo.TotalBlocks = ((pDevice->CardInfo.CardRegisters.sd.csd.c_size + 1) * 512);
    	}

    	/* ELSE (SDH slot has MMC or Version 1.X/2.0 standard capacity SD card) */
    	else
    	{
       		/* calculate Total number of blocks in this memory card */
      		pDevice->CardInfo.TotalBlocks = ((pDevice->CardInfo.CardRegisters.sd.csd.c_size + 1) * u32Temp);
    	}
	}

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)

   	/* check for valid read/write block length */
   	if (pDevice->CardInfo.BlockLength <= 8)
   	{
       	Result = ADI_DEV_RESULT_FAILED;
   	}
   	if (!pDevice->CardInfo.TotalBlocks)
   	{
       	/* this device has no memory */
       	Result = ADI_DEV_RESULT_FAILED;
   	}

#endif

   	if (Result == ADI_DEV_RESULT_SUCCESS)
   	{
       	/* If needed, resize the card block length to 512 bytes and adjust the total block count accordingly */
       	while(pDevice->CardInfo.BlockLength > 9)
       	{
           	/* Get Total block count, where 1 block = 512 bytes */
           	pDevice->CardInfo.BlockLength--;
           	pDevice->CardInfo.TotalBlocks <<= 1;
       	}
   	}


    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_sdh_UpdateOperatingFreq

    Description:    Calculates SD/SDIO/Multimedia card maximum operating
                    frequency and sets SDH CLK to that value

                    Calculates Maximum read delay (Time out) value for
                    SD/SDIo/MMC memory read

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_sdh_UpdateOperatingFreq(
    ADI_SDH_DEF *pDevice       /* pointer to SDH device we're working on   */
){
    u32 Result= ADI_DEV_RESULT_SUCCESS;
    u32 u32Temp;
    u64 u64Temp;
    /* Tx Rate exponent Map values = Actual value in spec / 10 */
    u32 TxRateMap[]             = {10000,100000,1000000,10000000,0,0,0,0};
    /* Time Exponent Map values = Actual value in spec * 1000 */
    u32 TimeExponentMap[]       = {1000000, 10000000, 100000000, 1000, 10000, 100000, 1, 10 };
    /* Tx Mantissa/TAAC Time value  Map values = Actual value in spec * 10 */
    u32 TxMantissaTimeValMap[]  = {0,10,12,13,15,20,25,30,35,40,45,50,55,60,70,80};


    if((pDevice->SlotStatus == ADI_SDH_SLOT_MMC) || (pDevice->SlotStatus == ADI_SDH_SLOT_MMC_HC))
    {
    	/* calculate card's maximum operating frequency */
    	u32Temp = TxRateMap[(u8)(pDevice->CardInfo.CardRegisters.mmc.csd.tran_speed&0x7)] * TxMantissaTimeValMap[(u8)((pDevice->CardInfo.CardRegisters.mmc.csd.tran_speed&0xF8)>>3)];
    	/* update SD CLK frequency */
    }
    else
    {
    	/* calculate card's maximum operating frequency */
    	u32Temp = TxRateMap[(u8)(pDevice->CardInfo.CardRegisters.sd.csd.tran_speed&0x7)] * TxMantissaTimeValMap[(u8)((pDevice->CardInfo.CardRegisters.sd.csd.tran_speed&0xF8)>>3)];
    	/* update SD CLK frequency */
    }
    Result = adi_sdh_SetClkDiv(pDevice,u32Temp);

    /* Apply override CLKDIV value if set */
    if (pDevice->ClkDiv) {
        *pADI_SDH_CLK_CTRL = ((*pADI_SDH_CLK_CTRL&(~0x00FF)) | (pDevice->ClkDiv&0x00FF));
    }

    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
        /* IF (SDH slot has Version 2.0 SDHC card) */
        if (pDevice->SlotStatus == ADI_SDH_SLOT_SDHC_CARD)
        {
            /* Fixed time out value (100ms for read, 250ms for write), we use 250ms */
            /* calculate equivalent SD clk count for 250ms (equivalent to 4Hz) */
            pDevice->CardInfo.CardTxTimeOut = (pDevice->CardInfo.OperatingFreq/4);
        }
        /* ELSE (SDH slot has MMC or Version 1.X/2.0 standard capacity SD card) */
        else
        {
            /* Calculate Maximum Read/Write access timeout delay in SD CLK cycles */
            /* Maximum read delay = TAAC (in SD CKL cycles) + (NSAC *100) */
            /* calculate TAAC in SD CLK cycles */
            u32Temp =   TimeExponentMap[ADI_SDH_EXTRACT_TAAC_TIME_EXPONENT] *
                        TxMantissaTimeValMap[ADI_SDH_EXTRACT_TAAC_TIME_VALUE] *
                        (pDevice->CardInfo.OperatingFreq/10000);
            pDevice->CardInfo.CardTxTimeOut = u32Temp + (ADI_SDH_EXTRACT_NSAC_VALUE*100);
            /* Writes take longer than reads by factor of 2**R2W_FACTOR */
            pDevice->CardInfo.CardTxTimeOut <<= ADI_SDH_EXTRACT_R2W_FACTOR;
        }

        /* Calculate Maximum Card Timeout value */
        u64Temp = (u64)pDevice->CardInfo.CardTxTimeOut;
        u64Temp *= pDevice->CardInfo.TxTimeOutMultiplier;
        pDevice->CardInfo.MaxCardTxTimeOut =
                (u64Temp > 0xFFFFFFFFull)? 0xFFFFFFFFu : (u32)u64Temp;
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_sdh_SetClkDiv

    Description:    Calculates and configures SD CLK divisor
                    for selected SD Clock frequency

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_sdh_SetClkDiv(
    ADI_SDH_DEF *pDevice,       /* pointer to SDH device we're working on   */
    u32         TargetSDClk     /* Target SD CLK frequency                  */
){
    u8  ClkDiv;
    u32 Result= ADI_DEV_RESULT_SUCCESS;

    /* Calculate SD CLK divisor only if Target SD CLK & SCLK frequency are vaild */
    if (TargetSDClk && pDevice->fsclk)
    {
        /* IF Target SD CLK frequency is more than SCLK, disable SD CLKDIV and use SCLK as SD CLK */
        if (TargetSDClk >= pDevice->fsclk)
        {
            ADI_SDH_CLK_CTRL_CLKDIV_BYPASS_ENABLE;  /* SD_CLK driven from SCLK */
            /* update card operating frequency info */
            pDevice->CardInfo.OperatingFreq = pDevice->fsclk;
            /* delay for 5 Sclk period - settling time for SDH Clk Control register write */
            adi_sdh_Delay (pDevice,pDevice->fsclk,5);
        }
        /* Target SDH must be less than SCLK, Calculate SDH ClkDiv */
        else
        {
            ADI_SDH_CLK_CTRL_CLKDIV_BYPASS_DISABLE;
            /* delay for 5 Sclk period - settling time for SDH Clk Control register write */
            adi_sdh_Delay (pDevice,pDevice->fsclk,5);
            /* SD CLK = SCLK/(2 * CLKDIV +1)  */
            /* CLKDIV = (SCLK/(2 * SD CLK))-1 */
            ClkDiv = (u8)((pDevice->fsclk/(2*TargetSDClk))-1);
            /* check if the calculated ClkDiv will result in a
               SD CLK within the given SD CLK frequency range */
            /* continue until we have a vaild CLKDIV */
            while ((pDevice->fsclk/(2 *(ClkDiv+1))) > TargetSDClk)
            {
                /* reduce SD CLK by incrementing CLKDIV */
                ClkDiv++;
                /* Exit on reaching the max limit */
                if (ClkDiv == 0xffu)
                {
                    break;
                }
            }
            /* update card operating frequency info */
            pDevice->CardInfo.OperatingFreq = (pDevice->fsclk/(2 *(ClkDiv+1)));
            /* Set SD CLK Divisor */
            ADI_SDH_CLK_CTRL_SET_CLKDIV(ClkDiv);
        }
        /* delay for 5 Sclk period - settling time for SDH Clk Control register write */
        adi_sdh_Delay (pDevice,pDevice->fsclk,5);
    }
    else
    {
        Result= ADI_DEV_RESULT_FAILED;
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_sdh_Delay

    Description:    Delay loop in terms of CCLK

*********************************************************************/
#pragma optimize_for_speed
__ADI_SDH_SECTION_CODE
static void adi_sdh_Delay (
    ADI_SDH_DEF *pDevice,       /* pointer to SDH device we're working on   */
    u32         BaseFreq,       /* Base frequency to calculate delay        */
    u32         DelayCycle      /* # base frequency cycles to delay         */
){
    u32 i;

    /* calculate CCLK delay cycles required to match base frequency */
    i = ((pDevice->fcclk + BaseFreq - 1) / BaseFreq) * DelayCycle;

    /* delay */
    while (i--)
        asm volatile("nop;");

    /* return */
}
#pragma optimize_as_cmd_line

/*********************************************************************

    Function:       adi_sdh_InitiateMemAccess

    Description:    Initiates SD/SDIO/MMC Memory access by issuing
                    Card specific commands

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_sdh_InitiateMemAccess (
    ADI_SDH_DEF     *pDevice    /* pointer to SDH device we're working on   */
){
    /* SDH Read/Write command to use */
    u8  nSdhRWCmd;
    /* SDH Data control register value */
    u16 nSdhDataCtrl;
    /* Access retry count to wait for card data ready */
    u32 AccessRetryCount = ADI_SDH_SD_MMC_ACCESS_RETRY_COUNT;
    /* SD/MMC start address to access */
    u32 nStartAddress;
    /* Result */
    u32 Result = ADI_FSS_RESULT_SUCCESS;

    /* pointer to the FSS super buffer we're working on */
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pDevice->CurrentLBARequest.pBuffer;
    
    /* Set PID callback function and PID handle to the current FSS super buffer */
    pSuperBuffer->PIDCallbackFunction   = adi_sdh_CallbackFromFSS;
    pSuperBuffer->PIDCallbackHandle     = (void*)pDevice;

    /* Check whether the SD slot contains a valid device and sector count is not zero */
    if ((pDevice->SlotStatus == ADI_SDH_SLOT_EMPTY) || (pDevice->CurrentLBARequest.SectorCount == 0))
    {
        Result = ADI_FSS_RESULT_FAILED;
    }
    else
    {
        /* wait until the memory device is ready for next data transfer */
        do
        {
            /* Read memory Card status register */
            /* CMD13, short response, RCA as argument, Timeout for ID mode */
            if (adi_sdh_SendCommand (   pDevice,
                                        ADI_SD_MMC_CMD_SEND_STATUS | ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE,
                                        pDevice->CardInfo.RCA,
                                        ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                                        ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                        ADI_SDH_TIMEOUT_NCR
                                    ))
            {
                /* Timeout or CRC error - update SD Slot status */
                adi_sdh_UpdateSlotStatus(pDevice);
                /* return failure */
                Result = ADI_FSS_RESULT_FAILED;
                break;
            }
            AccessRetryCount--;
        } while ((ADI_SDH_SD_MMC_READY_FOR_DATA_STATUS != ADI_SDH_SD_MMC_READY_FOR_DATA) && AccessRetryCount);

        /* Continue only if the device is ready for data transfer */
        if ((Result == ADI_FSS_RESULT_SUCCESS) && (AccessRetryCount))
        {
            /* Clear all SDH status bits */
            ADI_SDH_STATUS_CLEAR_ALL;

            /* Read from SD/SDIO/MMC memory */
            if (pDevice->CurrentLBARequest.ReadFlag)
            {
                /* SDH Data control register - data transfer direction */
                nSdhDataCtrl = ADI_SDH_READ_DATA_FROM_SD_MMC;

                /* Read Command to SD/MMC */
                nSdhRWCmd = ADI_SD_MMC_CMD_READ_MULTIPLE_BLOCKS;
            }
            /* Write to SD/SDIO/MMC memory */
            else
            {
                /* SDH Data control register - data transfer direction */
                nSdhDataCtrl = ADI_SDH_WRITE_DATA_TO_SD_MMC;

                /* Write Command to SD/MMC */
                nSdhRWCmd = ADI_SD_MMC_CMD_WRITE_MULTIPLE_BLOCKS;
            }

            /* set data length to transfer */
            ADI_SDH_DATA_LENGTH_SET(512U * pDevice->CurrentLBARequest.SectorCount);

            /* Set SD/MMC Block Length */
            /* CMD16, Short response, Read/Write block length as Argument, Card transfer Timeout */
            if(adi_sdh_SendCommand (    pDevice,
                                        ADI_SD_MMC_CMD_SET_BLOCK_LENGTH | ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE,
                                        512U,
                                        ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                                        ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                        ADI_SDH_TIMEOUT_NCR
                                   ))
            {
                /* Timeout or CRC error - update SD Slot status */
                adi_sdh_UpdateSlotStatus(pDevice);
                /* return failure */
                Result = ADI_FSS_RESULT_FAILED;
            }

            if (Result == ADI_FSS_RESULT_SUCCESS)
            {

                /* Set start address of Memory device data block to be accessed */
                /* IF (SDH slot has Version 2.0 SDHC card) */
                if ((pDevice->SlotStatus == ADI_SDH_SLOT_SDHC_CARD) || (pDevice->SlotStatus == ADI_SDH_SLOT_MMC_HC))
                {
                    /* for SDHC - access memory in terms of sector numbers */
                    nStartAddress = pDevice->CurrentLBARequest.StartSector;
                }
                /* ELSE (SDH slot has MMC or version 1.x SD) */
                else
                {
                    /* Calculate SD/MMC start address to access */
                    nStartAddress = (512U * pDevice->CurrentLBARequest.StartSector);
                }

                /*
                   Single/Block read/write,
                   Short response,
                   Start sector number (for SDHC)/Data Address (SD/MMC) as Argument,
                   Card transfer Timeout
                */
                if(pDevice->CurrentLBARequest.ReadFlag)
                {
                    /* set the bus state to data transfer in progress mode */
                    pDevice->BusState = ADI_SDH_BUS_XFER_IN_PROGRESS;
                    /* Set Time out for Data transfer */
                    ADI_SDH_DATA_TIMER_SET(pDevice->CardInfo.MaxCardTxTimeOut);
                    /* Configure SDH Data control register - Enable Dataflow */
                    ADI_SDH_DATA_CTRL_SET_VALUE(nSdhDataCtrl);
                }
                if(adi_sdh_SendCommand (    pDevice,
                                            nSdhRWCmd | ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE,
                                            nStartAddress,
                                            ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                                            ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                            ADI_SDH_TIMEOUT_NCR
                                       ))
                {
                    /* Timeout or CRC error - update SD Slot status */
                    adi_sdh_UpdateSlotStatus(pDevice);
                    /* return failure */
                    Result = ADI_FSS_RESULT_FAILED;
                }
            }

            if (Result == ADI_FSS_RESULT_SUCCESS)
            {
                if(!pDevice->CurrentLBARequest.ReadFlag)
                {
                    /* set the bus state to data transfer in progress mode */
                    pDevice->BusState = ADI_SDH_BUS_XFER_IN_PROGRESS;
                    /* Set Time out for Data transfer */
                    ADI_SDH_DATA_TIMER_SET(pDevice->CardInfo.MaxCardTxTimeOut);
                    /* Configure SDH Data control register - Enable Dataflow */
                    ADI_SDH_DATA_CTRL_SET_VALUE(nSdhDataCtrl);

                }
            }
            /* Memory device is still busy. Return control to the file system */
            else
            {
                Result = ADI_FSS_RESULT_FAILED;
            }
        }
    }

    /* return */
    return (Result);
}


/*********************************************************************

    Function:       adi_sdh_UpdateSlotStatus

    Description:    Hot plug support function - updates SD slot status
                    by checking whether the media in SD slot was removed
                    or not. If removed, the function also unmounts
                    any volumes/partitions mounted by SDH.

*********************************************************************/

__ADI_SDH_SECTION_CODE
static void adi_sdh_UpdateSlotStatus (
    ADI_SDH_DEF         *pDevice    /* pointer to SDH device we're working on   */
){
    u8      RetryCount = 3; /* # times to retry before confirming SD slot status */
    u8      MediaDetectCmd; /* Command to be used to detect media */

    /* send a SD/MMC/SDIO specific command and wait for a response or until timeout occurs */
    /* If the device sends a valid response - do nothing and just return
       If no valid response is received, mark SD slot status as Media removed */

    /* IF the slot have/had a SDIO card, send SDIO specific command */
    if (pDevice->SlotStatus == ADI_SDH_SLOT_SDIO_CARD)
    {
        /* TBD */
        /* For now, send a command common to SD/MMC/SDIO-Combo card */
         MediaDetectCmd = ADI_SD_MMC_CMD_SEND_STATUS;
    }
    /* ELSE, the slot must have/had a SD/MMC or SDIO-Combo card */
    else
    {
        MediaDetectCmd = ADI_SD_MMC_CMD_SEND_STATUS;
    }

    do
    {
        /* Command, Short response, RCA as argument, Card identification Timeout */
        if(adi_sdh_SendCommand (    pDevice,
                                    ADI_SD_MMC_CMD_SEND_STATUS | ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE,
                                    pDevice->CardInfo.RCA,
                                    ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                                    ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                    ADI_SDH_TIMEOUT_NCR
                               ))
        {
            RetryCount--;   /* Timeout or CRC error, decrement retry count */
        }
        else
        {
            break;  /* received valid response from the card */
        }
    } while (RetryCount);

    if (!RetryCount)
    {
        /* Mark SD slot status as media removed so that we could unmount any volumes already mounted by SDH */
        pDevice->SlotStatus = ADI_SDH_SLOT_MEDIA_REMOVED;
        /* Reset SDH registers */
        adi_sdh_Reset();
        /* Enable SDH bus */
        adi_sdh_EnableHostBus(pDevice);
        /* Reset SD/MMC/SDIO card information table */
        adi_sdh_ResetCardInfo(pDevice);
        /* enable card detect interrupt */
        ADI_SDH_E_STATUS_MASK_SD_CARD_DETECT_ENABLE;
    }

    /* return */
}

__ADI_SDH_SECTION_CODE
static void adi_sdh_WaitForReady (
    ADI_SDH_DEF         *pDevice    /* pointer to SDH device we're working on   */
){
    u8      RetryCount = 3; /* # times to retry before confirming SD slot status */

    do
    {
        /* Command, Short response, RCA as argument, Card identification Timeout */
        if(adi_sdh_SendCommand (    pDevice,
                                    ADI_SD_MMC_CMD_SEND_STATUS | ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE,
                                    pDevice->CardInfo.RCA,
                                    ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                                    ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                    ADI_SDH_TIMEOUT_NCR
                               ))
        {
            RetryCount--;   /* Timeout or CRC error, decrement retry count */
        }
        else
        {
            if(*pADI_SDH_RESPONSE_0 & 0x00000100)
            {
                break;
            }
        }
    } while (RetryCount);

    if (!RetryCount)
    {
        /* Mark SD slot status as media removed so that we could unmount any volumes already mounted by SDH */
        pDevice->SlotStatus = ADI_SDH_SLOT_MEDIA_REMOVED;
        /* Reset SDH registers */
        adi_sdh_Reset();
        /* Enable SDH bus */
        adi_sdh_EnableHostBus(pDevice);
        /* Reset SD/MMC/SDIO card information table */
        adi_sdh_ResetCardInfo(pDevice);
        /* enable card detect interrupt */
        ADI_SDH_E_STATUS_MASK_SD_CARD_DETECT_ENABLE;
    }

    /* return */
}

/*********************************************************************

    Function:       adi_sdh_InterruptHandler

    Description:    Handles SDH interrupts

*********************************************************************/
__ADI_SDH_SECTION_CODE
static ADI_INT_HANDLER(adi_sdh_InterruptHandler)    /* SDH Interrupt handler */
{
    ADI_INT_HANDLER_RESULT  Result;
    ADI_SDH_DEF *pDevice = (ADI_SDH_DEF*)ClientArg;
    u32 i;

    /* save current status for diagnostic purposes */
    pDevice->CardInfo.LatestStatus = *pADI_SDH_STATUS;

    /* assume the interrupt is not for us */
    Result  = ADI_INT_RESULT_NOT_PROCESSED;
    /* SDH data transfer error or SDH data transfer complete */
    if (ADI_SDH_STATUS_IS_DATA_TX_ERROR | ADI_SDH_STATUS_DATA_END)
    {

        #if defined(_MMC_STOP_TRANSMISSION_WORKAROUND__)
        /* Workaround for some MMC cards that are funny when it comes to sending the STOP_TRANSMISSION command */
        /* Adding a delay resolves the problem */
        /* This does not appear to be applicable to SD cards or the eMMC device */
        for(i=0; i<1024; i++);
        #endif

        /* Issue a Stop command to terminate data transfer */
        /* CMD12, short response, stuff bits as argument, Timeout for ID mode */
        adi_sdh_SendCommand (  pDevice,
                               ADI_SD_MMC_CMD_STOP_TRANSMISSION | ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE,
                               ADI_SDH_SD_MMC_STUFF_BITS,
                               ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                               ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                               ADI_SDH_TIMEOUT_NCR
                            );

        /* If SDH data transfer results in error */
        if (ADI_SDH_STATUS_IS_DATA_TX_ERROR)
        {
            if ((ADI_SDH_STATUS_TX_FIFO_UNDERRUN || ADI_SDH_STATUS_RX_FIFO_OVERRUN) &&
                    !pDevice->InErrorRecovery)
            {
                /* Need to reset the DMA channel otherwise it just hangs */
                pDevice->InErrorRecovery = TRUE;

                /* clear data Tx error status */
                ADI_SDH_STATUS_CLEAR_DATA_TX_ERROR;

                /* restore CDPRIO setting */
                adi_sdh_RestoreCdprio(pDevice);

                /* Disable DMA dataflow */
                adi_dma_Control(pDevice->SdhDmaInfo.ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)FALSE);
                /* Flush SDH DMA */
                adi_dma_Control(pDevice->SdhDmaInfo.ChannelHandle, ADI_DMA_CMD_FLUSH, NULL);
                /* Enable DMA dataflow */
                adi_dma_Control(pDevice->SdhDmaInfo.ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)TRUE);
                /* requeue DMA transfer(s) */
                adi_sdh_SubmitBuffers(pDevice,
                                      ADI_DEV_1D,
                                      (ADI_DEV_BUFFER*)&(pDevice->CurrentLBARequest.pBuffer->Buffer),
                                      (pDevice->CurrentLBARequest.ReadFlag)?
                                          ADI_DEV_DIRECTION_INBOUND :
                                          ADI_DEV_DIRECTION_OUTBOUND);

                /* halve the card's operating frequency and retry the I/O */
                adi_dev_Control(pDevice->DeviceHandle, ADI_PID_CMD_ENABLE_DATAFLOW, (void*)FALSE);
                adi_sdh_SetClkDiv(pDevice, pDevice->CardInfo.OperatingFreq / 2);
                pDevice->CardInfo.HalvingCount += 1;
                adi_dev_Control(pDevice->DeviceHandle, ADI_PID_CMD_ENABLE_DATAFLOW, (void*)TRUE);

                /* Set state to DMA failed */
                pDevice->BusState = ADI_SDH_BUS_DMA_INT_XFER_FAILED;

                return ADI_INT_RESULT_PROCESSED;
            }
            else
            {
                /* Set data transfer as failed */
                pDevice->BusState = ADI_SDH_BUS_DEV_INT_XFER_FAILED;

                /* clear data Tx error status */
                ADI_SDH_STATUS_CLEAR_DATA_TX_ERROR;

                pDevice->InErrorRecovery = FALSE;
            }
        }
        /* ELSE, SDH Data transfer complete */
        else
        {
            /* clear data complete status flag */
            AndyVar+=1;
            if (AndyVar == 0x109)
            {
            	asm("nop;");
            }
            ADI_SDH_STATUS_CLEAR_DATA_END;
            ADI_SDH_STATUS_CLEAR_DATA_BLOCK_END;
            /* update SDH bus state as SDH device interrupt data transfer complete */
            BEGIN_CRITICAL_REGION
                pDevice->BusState |= ADI_SDH_BUS_DEV_INT_XFER_DONE;
            END_CRITICAL_REGION

            pDevice->InErrorRecovery = FALSE;
        }
        Result = ADI_INT_RESULT_PROCESSED;
        /* process this interrupt */
        adi_sdh_ProcessInterrupt(pDevice);
    }

    /* Card detect interrupt occured? */
    if (ADI_SDH_E_STATUS_IS_SD_CARD_DETECT_ENABLED && ADI_SDH_E_STATUS_IS_SD_CARD_DETECTED)
    {
        /* Reset SDH registers */
        adi_sdh_Reset();
        /* Enable SDH bus */
        adi_sdh_EnableHostBus(pDevice);
        /* Reset SD/MMC/SDIO card information table */
        adi_sdh_ResetCardInfo(pDevice);
        /* update SD slot status */
        if (pDevice->SlotStatus == ADI_SDH_SLOT_EMPTY)
        {
            /* Mark SD slot status as new media inserted */
            pDevice->SlotStatus = ADI_SDH_SLOT_MEDIA_INSERTED;
        }
        else
        {
            /*
               The Slot previously had a valid media.
               Mark SD slot status media removed so that we could unmount any volumes previously mounted SDH
            */
            pDevice->SlotStatus = ADI_SDH_SLOT_MEDIA_REMOVED;
        }

        Result = ADI_INT_RESULT_PROCESSED;
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_sdh_CallbackFromFSS

    Description:    The callback to be taken upon buffer completion and
                    device interrupt callbacks.
*********************************************************************/
__ADI_SDH_SECTION_CODE
static void adi_sdh_CallbackFromFSS (
    void    *pHandle,                   /* Client Handle                                */
    u32     Event,                      /* Callback Event                               */
    void    *pArg                       /* Buffer address that caused this callback     */
){
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pArg;
    ADI_SDH_DEF *pDevice = (ADI_SDH_DEF *)pHandle;

    /* CASEOF (Event) */
    switch (Event)
    {
        /* CASE: PID Event device interrupt occured */
        case (ADI_PID_EVENT_DEVICE_INTERRUPT):
            /* if the Data semaphore belongs to SDH PID */
            if (pSuperBuffer->SemaphoreHandle == pDevice->DataSemaphoreHandle)
            {
                /* release SDH device Lock Semaphore */
                adi_dev_Control(pDevice->DeviceHandle, ADI_FSS_CMD_RELEASE_LOCK_SEMAPHORE, NULL);
                /* and post the data semaphore */
                adi_sem_Post(pSuperBuffer->SemaphoreHandle);
            }
            break;

        /* CASE: Device event buffer processed (SDH DMA interrupt occured) */
        case (ADI_DEV_EVENT_BUFFER_PROCESSED):
            /* IF (this is the first time we're getting here after
                    the start of this LBA transfer) */
            if (pDevice->BusState < ADI_SDH_BUS_XFER_COMPLETE)
            {
                /* update SDH bus state as SDH device interrupt data transfer complete */
                BEGIN_CRITICAL_REGION
                    pDevice->BusState |= ADI_SDH_BUS_DMA_INT_XFER_DONE;
                END_CRITICAL_REGION
                
                /* process this interrupt */
                adi_sdh_ProcessInterrupt(pDevice);
            }
            break;

        /* other events - assumed as DMA error/SDH DMA failure */
        default:
            /* update SDH bus state as SDH DMA interrupt data transfer failed */
            pDevice->BusState = ADI_SDH_BUS_DMA_INT_XFER_FAILED;
            /* process this interrupt */
            adi_sdh_ProcessInterrupt(pDevice);
            break;
    }
    /* return */
}


/*********************************************************************

    Function:       adi_sdh_RestoreCdprio

    Description:    Restores CDPRIO setting to default

*********************************************************************/
__ADI_SDH_SECTION_CODE
static void adi_sdh_RestoreCdprio (
    ADI_SDH_DEF         *pDevice    /* pointer to SDH device we're working on   */
){
    if (pDevice->CdprioModeSupport != FALSE)
    {
        u32 DMA_running;

        BEGIN_CRITICAL_REGION

            adi_dma_Control(pDevice->SdhDmaInfo.ChannelHandle,
                            ADI_DMA_CMD_GET_TRANSFER_STATUS,
                            (void*)&DMA_running);
            if (DMA_running == FALSE) {
                /* restore the default CDPRIO setting */
                *pEBIU_AMGCTL &= ~CDPRIO;
                *pEBIU_AMGCTL |= pDevice->SavedCdprio;
            }

        END_CRITICAL_REGION
    }
}


/*********************************************************************

    Function:       adi_sdh_ProcessInterrupt

    Description:    Processes SDH Device and DMA interrupts

*********************************************************************/
__ADI_SDH_SECTION_CODE
static void adi_sdh_ProcessInterrupt (
    ADI_SDH_DEF         *pDevice    /* pointer to SDH device we're working on   */
){

    /* pointer to the FSS super buffer we're working on */
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pDevice->CurrentLBARequest.pBuffer;
    /* if the Data semaphore belongs to SDH PID */
    if ((pDevice->BusState == (ADI_SDH_BUS_DEV_INT_XFER_DONE | ADI_SDH_BUS_XFER_IN_PROGRESS)) &&
        (pSuperBuffer->SemaphoreHandle == pDevice->DataSemaphoreHandle))
    {
        /* restore CDPRIO setting */
        adi_sdh_RestoreCdprio(pDevice);

        /* post callback to release Data semaphore */
        (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_PID_EVENT_DEVICE_INTERRUPT, (void*)pSuperBuffer);
        /* Set SDH bus as idle */
        pDevice->BusState = ADI_SDH_BUS_IDLE;
    }
    /* Data semaphore does not belong to SDH PID. Post callback to release SDH PID lock ? */
    else if (pDevice->BusState >= ADI_SDH_BUS_XFER_COMPLETE)
    {
        /* SDH data transfer is complete or failed */
        /* SDH data transfer failed? */
        if (pDevice->BusState != ADI_SDH_BUS_XFER_COMPLETE)
        {
            /* clear processed element count to report data transfer failure to the application */
            (pDevice->CurrentLBARequest.pBuffer)->Buffer.ProcessedElementCount = 0;
        }

        /* restore CDPRIO setting */
        adi_sdh_RestoreCdprio(pDevice);

        /* post callback to release Data semaphore & PID Lock Semaphore */
        (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_PID_EVENT_DEVICE_INTERRUPT, (void*)pSuperBuffer);
        /* Set SDH bus as idle */
        pDevice->BusState = ADI_SDH_BUS_IDLE;
    }
    /* SDH is idle or still transferring data */
    else
    {
        /* nothing to do here */
    }

    /* return */
}

/* Static functions for Moab/Brodie class devices */
#if defined(__ADSP_MOAB__) || defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
/*********************************************************************

    Function:       adi_sdh_SetPortControl

    Description:    Configures the general purpose ports of BF54x
                    class devices for SDH usage.

*********************************************************************/
__ADI_SDH_SECTION_CODE
static u32 adi_sdh_SetPortControl(ADI_SDH_DEF *pDevice)     /* configures port control registers  for SDH use  */
{
     /* Directives to enable SDH Clock and Data ports */
    ADI_PORTS_DIRECTIVE     SdhDirectives [] =
    {
        ADI_PORTS_DIRECTIVE_SDH_CLK,
        ADI_PORTS_DIRECTIVE_SDH_CMD,
        ADI_PORTS_DIRECTIVE_SDH_D0,
        ADI_PORTS_DIRECTIVE_SDH_D1,
        ADI_PORTS_DIRECTIVE_SDH_D2,
        ADI_PORTS_DIRECTIVE_SDH_D3,

#if defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
        /* only Brodie & Moy supports 8-bit mode (on eMMC) */
        ADI_PORTS_DIRECTIVE_SDH_D4,
        ADI_PORTS_DIRECTIVE_SDH_D5,
        ADI_PORTS_DIRECTIVE_SDH_D6,
        ADI_PORTS_DIRECTIVE_SDH_D7,
#endif

    };
    /* Number of directives to be passed */
    u32     nDirectives;
    u32     Result;

    /* by default - enable SDH Clk, Cmd & D0 pins; 1-bit mode */
    nDirectives = 3;

    /* enable extra pins if SDIO 4-Bit mode or wide bus mode is selected */
    if (pDevice->CardInfo.EnableWideBus || ADI_SDH_CFG_IS_SDIO_4BIT_ENABLED || ADI_SDH_CLK_CTRL_IS_WIDE_BUS_ENABLED)
    {
        nDirectives = 6;
    }


#if defined(__ADSP_BRODIE__) || defined(__ADSP_MOY__)
    /* only Brodie & Moy supports 8-bit mode (on eMMC) */
    /* enable even more extra pins if byte bus mode is selected */
    if (pDevice->CardInfo.EnableByteBus || ADI_SDH_CLK_CTRL_IS_BYTE_BUS_ENABLED)
    {
        nDirectives = 10;
    }
#endif

    /* call port control to enable selected pins for SDH use */
    Result = adi_ports_Configure(&SdhDirectives[0], nDirectives);

    /* return */
    return (Result);
}

#endif

__ADI_SDH_SECTION_CODE
static void adi_sdh_GetCIDRegister (
    ADI_SDH_DEF         *pDevice    /* pointer to SDH device we're working on   */
){
	/*** Get SD/MMC card identification number (CID) ***/
    /* CMD2, long response, stuff bits as argument, Timeout for card ID mode */
    if(adi_sdh_SendCommand (    pDevice,
                                ADI_SD_MMC_CMD_GET_ALL_CID | ADI_SDH_SD_MMC_CMD_LONG_RESPONSE,
                                ADI_SDH_SD_MMC_STUFF_BITS,
                                ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                                ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                ADI_SDH_TIMEOUT_NID
                           ))
    {
        pDevice->SlotStatus = ADI_SDH_SLOT_EMPTY;
    }

    else
    {
    	if((pDevice->SlotStatus == ADI_SDH_SLOT_MMC) || (pDevice->SlotStatus == ADI_SDH_SLOT_MMC_HC))
    	{
    		pDevice->CardInfo.CardRegisters.mmc.cid.mid = ADI_MMC_CID_MID;
        	pDevice->CardInfo.CardRegisters.mmc.cid.cbx = ADI_MMC_CID_CBX;
        	pDevice->CardInfo.CardRegisters.mmc.cid.oid = ADI_MMC_CID_OID;
        	pDevice->CardInfo.CardRegisters.mmc.cid.pnm[0] = ADI_MMC_CID_PNM0;
        	pDevice->CardInfo.CardRegisters.mmc.cid.pnm[1] = ADI_MMC_CID_PNM1;
        	pDevice->CardInfo.CardRegisters.mmc.cid.pnm[2] = ADI_MMC_CID_PNM2;
        	pDevice->CardInfo.CardRegisters.mmc.cid.pnm[3] = ADI_MMC_CID_PNM3;
        	pDevice->CardInfo.CardRegisters.mmc.cid.pnm[4] = ADI_MMC_CID_PNM4;
        	pDevice->CardInfo.CardRegisters.mmc.cid.pnm[5] = ADI_MMC_CID_PNM5;
        	pDevice->CardInfo.CardRegisters.mmc.cid.prv = ADI_MMC_CID_PRV;
        	pDevice->CardInfo.CardRegisters.mmc.cid.psn = ADI_MMC_CID_PSN;
        	pDevice->CardInfo.CardRegisters.mmc.cid.mdt = ADI_MMC_CID_MDT;
        	pDevice->CardInfo.CardRegisters.mmc.cid.crc = ADI_MMC_CID_CRC;
    	}
    	else if((pDevice->SlotStatus == ADI_SDH_SLOT_SD_CARD_VER_1_X) ||
    			(pDevice->SlotStatus == ADI_SDH_SLOT_SD_CARD_VER_2_0) ||
    			(pDevice->SlotStatus == ADI_SDH_SLOT_SDHC_CARD))
    	{
    		pDevice->CardInfo.CardRegisters.sd.cid.mid = (*pADI_SDH_RESPONSE_0 & 0xFF000000) >> 24;
        	pDevice->CardInfo.CardRegisters.sd.cid.oid[0] = (*pADI_SDH_RESPONSE_0 & 0x00FF0000) >> 16;
        	pDevice->CardInfo.CardRegisters.sd.cid.oid[1] = (*pADI_SDH_RESPONSE_0 & 0x0000FF00) >> 8;
        	pDevice->CardInfo.CardRegisters.sd.cid.pnm[0] = (*pADI_SDH_RESPONSE_0 & 0x000000FF);
        	pDevice->CardInfo.CardRegisters.sd.cid.pnm[1] = (*pADI_SDH_RESPONSE_1 & 0xFF000000) >> 24;
        	pDevice->CardInfo.CardRegisters.sd.cid.pnm[2] = (*pADI_SDH_RESPONSE_1 & 0x00FF0000) >> 16;
        	pDevice->CardInfo.CardRegisters.sd.cid.pnm[3] = (*pADI_SDH_RESPONSE_1 & 0x0000FF00) >> 8;
        	pDevice->CardInfo.CardRegisters.sd.cid.pnm[4] = (*pADI_SDH_RESPONSE_1 & 0x000000FF);
        	pDevice->CardInfo.CardRegisters.sd.cid.prv = (*pADI_SDH_RESPONSE_2 & 0xFF000000) >> 24;
        	pDevice->CardInfo.CardRegisters.sd.cid.psn = ((*pADI_SDH_RESPONSE_2 & 0x00FFFFFF)<< 8) | ((*pADI_SDH_RESPONSE_3 & (0x7F8000000>>1))>> 22);
        	pDevice->CardInfo.CardRegisters.sd.cid.mdt = (*pADI_SDH_RESPONSE_3 & (0x0007FF80>>1))  >> 6;
        	pDevice->CardInfo.CardRegisters.sd.cid.crc = (*pADI_SDH_RESPONSE_3 & (0x0000007F>>1));
    	}
    }
}

__ADI_SDH_SECTION_CODE
static void adi_sdh_GetCSDRegister (
    ADI_SDH_DEF         *pDevice    /* pointer to SDH device we're working on   */
){
	/*** Read Card Specific Data (CSD) register ***/
    /* CMD9, long response, RCA value as argument, Card identification Timeout */
    if(adi_sdh_SendCommand (    pDevice,
                                ADI_SD_MMC_CMD_SEND_CSD | ADI_SDH_SD_MMC_CMD_LONG_RESPONSE,
                                pDevice->CardInfo.RCA,
                                ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD,
                                ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD,
                                ADI_SDH_TIMEOUT_NID
                           ))
    {
        pDevice->SlotStatus = ADI_SDH_SLOT_EMPTY;
    }
    else
    {
    	if((pDevice->SlotStatus == ADI_SDH_SLOT_MMC) || (pDevice->SlotStatus == ADI_SDH_SLOT_MMC_HC))
    	{
    		pDevice->CardInfo.CardRegisters.mmc.csd.csd_structure = (ADI_MMC_CSD_STRUCTURE)((*pADI_SDH_RESPONSE_0 & 0xC0000000)>>30);
            pDevice->CardInfo.CardRegisters.mmc.csd.spec_vers = (ADI_MMC_CSD_SPEC_VERS)((*pADI_SDH_RESPONSE_0 & 0x3C000000)>>26);
            pDevice->CardInfo.CardRegisters.mmc.csd.taac = (*pADI_SDH_RESPONSE_0 & 0x00FF0000)>>16;
            pDevice->CardInfo.CardRegisters.mmc.csd.nsac = (*pADI_SDH_RESPONSE_0 & 0x0000FF00)>>8;
            pDevice->CardInfo.CardRegisters.mmc.csd.tran_speed = *pADI_SDH_RESPONSE_0 & 0x000000FF;
            pDevice->CardInfo.CardRegisters.mmc.csd.ccc = (*pADI_SDH_RESPONSE_1 & 0xFFF00000)>>20;
            pDevice->CardInfo.CardRegisters.mmc.csd.read_bl_len = (*pADI_SDH_RESPONSE_1 & 0x000F0000)>>16;
            pDevice->CardInfo.CardRegisters.mmc.csd.read_bl_partial = (*pADI_SDH_RESPONSE_1 & 0x00008000)>>15;
            pDevice->CardInfo.CardRegisters.mmc.csd.write_blk_misalign = (*pADI_SDH_RESPONSE_1 & 0x00004000)>>14;
            pDevice->CardInfo.CardRegisters.mmc.csd.read_blk_misalign = (*pADI_SDH_RESPONSE_1 & 0x00002000)>>13;
            pDevice->CardInfo.CardRegisters.mmc.csd.dsr_imp = (*pADI_SDH_RESPONSE_1 & 0x00001000)>>12;
            pDevice->CardInfo.CardRegisters.mmc.csd.c_size = ((*pADI_SDH_RESPONSE_1 & 0x000003FF)<<2) | ((*pADI_SDH_RESPONSE_2 & 0xC0000000)>>30);
            pDevice->CardInfo.CardRegisters.mmc.csd.vdd_r_curr_min = (ADI_MMC_CSD_VDD_CURR_MIN)(*pADI_SDH_RESPONSE_2 & 0x38000000)>>27;
            pDevice->CardInfo.CardRegisters.mmc.csd.vdd_r_curr_max = (ADI_MMC_CSD_VDD_CURR_MAX)(*pADI_SDH_RESPONSE_2 & 0x07000000)>>24;
            pDevice->CardInfo.CardRegisters.mmc.csd.vdd_w_curr_min = (ADI_MMC_CSD_VDD_CURR_MIN)(*pADI_SDH_RESPONSE_2 & 0x00E00000)>>21;
            pDevice->CardInfo.CardRegisters.mmc.csd.vdd_w_curr_max = (ADI_MMC_CSD_VDD_CURR_MAX)(*pADI_SDH_RESPONSE_2 & 0x001C0000)>>18;
            pDevice->CardInfo.CardRegisters.mmc.csd.c_size_mult = (*pADI_SDH_RESPONSE_2 & 0x00038000)>>15;
            pDevice->CardInfo.CardRegisters.mmc.csd.erase_grp_size = (*pADI_SDH_RESPONSE_2 & 0x00007C00)>>10;
            pDevice->CardInfo.CardRegisters.mmc.csd.erase_grp_mult = (*pADI_SDH_RESPONSE_2 & 0x000003E0)>>5;
            pDevice->CardInfo.CardRegisters.mmc.csd.wp_grp_size = *pADI_SDH_RESPONSE_2 & 0x0000001F;
            pDevice->CardInfo.CardRegisters.mmc.csd.wp_grp_enable = (*pADI_SDH_RESPONSE_3 & (0x80000000>>1))>>30;
            pDevice->CardInfo.CardRegisters.mmc.csd.default_ecc = (ADI_MMC_CSD_ECC)(*pADI_SDH_RESPONSE_3 & (0x60000000>>1))>>28;
            pDevice->CardInfo.CardRegisters.mmc.csd.r2w_factor = (*pADI_SDH_RESPONSE_3 & (0x1C000000>>1))>>25;
            pDevice->CardInfo.CardRegisters.mmc.csd.write_bl_length = (*pADI_SDH_RESPONSE_3 & (0x03C00000>>1))>>21;
            pDevice->CardInfo.CardRegisters.mmc.csd.write_bl_partial = (*pADI_SDH_RESPONSE_3 & (0x00200000>>1))>>20;
            pDevice->CardInfo.CardRegisters.mmc.csd.content_prot_app = (*pADI_SDH_RESPONSE_3 & (0x00010000>>1))>>15;
            pDevice->CardInfo.CardRegisters.mmc.csd.file_format_grp = (*pADI_SDH_RESPONSE_3 & (0x00008000>>1))>>14;
            pDevice->CardInfo.CardRegisters.mmc.csd.copy = (*pADI_SDH_RESPONSE_3 & (0x00004000>>1))>>13;
            pDevice->CardInfo.CardRegisters.mmc.csd.perm_write_protect = (*pADI_SDH_RESPONSE_3 & (0x00002000>>1))>>12;
            pDevice->CardInfo.CardRegisters.mmc.csd.tmp_write_protect = (*pADI_SDH_RESPONSE_3 & (0x00001000>>1))>>11;
            pDevice->CardInfo.CardRegisters.mmc.csd.file_format = (ADI_MMC_CSD_FILE_FORMAT)(*pADI_SDH_RESPONSE_3 & (0x00000C00>>1))>>9;
            pDevice->CardInfo.CardRegisters.mmc.csd.ecc = (ADI_MMC_CSD_ECC)((*pADI_SDH_RESPONSE_3 & (0x00000300>>1))>>7);
            pDevice->CardInfo.CardRegisters.mmc.csd.crc = (*pADI_SDH_RESPONSE_3 & (0x000000FE>>1))>>0;

            pDevice->CardInfo.CardCmdClasses = (u16)pDevice->CardInfo.CardRegisters.mmc.csd.ccc;

            if(pDevice->CardInfo.CardRegisters.mmc.csd.perm_write_protect | pDevice->CardInfo.CardRegisters.mmc.csd.tmp_write_protect)
            {
            	pDevice->CardInfo.WriteProtect = TRUE;
            }
            else
            {
            	pDevice->CardInfo.WriteProtect = FALSE;
            }

    	}
    	else if((pDevice->SlotStatus == ADI_SDH_SLOT_SD_CARD_VER_1_X) ||
    			(pDevice->SlotStatus == ADI_SDH_SLOT_SD_CARD_VER_2_0) ||
    			(pDevice->SlotStatus == ADI_SDH_SLOT_SDHC_CARD))
    	{
    		pDevice->CardInfo.CardRegisters.sd.csd.csd_structure = (ADI_SD_CSD_STRUCTURE)((*pADI_SDH_RESPONSE_0 & 0xC0000000)>>30);
            pDevice->CardInfo.CardRegisters.sd.csd.taac = (*pADI_SDH_RESPONSE_0 & 0x00FF0000)>>16;
            pDevice->CardInfo.CardRegisters.sd.csd.nsac = (*pADI_SDH_RESPONSE_0 & 0x0000FF00)>>8;
            pDevice->CardInfo.CardRegisters.sd.csd.tran_speed = *pADI_SDH_RESPONSE_0 & 0x000000FF;
            pDevice->CardInfo.CardRegisters.sd.csd.ccc = (*pADI_SDH_RESPONSE_1 & 0xFFF00000)>>20;
            pDevice->CardInfo.CardRegisters.sd.csd.read_bl_len = (*pADI_SDH_RESPONSE_1 & 0x000F0000)>>16;
            pDevice->CardInfo.CardRegisters.sd.csd.read_bl_partial = (*pADI_SDH_RESPONSE_1 & 0x00008000)>>15;
            pDevice->CardInfo.CardRegisters.sd.csd.write_blk_misalign = (*pADI_SDH_RESPONSE_1 & 0x00004000)>>14;
            pDevice->CardInfo.CardRegisters.sd.csd.read_blk_misalign = (*pADI_SDH_RESPONSE_1 & 0x00002000)>>13;
            pDevice->CardInfo.CardRegisters.sd.csd.dsr_imp = (*pADI_SDH_RESPONSE_1 & 0x00001000)>>12;
            if(pDevice->CardInfo.CardRegisters.sd.csd.csd_structure == ADI_SD_CSD_STRUCTURE_VERSION_1_0)
            {
                pDevice->CardInfo.CardRegisters.sd.csd.c_size = ((*pADI_SDH_RESPONSE_1 & 0x000003FF)<<2) | ((*pADI_SDH_RESPONSE_2 & 0xC0000000)>>30);
                pDevice->CardInfo.CardRegisters.sd.csd.vdd_r_curr_min = (ADI_SD_CSD_VDD_CURR_MIN)(*pADI_SDH_RESPONSE_2 & 0x38000000)>>27;
                pDevice->CardInfo.CardRegisters.sd.csd.vdd_r_curr_max = (ADI_SD_CSD_VDD_CURR_MAX)(*pADI_SDH_RESPONSE_2 & 0x07000000)>>24;
                pDevice->CardInfo.CardRegisters.sd.csd.vdd_w_curr_min = (ADI_SD_CSD_VDD_CURR_MIN)(*pADI_SDH_RESPONSE_2 & 0x00E00000)>>21;
                pDevice->CardInfo.CardRegisters.sd.csd.vdd_w_curr_max = (ADI_SD_CSD_VDD_CURR_MAX)(*pADI_SDH_RESPONSE_2 & 0x001C0000)>>18;
                pDevice->CardInfo.CardRegisters.sd.csd.c_size_mult = (*pADI_SDH_RESPONSE_2 & 0x00038000)>>15;
            }
            else if(pDevice->CardInfo.CardRegisters.sd.csd.csd_structure == ADI_SD_CSD_STRUCTURE_VERSION_2_0)
            {
                pDevice->CardInfo.CardRegisters.sd.csd.vdd_r_curr_min = ADI_SD_CSD_VDD_CURR_MIN_NOT_APPLICABLE;
                pDevice->CardInfo.CardRegisters.sd.csd.vdd_r_curr_max = ADI_SD_CSD_VDD_CURR_MAX_NOT_APPLICABLE;
                pDevice->CardInfo.CardRegisters.sd.csd.vdd_w_curr_min = ADI_SD_CSD_VDD_CURR_MIN_NOT_APPLICABLE;
                pDevice->CardInfo.CardRegisters.sd.csd.vdd_w_curr_max = ADI_SD_CSD_VDD_CURR_MAX_NOT_APPLICABLE;
                pDevice->CardInfo.CardRegisters.sd.csd.c_size_mult = 0;
                pDevice->CardInfo.CardRegisters.sd.csd.c_size = ((*pADI_SDH_RESPONSE_1 & 0x0000003F)<<16) | ((*pADI_SDH_RESPONSE_2 & 0xFFFF0000)>>16);
            }
            else
            {
                pDevice->SlotStatus = ADI_SDH_SLOT_EMPTY; /* error */
            }
            pDevice->CardInfo.CardRegisters.sd.csd.erase_blk_en = (*pADI_SDH_RESPONSE_2 & 0x00004000)>>14;
            pDevice->CardInfo.CardRegisters.sd.csd.sector_size = (*pADI_SDH_RESPONSE_2 & 0x00003F80)>>7;
            pDevice->CardInfo.CardRegisters.sd.csd.wp_grp_size = *pADI_SDH_RESPONSE_2 & 0x0000007F;
            pDevice->CardInfo.CardRegisters.sd.csd.wp_grp_enable = (*pADI_SDH_RESPONSE_3 & (0x80000000>>1))>>30;
            pDevice->CardInfo.CardRegisters.sd.csd.r2w_factor = (*pADI_SDH_RESPONSE_3 & (0x1C000000>>1))>>25;
            pDevice->CardInfo.CardRegisters.sd.csd.write_bl_length = (*pADI_SDH_RESPONSE_3 & (0x03C00000>>1))>>21;
            pDevice->CardInfo.CardRegisters.sd.csd.write_bl_partial = (*pADI_SDH_RESPONSE_3 & (0x00200000>>1))>>20;
            pDevice->CardInfo.CardRegisters.sd.csd.file_format_grp = (*pADI_SDH_RESPONSE_3 & (0x00008000>>1))>>14;
            pDevice->CardInfo.CardRegisters.sd.csd.copy = (*pADI_SDH_RESPONSE_3 & (0x00004000>>1))>>13;
            pDevice->CardInfo.CardRegisters.sd.csd.perm_write_protect = (*pADI_SDH_RESPONSE_3 & (0x00002000>>1))>>12;
            pDevice->CardInfo.CardRegisters.sd.csd.tmp_write_protect = (*pADI_SDH_RESPONSE_3 & (0x00001000>>1))>>11;
            pDevice->CardInfo.CardRegisters.sd.csd.file_format = (ADI_SD_CSD_FILE_FORMAT)(*pADI_SDH_RESPONSE_3 & (0x00000C00>>1))>>9;
            pDevice->CardInfo.CardRegisters.sd.csd.crc = (*pADI_SDH_RESPONSE_3 & (0x000000FE>>1))>>0;

            pDevice->CardInfo.CardCmdClasses = (u16)pDevice->CardInfo.CardRegisters.sd.csd.ccc;

            if(pDevice->CardInfo.CardRegisters.sd.csd.perm_write_protect | pDevice->CardInfo.CardRegisters.sd.csd.tmp_write_protect)
            {
            	pDevice->CardInfo.WriteProtect = TRUE;
            }
            else
            {
            	pDevice->CardInfo.WriteProtect = FALSE;
            }
    	}
    }
}

__ADI_SDH_SECTION_CODE
static void adi_sdh_GetExtendedCSDRegister (
    ADI_SDH_DEF         *pDevice    /* pointer to SDH device we're working on   */
){
	    /* SDH Read/Write command to use */
    u8  nSdhRWCmd;
    /* SDH Data control register value */
    u16 nSdhDataCtrl;
    /* Access retry count to wait for card data ready */
    u32 AccessRetryCount = ADI_SDH_SD_MMC_ACCESS_RETRY_COUNT;
    /* SD/MMC start address to access */
    u32 nStartAddress;
    /* Result */
    u32 Result = ADI_FSS_RESULT_SUCCESS;

    //u8 tempBuffer[512];

    ADI_DEV_1D_BUFFER		ReadBuff;	// buffer pointer

    pDevice->CurrentLBARequest.ReadFlag = TRUE;


	/* SDH Data control register - data transfer direction */
    nSdhDataCtrl = ADI_SDH_READ_DATA_FROM_SD_MMC;

    /* Read Command to SD/MMC */
    nSdhRWCmd = ADI_MMC_CMD_SEND_EXT_CSD;

    /* set data length to transfer */
    ADI_SDH_DATA_LENGTH_SET(512U);




    if (Result == ADI_FSS_RESULT_SUCCESS)
    {

        ReadBuff.Data = (void *)tempBuffer;
        //ReadBuff.Data = (void *)Andy_Tester;
	    ReadBuff.ElementCount = 512>>2;
	    ReadBuff.ElementWidth = 4;
	    ReadBuff.CallbackParameter = NULL;
	    ReadBuff.ProcessedFlag = false;
	    ReadBuff.pNext = NULL;

        Result = adi_pdd_Read(pDevice, ADI_DEV_1D, (ADI_DEV_BUFFER *)&ReadBuff);

        /* set the bus state to data transfer in progress mode */
        //pDevice->BusState = ADI_SDH_BUS_XFER_IN_PROGRESS;
        /* Set Time out for Data transfer */
        ADI_SDH_DATA_TIMER_SET(0xFFFFFFFF);
        /* Configure SDH Data control register - Enable Dataflow */
        ADI_SDH_DATA_CTRL_SET_VALUE(nSdhDataCtrl);

        if(adi_sdh_SendCommand (    pDevice,
                                    nSdhRWCmd | ADI_SDH_SD_MMC_CMD_SHORT_RESPONSE,
                                    0x00000000,
                                    ADI_SDH_STATUS_CMD_RESPONSE_TIMEOUT_FIELD | ADI_SDH_STATUS_CMD_CRC_FAIL_FIELD | ADI_SDH_STATUS_START_BIT_ERROR_FIELD,
                                    //ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD | ADI_SDH_STATUS_DATA_BLOCK_END_FIELD | ADI_SDH_STATUS_DATA_END_FIELD,
                                    ADI_SDH_STATUS_DATA_BLOCK_END_FIELD,
                                    ADI_SDH_TIMEOUT_NCR
                               ))
        {
            /* Timeout or CRC error - update SD Slot status */
            adi_sdh_UpdateSlotStatus(pDevice);
            /* return failure */
            Result = ADI_FSS_RESULT_FAILED;
        }

        /* clear all SDH status bits */
    	*pADI_SDH_STATUS_CLEAR = ADI_SDH_STATUS_CMD_RESPONSE_END_FIELD | ADI_SDH_STATUS_DATA_BLOCK_END_FIELD | ADI_SDH_STATUS_DATA_END_FIELD;

        pDevice->CardInfo.CardRegisters.mmc.extcsd.s_cmd_set = tempBuffer[504];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.boot_info = tempBuffer[228];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.boot_size_mult = tempBuffer[226];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.acc_size = tempBuffer[225];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.hc_erase_grp_size = tempBuffer[224];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.erase_timeout_mult = tempBuffer[223];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.rel_wr_sec_c = tempBuffer[222];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.hc_wp_grp_size = tempBuffer[221];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.s_c_vcc = tempBuffer[220];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.s_c_vccq = tempBuffer[219];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.s_a_timeout = tempBuffer[217];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.sec_count = tempBuffer[215]<<24 | tempBuffer[214]<<16 | tempBuffer[213]<<8 | tempBuffer[212];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.min_perf_w_8_52 = tempBuffer[210];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.min_perf_r_8_52 = tempBuffer[209];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.min_perf_w_8_26_4_52 = tempBuffer[208];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.min_perf_r_8_26_4_52 = tempBuffer[207];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.min_perf_w_4_26 = tempBuffer[206];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.min_perf_r_4_26 = tempBuffer[205];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.pwr_cl_26_360 = tempBuffer[203];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.pwr_cl_52_360 = tempBuffer[202];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.pwr_cl_26_195 = tempBuffer[201];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.pwr_cl_52_195 = tempBuffer[200];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.card_type = tempBuffer[196];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.csd_structure = tempBuffer[194];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.ext_csd_rev = tempBuffer[192];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.cmd_set = tempBuffer[191];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.cmd_set_rev = tempBuffer[189];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.power_class = tempBuffer[187];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.hs_timing = tempBuffer[185];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.bus_width = tempBuffer[183];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.erased_mem_cont = tempBuffer[181];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.boot_config = tempBuffer[179];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.boot_bus_width = tempBuffer[177];
        pDevice->CardInfo.CardRegisters.mmc.extcsd.erase_group_def = tempBuffer[175];

    }
}


/*****/


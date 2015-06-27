/**********************************************************************

Copyright(c) Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_nfc.c,v $
$Revision: 3939 $
$Date: 2010-12-10 09:07:11 -0500 (Fri, 10 Dec 2010) $

Description:
    This is the primary source code for ADSP-BF52x and
    ADSP-BF54x NAND Flash Controller driver

***********************************************************************/

/*********************************************************************

Include files

*********************************************************************/

#include <services/services.h>      /* system service includes                          */
#include <drivers/adi_dev.h>        /* device manager includes                          */
#include <drivers/nfc/adi_nfc.h>    /* NFC driver includes                              */
#include "adi_nfc_reg.h"            /* NFC MMR definititons and bitmask access macros   */

/*********************************************************************

Enumerations and defines

*********************************************************************/

#if !defined(__ADSP_MOAB__)         /*NFC is available only on BF52x & BF54x family*/
#if !defined(__ADSP_KOOKABURRA__)
#if !defined(__ADSP_MOCKINGBIRD__)
#error "NAND Flash Controller is available only on ADSP-BF52x and ADSP-BF54x family processors"
#endif
#endif
#endif

/* number of NFC devices in the system  */
#define ADI_NFC_NUM_DEVICES 	        (sizeof(NFCDevice)/sizeof(ADI_NFC_DEF))
/* NFD Data Memory size in bytes for 64MB */
#define ADI_NFD_SIZE_64MB               67108864
/* NFD Data Memory size in bytes for 128MB */
#define ADI_NFD_SIZE_128MB              134217728
/* Read from NFD */
#define ADI_NFC_READ_NFD                1
/* Write to NFD */
#define ADI_NFC_WRITE_NFD               0
/* Use NFC DMA */
#define ADI_NFC_USE_DMA                 1
/* Access NFD directly (do no use DMA) */
#define ADI_NFC_NO_DMA                  0
/* ECC Disabled */
#define ADI_NFC_ECC_DISABLED            0xFF

/*********************************************************************

Enums to identify NFC interrupt status flags to wait for

*********************************************************************/
enum
{
    /* There are no NFC IRQ status flags to wait for */
    ADI_NFC_WAIT_STATUS_NONE            = 0x00,
    /* NFC Data transfer in progress, wait for Not Busy IRQ             */
    ADI_NFC_WAIT_FOR_NBUSYIRQ           = ADI_NFC_IRQSTAT_NBUSYIRQ_MASK,
    /* NFC Data transfer in progress, wait for write buffer empty IRQ   */
    ADI_NFC_WAIT_FOR_WRITE_BUF_EMPTY    = ADI_NFC_IRQSTAT_WB_EMPTY_MASK,
    /* NFC Data transfer in progress, wait for Read Data ready IRQ      */
    ADI_NFC_WAIT_FOR_READ_DATA_READY    = ADI_NFC_IRQSTAT_RD_RDY_MASK,
    /* NFC Data transfer in progress, wait for Read Data ready IRQ      */
    ADI_NFC_WAIT_FOR_PAGE_WRITE_DONE    = ADI_NFC_IRQSTAT_PG_WR_DONE_MASK,
};

/*********************************************************************

Enums to identify NFC Data transfer state

*********************************************************************/
enum
{
    /* NFC is idle                                      */
    ADI_NFC_STATE_IDLE                      = 0,
    /* NFC is busy transfering data to NFD using DMA    */
    ADI_NFC_STATE_DATA_XFER_IN_PROGRESS     = 1,
    /* NFC is busy Accessing NFD without using DMA      */
    ADI_NFC_STATE_DIRECT_ACCESS_IN_PROGRESS = 2,
    /* NFC Device interrupt - Data transfer complete    */
    ADI_NFC_STATE_DEV_DATA_XFER_DONE        = 4,
    /* NFC DMA interrupt - Data transfer complete       */
    ADI_NFC_STATE_DMA_DATA_XFER_DONE        = 8,
    /* NFC Direct Access complete                       */
    ADI_NFC_STATE_DIRECT_ACCESS_DONE        = ( ADI_NFC_STATE_DIRECT_ACCESS_IN_PROGRESS |\
                                                ADI_NFC_STATE_DEV_DATA_XFER_DONE),
    /* NFC Data transfer complete                       */
    ADI_NFC_STATE_DATA_XFER_DONE            = ( ADI_NFC_STATE_DATA_XFER_IN_PROGRESS |\
                                                ADI_NFC_STATE_DEV_DATA_XFER_DONE    |\
                                                ADI_NFC_STATE_DMA_DATA_XFER_DONE),
    /* NFC DMA error */
    ADI_NFC_STATE_DMA_INT_XFER_FAILED       = 0x10,
};

/*********************************************************************

Enums to identify NFD Address to insert

*********************************************************************/
typedef enum ADI_NFD_ADDR_TYPE
{
    /* Address type to access a Page (Inserts complete address)*/
    ADI_NFD_ADDR_TYPE_ACCESS_PAGE,
    /* Address type to access a Page data in random (Inserts column address only)*/
    ADI_NFD_ADDR_TYPE_ACCESS_RANDOM,
    /* Address type to erase a block (Inserts block address only)*/
    ADI_NFD_ADDR_TYPE_ERASE_BLOCK
}ADI_NFD_ADDR_TYPE;

/*********************************************************************

Data structure to hold NFC access parameters to access selected NFD

*********************************************************************/

typedef struct ADI_NFC_ACCESS_PARAMS
{
    u8                          InUseFlag;              /* 1 = NFC is already in use,
                                                           0 = NFC is available for use                             */
    u8                          ExclusiveAccessFlag;    /* 1 = NFC/NFD access request already in progress,
                                                           0 = NFC/NFD access is available for use                  */
    u8                          ReadFlag;               /* 1 = Read from NFD, 0 = Write to NFD                      */
    u8                          NumDescriptors;         /* Number of descriptors required to access a NFD page      */
    u8                          RemainDescriptors;      /* # descriptors to process                                 */
    u8                          NumAddressCycles;       /* Number of address cycles required to access NFD          */
    u8                          NFCXferStatus;          /* NFC Data Transfer status                                 */
    u8                          SpareUnitsPerPkt;       /* # spare units per NFD Data packet                        */
    u16                         NFDDataPktSize;         /* NFD Data packet size (256 for x8, 128 for x16)           */
    u16                         BlockAddress;           /* NFD Block address to access                              */
    u16                         PageAddress;            /* NFD Page address to access                               */
    u16                         ColumnAddress;          /* NFD Column address to access                             */
    u16                         NFCWaitStatus;          /* NFC status flags to wait for                             */
    u16                         ECCStart;               /* Nth location of NFD spare area to hold ECC start         */
    ADI_NFC_ECC_MODE            eECCMode;               /* ECC Mode                                                 */
    u8                          *pAlignBuf256Bytes;     /* 256 bytes array used for NFD Data Packet alignment       */
    u16                         *pECCArrayStart;        /* Start of ECC Storage array                               */
    u16                         *pECCArray;             /* ECC array location to hold next ECC                      */
    ADI_DMA_DESCRIPTOR_LARGE    *pStartDescriptor;      /* Pointer to first descriptor in NFC DMA descriptor chain  */
    ADI_DMA_DESCRIPTOR_LARGE    *pProcessDescriptor;    /* Pointer to NFC DMA descriptor being processed            */
}ADI_NFC_ACCESS_PARAMS;

/*********************************************************************

Device Driver instance structure

*********************************************************************/

#pragma pack(4)

typedef struct ADI_NFC_DEF
{                                                       /* NFC device structure                                     */
    ADI_NFC_ACCESS_PARAMS       NFCAccess;              /* Structure to hold NFC access parameters                  */
    ADI_DEV_DEVICE_HANDLE       DeviceHandle;           /* Device instance Handle provided by Device Manager        */
    ADI_DCB_CALLBACK_FN         DMCallback;             /* callback function supplied by the Device Manager         */
    ADI_DMA_MANAGER_HANDLE      DMAHandle;              /* Handle to the DMA manager                                */
    ADI_DCB_HANDLE              DCBHandle;              /* Deffered Callback handle                                 */
    void                        *pEnterCriticalArg;     /* critical region argument                                 */
    ADI_DEV_DIRECTION           Direction;              /* Dataflow Direction of this NFC Device                    */
    ADI_DEV_DMA_INFO            NFCDMAInfo;             /* NFC DMA channel information table(NFC is half-duplex)    */
    ADI_INT_PERIPHERAL_ID       NFCStatusIntID;         /* NFC Status Interrupt peripheral ID                       */
    ADI_SEM_HANDLE              DataSemaphoreHandle;    /* Semaphore for Internal data transfers                    */
    ADI_NFD_INFO_TABLE          *pNFDInfo;              /* Pointer to structure holding NFD related Information     */
    ADI_DEV_1D_BUFFER           *pNFCBuffer;            /* Pointer to 1D buffer we're working on                    */
} ADI_NFC_DEF;

#pragma pack()

/*********************************************************************

Device specific data

*********************************************************************/

/*********************
	Moab
**********************/

#if defined(__ADSP_MOAB__)                      /* settings for Moab class devices  */

/* Create NFC device (driver) instance */

ADI_NFC_DEF   NFCDevice[] =
{
    {   /* NFC Device 0 */
        {
            FALSE,                              /* NFC is available for use                 */
            FALSE,                              /* NFC/NFD access is available for use      */
            1,                                  /* Read NFD                                 */
            0,                                  /* Num Descriptors - Unknown at this point  */
            0,                                  /* # descriptors to process                 */
            0,                                  /* Address cycles                           */
            ADI_NFC_STATE_IDLE,                 /* NFC in idle mode by default              */
            0,                                  /*Spare units per NFD Data packet is unknown*/
            ADI_NFD_DATA_PKT_SIZE_IN_BYTES,     /* Assume this is a x8 type NFD             */
            0,                                  /* Block Address                            */
            0,                                  /* Page Address                             */
            0,                                  /* Column Address                           */
            ADI_NFC_WAIT_STATUS_NONE,           /* No status flags to wait for              */
            0,                                  /* NFD spare area ECC start                 */
            ADI_NFC_ECC_MODE_DISABLE,           /* Disable ECC                              */
            NULL,                               /* Reset 256 bytes array pointer            */
            NULL,                               /* Reset ECC array pointers                 */
            NULL,                               /* Reset ECC array pointers                 */
            NULL,                               /* Reset pointer to 1st NFC DMA descriptor  */
            NULL,                               /* Reset pointer to Process descriptor      */
        },
        NULL,                                   /* NFC Device instance Handle               */
        NULL,                                   /* Device Manager Callback Function         */
        NULL,                                   /* DMA Manager Handle                       */
        NULL,                                   /* DCB Manager Handle                       */
        NULL,                                   /* Critical region argument                 */
        ADI_DEV_DIRECTION_UNDEFINED,            /* Direction Undefined                      */
        {                                       /* NFC DMA Information table                */
            ADI_DMA_PMAP_SDH_NFC,       	    /* NFC DMA peripheral mapping ID            */
            NULL,                               /* DMA channel handle                       */
            FALSE,                              /* switch mode disabled by default          */
            NULL,                               /* pointer to switch queue head             */
            NULL,                               /* pointer to switch queue tail             */
            NULL,                               /* No further DMA channels to link          */
        },
        ADI_INT_NFC_ERROR,                      /* NFC Error Interrupt peripheral ID        */
        NULL,                                   /* Data Semaphore Handle                    */
        NULL,                                   /* pointer to NFD information table         */
        NULL,                                   /* clear data buffer pointer                */
    }
};
#endif

/* settings for Kokkaburra and Mokingbird class devices    */
#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)

/* Create NFC device (driver) instance */

/*********************************************************************************
    Note:   Kookaburra has only one physical NAND Flash Controller (NFC) shared
            between two I/O port mappings (one on PORT H and one on PORT F).
            We handle the second instance of NFC (on PORT F) as a second, virtual
            NFC device.  Control register access for this virtual device will
            conflict if users attempt to open both NFC devices.
*********************************************************************************/

ADI_NFC_DEF   NFCDevice[] =
{
    {   /* Device 0 - uses NFC on PORT H, shared with MAC Tx & HOST                         */
        {
            FALSE,                              /* NFC is available for use                 */
            FALSE,                              /* NFC/NFD access is available for use      */
            1,                                  /* Read NFD                                 */
            0,                                  /* Num Descriptors - Unknown at this point  */
            0,                                  /* # descriptors to process                 */
            0,                                  /* Address cycles                           */
            ADI_NFC_STATE_IDLE,                 /* NFC in idle mode by default              */
            0,                                  /*Spare units per NFD Data packet is unknown*/
            ADI_NFD_DATA_PKT_SIZE_IN_BYTES,     /* Assume this is a x8 type NFD             */
            0,                                  /* Block Address                            */
            0,                                  /* Page Address                             */
            0,                                  /* Column Address                           */
            ADI_NFC_WAIT_STATUS_NONE,           /* No status flags to wait for              */
            0,                                  /* NFD spare area ECC start                 */
            ADI_NFC_ECC_MODE_DISABLE,           /* Disable ECC                              */
            NULL,                               /* Reset 256 bytes array pointer            */
            NULL,                               /* Reset ECC array pointers                 */
            NULL,                               /* Reset ECC array pointers                 */
            NULL,                               /* Reset pointer to 1st NFC DMA descriptor  */
            NULL,                               /* Reset pointer to Process descriptor      */
        },
        NULL,                                   /* NFC Device instance Handle               */
        NULL,                                   /* Device Manager Callback Function         */
        NULL,                                   /* DMA Manager Handle                       */
        NULL,                                   /* DCB Manager Handle                       */
        NULL,                                   /* Critical region argument                 */
        ADI_DEV_DIRECTION_UNDEFINED,            /* Direction Undefined                      */
        {                                       /* NFC DMA Information table                */
            ADI_DMA_PMAP_MAC_TX,       	        /* NFC DMA peripheral mapping ID            */
            NULL,                               /* DMA channel handle                       */
            FALSE,                              /* switch mode disabled by default          */
            NULL,                               /* pointer to switch queue head             */
            NULL,                               /* pointer to switch queue tail             */
            NULL,                               /* No further DMA channels to link          */
        },
        ADI_INT_NAND_ERROR,                     /* NFC Error Interrupt peripheral ID        */
        NULL,                                   /* Data Semaphore Handle                    */
        NULL,                                   /* pointer to NFD information table         */
        NULL,                                   /* clear data buffer pointer                */
    },
    {   /* Device 1 - uses NFC on PORT F, shared with PPI & SPORT 0 on PORT F               */
        {
            FALSE,                              /* NFC is available for use                 */
            FALSE,                              /* NFC/NFD access is available for use      */
            1,                                  /* Read NFD                                 */
            0,                                  /* Num Descriptors - Unknown at this point  */
            0,                                  /* # descriptors to process                 */
            0,                                  /* Address cycles                           */
            ADI_NFC_STATE_IDLE,                 /* NFC in idle mode by default              */
            0,                                  /*Spare units per NFD Data packet is unknown*/
            ADI_NFD_DATA_PKT_SIZE_IN_BYTES,     /* Assume this is a x8 type NFD             */
            0,                                  /* Block Address                            */
            0,                                  /* Page Address                             */
            0,                                  /* Column Address                           */
            ADI_NFC_WAIT_STATUS_NONE,           /* No status flags to wait for              */
            0,                                  /* NFD spare area ECC start                 */
            ADI_NFC_ECC_MODE_DISABLE,           /* Disable ECC                              */
            NULL,                               /* Reset 256 bytes array pointer            */
            NULL,                               /* Reset ECC array pointers                 */
            NULL,                               /* Reset ECC array pointers                 */
            NULL,                               /* Reset pointer to 1st NFC DMA descriptor  */
            NULL,                               /* Reset pointer to Process descriptor      */
        },
        NULL,                                   /* NFC Device instance Handle               */
        NULL,                                   /* Device Manager Callback Function         */
        NULL,                                   /* DMA Manager Handle                       */
        NULL,                                   /* DCB Manager Handle                       */
        NULL,                                   /* Critical region argument                 */
        ADI_DEV_DIRECTION_UNDEFINED,            /* Direction Undefined                      */
        {                                       /* NFC DMA Information table                */
            ADI_DMA_PMAP_PPI,       	        /* NFC DMA peripheral mapping ID            */
            NULL,                               /* DMA channel handle                       */
            FALSE,                              /* switch mode disabled by default          */
            NULL,                               /* pointer to switch queue head             */
            NULL,                               /* pointer to switch queue tail             */
            NULL,                               /* No further DMA channels to link          */
        },
        ADI_INT_NAND_ERROR,                     /* NFC Error Interrupt peripheral ID        */
        NULL,                                   /* Data Semaphore Handle                    */
        NULL,                                   /* pointer to NFD information table         */
        NULL,                                   /* clear data buffer pointer                */
    },
};

#endif

/*********************************************************************

External Function prototypes

*********************************************************************/
extern void _adi_fss_FlushMemory( char *StartAddress, u32 NumBytes, ADI_DMA_WNR Direction);

/*********************************************************************

Local Function prototypes

*********************************************************************/

/* Activates NFC device and configures it for use */
static u32 adi_nfc_Activate(
    ADI_NFC_DEF                 *pDevice,       /* pointer to NFC device we're working on   */
    u32                         EnableFlag      /* Enable/Disable Flag                      */
);

/* Initialises NFC registers */
static u32 adi_nfc_InitRegisters (
    ADI_NFC_DEF                 *pDevice        /* pointer to NFC device we're working on   */
);

/* Reset NFC registers and access parameters */
static void adi_nfc_ResetNFCAccess (
    ADI_NFC_DEF                 *pDevice        /* pointer to NFC device we're working on   */
);

/* Updates NFD Address field in NFC access structure */
static void adi_nfc_UpdateNFDAddress (
    ADI_NFC_DEF                 *pDevice,       /* pointer to NFC device we're working on   */
    u32                         UpdateCount,    /* #bytes or #sectors to update             */
    u32                         MaxColumnAddress    /* Maximum Column Address Value         */
);

/* Translates Block/Column/Page address to suitable format in order to access
   NAND flash Device Memory and inserts the calculated address to NFC address register */
static u32 adi_nfc_InsertAddress (
    ADI_NFC_DEF                 *pDevice,       /* pointer to NFC device we're working on   */
    ADI_NFD_ADDR_TYPE           AddressType     /* Address type to insert                   */
);

/*
   Reads #bytes/#words from NFD starting from the given Block/Page/Column address
   Writes #bytes/#words to NFD starting from the given Block/Page/Column address
*/
static u32 adi_nfc_DirectAccessNFD (
    ADI_NFC_DEF                 *pDevice,       /* pointer to NFC device we're working on   */
    ADI_NFD_DIRECT_ACCESS       *pAccess,       /* pointer to NFD direct access structure   */
    u8                          ReadFlag        /* 1 = Read from NFD, 0 = Write to NFD      */
);

/* Reads 1 byte or 1 word from NFD */
static u32 adi_nfc_ReadSingleUnit(
    ADI_NFC_DEF                 *pDevice,       /* pointer to NFC device we're working on   */
    void                        *pData          /* Location to store the data read from NFD */
);

/* Writes 1 byte or 1 word to NFD */
static u32 adi_nfc_WriteSingleUnit(
    ADI_NFC_DEF                 *pDevice,       /* pointer to NFC device we're working on   */
    void                        *pData          /* pointer to location holding
                                                   Data to be sent to NFD                   */
);

/* Prepares a single or list of descriptors */
static u32 adi_nfc_PrepareDescriptors(
    ADI_NFC_DEF                 *pDevice,       /* pointer to NFC device we're working on   */
    ADI_DEV_BUFFER_TYPE         BufferType,     /* type of Buffer                           */
    ADI_DEV_BUFFER              *pBuffer        /* pointer to the start of buffer chain     */
);

/* Configures NFC DMA Descriptor fields */
static void adi_nfc_ConfigureDescriptors(
    ADI_NFC_DEF                 *pDevice,       /* pointer to NFC device we're working on   */
    u32                         DescriptorCount /* # descriptors to configure               */
);

/* queues Descriptors to NFC DMA */
static u32 adi_nfc_SubmitDescriptors(
    ADI_NFC_DEF                 *pDevice       /* pointer to NFC device we're working on   */
);

/* Accesses NFD by aligning NFD data packet to 256 bytes boundary */
static u32 adi_nfc_AlignNFDPkt(
    ADI_NFC_DEF                 *pDevice,     /* pointer to NFC device we're working on     */
    u32                         MaxColumnAddress    /* Maximum Column Address range         */
);

/* Kicks off NFD Page Access */
static u32 adi_nfc_KickOffPageAccess(           /* Kicks off NFD Page Access                */
    ADI_NFC_DEF                 *pDevice,       /* pointer to NFC device we're working on   */
    u8                          DMAFlag,        /* 1 = Use NFC DMA, 0 = Do not use DMA      */
    u8                          ReadFlag        /* 1 = Read from NFD, 0 = Write to NFD      */
);

/* Processes Hardware generated ECC */
static u32 adi_nfc_ProcessECC(
    ADI_NFC_DEF                 *pDevice,       /* pointer to NFC device we're working on   */
    u32                         NumPktsAccessed /* # NFD Data packets accessed in this page */
);

/* Waits until the given NFC IRQ status bits gets set */
static u32 adi_nfc_WaitOnStatus (
    ADI_NFC_DEF                 *pDevice,       /* pointer to NFC device we're working on   */
    u16                         StatusFlag      /* NFC Status bits to look for              */
);

/* Validates the given NFD Block/Page/Column address */
static u32 adi_nfc_ValidateNFDAddress(
    ADI_NFC_DEF                 *pDevice        /* pointer to NFC device we're working on   */
);

/**************************************
Functions to handle NFC interrupts
**************************************/
/* NFC Status interrupt handler */
static ADI_INT_HANDLER(adi_nfc_InterruptHandler);

/* NFC DMA Callback function */
static void adi_nfc_DMACallback(
    void                        *DeviceHandle,  /* Device handle that generated callback    */
    u32                         CallbackEvent,  /* DMA Callback event                       */
    void                        *pArg           /* Callback argument                        */
);

/* Processes NFC Device and DMA interrupts */
static void adi_nfc_ProcessInterrupt (
    ADI_NFC_DEF                 *pDevice        /* pointer to NFC device we're working on   */
);

/*********************************************************************

Processor specific functions

*********************************************************************/

/* Static functions for Moab class devices */
#if defined(__ADSP_MOAB__)

/* configures port control registers for NFC */
static u32 adi_nfc_SetPortControl(
    ADI_NFC_DEF                 *pDevice        /* pointer to NFC device we're working on   */
);

#endif

/* Static functions for Kookaburra and Mokingbird class devices */
#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)

/* configures port control registers for NFC */
static u32 adi_nfc_SetPortControl(
    ADI_NFC_DEF                 *pDevice        /* pointer to NFC device we're working on   */
);

#endif

/*********************************************************************

NFC Driver - Entry point functions

*********************************************************************/

static u32 adi_pdd_Open(                        /* Opens a device                                   */
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

static u32 adi_pdd_Close(                       /* Closes a device                                  */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of the device to close                */
);

static u32 adi_pdd_Read(                        /* Reads data/queues an inbound buffer to a device  */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                                       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                                      */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                                */
);

static u32 adi_pdd_Write(                       /* Writes data/queues outbound buffer to a device   */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                                       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                                      */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                                */
);

static u32 adi_pdd_SequentialIO(                /* Sequentially read/writes data to a device        */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a EPPI device                      */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                                      */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                                */
);

static u32 adi_pdd_Control(                     /* Sets or senses device specific parameter         */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                                       */
    u32                     Command,            /* Command ID                                       */
    void                    *Value              /* Command specific value                           */
);

/*********************************************************************

Entry point for device manager

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT ADI_NFC_EntryPoint =
{
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control,
    adi_pdd_SequentialIO
};

/*********************************************************************

    Function:       adi_pdd_Open

    Description:    Opens NFC Physical Interface Device driver for use

*********************************************************************/
static u32 adi_pdd_Open(                        /* Open a device                    */
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
    /* return code */
    u32 Result= ADI_DEV_RESULT_SUCCESS;
    /* pointer to NFC device instance */
    ADI_NFC_DEF *pDevice;
    /* To hold NFC IVG */
    u32 NFCIntIVG;
    /* exit critical region parameter */
    void    *pExitCriticalArg;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* check the device number */
    if (DeviceNumber >= ADI_NFC_NUM_DEVICES)
    {
        /* Invalid Device number */
        Result = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    }

    /*  Continue only if the device number is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* insure the device the client wants is available  */
        Result  = ADI_DEV_RESULT_DEVICE_IN_USE;
        /* NFC device we're working on */
        pDevice = &NFCDevice[DeviceNumber];

        /* Protect this section of code - entering a critical region    */
        pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
        /* Check the device usage status*/
        if (pDevice->NFCAccess.InUseFlag == FALSE)
        {
            /* Device is not in use. Reserve the device for this client */
            pDevice->NFCAccess.InUseFlag = TRUE;
            Result = ADI_DEV_RESULT_SUCCESS;
        }
        /* Exit the critical region */
        adi_int_ExitCriticalRegion(pExitCriticalArg);

        /* Continue only when the NFC device is available for use  */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            /*** initialise the NFC Device driver structure ***/
            /* Save the Device handle */
            pDevice->DeviceHandle       = DeviceHandle;
            /* Save the DMA handle */
            pDevice->DMAHandle         	= DMAHandle;
            /* Save the DCB handle */
            pDevice->DCBHandle          = DCBHandle;
            /* Save the Device Manager callback function handle */
            pDevice->DMCallback         = DMCallback;
            /* Pointer to critical region                   */
            pDevice->pEnterCriticalArg  = pEnterCriticalArg;
            /* Device Dataflow Direction */
            pDevice->Direction          = Direction;

            /*** Populate NFC DMA channel information table ***/
            /*
               NFC is a half-duplex device and is mapped to a single DMA channel
               To have a better control over DMA dataflow, the NFC DMA and buffers
               are handled by the driver itself rather than the Device manager
            */

#if defined(__ADSP_MOAB__)  /* for Moab class devices */

            /* DMA Mapping ID - DMA channel shared btwn NFC and SDH */
            pDevice->NFCDMAInfo.MappingID       = ADI_DMA_PMAP_SDH_NFC;
            /* No DMA channel handle to start with */
            pDevice->NFCDMAInfo.ChannelHandle   = NULL;
            /* NFC Status Interrupt peripheral ID */
            pDevice->NFCStatusIntID             = ADI_INT_NFC_ERROR;

/* for Kookaburra and Mokingbird class devices */
#elif defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)

            /* if (use NFC Device 0 - NFC on PORT H, shared with MAC Tx & HOST)  */
            if (DeviceNumber == 0)
            {
                /* DMA Mapping ID - DMA channel shared btwn NFC and MAC/HOST */
                pDevice->NFCDMAInfo.MappingID   = ADI_DMA_PMAP_MAC_TX;
            }
            /* if (use NFC Device 1 - NFC on PORT F, shared with PPI & SPORT 0 on PORT F)  */
            else
            {
                /* DMA Mapping ID - DMA channel shared btwn NFC and PPI/SPORT0 */
                pDevice->NFCDMAInfo.MappingID   = ADI_DMA_PMAP_PPI;
            }

            /* No DMA channel handle to start with */
            pDevice->NFCDMAInfo.ChannelHandle   = NULL;
            /* NFC Status Interrupt peripheral ID */
            pDevice->NFCStatusIntID = ADI_INT_NAND_ERROR;

#endif
            /*** Reset NFD access parameters ***/
            pDevice->NFCAccess.ExclusiveAccessFlag  = FALSE;
            pDevice->NFCAccess.ECCStart             = 0;
            pDevice->NFCAccess.eECCMode             = ADI_NFC_ECC_MODE_DISABLE;
            pDevice->NFCAccess.NumDescriptors       = 0;
            pDevice->NFCAccess.NumAddressCycles     = 0;
            pDevice->NFCAccess.pAlignBuf256Bytes    = NULL;
            pDevice->NFCAccess.pECCArrayStart       = NULL;
            pDevice->NFCAccess.pECCArray            = NULL;
            pDevice->NFCAccess.pStartDescriptor     = NULL;

            /*** Reset NFD information table pointer ***/
            pDevice->pNFDInfo   = NULL;

            /*** Reset NFC buffer pointer ***/
            pDevice->pNFCBuffer = NULL;

            /* Mask all NFC IRQ status interrupts */
            ADI_NFC_IRQMASK_DISABLE_ALL;

            /*** Hook NFC Status Interrupt Handler ***/
            /* Disable NFC Status interrupt to start with */
            adi_int_SICDisable(pDevice->NFCStatusIntID);
            /* Get NFC Mask interrupt line IVG */
            adi_int_SICGetIVG(pDevice->NFCStatusIntID, &NFCIntIVG);
            /* hook the interrupt handler into the system, enable interrupt nesting */
            if(adi_int_CECHook(NFCIntIVG, adi_nfc_InterruptHandler, pDevice, TRUE) != ADI_INT_RESULT_SUCCESS)
            {
                /* make this NFC device available */
                pDevice->NFCAccess.InUseFlag = FALSE;
                Result = ADI_DEV_RESULT_CANT_HOOK_INTERRUPT;
            }
            /* Successfully hooked NFC interrupt handler */
            else
            {
                /*** Create semaphore ***/
                if ((Result = (u32)adi_sem_Create(1,&pDevice->DataSemaphoreHandle, NULL)) == (u32)ADI_SEM_RESULT_SUCCESS)
                {
                    /* Enable NFC Status interrupts */
                    if ((Result = adi_int_SICEnable(pDevice->NFCStatusIntID)) == ADI_INT_RESULT_SUCCESS)
                    {
                        /* save the physical device handle to client supplied location */
                        *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;
                    }
                    else
                    {
                        /* un-hook NFC interrupt */
                        adi_int_CECUnhook(NFCIntIVG, adi_nfc_InterruptHandler, pDevice);
                        /* free this NFC device */
                        pDevice->NFCAccess.InUseFlag = FALSE;
                    }
                }
            }
        }
/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_pdd_Close

    Description:    Closes down the NFC driver

*********************************************************************/
static u32 adi_pdd_Close(
    ADI_DEV_PDD_HANDLE  PDDHandle       /* PDD handle   */
)
{
    /* Device Instance */
    ADI_NFC_DEF *pDevice = (ADI_NFC_DEF *)PDDHandle;
    /* return code */
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    /* to store NFC Status interrupt IVG */
    u32 NFCIntIVG;

    /* Delete NFC driver semaphores */
    adi_sem_Delete(pDevice->DataSemaphoreHandle);

    /* De-activate NFC */
    if ((Result = adi_nfc_Activate(pDevice,FALSE)) == ADI_DEV_RESULT_SUCCESS)
    {
        /* unhook the NFC interrupt handler from the system */
        adi_int_SICGetIVG(pDevice->NFCStatusIntID, &NFCIntIVG);
        if (adi_int_CECUnhook(NFCIntIVG, adi_nfc_InterruptHandler, pDevice) != ADI_INT_RESULT_SUCCESS)
        {
            Result = ADI_DEV_RESULT_CANT_UNHOOK_INTERRUPT;
        }
        else
        {
            /* IF (NFC DMA channel is open) */
            if (pDevice->NFCDMAInfo.ChannelHandle)
            {
                /* Close NFC DMA channel */
                Result = adi_dma_Close(pDevice->NFCDMAInfo.ChannelHandle,FALSE);
            }
        }
    }

    if (Result == 0)
    {
        /* free this NFC device */
        pDevice->NFCAccess.InUseFlag        = FALSE;
        pDevice->NFCDMAInfo.ChannelHandle   = NULL;
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_pdd_Read

    Description:    Reads from NFD using NFC DMA

*********************************************************************/
static u32 adi_pdd_Read(                        /* Reads data/queues an inbound buffer to a device  */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                                       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                                      */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                                */
){
    /* Device Instance */
    ADI_NFC_DEF *pDevice = (ADI_NFC_DEF *)PDDHandle;
    /* assume we're going to be successful */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* IF (NFC/NFD access is in progress) */
    if (pDevice->NFCAccess.ExclusiveAccessFlag == TRUE)
    {
        Result = ADI_NFC_RESULT_NFD_ACCESS_IN_PROGRESS;
    }
    /* ELSE IF (NFC DMA Channel is not open yet) */
    else if (pDevice->NFCDMAInfo.ChannelHandle == NULL)
    {
        Result = ADI_DEV_RESULT_DMA_CHANNEL_UNAVAILABLE;
    }
    /* ELSE (process the given buffer chain) */
    else
    {
        /* Read from NFD */
        pDevice->NFCAccess.ReadFlag = 1;
        /* Acquire exclusive access of NFC and NFD */
        pDevice->NFCAccess.ExclusiveAccessFlag = TRUE;
        /* prepare and submit list of buffers to NFC DMA */
        Result = adi_nfc_PrepareDescriptors(pDevice,BufferType,pBuffer);
        /* Release Exclusive access flag */
        pDevice->NFCAccess.ExclusiveAccessFlag = FALSE;
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_pdd_Write

    Description:    Writes to NFD using NFC DMA

*********************************************************************/
static u32 adi_pdd_Write(                       /* Writes data/queues outbound buffer to a device   */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                                       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                                      */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                                */
){
    /* Device Instance */
    ADI_NFC_DEF *pDevice = (ADI_NFC_DEF *)PDDHandle;
    /* assume we're going to be successful */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* IF (NFC/NFD access is in progress) */
    if (pDevice->NFCAccess.ExclusiveAccessFlag == TRUE)
    {
        Result = ADI_NFC_RESULT_NFD_ACCESS_IN_PROGRESS;
    }
    /* ELSE IF (NFC DMA Channel is not open yet) */
    else if (pDevice->NFCDMAInfo.ChannelHandle == NULL)
    {
        Result = ADI_DEV_RESULT_DMA_CHANNEL_UNAVAILABLE;
    }
    /* ELSE (process the given buffer chain) */
    else
    {
        /* Write to NFD */
        pDevice->NFCAccess.ReadFlag = 0;
        /* Acquire exclusive access of NFC and NFD */
        pDevice->NFCAccess.ExclusiveAccessFlag = TRUE;
        /* prepare and submit list of buffers to NFC DMA */
        Result = adi_nfc_PrepareDescriptors(pDevice,BufferType,pBuffer);
        /* Release Exclusive access flag */
        pDevice->NFCAccess.ExclusiveAccessFlag = FALSE;
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_pdd_SequentialIO

    Description:    Function not supported

*********************************************************************/
static u32 adi_pdd_SequentialIO(                /* Sequentially read/writes data to a device        */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle                                       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                                      */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                                */
){
    return (ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************

    Function:       adi_nfc_PrepareDescriptors

    Description:    Splits the application provided buffer chain
                    in to single or chain of descriptors that is
                    suitable to access a NFD packet.

*********************************************************************/
static u32 adi_nfc_PrepareDescriptors(
    ADI_NFC_DEF                 *pDevice,       /* pointer to NFC device we're working on   */
    ADI_DEV_BUFFER_TYPE         BufferType,     /* type of Buffer                           */
    ADI_DEV_BUFFER              *pBuffer        /* pointer to the start of buffer chain     */
){
    u16 ECCCalculated[4];           /* Array to hold ECC calculated by NFC hardware         */
    u32 AddressUpdateCount;         /* NFD address update count                             */
    u32 RemainElementsInPage;       /* # elements remaining in current page                 */
    u32 RemainElementsInBuffer;     /* # elements remaining in current buffer               */
    u32 RemainElementsToAccess;     /* # elements remaining to access                       */
    u32 DescriptorElementCount;     /* num elements to be processed by a Descriptor         */
    u32 ClientBufElementCount;      /* # current client buffer size                         */
    u32 MaxColumnAddress;           /* Maximum NFD column address that can be accessed + 1  */
    /* return code */
    u32 Result;
    /* value of the direction field within the DMA config register */
    ADI_DMA_WNR wnr;

    ADI_DEV_1D_BUFFER   *pCallbackBuffer = NULL;
    /*** build for large descriptors ***/
    /* pointer to the descriptor we're working on       */
    ADI_DMA_DESCRIPTOR_LARGE    *pWorkingDescriptor;
    /* Pointer to client Data */
    u8  *pClientData;

    /* assume we're going to be successful */
    Result = ADI_DEV_RESULT_SUCCESS;

    /* set the value of wnr for the config register */
    if (pDevice->NFCAccess.ReadFlag == 1)
    {
        /* Read NFD and write to SDRAM */
        wnr = ADI_DMA_WNR_WRITE;
    }
    else
    {
        /* Read SDRAM and write to NFD */
        wnr = ADI_DMA_WNR_READ;
    }

    /* CASEOF (buffer type) */
    switch (BufferType)
    {
        /* CASE (1D buffer) */
        case (ADI_DEV_1D):

            /* IF (we've no 256-bytes buffer to access Spare area and for NFD packet alignment) */
            if (pDevice->NFCAccess.pAlignBuf256Bytes == NULL)
            {
                /* return error */
                Result = ADI_NFC_RESULT_NO_ALIGNMENT_BUFFER;
                break;
            }

            /* save the address of 1D buffer we're currently working on */
            pDevice->pNFCBuffer = (ADI_DEV_1D_BUFFER*) pBuffer;
            /* Number of elements remaining in this 1D buffer */
            RemainElementsInBuffer  = (pDevice->pNFCBuffer->ElementWidth * pDevice->pNFCBuffer->ElementCount);
            /* Clear 1D buffer processed element count */
            pDevice->pNFCBuffer->ProcessedElementCount = 0;

            /* IF (application hasn't provided us with any DMA frame buffer memory) */
            if (pDevice->NFCAccess.NumDescriptors == 0)
            {
                /* use 1D buffer reserved area to build the descriptor */
                pDevice->NFCAccess.pStartDescriptor = (ADI_DMA_DESCRIPTOR_LARGE *)pDevice->pNFCBuffer->Reserved;
                /* Location to store Hardware calculated ECC */
                pDevice->NFCAccess.pECCArrayStart = &ECCCalculated[0];
                /* Configure descriptor fields */
                adi_nfc_ConfigureDescriptors(pDevice,1);
            }

            /* IF (Error Correction enabled) */
            if (pDevice->NFCAccess.eECCMode < ADI_NFC_ECC_MODE_DISABLE)
            {
                /* DMA will be used only to access Page Data Area.
                   Spare area will be accessed directly using NFC registers */
                MaxColumnAddress = pDevice->pNFDInfo->DataAreaPerPage;

                /* IF (given column address is not a multiple of NFD Data packet size) */
                if (pDevice->NFCAccess.ColumnAddress % pDevice->NFCAccess.NFDDataPktSize)
                {
                    /* Align client buffer start */
                    if (Result = adi_nfc_AlignNFDPkt(pDevice,MaxColumnAddress))
                    {
                        /* exit on error */
                        break;
                    }
                    /* Number of elements remaining in this 1D buffer */
                    RemainElementsInBuffer -= pDevice->pNFCBuffer->ProcessedElementCount;
                }
            }
            /* ELSE (error correction disabled) */
            else
            {
                /* DMA can be used to access a whole Page (both Data & Spare area) */
                MaxColumnAddress = pDevice->pNFDInfo->PageSize;
            }

             /* Align NFD address as per Maximum Column address to start with */
            adi_nfc_UpdateNFDAddress(pDevice,0,MaxColumnAddress);

            /* client data pointer */
            pClientData = ((u8*)(pDevice->pNFCBuffer->Data) + pDevice->pNFCBuffer->ProcessedElementCount);

            /* process all buffers in the chain */
            do
            {
                /* Number of elements remaining in this NFD Page */
                RemainElementsInPage    = (MaxColumnAddress - pDevice->NFCAccess.ColumnAddress);

                /* Get the number of elements that we could process */
                if (RemainElementsInBuffer < RemainElementsInPage)
                {
                    RemainElementsToAccess = RemainElementsInBuffer;
                }
                else
                {
                    RemainElementsToAccess = RemainElementsInPage;
                }

                /* IF (Remaining elements to access is more than 255 bytes) */
                if (RemainElementsToAccess >= 256)
                {
                    /* Reset Remain descriptors count */
                    pDevice->NFCAccess.RemainDescriptors = 0;
                    /* Reset pointer to working descriptor */
                    pWorkingDescriptor = pDevice->NFCAccess.pStartDescriptor;
                    /* Reset address update count */
                    AddressUpdateCount = 0;

                    /* try to build at least 1 descriptor */
                    do
                    {
                        /* set the data start address */
                        pWorkingDescriptor->StartAddress = (void *)(pClientData);

/* For Moab class devices */
#if defined(__ADSP_MOAB__)

                        /* IF (Remaining elements to access >= 512 bytes) */
                        if (RemainElementsToAccess >= 512)
                        {
                            /* use 512 bytes access (128 * 4) */
                            pWorkingDescriptor->XCount = 128;
                            /* # elements in this descriptor */
                            DescriptorElementCount = 512;
                        }
                        /* ELSE IF (Remaining elements must be less than 512 bytes) */
                        else
                        {
                            /* use 256 bytes access (64 * 4) */
                            pWorkingDescriptor->XCount = 64;
                            /* # elements in this descriptor */
                            DescriptorElementCount = 256;
                        }

/* for Kookaburra and Mokingbird class devices */
#elif defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)

                        /* IF (Remaining elements to access >= 512 bytes) */
                        if (RemainElementsToAccess >= 512)
                        {
                            /* use 512 bytes access (256 * 2) */
                            pWorkingDescriptor->XCount = 256;
                            /* # elements in this descriptor */
                            DescriptorElementCount = 512;
                        }
                        /* ELSE IF (Remaining elements must be less than 512 bytes) */
                        else
                        {
                            /* use 256 bytes access (128 * 2) */
                            pWorkingDescriptor->XCount = 128;
                            /* # elements in this descriptor */
                            DescriptorElementCount = 256;
                        }

#endif
                        /* Increment number of descriptors to process */
                        pDevice->NFCAccess.RemainDescriptors++;

                        /* DMA direction */
                        pWorkingDescriptor->Config.b_WNR = wnr;
                        /* update client data pointer */
                        pClientData             += DescriptorElementCount;
                        /* decrement remaining elements to access count */
                        RemainElementsToAccess  -= DescriptorElementCount;
                        /* decrement remaining elements in buffer count */
                        RemainElementsInBuffer  -= DescriptorElementCount;
                        /* increment address update count */
                        AddressUpdateCount      += DescriptorElementCount;

                        /* Get the next descriptor in chain */
                        pWorkingDescriptor->pNext = (pWorkingDescriptor+1);
                        /* move to next descriptor in chain */
                        pWorkingDescriptor++;

                    }while ((pDevice->NFCAccess.RemainDescriptors < pDevice->NFCAccess.NumDescriptors) &&
                            (RemainElementsToAccess > 255));

                    /* terminate the last descriptor in chain */
                    pWorkingDescriptor--;
                    pWorkingDescriptor->pNext = NULL;

                    /* Submit Descriptors to NFC DMA */
                    if (Result = adi_nfc_SubmitDescriptors(pDevice))
                    {
                        /* exit on error */
                        break;
                    }

                    /* IF (Error Correction enabled) */
                    if (pDevice->NFCAccess.eECCMode < ADI_NFC_ECC_MODE_DISABLE)
                    {
                        /* Process Hardware generated ECC */
                        if (Result = adi_nfc_ProcessECC(pDevice,(AddressUpdateCount/pDevice->NFCAccess.NFDDataPktSize)))
                        {
                            /* exit on error */
                            break;
                        }
                    }

                    /* Update NFD Address field in NFC access structure */
                    adi_nfc_UpdateNFDAddress(pDevice,AddressUpdateCount,MaxColumnAddress);
                }

                /* IF (remaining elements to access is less than 256 bytes) */
                if ((RemainElementsToAccess < 256) && (RemainElementsToAccess != 0))
                {
                    /* Align client buffer end */
                    if (Result = adi_nfc_AlignNFDPkt(pDevice,MaxColumnAddress))
                    {
                        /* exit on error */
                        break;
                    }
                    /* update processed element count */
                    pDevice->pNFCBuffer->ProcessedElementCount = (pDevice->pNFCBuffer->ElementWidth * pDevice->pNFCBuffer->ElementCount);
                    /* we've processed all the elements in this 1D buffer */
                    RemainElementsInBuffer = 0;
                }

                /* IF (we've completed processing this buffer) */
                if (RemainElementsInBuffer == 0)
                {
                    /* IF (callback required for this buffer) */
                    if(pDevice->pNFCBuffer->CallbackParameter != NULL)
                    {
                        pCallbackBuffer = pDevice->pNFCBuffer;
	                }
	                else
	                {
	                    pCallbackBuffer = NULL;
	                }

	                /* move to next buffer in chain */
                    pDevice->pNFCBuffer = pDevice->pNFCBuffer->pNext;

                    /* IF (we've more buffers to process) */
                    if (pDevice->pNFCBuffer != NULL)
                    {
                        /* Number of elements remaining in next 1D buffer */
                        RemainElementsInBuffer  = (pDevice->pNFCBuffer->ElementWidth *\
                                                   pDevice->pNFCBuffer->ElementCount);

                        /* IF (application hasn't provided us with any DMA frame buffer memory) */
                        if (pDevice->NFCAccess.NumDescriptors == 0)
                        {
                            /* use 1D buffer reserved area to build the descriptor */
                            pDevice->NFCAccess.pStartDescriptor = (ADI_DMA_DESCRIPTOR_LARGE *)pDevice->pNFCBuffer->Reserved;
                            /* Configure descriptor fields */
                            adi_nfc_ConfigureDescriptors(pDevice,1);
                        }

                        /* Clear 1D buffer processed element count */
                        pDevice->pNFCBuffer->ProcessedElementCount = 0;
                        /* update pointer to client data */
                        pClientData = (u8*)(pDevice->pNFCBuffer->Data);
                    }

                    /* IF (we have a callback to post) */
                    if (pCallbackBuffer != NULL)
                    {
                        /* no need to check for deferred callbacks as Device Manager will take care of it */
                        (pDevice->DMCallback)(pDevice->DeviceHandle, ADI_DEV_EVENT_BUFFER_PROCESSED, (void *)pCallbackBuffer);
                    }
                }

            }while (pDevice->pNFCBuffer != NULL);

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

    Function:       adi_nfc_ConfigureDescriptors

    Description:    Configures NFC DMA Descriptor fields

*********************************************************************/
static void adi_nfc_ConfigureDescriptors(
    ADI_NFC_DEF   *pDevice,         /* pointer to NFC device we're working on   */
    u32           DescriptorCount   /* # descriptors to configure               */
){

    /* pointer to the descriptor we're working on   */
    ADI_DMA_DESCRIPTOR_LARGE    *pWorkingDescriptor;

    /* Initialise working descriptor */
    pWorkingDescriptor = pDevice->NFCAccess.pStartDescriptor;

    /* Configure all Descriptors */
    while (DescriptorCount)
    {

/* For Moab class devices */
#if defined(__ADSP_MOAB__)

        /* 32-bit DAB for Moab class devices */
        pWorkingDescriptor->XModify = 4;
        /* 32-bit transfers */
        pWorkingDescriptor->Config.b_WDSIZE     = ADI_DMA_WDSIZE_32BIT;

/* for Kookaburra and Mokingbird class devices */
#elif defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)

        /* 16-bit DAB for Kookaburra class devices */
        pWorkingDescriptor->XModify = 2;
        /* 16-bit transfers */
        pWorkingDescriptor->Config.b_WDSIZE     = ADI_DMA_WDSIZE_16BIT;

#endif
        /* set the descriptor values */
        pWorkingDescriptor->Config.b_FLOW       = ADI_DMA_FLOW_LARGE;
        pWorkingDescriptor->Config.b_NDSIZE     = ADI_DMA_NDSIZE_LARGE;
        pWorkingDescriptor->Config.b_DI_SEL     = ADI_DMA_DI_SEL_OUTER_LOOP;
        /* set the word size, direction and transfer type according to the buffer */
        /*  Enable Synchronized transition, Enable Callback */
        pWorkingDescriptor->Config.b_DMA2D      = ADI_DMA_DMA2D_LINEAR;
        pWorkingDescriptor->Config.b_RESTART    = ADI_DMA_RESTART_DISCARD;
        pWorkingDescriptor->CallbackFlag        = TRUE;

        /* move to next descriptor in chain */
        pWorkingDescriptor++;
        /* decrement descriptor count */
        DescriptorCount--;

    }

    /* return */
}

/*********************************************************************

    Function:       adi_nfc_AlignNFDPkt

    Description:    Accesses NFD by aligning NFD data packet to
                    256 bytes boundary

*********************************************************************/
static u32 adi_nfc_AlignNFDPkt(
    ADI_NFC_DEF     *pDevice,           /* pointer to NFC device we're working on   */
    u32             MaxColumnAddress    /* Maximum Column Address range             */
){
    u32 BytesToAlign;
    u32 BytesToCopy;
    u32 RemainElementsInClientBuf;
    u32 i;
    u8  *pClientData;
    u8  *pNFDData;

    /* default return code */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /*** Configure Start Descriptor ***/
    /* Set the data start address */
    (pDevice->NFCAccess.pStartDescriptor)->StartAddress = (void *)(pDevice->NFCAccess.pAlignBuf256Bytes);

/* For Moab class devices */
#if defined(__ADSP_MOAB__)

    /* use 256 bytes access (64 * 4) */
    (pDevice->NFCAccess.pStartDescriptor)->XCount = 64;

/* for Kookaburra and Mokingbird class devices */
#elif defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)

    /* use 256 bytes access (128 * 2) */
    (pDevice->NFCAccess.pStartDescriptor)->XCount = 128;

#endif

    /* Terminate this descriptor chain */
    (pDevice->NFCAccess.pStartDescriptor)->pNext = NULL;
    /* Read NFD and write to SDRAM */
    (pDevice->NFCAccess.pStartDescriptor)->Config.b_WNR = ADI_DMA_WNR_WRITE;
    /* number of descriptors to process */
    pDevice->NFCAccess.RemainDescriptors = 1;

    /* calculate the remaining elements in client buffer */
    RemainElementsInClientBuf = ((pDevice->pNFCBuffer->ElementWidth * pDevice->pNFCBuffer->ElementCount) -\
                                 pDevice->pNFCBuffer->ProcessedElementCount);

    /* IF (we're going to access NFD Page Data Area) */
    if ((MaxColumnAddress - pDevice->NFCAccess.ColumnAddress) > pDevice->NFCAccess.NFDDataPktSize)
    {
        /* # bytes to align */
        BytesToAlign = (pDevice->NFCAccess.ColumnAddress % pDevice->NFCAccess.NFDDataPktSize);
        /* reset column address to access NFD in 256 bytes boundary */
        pDevice->NFCAccess.ColumnAddress -= BytesToAlign;
    }
    /* ELSE (we're going to access NFD Page Spare Area) */
    else
    {
        BytesToAlign = pDevice->NFCAccess.NFDDataPktSize - (MaxColumnAddress - pDevice->NFCAccess.ColumnAddress);
        /* reset column address to access NFD in 256 bytes boundary */
        pDevice->NFCAccess.ColumnAddress = (MaxColumnAddress - pDevice->NFCAccess.NFDDataPktSize);
    }

    /* # bytes we're interested in */
    BytesToCopy = pDevice->NFCAccess.NFDDataPktSize - BytesToAlign;
    /* IF (remaining elements in the client Buffer is less than Bytes to copy) */
    if (RemainElementsInClientBuf < BytesToCopy)
    {
        BytesToCopy = RemainElementsInClientBuf;
    }

    /* Client data location */
    pClientData = ((u8*)(pDevice->pNFCBuffer->Data) + pDevice->pNFCBuffer->ProcessedElementCount);
    /* Point to NFD Data location we're interested in */
    pNFDData = (pDevice->NFCAccess.pAlignBuf256Bytes + BytesToAlign);

    /* Submit descriptors to read a NFD Data packet */
    Result = adi_nfc_SubmitDescriptors(pDevice);

    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
        /* IF (Read from NFD) */
        if (pDevice->NFCAccess.ReadFlag == 1)
        {
            /* copy data from NFD buffer to client buffer */
            for (i = BytesToCopy; i; i--,pClientData++,pNFDData++)
            {
                *pClientData = *pNFDData;
            }
        }
        /* ELSE (Write to NFD) */
        else
        {
            /* copy data from client buffer to NFD buffer */
            for (i = BytesToCopy; i; i--,pClientData++,pNFDData++)
            {
                *pNFDData = *pClientData;
            }

            /*** Update NFD data packet ***/
            /* Read SDRAM and write to NFD */
            (pDevice->NFCAccess.pStartDescriptor)->Config.b_WNR = ADI_DMA_WNR_READ;
            /* number of descriptors to process */
            pDevice->NFCAccess.RemainDescriptors = 1;
            /* Submit descriptors to update NFD Data packet */
            Result = adi_nfc_SubmitDescriptors(pDevice);
        }

        /* Update processed element count */
        pDevice->pNFCBuffer->ProcessedElementCount += BytesToCopy;
        /* update NFD Address field in NFC access structure */
        adi_nfc_UpdateNFDAddress(pDevice,pDevice->NFCAccess.NFDDataPktSize,MaxColumnAddress);
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_nfc_SubmitDescriptors

    Description:    Queues Descriptors to NFC DMA

*********************************************************************/
static u32 adi_nfc_SubmitDescriptors(
    ADI_NFC_DEF                 *pDevice       /* pointer to NFC device we're working on   */
){
    /* default return code */
    u32  Result = ADI_DEV_RESULT_SUCCESS;

    /* check element count of start descriptor in chain */
    /* IF (512-bytes page access) */
    if ((pDevice->NFCAccess.pStartDescriptor)->XCount * (pDevice->NFCAccess.pStartDescriptor)->XModify == 512)
    {
        /* Configure NFC Control register for 512-bytes page access */
        ADI_NFC_CTRL_PG_SIZE_SET_512BYTES;
    }
    /* ELSE (256-bytes page access) */
    else
    {
        /* Configure NFC Control register for 256-bytes page access */
        ADI_NFC_CTRL_PG_SIZE_SET_256BYTES;
    }

    /* Reset ECC Array pointer */
    pDevice->NFCAccess.pECCArray = pDevice->NFCAccess.pECCArrayStart;

    _adi_fss_FlushMemory ( 
                            (pDevice->NFCAccess.pStartDescriptor)->StartAddress, 
                            (pDevice->NFCAccess.pStartDescriptor)->XCount * (pDevice->NFCAccess.pStartDescriptor)->XModify,
                            (ADI_DMA_WNR)(pDevice->NFCAccess.pStartDescriptor)->Config.b_WNR
                         );
                            
    /* submit the descriptor chain to NFC DMA */
    Result = adi_dma_Queue(pDevice->NFCDMAInfo.ChannelHandle, (ADI_DMA_DESCRIPTOR_HANDLE)pDevice->NFCAccess.pStartDescriptor);

    if (Result == ADI_DMA_RESULT_SUCCESS)
    {
        /* IF (read from NFD and write to SDRAM) */
        if ((pDevice->NFCAccess.pStartDescriptor)->Config.b_WNR == ADI_DMA_WNR_WRITE)
        {
            /* Kick-off Page read (use DMA) */
            Result = adi_nfc_KickOffPageAccess (pDevice,ADI_NFC_USE_DMA,ADI_NFC_READ_NFD);
        }
        /* ELSE (read from SDRAM and write to NFD) */
        else
        {
            /* Kick-off Page write (use DMA) */
            Result = adi_nfc_KickOffPageAccess (pDevice,ADI_NFC_USE_DMA,ADI_NFC_WRITE_NFD);
        }
    }

    if (Result) {
        Result = Result;
    }
    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_nfc_KickOffPageAccess

    Description:    Kicks off NFD Page Access

*********************************************************************/
static u32 adi_nfc_KickOffPageAccess(   /* Kicks off NFD Page Access                */
    ADI_NFC_DEF     *pDevice,           /* pointer to NFC device we're working on   */
    u8              DMAFlag,            /* 1 = Use NFC DMA, 0 = Do not use DMA      */
    u8              ReadFlag            /* 1 = Read from NFD, 0 = Write to NFD      */
){
    /* assume we're going to be successful */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* Clear IRQ status flags to start with */
    ADI_NFC_IRQSTAT_CLEAR_ALL;

    /* IF (read from NFD) */
    if (ReadFlag)
    {
        /* Insert command to read a page */
        ADI_NFC_INSERT_CMD(ADI_NFD_CMD_PAGE_READ);
    }
    /* ELSE (write to NFD) */
    else
    {
        /* Insert command to program a page */
        ADI_NFC_INSERT_CMD(ADI_NFD_CMD_PAGE_PROGRAM);
    }

    /* Insert NFD address to access */
    if ((Result = adi_nfc_InsertAddress(pDevice, ADI_NFD_ADDR_TYPE_ACCESS_PAGE)) == ADI_DEV_RESULT_SUCCESS)
    {
        /* IF (read from NFD) */
        if (ReadFlag)
        {
            /* confirm page read */
            ADI_NFC_INSERT_CMD(ADI_NFD_CMD_PAGE_READ_CONFIRM);
            /* wait until NFD completes reading this page to its page buffer */
            Result = adi_nfc_WaitOnStatus(pDevice,ADI_NFC_WAIT_FOR_NBUSYIRQ);
        }
    }

    /* IF (Use NFC DMA) */
    if ((DMAFlag) && (Result == ADI_DEV_RESULT_SUCCESS))
    {
        /* acquire NFC Data semaphore */
        if ((Result = adi_sem_Pend (pDevice->DataSemaphoreHandle,
                                    ADI_SEM_TIMEOUT_FOREVER)) == (u32)ADI_SEM_RESULT_SUCCESS)
        {
            /* Set NFC operating state as DMA data transfer in progress */
            pDevice->NFCAccess.NFCXferStatus = ADI_NFC_STATE_DATA_XFER_IN_PROGRESS;

            /* IF (read from NFD) */
            if (ReadFlag)
            {
                /* kick-off NFC DMA to read a page */
                ADI_NFC_PGCTL_PAGE_READ_START;
            }
            /* ELSE (write to NFD) */
            else
            {
                /* start page write */
                ADI_NFC_PGCTL_PAGE_WRITE_START;
            }

            /* wait until DMA completes this page access */
            if ((Result = adi_sem_Pend (pDevice->DataSemaphoreHandle,
                                        ADI_SEM_TIMEOUT_FOREVER)) == (u32)ADI_SEM_RESULT_SUCCESS)
            {
                /* release NFC data semaphore */
                adi_sem_Post (pDevice->DataSemaphoreHandle);

                /* IF (write to NFD) */
                if (!ReadFlag)
                {
                    /* Confirm Page Program */
                    ADI_NFC_INSERT_CMD(ADI_NFD_CMD_PAGE_PROGRAM_CONFIRM);
                    /* wait until NFD completes updating this page */
                    Result = adi_nfc_WaitOnStatus(pDevice,ADI_NFC_WAIT_FOR_NBUSYIRQ);
                }
            }
        }
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_nfc_ProcessECC

    Description:    Processes Hardware generated ECC

    For NFD Data Write - Saves calculated ECC code to corresponding
                         NFD Data packet spare area

    For NFD Data Read  - Performs Error correction on Data packets
                         read from NFD using hardware calculated ECC
                         and by reading ECC values of corresponding
                         NFD Data packet stored in NFD spare area

*********************************************************************/
static u32 adi_nfc_ProcessECC(
    ADI_NFC_DEF     *pDevice,           /* pointer to NFC device we're working on   */
    u32             NumPktsAccessed     /* # NFD Data packets accessed in this page */
){
    u8  DoErrorCorrection,ByteVal,DataPktSize;
	u16 Syndrome1,Syndrome2,Syndrome3,Syndrome4;
	u16 FailingBit, FailingByte;
	u16 BkupColumnAddress, BytesToSkip;
    u32 Syndrome0;
	u32 u32Temp;
    /* assume we're going to be successful */
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to array holding ECC stored in NFD */
    u16 *pECCStored;

    /* Store address of location that holds ECC calculated by NFC hardware */
    u16 *pECCCalculated = pDevice->NFCAccess.pECCArrayStart;
    /* Save the address of client data buffer that requires error checking */
	u8  *pBuffer = ((u8*)(pDevice->NFCAccess.pStartDescriptor)->StartAddress);

    /* Calculate Number of bytes to skip within the last 256 bytes of the page to reach Spare area start */
    BytesToSkip = (pDevice->NFCAccess.NFDDataPktSize - (pDevice->pNFDInfo->PageSize - pDevice->pNFDInfo->DataAreaPerPage));
    /* IF (ECC stored non-sequentially) */
    if (pDevice->NFCAccess.eECCMode == ADI_NFC_ECC_MODE_NON_SEQUENTIAL)
    {
        /* Also, add number of bytes to skip within the NFD spare area to reach
           Spare area start of NFD Data packet number that corresponds to the present NFD Column Address */
        BytesToSkip += ((pDevice->NFCAccess.ColumnAddress / pDevice->NFCAccess.NFDDataPktSize) * pDevice->NFCAccess.SpareUnitsPerPkt);
        /* Within the last 256 Bytes of a NFD page - Point to ECC0 location of
           NFD packet that corresponds to the present NFD Column Address */
        pECCStored = (u16*)((u8*)(pDevice->NFCAccess.pAlignBuf256Bytes) + (BytesToSkip + pDevice->NFCAccess.ECCStart));
    }
    /* ELSE (ECC Stored sequentially) */
    else
    {
        /* Skip # bytes in NFD spare area to reach ECC Start of specific data packet in that page
           (Multiplied by 4 to convert ECC size to bytes, as NFC generates 4-bytes ECC for every NFD data packet (256 bytes) */
        BytesToSkip += (((pDevice->NFCAccess.ColumnAddress / pDevice->NFCAccess.NFDDataPktSize) * 4U) + pDevice->NFCAccess.ECCStart);
        /* Within the last 256 Bytes of a NFD page - Point to ECC0 location of first NFD data packet in this page */
        pECCStored = (u16*)((u8*)(pDevice->NFCAccess.pAlignBuf256Bytes) + BytesToSkip );
    }

    /* save the present column address in some temp location */
	BkupColumnAddress = pDevice->NFCAccess.ColumnAddress;
    /* Calculate column address to read the last 256 bytes of the page */
    pDevice->NFCAccess.ColumnAddress = (pDevice->pNFDInfo->PageSize - pDevice->NFCAccess.NFDDataPktSize);

    /* IF (we have to process ECC for data read from NFD) */
    if((pDevice->NFCAccess.pStartDescriptor)->Config.b_WNR == ADI_DMA_WNR_WRITE)
    {
        DoErrorCorrection = 1;
    }
    /* ELSE (we have to store ECC to spare area as we've written some data to NFD) */
    else
    {
        DoErrorCorrection = 0;
    }

    /*** Configure Start Descriptor ***/
    /* Set the data start address */
    (pDevice->NFCAccess.pStartDescriptor)->StartAddress = (void *)(pDevice->NFCAccess.pAlignBuf256Bytes);

/* For Moab class devices */
#if defined(__ADSP_MOAB__)

    /* use 256 bytes access (64 * 4) */
    (pDevice->NFCAccess.pStartDescriptor)->XCount = 64;

/* for Kookaburra and Mokingbird class devices */
#elif defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)

    /* use 256 bytes access (128 * 2) */
    (pDevice->NFCAccess.pStartDescriptor)->XCount = 128;

#endif

    /* Terminate this descriptor chain */
    (pDevice->NFCAccess.pStartDescriptor)->pNext = NULL;
    /* Read NFD and write to SDRAM */
    (pDevice->NFCAccess.pStartDescriptor)->Config.b_WNR = ADI_DMA_WNR_WRITE;
    /* number of descriptors to process */
    pDevice->NFCAccess.RemainDescriptors = 1;

    /* Reset ECC Start to some temp location so that we don't corrupt the ECC calculated for client data */
    pDevice->NFCAccess.pECCArrayStart = (u16*)&u32Temp;

    /* Submit descriptors to read a NFD Data packet */
    Result = adi_nfc_SubmitDescriptors(pDevice);

    /* Restore ECC Array pointer */
    pDevice->NFCAccess.pECCArrayStart = pECCCalculated;

    /* IF (Perform error correction on client data) */
    if (DoErrorCorrection)
    {
        /* DO (error correction for all NFD Data packets accessed in this page) */
        while (NumPktsAccessed--)
        {
            /* calculate Error Syndrome 0 */
        	Syndrome0 = ((*(pECCCalculated+1) << 11) | *(pECCCalculated)) ^ ((*(pECCStored+1) << 11) | *pECCStored);

            /* IF (NFD Data error) */
    	    if(Syndrome0 != 0)
	        {
	            /* calculate other error syndromes */
    	        Syndrome1   = *pECCCalculated ^ *pECCStored;
                Syndrome2   = *pECCCalculated ^ *(pECCCalculated + 1);
                Syndrome3   = *(pECCStored+1) ^ *pECCStored;
                Syndrome4   = Syndrome2 ^ Syndrome3;

                /* IF (This is a single bit error) */
                if ((__builtin_ones(Syndrome0) == 11) && (Syndrome4 == 0x7FF))
                {
    	        	/* Get the failing bit within the byte */
                    FailingBit  = Syndrome1 & 0x7;
                    /* Get the byte number */
    	            FailingByte = Syndrome1 >> 0x3;
	                /* Get the actual byte */
    	            ByteVal     = *(pBuffer + FailingByte);
	                /* Invert the failing bit */
	                ByteVal     = ByteVal ^ (0x1 << FailingBit);
    	            /* Write the corrected data back */
        	        *(pBuffer + FailingByte) = ByteVal;
                }
                /* ELSE (this error is uncorrectable) */
                else
                {
                    Result = ADI_NFC_RESULT_NFD_DATA_ERROR;
                    break;
                }
	        }
	        /* Move to location holding ECC of next NFD Data packet */
    	    pECCCalculated += 2U;

    	    /* IF (ECC stored non-sequentially) */
            if (pDevice->NFCAccess.eECCMode == ADI_NFC_ECC_MODE_NON_SEQUENTIAL)
            {
    	        /* skip the remaining ECC spare area of this NFD data packet */
                pECCStored += (pDevice->NFCAccess.SpareUnitsPerPkt/sizeof(u16));
            }
            /* ELSE (ECC stored sequentially) */
            else
            {
                pECCStored += 2U;
            }

            /* Move to next data packet */
            pBuffer += 256U;
	    }
	}
	/* ELSE (save ECC to NFD spare area) */
	else
	{
        /* DO (save ECC for all NFD Data packets accessed in this page) */
        while (NumPktsAccessed--)
        {
            /* Copy calculated ECC0 to 256bytes buffer that is to be written to ECC spare area */
            *pECCStored = *pECCCalculated;
            /* Copy calculated ECC1 to 256bytes buffer that is to be written to ECC spare area */
            *(pECCStored+1) = *(pECCCalculated+1);
            /* Move to ECC calculated of next NFD data packet */
            pECCCalculated += 2;
            /* IF (ECC stored non-sequentially) */
            if (pDevice->NFCAccess.eECCMode == ADI_NFC_ECC_MODE_NON_SEQUENTIAL)
            {
    	        /* skip the remaining ECC spare area of this NFD data packet */
                pECCStored += (pDevice->NFCAccess.SpareUnitsPerPkt/sizeof(u16));
            }
            /* ELSE (ECC stored sequentially) */
            else
            {
                pECCStored += 2U;
            }
        }

        /* Read SDRAM and write to NFD */
        (pDevice->NFCAccess.pStartDescriptor)->Config.b_WNR = ADI_DMA_WNR_READ;
        /* number of descriptors to process */
        pDevice->NFCAccess.RemainDescriptors = 1;
        /* save the original ECC array start pointer */
        pECCCalculated = pDevice->NFCAccess.pECCArrayStart;
        /* Reset ECC Start to some temp location */
        pDevice->NFCAccess.pECCArrayStart = (u16*)&u32Temp;
        /* Submit descriptors to update NFD Spare Area */
        Result = adi_nfc_SubmitDescriptors(pDevice);
        /* Restore ECC Array pointer */
        pDevice->NFCAccess.pECCArrayStart = pECCCalculated;
    }

    /* Restore column address */
	pDevice->NFCAccess.ColumnAddress = BkupColumnAddress;

	/* return */
	return (Result);
}

/*********************************************************************

    Function:       adi_pdd_Control

    Description:    Configures the NFC device

*********************************************************************/
static u32 adi_pdd_Control(             /* Sets or senses device specific parameter */
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle                               */
    u32                     Command,    /* Command ID                               */
    void                    *Value      /* Command specific value                   */
){
    u8                          u8Temp;
    u16                         u16Temp,i;
    u16                         SectorsPerPage,SectorsPerBlock,SectorSize;
	u32                         u32Value;
    ADI_NFC_DEF                 *pDevice;       /* Device Instance */
    u32                         Result;         /* return value    */
    ADI_DEV_FREQUENCIES         *pFreqs;
    ADI_NFD_ADDRESS             *pAddress;
    ADI_NFC_DMA_FRAME_BUFFER    *pDMAFrame;
    ADI_DMA_CHANNEL_ID          ChannelID;      /* DMA channel ID */

	/* pointer to NFC device we're working on */
    pDevice = (ADI_NFC_DEF *)PDDHandle;
    u32Value = (u32)Value;

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

            break;

        /* CASE (dataflow method) */
        case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)

            /* NFC only supports chained dataflow method */
            if ((ADI_DEV_MODE)Value != ADI_DEV_MODE_CHAINED)
            {
                Result = ADI_DEV_RESULT_NOT_SUPPORTED;
                break;
            }

#endif  /* ADI_DEV_DEBUG */

            /* IF (NFC DMA is not open yet) */
            if (pDevice->NFCDMAInfo.ChannelHandle == NULL)
            {
                /* Get NFC DMA channel ID */
                if ((Result = adi_dma_GetMapping(pDevice->NFCDMAInfo.MappingID,&ChannelID)) != ADI_DMA_RESULT_SUCCESS)
                {
                    break;
                }

                /* Open NFC DMA channel */
                if (Result = adi_dma_Open(  pDevice->DMAHandle,
                                            ChannelID,
                                            pDevice,
                                            &(pDevice->NFCDMAInfo.ChannelHandle),
                                            ADI_DMA_MODE_DESCRIPTOR_LARGE,
                                            NULL,
                                            adi_nfc_DMACallback))
                {
                    break;
                }

                /* Activate/De-activate NFC */
                Result = adi_nfc_Activate(pDevice,(u8)u32Value);
            }

            break;

        /* CASE (query for processor DMA support) */
        case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):

            /*
               NFC is a half-duplex device and is mapped to a single DMA channel
               To have a better control over DMA dataflow, the NFC DMA and buffers
               are handled by the driver itself rather than the Device manager
            */
            *((u32 *)Value) = FALSE;

            break;

        /* CASE (Enable NFC Error reporting) */
        case (ADI_DEV_CMD_SET_ERROR_REPORTING):

            break;

        /* CASE (CCLK/SCLK Frequency Change) */
        case (ADI_DEV_CMD_FREQUENCY_CHANGE_PROLOG):
        case (ADI_DEV_CMD_FREQUENCY_CHANGE_EPILOG):

            /* Reset and initialise NFC registers */
            Result = adi_nfc_InitRegisters(pDevice);

            break;

        /****************************
        NFC driver Specific Commands
        ****************************/

        /* CASE (Set NAND Flash Device specific information) */
        case (ADI_NFC_CMD_PASS_NFD_INFO):
        /* CASE (Set address of DMA Frame buffer that can be
                 used to build descriptor chains to access a NFD page) */
        case (ADI_NFC_CMD_SET_DMA_FRAME_BUFFER):

            /* IF (Command is to set NAND Flash Device specific information) */
            if (Command == ADI_NFC_CMD_PASS_NFD_INFO)
            {
                /* Save NFD information table address */
                pDevice->pNFDInfo = (ADI_NFD_INFO_TABLE*)Value;

                /* reset and initialise NFC registers */
                if ((Result = adi_nfc_InitRegisters(pDevice)) != ADI_DEV_RESULT_SUCCESS)
                {
                    /* Given NFD information table is invalid. Reset pointer to NFD information table */
                    pDevice->pNFDInfo = NULL;
                    break;
                }

                /* Call port control to enable NFC signals */
                Result = adi_nfc_SetPortControl(pDevice);
            }
            /* ELSE (Command is to set DMA Frame buffer address) */
            else
            {
                pDMAFrame = (ADI_NFC_DMA_FRAME_BUFFER *)Value;

                /* update NFC Access structure */
                pDevice->NFCAccess.pStartDescriptor     = pDMAFrame->pDMAFrameMemory;
                pDevice->NFCAccess.NumDescriptors       = pDMAFrame->NumDMAFrames;
            }

            /* IF (application has provided us with any DMA frame buffer memory) */
            if ((pDevice->NFCAccess.NumDescriptors != 0) && (pDevice->pNFDInfo != NULL))
            {
                /* Calculate the exact number of descriptors required to access a NFD page */
                u8Temp = (u8)(pDevice->pNFDInfo->DataAreaPerPage / pDevice->NFCAccess.NFDDataPktSize);

                /* IF (the given DMA Frame memory is more than what we need) */
                if (pDevice->NFCAccess.NumDescriptors > u8Temp)
                {
                    /* adjust NumDescriptors to exact number of descriptors we need */
                    pDevice->NFCAccess.NumDescriptors = u8Temp;
                }

                /* Calculate ECC Array start location */
                pDevice->NFCAccess.pECCArrayStart = (u16*)((u8*)pDevice->NFCAccess.pStartDescriptor +\
                                                           (sizeof(ADI_DMA_DESCRIPTOR_LARGE) * pDevice->NFCAccess.NumDescriptors));

                /* Configure NFC DMA Descriptors */
                adi_nfc_ConfigureDescriptors(pDevice,pDevice->NFCAccess.NumDescriptors);
            }

            break;

        /* CASE (Pass address of 256 bytes buffer that can be used for NFD Data Packet alignment) */
        case (ADI_NFC_CMD_SET_256BYTES_ALIGN_BUFFER):

            /* save the address of Align buffer */
            pDevice->NFCAccess.pAlignBuf256Bytes = (u8*)Value;

            break;

        /* CASE (Get address of NAND Flash Device information structure) */
        case (ADI_NFC_CMD_GET_NFD_INFO):

            /* save the address of NAND Flash Device information structure */
            *((ADI_NFD_INFO_TABLE**)Value) = pDevice->pNFDInfo;

            break;

        /* CASE (Set ECC mode) */
        case (ADI_NFC_CMD_SET_ECC_MODE):

            /* Save ECC Mode */
            pDevice->NFCAccess.eECCMode = (ADI_NFC_ECC_MODE)(u32Value);

            break;

        /* CASE (Set Nth byte/word of NFD spare area
                 that holds/to hold 1st byte/word of ECC) */
        case (ADI_NFC_CMD_SET_ECC_START_LOC):

            /* Save ECC Start */
            pDevice->NFCAccess.ECCStart = (u16)(u32Value);

            break;

        /* CASE (Send a NFD access request using Block/Page/Column address) */
        case (ADI_NFC_CMD_SEND_NFD_ACCESS_REQUEST):

            pAddress = (ADI_NFD_ADDRESS*)Value;

            /* reset NFC registers and Access parameters */
            adi_nfc_ResetNFCAccess(pDevice);

            /* update NFC Access parameters */
            pDevice->NFCAccess.BlockAddress     = pAddress->BlockAddress;
            pDevice->NFCAccess.PageAddress      = pAddress->PageAddress;
            pDevice->NFCAccess.ColumnAddress    = pAddress->ColumnAddress;

            /* Validate the given NFD address */
            Result = adi_nfc_ValidateNFDAddress(pDevice);

            break;

        /* CASE (Send a NFD access request using physical sector number) */
        case (ADI_NFC_CMD_SEND_SECTOR_ACCESS_REQUEST):

            /* reset NFC registers and Access parameters */
            adi_nfc_ResetNFCAccess(pDevice);

            /* IF (x16 type NFD) */
            if ((pDevice->pNFDInfo->NFDType == ADI_NFD_SMALL_PAGE_x16) ||
                (pDevice->pNFDInfo->NFDType == ADI_NFD_LARGE_PAGE_x16))
            {
                SectorSize = ADI_NFD_SECTOR_SIZE_IN_BYTES/2;
            }
            /* ELSE (x8 type NFD) */
            else
            {
                SectorSize = ADI_NFD_SECTOR_SIZE_IN_BYTES;
            }

            /* Calculate Sectors per Page */
            SectorsPerPage = (pDevice->pNFDInfo->DataAreaPerPage / SectorSize);
            /* Calculate Sectors per Block */
            SectorsPerBlock = (SectorsPerPage * pDevice->pNFDInfo->PagesPerBlock);

            /*** Calculate Block Address ***/
            /* Block Address = (Sector Number / Sectors per Block) */
            pDevice->NFCAccess.BlockAddress = (u32Value / SectorsPerBlock);

            /*** Calculate Page Address ***/
            /* Page Address = (Sector number within this Block / Sectors per Page) */
            pDevice->NFCAccess.PageAddress  = ((u32Value % SectorsPerBlock) / SectorsPerPage);

            /*** Calculate Column address ***/
            /* Column address = (Sector number within this Page * Sector size) */
            pDevice->NFCAccess.ColumnAddress = ((u32Value % SectorsPerPage) * SectorSize);

            /* Validate the given NFD address */
            Result = adi_nfc_ValidateNFDAddress(pDevice);

            break;

        /* CASE (Terminates all NFD access thats been queued or in progress) */
        case (ADI_NFC_CMD_TERMINATE_NFD_ACCESS):

            /* IF (NFD information table is NULL) */
            if (pDevice->pNFDInfo == NULL)
            {
                Result = ADI_NFC_RESULT_NFD_UNDEFINED;
                break;
            }

            /* Reset NFC registers and Access parameters */
            adi_nfc_ResetNFCAccess(pDevice);
            /* Reset NFD Command interface and status register */
            ADI_NFC_INSERT_CMD(ADI_NFD_CMD_RESET);
            /* wait until NFD completes reading this page to its page buffer */
            if (Result = adi_nfc_WaitOnStatus(pDevice,ADI_NFC_WAIT_FOR_NBUSYIRQ))
            {
                /* exit on error */
                break;
            }
            /* IF (NFC DMA channel is in use) */
            if (pDevice->NFCDMAInfo.ChannelHandle)
            {
                /* Disable DMA dataflow */
                Result = adi_dma_Control(pDevice->NFCDMAInfo.ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)FALSE);
                /* Flush DMA buffers */
                adi_dma_Control(pDevice->NFCDMAInfo.ChannelHandle, ADI_DMA_CMD_FLUSH, (void *)NULL);
            }

            /* release exclusive access flag */
            pDevice->NFCAccess.ExclusiveAccessFlag = FALSE;

            break;

        /* default - commands that require exclusive access to NFC/NFD */
        default:

            /* IF (NFD information table is NULL) */
            if (pDevice->pNFDInfo == NULL)
            {
                Result = ADI_NFC_RESULT_NFD_UNDEFINED;
                break;
            }

            /* IF (NFC/NFD access request is already in progress) */
            if (pDevice->NFCAccess.ExclusiveAccessFlag == TRUE)
            {
                Result = ADI_NFC_RESULT_NFD_ACCESS_IN_PROGRESS;
                break;
            }

            /* NFD table is valid and NFC/NFD is avaliable for access */
            /* perform the requested task by acquiring exclusive access to NFC and NFD */

            /* Clear NFC Not Busy IRQ to start with */
            ADI_NFC_IRQSTAT_NBUSYIRQ_CLEAR;

            /* CASEOF (Command ID) */
            switch(Command)
            {
                /* CASE (Command to Erase a NFD Block) */
                case (ADI_NFC_CMD_ERASE_NFD_BLOCK):

                    /* Acquire exclusive access of NFC and NFD */
                    pDevice->NFCAccess.ExclusiveAccessFlag = TRUE;

                    /* update NFC Access parameters */
                    pDevice->NFCAccess.BlockAddress     = (u16)u32Value;
                    pDevice->NFCAccess.PageAddress      = 0;
                    pDevice->NFCAccess.ColumnAddress    = 0;

                    /* Issue Block erase Command to NFD */
       	            ADI_NFC_INSERT_CMD(ADI_NFD_CMD_BLOCK_ERASE);

   	                /* Inset block address to erase */
   	                if (Result = adi_nfc_InsertAddress(pDevice,ADI_NFD_ADDR_TYPE_ERASE_BLOCK))
       	            {
   	                    /* exit on error */
   	                    break;
       	            }

   	                /* Issue Block erase confirm Command to NFD */
   	                ADI_NFC_INSERT_CMD(ADI_NFD_CMD_BLOCK_ERASE_CONFIRM);

       	            /* wait until NFD completes erasing this block */
                    Result = adi_nfc_WaitOnStatus(pDevice,ADI_NFC_WAIT_FOR_NBUSYIRQ);

                    /* release exclusive access flag */
                    pDevice->NFCAccess.ExclusiveAccessFlag = FALSE;

                    break;

                /* CASE (Reads #bytes/#words from NFD starting from the given Block/Page/Column address) */
                case (ADI_NFC_CMD_READ_NFD_DIRECT):

                    /* Acquire exclusive access of NFC and NFD */
                    pDevice->NFCAccess.ExclusiveAccessFlag = TRUE;

                    /* Read NFD directly (without using DMA) */
                    Result = adi_nfc_DirectAccessNFD (pDevice,(ADI_NFD_DIRECT_ACCESS*)Value,ADI_NFC_READ_NFD);

                    /* release exclusive access flag */
                    pDevice->NFCAccess.ExclusiveAccessFlag = FALSE;

                    break;

                /* CASE (Writes #bytes/#words to NFD starting from the given Block/Page/Column address) */
                case (ADI_NFC_CMD_WRITE_NFD_DIRECT):

                    /* Acquire exclusive access of NFC and NFD */
                    pDevice->NFCAccess.ExclusiveAccessFlag = TRUE;

                    /* Write to NFD directly (without using DMA) */
                    Result = adi_nfc_DirectAccessNFD (pDevice,(ADI_NFD_DIRECT_ACCESS*)Value,ADI_NFC_WRITE_NFD);

                    /* release exclusive access flag */
                    pDevice->NFCAccess.ExclusiveAccessFlag = FALSE;

                    break;

                /* CASE (Get NFD status register value) */
                case (ADI_NFC_CMD_GET_NFD_STATUS):

                    /* Acquire exclusive access of NFC and NFD */
                    pDevice->NFCAccess.ExclusiveAccessFlag = TRUE;

                    /* clear client provided location */
                    *((u8 *)Value) = 0;

                    /* Issue NFD command to read NFD status register */
                    ADI_NFC_INSERT_CMD(ADI_NFD_CMD_READ_STATUS);

                    /* IF (x16 type NFD) */
                    if ((pDevice->pNFDInfo->NFDType == ADI_NFD_SMALL_PAGE_x16) ||
                        (pDevice->pNFDInfo->NFDType == ADI_NFD_LARGE_PAGE_x16))
                    {
                        /* read one word from NFD */
                        if (Result = adi_nfc_ReadSingleUnit(pDevice,&u16Temp))
                        {
                            break;  /* exit on error */
                        }
                        /* Store NFD status to client */
    	                *((u8 *)Value) = (u8)(u16Temp);
                    }
                    /* ELSE (x8 type NFD) */
                    else
                    {
                        /* read one byte from NFD */
                        if (Result = adi_nfc_ReadSingleUnit(pDevice,(u8 *)Value))
                        {
                            break;  /* exit on error */
                        }
                    }

                    /* release exclusive access flag */
                    pDevice->NFCAccess.ExclusiveAccessFlag = FALSE;

                    break;

                /* CASE (Get NFD Electronic signature) */
                case (ADI_NFC_CMD_GET_NFD_ELECTRONIC_SIGNATURE):

                    /* Acquire exclusive access of NFC and NFD */
                    pDevice->NFCAccess.ExclusiveAccessFlag = TRUE;

                    /* clear client provided location */
                    *((u32 *)Value) = 0;

                    /* Issue NFD command to NFD electronic signature */
                    ADI_NFC_INSERT_CMD(ADI_NFD_CMD_READ_ELECTRONIC_SIGN);
                    *pADI_NFC_ADDR = 0x00;
                    Result = adi_nfc_WaitOnStatus(pDevice,ADI_NFC_WAIT_FOR_WRITE_BUF_EMPTY);
                    
                    /* IF (x16 type NFD) */
                    if ((pDevice->pNFDInfo->NFDType == ADI_NFD_SMALL_PAGE_x16) ||
                        (pDevice->pNFDInfo->NFDType == ADI_NFD_LARGE_PAGE_x16))
                    {
                        /* read data in terms of words */
                        for(i=0; i<2; i++)
                        {
                            /* read one word from NFD */
                            if (Result = adi_nfc_ReadSingleUnit(pDevice,&u16Temp))
                            {
                                break;  /* exit on error */
                            }
                            /* append this word to form a complete data */
    	                    *((u32 *)Value) |= (u16Temp << (i * 16));
                        }
                    }
                    /* ELSE (x8 type NFD) */
                    else
                    {
                        /* read data in terms of bytes */
                        for(i=0; i<4; i++)
                        {
                            /* read one byte from NFD */
                            if (Result = adi_nfc_ReadSingleUnit(pDevice,&u8Temp))
                            {
                                break;  /* exit on error */
                            }
                            /* append this word to form a complete data */
    	                    *((u32 *)Value) |= (u8Temp << (i * 8));
                        }
                    }
                    /* release exclusive access flag */
                    pDevice->NFCAccess.ExclusiveAccessFlag = FALSE;

                    break;

                /* Command not supported */
                default:
                    Result = ADI_NFC_RESULT_CMD_NOT_SUPPORTED;
                    break;
            }
            break;
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_nfc_Activate

    Description:    Activates NFC device and configures it for use

*********************************************************************/
static u32 adi_nfc_Activate(
    ADI_NFC_DEF *pDevice,       /* pointer to NFC device we're working on   */
    u32         EnableFlag      /* Enable/Disable Flag                      */
){

    u32 Result = ADI_DEV_RESULT_SUCCESS;    /* default return code */

    /* IF (Enable NFC device) */
    if (EnableFlag)
    {
/* For ADSP-BF54x (MOAB) Family */
#if defined(__ADSP_MOAB__)

        /*** Configure DMA Peripheral Mux register for NFC use ***/
        /*### TEMPORARY - this register configuration will be replaced by a
                              DMA manager control command & adi_dma_Control() call ###*/
        *pDMAC1_PERIMUX = 0;

#endif
        /* IF (NFC DMA channel is already open) */
        if (pDevice->NFCDMAInfo.ChannelHandle)
        {
            /* Enable DMA dataflow */
            Result = adi_dma_Control(pDevice->NFCDMAInfo.ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)TRUE);
        }
        else
        {
            Result = ADI_DEV_RESULT_DMA_CHANNEL_UNAVAILABLE;
        }
    }
    /* ELSE (Disable NFC Device) */
    else
    {
        /* Disable NFC Status interrupts */
        if ((Result = adi_int_SICDisable(pDevice->NFCStatusIntID)) == ADI_INT_RESULT_SUCCESS)
        {
            /* IF (NFC DMA channel is in use) */
            if (pDevice->NFCDMAInfo.ChannelHandle)
            {
                /* Disable DMA dataflow */
                Result = adi_dma_Control(pDevice->NFCDMAInfo.ChannelHandle, ADI_DMA_CMD_SET_DATAFLOW, (void *)FALSE);
            }
        }
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_nfc_InitRegisters

    Description:    Initialises NFC registers

*********************************************************************/
static u32 adi_nfc_InitRegisters (
    ADI_NFC_DEF     *pDevice            /* pointer to NFC device we're working on  */
){
    u8  SclkPeriod,u8Temp,MaxVal;
    u16 DataAreaInBytes;
    u32 DeviceSize,u32Temp,fsclk;
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* IF (NFD Type Valid) */
    if (pDevice->pNFDInfo->NFDType == ADI_NFD_UNDEFINED)
    {
        /* NFD type is not defined */
        Result= ADI_NFC_RESULT_NFD_UNDEFINED;
    }

/* for Kookaburra and Mokingbird class devices */
#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)

    /* ELSE IF (x16 type NFD - not supported by kookaburra processors */
    else if ((pDevice->pNFDInfo->NFDType == ADI_NFD_SMALL_PAGE_x16) ||
             (pDevice->pNFDInfo->NFDType == ADI_NFD_LARGE_PAGE_x16))
    {
        /* Processor doesnot support this NFD type */
        Result= ADI_NFC_RESULT_NFD_NOT_SUPPORTED;
    }

#endif

    /* ELSE IF (small page device - not supported by the driver) */
    else if ((pDevice->pNFDInfo->NFDType == ADI_NFD_SMALL_PAGE_x8) ||
             (pDevice->pNFDInfo->NFDType == ADI_NFD_SMALL_PAGE_x16))
    {
        /* Driver doesnot support this NFD type */
        Result= ADI_NFC_RESULT_NFD_NOT_SUPPORTED;
    }
    /* ELSE (NFD type supported by the driver */
    else
    {
        /* Reset NFC registers and Access parameters to start with */
        adi_nfc_ResetNFCAccess(pDevice);

        /* set NAND Data width */
        /* IF (NFD buswidth = x8) */
        if ((pDevice->pNFDInfo->NFDType == ADI_NFD_SMALL_PAGE_x8) ||
            (pDevice->pNFDInfo->NFDType == ADI_NFD_LARGE_PAGE_x8))
        {
            /* Set NFC Control register for 8-bit NFC bus */
            ADI_NFC_CTRL_NWIDTH_SET_8BIT;
            /* 1 NFD Data packet = 256 bytes */
            pDevice->NFCAccess.NFDDataPktSize = ADI_NFD_DATA_PKT_SIZE_IN_BYTES;
            /* Page Data Area Size in bytes */
            DataAreaInBytes     = pDevice->pNFDInfo->DataAreaPerPage;
        }
        /* ELSE (NFD Buswidth = x16) */
        else
        {
            /* Set NFC Control register for 16-bit NFC bus */
            ADI_NFC_CTRL_NWIDTH_SET_16BIT;
            /* 1 NFD Data packet = 128 words */
            pDevice->NFCAccess.NFDDataPktSize = (ADI_NFD_DATA_PKT_SIZE_IN_BYTES / 2);
            /* Page Data Area Size in bytes */
            DataAreaInBytes     = (pDevice->pNFDInfo->DataAreaPerPage * 2);
        }

        /* Find the current SCLK frequency */
        /* default to power-up values (CCLK = 250MHz, SCLK = 50MHz) if call fails */
        if (adi_pwr_GetFreq(&u32Temp,&fsclk,&u32Temp)!= ADI_PWR_RESULT_SUCCESS)
        {
            fsclk = 50000000;
        }

        /* get SCLK period */
        SclkPeriod = (u8)(1000000000/fsclk);
        /* round off SCLK period */
        if (1000000000 % fsclk)
        {
            SclkPeriod++;
        }

        /*** Calculate Write Strobe delay ***/

        /* get maximum of tWP,tCS or SCLK assuming tWP as maximum */
        MaxVal = pDevice->pNFDInfo->tWPmin;

        /* IF (SCLK is the maximum) */
        if ((SclkPeriod >= pDevice->pNFDInfo->tCSmin) &&
            (SclkPeriod >= pDevice->pNFDInfo->tWPmin))
        {
            /* update MaxVal */
            MaxVal = SclkPeriod;
        }
        /* ELSE IF (tCSmin is the maximum) */
        else if (MaxVal < (pDevice->pNFDInfo->tCSmin - SclkPeriod))
        {
            MaxVal = (pDevice->pNFDInfo->tCSmin-SclkPeriod);
        }

        /* calculate WR_DLY in terms of SCLK cycles */
        u8Temp = MaxVal/SclkPeriod;
        /* round off WR_DLY */
        if (MaxVal%SclkPeriod)
        {
            u8Temp++;
        }

        /* Configure NFC Control register with calculated Write Strobe delay (WR_DLY) */
        ADI_NFC_CTRL_WR_DLY_SET(u8Temp);

        /*** Calculate Read Strobe delay ***/
        /* IF (tRPmin is higher than tREAmax) */
        if (pDevice->pNFDInfo->tRPmin >= pDevice->pNFDInfo->tREAmax)
        {
            /* update MaxVal as tRPmin */
            MaxVal = pDevice->pNFDInfo->tRPmin;
        }
        /* ELSE (tREAmax must be higher than tRPmin) */
        else
        {
            /* update MaxVal as tREAmax */
            MaxVal = pDevice->pNFDInfo->tREAmax;
        }

        /* IF (SCLK is the greatest of all) */
        if ((SclkPeriod >= pDevice->pNFDInfo->tCEAmax) &&
            (SclkPeriod >= MaxVal))
        {
            /* update MaxVal */
            MaxVal = SclkPeriod;
        }
        /* IF (tCEAmax is the greates of all) */
        else if (MaxVal < (pDevice->pNFDInfo->tCEAmax - SclkPeriod))
        {
            /* update MaxVal */
            MaxVal = (pDevice->pNFDInfo->tCEAmax-SclkPeriod);
        }

        /* calculate RD_DLY in terms of SCLK cycles */
        u8Temp = MaxVal/SclkPeriod;
        /* round off RD_DLY */
        if (MaxVal%SclkPeriod)
        {
            u8Temp++;
        }

        /* Configure NFC Control register with calculated Read Strobe delay (RD_DLY) */
        ADI_NFC_CTRL_RD_DLY_SET(u8Temp);

        /* Calculate NFD Data area size in bytes */
        DeviceSize = (DataAreaInBytes * pDevice->pNFDInfo->PagesPerBlock * pDevice->pNFDInfo->TotalBlocks);

        /*** Calculate Total Number of Address cycles required to access NFD ***/
        /* IF (device size is <= 64MB) */
        if (DeviceSize < ADI_NFD_SIZE_64MB)
        {
            pDevice->NFCAccess.NumAddressCycles = 3;
        }
        /* ELSE IF (device size is between 64MB and 128MB) */
        else if (DeviceSize <= ADI_NFD_SIZE_128MB)
        {
            pDevice->NFCAccess.NumAddressCycles = 4;
        }
        /* ELSE (device size must be more than 128MB) */
        else
        {
            pDevice->NFCAccess.NumAddressCycles = 5;
        }

        /* calculate # spare units per NFD Data packet */
        pDevice->NFCAccess.SpareUnitsPerPkt = ((pDevice->pNFDInfo->PageSize - pDevice->pNFDInfo->DataAreaPerPage) /\
                                               (DataAreaInBytes / pDevice->NFCAccess.NFDDataPktSize));
    }

    /* return */
    return(Result);
}

/*********************************************************************

    Function:       adi_nfc_ResetNFCAccess

    Description:    Reset NFC registers and access parameters

*********************************************************************/
static void adi_nfc_ResetNFCAccess (
    ADI_NFC_DEF     *pDevice    /* pointer to NFC device we're working on */
){
    /* Disable all NFC IRQ status interrupts */
    ADI_NFC_IRQMASK_DISABLE_ALL;
    /* Reset NFC Registers */
    ADI_NFC_RESET_REGS;

    /* Reset NFC access parameters */
    pDevice->NFCAccess.ReadFlag         = 1;    /* read NFD                 */
    pDevice->NFCAccess.BlockAddress     = 0;    /* Block Address            */
    pDevice->NFCAccess.PageAddress      = 0;    /* Page Address             */
    pDevice->NFCAccess.ColumnAddress    = 0;    /* Column Address           */
    pDevice->NFCAccess.RemainDescriptors= 0;    /* # descriptors to process */
    pDevice->pNFCBuffer                 = NULL; /* Client 1D buffer         */

    /* No status flags to wait for */
    pDevice->NFCAccess.NFCWaitStatus    = ADI_NFC_WAIT_STATUS_NONE;
    /* NFC Operating state = idle */
    pDevice->NFCAccess.NFCXferStatus    = ADI_NFC_STATE_IDLE;

    /* return */
}

/*********************************************************************

    Function:       adi_nfc_UpdateNFDAddress

    Description:    Updates NFD Address fields in NFC Access structure

*********************************************************************/
static void adi_nfc_UpdateNFDAddress (
    ADI_NFC_DEF     *pDevice,           /* pointer to NFC device we're working on   */
    u32             UpdateCount,        /* #bytes or #sectors to update             */
    u32             MaxColumnAddress    /* Maximum Column Address range             */
){

    /* IF (x16 type NFD) */
    if ((pDevice->pNFDInfo->NFDType == ADI_NFD_SMALL_PAGE_x16) ||
        (pDevice->pNFDInfo->NFDType == ADI_NFD_LARGE_PAGE_x16))
    {
        UpdateCount /= 2; /* convert update count to words */
    }

    /* IF (Update count + present column address equals Maximum Column address limit) */
    if ((UpdateCount + pDevice->NFCAccess.ColumnAddress) >= MaxColumnAddress)
    {
        /* increment Page address */
        pDevice->NFCAccess.PageAddress++;
        /* reset column address */
        pDevice->NFCAccess.ColumnAddress = 0;

        /* IF (page address exceeds number of pages per block) */
        if (pDevice->NFCAccess.PageAddress >= pDevice->pNFDInfo->PagesPerBlock)
        {
            /* increment Block address */
            pDevice->NFCAccess.BlockAddress++;
            /* reset page and column address */
            pDevice->NFCAccess.PageAddress = 0;
            pDevice->NFCAccess.ColumnAddress = 0;
        }
    }
    /* ELSE (Update count + present column address lies within page size limit) */
    else
    {
        /* increment column address as per update count value */
        pDevice->NFCAccess.ColumnAddress += UpdateCount;
    }

    /* return */
    return;
}

/*********************************************************************

    Function:       adi_nfc_InsertAddress

    Description:    Translates Block/Column/Page address to
                    suitable format in order to access
                    NAND flash Device Memory and inserts
                    the calculated address to NFC address register

*********************************************************************/
static u32 adi_nfc_InsertAddress (
    ADI_NFC_DEF         *pDevice,   /* pointer to NFC device we're working on   */
    ADI_NFD_ADDR_TYPE   AddressType /* Address type to insert                   */
){
    u8  AddressCycles;
    u16 NFDAddress[5] = {0};
    u16 *pNFDAddress;
    u32 Result= ADI_DEV_RESULT_SUCCESS;

    /* Validate the NFD address to access */
    if ((Result = adi_nfc_ValidateNFDAddress(pDevice)) == ADI_DEV_RESULT_SUCCESS)
    {
        /* calculate NFD address A7 to A0 */
        NFDAddress[0]   = (pDevice->NFCAccess.ColumnAddress & 0xFF);
        /* calculate NFD address A8 to A11 */
        NFDAddress[1]   = ((pDevice->NFCAccess.ColumnAddress >> 8) & 0x0F);
        /* calculate NFD address A12 to A19 */
        NFDAddress[2]   = (((pDevice->NFCAccess.BlockAddress << 6) & 0xC0) |\
                            (pDevice->NFCAccess.PageAddress & 0x3F));
        /* calculate NFD address A20 to A27 */
        NFDAddress[3]   = ((pDevice->NFCAccess.BlockAddress >> 2) & 0xFF);
        /* calculate NFD address A28 to A31 */
        NFDAddress[4]   = ((pDevice->NFCAccess.BlockAddress >> 10) & 0x07);

        /* IF (Access a NFD Page) */
        if (AddressType == ADI_NFD_ADDR_TYPE_ACCESS_PAGE)
        {
            pNFDAddress     = &NFDAddress[0];
            AddressCycles   = pDevice->NFCAccess.NumAddressCycles;
        }
        /* ELSE (Access a NFD page in random) */
        else if (AddressType == ADI_NFD_ADDR_TYPE_ACCESS_RANDOM)
        {
            pNFDAddress     = &NFDAddress[0];
            AddressCycles   = 2;
        }
        /* ELSE (Erase NFD Block) */
        else
        {
            pNFDAddress     = &NFDAddress[2];
            AddressCycles   = (pDevice->NFCAccess.NumAddressCycles - 2);
        }

        while(AddressCycles--)
        {
            /* Insert NFC address to access */
            *pADI_NFC_ADDR = *pNFDAddress;
            /* move to next address chunk in the array */
            pNFDAddress++;
            /* IF (write buffer is full) */
            if (ADI_NFC_STATUS_WB_FULL_STAT)
            {
                /* wait until Write buffer becomes empty */
                if (Result = adi_nfc_WaitOnStatus(pDevice,ADI_NFC_WAIT_FOR_WRITE_BUF_EMPTY))
                {
                    /* exit on error */
                    break;
                }
            }
        }
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_nfc_DirectAccessNFD

    Description:    Reads #bytes/#words from NFD starting from
                    the given Block/Page/Column address.

                    Writes #bytes/#words to NFD starting from
                    the given Block/Page/Column address.

*********************************************************************/
static u32 adi_nfc_DirectAccessNFD (
    ADI_NFC_DEF             *pDevice,   /* pointer to NFC device we're working on           */
    ADI_NFD_DIRECT_ACCESS   *pAccess,   /* pointer to NFD direct access request structure   */
    u8                      ReadFlag    /* 1 = Read from NFD, 0 = Write to NFD              */
){
    u8  *pu8Data;
    u16 *pu16Data;
    u32 NumUnitsRemainToAccess,NumUnitsRemainInPage,AddressUpdateCount,AccessCount;

    /* default return code */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* Pointer to client data */
    pu8Data  = (u8*)pAccess->pData;
    pu16Data = (u16*)pAccess->pData;

    /* Number of units remaining to access */
    NumUnitsRemainToAccess = pAccess->AccessCount;
    /* save the given NFD address to access */
    pDevice->NFCAccess.ColumnAddress    = pAccess->ColumnAddress;
    pDevice->NFCAccess.PageAddress      = pAccess->PageAddress;
    pDevice->NFCAccess.BlockAddress     = pAccess->BlockAddress;

    /* Access NFD */
    do
    {
        /* Number Units left in this page */
        NumUnitsRemainInPage   = (pDevice->pNFDInfo->PageSize - pDevice->NFCAccess.ColumnAddress);

        /* IF (Number of units to read is more than Number of units left in this page) */
        if (NumUnitsRemainToAccess > NumUnitsRemainInPage)
        {
            /* count of num units to access */
            AccessCount = NumUnitsRemainInPage;
        }
        /* ELSE (Number of units to read is less than Number of units left in this page) */
        else
        {
            /* count of num units to access */
            AccessCount = NumUnitsRemainToAccess;
        }

        /* NFD Address update count */
        AddressUpdateCount = AccessCount;

        /* IF (any units left to access) */
        if (AccessCount)
        {
            /* IF (read from NFD) */
            if (ReadFlag)
            {
                /* Kick-off Page Read (Don't use DMA) */
                if (Result = adi_nfc_KickOffPageAccess (pDevice,ADI_NFC_NO_DMA,ADI_NFC_READ_NFD))
                {
                    /* exit on error */
                    break;
                }

                /* IF (x16 type NFD) */
                if ((pDevice->pNFDInfo->NFDType == ADI_NFD_SMALL_PAGE_x16) ||
                    (pDevice->pNFDInfo->NFDType == ADI_NFD_LARGE_PAGE_x16))
                {
                    /* read 'AccessCount' number of units from NFD */
                    while (AccessCount)
                    {
                        /* read a word */
                        if (Result = adi_nfc_ReadSingleUnit(pDevice,pu16Data))
                        {
                            /* exit on error */
                            break;
                        }
                        pu16Data++;
                        AccessCount--;
                    }
                }
                /* ELSE (x8 type NFD) */
                else
                {
                    /* read 'AccessCount' number of units from NFD */
                    while (AccessCount)
                    {
                        /* read a byte */
                        if (Result = adi_nfc_ReadSingleUnit(pDevice,pu8Data))
                        {
                            /* exit on error */
                            break;
                        }
                        pu8Data++;
                        AccessCount--;
                    }
                }
            }
            /* ELSE (write to NFD) */
            else
            {
                /* Kick-off Page Write (Don't use DMA) */
                if (Result = adi_nfc_KickOffPageAccess (pDevice,ADI_NFC_NO_DMA,ADI_NFC_WRITE_NFD))
                {
                    /* exit on error */
                    break;
                }

                /* IF (x16 type NFD) */
                if ((pDevice->pNFDInfo->NFDType == ADI_NFD_SMALL_PAGE_x16) ||
                    (pDevice->pNFDInfo->NFDType == ADI_NFD_LARGE_PAGE_x16))
                {
                    /* write 'AccessCount' number of units from NFD */
                    while (AccessCount)
                    {
                        /* write a word */
                        if (Result = adi_nfc_WriteSingleUnit(pDevice,pu16Data))
                        {
                            /* exit on error */
                            break;
                        }
                        pu16Data++;
                        AccessCount--;
                    }
                }
                /* ELSE (x8 type NFD) */
                else
                {
                    /* write 'AccessCount' number of units from NFD */
                    while (AccessCount)
                    {
                        /* write a byte */
                        if (Result = adi_nfc_WriteSingleUnit(pDevice,pu8Data))
                        {
                            /* exit on error */
                            break;
                        }
                        pu8Data++;
                        AccessCount--;
                    }
                }

                /* IF (all data units were sent to NFD) */
                if (AccessCount == 0)
                {
                    /* Confirm Page Program */
                    ADI_NFC_INSERT_CMD(ADI_NFD_CMD_PAGE_PROGRAM_CONFIRM);
                }

                /* wait until NFD completes updating this page */
                if (Result = adi_nfc_WaitOnStatus(pDevice,ADI_NFC_WAIT_FOR_NBUSYIRQ))
                {
                    /* exit on error */
                    break;
                }
            }
        }

        /* IF (NFD Access resulted in success) */
        if ((Result == ADI_DEV_RESULT_SUCCESS) && (AccessCount == 0))
        {
            /* update number of bytes remaining to access */
            NumUnitsRemainToAccess -= AddressUpdateCount;
            /* update NFD Address field in NFC access structure */
            adi_nfc_UpdateNFDAddress(pDevice,AddressUpdateCount,pDevice->pNFDInfo->PageSize);
        }
        else
        {
            /* exit on error */
            break;
        }
    }while(NumUnitsRemainToAccess); /* access # units requested by the client */

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_nfc_ReadSingleUnit

    Description:    Reads 1 byte or 1 word from NFD

*********************************************************************/
static u32 adi_nfc_ReadSingleUnit(
    ADI_NFC_DEF *pDevice,       /* pointer to NFC device we're working on   */
    void        *pData          /* Location to store the data read from NFD */
){
    /* default return code */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* Trigger a read request */
    ADI_NFC_TRIGGER_DATA_READ;

    /* wait until Read data is ready */
    Result = adi_nfc_WaitOnStatus(pDevice,ADI_NFC_WAIT_FOR_READ_DATA_READY);

    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
        /* IF (x16 type NFD) */
        if ((pDevice->pNFDInfo->NFDType == ADI_NFD_SMALL_PAGE_x16) ||
            (pDevice->pNFDInfo->NFDType == ADI_NFD_LARGE_PAGE_x16))
        {
            /* get data from NFC Read register */
            *(u16*)pData = *pADI_NFC_READ_DATA;
        }
        /* ELSE (x8 type NFD) */
        else
        {
            /* get data from NFC Read register */
            *(u8*)pData = *pADI_NFC_READ_DATA;
        }
    }

    /* clear Read Data Ready flag */
    ADI_NFC_IRQSTAT_CLEAR_FLAGS(ADI_NFC_WAIT_FOR_READ_DATA_READY);

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_nfc_WriteSingleUnit

    Description:    Writes 1 byte or 1 word to NFD

*********************************************************************/
static u32 adi_nfc_WriteSingleUnit(
    ADI_NFC_DEF *pDevice,       /* pointer to NFC device we're working on               */
    void        *pData          /* pointer to location holding Data to be sent to NFD   */
){
    /* default return code */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* IF (x16 type NFD) */
    if ((pDevice->pNFDInfo->NFDType == ADI_NFD_SMALL_PAGE_x16) ||
        (pDevice->pNFDInfo->NFDType == ADI_NFD_LARGE_PAGE_x16))
    {
        /* write 1 word to NFD */
        *pADI_NFC_WRITE_DATA = *((u16*)pData);
    }
    /* ELSE (x8 type NFD) */
    else
    {
        /* write 1 byte to NFD */
        *pADI_NFC_WRITE_DATA = *((u8*)pData);
    }

    /* IF (write buffer is full) */
    if (ADI_NFC_STATUS_WB_FULL_STAT)
    {
        /* wait until Write buffer becomes empty */
        Result = adi_nfc_WaitOnStatus(pDevice,ADI_NFC_WAIT_FOR_WRITE_BUF_EMPTY);
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_nfc_WaitOnStatus

    Description:    Waits until the given NFC IRQ status bits gets set

*********************************************************************/
static u32 adi_nfc_WaitOnStatus (
    ADI_NFC_DEF                 *pDevice,   /* pointer to NFC device instance we're working on  */
    u16                         StatusFlag  /* NFC Status bits to look for                      */
){
    /* return code */
    u32 Result= ADI_DEV_RESULT_SUCCESS;

    /* acquire NFC Data semaphore */
    if ((Result = adi_sem_Pend (pDevice->DataSemaphoreHandle, ADI_SEM_TIMEOUT_FOREVER)) == (u32)ADI_SEM_RESULT_SUCCESS)
    {
        /* Update NFC Operating state as Direct Access in progress */
        pDevice->NFCAccess.NFCXferStatus = ADI_NFC_STATE_DIRECT_ACCESS_IN_PROGRESS;
        /* wait for selected status interrupt(s) */
        pDevice->NFCAccess.NFCWaitStatus = StatusFlag;
        /* enable IRQ status interrupt(s) */
        ADI_NFC_IRQMASK_ENABLE(StatusFlag);

        /* wait until selected status flag(s) gets set */
        if ((Result = adi_sem_Pend (pDevice->DataSemaphoreHandle, ADI_SEM_TIMEOUT_FOREVER)) == (u32)ADI_SEM_RESULT_SUCCESS)
        {
            /* release NFC data semaphore */
            adi_sem_Post (pDevice->DataSemaphoreHandle);
        }
        else
        {
            /* disable interrupt for selected status flag(s) */
            ADI_NFC_IRQMASK_DISABLE(pDevice->NFCAccess.NFCWaitStatus);
        }
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_nfc_InterruptHandler

    Description:    Handles NFC status interrupts

*********************************************************************/
static ADI_INT_HANDLER(adi_nfc_InterruptHandler)    /* NFC Interrupt handler */
{
    /* Return code */
    ADI_INT_HANDLER_RESULT  Result;
    /* pointer to the device that caused this callback */
    ADI_NFC_DEF *pDevice = (ADI_NFC_DEF*)ClientArg;

    /* assume the interrupt is not for us */
    Result  = ADI_INT_RESULT_NOT_PROCESSED;

    /* IF (this interrupt is for us) */
    if ((*pADI_NFC_IRQSTAT & pDevice->NFCAccess.NFCWaitStatus) == pDevice->NFCAccess.NFCWaitStatus)
    {
        /* update NFC Operating state as Device Data transfer done */
        pDevice->NFCAccess.NFCXferStatus |= ADI_NFC_STATE_DEV_DATA_XFER_DONE;
        /* Clear Status IRQ flag(s) that generated this interrupt */
        ADI_NFC_IRQSTAT_CLEAR_FLAGS(pDevice->NFCAccess.NFCWaitStatus);
        /* disable interrupt for selected status flag(s) */
        ADI_NFC_IRQMASK_DISABLE(pDevice->NFCAccess.NFCWaitStatus);
        /* Clear NFC Wait status */
        pDevice->NFCAccess.NFCWaitStatus = ADI_NFC_WAIT_STATUS_NONE;
        /* process this interrupt */
        adi_nfc_ProcessInterrupt(pDevice);
        /* update return code as we've processed this interrupt */
        Result = ADI_INT_RESULT_PROCESSED;
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       adi_nfc_DMACallback

    Description:    Callback function for NFC DMA events.
                    Note that this function never has to check for deferred
                    callbacks but can always call the client directly.  The
                    reason for this is that the DMA Manager is passed the
                    callback service handle so when this routine gets invoked
                    we're already deferred.

*********************************************************************/
static void adi_nfc_DMACallback(
    void    *DeviceHandle,  /* Handle to the device that generated callback */
    u32     CallbackEvent,  /* DMA Callback event                           */
    void    *pArg           /* Callback argument                            */
){
    /* Device instance that generated this callback */
    ADI_NFC_DEF *pDevice = (ADI_NFC_DEF*)DeviceHandle;
    /* DMA callback event */
    ADI_DMA_EVENT  Event = (ADI_DMA_EVENT)CallbackEvent;

/* Debug Build only - Check for errors */
#if defined(ADI_DEV_DEBUG)
    /* Validate Device Handle */
    if (pDevice != NULL)
    {
#endif

        /* CASEOF (event type) */
        switch (Event)
        {
            /* CASE (descriptor was processed) */
            case ADI_DMA_EVENT_DESCRIPTOR_PROCESSED:

                /* pointer to DMA descriptor that generated this data completion interrupt */
                pDevice->NFCAccess.pProcessDescriptor = (ADI_DMA_DESCRIPTOR_LARGE *)pArg;

                /* IF (read from NFD and write to SDRAM) */
                if ((pDevice->NFCAccess.pProcessDescriptor)->Config.b_WNR == ADI_DMA_WNR_WRITE)
                {
                    /* Read Data complete - no need to wait for Device interrupt */
                    /* update NFC Operating state as DMA Data transfer complete */
                    pDevice->NFCAccess.NFCXferStatus = ADI_NFC_STATE_DATA_XFER_DONE;
                }
                /* ELSE (read from SDRAM and write to NFD) */
                else
                {
                    /* update NFC Operating state as DMA transfer done */
                    pDevice->NFCAccess.NFCXferStatus |= ADI_NFC_STATE_DMA_DATA_XFER_DONE;
                    pDevice->NFCAccess.NFCWaitStatus |= ADI_NFC_WAIT_FOR_PAGE_WRITE_DONE;
                    /* Enable page write done interrupt */
                    ADI_NFC_IRQMASK_PG_WR_DONE_ENABLE;
                }

                /* process this interrupt */
                adi_nfc_ProcessInterrupt(pDevice);

                break;

            /* CASE (DMA error) */
            case ADI_DMA_EVENT_ERROR_INTERRUPT:

                break;

            /* default */
            default:
                break;
        }

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
}

/*********************************************************************

    Function:       adi_nfc_ProcessInterrupt

    Description:    Processes NFC Device and DMA interrupts

*********************************************************************/
static void adi_nfc_ProcessInterrupt (
    ADI_NFC_DEF         *pDevice    /* pointer to NFC device we're working on   */
){
    u16 DescriptorElementCount;

    /* IF (DMA error) */
    if (pDevice->NFCAccess.NFCXferStatus > ADI_NFC_STATE_DATA_XFER_DONE)
    {

    }
    /* ELSE IF (Direct Access complete) */
    else if (pDevice->NFCAccess.NFCXferStatus == ADI_NFC_STATE_DIRECT_ACCESS_DONE)
    {
        /* release NFC Data semaphore */
        adi_sem_Post(pDevice->DataSemaphoreHandle);
        /* Set NFC operating state as idle */
        pDevice->NFCAccess.NFCXferStatus = ADI_NFC_STATE_IDLE;
    }
    /* ELSE IF (DMA Data transfer complete) */
    else if (pDevice->NFCAccess.NFCXferStatus == ADI_NFC_STATE_DATA_XFER_DONE)
    {
        /* Element count of the Descriptor that is processed */
        DescriptorElementCount = ((pDevice->NFCAccess.pProcessDescriptor)->XCount *\
                                  (pDevice->NFCAccess.pProcessDescriptor)->XModify);

        /* save ECC0 & ECC1 */
        *(pDevice->NFCAccess.pECCArray) = *pADI_NFC_ECC0;
        pDevice->NFCAccess.pECCArray++;
        *(pDevice->NFCAccess.pECCArray) = *pADI_NFC_ECC1;
        pDevice->NFCAccess.pECCArray++;

        /* IF (512-byte NFC Access) */
        if (DescriptorElementCount == 512)
        {
            /* save ECC2 & ECC3 */
            *(pDevice->NFCAccess.pECCArray) = *pADI_NFC_ECC2;
            pDevice->NFCAccess.pECCArray++;
            *(pDevice->NFCAccess.pECCArray) = *pADI_NFC_ECC3;
            pDevice->NFCAccess.pECCArray++;
        }

        /* reset ECC registers */
        ADI_NFC_RESET_REGS;

        /* IF (we're accessing NFD spare area) */
        if (pDevice->NFCAccess.ColumnAddress % 256U)
        {
            /* do nothing */
        }
        /* ELSE (we're accessing NFD data area) */
        else
        {
            /* update processed element count */
            pDevice->pNFCBuffer->ProcessedElementCount += DescriptorElementCount;
        }

        /* decrement number of descriptors left to process */
        pDevice->NFCAccess.RemainDescriptors--;

        /* IF (we have more descriptors to process) */
        if (pDevice->NFCAccess.RemainDescriptors)
        {
            /* check element count of next descriptor in chain */
            /* IF (512-bytes page access) */
            if (((pDevice->NFCAccess.pProcessDescriptor)->pNext->XCount *
                 (pDevice->NFCAccess.pProcessDescriptor)->pNext->XModify) == 512)
            {
                /* Configure NFC Control register for 512-bytes page access */
                ADI_NFC_CTRL_PG_SIZE_SET_512BYTES;
            }
            /* ELSE (256-bytes page access) */
            else
            {
                /* Configure NFC Control register for 256-bytes page access */
                ADI_NFC_CTRL_PG_SIZE_SET_256BYTES;
            }

            /* Set NFC operating state as data transfer in progress */
            pDevice->NFCAccess.NFCXferStatus = ADI_NFC_STATE_DATA_XFER_IN_PROGRESS;

            /* IF (read from NFD and write to SDRAM) */
            if ((pDevice->NFCAccess.pProcessDescriptor)->pNext->Config.b_WNR == ADI_DMA_WNR_WRITE)
            {
                /* start NFD page read */
                ADI_NFC_PGCTL_PAGE_READ_START;
            }
            /* ELSE (read from SDRAM and write to NFD) */
            else
            {
                /* start page write */
                ADI_NFC_PGCTL_PAGE_WRITE_START;
            }
        }
        /* ELSE (completed processing all descriptors in chain) */
        else
        {
            /* release NFC Data semaphore */
            adi_sem_Post(pDevice->DataSemaphoreHandle);
            /* Set NFC operating state as idle */
            pDevice->NFCAccess.NFCXferStatus = ADI_NFC_STATE_IDLE;
        }
    }

    /* return */
}

/*********************************************************************

    Function:       adi_nfc_ValidateNFDAddress

    Description:    Validates the given NFD Block/Page/Column address.

*********************************************************************/
static u32 adi_nfc_ValidateNFDAddress(
    ADI_NFC_DEF         *pDevice    /* pointer to NFC device instance we're working on */
){
    /* assume that the given address is invalid */
    u32 Result= ADI_NFC_RESULT_NFD_ADDRESS_OUT_OF_BOUNDARY;

    /* IF (Block address is valid) */
    if (pDevice->NFCAccess.BlockAddress < pDevice->pNFDInfo->TotalBlocks)
    {
        /* IF (Page address is valid) */
        if (pDevice->NFCAccess.PageAddress < pDevice->pNFDInfo->PagesPerBlock)
        {
            /* IF (Column address is valid) */
            if (pDevice->NFCAccess.ColumnAddress < pDevice->pNFDInfo->PageSize)
            {
                /* return success */
                Result = ADI_DEV_RESULT_SUCCESS;
            }
        }
    }

    /* return */
    return (Result);
}

/***** Static functions for Moab class devices *****/

#if defined(__ADSP_MOAB__)
/*********************************************************************

    Function:       adi_nfc_SetPortControl

    Description:    Configures the general purpose ports of BF54x
                    class devices for NFC usage.

*********************************************************************/
static u32 adi_nfc_SetPortControl(
    ADI_NFC_DEF     *pDevice    /* pointer to NFC device we're working on */
){
     /* Directives to enable NFC CE and RB pins */
    ADI_PORTS_DIRECTIVE Directives [] =
    {
        ADI_PORTS_DIRECTIVE_NAND_CE,    /* NFC Chip Enable  */
        ADI_PORTS_DIRECTIVE_NAND_RB     /* NFC Read Busy    */
    };

    /* call port control to enable NFC pins */
    return (adi_ports_Configure(&Directives[0], (sizeof(Directives)/sizeof(Directives[0]))));
}

#endif

/***** Static functions for Kookaburra and Mokingbird class devices *****/
#if defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)
/*********************************************************************

    Function:       adi_nfc_SetPortControl

    Description:    Configures the general purpose ports of BF52x
                    class devices for NFC usage.

*********************************************************************/
static u32 adi_nfc_SetPortControl(
    ADI_NFC_DEF     *pDevice    /* pointer to NFC device we're working on */
){
    /* default return code */
    u32 Result = ADI_PORTS_RESULT_SUCCESS;

    /* Directives common for Device 0 & Device 1 */
    ADI_PORTS_DIRECTIVE CommonDirectives [] =
    {
        ADI_PORTS_DIRECTIVE_NAND_CE,    /* NFC Chip Enable          */
        ADI_PORTS_DIRECTIVE_NAND_WEB,   /* NFC Write Enable         */
        ADI_PORTS_DIRECTIVE_NAND_REB,   /* NFC Read Enable          */
        ADI_PORTS_DIRECTIVE_NAND_BUSY,  /* NFC Busy                 */
        ADI_PORTS_DIRECTIVE_NAND_CLE,   /* NFC Command Latch Enable */
        ADI_PORTS_DIRECTIVE_NAND_ALE    /* NFC Address Latch Enable */
    };

    /* Device 0 specific Directives */
    ADI_PORTS_DIRECTIVE Dev0Directives [] =
    {
        ADI_PORTS_DIRECTIVE_NAND_HD0,   /* NFC Port H Data Bit 0    */
        ADI_PORTS_DIRECTIVE_NAND_HD1,   /* NFC Port H Data Bit 1    */
        ADI_PORTS_DIRECTIVE_NAND_HD2,   /* NFC Port H Data Bit 2    */
        ADI_PORTS_DIRECTIVE_NAND_HD3,   /* NFC Port H Data Bit 3    */
        ADI_PORTS_DIRECTIVE_NAND_HD4,   /* NFC Port H Data Bit 4    */
        ADI_PORTS_DIRECTIVE_NAND_HD5,   /* NFC Port H Data Bit 5    */
        ADI_PORTS_DIRECTIVE_NAND_HD6,   /* NFC Port H Data Bit 6    */
        ADI_PORTS_DIRECTIVE_NAND_HD7    /* NFC Port H Data Bit 7    */
    };

    /* Device 1 specific Directives */
    ADI_PORTS_DIRECTIVE Dev1Directives [] =
    {
        ADI_PORTS_DIRECTIVE_NAND_FD0,   /* NFC Port F Data Bit 0    */
        ADI_PORTS_DIRECTIVE_NAND_FD1,   /* NFC Port F Data Bit 1    */
        ADI_PORTS_DIRECTIVE_NAND_FD2,   /* NFC Port F Data Bit 2    */
        ADI_PORTS_DIRECTIVE_NAND_FD3,   /* NFC Port F Data Bit 3    */
        ADI_PORTS_DIRECTIVE_NAND_FD4,   /* NFC Port F Data Bit 4    */
        ADI_PORTS_DIRECTIVE_NAND_FD5,   /* NFC Port F Data Bit 5    */
        ADI_PORTS_DIRECTIVE_NAND_FD6,   /* NFC Port F Data Bit 6    */
        ADI_PORTS_DIRECTIVE_NAND_FD7    /* NFC Port F Data Bit 7    */
    };

    /* call port control to enable NFC pins common for both devices */
    Result = adi_ports_Configure(&CommonDirectives[0], (sizeof(CommonDirectives)/sizeof(CommonDirectives[0])));

    if (Result == ADI_PORTS_RESULT_SUCCESS)
    {
        /* IF (Device 0) */
        if (pDevice == &NFCDevice[0])
        {
            /* call port control to enable NFC pins specific to device 0 */
            Result = adi_ports_Configure(&Dev0Directives[0], (sizeof(Dev0Directives)/sizeof(Dev0Directives[0])));
        }
        /* ELSE (Device 1) */
        else
        {
            /* call port control to enable NFC pins specific to device 1 */
            Result = adi_ports_Configure(&Dev1Directives[0], (sizeof(Dev1Directives)/sizeof(Dev1Directives[0])));
        }
    }

    /* return */
    return (Result);
}

#endif

/*****/

/*****************************************************************************
Copyright (c), 2002-2009 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

$Revision: 2095 $
$Date: 2010-02-18 17:08:05 -0500 (Thu, 18 Feb 2010) $

Title: AD7879 Touchscreen Controller Driver

Description: This is the primary source file for AD7879 Touchscreen
             Controller Driver. The driver supports Device access commands
             to access AD7879 registers.

Note:        The code in this driver file supports both the AD7879 and the
             AD7879-1, which differ only in the interface they offer for
             accessing their internal registers (SPI and TWI respectively).

             For the AD7879 just add this file to your project as normal.
             For the AD7879-1 add the file drivers/touchscreen/adi_ad7879_1.c
             to your project instead: it defines a preprocessor identifier
             and then includes this file.  This file uses the presence or 
             absence of the identifier to select between alternative code in
             a few places.

*****************************************************************************/

/*=============  I N C L U D E S   =============*/

/* system service includes */
#include <services/services.h>
/* device manager includes */
#include <drivers/adi_dev.h>

#ifdef ADI_IMPLEMENT_AD7879_1

/* device access service (TWI/SPI-based register access) */
#include <drivers/deviceaccess/adi_device_access.h>
/* AD7879 driver include */
#include <drivers/touchscreen/adi_ad7879_1.h>

#else

/* SPI driver include */
#include <drivers/spi/adi_spi.h>
/* AD7879 driver include */
#include <drivers/touchscreen/adi_ad7879.h>

#endif


/*=============  D E F I N E S  =============*/

/*
** ADI_AD7879_NUM_DEVICES
**  - Number of AD7879 devices in the system
*/
#define ADI_AD7879_NUM_DEVICES                  (sizeof(_gaoAd7879Device)/\
                                                 sizeof(ADI_DD_AD7879_DEF))

#ifdef ADI_IMPLEMENT_AD7879_1

/*
** AD7879_END_REG_ADDR
**  - Address of last register in AD7879
*/
#define AD7879_END_REG_ADDR                     AD7879_REG_REF_ID

#else

/*
** ADI_AD7879_BITS_PER_REGISTER
**  - Bits per Resiter for AD7879 registers
*/
#define ADI_AD7879_BITS_PER_REGISTER            16U

/*
** ADI_AD7879_SPI_BAUD
**  - SPI Baud rate to meet all possible SCLK values
*/
#define ADI_AD7879_SPI_BAUD                     0x1FFU

/*
**
** AD7879 hardware register address
**
*/

/*
** ADI_AD7879_END_REG_ADDR
**  - Address of last register in AD7879
*/
#define ADI_AD7879_END_REG_ADDR                 AD7879_REG_REF_ID
/*
** ADI_AD7879_RO_REG_START_ADDR
**  - Address of first Read-only Type Register in AD7879
*/
#define ADI_AD7879_RO_REG_START_ADDR            AD7879_REG_XPOS
/*
** ADI_AD7879_RO_REG_END_ADDR
**  - Address of last Read-only Type Register in AD7879
*/
#define ADI_AD7879_RO_REG_END_ADDR              AD7879_REG_REF_ID
/*
** ADI_AD7879_INVALID_REG1
**  - Invalid registers in AD7879
*/
#define ADI_AD7879_INVALID_REG1                 0U

#endif

/*
**
** Shift count and Masks to access AD7879 hardware register fields
**
*/

/*
** ADI_AD7879_ADCMODE_FIELD_SHIFT
**  - Shift count to reach ADC MODE field in Control Register 1
*/
#define ADI_AD7879_ADCMODE_FIELD_SHIFT          10U
/*
** ADI_AD7879_REG3_XYZ_SHIFT
**  - Shift count to reach X/Y/Z1/Z2 bits in Control Register 3
*/
#define ADI_AD7879_REG3_XYZ_SHIFT               4U
/*
** ADI_AD7879_REG_VALUE_MASK
**  - Masks AD7879 Register data Mask
*/
#define ADI_AD7879_REG_VALUE_MASK               0xFFFFU
/*
** ADI_AD7879_CTRL_REG1_PENIRQ_MASK
**  - Masks AD7879 Control Register 1 PENIRQ field Mask
*/
#define ADI_AD7879_CTRL_REG1_PENIRQ_MASK        0x8000U
/*
** ADI_AD7879_CTRL_REG1_TMR_MASK
**  - Masks AD7879 Control Register 1 Timer field Mask
*/
#define ADI_AD7879_CTRL_REG1_TMR_MASK           0x00FFU
/*
** ADI_AD7879_CTRL_REG1_MODE_MASK
**  - Masks AD7879 Control Register 1 ADC Mode field Mask
*/
#define ADI_AD7879_CTRL_REG1_MODE_MASK          0x0C00U
/*
** ADI_AD7879_CTRL_REG1_CHADD_MASK
**  - Masks AD7879 Control Register 1 CHADD field Mask
*/
#define ADI_AD7879_CTRL_REG1_CHADD_MASK         0x7000U
/*
** ADI_AD7879_CTRL_REG3_INT_MODE_MASK
**  - Masks AD7879 Control Register 3 INT MODE field Mask
*/
#define ADI_AD7879_CTRL_REG3_INT_MODE_MASK      0x2000U
/*
** ADI_AD7879_CTRL_REG3_SEQ_MASK
**  - Masks AD7879 Control Register 3 SEQUENCER field Mask
*/
#define ADI_AD7879_CTRL_REG3_SEQ_MASK           0x00F0U
/*
** ADI_AD7879_CTRL_REG1_FIELD_MASK
**  - Masks first bit of each field in AD7879 Control Register 1
*/
#define ADI_AD7879_CTRL_REG1_FIELD_MASK         0x9501U
/*
** ADI_AD7879_CTRL_REG2_FIELD_MASK
**  - Masks first bit of each field in AD7879 Control Register 2
*/
#define ADI_AD7879_CTRL_REG2_FIELD_MASK         0x46B1U
/*
** ADI_AD7879_CTRL_REG3_FIELD_MASK
**  - Masks first bit of each field in AD7879 Control Register 3
*/
#define ADI_AD7879_CTRL_REG3_FIELD_MASK         0xFFFEU
/*
** ADI_AD7879_DEFAULT_FIELD_MASK
**  - Default field mask for AD7879 registers
*/
#define ADI_AD7879_DEFAULT_FIELD_MASK           0xFFFFU

/*
**
** AD7879 Conversion modes
**
*/

/*
** ADI_AD7879_SINGLE_CONVERSION
**  - AD7879 is in Single Conversion mode
*/
#define ADI_AD7879_SINGLE_CONVERSION            1U
/*
** ADI_AD7879_REPEAT_CONVERSION
**  - AD7879 is in Repeat Conversion mode
*/
#define ADI_AD7879_REPEAT_CONVERSION            2U

/*
**
** AD7879 Register access modes
**
*/

/*
** ADI_AD7879_REGISTER_READ
**  - AD7879 register read request
*/
#define ADI_AD7879_REGISTER_READ                0xE400U
/*
** ADI_AD7879_REGISTER_WRITE
**  - AD7879 register write request
*/
#define ADI_AD7879_REGISTER_WRITE               0xE000U

/*
**
** AD7879 ADC modes (Bit 10 & 11 in Control register 1)
**
*/

/*
** ADI_AD7879_SINGLE_CHANNEL
**  - AD7879 is in Single channel mode
*/
#define ADI_AD7879_SINGLE_CHANNEL               0x400U
/*
** ADI_AD7879_SLAVE_SEQUENCE
**  - AD7879 is in Slave sequencer mode
*/
#define ADI_AD7879_SLAVE_SEQUENCE               0x800U
/*
** ADI_AD7879_MASTER_SEQUENCE
**  - AD7879 is in Master sequencer mode
*/
#define ADI_AD7879_MASTER_SEQUENCE              0xC00U

/*
**
** AD7879 Interrupt modes
**
*/

/*
** ADI_AD7879_DAV_INT_MODE
**  - AD7879 Data Available interrupt mode
*/
#define ADI_AD7879_DAV_INT_MODE                 0U
/*
** ADI_AD7879_ALARM_INT_MODE
**  - AD7879 alarm interrupt mode
*/
#define ADI_AD7879_ALARM_INT_MODE               0x2000U

/*
**
** AD7879 Interrupt indicator bits
**
*/
/*
** ADI_AD7879_PENIRQ_DISABLED
**  - AD7879 PEN Interrupt disabled bit
*/
#define ADI_AD7879_PENIRQ_DISABLED              0x8000U
/*
** ADI_AD7879_PENIRQ_BIT
**  - AD7879 PEN Interrupt request indicator
*/
#define ADI_AD7879_PENIRQ_BIT                   1U
/*
** ADI_AD7879_INTIRQ_BIT
**  - AD7879 INT Interrupt request indicator
*/
#define ADI_AD7879_INTIRQ_BIT                   2U
/*
** ADI_AD7879_PENIRQ_STATUS
**  - AD7879 PENIRQ status indicator - TRUE when detects a screen touch,
**                                     FALSE when detects a screen release
*/
#define ADI_AD7879_PENIRQ_STATUS                8U

/*
**
** Data Structures
**
*/

#ifdef ADI_IMPLEMENT_AD7879_1

/*
**
** AD7879-1 TWI interface
**
*/
typedef struct {
    /* TWI Device number used for AD7879 register access */
    u32                         nTwiDevNumber;
    /* TWI address of the AD7879 device */
    u32                         nTwiDevAddress;
    /* TWI Configuration Table specific to the application */
    ADI_DEV_CMD_VALUE_PAIR      *poTwiConfigTable;
} ADI_AD7879_TWI;

#define DEFAULT_TWI_DEVICE_ADDRESS 0x2Cu    /* BF527 EZ-Kit default */

#endif

/* Device instance structure is mostly common between AD7879 and AD7879-1 */

/*
** ADI_DD_AD7879_DEF
**  - AD7879 Device instance structure
*/
#pragma pack(4)
typedef struct __AdiDdAd7879Def
{
    /* Variable: bIsInUse
        - Device usage status flag. true when device is in use */
    u8                              bIsInUse;

    /* Variable: nInterruptMonitorStatus
        - Interrupt monitoring status indicator */
    u8                              nInterruptMonitorStatus;

#if !defined(ADI_IMPLEMENT_AD7879_1)

    /* Variable: nSpiDevNumber
        - SPI Device number to use to access AD7879 registers */
    u8                              nSpiDevNumber;

    /* Variable: nSpiChipSelect
        - SPI Chipselect to select AD7879 to access its registers */
    u8                              nSpiChipSelect;

#endif /* ADI_IMPLEMENT_AD7879_1 */

    /* Variable: nControlReg1
        - AD7879 Control register 1 cache */
    u16                             nControlReg1;

    /* Variable: nControlReg2
        - AD7879 Control register 2 cache */
    u16                             nControlReg2;

    /* Variable: nControlReg3
        - AD7879 Control register 3 cache */
    u16                             nControlReg3;

    /* Variable: oResultReg
        - AD7879 result registers instance */
    ADI_AD7879_RESULT_REGS          oResultReg;

    /* Variable: oIrqPort
        - Port info for AD7879 PENIRQ/INT interrupt signal */
    ADI_AD7879_INTERRUPT_PORT       oIrqPort;

    /* Handle: hDeviceManager
        - Device Manager Handle */
    ADI_DEV_MANAGER_HANDLE          hDeviceManager;

    /* Handle: hDeviceManager
        - Device Handle allocated by the Device Manager */
    ADI_DEV_DEVICE_HANDLE           hDevice;

    /* Handle: hDcbManager
        - Deferred Callback Manager Handle */
    ADI_DCB_HANDLE                  hDcbManager;

#ifdef ADI_IMPLEMENT_AD7879_1

    /* TWI addressing information for AD7879 */
    ADI_AD7879_TWI                  oAD7879Twi;

#else

    /* Handle: hSpiDriver
        - SPI Driver Handle */
    ADI_DEV_PDD_HANDLE              hSpiDriver;

#endif /* ADI_IMPLEMENT_AD7879_1 */

    /* Function pointer: pfDeviceManagerCallback
        - Pointer to callback function supplied by the Device Manager */
    ADI_DCB_CALLBACK_FN             pfDeviceManagerCallback;

} ADI_DD_AD7879_DEF;
#pragma pack()

/*=============  D A T A  =============*/

/*
** AD7879Device
**  - AD7879 Device instance
*/
static ADI_DD_AD7879_DEF    _gaoAd7879Device[] =
{
    /* Device 0 */
    {
        /* Device is not in use */
        false,
        /* Interrupt is disabled */
        false,
#if !defined(ADI_IMPLEMENT_AD7879_1)
        /* No SPI Device number by default */
        0xFFU,
        /* No chipselect by default */
        0U,
#endif
        /* Control register 1 */
        0U,
        /* Control register 2 */
        0U,
        /* Control register 3 */
        0U,
        /* Result registers */
        {
            /* X position */
            0U,
            /* Y position */
            0U,
            /* Z1 measurement */
            0U,
            /* Z2 measurement */
            0U,
            /* AUX/VBAT voltage measurement */
            0U,
            /* Temperature conversion measurement */
            0U,
        },
        /*  AD7879 PENIRQ/INT interrupt port */
        { 
            ADI_FLAG_UNDEFINED,
            (ADI_INT_PERIPHERAL_ID) 0,
        },
        /* Device Manager Handle */
        NULL,
        /* Device Handle allocated by the Device Manager */
        NULL,
        /* Deferred Callback Manager Handle */
        NULL,
#ifdef ADI_IMPLEMENT_AD7879_1
        {   /* TWI access configuration */
            0,
            0,
            NULL
        },
#else
        /* SPI Driver Handle */
        NULL,
#endif
        /* Pointer to callback function supplied by the Device Manager */
        NULL,
    },
};

#ifdef ADI_IMPLEMENT_AD7879_1

/*
    TWI Device access service configuration
*/
static ADI_DEV_CMD_VALUE_PAIR DevAccessTWIConfig[] = {
    /* Dataflow is always Seq IO    */
    { ADI_DEV_CMD_SET_DATAFLOW_METHOD,  (void *)ADI_DEV_MODE_SEQ_CHAINED    },
    /* Enable TWI Dataflow          */
    { ADI_DEV_CMD_SET_DATAFLOW,         (void *)TRUE                        },
     /* End of command table         */
    { ADI_DEV_CMD_END,                  (void *)NULL                        }
};

/*********************************************************************
Table for Register Field Error check and Register field access

Table structure
    Count of registers containing individual fields
    Array of addresses of those registers
    Array of register field locations within those registers

Register field location
    Bit indicating least significant end of each field is 1
*********************************************************************/

/* AD7879 registers containing bitfields */
static u16 AD7879_BitfieldRegs[] =
{
    AD7879_REG_CONTROL_1,
    AD7879_REG_CONTROL_2,
    AD7879_REG_CONTROL_3,
};

/* A bitfield map for each entry in AD7879_BitfieldRegs.  The lsb of each
** bitfield in a register is set to 1.
*/
static u16 AD7879_BitfieldMap[] =
{
    0x9501u,
    0x7EB1u,
    0xFFFFu,
};

static ADI_DEVICE_ACCESS_REGISTER_FIELD AD7879_BitfieldTable[] = {
    /* count of bitfield-containing registers */
    sizeof(AD7879_BitfieldRegs)/sizeof(AD7879_BitfieldRegs[0]),
    /* array of addresses of bitfield-containing registers */
    AD7879_BitfieldRegs,
    /* array of field locations in the bitfield-containing registers */
    AD7879_BitfieldMap
};


/*********************************************************************
Table for reserved bit configuration

Table structure
    Count of registers containing reserved bits
    Array of addresses of those registers
    Array of reserved bit locations within those registers
    Array of reserved bit values within those registers

Reserved bit location and value
    Reserved bit locations are indicated by a '1'
    Reserved bit values are just a bit pattern of '1' and '0' as appropriate
*********************************************************************/

/* AD7879_ registers having reserved locations */
static u16 AD7879_ReservedBitRegs[] =
{
    AD7879_REG_CONTROL_3,
};

/* A reserved bit map for each entry in AD7879_ReservedBitregs.  Each reserved
** bit in a register is indicated by a 1.
*/
static u16 AD7879_ReservedBitPos[] =
{
    0x0001u,
};

/* Reserved bit values for each entry in AD7879_ReservedBitregs.  Each reserved
** bit location is either '0' or '1' as appropriate ('0' for AD7879).
*/
static u16 AD7879_ReservedBitVal[] = {0};

static ADI_DEVICE_ACCESS_RESERVED_VALUES AD7879_ReservedBitTable[] = {
    /* count of reserved-bit-containing registers */
    sizeof(AD7879_ReservedBitRegs)/sizeof(AD7879_ReservedBitRegs[0]),
    /* array of addresses of reserved-bit-containing registers */
    AD7879_ReservedBitRegs,
    /* array of reserved bit location maps */
    AD7879_ReservedBitPos,
    /* array of reserved bit recommended values */
    AD7879_ReservedBitVal
};


/*********************************************************************
Table for invalid register access checks

Structure - Count of invalid register addresses
            Array of invalid register addresses
            Count of read-only register addresses
            Array of read-only register addresses
*********************************************************************/

static u16 AD7879_InvalidRegs[] = { 0 }; /* Only address 0 is invalid */

static u16 AD7879_ReadOnlyRegs[] =
{
    AD7879_REG_XPOS,
    AD7879_REG_YPOS,
    AD7879_REG_Z1,
    AD7879_REG_Z2,
    AD7879_REG_AUX_BAT,
    AD7879_REG_TEMP,
    AD7879_REG_REF_ID,
};

static ADI_DEVICE_ACCESS_VALIDATE_REGISTER AD7879_ReadonlyTable[] =
{
    /* count of invalid register addresses */
    sizeof(AD7879_InvalidRegs)/sizeof(AD7879_InvalidRegs[0]),
    /* array of invalid register addresses */
    AD7879_InvalidRegs,
    /* count of read-only registers */
    sizeof(AD7879_ReadOnlyRegs)/sizeof(AD7879_ReadOnlyRegs[0]),
    /* array of addresses of read-only registers */
    AD7879_ReadOnlyRegs
};

#else

/* Flag to monitor SPI data transfer */
static volatile u8  bIsSpiTransferComplete = false;

#endif /* ADI_IMPLEMENT_AD7879_1 */

/*
**
**  Local Function Prototypes
**
*/

/* Opens AD7879 device */
static u32  adi_pdd_Open(
    ADI_DEV_MANAGER_HANDLE      hDeviceManager,
    u32                         nDeviceNumber,
    ADI_DEV_DEVICE_HANDLE       hDevice,
    ADI_DEV_PDD_HANDLE          *phPhysicalDevice,
    ADI_DEV_DIRECTION           eDirection,
    void                        *pCriticalRegionArg,
    ADI_DMA_MANAGER_HANDLE      hDmaManager,
    ADI_DCB_HANDLE              hDcbManager,
    ADI_DCB_CALLBACK_FN         pfDeviceManagerCallback
);

/* Closes AD7879 device */
static u32  adi_pdd_Close(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice
);

/* Not supported for AD7879 device */
static u32  adi_pdd_Read(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poRxBuffer
);

/* Not supported for AD7879 device */
static u32  adi_pdd_Write(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poTxBuffer
);

/* Not supported for AD7879 device */
static u32  adi_pdd_SequentialIO(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poSeqBuffer
);

/* Sets or senses device specific parameter */
static u32  adi_pdd_Control(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    u32                         nCommandID,
    void                        *pCommandValue
);

/* Access AD7879 device registers */
static u32 AccessRegs (
    ADI_DD_AD7879_DEF  *poDevice, /* pointer to the device we're working on   */
    u32                Command,   /* Command ID from the client               */
    void               *Value     /* Command specific value                   */
);

#ifdef ADI_IMPLEMENT_AD7879_1

/*
** ADIAD7879_1_EntryPoint
**  - Device Manager Entry point for AD7879-1 driver
*/
ADI_DEV_PDD_ENTRY_POINT     ADIAD7879_1_EntryPoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control,
    adi_pdd_SequentialIO
};

#else

/* Mask function for AD7879 register field access */
static void MaskField(
    u16         RegAddr,        /* Register Address being accessed      */
    u16         *RegData,       /* Present value of the Register        */
    u16         RegField,       /* Register Field of the register to access */
    u8          AccessFlag  /* SPI Read/Write indicator                 */
);

/* Open SPI device to access AD7879 registers */
static u32 SpiOpen(
    ADI_DD_AD7879_DEF          *poDevice        /* pointer to the device we're working on   */
);

/* Calls SPI to Access AD7879 device registers */
static u32 SpiAccess(               /* Access AD7879 device registers via SPI   */
    ADI_DD_AD7879_DEF          *poDevice,       /* pointer to the device we're working on   */
    u16             RegAddr,        /* AD7879 Register address to access        */
    u16             *RegData,       /* AD7879 Register data             */
    u8              AccessFlag      /* SPI Read/Write indicator         */
);

/* Controls AD7879 Chip select signal to access its registers */
static u32  SpiSlaveSelect(
    ADI_DD_AD7879_DEF  *poDevice
);

/* SPI driver Callback for AD7879 */
static void AD7879SpiCallback (
    void    *phClient,
    u32     nEvent,
    void    *pArg
);

/*
** ADIAD7879EntryPoint
**  - Device Manager Entry point for AD7879 driver
*/
ADI_DEV_PDD_ENTRY_POINT     ADIAD7879EntryPoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control,
    adi_pdd_SequentialIO
};

#endif

/* Callback for AD7879 PENIRQ/INT Interrupts */
static void AD7879IntCallback (
    void    *phClient,
    u32     nEvent,
    void    *pArg
);

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)

/* Validates Physical Device Driver Handle */
static u32 ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE      hPhysicalDevice
);

#endif

/*********************************************************************

    Function: adi_pdd_Open

     Opens AD7879 device

    Parameters:
     hDeviceManager     - Device Manager handle
     nDeviceNumber      - AD7879 device number to open
     hDevice            - Device Handle allocated by the Device Manager
     phPhysicalDevice   - Pointer to Physical Device Handle
     eDirection         - Data Direction
     pCriticalRegionArg - Critical region parameter
     hDmaManager        - DMA Manager Handle
     hDcbManager        - Deferred Callback Handle
     pfDMCallback       - Pointer to Device Manager supplied
                          Callback function

    Returns:
     - ADI_DEV_RESULT_SUCCESS on success
     - ADI_DEV_RESULT_BAD_DEVICE_NUMBER when the given
       device number is invalid
     - ADI_DEV_RESULT_BAD_MANAGER_HANDLE when the given
       device manager handle/DMA manager handle is invalid
     - ADI_DEV_RESULT_DEVICE_IN_USE if the device number is
       already in use

*********************************************************************/
static u32 adi_pdd_Open(
    ADI_DEV_MANAGER_HANDLE      hDeviceManager,
    u32                         nDeviceNumber,
    ADI_DEV_DEVICE_HANDLE       hDevice,
    ADI_DEV_PDD_HANDLE          *phPhysicalDevice,
    ADI_DEV_DIRECTION           eDirection,
    void                        *pCriticalRegionArg,
    ADI_DMA_MANAGER_HANDLE      hDmaManager,
    ADI_DCB_HANDLE              hDcbManager,
    ADI_DCB_CALLBACK_FN         pfDMCallback
)
{
    /* Return value - assume we're going to be successful */
    u32     nResult = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we're working on */
    ADI_DD_AD7879_DEF  *poDevice;
    /* exit critical region parameter */
    void    *pExitCriticalArg;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    /* check the device number */
    if (nDeviceNumber >= ADI_AD7879_NUM_DEVICES)
    {
        /* Invalid Device number */
        nResult = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    }

    /*  Continue only if the device number is valid */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* insure the device the client wants is available  */
        nResult  = ADI_DEV_RESULT_DEVICE_IN_USE;
        /* AD7879 device we're working on */
        poDevice = &_gaoAd7879Device[nDeviceNumber];

        /* Protect this section of code - entering a critical region    */
        pExitCriticalArg = adi_int_EnterCriticalRegion(pCriticalRegionArg);

        /* Check the device usage status*/
        if (poDevice->bIsInUse == FALSE)
        {
            /* Device is not in use. Reserve the device for this client */
            poDevice->bIsInUse = TRUE;
            nResult = ADI_DEV_RESULT_SUCCESS;
        }

        /* Exit the critical region */
        adi_int_ExitCriticalRegion(pExitCriticalArg);

        /* Continue only when the AD7879 device is available for use  */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* AD7879 device is reserved for this client */

            /* Reset the actual device */
            /* Clear all AD7879 register cache */
            poDevice->nControlReg1              = 0;
            poDevice->nControlReg2              = 0;
            poDevice->nControlReg3              = 0;
            /* clear results register*/
            poDevice->oResultReg.nX             = 0;
            poDevice->oResultReg.nY             = 0;
            poDevice->oResultReg.nZ1            = 0;
            poDevice->oResultReg.nZ2            = 0;
            poDevice->oResultReg.nAuxVbat       = 0;
            poDevice->oResultReg.nTemperature   = 0;
#ifdef ADI_IMPLEMENT_AD7879_1
            /* Default TWI settings */
            poDevice->oAD7879Twi.nTwiDevNumber    = 0;
            poDevice->oAD7879Twi.nTwiDevAddress   = DEFAULT_TWI_DEVICE_ADDRESS;
            poDevice->oAD7879Twi.poTwiConfigTable = DevAccessTWIConfig;
#else
            /* No SPI Chipselect by default */
            poDevice->nSpiChipSelect            = 0;
#endif
            /* All AD7879 interrupts are disabled by default */
            poDevice->nInterruptMonitorStatus   = 0;

            /* initialize the device settings */
            /* Pointer to Device manager handle */
            poDevice->hDeviceManager            = hDeviceManager;
            /* Pointer to Device handle */
            poDevice->hDevice                   = hDevice;
            /* Pointer to Deffered Callback manager handle */
            poDevice->hDcbManager               = hDcbManager;
            /* Pointer to Device manager callback function  */
            poDevice->pfDeviceManagerCallback   = pfDMCallback;

            /* save the physical device handle to client supplied location */
            *phPhysicalDevice = (ADI_DEV_PDD_HANDLE *)poDevice;
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(nResult);
}

/*********************************************************************

    Function: adi_pdd_Close

     Closes AD7879 device

    Parameters:
     hPhysicalDevice    - Physical Device handle to AD7879 device instance

    Returns:
     - ADI_DEV_RESULT_SUCCESS on success
     - Error code returned from remove flag callback function
     - Error code returned by device access function

*********************************************************************/
static u32  adi_pdd_Close(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice
)
{
    /* Return value - assume we're going to be successful   */
    u32     nResult = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we're working on */
    ADI_DD_AD7879_DEF  *poDevice;

    /* Configuration table to disable AD7879 */
    ADI_DEV_ACCESS_REGISTER     aoDisableAD7879[] =
    {
        { AD7879_REG_CONTROL_1, 0U  },
        { AD7879_REG_CONTROL_2, 0U  },
        { AD7879_REG_CONTROL_3, 0U  },
        { ADI_DEV_REGEND,       0U  }
    };

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given device handle */
    nResult = ValidatePDDHandle(hPhysicalDevice);

    /* Continue only if the given device handle is valid */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {

#endif

        /* avoid casts */
        poDevice = (ADI_DD_AD7879_DEF *)hPhysicalDevice;

        /* Unhook all AD7879 interrupts */
        /* IF (any AD7879 interrupt is enabled) */
        if (poDevice->nInterruptMonitorStatus != 0U)
        {
            /* IF (PENIRQ/INT interrupt is enabled & monitored by this driver) */
            if ((poDevice->nInterruptMonitorStatus & ADI_AD7879_PENIRQ_BIT)||\
                (poDevice->nInterruptMonitorStatus & ADI_AD7879_INTIRQ_BIT))
            {
                /* Disable PENIRQ/INT Interrupt Flag callback  */
                nResult = adi_flag_RemoveCallback(poDevice->oIrqPort.eFlagId);

            }
        }

        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* Disable AD7879 */
            nResult = adi_dev_Control(poDevice->hDevice,
                                      ADI_DEV_CMD_REGISTER_TABLE_WRITE,
                                      (void *)aoDisableAD7879);
            /* Clear the interrupt enable indicator */
            poDevice->nInterruptMonitorStatus = 0;
            /* mark this AD7879 device as closed */
            poDevice->bIsInUse = FALSE;
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(nResult);
}

/*********************************************************************

    Function: adi_pdd_Read

     Never called as AD7879 uses Device Access commands
     to access its internal registers

    Parameters:
     hPhysicalDevice    - Physical Device handle to AD7879 device instance
     eBufferType        - Buffer type to queue
     poTxBuffer         - Pointer to receive buffer

    Returns:
     - ADI_DEV_RESULT_NOT_SUPPORTED as this function is not supported
       by this device

*********************************************************************/
static u32  adi_pdd_Read(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poRxBuffer
)
{
    /* return error (not supported) */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************

    Function: adi_pdd_Write

     Never called as AD7879 uses Device Access commands
     to access its internal registers

    Parameters:
     hPhysicalDevice    - Physical Device handle to AD7879 device instance
     eBufferType        - Buffer type to queue
     poTxBuffer         - Pointer to receive buffer

    Returns:
     - ADI_DEV_RESULT_NOT_SUPPORTED as this function is not supported
       by this device

*********************************************************************/
static u32  adi_pdd_Write(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poTxBuffer
)
{
    /* return error (not supported) */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************

    Function: adi_pdd_SequentialIO

     Never called as AD7879 uses Device Access commands
     to access its internal registers

    Parameters:
     hPhysicalDevice    - Physical Device handle to AD7879 device instance
     eBufferType        - Buffer type to queue
     poTxBuffer         - Pointer to receive buffer

    Returns:
     - ADI_DEV_RESULT_NOT_SUPPORTED as this function is not supported
       by this device

*********************************************************************/
static u32  adi_pdd_SequentialIO(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poSeqBuffer
)
{
    /* return error (not supported) */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************

    Function: adi_pdd_Control

     Sets or senses device specific parameter

    Parameters:
     hPhysicalDevice    - Physical Device handle to
                          AD7879 device instance
     nCommandID         - Command ID
     pCommandValue      - Command specific value

    Returns:
     - ADI_DEV_RESULT_SUCCESS on success
*********************************************************************/
static u32  adi_pdd_Control(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    u32                         nCommandID,
    void                        *pCommandValue
)
{

    /* Pointer to the device we're working on*/
    ADI_DD_AD7879_DEF   *poDevice;
    /* Return value - assume we're going to be successful */
    u32                 nResult  = ADI_DEV_RESULT_SUCCESS;
    /* u32 equivalent of command specific value */
    u32                 nu32Value;

    /* avoid casts */
    poDevice    = (ADI_DD_AD7879_DEF *)hPhysicalDevice;
    /* assign 32 bit values for the Value argument */
    nu32Value    = (u32)pCommandValue;

    /* interrupt ports for flag and interrupt designations */
    ADI_AD7879_INTERRUPT_PORT *poPenIrqPort;

/* for Debug build only - check for errors if required */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given PDDHandle */
    nResult = ValidatePDDHandle(hPhysicalDevice);

    /* Continue only if the given PDDHandle is valid */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* CASEOF (Command ID)  */
        switch (nCommandID)
        {

            /*
            ** Device Manager specific commands
            */

            /* CASE (Control dataflow) */
            case ADI_DEV_CMD_SET_DATAFLOW:
            /* CASE (Set dataflow method) */
            case ADI_DEV_CMD_SET_DATAFLOW_METHOD:
                /* Do nothing & simply return back for these commands */
                break;

            /* CASE (Query for processor DMA support) */
            case ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT:
                /* AD7879 does not support DMA */
                *((u32 *)pCommandValue) = FALSE;
                break;
#ifdef ADI_IMPLEMENT_AD7879_1
            /*
            ** TWI related commands
            */

            /* CASE (Set AD7879 TWI Device Number to use) */
            case ADI_AD7879_CMD_SET_TWI_DEVICE_ADDRESSES:
                /* Save the TWI device number to use */
                poDevice->oAD7879Twi.nTwiDevNumber = (u8)(nu32Value >> 8);
                /* Save the TWI device address to use */
                poDevice->oAD7879Twi.nTwiDevAddress = (u8)(nu32Value & 0xFFu);
                break;

            /* CASE (Set TWI Configuration table specific to the application) */
            case ADI_AD7879_CMD_SET_TWI_CONFIG_TABLE:
                /* Save the TWI configuration table to use */
                poDevice->oAD7879Twi.poTwiConfigTable =
                    (ADI_DEV_CMD_VALUE_PAIR*)nu32Value;
                break;
#else
            /*
            ** SPI related commands
            */

            /* CASE (Set AD7879 SPI Device Number to use) */
            case ADI_AD7879_CMD_SET_SPI_DEVICE_NUMBER:
                /* Save the SPI device number to use */
                poDevice->nSpiDevNumber   = (u8)nu32Value;
                break;

            /* CASE (Set AD7879 SPI Chipselect) */
            case ADI_AD7879_CMD_SET_SPI_CS:
                /* Save the SPI chipselect for AD7879 */
                poDevice->nSpiChipSelect  = (u8)nu32Value;
                break;
#endif
            /*
            ** Commands to Enable Interrupt monitoring
            */

            /* CASE: (Set driver to monitor PENIRQ and
                      Data Available/Alarm (INT) interrupts) */
            case ADI_AD7879_CMD_ENABLE_INTERRUPT_PEN_INT_IRQ:
            
                /* IF (PENIRQ and INT Interrupt monitoring already enabled) */
                if (poDevice->oIrqPort.eFlagId != ADI_FLAG_UNDEFINED )
                {
                    /* Remove previously callback installed on the
                       interrupt monitoring flag pin */
                     adi_flag_RemoveCallback(poDevice->oIrqPort.eFlagId);
                    /* Interrupot monitoring is disabled for this flag pin */
                     poDevice->nInterruptMonitorStatus = 0U;
                }
                
                /* Map PENIRQ Interrupt to this Flag */
                poPenIrqPort = (ADI_AD7879_INTERRUPT_PORT*)pCommandValue;

                /* IF (PENIRQ and INT interrupt monitoring is disabled) */
                if (poDevice->nInterruptMonitorStatus == 0U)
                {

                    /* Open corresponding Blackfin flag */
                    if ((nResult = adi_flag_Open (poPenIrqPort->eFlagId)) != ADI_FLAG_RESULT_SUCCESS)
                    {
                        /* exit on error */
                        break;
                    }

                    /* Set flag direction as input */
                    if ((nResult = adi_flag_SetDirection(poPenIrqPort->eFlagId,
                                                         ADI_FLAG_DIRECTION_INPUT))
                                                       != ADI_FLAG_RESULT_SUCCESS)
                    {
                        /* exit on error */
                        break;
                    }

                    /*
                    ** Set flag trigger to sense interrupt & generate callback
                    */

                    /* Senses on Falling edge, Parameters are
                        - Flag ID, Flag Peripheral interrupt ID,
                          Flag Trigger mode, Is this a wake-up flag,
                          Client Handle, Callback Manager, Callback Function */

                    nResult = adi_flag_InstallCallback (poPenIrqPort->eFlagId,
                                                        poPenIrqPort->eFlagIntId,
                                                        ADI_FLAG_TRIGGER_FALLING_EDGE,
                                                        true,
                                                        (void *)poDevice,
                                                        poDevice->hDcbManager,
                                                        AD7879IntCallback);
                }

                /* IF (Successfully installed callback on interrupt flag) */
                if (nResult == ADI_DEV_RESULT_SUCCESS)
                {
                    /* Update 'nInterruptMonitorStatus' to indicate that
                       PENIRQ interrupt monitoring is enabled */
                    poDevice->nInterruptMonitorStatus |= ADI_AD7879_PENIRQ_BIT;
                    /* Update PENIRQ status bit as screen not touched */
                    poDevice->nInterruptMonitorStatus &= ~ADI_AD7879_PENIRQ_STATUS;
                    /* Update 'nInterruptMonitorStatus' to indicate that
                       Data available (INTIRQ) monitoring is enabled */
                    poDevice->nInterruptMonitorStatus |= ADI_AD7879_INTIRQ_BIT;
                    
                    /* Save the Flag info */
                    poDevice->oIrqPort.eFlagId = poPenIrqPort->eFlagId;
                    poDevice->oIrqPort.eFlagIntId = poPenIrqPort->eFlagIntId;

                }
                break;

            /*
            ** Commands to Disable Interrupt monitoring
            */

            /* CASE (Disable PENIRQ (Remove PENIRQ from monitoring) )*/
            case ADI_AD7879_CMD_DISABLE_INTERRUPT_PENIRQ:

                /* Update 'nInterruptMonitorStatus' indicating that
                   PENIRQ interrupt is disabled */
                poDevice->nInterruptMonitorStatus &= ~ADI_AD7879_PENIRQ_BIT;
                /* Update PENIRQ status bit as screen not touched */
                poDevice->nInterruptMonitorStatus &= ~ADI_AD7879_PENIRQ_STATUS;

                /* IF (INTIRQ monitoring is disabled) */
                if (!(poDevice->nInterruptMonitorStatus & ADI_AD7879_INTIRQ_BIT))
                {
                    /* Remove callback installed on the
                       interrupt monitoring flag pin */
                    nResult = adi_flag_RemoveCallback(poDevice->oIrqPort.eFlagId);
                }
                break;

            /* CASE (Disable INTIRQ (Remove INTIRQ from monitoring)( */
            case ADI_AD7879_CMD_DISABLE_INTERRUPT_INTIRQ:

                 /* Update 'nInterruptMonitorStatus' indicating that
                   (INTIRQ interrupt is disabled */
                poDevice->nInterruptMonitorStatus &= ~ADI_AD7879_INTIRQ_BIT;

                /* IF (PENIRQ monitoring is disabled) */
                if (!(poDevice->nInterruptMonitorStatus & ADI_AD7879_PENIRQ_BIT))
                {
                    /* Remove callback installed on the
                       interrupt monitoring flag pin */
                    nResult = adi_flag_RemoveCallback(poDevice->oIrqPort.eFlagId);
                }
                break;

            /*
            ** Device access commands to Access AD7879 registers
            */

            /* CASE: (Read a specific register from the AD7879)     */
            case ADI_DEV_CMD_REGISTER_READ:
            /* CASE: (Configure a specific register in the AD7879)  */
            case ADI_DEV_CMD_REGISTER_WRITE:
            /* CASE: (Read a specific field from a given AD7879 device register) */
            case ADI_DEV_CMD_REGISTER_FIELD_READ:
            /* CASE: (Write to a specific field in a given AD7879 device register) */
            case ADI_DEV_CMD_REGISTER_FIELD_WRITE:
            /* CASE: (Read block of AD7879 registers) */
            case ADI_DEV_CMD_REGISTER_BLOCK_READ:
            /* CASE: (Write to a block of AD7879 registers) */
            case ADI_DEV_CMD_REGISTER_BLOCK_WRITE:
            /* CASE: (Read a table of selective registers in AD7879) */
            case ADI_DEV_CMD_REGISTER_TABLE_READ:
            /* CASE: (Write to a table of selective registers in AD7879) */
            case ADI_DEV_CMD_REGISTER_TABLE_WRITE:
            /* CASE (Read a table of selective register(s) field(s) in AD7879) */
            case ADI_DEV_CMD_REGISTER_FIELD_TABLE_READ:
            /* CASE (Write to a table of selective register(s) field(s) in AD7879) */
            case ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE:

                /* Access AD7879 registers */
                nResult = AccessRegs(poDevice,nCommandID,pCommandValue);
                break;

            /*
            ** Default: other commands not supported
            */

            /* DEFAULT: Command not supported */
            default:
                nResult = ADI_AD7879_RESULT_CMD_NOT_SUPPORTED;
                break;
        }
/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(nResult);
}

/*********************************************************************

    Function: AD7879IntCallback

     Callback routine for AD7879 PENIRQ/INT Interrupt

    Parameters:
     phClient       - Client handle passed to flag service
     nEvent         - Callback Event
     pArg           - Callback Argument

    Returns:
     None
*********************************************************************/
static void AD7879IntCallback (
    void    *phClient,
    u32     nEvent,
    void    *pArg
)
{
    /* Retrive the device instance to which this callback belongs */
    ADI_DD_AD7879_DEF  *poDevice = (ADI_DD_AD7879_DEF *)phClient;
    /* Instance to access block of registers */
    ADI_DEV_ACCESS_REGISTER_BLOCK   oAccessBlock;
    /* ADC Channel selected for conversion */
    u16  nChannel;
    /* Callback Event ID to client */
    u32  nClientEvent;

/* Debug build only */
#ifdef ADI_DEV_DEBUG
    /* verify the Device handle */
    if (ValidatePDDHandle(phClient) == ADI_DEV_RESULT_SUCCESS)
    {
#endif
            
        /* IF (AD7879 INT/DAV is enabled) */
        if ((poDevice->nControlReg1 & ADI_AD7879_CTRL_REG1_PENIRQ_MASK) == ADI_AD7879_PENIRQ_DISABLED)
        {
            /* IF (AD7879 ADC Data Available interrupt occurred ) */
            if ((poDevice->nControlReg3 & ADI_AD7879_CTRL_REG3_INT_MODE_MASK) == ADI_AD7879_DAV_INT_MODE)
            {
                /* IF (AD7879 ADC is in Master sequencer mode) */
                if ((poDevice->nControlReg1 & ADI_AD7879_CTRL_REG1_MODE_MASK) == ADI_AD7879_MASTER_SEQUENCE)
                {   
                
                    /* Access all four ADC channel Result registers (X, Y, Z1 & Z2) */
                    oAccessBlock.Count   = 4U;
                    /* Result register address to start with */
                    oAccessBlock.Address = AD7879_REG_XPOS;
                    /* location to save the read result value */
                    oAccessBlock.pData   = (u16 *)(&poDevice->oResultReg);

                    /* IF (successfully read conversion results) */
                    if (AccessRegs(poDevice, ADI_DEV_CMD_REGISTER_BLOCK_READ,
                                    (void *) &oAccessBlock)
                                    == ADI_DEV_RESULT_SUCCESS )
                    {
                        /* Callback Event ID to client */
                        nClientEvent = ADI_AD7879_EVENT_SEQUENCER_DAV;

                        /* post callback to client indicating that DAV has occurred */
                        /* (parameters are Device Handle, nClientEvent, pointer to location holding ADC result(s)) */
                        (poDevice->pfDeviceManagerCallback)(poDevice->hDevice,
                                                        nClientEvent,
                                                        (void*) oAccessBlock.pData);
                    }
                    
                }
                /* ELSE IF (AD7879 ADC in Single channel) */
                else if ((poDevice->nControlReg1 & ADI_AD7879_CTRL_REG1_MODE_MASK) == ADI_AD7879_SINGLE_CHANNEL)
                {
                    /* Read the Result value of the converted ADC channel */
                    /* Access a single ADC channel Result register */
                    oAccessBlock.Count   = 1U;
                    /* ADC channel selected for conversion */
                    nChannel = ((poDevice->nControlReg1 & ADI_AD7879_CTRL_REG1_CHADD_MASK) >> AD7879_RFLD_CHADD);
                    /* Result register address to access */
                    oAccessBlock.Address = (nChannel + AD7879_REG_XPOS);
                    /* location to save the read result value */
                    oAccessBlock.pData   = (((u16 *)(&poDevice->oResultReg)) + nChannel);
                    /* Callback Event ID to client */
                    nClientEvent = ADI_AD7879_EVENT_SINGLE_DAV;

                    /* IF (Successfully read the channel register result) */
                    if(AccessRegs(poDevice, ADI_DEV_CMD_REGISTER_BLOCK_READ,
                                    (void *) &oAccessBlock) == ADI_DEV_RESULT_SUCCESS)
                    {
                        /* post callback to client indicating that DAV has occurred */
                        /* parameters are Device Handle, nClientEvent, pointer to location holding ADC result(s) */
                        (poDevice->pfDeviceManagerCallback)(poDevice->hDevice,
                                                    nClientEvent,
                                                    (void*)oAccessBlock.pData);
                    }
               }
                
             }
            /* ELSE (Out of limit interrupt, this function is not supported by the EZKIT) */
            else
            {
                    /* Callback Event ID to client for indication only */
                    nClientEvent = ADI_AD7879_EVENT_ALARM_OFL;

                    /* post callback to client indicating that OFL alarm has occurred */
                    /* (parameters are Device Handle, nClientEvent, pointer to location holding ADC result(s)) */
                    (poDevice->pfDeviceManagerCallback)(poDevice->hDevice,
                                                    nClientEvent,
                                                    NULL);
            }
            
        }
        else /* PENIRQ is enabled */
        {
            /* IF (Screen touch event was registered before) */
            if (poDevice->nInterruptMonitorStatus & ADI_AD7879_PENIRQ_STATUS)
            {
                /* PENIRQ interrupt status bit indicates the screen was in touch state.
                   This interrupt must be caused by screen release event */
                /* Clear this status bit to indicate that screen is not touched */
                poDevice->nInterruptMonitorStatus &= ~ADI_AD7879_PENIRQ_STATUS;

                /* change flag to trigger on falling edge so that
                   we could detect next screen touch */
                adi_flag_SetTrigger(poDevice->oIrqPort.eFlagId,
                                        ADI_FLAG_TRIGGER_FALLING_EDGE);
                                        
                                        
                 /* Access all four ADC channel Result registers (X, Y, Z1 & Z2) */
                 oAccessBlock.Count   = 4U;
                 /* Result register address to start with */
                 oAccessBlock.Address = AD7879_REG_XPOS;
                 /* location to save the read result value */
                 oAccessBlock.pData   = (u16 *)(&poDevice->oResultReg);

                 /* IF (successfully read conversion results) */
                 if (AccessRegs(poDevice, ADI_DEV_CMD_REGISTER_BLOCK_READ,
                                (void *) &oAccessBlock) == ADI_DEV_RESULT_SUCCESS )
                 {
                        /* Client Event ID */
                        nClientEvent = ADI_AD7879_EVENT_PENIRQ_RELEASE;

                        /* post callback to client indicating that PENIRQ release has occurred */
                        /* parameters are Device Handle, nClientEvent, pointer to location holding ADC result(s) */
                        (poDevice->pfDeviceManagerCallback)(poDevice->hDevice,
                                                            nClientEvent,
                                                            (void*) oAccessBlock.pData);
                 }
             }
            /* ELSE (Screen touch event occurred) */
            else
            {
                /* PENIRQ interrupt status bit indicates the screen was in release state.
                This interrupt must be caused by screen touch event */
                /* Set this status bit to indicate that screen is touched */
                poDevice->nInterruptMonitorStatus |= ADI_AD7879_PENIRQ_STATUS;
                /* Client Event ID */
                nClientEvent = ADI_AD7879_EVENT_PENIRQ_TOUCH;
                /* change flag to trigger on raising edge so that we could detect screen release */
                adi_flag_SetTrigger(poDevice->oIrqPort.eFlagId,
                                        ADI_FLAG_TRIGGER_RISING_EDGE);

                /* post callback to client indicating that PENIRQ has occurred */
                /* parameters are Device Handle, nClientEvent, NULL */
                (poDevice->pfDeviceManagerCallback)(poDevice->hDevice,
                                                        nClientEvent,
                                                        NULL);
            }
            
        }
            
  
/* Debug build only */
#ifdef ADI_DEV_DEBUG
    }
#endif

}

#ifdef ADI_IMPLEMENT_AD7879_1
/*********************************************************************
*
* Function:  scan_commands
*
* Description: Helper function which returns true if the command
*              it is given to scan could involve a write to one of
*              the AD7879-1's control registers.
*
*********************************************************************/

static bool scan_commands(u32 command, void* value)
{
    switch (command) {
        case ADI_DEV_CMD_REGISTER_WRITE:
        case ADI_DEV_CMD_REGISTER_FIELD_WRITE:
        case ADI_DEV_CMD_REGISTER_BLOCK_WRITE:
        case ADI_DEV_CMD_REGISTER_TABLE_WRITE:
        case ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE:
            /* if this naive implementation makes the driver too
             * inefficient then add code to scan the command values
             * for writes specifically to the control registers.
             */
            return true;

        default:
            return false;
    }

    /* should not reach here */
}

/*********************************************************************
*
* Function:  AccessRegs
*
* Description: Access AD7879 Device registers via Device Access Service
*
*********************************************************************/

static u32 AccessRegs(
    ADI_DD_AD7879_DEF  *pDevice,  /* pointer to the device we're working on   */
    u32                 nCommand,  /* Command ID from the client              */
    void                *Value     /* Command specific value                  */
){
    /* Return value - assume success */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;

    /* Need to update cache of control registers? */
    bool wrote_a_control_reg = scan_commands(nCommand, Value);

    /* Device selection information passed to device access service */
    ADI_DEVICE_ACCESS_SELECT da_sel;

    /* Register access information passed to device access service */
    ADI_DEVICE_ACCESS_REGISTERS da_reg;

    /* Set up AD7879 device selection information */
    da_sel.DeviceCS   = 0;                          /* (dummy for TWI) */
    da_sel.Gaddr_len  = ADI_DEVICE_ACCESS_LENGTH1;  /* (dummy for TWI) */
    da_sel.Raddr_len  = ADI_DEVICE_ACCESS_LENGTH1;  /* reg addrs are 8-bit */
    da_sel.Rdata_len  = ADI_DEVICE_ACCESS_LENGTH2;  /* reg data are 16-bit */
    da_sel.AccessType = ADI_DEVICE_ACCESS_TYPE_TWI; /* TWI-connected */

    /* Set up AD7879 register access request information */
    da_reg.ManagerHandle    = pDevice->hDeviceManager;
    da_reg.ClientHandle     = NULL;
    da_reg.DeviceNumber     = pDevice->oAD7879Twi.nTwiDevNumber;
    da_reg.DeviceAddress    = pDevice->oAD7879Twi.nTwiDevAddress;
    da_reg.DCBHandle        = pDevice->hDcbManager;
    da_reg.DeviceFunction   = NULL;
    da_reg.Command          = nCommand;
    da_reg.Value            = Value;
    da_reg.FinalRegAddr     = AD7879_END_REG_ADDR;
    da_reg.RegisterField    = AD7879_BitfieldTable;
    da_reg.ReservedValues   = AD7879_ReservedBitTable;
    da_reg.ValidateRegister = AD7879_ReadonlyTable;
    da_reg.ConfigTable      = pDevice->oAD7879Twi.poTwiConfigTable;
    da_reg.SelectAccess     = &da_sel;
    da_reg.pAdditionalinfo  = (void *)NULL;

    /* Access the requested register(s) or field(s) via device access service */
    nResult = adi_device_access(&da_reg);

    /* Update cache of control register values if necessary */
    if (nResult == ADI_DEV_RESULT_SUCCESS && wrote_a_control_reg)
    {
        ADI_DEV_ACCESS_REGISTER_BLOCK block;
        u16 regs[3];

        block.Count = 3;
        block.Address = AD7879_REG_CONTROL_1;
        block.pData = regs;

        AccessRegs(pDevice, ADI_DEV_CMD_REGISTER_BLOCK_READ, &block);
        pDevice->nControlReg1 = regs[0];
        pDevice->nControlReg2 = regs[1];
        pDevice->nControlReg3 = regs[2];
    }

    return nResult;
}

#else

/*********************************************************************

    Function: AccessRegs

     Prepares AD7879 device register access

    Parameters:
     poDevice       - Pointer to the device we're working on
     nCommandID     - Command ID
     pCommandValue  - Command specific value

    Returns:
     - ADI_DEV_RESULT_SUCCESS on success
*********************************************************************/
static u32 AccessRegs (
    ADI_DD_AD7879_DEF   *poDevice,
    u32                 nCommandID,
    void                *pCommandValue
)
{
    /* AD7879 Register read/write flag
       true to read, false to write */
    u8      bReadFlag;
    /* Temp location to hold AD7879 register address and data */
    u16     nRegAddr, nRegData;
    /* Return value - assume we're going to be successful   */
    u32     nResult = ADI_DEV_RESULT_SUCCESS;
    /* Register access count */
    u8      nCount;

    /* Pointers to Device access structure passed by the client */
    ADI_DEV_ACCESS_REGISTER         *poSelectiveAccess;
    ADI_DEV_ACCESS_REGISTER_BLOCK   *poBlockAccess;
    ADI_DEV_ACCESS_REGISTER_FIELD   *poFieldAccess;

    /* Configure AD7879 registers? */
    if ((nCommandID == ADI_DEV_CMD_REGISTER_WRITE)             ||
        (nCommandID == ADI_DEV_CMD_REGISTER_FIELD_WRITE)       ||
        (nCommandID == ADI_DEV_CMD_REGISTER_TABLE_WRITE)       ||
        (nCommandID == ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE) ||
        (nCommandID == ADI_DEV_CMD_REGISTER_BLOCK_WRITE))
    {
        /* Write to AD7879 register(s) */
        bReadFlag = false;
    }
    else
    {
        /* Read AD7879 register(s) */
        bReadFlag = true;
    }

    /* CASEOF (Command ID)  */
    switch (nCommandID)
    {
        /* CASE (Read a block of AD7879 registers) */
        case ADI_DEV_CMD_REGISTER_BLOCK_READ:
        /* CASE (Configure a block of AD7879 registers) */
        case ADI_DEV_CMD_REGISTER_BLOCK_WRITE:

            /* Pointer to block access table */
            poBlockAccess = (ADI_DEV_ACCESS_REGISTER_BLOCK *) pCommandValue;
            /* Start address of register block to access */
            nRegAddr = poBlockAccess->Address;

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)

            /* IF (Register access count is invalid) */
            if (poBlockAccess->Count > ADI_AD7879_END_REG_ADDR)
            {
                /* return error */
                nResult = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
                break;
            }
#endif

            /* FOR (All register in this block to access)*/
            for (nCount=0;
                 nCount < poBlockAccess->Count;
                 nCount++)
            {
                /* IF (AD7879 register address to access is invalid) */
                if ((nRegAddr == ADI_AD7879_INVALID_REG1) ||
                    (nRegAddr > ADI_AD7879_END_REG_ADDR))
                {
                    /* return error (invalid register access) */
                    nResult = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
                    break;
                }
                /* ELSE (AD7879 register address is valid) */
                else
                {
                    /* IF (Read AD7879 registers) */
                    if (bReadFlag == true)
                    {
                        /* IF (Read the selected register failed) */
                        if ((nResult = SpiAccess(poDevice,
                                                 nRegAddr,
                                                 &nRegData,
                                                 bReadFlag))
                                               != ADI_DEV_RESULT_SUCCESS)
                        {
                            /* exit on error */
                            break;
                        }
                        /* ELSE (Successfully read selected register) */
                        else
                        {
                            /* update client table with read data */
                            *(poBlockAccess->pData + nCount) = nRegData;
                            /* next register address to be read */
                            nRegAddr++;
                        }
                    }
                    /* ELSE (Configure AD7879 device registers) */
                    else
                    {
                        /* IF (this is a Read-only register) */
                        if ((nRegAddr >= ADI_AD7879_RO_REG_START_ADDR) &&
                            (nRegAddr <= ADI_AD7879_RO_REG_END_ADDR))
                        {
                            /* Skip this register */
                            nRegAddr++;
                        }
                        /* ELSE (this register is writeable) */
                        else
                        {
                            /* IF (Failed to configure this register) */
                            if ((nResult = SpiAccess(poDevice,
                                                     nRegAddr,
                                                     (poBlockAccess->pData + nCount),
                                                     bReadFlag))
                                                  != ADI_DEV_RESULT_SUCCESS)
                            {
                                /* exit on error */
                                break;
                            }
                            /* ELSE (Successfully configured AD7879 register) */
                            else
                            {
                                /* next register address to configure */
                                nRegAddr++;
                            }

                        } /* End of if (this is a Read-only register) else case */

                    } /* End of if (Read AD7879 registers) else case */

                } /* End of if (invalid AD7879 register) else (valid register) case */

            } /* End of for (all registers in the block to access) */

            break;

        /* CASE (Read a specific AD7879 register value) */
        case ADI_DEV_CMD_REGISTER_READ:
        /* CASE (Configure a specific AD7879 register) */
        case ADI_DEV_CMD_REGISTER_WRITE:
        /* CASE (Read a table of AD7879 registers) */
        case ADI_DEV_CMD_REGISTER_TABLE_READ:
        /* CASE (Configure a table of AD7879 registers) */
        case ADI_DEV_CMD_REGISTER_TABLE_WRITE:

            /* Pointer to selective access table */
            poSelectiveAccess = (ADI_DEV_ACCESS_REGISTER *) pCommandValue;
            /* Load the first register address in the table to access */
            nRegAddr = poSelectiveAccess->Address;

            /* Continue until we reach end of register access */
            while (nRegAddr != ADI_DEV_REGEND)
            {

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)

                /* IF (AD7879 register address to access is invalid) */
                if ((nRegAddr == ADI_AD7879_INVALID_REG1) ||
                    (nRegAddr > ADI_AD7879_END_REG_ADDR))
                {
                    /* return error (invalid register) */
                    nResult = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
                    /* exit on error */
                    break;
                }
                /* ELSE (trying to Write to Read-only register) */
                else if (((nRegAddr >= ADI_AD7879_RO_REG_START_ADDR) &&
                          (nRegAddr <= ADI_AD7879_RO_REG_END_ADDR)) &&
                         (bReadFlag != true))
                {
                    /* return error */
                    nResult = ADI_DEV_RESULT_ATTEMPT_TO_WRITE_READONLY_REG;
                    break;
                }
#endif

                /* ELSE (Configure AD7879 device registers) */
                if (bReadFlag == true)
                {
                    /* IF (Failed to read selected AD7879 register value) */
                    if ((nResult = SpiAccess(poDevice,
                                             nRegAddr,
                                             &nRegData,
                                             bReadFlag))
                                          != ADI_DEV_RESULT_SUCCESS)
                    {
                        /* exit on error */
                        break;
                    }
                    /* update client table with read data */
                    poSelectiveAccess->Data = nRegData;
                }
                /* ELSE (Configure AD7879 device registers) */
                else
                {
                    /* If (Failed to update selected register value) */
                    if ((nResult = SpiAccess(poDevice,
                                             nRegAddr,
                                             &poSelectiveAccess->Data,
                                             bReadFlag))
                                          != ADI_DEV_RESULT_SUCCESS)
                    {
                        /* exit on error */
                        break;
                    }
                }

                /* IF (Single register access Command) */
                if ((nCommandID == ADI_DEV_CMD_REGISTER_READ)  ||
                    (nCommandID == ADI_DEV_CMD_REGISTER_WRITE))
                {
                    /* Mark as end of register access */
                    nRegAddr = ADI_DEV_REGEND;
                }
                /* ELSE (Multiple register access) */
                else
                {
                    /* Move to next Register Access pair */
                    poSelectiveAccess++;
                    /* Get the next register address in the client list */
                    nRegAddr = poSelectiveAccess->Address;
                }
            }

            break;

        /* CASE (Read a specific AD7879 register field value) */
        case ADI_DEV_CMD_REGISTER_FIELD_READ:
        /* CASE (Configure a specific AD7879 register field) */
        case ADI_DEV_CMD_REGISTER_FIELD_WRITE:
        /* CASE (Read a table of AD7879 register(s) fields) */
        case ADI_DEV_CMD_REGISTER_FIELD_TABLE_READ:
        /* CASE (Configure a table of AD7879 register(s) fields) */
        case ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE:

            /* Pointer to register field access table */
            poFieldAccess = (ADI_DEV_ACCESS_REGISTER_FIELD *) pCommandValue;
            /* Load the first register address in the table to access */
            nRegAddr = poFieldAccess->Address;

            /* Continue until we reach end of register access */
            while (nRegAddr != ADI_DEV_REGEND)
            {

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)

                /* IF (AD7879 register address to access is invalid OR
                       Field access other than Control Registers 1,2 and 3) */
                if ((nRegAddr == ADI_AD7879_INVALID_REG1) ||
                    (nRegAddr > AD7879_REG_CONTROL_3))
                {
                    /* return error */
                    /* only Control Register 1,2 and 3 have individual fields to access */
                    nResult = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
                    break;
                }

#endif

                /* IF (Access Control Register 1 field) */
                if(nRegAddr == AD7879_REG_CONTROL_1)
                {
                    /* update value from internal cache */
                    nRegData = poDevice->nControlReg1;
                }
                /* ELSE IF (Access Control Register 2 field) */
                else if(nRegAddr == AD7879_REG_CONTROL_2)
                {
                    /* update value from internal cache */
                    nRegData = poDevice->nControlReg2;
                }
                /* ELSE (Access Control Register 3 field) */
                else
                {
                    /* update value from internal cache */
                    nRegData = poDevice->nControlReg3;
                }

                /* 'nRegData' holds the present value of AD7879 register to be accessed */
                /* Mask the register field we're interested in */
                MaskField(nRegAddr,
                          &nRegData,
                          poFieldAccess->Field,
                          bReadFlag);

                /* IF (Read AD7879 register field) */
                if (bReadFlag == true)
                {
                    /* Update client table with read data */
                    poFieldAccess->Data = nRegData;
                }
                /* ELSE (Configure AD7879 field) */
                else
                {
                    /* Update register field with new value */
                    nRegData |= (poFieldAccess->Data << poFieldAccess->Field);

                    /* IF (Failed to update selected register field with new value) */
                    if ((nResult = SpiAccess(poDevice,
                                             nRegAddr,
                                             &nRegData,
                                             bReadFlag))
                                          != ADI_DEV_RESULT_SUCCESS)
                    {
                        /* exit on error */
                        break;
                    }
                }

                /* IF (Single register access Command) */
                if ((nCommandID == ADI_DEV_CMD_REGISTER_FIELD_READ) ||
                    (nCommandID == ADI_DEV_CMD_REGISTER_FIELD_WRITE))
                {
                    /* Mark as end of register access */
                    nRegAddr = ADI_DEV_REGEND;
                }
                /* ELSE (Multiple register access) */
                else
                {
                    /* Move to next Register Field Access pair */
                    poFieldAccess++;
                    /* get the next register address to be accessed */
                    nRegAddr = poFieldAccess->Address;
                }
            }
            break;

        /* DEFAULT (this will never occur) */
        default:
            break;
    }

    return (nResult);
}

/*********************************************************************

    Function: MaskField

     Handles AD7879 register field masking operations for
     register field access commands

     Write mode => Clears the present value of the register
                   field and updates it with provided value

     Read mode  => Extracts the value of register field we're
                   interested in

    Parameters:
     nRegAddr     - Register Address to access
     pnRegData    - Pointer to location holding present data in the register
     nRegField    - Register Field location to access
     bReadFlag    - true to read register field, false to configure

    Returns:
     None

*********************************************************************/
static void MaskField(
    u16     nRegAddr,
    u16     *pnRegData,
    u16     nRegField,
    u8      bReadFlag
)
{
    /* register mask */
    u16     nRegMask;
    /* shift count to reach register field */
    u16     nShiftCount;
    /* register field length - initialise with minimum possible value */
    u16     nFieldLength = 1U;

    /* CASEOF (Register Address) */
    switch (nRegAddr)
    {
        /* CASE: Control Register 1 */
        case AD7879_REG_CONTROL_1:
            nRegMask = ADI_AD7879_CTRL_REG1_FIELD_MASK;
            break;

        /* CASE: Control Register 2 */
        case AD7879_REG_CONTROL_2:
            nRegMask = ADI_AD7879_CTRL_REG2_FIELD_MASK;
            break;

        /* CASE: Control Register 3 */
        case AD7879_REG_CONTROL_3:
            nRegMask = ADI_AD7879_CTRL_REG3_FIELD_MASK;
            break;

        /* default: */
        default:
            nRegMask = ADI_AD7879_DEFAULT_FIELD_MASK;
            break;
    }

    /* Shift count to reach the register field */
    nShiftCount = nRegField;
    /* Get mask value for the field we're interested in */
    nRegMask >>= (nRegField + 1U);

    /* Calculate the register field length and shift count */
    while ((!(nRegMask & 1U)) && (nShiftCount < ADI_AD7879_BITS_PER_REGISTER))
    {
        /* increment register field length */
        nFieldLength++;
        /* Move mask by one until we reach last bit in this field */
        nRegMask >>= 1U;
        /* Increment Shift count to reach this field */
        nShiftCount++;
    }

    /* Reset mask */
    nRegMask = 0U;

    /* IF (this field is not bit 0 in the register) */
    if (nRegField)
    {
        /* Shift register field times to mask bits after the register field to be accessed */
        nRegMask = (ADI_AD7879_DEFAULT_FIELD_MASK >> (ADI_AD7879_BITS_PER_REGISTER - nRegField));
    }

    /* Mask bits present before the register field to be accessed */
    nRegMask |= (ADI_AD7879_DEFAULT_FIELD_MASK << (nRegField + nFieldLength));

    /* IF (Read register field) */
    if (bReadFlag == true)
    {
        /* Clear register fields other than the one we're interested in */
        *pnRegData = (*pnRegData & ~nRegMask);
        /* Extract register field data */
        *pnRegData >>= nRegField;
    }
    /* ELSE (Update register field) */
    else
    {
        /* clear present register field value */
        *pnRegData = *pnRegData & nRegMask;
    }
}

/*********************************************************************

    Function: SpiAccess

     Accesses AD7879 device registers via SPI by preparing and
     submitting SPI buffers

    Parameters:
     poDevice     - Pointer to the device we're working on
     nRegAddr     - Register Address to access
     pnRegData    - Pointer to location holding register data
     bReadFlag    - true to read register field, false to configure

    Returns:
     - ADI_DEV_RESULT_SUCCESS when success
     - Result code returned by SPI driver

*********************************************************************/
static u32 SpiAccess(
    ADI_DD_AD7879_DEF   *poDevice,
    u16                 nRegAddr,
    u16                 *pnRegData,
    u8                  bReadFlag
)
{
    /* Return value */
    u32     nResult;
    /* array to hold data to be sent out by SPI    */
    u16     aSpiData[2];

    /* 1D buffers for SPI access  */
    ADI_DEV_SEQ_1D_BUFFER   aoSpiSeqBuf[2];

    /*
    ** Initialise SPI data buffers
    */

    /* SPI Data pointer */
    aoSpiSeqBuf[0].Buffer.Data                  = (void *)&aSpiData[0];
    /* AD7879 data packet is always 2 bytes */
    aoSpiSeqBuf[0].Buffer.ElementCount          = 2U;
    aoSpiSeqBuf[0].Buffer.ElementWidth          = 1U;
    aoSpiSeqBuf[0].Buffer.CallbackParameter     = NULL;
    aoSpiSeqBuf[0].Buffer.pNext                 = &aoSpiSeqBuf[1].Buffer;
    aoSpiSeqBuf[0].Direction                    = ADI_DEV_DIRECTION_OUTBOUND;

    /* 2nd buffer can be an inbound or outbound depending on
       data access mode (bReadFlag status) */
    aoSpiSeqBuf[1].Buffer.Data                  = (void *)&aSpiData[1];
    aoSpiSeqBuf[1].Buffer.ElementCount          = 2U;
    aoSpiSeqBuf[1].Buffer.ElementWidth          = 1U;
    aoSpiSeqBuf[1].Buffer.CallbackParameter     = &aoSpiSeqBuf[0].Buffer;
    aoSpiSeqBuf[1].Buffer.pNext                 = NULL;

    /* IF (Successfully opened and configured SPI) */
    if((nResult = SpiOpen(poDevice)) == ADI_DEV_RESULT_SUCCESS)
    {
        /* IF (Read AD7879 register) */
        if (bReadFlag == true)
        {
            /* Inbound data flow */
            aoSpiSeqBuf[1].Direction = ADI_DEV_DIRECTION_INBOUND;

            /*
            ** Create command read packet
            */

            /* Set r/w bit */
            aSpiData[0] = (ADI_AD7879_REGISTER_READ | nRegAddr);
            /* Clear register data location in SPI data array */
            aSpiData[1] = 0U;

            /* Submit the SEQ 1D buffers to the SPI */
            nResult = adi_dev_SequentialIO(poDevice->hSpiDriver,
                                           ADI_DEV_SEQ_1D,
                                           (ADI_DEV_BUFFER *)&aoSpiSeqBuf[0]);
        }
        /* ELSE (Configure to AD7879 register) */
        else
        {
            /* Outbound data flow */
            aoSpiSeqBuf[1].Direction = ADI_DEV_DIRECTION_OUTBOUND;

            /*
            ** Create command write and register data packet
            */

            /* Clear r/w bit */
            aSpiData[0] = (ADI_AD7879_REGISTER_WRITE | nRegAddr);
            /* Register value to configure */
            aSpiData[1] = (*pnRegData & ADI_AD7879_REG_VALUE_MASK);

            /*
            ** Update driver register cache
            */

            /* IF (configure Control Reg 1) */
            if (nRegAddr == AD7879_REG_CONTROL_1)
            {
                poDevice->nControlReg1    = aSpiData[1];
            }
            /* ELSE IF (configure Control Reg 2) */
            else if (nRegAddr == AD7879_REG_CONTROL_2)
            {
                poDevice->nControlReg2    = aSpiData[1];
            }
            /* ELSE (Configure Control Reg 3) */
            else
            {
                poDevice->nControlReg3    = aSpiData[1];
            }

            /* Submit the SEQ 1D outbound buffer to the SPI */
            nResult = adi_dev_SequentialIO(poDevice->hSpiDriver,
                                           ADI_DEV_SEQ_1D,
                                           (ADI_DEV_BUFFER *)&aoSpiSeqBuf[0]);
        }

        /* IF (Successfully queued AD7879 access buffers to SPI) */
        if (nResult == ADI_DEV_RESULT_SUCCESS);
        {
            /* Control SPI slave device to transmit data */
            nResult = SpiSlaveSelect(poDevice);

            /* IF (Successfully read AD7879 register) */
            if (nResult == ADI_DEV_RESULT_SUCCESS &&
                (bReadFlag == true))
            {

                /* Copy register data to provided location */
                *pnRegData = aSpiData[1];
            }
        }

        /* Close SPI device */
        nResult = adi_dev_Close(poDevice->hSpiDriver);

    }

    return (nResult);
}

/*********************************************************************

    Function: SpiSlaveSelect

     Controls SPI slave select line to access AD7879 registers

    Parameters:
     poDevice     - Pointer to the device we're working on

    Returns:
     - ADI_DEV_RESULT_SUCCESS when success
     - Result code returned by SPI driver

*********************************************************************/
static u32  SpiSlaveSelect(
    ADI_DD_AD7879_DEF   *poDevice
)
{
    /* Return value */
    u32 nResult;

    /* IF (Successfully Activated AD7879 SPI chip select line) */
    if ((nResult = adi_dev_Control(poDevice->hSpiDriver,
                                   ADI_SPI_CMD_ENABLE_SLAVE_SELECT,
                                   (void*)poDevice->nSpiChipSelect))
                                == ADI_DEV_RESULT_SUCCESS)
    {
        /* IF (Successfully driven AD7879 chipselect line to low) */
        if ((nResult = adi_dev_Control(poDevice->hSpiDriver,
                                       ADI_SPI_CMD_SELECT_SLAVE,
                                       (void*)poDevice->nSpiChipSelect))
                                    == ADI_DEV_RESULT_SUCCESS)
        {
            /* IF (Successfully enabled SPI data flow) */
            if ((nResult = adi_dev_Control(poDevice->hSpiDriver,
                                           ADI_DEV_CMD_SET_DATAFLOW,
                                           (void *)true))
                                        == ADI_DEV_RESULT_SUCCESS)
            {
                /* wait untill SPI is done with AD7879 register access */
                /* SPI Callback function will set 'bIsSpiTransferComplete'
                   when it is done with AD7879 register access */
                while(!bIsSpiTransferComplete)
                {
                    asm("nop;");
                }

                /* clear the semaphore for next SPI access */
                bIsSpiTransferComplete = false;

                /* IF (Successfully de-selected SPI device after
                       processing all SPI buffers queued) */
                if((nResult = adi_dev_Control(poDevice->hSpiDriver,
                                              ADI_SPI_CMD_DESELECT_SLAVE,
                                              (void*)poDevice->nSpiChipSelect))
                                           ==ADI_DEV_RESULT_SUCCESS)
                {
                    /* IF (Successfully disabled SPI data flow) */
                    if ((nResult = adi_dev_Control(poDevice->hSpiDriver,
                                                   ADI_DEV_CMD_SET_DATAFLOW,
                                                   (void *)false)) == ADI_DEV_RESULT_SUCCESS)
                    {

                        /* Deactivate AD7879 SPI chip select line */
                        nResult = adi_dev_Control(poDevice->hSpiDriver,
                                                  ADI_SPI_CMD_DISABLE_SLAVE_SELECT,
                                                  (void*)poDevice->nSpiChipSelect);

                    } /* End of if (Successfully disabled SPI data flow) */

                } /* End of if (Successfully de-selected SPI device) */

            } /* End of if (Successfully enabled SPI data flow) */

        } /* End of if (Successfully driven AD7879 chipselect line to low) */

    } /* End of if (Successfully Activated AD7879 SPI chip select line) */

    return (nResult);
}

/*********************************************************************

    Function: SpiOpen

     Opens the SPI device to access AD7879 registers

    Parameters:
     poDevice     - Pointer to the device we're working on

    Returns:
     - ADI_DEV_RESULT_SUCCESS when success
     - Result code returned by SPI driver

*********************************************************************/
static u32 SpiOpen(
    ADI_DD_AD7879_DEF   *poDevice
)
{

    /* Return value */
    u32 nResult;

    /*
    ** SPI configuration table to access AD7879 registers
    */

    /* Baud Rate, Active low SCLK, chained dataflow, Blackfin SPI as Master,
       16bit SPI data packet, Hardware controlled chip select, Activate AD7879 chip select,
       Enable SPI dataflow */
    ADI_DEV_CMD_VALUE_PAIR aoSpiConfig[] =
    {
        /* Baud rate to meet all possible SCLK values                       */
        { ADI_SPI_CMD_SET_BAUD_REG,         (void *)ADI_AD7879_SPI_BAUD     },
        /* Active High SCLK                                                 */
        { ADI_SPI_CMD_SET_CLOCK_POLARITY,   (void *)1U                      },
        /* Blackfin always as Master                                        */
        { ADI_SPI_CMD_SET_MASTER,           (void *)true                    },
        /* 16-bit transfers                                                 */
        { ADI_SPI_CMD_SET_WORD_SIZE,        (void *)16U                     },
        /* Software controlled chipselect                                   */
        { ADI_SPI_CMD_SET_CLOCK_PHASE,      (void *)1U                      },
        /* Send Zeros                                                       */
        { ADI_SPI_CMD_SEND_ZEROS,           (void *)1U                      },
        /* Sequential Chained Dataflow method                               */
        { ADI_DEV_CMD_SET_DATAFLOW_METHOD,  (void *)ADI_DEV_MODE_SEQ_CHAINED},
        /* End of configuration table                                       */
        { ADI_DEV_CMD_END,                  (void *)NULL                    }
    };

    /* Open SPI device in interrupt driven mode */
    nResult = adi_dev_Open(poDevice->hDeviceManager,
                           &ADISPIIntEntryPoint,
                           poDevice->nSpiDevNumber,
                           NULL,
                           &poDevice->hSpiDriver,
                           ADI_DEV_DIRECTION_BIDIRECTIONAL,
                           NULL,
                           poDevice->hDcbManager,
                           AD7879SpiCallback
                          );

    /* IF (Successfully opened SPI device) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Configure SPI driver to access AD7879 registers */
        nResult = adi_dev_Control(poDevice->hSpiDriver,
                                  ADI_DEV_CMD_TABLE,
                                  (void*)aoSpiConfig);
    }

    return (nResult);

}

/*********************************************************************

    Function: AD7879SpiCallback

     SPI callback routine

    Parameters:
     phClient       - Client handle passed to SPI
     nEvent         - Callback Event
     pArg           - Callback Argument

    Returns:
     None
*********************************************************************/
static void AD7879SpiCallback (
    void    *phClient,
    u32     nEvent,
    void    *pArg
)
{
    /* Pointer to the AD7879 device instant to which this callback belongs */
    ADI_DD_AD7879_DEF *poDevice = (ADI_DD_AD7879_DEF *)phClient;

    /* CASE OF (Callback Event) */
    switch (nEvent)
    {
        /* CASE (Buffer processed) */
        case ADI_SPI_EVENT_WRITE_BUFFER_PROCESSED:
        case ADI_SPI_EVENT_READ_BUFFER_PROCESSED:
        case ADI_SPI_EVENT_SEQ_BUFFER_PROCESSED:
            /* Mark as SPI data transfer complete */
            bIsSpiTransferComplete = true;
            break;

        default:
            break;
    }
}

#endif /*ADI_IMPLEMENT_AD7879_1 */

/* Debug build only */
#if defined(ADI_DEV_DEBUG)

/*********************************************************************

    Function:       ValidatePDDHandle

     Validates a PDD handle

    Parameters:
     hPhysicalDevice    - Physical Device handle to AD7879 device instance

    Returns:
     - ADI_DEV_RESULT_SUCCESS on success
     - ADI_DEV_RESULT_BAD_PDD_HANDLE when the given device handle is invalid
*********************************************************************/
static u32 ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice
)
{
    /* Return value - insure the client has provided a valid PDD Handle */
    u32     nResult = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    /* Device index */
    u32     nIndex;

    /* compare the given PDD handle with PDDHandle allocated to
       all AD7879 devices in the list */
    for (nIndex = 0U;
         nIndex < ADI_AD7879_NUM_DEVICES;
         nIndex++)
    {
        /* IF (we have a matching device handle form the instance array) */
        if (hPhysicalDevice == (ADI_DEV_PDD_HANDLE)&_gaoAd7879Device[nIndex])
        {
            /* Given Physical Device Driver Handle is valid
               quit this loop */
            nResult = ADI_DEV_RESULT_SUCCESS;
            break;
        }
    }

    return (nResult);
}

#endif

/*****/

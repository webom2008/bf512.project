/*****************************************************************************
Copyright (c), 2002-2009 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

$RCSfile: adi_lq035q1dh02.c,v $
$Revision: 3492 $
$Date: 2010-10-07 09:34:20 -0400 (Thu, 07 Oct 2010) $

Title: Sharp LQ035Q1DH02 LCD driver

Description: This is the primary source file for Sharp LQ035Q1DH02 LCD driver

*****************************************************************************/

/*=============  I N C L U D E S   =============*/

/* system service includes */
#include <services/services.h>

/* device manager includes */
#include <drivers/adi_dev.h>

/* IF (ADSP-BF52x/ADSP-BF53x/ADSP-BF56x device) */
#if defined (__ADSP_EDINBURGH__)    ||\
    defined (__ADSP_BRAEMAR__)      ||\
    defined (__ADSP_STIRLING__)     ||\
    defined (__ADSP_TETON__)        ||\
    defined (__ADSP_KOOKABURRA__)   ||\
    defined (__ADSP_MOCKINGBIRD__)  ||\
    defined (__ADSP_BRODIE__)
/* PPI driver includes */
#include <drivers/ppi/adi_ppi.h>
/* ELSE IF (ADSP-BF54x device) */
#elif defined (__ADSP_MOAB__)
/* EPPI driver includes */
#include <drivers/eppi/adi_eppi.h>
/* ELSE (other processors) */
#else
#error "*** ERROR: Driver does not support this processor ***"
#endif
/* SPI driver includes */
#include <drivers/spi/adi_spi.h>
/* SHARP LQ035Q1DH02 LCD driver includes*/
#include <drivers/lcd/sharp/adi_lq035q1dh02.h>

/*=============  D E F I N E S  =============*/

/*
** ADI_DD_LQ035Q1DH02_NUM_DEVICES
**  - Number of Sharp LQ035Q1DH02 LCD(s) in the system
*/
#define ADI_DD_LQ035Q1DH02_NUM_DEVICES      (sizeof(__gaLq035q1dh02Device)/\
                                            sizeof(__gaLq035q1dh02Device[0]))
/*
** ADI_DD_LQ035Q1DH02_OPERATING_FREQUENCY
**  - Sharp LQ035Q1DH02 LCD Operating Frequency (5MHz)
*/
#define ADI_DD_LQ035Q1DH02_OPERATING_FREQ           5000000U
/*
**  ADI_DD_LQ035Q1DH02_PARM_VALUE_ZERO
**   - To clear a register content or to pass zero as parameter
*/
#define ADI_DD_LQ035Q1DH02_PARM_VALUE_ZERO          0U
/*
**  ADI_DD_LQ035Q1DH02_SPI_NUM_INVALID
**   - Invalid SPI device number
*/
#define ADI_DD_LQ035Q1DH02_SPI_NUM_INVALID          0xFFU
/*
**  ADI_DD_LQ035Q1DH02_SPI_CS_INVALID
**   - Invalid SPI chip select
*/
#define ADI_DD_LQ035Q1DH02_SPI_CS_INVALID           0U
/*
**  ADI_DD_LQ035Q1DH02_LCD_REG_ADDR_BYTE_0
**   - First byte to transmit while sending LCD register address
*/
#define ADI_DD_LQ035Q1DH02_LCD_REG_ADDR_BYTE_0      0x74U
/*
**  ADI_DD_LQ035Q1DH02_LCD_REG_ADDR_BYTE_1
**   - Second byte to transmit while sending LCD register address
*/
#define ADI_DD_LQ035Q1DH02_LCD_REG_ADDR_BYTE_1      0U
/*
**  ADI_DD_LQ035Q1DH02_LCD_REG_DATA_BYTE_0
**   - First byte to transmit while sending LCD register data
*/
#define ADI_DD_LQ035Q1DH02_LCD_REG_DATA_BYTE_0      0x76U
/*
** ADI_DD_LQ035Q1DH02_LCD_REG_WRITE
**  - Write to LCD register
*/
#define ADI_DD_LQ035Q1DH02_LCD_REG_WRITE            0U
/*
** ADI_DD_LQ035Q1DH02_LCD_REG_READ
**  - Read from LCD register
*/
#define ADI_DD_LQ035Q1DH02_LCD_REG_READ             1U
/*
** ADI_DD_LQ035Q1DH02_ACTIVE_PIXELS_PER_LINE
**  - Active Pixels Per Line (constant)
**
*/
#define ADI_DD_LQ035Q1DH02_ACTIVE_PIXELS_PER_LINE   320U
/*
** ADI_DD_LQ035Q1DH02_ACTIVE_LINES_PER_FRAME
**  - Active Lines Per Frame (constant)
**
*/
#define ADI_DD_LQ035Q1DH02_ACTIVE_LINES_PER_FRAME   240U
/*
** ADI_DD_LQ035Q1DH02_HFP
**  - Horizontal Front Porch (constant)
*/
#define ADI_DD_LQ035Q1DH02_HFP                      8U
/*
** ADI_DD_LQ035Q1DH02_VFP
**  - Vertical Front Porch (constant)
*/
#define ADI_DD_LQ035Q1DH02_VFP                      2U

/*
**
** LCD hardware register address
**
*/

/*
** ADI_DD_LQ035Q1DH02_DRIVER_OUT_CTRL
**  - Driver Output Control register address
*/
#define ADI_DD_LQ035Q1DH02_DRIVER_OUT_CTRL          0x01U
/*
** ADI_DD_LQ035Q1DH02_SHUT_8_COLOR
**  - Shut and 8-color register address
*/
#define ADI_DD_LQ035Q1DH02_SHUT_8_COLOR             0x11U
/*
** ADI_DD_LQ035Q1DH02_ENTRY_MODE
**  - Entry Mode register address
*/
#define ADI_DD_LQ035Q1DH02_ENTRY_MODE               0x12U
/*
** ADI_DD_LQ035Q1DH02_PIXEL_PER_LINE
**  - Pixel Per Line register address
*/
#define ADI_DD_LQ035Q1DH02_PIXEL_PER_LINE           0x16U
/*
** ADI_DD_LQ035Q1DH02_VERTICAL_PORCH
**  - Vertical Porch register address
*/
#define ADI_DD_LQ035Q1DH02_VERTICAL_PORCH           0x17U

/*
**
** LCD hardware register default values
**
*/

/*
** ADI_DD_LQ035Q1DH02_DRIVER_OUT_CTRL_DEFAULT
**  - Driver Output Control register default value
*/
#define ADI_DD_LQ035Q1DH02_DRIVER_OUT_CTRL_DEFAULT  0x22EFU
/*
** ADI_DD_LQ035Q1DH02_SHUT_8_COLOR_DEFAULT
**  - Shut and 8-color register address
*/
#define ADI_DD_LQ035Q1DH02_SHUT_8_COLOR_DEFAULT     0x0000U
/*
** ADI_DD_LQ035Q1DH02_ENTRY_MODE_DEFAULT
**  - Entry Mode register address
*/
#define ADI_DD_LQ035Q1DH02_ENTRY_MODE_DEFAULT       0x0064U
/*
** ADI_DD_LQ035Q1DH02_PIXEL_PER_LINE_DEFAULT
**  - Pixel Per Line register address
*/
#define ADI_DD_LQ035Q1DH02_PIXEL_PER_LINE_DEFAULT   0x9F86U
/*
** ADI_DD_LQ035Q1DH02_VERTICAL_PORCH_DEFAULT
**  - Vertical Porch register address
*/
#define ADI_DD_LQ035Q1DH02_VERTICAL_PORCH_DEFAULT   0x0002U

/*
**
** LCD hardware register field masks
**
*/

/*
** ADI_DD_LQ035Q1DH02_REV_MASK
**  - Driver Output Control register Reversal Mask
*/
#define ADI_DD_LQ035Q1DH02_REV_MASK                 0x2000U
/*
** ADI_DD_LQ035Q1DH02_TB_MASK
**  - Driver Output Control register Top to Bottom Mask
*/
#define ADI_DD_LQ035Q1DH02_TB_MASK                  0x0200U
/*
** ADI_DD_LQ035Q1DH02_RL_MASK
**  - Driver Output Control Right to Left Mask
*/
#define ADI_DD_LQ035Q1DH02_RL_MASK                  0x0100U
/*
** ADI_DD_LQ035Q1DH02_CM_MASK
**  - Shut and 8-color Color Mode mask
*/
#define ADI_DD_LQ035Q1DH02_CM_MASK                  0x0100U
/*
** ADI_DD_LQ035Q1DH02_SHUT_MASK
**  - Shut and 8-color Sleep (SHUT) mask
*/
#define ADI_DD_LQ035Q1DH02_SHUT_MASK                0x0001U
/*
** ADI_DD_LQ035Q1DH02_VALID_PIX_PER_LINE_MASK
**  - Pixel Per Line register Valid Pixel per line mask
*/
#define ADI_DD_LQ035Q1DH02_VALID_PIX_PER_LINE_MASK  0xFF80U
/*
** ADI_DD_LQ035Q1DH02_HBP_MASK
**  - Dummy clocks per line or Horizontal Back Porch (HBP) mask
*/
#define ADI_DD_LQ035Q1DH02_HBP_MASK                 0x003FU
/*
** ADI_DD_LQ035Q1DH02_VBP_MASK
**  - Dummy lines per frame or Vertical Back Porch (VBP) mask
*/
#define ADI_DD_LQ035Q1DH02_VBP_MASK                 0x00FFU

/*
**
** Shift count to reach a particular field in a LCD register
**
*/
/*
** ADI_DD_LQ035Q1DH02_PIX_PER_LINE_SHIFT
**  - Shift count to reach valid Pixel Per Line field in
**    Pixel Per Line register
*/
#define ADI_DD_LQ035Q1DH02_PIX_PER_LINE_SHIFT       7U

/*
**
** LCD hardware register field minimum and maximum acceptable values
**
*/
/*
** ADI_DD_LQ035Q1DH02_PIX_PER_LINE_MIN_VAL
**  - Minimum Pixels per line value supported
*/
#define ADI_DD_LQ035Q1DH02_PIX_PER_LINE_MIN_VAL     1U
/*
** ADI_DD_LQ035Q1DH02_PIX_PER_LINE_MAX_VAL
**  - Maximum Pixels per line value supported
*/
#define ADI_DD_LQ035Q1DH02_PIX_PER_LINE_MAX_VAL     320U
/*
** ADI_DD_LQ035Q1DH02_HBP_MIN_VAL
**  - Minimum HBP value supported
*/
#define ADI_DD_LQ035Q1DH02_HBP_MIN_VAL              2U
/*
** ADI_DD_LQ035Q1DH02_HBP_MAX_VAL
**  - Maximum HBP value supported
*/
#define ADI_DD_LQ035Q1DH02_HBP_MAX_VAL              65U
/*
** ADI_DD_LQ035Q1DH02_VBP_MIN_VAL
**  - Minimum VBP value supported
*/
#define ADI_DD_LQ035Q1DH02_VBP_MIN_VAL              1U
/*
** ADI_DD_LQ035Q1DH02_VBP_MAX_VAL
**  - Maximum VBP value supported
*/
#define ADI_DD_LQ035Q1DH02_VBP_MAX_VAL              240U

/*
**
** SPI configuration macros
**
*/

/*
** ADI_DD_LQ035Q1DH02_SPI_BAUD
**  - SPI Baud rate to support all possible SCLK frequencies
*/
#define ADI_DD_LQ035Q1DH02_SPI_BAUD                 0x1FFU
/*
** ADI_DD_LQ035Q1DH02_SPI_WORD_SIZE
**  - SPI transfer word size in bits
*/
#define ADI_DD_LQ035Q1DH02_SPI_WORD_SIZE            8U

/*
**
** PPI register default values
**
*/
/* IF (ADSP-BF561) */
#if defined (__ADSP_TETON__)

/*
** ADI_DD_LQ035Q1DH02_PPI_CTRL_DEFAULT
**  - PPI Control register default value
**    PPI as output, 32-Bit DMA enabled, 2 or 3 syncs,
**    16bit data, FS1 & FS2 treated as falling edge asserted
*/
#define ADI_DD_LQ035Q1DH02_PPI_CTRL_DEFAULT         0xB91EU

/* ELSE IF (ADSP-BF54x) */
#elif defined (__ADSP_MOAB__)

/*
** ADI_DD_LQ035Q1DH02_PPI_CTRL_DEFAULT
**  - EPPI Control register default value
**    EPPI transmit, GP Transfer mode, GP2 mode, External clock generation,
**    Internal FS generation, Tx raising edge, FS1 & FS2 active low,
**    16 bit data, Skip disabled, Packing enabled, Swap disabled,
**    one DMA channel mode, RGB formatting disabled,
**    regular watermark = 75% full, urgent watermark = 25% full
*/
#define ADI_DD_LQ035Q1DH02_PPI_CTRL_DEFAULT         0x68126C2EU

/* ELSE (ADSP-BF52x/BF53x) */
#else

/*
** ADI_DD_LQ035Q1DH02_PPI_CTRL_DEFAULT
**  - PPI Control register default value
**    PPI as output, 2 or 3 syncs, 16bit data,
**    FS1 & FS2 treated as falling edge asserted
**
**    NOTE: these values are suitable for the LCD Extender card, not
**          the on-board Sharp LCD on the later BF527 EZ-Kits.
**          For the EZ-Kit, use ADI_LQ035Q1DH02_CMD_SET_PPI_CTL_VALUE
**          with an operand of (void*)0x809EU.
*/
#define ADI_DD_LQ035Q1DH02_PPI_CTRL_DEFAULT         0xB81EU

#endif

/*
** ADI_DD_LQ035Q1DH02_PPI_CLK_PER_DOT_DEFAULT
**  - On later BF527 EZ-Kits the LCD is driven from an FPGA rather than
**    directly by the PPI.  The FPGA clocks the LCD at a different rate
**    and this affects some of the timings of sync pulses and the like.
**    The driver was originally written for the LCD_Extender card on
**    which the LCD is directly driven by the PPI, hence the default
**    ratio is 1.  For the BF527 EZ-Kit use command
**    ADI_LQ035Q1DH02_CMD_SET_PPI_CLK_PER_DOT with an operand of (void*)3.
*/
#define ADI_DD_LQ035Q1DH02_PPI_CLK_PER_DOT_DEFAULT  1


/*
** ADI_DD_LQ035Q1DH02_REG_ACCESS
**  - Sharp LQ035Q1DH02 LCD access structure
*/
typedef struct __AdiDDLq035q1dh02RegAccess
{

    /* uint: nRegAddr
        - LCD register address to access */
    u8      nRegAddr;

    /* uint: nRegData
        - LCD register data */
    u16     nRegData;

} ADI_DD_LQ035Q1DH02_REG_ACCESS;

/*
** ADI_DD_LQ035Q1DH02_REGS
**  - Sharp LQ035Q1DH02 LCD device register cache
*/
#pragma pack(4)
typedef struct __AdiDDLq035q1dh02Regs
{

    /* uint: nDriverOutControl
        - LCD Driver Output Control register */
    u16     nDriverOutControl;

    /* uint: nShutAndColor
        - Shut and 8-Color register */
    u16     nShutAndColor;

    /* uint: nPixelPerLine
        - Pixels Per Line register */
    u16     nPixelPerLine;

    /* uint: nVerticalPorch
        Vertical Porch Register */
    u16     nVerticalPorch;

}ADI_DD_LQ035Q1DH02_REGS;

/*
** ADI_DD_LQ035Q1DH02_DEF
**  - Sharp LQ035Q1DH02 LCD Device instance structure
*/
#pragma pack(4)
typedef struct __AdiDDLq035q1dh02Def
{

    /* Variable: bIsDeviceInUse
        - 'true' when device is in use, 'false' when device is free */
    bool                        bIsDeviceInUse;

    /* Variable: bIsLcdInitialised
        - 'true' when LCD registers are initialised,
          'false' when they are yet to be initialised */
    bool                        bIsLcdInitialised;

    /* Variable: bIsAutoPpiControlEnabled
        - 'true' when Automatic PPI control register configuration is enabled
          'false' when Automatic PPI control register configuration is disabled */
    bool                        bIsAutoPpiControlEnabled;

    /* Variable: nSpiDeviceNumber
        - SPI Device number used to configure LCD registers */
    u8                          nSpiDeviceNumber;

    /* Variable: nSpiChipSelect
        - SPI Chipselect number used to select LCD device */
    u8                          nSpiChipSelect;

    /* Variable: nPPICtlReg
        - Value to use for PPI control register
        (differs between BF527 EZ-Kit and LCD Extender card) */
#if defined (__ADSP_MOAB__)
    u32                         nPPICtlReg;
#else
    u16                         nPPICtlReg;
#endif

    /* Variable: nPPIClkPerDot
        - Ratio of PPI clocks to LCD clocks
        (3 on BF527 EZ-Kit, 1 on LCD Extender card) */
    u16                         nPPIClkPerDot;

    /* Variable: oLcdRegs
        - LCD register cache */
    ADI_DD_LQ035Q1DH02_REGS     oLcdRegs;

    /* Variable: hSpiDevice
        - Handle to the underlying SPI device driver */
    ADI_DEV_PDD_HANDLE          hSpiDevice;

    /* Variable: hSpiDataSemaphore
        - Semaphore to Handle SPI Dataflow */
    ADI_SEM_HANDLE              hSpiDataSemaphore;

    /* Variable: hPpiDevice
        - Handle to the underlying PPI device driver */
    ADI_DEV_PDD_HANDLE          hPpiDevice;

    /* Variable: hDeviceManager
        - Device Manager Handle */
    ADI_DEV_MANAGER_HANDLE      hDeviceManager;

    /* Variable: hDeviceManager
        - Device Handle allocated by the Device Manager */
    ADI_DEV_DEVICE_HANDLE       hDevice;

    /* Variable: hDmaManager
        - DMA Manager Handle */
    ADI_DMA_MANAGER_HANDLE      hDmaManager;

    /* Variable: hDcbManager
        - Deferred Callback Manager Handle */
    ADI_DCB_HANDLE              hDcbManager;

    /* Variable: pfDMCallback
        - Pointer to callback function supplied by the Device Manager */
    ADI_DCB_CALLBACK_FN         pfDMCallback;

} ADI_DD_LQ035Q1DH02_DEF;
#pragma pack()

/*=============  D A T A  =============*/

/*
** __gaLq035q1dh02Device
**  - Sharp LQ035Q1DH02 LCD Device specific data
*/

static ADI_DD_LQ035Q1DH02_DEF   __gaLq035q1dh02Device[] =
{
    {
        /* Device not in use by default */
        false,
        /* LCD yet to initialised */
        false,
        /* PPI Auto Configuration enabled by default */
        true,
        /* SPI device number invalid by default */
        ADI_DD_LQ035Q1DH02_SPI_NUM_INVALID,
        /* SPI device number no valid */
        ADI_DD_LQ035Q1DH02_SPI_CS_INVALID,
        /* PPI_CONTROL value */
        0,
        /* PPI/LCD clock ratio */
        0,
        {
            /* Driver Output Control Register Default value */
            ADI_DD_LQ035Q1DH02_DRIVER_OUT_CTRL_DEFAULT,
            /* Shut register default (assumption) */
            ADI_DD_LQ035Q1DH02_SHUT_8_COLOR_DEFAULT,
            /* Pixels Per Line default */
            ADI_DD_LQ035Q1DH02_PIXEL_PER_LINE_DEFAULT,
            /* Vertical Porch register default */
            ADI_DD_LQ035Q1DH02_VERTICAL_PORCH_DEFAULT
        },
        /* Clear SPI device handle */
        NULL,
        /* Clear SPI data sempahore handle */
        NULL,
        /* Clear PPI device handle */
        NULL,
        /* Clear Device Manager Handle */
        NULL,
        /* Clear Device Handle allocated by the Device Manager */
        NULL,
        /* Clear DMA Manager Handle */
        NULL,
        /* Clear Deferred Callback Manager Handle */
        NULL,
        /* Clear Pointer to Device Manager supplied callback function */
        NULL
    },
};

/*
**
**  Local Function Prototypes
**
*/

/* Opens Sharp LQ035Q1DH02 device */
static u32  adi_pdd_Open(
    ADI_DEV_MANAGER_HANDLE      hDeviceManager,
    u32                         nDeviceNumber,
    ADI_DEV_DEVICE_HANDLE       hDevice,
    ADI_DEV_PDD_HANDLE          *phPhysicalDevice,
    ADI_DEV_DIRECTION           eDirection,
    void                        *pCriticalRegionArg,
    ADI_DMA_MANAGER_HANDLE      hDmaManager,
    ADI_DCB_HANDLE              hDcbManager,
    ADI_DCB_CALLBACK_FN         pfDMCallback
);

/* Closes Sharp LQ035Q1DH02 device */
static u32  adi_pdd_Close(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice
);

/* Not supported for Sharp LQ035Q1DH02 device */
static u32  adi_pdd_Read(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poRxBuffer
);

/* Writes data/queues outbound buffer to PPI */
static u32  adi_pdd_Write(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poTxBuffer
);

/* Not supported for Sharp LQ035Q1DH02 device */
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

/* Accesses LCD registers via SPI device */
static u32  adi_dd_Lq035q1dh02_SpiAccess(
    ADI_DD_LQ035Q1DH02_DEF      *poDevice,
    u8                          nRegAddr,
    u16                         *pnRegData,
    u8                          nReadFlag
);

/* Opens SPI device connected to LCD SPI Port */
static u32  adi_dd_Lq035q1dh02_SpiOpen(
    ADI_DD_LQ035Q1DH02_DEF      *poDevice
);

/* Initialises LCD registers and
   updates PPI registes to match LCD configuration */
static u32  adi_dd_Lq035q1dh02_InitLcdRegs (
    ADI_DD_LQ035Q1DH02_DEF      *poDevice
);

/* Writes to LCD registers via SPI device */
static u32  adi_dd_Lq035q1dh02_SpiWrite(
    ADI_DD_LQ035Q1DH02_DEF      *poDevice,
    u8                          nRegAddr,
    u16                         nRegData
);

/* Controls SPI slave select line corresponding
   to the LCD device */
static u32  adi_dd_Lq035q1dh02_SpiSlaveSelect(
    ADI_DD_LQ035Q1DH02_DEF      *poDevice
);

/* SPI Callback function */
static void adi_dd_Lq035q1dh02_SpiCallback (
    void                        *phClient,
    u32                         nEvent,
    void                        *pArg
);

/* Configures PPI Frame Sync Registers */
static u32  adi_dd_Lq035q1dh02_PpiFsConfig(
    ADI_DD_LQ035Q1DH02_DEF      *poDevice
);

/* PPI Callback function */
static void adi_dd_Lq035q1dh02_PpiCallback(
    void                        *phClient,
    u32                         nEvent,
    void                        *pArg
);

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)

static u32  adi_dd_Lq035q1dh02_ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice
);

#endif  /* ADI_DEV_DEBUG */

/*
** ADILQ035Q1DH02EntryPoint
**  - Device Manager Entry point for Sharp LQ035Q1DH02 driver
**
*/
ADI_DEV_PDD_ENTRY_POINT     ADILQ035Q1DH02EntryPoint =
{
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control,
    adi_pdd_SequentialIO
};

/*=============  C O D E  =============*/

/*********************************************************************

    Function: adi_pdd_Open

        Opens Sharp LQ035Q1DH02 device

    Parameters:
        hDeviceManager     - Device Manager handle
        nDeviceNumber      - Sharp LQ035Q1DH02 device number to open
        hDevice            - Device Handle allocated by the Device Manager
        phPhysicalDevice   - Pointer to Physical Device Handle
        eDirection         - Data Direction
        pCriticalRegionArg - Critical region parameter
        hDmaManager        - DMA Manager Handle
        hDcbManager        - Deferred Callback Handle
        pfDMCallback       - Pointer to Device Manager supplied
                              Callback function

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully opened device
        ADI_DEV_RESULT_BAD_DEVICE_NUMBER
            - Given device number is invalid
        ADI_DEV_RESULT_BAD_MANAGER_HANDLE
            - Given device manager handle/DMA manager handle invalid
        ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED
            - Data direction is other than outbound
        ADI_DEV_RESULT_DEVICE_IN_USE
            - Selected Device number is already in use

*********************************************************************/
static u32  adi_pdd_Open(
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
    /* Return value*/
    u32                     eResult;
    /* pointer to the device instance we're working on */
    ADI_DD_LQ035Q1DH02_DEF  *poDevice;
    /* Critical region parameter */
    void                    *pExitCriticalRegion;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* IF (Device number is invalid) */
    if (nDeviceNumber >= ADI_DD_LQ035Q1DH02_NUM_DEVICES)
    {
        /* report error (bad device number) */
        eResult = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    }
    /* ELSE IF (DMA manager/Device Manager handle is invalid) */
    else if((hDeviceManager == NULL) ||\
            (hDmaManager == NULL))
    {
        /* report error (Bad Manager Handle) */
        eResult = ADI_DEV_RESULT_BAD_MANAGER_HANDLE;
    }
    /* ELSE IF (Direction other than outbound) */
    else if (eDirection != ADI_DEV_DIRECTION_OUTBOUND)
    {
        /* report error (Data direction not supported) */
        eResult = ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED;
    }
    /* ELSE (Device number, Handles and Direction are Valid. Open device) */
    else
    {

#endif  /* ADI_DEV_DEBUG */

        /* assume that the device client wants is already in use */
        eResult  = ADI_DEV_RESULT_DEVICE_IN_USE;
        /* Get the address of device instance requested by the client */
        poDevice = &__gaLq035q1dh02Device[nDeviceNumber];

        /* Protect this section of code - entering a critical region    */
        pExitCriticalRegion = adi_int_EnterCriticalRegion(pCriticalRegionArg);
        /* Check the device usage status */
        if (poDevice->bIsDeviceInUse == false)
        {
            /* Device not in use. Reserve device for this client */
            poDevice->bIsDeviceInUse = true;
            eResult = ADI_DEV_RESULT_SUCCESS;
        }
        /* Exit the critical region */
        adi_int_ExitCriticalRegion(pExitCriticalRegion);

        /* Continue only when the device requested by
           the client is available for use */
        if (eResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* This Sharp LQ035Q1DH02 LCD device instance is
               reserved for this client */

            /* Configure this Sharp LQ035Q1DH02 LCD device instance */
            /* LCD registers are yet to be initialised */
            poDevice->bIsLcdInitialised         = false;
            /* Automatic PPI Control register Configuration enabled by default */
            poDevice->bIsAutoPpiControlEnabled   = true;
            /* Reset SPI device number as invalid */
            poDevice->nSpiDeviceNumber          = ADI_DD_LQ035Q1DH02_SPI_NUM_INVALID;
            /* Reset SPI chipselect as invalid */
            poDevice->nSpiChipSelect            = ADI_DD_LQ035Q1DH02_SPI_CS_INVALID;
            /* Default PPI control register value */
            poDevice->nPPICtlReg                = ADI_DD_LQ035Q1DH02_PPI_CTRL_DEFAULT;
            /* Default PPI to LCD dot clock ratio */
            poDevice->nPPIClkPerDot             = ADI_DD_LQ035Q1DH02_PPI_CLK_PER_DOT_DEFAULT;
            /* store the Manager handle */
            poDevice->hDeviceManager            = hDeviceManager;
            /* and store the Device Handle allocated by the Device Manager */
            poDevice->hDevice                   = hDevice;
            /* and store the DMA Manager handle */
            poDevice->hDmaManager               = hDmaManager;
            /* and store the DCallback Manager handle */
            poDevice->hDcbManager               = hDcbManager;
            /* and callback function */
            poDevice->pfDMCallback              = pfDMCallback;
            /* clear the SPI device Handle */
            poDevice->hSpiDevice                = NULL;
            /* clear the SPI data semaphore Handle */
            poDevice->hSpiDataSemaphore         = NULL;
            /* clear the PPI device Handle */
            poDevice->hPpiDevice                = NULL;

            /* Reset LCD register cache to default values */
            /* Driver Output Control Register Default value */
            poDevice->oLcdRegs.nDriverOutControl    = ADI_DD_LQ035Q1DH02_DRIVER_OUT_CTRL_DEFAULT;
            /* Shut register default (assumption) */
            poDevice->oLcdRegs.nShutAndColor        = ADI_DD_LQ035Q1DH02_SHUT_8_COLOR_DEFAULT;
            /* Pixels Per Line default */
            poDevice->oLcdRegs.nPixelPerLine        = ADI_DD_LQ035Q1DH02_PIXEL_PER_LINE_DEFAULT;
            /* Vertical Porch register default */
            poDevice->oLcdRegs.nVerticalPorch       = ADI_DD_LQ035Q1DH02_VERTICAL_PORCH_DEFAULT;

            /* save physical device handle to the client supplied location */
            *phPhysicalDevice           = (ADI_DEV_PDD_HANDLE *)poDevice;
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif  /* ADI_DEV_DEBUG */

    /* return */
    return(eResult);
}

/*********************************************************************

    Function: adi_pdd_Close

        Closes Sharp LQ035Q1DH02 device

    Parameters:
        hPhysicalDevice    - Physical Device handle to
                             Sharp LQ035Q1DH02 device instance

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully closed device
        Error code returned from PPI and SPI close functions

*********************************************************************/
static u32  adi_pdd_Close(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice
)
{
    /* Return value - assume we're going to be successful */
    u32                     eResult = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device instance we're working on */
    ADI_DD_LQ035Q1DH02_DEF  *poDevice;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given device handle */
    eResult = adi_dd_Lq035q1dh02_ValidatePDDHandle(hPhysicalDevice);

    /* IF (the device handle is valid) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {

#endif  /* ADI_DEV_DEBUG */

        /* avoid casts */
        poDevice = (ADI_DD_LQ035Q1DH02_DEF *)hPhysicalDevice;

        /* IF (we've a open PPI device) */
        if (poDevice->hPpiDevice != NULL)
        {
            /* Close the PPI device used by this LCD instance */
            eResult = adi_dev_Close(poDevice->hPpiDevice);
        }

        /* IF (successfully closed PPI) */
        if (eResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* mark this Sharp LQ035Q1DH02 device as closed */
            poDevice->bIsDeviceInUse = false;
            /* Clear PPI Device Handle */
            poDevice->hPpiDevice = NULL;
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif  /* ADI_DEV_DEBUG */

    /* return */
    return(eResult);
}

/*********************************************************************

    Function: adi_pdd_Read

        Function not supported as Sharp LQ035Q1DH02 is an outbound device

    Parameters:
        hPhysicalDevice    - Physical Device handle to
                             Sharp LQ035Q1DH02 device instance
        eBufferType        - Buffer type to queue
        poTxBuffer         - Pointer to receive buffer

    Returns:
        ADI_DEV_RESULT_NOT_SUPPORTED
            - Function not supported by this device

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

        Writes data/queues outbound buffer to PPI used by Sharp LQ035Q1DH02

    Parameters:
        hPhysicalDevice    - Physical Device handle to
                             Sharp LQ035Q1DH02 device instance
        eBufferType        - Buffer type to queue
        poTxBuffer         - Pointer to transmit buffer

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully queued buffer to PPI
        Error code returned from PPI and SPI close functions

*********************************************************************/
static u32  adi_pdd_Write(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poTxBuffer
)
{
    /* Return value - assume we're going to be successful */
    u32                     eResult = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device instance we're working on */
    ADI_DD_LQ035Q1DH02_DEF  *poDevice;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given device handle */
    eResult = adi_dd_Lq035q1dh02_ValidatePDDHandle(hPhysicalDevice);

    /* IF (the device handle is valid) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {

#endif  /* ADI_DEV_DEBUG */

        /* avoid casts */
        poDevice = (ADI_DD_LQ035Q1DH02_DEF *)hPhysicalDevice;

        /* IF (PPI is open and ready to use) */
        if (poDevice->hPpiDevice != NULL)
        {
            /* pass the request on to underlying PPI device */
            eResult = adi_dev_Write(poDevice->hPpiDevice,
                                    eBufferType,
                                    poTxBuffer);
        }
        /* ELSE (PPI device is not ready) */
        else
        {
            /* report error (PPI not open) */
            eResult = ADI_LQ035Q1DH02_RESULT_PPI_NOT_OPEN;
        }

/* Debug Build Only */
#if defined (ADI_DEV_DEBUG)
    }
#endif  /* ADI_DEV_DEBUG */

    /* return */
    return(eResult);
}

/*********************************************************************

    Function: adi_pdd_SequentialIO

        Function not supported as Sharp LQ035Q1DH02 is an outbound device

    Parameters:
        hPhysicalDevice    - Physical Device handle to
                             Sharp LQ035Q1DH02 device instance
        eBufferType        - Buffer type to queue
        poSeqBuffer        - Pointer to sequential buffer

    Returns:
        ADI_DEV_RESULT_NOT_SUPPORTED
            - Function not supported by this device

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
                             Sharp LQ035Q1DH02 device instance
        nCommandID         - Command ID
        pCommandValue      - Command specific value

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully processed command
        PPI/SPI/Sharp LQ035Q1DH02 Driver specific Error codes

*********************************************************************/
static u32  adi_pdd_Control(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    u32                         nCommandID,
    void                        *pCommandValue
)
{
    /* Convert command value to u8, u16 and u8 types */
    u8                      nu8CommandValue  = ((u8)((u32)pCommandValue));
    u16                     nu16CommandValue = ((u16)((u32)pCommandValue));
    u32                     nu32CommandValue = (u32)pCommandValue;
    /* Return value - assume we're going to be successful */
    u32                     eResult = ADI_DEV_RESULT_SUCCESS;
    /* delay loop counter to reset LCD */
    volatile u32            nCount;
    /* pointer to the device instance we're working on */
    ADI_DD_LQ035Q1DH02_DEF  *poDevice;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given device handle */
    eResult = adi_dd_Lq035q1dh02_ValidatePDDHandle(hPhysicalDevice);

    /* IF (the device handle is valid) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {

#endif  /* ADI_DEV_DEBUG */

        /* avoid casts */
        poDevice = (ADI_DD_LQ035Q1DH02_DEF *)hPhysicalDevice;

        /* CASE OF (Command ID) */
        switch(nCommandID)
        {

            /***********************************
            ** Device Manager Specific Commands
            ***********************************/

            /* CASE (Control Dataflow) */
            case ADI_DEV_CMD_SET_DATAFLOW:

                /* IF (we PPI device is closed) */
                if (poDevice->hPpiDevice == NULL)
                {
                    /* IF (Enable Dataflow) */
                    if (nu8CommandValue)
                    {
                        /* report error (PPI not open) */
                        eResult = ADI_LQ035Q1DH02_RESULT_PPI_NOT_OPEN;
                    }
                }
                /* ELSE (PPI device is open and ready for use) */
                else
                {
                    /* IF (Enable Dataflow) */
                    if (nu8CommandValue == true)
                    {
                        /* IF (Automatic PPI Control register Configuration enabled) */
                        if (poDevice->bIsAutoPpiControlEnabled == true)
                        {

/* IF (EPPI Class Device) */
#if defined (__ADSP_MOAB__)
                            /* Configure EPPI control register */
                            eResult = adi_dev_Control(poDevice->hPpiDevice,
                                                      ADI_EPPI_CMD_SET_CONTROL_REG,
                                                      (void *)poDevice->nPPICtlReg);
/* ELSE (PPI Class Device) */
#else
                            /* Configure PPI control register */
                            eResult = adi_dev_Control(poDevice->hPpiDevice,
                                                      ADI_PPI_CMD_SET_CONTROL_REG,
                                                      (void *)poDevice->nPPICtlReg);
#endif

                        }

                        /* IF (LCD registers are yet to be initialised) */
                        if (poDevice->bIsLcdInitialised == false)
                        {
                            /* IF (Successfully configured PPI registers) */
                            if (eResult == ADI_DEV_RESULT_SUCCESS)
                            {
                                /* initialise LCD registers */
                                eResult = adi_dd_Lq035q1dh02_SpiAccess(poDevice,
                                                                       0,
                                                                       NULL,
                                                                       ADI_DD_LQ035Q1DH02_LCD_REG_WRITE);
                            }

                        } /* End of if (poDevice->bIsLcdInitialised == false) case */

                    } /* End of if (Enable dataflow) case */

                    /* IF (Successfully initialised LCD registers) */
                    if (eResult == ADI_DEV_RESULT_SUCCESS)
                    {
                        /* Pass dataflow control to PPI driver */
                        eResult = adi_dev_Control(poDevice->hPpiDevice,
                                                  nCommandID,
                                                  pCommandValue);
                    }
                }
                break;

            /* CASE (Set Dataflow Method) */
            case ADI_DEV_CMD_SET_DATAFLOW_METHOD:

                /* IF (we've a PPI device already open) */
                if (poDevice->hPpiDevice != NULL)
                {
                    /* Pass dataflow method to PPI driver */
                    eResult = adi_dev_Control(poDevice->hPpiDevice,
                                              nCommandID,
                                              pCommandValue);
                }
                /* ELSE (PPI must be opened before setting dataflow method) */
                else
                {
                    /* report error (PPI not open) */
                    eResult = ADI_LQ035Q1DH02_RESULT_PPI_NOT_OPEN;
                }
                break;

            /* CASE (Query for processor DMA support) */
            case ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT:

                /* Sharp LQ035Q1DH02 uses DMA indirectly through PPI */
                *((u32 *)pCommandValue) = false;

                break;

            /*****************************
            ** Commands specific to SPI
            *****************************/

            /* CASE (Set SPI device number to use to configure LCD registers) */
            case ADI_LQ035Q1DH02_CMD_SET_SPI_DEVICE_NUMBER:
                /* Save the given SPI device number */
                poDevice->nSpiDeviceNumber = nu8CommandValue;
                break;

            /* CASE (Set SPI Chipselect Number connected to LCD CSB pin) */
            case ADI_LQ035Q1DH02_CMD_SET_SPI_CHIP_SELECT:
                /* Save the given SPI chipselect number */
                poDevice->nSpiChipSelect = nu8CommandValue;
                break;

            /*****************************
            ** Commands specific to PPI
            *****************************/

            /* CASE (Open PPI device number connected to LCD) */
            case ADI_LQ035Q1DH02_CMD_OPEN_PPI:

                /* IF (we've a PPI device already open) */
                if (poDevice->hPpiDevice != NULL)
                {
                    /* report error (PPI already allocated to this LCD instance) */
                    eResult = ADI_LQ035Q1DH02_RESULT_PPI_ALREADY_ALLOCATED;
                }
                /* ELSE (allocate this PPI device to LCD instance) */
                else
                {
                    /* Open the PPI device allocated to this LCD */
                    eResult = adi_dev_Open(poDevice->hDeviceManager,
/* IF (EPPI Class Device) */
#if defined (__ADSP_MOAB__)
                                           /* EPPI Entry point */
                                           &ADIEPPIEntryPoint,
/* ELSE (PPI Class Device) */
#else
                                           /* PPI Entry point */
                                           &ADIPPIEntryPoint,
#endif

                                           nu8CommandValue,
                                           poDevice,
                                           &poDevice->hPpiDevice,
                                           ADI_DEV_DIRECTION_OUTBOUND,
                                           poDevice->hDmaManager,
                                           poDevice->hDcbManager,
                                           adi_dd_Lq035q1dh02_PpiCallback);

                    /* IF (Successfully opened PPI Device) */
                    if (eResult == ADI_DEV_RESULT_SUCCESS)
                    {
                        /* IF (Automatic PPI Control register Configuration enabled) */
                        if (poDevice->bIsAutoPpiControlEnabled == true)
                        {
                                /* Enable Streaming for PPI DMA */
                                eResult = adi_dev_Control(poDevice->hPpiDevice,
                                                          ADI_DEV_CMD_SET_STREAMING,
                                                          (void *)true);
                        }

                    } /* End of if (Successfully opened PPI Device) case */

                } /* End of if (we've a PPI device already open) else case */


                break;

            /* CASE (Close PPI device number connected to LCD) */
            case ADI_LQ035Q1DH02_CMD_CLOSE_PPI:

                /* IF (PPI is open) */
                if (poDevice->hPpiDevice != NULL)
                {
                    /* Close the PPI device used by this LCD instance */
                    eResult = adi_dev_Close(poDevice->hPpiDevice);

                    /* IF (successfully closed PPI device) */
                    if (eResult == ADI_DEV_RESULT_SUCCESS)
                    {
                        /* Clear PPI device handle */
                        poDevice->hPpiDevice = NULL;
                    }
                }
                break;

            /* CASE (Enable/Disable Automatic PPI Configuration) */
            case ADI_LQ035Q1DH02_CMD_ENABLE_AUTO_PPI_CONTROL_CONFIG:

                /* Update Automatic PPI control configuration settings */
                poDevice->bIsAutoPpiControlEnabled = nu8CommandValue;
                break;

            /* CASE (Set the value to be used for PPI_CONTROL) */
            case ADI_LQ035Q1DH02_CMD_SET_PPI_CTL_VALUE:

                /* Save supplied value in device structure */
#if defined(__ADSP_MOAB__)
                poDevice->nPPICtlReg = nu32CommandValue;
#else
                poDevice->nPPICtlReg = nu16CommandValue;
#endif
                break;

            /* CASE (Set the PPI clock to LCD dot clock ratio) */
            case ADI_LQ035Q1DH02_CMD_SET_PPI_CLK_PER_DOT:

                /* Save supplied value in device structure */
                poDevice->nPPIClkPerDot = nu16CommandValue;
                break;

            /*************************************************************
            ** Commands specific to Sharp LQ035Q1DH02 LCD register access
            *************************************************************/

            /* CASE (Get Number of padding lines required at
                    the start/top of each frame) */
            case ADI_LQ035Q1DH02_CMD_GET_TOP_PAD_LINES_PER_FRAME:
                *((u32 *)pCommandValue) = (u32)poDevice->oLcdRegs.nVerticalPorch;
                break;

            /* CASE (Get Number of padding lines required at
                    the end/bottom of each frame) */
            case ADI_LQ035Q1DH02_CMD_GET_BOTTOM_PAD_LINES_PER_FRAME:
                *((u32 *)pCommandValue) = ADI_DD_LQ035Q1DH02_VFP;
                break;

            /* CASE (Set Output shift direction (Top to Botton and
                     Left to Right) of the LCD display) */
            case ADI_LQ035Q1DH02_CMD_SET_OUTPUT_SHIFT_DIRECTION:

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

                /* Validate LCD output shift mode */
                if ((ADI_LQ035Q1DH02_OUT_SHIFT_MODE)pCommandValue > ADI_LQ035Q1DH02_OUT_SHIFT_BT_RL)
                {
                    /* report error (Output shift mode invalid) */
                    eResult = ADI_LQ035Q1DH02_RESULT_OUT_SHIFT_MODE_INVALID;
                    break;
                }

#endif  /* ADI_DEV_DEBUG */

                /* IF (Top Shifts to Bottom) */
                if (((ADI_LQ035Q1DH02_OUT_SHIFT_MODE)pCommandValue == ADI_LQ035Q1DH02_OUT_SHIFT_TB_RL) ||\
                     ((ADI_LQ035Q1DH02_OUT_SHIFT_MODE)pCommandValue == ADI_LQ035Q1DH02_OUT_SHIFT_TB_LR))
                {
                    /* Set TB bit in LCD driver output control register */
                    poDevice->oLcdRegs.nDriverOutControl |= ADI_DD_LQ035Q1DH02_TB_MASK;
                }
                /* ELSE (Bottom shifts to Top) */
                else
                {
                    /* Clear TB bit in LCD driver output control register */
                    poDevice->oLcdRegs.nDriverOutControl &= ~ADI_DD_LQ035Q1DH02_TB_MASK;
                }

                /* IF (Right Shifts to Left) */
                if (((ADI_LQ035Q1DH02_OUT_SHIFT_MODE)pCommandValue == ADI_LQ035Q1DH02_OUT_SHIFT_TB_RL) ||\
                    ((ADI_LQ035Q1DH02_OUT_SHIFT_MODE)pCommandValue == ADI_LQ035Q1DH02_OUT_SHIFT_BT_RL))
                {
                    /* Set RL bit in LCD driver output control register */
                    poDevice->oLcdRegs.nDriverOutControl |= ADI_DD_LQ035Q1DH02_RL_MASK;
                }
                /* ELSE (Left shifts to Right) */
                else
                {
                    /* Clear RL bit in LCD driver output control register */
                    poDevice->oLcdRegs.nDriverOutControl &= ~ADI_DD_LQ035Q1DH02_RL_MASK;
                }

                /* Update LCD driver control register */
                eResult = adi_dd_Lq035q1dh02_SpiAccess(poDevice,
                                                       ADI_DD_LQ035Q1DH02_DRIVER_OUT_CTRL,
                                                       &(poDevice->oLcdRegs.nDriverOutControl),
                                                       ADI_DD_LQ035Q1DH02_LCD_REG_WRITE);
                break;

            /* CASE (Enable/Disable Reverse display) */
            case ADI_LQ035Q1DH02_CMD_ENABLE_REVERSE_DISPLAY:

                /* IF (Enable reverse display) */
                if (nu8CommandValue)
                {
                    /* Clear REV bit in LCD driver output control register */
                    poDevice->oLcdRegs.nDriverOutControl &= ~ADI_DD_LQ035Q1DH02_REV_MASK;
                }
                /* ELSE (Disable reverse display) */
                else
                {
                    /* Set REV bit in LCD driver output control register */
                    poDevice->oLcdRegs.nDriverOutControl |= ADI_DD_LQ035Q1DH02_REV_MASK;
                }

                /* Update LCD driver control register */
                eResult = adi_dd_Lq035q1dh02_SpiAccess(poDevice,
                                                       ADI_DD_LQ035Q1DH02_DRIVER_OUT_CTRL,
                                                       &(poDevice->oLcdRegs.nDriverOutControl),
                                                       ADI_DD_LQ035Q1DH02_LCD_REG_WRITE);
                break;

            /* CASE (Enable/Disable 256k color mode (OR) Disable/Enable 8-color mode) */
            case ADI_LQ035Q1DH02_CMD_ENABLE_256K_COLOR:

                /* IF (Enable 256k color display) */
                if (nu8CommandValue)
                {
                    /* Clear CM bit in LCD shut and 8-color register */
                    poDevice->oLcdRegs.nShutAndColor &= ~ADI_DD_LQ035Q1DH02_CM_MASK;
                }
                /* ELSE (Enable 8-color mode) */
                else
                {
                    /* Set CM bit in LCD shut and 8-color register */
                    poDevice->oLcdRegs.nShutAndColor |= ADI_DD_LQ035Q1DH02_CM_MASK;
                }

                /* Update LCD Shut and 8-Color register */
                eResult = adi_dd_Lq035q1dh02_SpiAccess(poDevice,
                                                       ADI_DD_LQ035Q1DH02_SHUT_8_COLOR,
                                                       &(poDevice->oLcdRegs.nShutAndColor),
                                                       ADI_DD_LQ035Q1DH02_LCD_REG_WRITE);
                break;

            /* CASE (Set number of valid pixels per line) */
            case ADI_LQ035Q1DH02_CMD_SET_NUM_VALID_PIXELS_PER_LINE:

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

                /* Validate valid pixels per line value */
                if ((nu16CommandValue < ADI_DD_LQ035Q1DH02_PIX_PER_LINE_MIN_VAL) ||\
                    (nu16CommandValue > ADI_DD_LQ035Q1DH02_PIX_PER_LINE_MAX_VAL))
                {
                    /* report error (Pixels Per Line count invalid) */
                    eResult = ADI_LQ035Q1DH02_RESULT_PIXELS_PER_LINE_INVALID;
                    break;
                }

#endif  /* ADI_DEV_DEBUG */

                /* Clear Pixel Per Line Field in Pixels Per Line register */
                poDevice->oLcdRegs.nPixelPerLine &= ~ADI_DD_LQ035Q1DH02_VALID_PIX_PER_LINE_MASK;

                /* Decrement given value by 1 to get the register field equivalent value */
                /* Update Pixel Per Line Field with new value */
                poDevice->oLcdRegs.nPixelPerLine |= ((nu16CommandValue-1) << ADI_DD_LQ035Q1DH02_PIX_PER_LINE_SHIFT);

                /* Update LCD Pixel Per Line register */
                eResult = adi_dd_Lq035q1dh02_SpiAccess(poDevice,
                                                       ADI_DD_LQ035Q1DH02_PIXEL_PER_LINE,
                                                       &(poDevice->oLcdRegs.nPixelPerLine),
                                                       ADI_DD_LQ035Q1DH02_LCD_REG_WRITE);
                break;

            /* CASE (Set number of dummy dot-clocks per line) */
            case ADI_LQ035Q1DH02_CMD_SET_NUM_DUMMY_DOT_CLOCKS_PER_LINE:

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

                /* Validate dummy dot-clocks per line value */
                if ((nu16CommandValue < ADI_DD_LQ035Q1DH02_HBP_MIN_VAL) ||\
                    (nu16CommandValue > ADI_DD_LQ035Q1DH02_HBP_MAX_VAL))
                {
                    /* report error (Dummy Dot clocks count invalid) */
                    eResult = ADI_LQ035Q1DH02_RESULT_DUMMY_DOT_CLOCKS_PER_LINE_INVALID;
                    break;
                }

#endif  /* ADI_DEV_DEBUG */

                /* Clear Dummy clocks per line (HBP) field in Pixels Per Line register */
                poDevice->oLcdRegs.nPixelPerLine &= ~ADI_DD_LQ035Q1DH02_HBP_MASK;

                /* Decrement given value by 2 to get the register field equivalent */
                /* Update Dummy clocks per line (HBP) Field with new value */
                poDevice->oLcdRegs.nPixelPerLine |= (nu8CommandValue-2);

                /* Update LCD Pixel Per Line register */
                eResult = adi_dd_Lq035q1dh02_SpiAccess(poDevice,
                                                       ADI_DD_LQ035Q1DH02_PIXEL_PER_LINE,
                                                       &(poDevice->oLcdRegs.nPixelPerLine),
                                                       ADI_DD_LQ035Q1DH02_LCD_REG_WRITE);

                break;

            /* CASE (Set number of dummy lines per frame) */
            case ADI_LQ035Q1DH02_CMD_SET_NUM_DUMMY_LINES_PER_FRAME:

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

                /* Validate dummy dot-clocks per line value */
                if ((nu16CommandValue < ADI_DD_LQ035Q1DH02_VBP_MIN_VAL) ||\
                    (nu16CommandValue > ADI_DD_LQ035Q1DH02_VBP_MAX_VAL))
                {
                    /* report error (Dummy Dot lines per frame count invalid) */
                    eResult = ADI_LQ035Q1DH02_RESULT_DUMMY_LINES_PER_FRAME_INVALID;
                    break;
                }

#endif  /* ADI_DEV_DEBUG */

                /* update vertical porch register in driver cache */
                poDevice->oLcdRegs.nVerticalPorch = (nu16CommandValue & ADI_DD_LQ035Q1DH02_VBP_MASK);

                /* Update LCD dummy lines per frame register */
                eResult = adi_dd_Lq035q1dh02_SpiAccess(poDevice,
                                                       ADI_DD_LQ035Q1DH02_VERTICAL_PORCH,
                                                       &(poDevice->oLcdRegs.nVerticalPorch),
                                                       ADI_DD_LQ035Q1DH02_LCD_REG_WRITE);
                break;

            /* CASE (Enter sleep mode by writing to LCD SHUT register) */
            case ADI_LQ035Q1DH02_CMD_ENTER_SLEEP:
            /* CASE (Exit sleep mode by writing to LCD SHUT register) */
            case ADI_LQ035Q1DH02_CMD_EXIT_SLEEP:

                /* IF (Enter Sleep Mode) */
                if (nCommandID == ADI_LQ035Q1DH02_CMD_ENTER_SLEEP)
                {
                    /* Set SHUT bit in LCD shut and 8-color register */
                    poDevice->oLcdRegs.nShutAndColor |= ADI_DD_LQ035Q1DH02_SHUT_MASK;
                }
                /* ELSE (Exit Sleep mode) */
                else
                {
                    /* Clear SHUT bit in LCD shut and 8-color register */
                    poDevice->oLcdRegs.nShutAndColor &= ~ADI_DD_LQ035Q1DH02_SHUT_MASK;
                }

                /* Update LCD Shut and 8-Color register */
                eResult = adi_dd_Lq035q1dh02_SpiAccess(poDevice,
                                                       ADI_DD_LQ035Q1DH02_SHUT_8_COLOR,
                                                       &(poDevice->oLcdRegs.nShutAndColor),
                                                       ADI_DD_LQ035Q1DH02_LCD_REG_WRITE);

                /* IF (Successfully configured LCD in sleep mode) */
                if ((eResult == ADI_DEV_RESULT_SUCCESS) &&
                    (nCommandID == ADI_LQ035Q1DH02_CMD_ENTER_SLEEP))
                {
                    /* Give some delay allowing LCD hardware to enter sleep mode */
                    for (nCount = 0xFFFFF; nCount; nCount--);
                }

                break;

            /* CASE (Reset LCD display by pulling down LCD reset line) */
            case ADI_LQ035Q1DH02_CMD_HARDWARE_RESET:

                /* IF (PPI Device is open) */
                if (poDevice->hPpiDevice != NULL)
                {
                    /* Disable LCD (PPI) dataflow */
                    eResult = adi_dev_Control(poDevice->hPpiDevice,
                                              ADI_DEV_CMD_SET_DATAFLOW,
                                              (void *) false);
                }

                /* IF (Successfully disabled LCD dataflow) */
                if (eResult == ADI_DEV_RESULT_SUCCESS)
                {
                    /* Open the Reset Flag pin */
                    eResult = adi_flag_Open((ADI_FLAG_ID)pCommandValue);
                }

                /* IF (Successfuly opened LCD reset pin) */
                if(eResult == ADI_FLAG_RESULT_SUCCESS)
                {
                    /* Set this flag in output mode */
                    eResult = adi_flag_SetDirection((ADI_FLAG_ID)pCommandValue, ADI_FLAG_DIRECTION_OUTPUT);
                }

                /* IF (Successfuly opened & configured LCD reset pin) */
                if(eResult == ADI_FLAG_RESULT_SUCCESS)
                {
                    /* Clear Flag pin to Reset LCD */
                    eResult = adi_flag_Clear((ADI_FLAG_ID)pCommandValue);
                }

                /* IF (Successfuly opened, configured and cleared LCD reset pin) */
                if(eResult == ADI_FLAG_RESULT_SUCCESS)
                {
                    /* Give some delay to reset LCD (at least 10us) */
                    for (nCount = 0xFFFFF; nCount; nCount--);

                    /* Set Flag pin pull LCD out of Reset */
                    eResult = adi_flag_Set((ADI_FLAG_ID)pCommandValue);
                    /* Clear LCD init flag to indicate that
                       LCD registers needs to be initialised */
                    poDevice->bIsLcdInitialised = false;
                }
                break;

            /* default (pass this command to PPI device if it is open) */
            default:
                /* IF (PPI device is open) */
                if (poDevice->hPpiDevice != NULL)
                {
                    eResult = adi_dev_Control(poDevice->hPpiDevice,
                                              nCommandID,
                                              pCommandValue);
                }
                /* ELSE (Command not supported. report error) */
                else
                {
                    eResult = ADI_LQ035Q1DH02_RESULT_CMD_NOT_SUPPORTED;
                }
                break;
        }
/* Debug Build Only */
#if defined(ADI_DEV_DEBUG)
    }
#endif  /* ADI_DEV_DEBUG */

    /* return */
    return(eResult);
}

/*********************************************************************

    Function: adi_dd_Lq035q1dh02_SpiAccess

        Accesses LCD registers via SPI device

    Parameters:
        poDevice       - Pointer to Sharp LQ035Q1DH02 device instance
                         we're working on
        nRegAddr       - Register Address to access
        pnRegData      - Location holding/to hold register data
        nReadFlag      - 1 to read LCD register,
                         0 to write to LCD register

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully accessed LCD register(s)
        ADI_LQ035Q1DH02_RESULT_SPI_DEVICE_INVALID
            - SPI Deivce number invalid
        ADI_LQ035Q1DH02_RESULT_SPI_CS_INVALID
            - SPI Chipselect invalid

*********************************************************************/
static u32  adi_dd_Lq035q1dh02_SpiAccess(
    ADI_DD_LQ035Q1DH02_DEF      *poDevice,
    u8                          nRegAddr,
    u16                         *pnRegData,
    u8                          nReadFlag
)
{
    /* Return value - assume we're going to be successful   */
    u32     eResult = ADI_DEV_RESULT_SUCCESS;

    /* IF (SPI device number is invalid) */
    if (poDevice->nSpiDeviceNumber == ADI_DD_LQ035Q1DH02_SPI_NUM_INVALID)
    {
        /* report error (SPI Device invalid) */
        eResult = ADI_LQ035Q1DH02_RESULT_SPI_DEVICE_INVALID;
    }
    /* ELSE IF (SPI Chip select is invalid) */
    else if (poDevice->nSpiChipSelect == ADI_DD_LQ035Q1DH02_SPI_CS_INVALID)
    {
        /* report error (SPI Chipselect invalid) */
        eResult = ADI_LQ035Q1DH02_RESULT_SPI_CS_INVALID;
    }
    /* ELSE (SPI Device number and Chipselect must be valid) */
    else
    {
        /* Open SPI device */
        eResult = adi_dd_Lq035q1dh02_SpiOpen(poDevice);

        /* IF (successfully opened SPI device) */
        if (eResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* IF (Read LCD registers) */
            if (nReadFlag)
            {
                /* nothing to do here */
            }
            /* ELSE (write to LCD registers) */
            else
            {
                /* IF (LCD registers are not yet initialised) */
                if (poDevice->bIsLcdInitialised == false)
                {
                    /* Initialise LCD registers */
                    eResult = adi_dd_Lq035q1dh02_InitLcdRegs(poDevice);
                }
                /* ELSE (LCD registers are already initialised) */
                else
                {
                    /* write to the selected LCD register */
                    eResult = adi_dd_Lq035q1dh02_SpiWrite(poDevice,
                                                          nRegAddr,
                                                          *pnRegData);
                }
            }

            /* Close SPI device driver */
            adi_dev_Close (poDevice->hSpiDevice);
            /* Delete SPI Data Semaphore */
            adi_sem_Delete (poDevice->hSpiDataSemaphore);
        }
    }

    /* return */
    return(eResult);
}

/*********************************************************************

    Function: adi_dd_Lq035q1dh02_SpiOpen

        Opens SPI device connected to LCD SPI Port

    Parameters:
        poDevice - Pointer to Sharp LQ035Q1DH02 device instance
                   we're working on

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully opend SPI device
        Error code from Device Manager or SPI driver

*********************************************************************/
static u32  adi_dd_Lq035q1dh02_SpiOpen(
    ADI_DD_LQ035Q1DH02_DEF      *poDevice
)
{
    /* Return value - assume we're going to be successful   */
    u32     eResult = ADI_DEV_RESULT_SUCCESS;

    /* SPI configuration table to access LCD registers */
    ADI_DEV_CMD_VALUE_PAIR  aoSpiConfig[] =
    {
        /* SPI Baud rate */
        {
            ADI_SPI_CMD_SET_BAUD_REG,
            (void *)ADI_DD_LQ035Q1DH02_SPI_BAUD
        },
        /* Active High SCLK */
        {
            ADI_SPI_CMD_SET_CLOCK_POLARITY,
            (void *)1U
        },
        /* Blackfin always as Master */
        {
            ADI_SPI_CMD_SET_MASTER,
            (void *)true
        },
        /* SPI transfer word size */
        {
            ADI_SPI_CMD_SET_WORD_SIZE,
            (void *)ADI_DD_LQ035Q1DH02_SPI_WORD_SIZE
        },
        /* Software controller chipselect */
        {
            ADI_SPI_CMD_SET_CLOCK_PHASE,
            (void *)1
        },
        /* SPI Slave Select for LCD device */
        {
            ADI_SPI_CMD_ENABLE_SLAVE_SELECT,
            (void *)poDevice->nSpiChipSelect
        },
        /* Chained Dataflow method */
        {
            ADI_DEV_CMD_SET_DATAFLOW_METHOD,
            (void *)ADI_DEV_MODE_CHAINED
        },
        /* End of configuration table */
        {
            ADI_DEV_CMD_END,
            NULL
        }
    };

    /* Open SPI device in interrupt driven mode */
    eResult = adi_dev_Open(poDevice->hDeviceManager,
                           &ADISPIIntEntryPoint,
                           poDevice->nSpiDeviceNumber,
                           poDevice,
                           &poDevice->hSpiDevice,
                           ADI_DEV_DIRECTION_OUTBOUND,
                           NULL,
                           poDevice->hDcbManager,
                           adi_dd_Lq035q1dh02_SpiCallback);

    /* IF (Successfully opened SPI device) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Configure SPI driver to configure LCD registers */
        eResult = adi_dev_Control(poDevice->hSpiDevice,
                                  ADI_DEV_CMD_TABLE,
                                  (void*)&aoSpiConfig[0]);
    }

    /* IF (Successfully opened and configured SPI driver) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* create a binary semaphore to handle SPI data transfer */
        eResult = (u32)adi_sem_Create(1,&poDevice->hSpiDataSemaphore, NULL);
    }

    /* Return */
    return (eResult);
}

/*********************************************************************

    Function: adi_dd_Lq035q1dh02_InitLcdRegs

        Initialises LCD registers and updates
        PPI registers to match LCD configuration

    Parameters:
        poDevice - Pointer to Sharp LQ035Q1DH02 device instance
                   we're working on

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully initialised LCD registers
        Error code from 'adi_dd_Lq035q1dh02_SpiWrite' function

*********************************************************************/
static u32  adi_dd_Lq035q1dh02_InitLcdRegs (
    ADI_DD_LQ035Q1DH02_DEF      *poDevice
)
{
    /* LUT Index */
    u8      nIndex;
    /* Return value - assume we're going to be successful   */
    u32     eResult = ADI_DEV_RESULT_SUCCESS;

    /* LUT to initialise LCD registers */
    ADI_DD_LQ035Q1DH02_REG_ACCESS   aLcdInitRegs[] =
    {
        /* LCD Driver Output Control register */
        {
            ADI_DD_LQ035Q1DH02_DRIVER_OUT_CTRL,
            poDevice->oLcdRegs.nDriverOutControl
        },
        /* LCD Shut and 8-color register */
        {
            ADI_DD_LQ035Q1DH02_SHUT_8_COLOR,
            poDevice->oLcdRegs.nShutAndColor
        },
        /* LCD Entry Mode register */
        {
            ADI_DD_LQ035Q1DH02_ENTRY_MODE,
            ADI_DD_LQ035Q1DH02_ENTRY_MODE_DEFAULT
        },
        /* LCD Pixel Per Line register */
        {
            ADI_DD_LQ035Q1DH02_PIXEL_PER_LINE,
            poDevice->oLcdRegs.nPixelPerLine
        },
        /* LCD Vertical Porch register */
        {
            ADI_DD_LQ035Q1DH02_VERTICAL_PORCH,
            poDevice->oLcdRegs.nVerticalPorch
        }
    };

    /* Configure all LCD registers listed in above LUT */
    for (nIndex = 0;
         nIndex < (sizeof(aLcdInitRegs)/sizeof(aLcdInitRegs[0]));
         nIndex++
        )
    {
        eResult = adi_dd_Lq035q1dh02_SpiWrite (poDevice,
                                               aLcdInitRegs[nIndex].nRegAddr,
                                               aLcdInitRegs[nIndex].nRegData);

        /* IF (Failed to initialise LCD register) */
        if (eResult != ADI_DEV_RESULT_SUCCESS)
        {
            /* quit this loop */
            break;
        }
    }

    /* return */
    return (eResult);
}

/*********************************************************************

    Function: adi_dd_Lq035q1dh02_SpiWrite

         Writes to LCD registers via SPI device

    Parameters:
        poDevice - Pointer to Sharp LQ035Q1DH02 device instance
                   we're working on

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully configured LCD registers via SPI
        Error code from SPI driver or device manager

*********************************************************************/
static u32  adi_dd_Lq035q1dh02_SpiWrite(
    ADI_DD_LQ035Q1DH02_DEF      *poDevice,
    u8                          nRegAddr,
    u16                         nRegData
)
{
    /* array to hold data to be sent out by SPI to LCD */
    u8                  aSpiData[3];
    /* Return value - assume we're going to be successful   */
    u32                 eResult = ADI_DEV_RESULT_SUCCESS;
    /* 1D buffer instance to write to LCD register */
    ADI_DEV_1D_BUFFER   oSpiWrite1DBuf;

    /* Populate SPI 1D write buffer */
    oSpiWrite1DBuf.Data                 = (void *)aSpiData;
    /* Send 3 bytes of data */
    oSpiWrite1DBuf.ElementCount         = 3U;
    oSpiWrite1DBuf.ElementWidth         = 1U;
    /* Enable callback */
    oSpiWrite1DBuf.CallbackParameter    = &oSpiWrite1DBuf;
    /* Terminate SPI buffer */
    oSpiWrite1DBuf.pNext                = NULL;

    /* Create LCD register address write packet */
    aSpiData[0] = ADI_DD_LQ035Q1DH02_LCD_REG_ADDR_BYTE_0;
    aSpiData[1] = ADI_DD_LQ035Q1DH02_LCD_REG_ADDR_BYTE_1;
    aSpiData[2] = nRegAddr;

    /* Submit write buffer to SPI driver */
    eResult = adi_dev_Write (poDevice->hSpiDevice,
                             ADI_DEV_1D,
                             (ADI_DEV_BUFFER *)&oSpiWrite1DBuf);

    /* IF (successfully queued LCD register address write buffer to SPI) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Select SPI slave device (LCD) to transmit data */
        eResult = adi_dd_Lq035q1dh02_SpiSlaveSelect(poDevice);
    }

    /* IF (successfully sent LCD register address to write) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Create LCD register data write packet */
        aSpiData[0] = ADI_DD_LQ035Q1DH02_LCD_REG_DATA_BYTE_0;
        aSpiData[1] = (u8)(nRegData >> 8U);
        aSpiData[2] = (u8)nRegData;

        /* Submit write buffer to SPI driver */
        eResult = adi_dev_Write (poDevice->hSpiDevice,
                                 ADI_DEV_1D,
                                 (ADI_DEV_BUFFER *)&oSpiWrite1DBuf);
    }

    /* IF (successfully queued LCD register data write buffer to SPI) */
    if (eResult == ADI_DEV_RESULT_SUCCESS);
    {
        /* Select SPI slave device (LCD) to transmit data */
        eResult = adi_dd_Lq035q1dh02_SpiSlaveSelect(poDevice);
    }

    /* IF (Successfully updated LCD registers) */
    if (eResult == ADI_DEV_RESULT_SUCCESS);
    {
        /* IF (PPI is already open and
               LCD Pixel Per line or VBF register updated) */
        if ((poDevice->hPpiDevice != NULL) &&
            ((nRegAddr == ADI_DD_LQ035Q1DH02_PIXEL_PER_LINE) ||
             (nRegAddr == ADI_DD_LQ035Q1DH02_VERTICAL_PORCH)))
        {

            /* Update PPI Frame Sync registers */
            eResult = adi_dd_Lq035q1dh02_PpiFsConfig(poDevice);
        }
    }

    /* return */
    return (eResult);
}

/*********************************************************************

    Function: adi_dd_Lq035q1dh02_SpiSlaveSelect

        Controls SPI slave select line corresponding to the LCD device

    Parameters:
        poDevice - Pointer to Sharp LQ035Q1DH02 device instance
                   we're working on

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully controlled LCD SPI Slave select pin
        Error code from SPI driver

*********************************************************************/
static u32  adi_dd_Lq035q1dh02_SpiSlaveSelect(
    ADI_DD_LQ035Q1DH02_DEF      *poDevice
)
{
    /* Return value - assume we're going to be successful   */
    u32     eResult = ADI_DEV_RESULT_SUCCESS;

    /* Activate SPI chip select line for the LCD device */
    eResult = adi_dev_Control(poDevice->hSpiDevice,
                              ADI_SPI_CMD_ENABLE_SLAVE_SELECT,
                              (void*)poDevice->nSpiChipSelect);

    /* IF (Successfully activated SPI chipselect) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Drive LCD chipselect line to low (CSB) */
        eResult = adi_dev_Control(poDevice->hSpiDevice,
                                  ADI_SPI_CMD_SELECT_SLAVE,
                                  (void*)poDevice->nSpiChipSelect);
    }

    /* IF (Successfully driven LCD chipselect line to low) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Acquire SPI Data semaphore */
        eResult = (u32)adi_sem_Pend (poDevice->hSpiDataSemaphore,
                                     ADI_SEM_TIMEOUT_FOREVER);
    }

    /* IF (Successfully acquired SPI data semaphore) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Enable SPI data flow */
        eResult = adi_dev_Control(poDevice->hSpiDevice,
                                  ADI_DEV_CMD_SET_DATAFLOW,
                                  (void *)true);
    }

    /* IF (Successfully enabled SPI dataflow) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* wait until SPI completes receiving/transfering the data packet */
        /* SPI callback function posts SPI data semaphore
           after completing SPI read/write */
        eResult = (u32)adi_sem_Pend (poDevice->hSpiDataSemaphore,
                                     ADI_SEM_TIMEOUT_FOREVER);

        /* IF (SPI Data transfer complete) */
        if (eResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* Release SPI Data Semaphore */
            adi_sem_Post (poDevice->hSpiDataSemaphore);
        }
    }

    /* IF (Successfully completed SPI data transfer) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Disable SPI dataflow */
        eResult = adi_dev_Control(poDevice->hSpiDevice,
                                  ADI_DEV_CMD_SET_DATAFLOW,
                                  (void *)false);
    }

    /* IF (Successfully completed SPI data transfer and
           disabled SPI dataflow) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Deactivate LCD SPI chip select line */
        eResult = adi_dev_Control(poDevice->hSpiDevice,
                                  ADI_SPI_CMD_DISABLE_SLAVE_SELECT,
                                  (void*)poDevice->nSpiChipSelect);
    }

    /* return */
    return (eResult);
}

/*********************************************************************

    Function: adi_dd_Lq035q1dh02_SpiCallback

        SPI Callback to LCD driver

    Parameters:
        phClient       - Client handle passed to
                         SPI device with adi_dev_Open
        nEvent         - Callback Event
        pArg           - Callback Argument

    Returns:
        None

*********************************************************************/
static void adi_dd_Lq035q1dh02_SpiCallback (
    void    *phClient,
    u32     nEvent,
    void    *pArg
)
{
    /* pointer to the LCD device instant to which this callback belongs */
    ADI_DD_LQ035Q1DH02_DEF  *poDevice = (ADI_DD_LQ035Q1DH02_DEF *)phClient;

    /* CASE OF (Callback Event) */
    switch (nEvent)
    {
        /* identify which buffer is generating the callback */
        case ADI_SPI_EVENT_WRITE_BUFFER_PROCESSED:
        case ADI_SPI_EVENT_READ_BUFFER_PROCESSED:

            /* SPI has finished procesing Iuput/Outbut buffer. De-select the slave device */
            /* LCD SPI CS is tied high to De-select it */
            adi_dev_Control(poDevice->hSpiDevice,
                            ADI_SPI_CMD_DESELECT_SLAVE,
                            (void*)poDevice->nSpiChipSelect);

            /* Release SPI Data Semaphore */
            adi_sem_Post (poDevice->hSpiDataSemaphore);
            break;

        default:
            break;
    }
}

/* IF (ADSP-BF52x/ADSP-BF53x/ADSP-BF56x device) */
#if defined (__ADSP_EDINBURGH__)    ||\
    defined (__ADSP_BRAEMAR__)      ||\
    defined (__ADSP_STIRLING__)     ||\
    defined (__ADSP_TETON__)        ||\
    defined (__ADSP_KOOKABURRA__)   ||\
    defined (__ADSP_BRODIE__)       ||\
    defined (__ADSP_MOCKINGBIRD__)

/*********************************************************************

    Function: adi_dd_Lq035q1dh02_PpiFsConfig

        Configures PPI Frame Sync registers as per LCD requirements

    Parameters:
        poDevice - Pointer to Sharp LQ035Q1DH02 device instance
                   we're working on

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully updated PPI Frame Sync registers
        Error code from Device Manager or PPI Driver

*********************************************************************/
static u32  adi_dd_Lq035q1dh02_PpiFsConfig(
    ADI_DD_LQ035Q1DH02_DEF      *poDevice
)
{
    /* Active Pixels Per Line */
    u16     nPixelsPerLine;
    /* Horizontal Back Porch */
    u16     nHorizontalBackPorch;
    /* Clock delay after frame sync */
    u16     nFsClockDelay;
    /* Ratio of PPI clock to LCD dot clock */
    u16     nClkRatio;
    /* assume we're going to be success */
    u32     eResult = ADI_DEV_RESULT_SUCCESS;
    /* PPI Frame Sync configuration instance */
    ADI_PPI_FS_TMR  oFsConfigTimer;

    /* Extract pixel per line register field value */
    nPixelsPerLine = (poDevice->oLcdRegs.nPixelPerLine & ADI_DD_LQ035Q1DH02_VALID_PIX_PER_LINE_MASK);
    /* Shift register field to get the exact register field value */
    nPixelsPerLine >>= ADI_DD_LQ035Q1DH02_PIX_PER_LINE_SHIFT;

    /* PPI to LCD dot clock ratio (for sync timers) */
    nClkRatio = poDevice->nPPIClkPerDot;

    /*
    ** Horizontal Back Porch = HBF Field value + 2
    */
    nHorizontalBackPorch = ((poDevice->oLcdRegs.nPixelPerLine & ADI_DD_LQ035Q1DH02_HBP_MASK) + 2U);

    /* IF (HBP = Maximum value) */
    if (nHorizontalBackPorch >= ADI_DD_LQ035Q1DH02_HBP_MAX_VAL)
    {
        /* Clock delay after frame sync = 0 */
        nFsClockDelay = 0;
    }
    /* ELSE (HBP must be between 2 & 64) */
    else
    {
        /* Clock delay after frame sync = (HBP - 1) */
        nFsClockDelay = nHorizontalBackPorch - 1;
        if (nClkRatio != 1) {
                /*NB this calculation only suitable for CPLD MODE 3 (RGB888) */
                nFsClockDelay = nFsClockDelay * nClkRatio + 5;
        }
    }

    /* Populate FS1-Hsync Timer configuration instance */

    /* Positive action pulse */
    oFsConfigTimer.pulse_hi     = 0;
    /* Timer Counter runs during emulation */
    oFsConfigTimer.emu_run      = 1U;
    /*
    **
    ** FS1 (HSYNC) Period = Pixels Per Line +
    **                      Dummy dot clocks per line (HBP) +
    **                      Horizontal Front Porch (HFP)
    **
    ** Increment LCD Pixel Per Line register value by 1 to get
    ** exact pixel per line value
    */
    oFsConfigTimer.period       = ((nPixelsPerLine + 1) +\
                                   nHorizontalBackPorch +\
                                   ADI_DD_LQ035Q1DH02_HFP) * nClkRatio;
    /* HSYNC Width */
    oFsConfigTimer.width        = 2U * nClkRatio;

    /* No delay */
    oFsConfigTimer.enable_delay = 0;

    /* Configure Frame Sync 1 Timer */
    eResult = adi_dev_Control(poDevice->hPpiDevice,
                              ADI_PPI_CMD_SET_TIMER_FRAME_SYNC_1,
                              (void *)&oFsConfigTimer);

    /* IF (Successfully configured Frame Sync 1 Timer) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {
        u32 hsync_period = oFsConfigTimer.period;

        /* Populate FS2-Vsync Timer configuration instance */

        /*
        **
        ** FS2 (VSYNC) Period = FS1 (HSYNC) Period *
        **                      (Active Lines per frame +
        **                       Dummy Lines Per Frame (VBF) +
        **                       Vertical Front Porch (VFP))
        **
        */
        oFsConfigTimer.period   = (oFsConfigTimer.period *\
                                   (ADI_DD_LQ035Q1DH02_ACTIVE_LINES_PER_FRAME +\
                                    poDevice->oLcdRegs.nVerticalPorch +\
                                    ADI_DD_LQ035Q1DH02_VFP));
        /* VSYNC Width */
        /*
        ** FS2 (VSYNC) Width = Dummy Lines Per Frame or Vertical Back Porch (VBF)
        **
        */
        oFsConfigTimer.width    = hsync_period;

        /* Configure Frame Sync 2 Timer */
        eResult = adi_dev_Control(poDevice->hPpiDevice,
                                  ADI_PPI_CMD_SET_TIMER_FRAME_SYNC_2,
                                  (void *)&oFsConfigTimer);
    }

    /* IF (Successfully configured Frame Sync Timer Registers) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Configure PPI Lines per Frame Register */
        eResult = adi_dev_Control(poDevice->hPpiDevice,
                                  ADI_PPI_CMD_SET_LINES_PER_FRAME_REG,
                                  (void *) (ADI_DD_LQ035Q1DH02_ACTIVE_LINES_PER_FRAME +\
                                            poDevice->oLcdRegs.nVerticalPorch +\
                                            ADI_DD_LQ035Q1DH02_VFP));
    }

    /* IF (Successfully configured PPI Lines per Frame Register) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Configure PPI Delay Count Register */
        eResult = adi_dev_Control(poDevice->hPpiDevice,
                                  ADI_PPI_CMD_SET_DELAY_COUNT_REG,
                                  (void *)nFsClockDelay);
    }

    /* IF (Successfully configured PPI Delay Count Register) */
    if (eResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Configure Transfer Count Register */
        eResult = adi_dev_Control(poDevice->hPpiDevice,
                                  ADI_PPI_CMD_SET_TRANSFER_COUNT_REG,
                                  (void *)(((nPixelsPerLine+1) * nClkRatio) - 1));
    }

    /* return */
    return (eResult);
}

#endif  /* ADSP-BF52x/ADSP-BF53x/ADSP-BF56x device */

/* IF (ADSP-BF54x) */
#if defined (__ADSP_MOAB__)

/*********************************************************************

    Function: adi_dd_Lq035q1dh02_PpiFsConfig

        Configures EPPI Frame Sync registers as per LCD requirements

    Parameters:
        poDevice - Pointer to Sharp LQ035Q1DH02 device instance
                   we're working on

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully updated PPI Frame Sync registers
        Error code from Device Manager or PPI Driver

*********************************************************************/
static u32  adi_dd_Lq035q1dh02_PpiFsConfig(
    ADI_DD_LQ035Q1DH02_DEF      *poDevice
)
{

    /* Active Pixels Per Line */
    u16     nPixelsPerLine;
    /* Horizontal Back Porch */
    u16     nHorizontalBackPorch;
    /* Samples per line */
    u16     nSamplesPerLine;
    /* Lines per frame */
    u16     nLinesPerFrame;
    /* Return Code */
    u32     eResult;

    /* Extract pixel per line register field value */
    nPixelsPerLine = (poDevice->oLcdRegs.nPixelPerLine & ADI_DD_LQ035Q1DH02_VALID_PIX_PER_LINE_MASK);
    /* Shift register field to get the exact register field value */
    nPixelsPerLine >>= ADI_DD_LQ035Q1DH02_PIX_PER_LINE_SHIFT;

    /*
    ** Horizontal Back Porch = HBF Field value + 2
    */
    nHorizontalBackPorch = ((poDevice->oLcdRegs.nPixelPerLine & ADI_DD_LQ035Q1DH02_HBP_MASK) + 2U);

    /* Calculate Samples per Line */
    nSamplesPerLine = ((nPixelsPerLine + 1) +\
                       nHorizontalBackPorch +\
                       ADI_DD_LQ035Q1DH02_HFP);

    /* Calculate Lines per Frame */
    nLinesPerFrame = (ADI_DD_LQ035Q1DH02_ACTIVE_LINES_PER_FRAME +\
                      poDevice->oLcdRegs.nVerticalPorch +\
                      ADI_DD_LQ035Q1DH02_VFP);

    do
    {
        /* Set Samples per Line */
        if ((eResult = adi_dev_Control(poDevice->hPpiDevice,
                                       ADI_EPPI_CMD_SET_SAMPLES_PER_LINE,
                                       (void *)nSamplesPerLine))
                                    != ADI_DEV_RESULT_SUCCESS)
        {
            break;
        }

        /* Lines per Frame */
        if ((eResult = adi_dev_Control(poDevice->hPpiDevice,
                                       ADI_EPPI_CMD_SET_LINES_PER_FRAME,
                                       (void *)nLinesPerFrame))
                                    != ADI_DEV_RESULT_SUCCESS)
        {
            break;
        }

        /* FS1 (Hsync) Width */
        if ((eResult = adi_dev_Control(poDevice->hPpiDevice,
                                       ADI_EPPI_CMD_SET_FS1_WIDTH,
                                       (void *)1U))
                                    != ADI_DEV_RESULT_SUCCESS)
        {
            break;
        }

        /* FS1 (Hsync) Period */
        if ((eResult = adi_dev_Control(poDevice->hPpiDevice,
                                       ADI_EPPI_CMD_SET_FS1_PERIOD,
                                       (void *)nSamplesPerLine))
                                    != ADI_DEV_RESULT_SUCCESS)
        {
            break;
        }

        /* Horizontal Delay clock after assertion of Hsync */
        if ((eResult = adi_dev_Control(poDevice->hPpiDevice,
                                       ADI_EPPI_CMD_SET_HORIZONTAL_DELAY,
                                       (void *)(nHorizontalBackPorch - 1)))
                                    != ADI_DEV_RESULT_SUCCESS)
        {
            break;
        }

        /* # active data to transfer after Horizontal Delay clock */
        if ((eResult = adi_dev_Control(poDevice->hPpiDevice,
                                       ADI_EPPI_CMD_SET_SAMPLES_PER_LINE,
                                       (void *)(nPixelsPerLine + 1)))
                                    != ADI_DEV_RESULT_SUCCESS)
        {
            break;
        }

        /* FS2 (Vsync) Width = FS1 (Hsync) Period * 2 */
        if ((eResult = adi_dev_Control(poDevice->hPpiDevice,
                                       ADI_EPPI_CMD_SET_FS2_WIDTH,
                                       (void *)(nSamplesPerLine << 1)))
                                    != ADI_DEV_RESULT_SUCCESS)
        {
            break;
        }

        /* FS2 (Vsync) Period */
        if ((eResult = adi_dev_Control(poDevice->hPpiDevice,
                                       ADI_EPPI_CMD_SET_FS2_PERIOD,
                                       (void *)(nSamplesPerLine * nLinesPerFrame)))
                                    != ADI_DEV_RESULT_SUCCESS)
        {
            break;
        }

        /* Vertical Delay after assertion of Vsync */
        if ((eResult = adi_dev_Control(poDevice->hPpiDevice,
                                       ADI_EPPI_CMD_SET_VERTICAL_DELAY,
                                       (void *)poDevice->oLcdRegs.nVerticalPorch))
                                    != ADI_DEV_RESULT_SUCCESS)
        {
            break;
        }

        /* Vertical Delay after assertion of Vsync */
        if ((eResult = adi_dev_Control(poDevice->hPpiDevice,
                                       ADI_EPPI_CMD_SET_VERTICAL_TX_COUNT,
                                       (void *)ADI_DD_LQ035Q1DH02_ACTIVE_LINES_PER_FRAME))
                                    != ADI_DEV_RESULT_SUCCESS)
        {
            break;
        }

    } while (0);

    /* return */
    return (eResult);
}

#endif  /* ADSP-BF54x device */

/*********************************************************************

    Function: adi_dd_Lq035q1dh02_PpiCallback

        PPI Callback to LCD driver

    Parameters:
        phClient    - Client handle passed to
                      PPI device with adi_dev_Open
        nEvent      - Callback Event
        pArg        - Callback Argument

    Returns:
        None

*********************************************************************/
static void adi_dd_Lq035q1dh02_PpiCallback (
    void    *phClient,
    u32     nEvent,
    void    *pArg
)
{
    /* pointer to the LCD device instant to which this callback belongs */
    ADI_DD_LQ035Q1DH02_DEF  *poDevice = (ADI_DD_LQ035Q1DH02_DEF *)phClient;

/* Debug build only */
#if defined(ADI_DEV_DEBUG)
    /* Validate the PDD handle */
    if (adi_dd_Lq035q1dh02_ValidatePDDHandle(poDevice) == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* simply pass the callback along to the Device Manager Callback */
        (poDevice->pfDMCallback)(poDevice->hDevice,nEvent,pArg);

/* Debug Build Only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

}

/* Debug build only */
#if defined(ADI_DEV_DEBUG)
/*********************************************************************

    Function: adi_dd_Lq035q1dh02_ValidatePDDHandle

        Validates Physical Device Driver Handle

    Parameters:
        hPhysicalDevice    - Physical Device handle to
                             Sharp LQ035Q1DH02 device instance

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Given PDD handle is valid
        ADI_DEV_RESULT_BAD_PDD_HANDLE
            - Given PDD Handle is invalid

*********************************************************************/
static u32  adi_dd_Lq035q1dh02_ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice
)
{
    /* Return value - insure the client has provided a valid PDD Handle */
    u32     eResult = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    /* index */
    u32     nIndex;

    /* compare the given PDD handle with PDDHandle allocated to
       all LCD devices in the list    */
    for (nIndex = 0; nIndex < ADI_DD_LQ035Q1DH02_NUM_DEVICES; nIndex++)
    {
        if (hPhysicalDevice == (ADI_DEV_PDD_HANDLE)&__gaLq035q1dh02Device[nIndex])
        {
            /* Given Physical Device Driver Handle is valid
               quit this loop */
            eResult = ADI_DEV_RESULT_SUCCESS;
            break;
        }
    }
    /* return */
    return (eResult);
}

#endif /* ADI_DEV_DEBUG */

/*****/

/*
**
** EOF:
**
*/

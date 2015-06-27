/* $Revision: 3492 $
 * $Date: 2010-10-07 09:34:20 -0400 (Thu, 07 Oct 2010) $ 
******************************************************************************
Copyright (c), 2007-2009 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title: ADI Device Driver for BF52xC1 Audio CODEC

Description:
    This is primary source file for BF52xC1 Audio CODEC driver

*****************************************************************************/

/*=============  I N C L U D E S   =============*/

/* system service includes  */
#include <services/services.h>
/* device manager includes  */
#include <drivers/adi_dev.h>
/* SPORT driver includes */
#include <drivers/sport/adi_sport.h>
/* Device Access includes */
#include <drivers/deviceaccess/adi_device_access.h>
/* BF52xC1 driver includes */
#include <drivers/codec/adi_bf52xc1.h>
/* type defenition includes */
#include <adi_types.h>

/*=============  D E F I N E S  =============*/

/*
**
**  Enumerations
**
*/

/* Number of BF52xC1 devices in the system  */
#define ADI_BF52xC1_NUM_DEVICES     (sizeof(gaoBF52xC1Device)/sizeof(ADI_BF52XC1_DRIVER_DEF))

/*
**
** Macros to manipulate BF52xC1 hardware register access
**
*/

/* Number of BF52xC1 registers to cache */
#define ADI_BF52xC1_NUM_REGS_TO_CACHE       10U
/* Address of the last register in BF52xC1 */
#define ADI_BF52xC1_FINAL_REG_ADDRESS       0x1FFFU
/* BF52xC1 Register Address mask */
#define ADI_BF52xC1_REG_ADDR_MASK           0x1E00U
/* BF52xC1 Register Data mask */
#define ADI_BF52xC1_REG_DATA_MASK           0x01FFU
/* Shift Count to reach BF52xC1 Register Address */
#define ADI_BF52xC1_REG_ADDR_SHIFT          9U
/* Shift count to reach BF52xC1 Sampling control register - Sampling Rate field */
#define ADI_BF52xC1_SHIFT_SR                2U
/* Shift count to reach BF52xC1 Sampling control register - CLKDIV2 field       */
#define ADI_BF52xC1_SHIFT_CLKDIV2           6U

/*
**
** Macros to manipulate SPORT Register fields
**
*/
/* SPORT Internal Transmit clock select */
#define ADI_SPORT_ITCLK                 0x0002U
/* SPORT Internal Transmit FS select */
#define ADI_SPORT_ITFS                  0x0200U
/* SPORT Transmit FS required */
#define ADI_SPORT_TFSR                  0x0400U
/* SPORT Active Low Transmit FS select */
#define ADI_SPORT_LTFS                  0x1000U
/* SPORT Late Transmit FS select */
#define ADI_SPORT_LATFS                 0x2000U
/* SPORT Transmit Clock Falling edge Select */
#define ADI_SPORT_TCKFE                 0x4000U
/* SPORT Transmit Stereo FS enable */
#define ADI_SPORT_TSFSE                 0x0200U

/* SPORT Internal Receive clock select */
#define ADI_SPORT_IRCLK                 0x0002U
/* SPORT Internal Receive FS select */
#define ADI_SPORT_IRFS                  0x0200U
/* SPORT Receive FS required */
#define ADI_SPORT_RFSR                  0x0400U
/* SPORT Active Low Receive FS select */
#define ADI_SPORT_LRFS                  0x1000U
/* SPORT Late Receive FS select */
#define ADI_SPORT_LARFS                 0x2000U
/* SPORT Receive Clock Falling edge Select */
#define ADI_SPORT_RCKFE                 0x4000U
/* SPORT Receive Stereo FS enable */
#define ADI_SPORT_RSFSE                 0x0200U

/*
**
** SPORT word length (actual wordlength - 1)
**
*/
/* 16 bits */
#define ADI_SPORT_SLEN_16               (15U)
/* 20 bits */
#define ADI_SPORT_SLEN_20               (19U)
/* 24 bits */
#define ADI_SPORT_SLEN_24               (23U)
/* 32 bits */
#define ADI_SPORT_SLEN_32               (31U)

/*
**
** Stereo modes supported by BF52xC1 hardware
**
*/

/* BF52xC1 driver Auto-SPORT configuration does not support
   DSP and Right-Justified modes */
enum
{
    BF52xC1_STEREO_MODE_RJ = 0,
    BF52xC1_STEREO_MODE_LJ,
    BF52xC1_STEREO_MODE_I2S,
    BF52xC1_STEREO_MODE_DSP
};

/*
** List of data word lengths supported by BF52xC1
*/
enum
{
    BF52xC1_WORD_LENGTH_16BITS = 0x00,
    BF52xC1_WORD_LENGTH_20BITS = 0x04,
    BF52xC1_WORD_LENGTH_24BITS = 0x08,
    BF52xC1_WORD_LENGTH_32BITS = 0x0C
};

/*
**
** Data Structures
**
*/

/*
** Structure to hold BF52xC1 Audio Data Interface Port specific information
*/
typedef struct __AdiBf52xc1DriverDataPortInfo
{

    /* true when dataflow is active, false otherwise */
    bool                            bIsActive;

    /* 'true' when automatic SPORT configuration enabled,
        'false' when disabled */
    bool                            bIsAutoSportConfigEnabled;

    /* BF52xC1 device instance number to with this port belongs */
    const u8                        nAudioDevNumber;

    /* SPORT Device number connected to this audio data port */
    u8                              nSportDevNumber;

    /* Data port direction */
    ADI_DEV_DIRECTION               eDirection;

    /* Handle to the SPORT device connected to this audio data port */
    ADI_DEV_PDD_HANDLE              hSport;

    /* Device Manager handle */
    ADI_DEV_MANAGER_HANDLE          hDeviceManager;

    /* Device instance handle provided by Device Manager*/
    ADI_DEV_DEVICE_HANDLE           hDevice;

    /* Handle to the DMA manager */
    ADI_DMA_MANAGER_HANDLE          hDmaManager;

    /* Deferred Callback handle */
    ADI_DCB_HANDLE                  hDcbManager;

} ADI_BF52XC1_DRIVER_DATA_PORT_INFO;

/*
** BF52xC1 Driver instance structure
*/
typedef struct __AdiBf52xc1DriverDef
{
    /* 'true' to use TWI to access device registers, 'false' to use SPI */
    bool                                bUseTwiForRegAccess;

    /* TWI/SPI device number to use to access BF52xC1 registers */
    u8                                  nTwiSpiDevNumber;

    /* TWI global address / SPI chipselect for BF52xC1 register access */
    u8                                  nTwiGAddrSpiCs;

    /* SPORT Transmit control register 1 value for Auto SPORT config */
    u16                                 nTxCtrlReg1;

    /* SPORT Transmit control register 2 value for Auto SPORT config */
    u16                                 nTxCtrlReg2;

    /* SPORT Receive control register 1 value for Auto SPORT config */
    u16                                 nRxCtrlReg1;

    /* SPORT Receive control register 2 value for Auto SPORT config */
    u16                                 nRxCtrlReg2;

    /* BF52xC1 hardware registers cache */
    u16                                 anBF52xC1Reg[ADI_BF52xC1_NUM_REGS_TO_CACHE];

    /* SPI/TWI Configuration Table passed by the client */
    ADI_DEV_CMD_VALUE_PAIR              *pSpiTwiConfigTable;

    /* Callback function supplied by the Device Manager */
    ADI_DCB_CALLBACK_FN                 pfCallback;

    /* Instance to manage audio transmit or bi-directional data */
    ADI_BF52XC1_DRIVER_DATA_PORT_INFO   oAudioTx;

    /* Instance to manage audio receive data */
    ADI_BF52XC1_DRIVER_DATA_PORT_INFO   oAudioRx;

} ADI_BF52XC1_DRIVER_DEF;

/*=============  D A T A  =============*/

/* Create BF52xC1 device instance(s) */
static ADI_BF52XC1_DRIVER_DEF   gaoBF52xC1Device[] =
{
    /* BF52xC1 Device 0 */
    {
        /* Configure to use SPI by default */
        false,
        /* TWI/SPI device number to use */
        0,
        /* TWI Global Address/SPI Chipselect */
        0,
        /* Clear SPORT Tx Control Reg 1 */
        0,
        /* Clear SPORT Tx Control Reg 2 */
        0,
        /* Clear SPORT Rx Control Reg 1 */
        0,
        /* Clear SPORT Rx Control Reg 2 */
        0,
        /* Clear BF52xC1 register cache */
        {
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0
        },
        /* SPI/TWI Configuration Table passed by the client */
        NULL,
        /* Callback function supplied by the Device Manager */
        NULL,
        /* Instance to handle transmit or bi-directional audio data */
        {
            /* Dataflow is not active */
            false,
            /* Enable Auto-SPORT Config */
            true,
            /* BF52xC1 instance number */
            0,
            /* SPORT Device number to use */
            0,
            /* Direction yet to be defined */
            ADI_DEV_DIRECTION_UNDEFINED,
            /* SPORT Device Handle */
            NULL,
            /* Device Manager Handle */
            NULL,
            /* Device instance handle provided by Device Manager */
            NULL,
            /* Handle to the DMA manager */
            NULL,
            /* Deferred Callback handle */
            NULL
        },
        /* Instance to handle audio receive data */
        {
            /* Dataflow is not active */
            false,
            /* Enable Auto-SPORT Config */
            true,
            /* BF52xC1 instance number */
            0,
            /* SPORT Device number to use */
            0,
            /* Direction yet to be defined */
            ADI_DEV_DIRECTION_UNDEFINED,
            /* SPORT Device Handle */
            NULL,
            /* Device Manager Handle */
            NULL,
            /* Device instance handle provided by Device Manager */
            NULL,
            /* Handle to the DMA manager */
            NULL,
            /* Deferred Callback handle */
            NULL
        },
    },

};

/*
**
**  Local Function Prototypes
**
*/

/* Opens BF52xC1 for specified data direction */
static u32 adi_pdd_Open(
    ADI_DEV_MANAGER_HANDLE              hDeviceManager,
    u32                                 nDeviceNumber,
    ADI_DEV_DEVICE_HANDLE               hDevice,
    ADI_DEV_PDD_HANDLE                  *pPDDHandle,
    ADI_DEV_DIRECTION                   eDirection,
    void                                *pEnterCriticalArg,
    ADI_DMA_MANAGER_HANDLE              hDmaManager,
    ADI_DCB_HANDLE                      hDcbManager,
    ADI_DCB_CALLBACK_FN                 pfCallback
);

/* Closes a BF52xC1 device */
static u32 adi_pdd_Close(
    ADI_DEV_PDD_HANDLE                  hPhysicalDevice
);

/* Queues Inbound buffers to BF52xC1 SPORT */
static u32 adi_pdd_Read(
    ADI_DEV_PDD_HANDLE                  hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE                 eBufferType,
    ADI_DEV_BUFFER                      *pBuffer
);

/* Queues Outbound buffers to BF52xC1 SPORT */
static u32 adi_pdd_Write(
    ADI_DEV_PDD_HANDLE                  hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE                 eBufferType,
    ADI_DEV_BUFFER                      *pBuffer
);

/* Sequentially read/writes data to a device
   Function not supported by BF52xC1 */
static u32 adi_pdd_SequentialIO(
    ADI_DEV_PDD_HANDLE                  hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE                 eBufferType,
    ADI_DEV_BUFFER                      *pBuffer
);

/* Sets or senses device specific parameters */
static u32 adi_pdd_Control(
    ADI_DEV_PDD_HANDLE                  hPhysicalDevice,
    u32                                 nCommand,
    void                                *Value
);

/* Function to Access BF52xC1 registers */
static u32 adi_bf52xc1_RegAccess(
    ADI_BF52XC1_DRIVER_DEF              *poDevice,
    u32                                 nCommand,
    void                                *Value
);

/* Writes to a BF52xC1 hardware register using Device Access Service */
static u32 adi_bf52xc1_DeviceAccess(
    ADI_BF52XC1_DRIVER_DEF              *poDevice,
    ADI_DEV_ACCESS_REGISTER             *oAccessBF52xC1
);

/* Function to update BF52xC1 register cache */
static u32 adi_bf52xc1_UpdateDriverCache(
    ADI_BF52XC1_DRIVER_DEF              *poDevice,
    ADI_DEV_ACCESS_REGISTER             *oAccessBF52xC1
);

/* Updates BF52xC1 Sample Control register to match ADC/DAC sampling rate */
static u32 adi_bf52xc1_UpdateSampleRate(
    ADI_BF52XC1_DRIVER_DEF              *poDevice,
    ADI_BF52xC1_SAMPLE_RATE             *poSampleRate
);

/* Function to open a SPORT device */
static u32 adi_bf52xc1_SportOpen(
    ADI_BF52XC1_DRIVER_DATA_PORT_INFO   *poDataPort
);

/* Function to Configure SPORT registers */
static u32 adi_bf52xc1_ConfigSport(
    ADI_BF52XC1_DRIVER_DEF              *poDevice,
    ADI_BF52XC1_DRIVER_DATA_PORT_INFO   *poDataPort
);

/* Callback function passed to SPORT driver */
static void adi_bf52xc1_SportCallback(
    void                                *hCallback,
    u32                                 nEvent,
    void                                *pArgument
);

/*
**
** Debug Mode functions (debug build only)
**
*/
#if defined(ADI_DEV_DEBUG)

/* Validates Physical Device Driver Handle */
static u32 ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE                  hPhysicalDevice
);

#endif

/*
**
** Entry point for device manager
**
*/
ADI_DEV_PDD_ENTRY_POINT     ADIBF52xC1EntryPoint =
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

        Opens BF52xC1 for use in specified data direction

    Parameters:
        hDeviceManager      - Device Manager handle
        nDeviceNumber       - BF52xC1 Device number to open
        hDevice             - Device handle
        pPDDHandle          - Location to store Physical Device Driver Handle
        eDirection          - Data direction to open
        pEnterCriticalArg   - Critical region storage location
        hDmaManager         - Handle to the DMA Manager
        hDcbManager         - Handle to the Deferred Callback Manager
        pfCallback          - Pointer to Device manager callback function

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully opened audio device in specified direction
        ADI_DEV_RESULT_BAD_DEVICE_NUMBER
            - Device Number is invalid
        ADI_DEV_RESULT_DEVICE_IN_USE
            - Device is already in use

*********************************************************************/
static u32 adi_pdd_Open(
    ADI_DEV_MANAGER_HANDLE  hDeviceManager,
    u32                     nDeviceNumber,
    ADI_DEV_DEVICE_HANDLE   hDevice,
    ADI_DEV_PDD_HANDLE      *pPDDHandle,
    ADI_DEV_DIRECTION       eDirection,
    void                    *pEnterCriticalArg,
    ADI_DMA_MANAGER_HANDLE  hDmaManager,
    ADI_DCB_HANDLE          hDcbManager,
    ADI_DCB_CALLBACK_FN     pfCallback
)
{
    /* flag to indicate if device instance is to be reset to default or not */
    bool                                bResetInstance;
    /* Return value - assume we're going to be successful */
    u32                                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the device instance we will be working on */
    ADI_BF52XC1_DRIVER_DEF              *poDevice;
    /* Pointer to the device data port we will be working on */
    ADI_BF52XC1_DRIVER_DATA_PORT_INFO   *poDataPort;
    /* Exit critical region parameter */
    void                                *pExitCriticalArg;
    /* Instance to access a BF52xC1 register */
    ADI_DEV_ACCESS_REGISTER             oAccessBF52xC1;

/* Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* IF (Device Number exceeds the number of device instances */
    if (nDeviceNumber >= ADI_BF52xC1_NUM_DEVICES)
    {
        /* report failure (Invalid Device number) */
        nResult = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    }

    /* IF (Device number is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
#endif
        /* assume that the device client requesting for is
           already in use  */
        nResult = ADI_DEV_RESULT_DEVICE_IN_USE;
        /* BF52xC1 device to work on */
        poDevice = &gaoBF52xC1Device[nDeviceNumber];
        /* assume that the device instance has to be reset*/
        bResetInstance = true;

        /* Entering critical code segment - Protect it from interrupts */
        pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);

        /* IF (Open device in bi-directional mode) */
        if (eDirection == ADI_DEV_DIRECTION_BIDIRECTIONAL)
        {
            /* IF (None of the audio ports are in use) */
            if ((poDevice->oAudioTx.eDirection == ADI_DEV_DIRECTION_UNDEFINED) &&\
                (poDevice->oAudioRx.eDirection == ADI_DEV_DIRECTION_UNDEFINED))
            {
                /* Register transmit port for client */
                poDevice->oAudioTx.eDirection = ADI_DEV_DIRECTION_BIDIRECTIONAL;
                /* Register receive port for client */
                poDevice->oAudioRx.eDirection = ADI_DEV_DIRECTION_BIDIRECTIONAL;
                /* Use transmit data port instance to manage this dataflow */
                poDataPort = &poDevice->oAudioTx;
                /* assume we're going to be successful in
                   initialising the device */
                nResult = ADI_DEV_RESULT_SUCCESS;
            }
        }
        /* ELSE IF (Open device for inbound/receive data) */
        else if (eDirection == ADI_DEV_DIRECTION_INBOUND)
        {
            /* IF (Receive data port is not in use) */
            if (poDevice->oAudioRx.eDirection == ADI_DEV_DIRECTION_UNDEFINED)
            {
                /* Use receive data port instance to manage this dataflow */
                poDataPort = &poDevice->oAudioRx;
                /* Register receive port for client */
                poDataPort->eDirection = eDirection;
                /* IF (transmit port is already in use) */
                if (poDevice->oAudioTx.eDirection != ADI_DEV_DIRECTION_UNDEFINED)
                {
                    /* Do not reset device instance */
                    bResetInstance = false;
                }
                /* assume we're going to be successful in
                   initialising the device */
                nResult = ADI_DEV_RESULT_SUCCESS;
            }
        }
        /* ELSE (Open device for outbound/transmit data) */
        else
        {
            /* IF (Transmit data port is not in use) */
            if (poDevice->oAudioTx.eDirection == ADI_DEV_DIRECTION_UNDEFINED)
            {
                /* Use transmit data port instance to manage this dataflow */
                poDataPort = &poDevice->oAudioTx;
                /* Register transmit port for client */
                poDataPort->eDirection = eDirection;
                /* IF (receive port is already in use) */
                if (poDevice->oAudioRx.eDirection != ADI_DEV_DIRECTION_UNDEFINED)
                {
                    /* Do not reset device instance */
                    bResetInstance = false;
                }
                /* assume we're going to be successful in
                   initialising the device */
                nResult = ADI_DEV_RESULT_SUCCESS;
            }
        }

        /* Exit the critical region to re-enable interrupts */
        adi_int_ExitCriticalRegion(pExitCriticalArg);

        /* IF (Device number is available for use) */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* Save the Device Manager handle */
            poDataPort->hDeviceManager      = hDeviceManager;
            /* Save the Device handle */
            poDataPort->hDevice             = hDevice;
            /* Save the DMA handle */
            poDataPort->hDmaManager         = hDmaManager;
            /* Save the DCB handle */
            poDataPort->hDcbManager         = hDcbManager;
            /* Clear the SPORT handle */
            poDataPort->hSport              = NULL;
            /* By default, use SPORT 0 for audio dataflow */
            poDataPort->nSportDevNumber      = 0;

            /* IF (Reset device instance) */
            if (bResetInstance == true)
            {
                /* Save the Device Manager callback function handle */
                poDevice->pfCallback                = pfCallback;
                /* By default, use SPI to access device registers */
                poDevice->bUseTwiForRegAccess       = false;
                /* By default, use SPI 0 to access BF52xC1 registers */
                poDevice->nTwiSpiDevNumber          = 0;
                /* By default, no valid SPI chipselect is available to select BF52xC1 */
                poDevice->nTwiGAddrSpiCs            = 0;
                /* No SPI or TWI configuration is available */
                poDevice->pSpiTwiConfigTable        = NULL;

                /* Auto-SPORT Configuration is enabled by default */
                poDevice->oAudioTx.bIsAutoSportConfigEnabled = true;
                poDevice->oAudioRx.bIsAutoSportConfigEnabled = true;

                /* Initialise BF52xC1 register cache & SPORT register cache to default values */
                /* issue a dummy BF52xC1 soft-reset within the driver */
                /* Shift the register address (Bit 15 to Bit 9 in BF52xC1 write packet holds the register address to configure) */
                oAccessBF52xC1.Address = ((BF52xC1_REG_RESET << ADI_BF52xC1_REG_ADDR_SHIFT) & ADI_BF52xC1_REG_ADDR_MASK);
                /* Bit 8 to Bit 0 in BF52xC1 write packet holds the register data (zero to reset BF52xC1) */
                /* call update driver cache to reset BF52xC1 register cache & SPORT register cache */
                adi_bf52xc1_UpdateDriverCache(poDevice,&oAccessBF52xC1);
            }

            /* Save physical device handle in the client supplied location */
            *pPDDHandle = (ADI_DEV_PDD_HANDLE *)poDataPort;
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(nResult);
}

/*********************************************************************

    Function: adi_pdd_Close

        Closes a BF52xC1 device data port and/or instance

    Parameters:
        hPhysicalDevice - Physical Device Handle of the device to close

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully closed audio device
        ADI_DEV_RESULT_BAD_PDD_HANDLE
            - Given Physical Device Driver Handle is invalid
        Error return codes from SPORT driver / Device Manager

*********************************************************************/
static u32 adi_pdd_Close(
    ADI_DEV_PDD_HANDLE      hPhysicalDevice
)
{
    /* Return value - assume we're going to be successful */
    u32                                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we will be working on */
    ADI_BF52XC1_DRIVER_DEF              *poDevice;
    /* Pointer to the audio data port we're working on */
    ADI_BF52XC1_DRIVER_DATA_PORT_INFO   *poDataPort = (ADI_BF52XC1_DRIVER_DATA_PORT_INFO *)hPhysicalDevice;

/* Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given Physical Device Driver Handle */
    nResult = ValidatePDDHandle(hPhysicalDevice);

    /* IF (Given Physical Device Driver Handle is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* Get the address of device instance to work on */
        poDevice = &gaoBF52xC1Device[poDataPort->nAudioDevNumber];

        /* IF (The SPORT device connected to this data port is opened) */
        if (poDataPort->hSport != NULL)
        {
            /* Close the SPORT device */
            nResult = adi_dev_Close(poDataPort->hSport);
        }

        /* IF (Successfully closed SPORT device) */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* IF (Data port is opened for bi-directional dataflow) */
            if (poDataPort->eDirection == ADI_DEV_DIRECTION_BIDIRECTIONAL)
            {
                /* Mark both ports of this device instance as not in use */
                poDevice->oAudioTx.eDirection = ADI_DEV_DIRECTION_UNDEFINED;
                poDevice->oAudioRx.eDirection = ADI_DEV_DIRECTION_UNDEFINED;
                /* Clear the SPORT device handle */
                poDevice->oAudioTx.hSport = NULL;
                poDevice->oAudioRx.hSport = NULL;
            }
            /* ELSE (Inbound and outbound must have been managed independently) */
            else
            {
                /* Mark as this port is not in use */
                poDataPort->eDirection = ADI_DEV_DIRECTION_UNDEFINED;
                /* Clear the SPORT device handle */
                poDataPort->hSport = NULL;
            }
        }

/* Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(nResult);
}

/*********************************************************************

    Function: adi_pdd_Read

        Queues Inbound/Receive buffers to BF52xC1 SPORT

    Parameters:
        hPhysicalDevice - Physical Device Handle of the device to work on
        eBufferType     - Buffer type
        pBuffer         - Pointer to data buffer

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully submitted buffer to SPORT
        ADI_DEV_RESULT_BAD_PDD_HANDLE
            - Given Physical Device Driver Handle is invalid
        Error return codes from SPORT driver / Device Manager

*********************************************************************/
static u32 adi_pdd_Read(
    ADI_DEV_PDD_HANDLE      hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE     eBufferType,
    ADI_DEV_BUFFER          *pBuffer
)
{

    /* Return value - assume we're going to be successful */
    u32                                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the audio data port we're working on */
    ADI_BF52XC1_DRIVER_DATA_PORT_INFO   *poDataPort = (ADI_BF52XC1_DRIVER_DATA_PORT_INFO *)hPhysicalDevice;

/* Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given Physical Device Driver Handle */
    nResult = ValidatePDDHandle(hPhysicalDevice);

    /* IF (Given Physical Device Driver Handle is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* Submit the Inbound/Receive buffer to SPORT receive channel */
        nResult = adi_dev_Read(poDataPort->hSport, eBufferType, pBuffer);

/* Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(nResult);
}

/*********************************************************************

    Function: adi_pdd_Write

        Queues Outbound/Transmit buffers to BF52xC1 SPORT

    Parameters:
        hPhysicalDevice - Physical Device Handle of the device to work on
        eBufferType     - Buffer type
        pBuffer         - Pointer to data buffer

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully submitted buffer to SPORT
        ADI_DEV_RESULT_BAD_PDD_HANDLE
            - Given Physical Device Driver Handle is invalid
        Error return codes from SPORT driver / Device Manager

*********************************************************************/
static u32 adi_pdd_Write(
    ADI_DEV_PDD_HANDLE      hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE     eBufferType,
    ADI_DEV_BUFFER          *pBuffer
)
{

    /* Return value - assume we're going to be successful */
    u32                                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the audio data port we're working on */
    ADI_BF52XC1_DRIVER_DATA_PORT_INFO   *poDataPort = (ADI_BF52XC1_DRIVER_DATA_PORT_INFO *)hPhysicalDevice;

/* Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given Physical Device Driver Handle */
    nResult = ValidatePDDHandle(hPhysicalDevice);

    /* IF (Given Physical Device Driver Handle is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* Submit the Outbound/Transmit buffer to SPORT transmit channel */
        nResult = adi_dev_Write(poDataPort->hSport, eBufferType, pBuffer);

/* Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(nResult);
}

/*********************************************************************

    Function: adi_pdd_SequentialIO

        Sequentially read/writes data to a device
        BF52xC1 driver does not support this function

    Parameters:
        hPhysicalDevice - Physical Device Handle of the device to work on
        eBufferType     - Buffer type
        pBuffer         - Pointer to data buffer

    Returns:
        ADI_DEV_RESULT_NOT_SUPPORTED
            - Function not supported by BF52xC1 driver

*********************************************************************/
static u32 adi_pdd_SequentialIO(
    ADI_DEV_PDD_HANDLE      hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE     eBufferType,
    ADI_DEV_BUFFER          *pBuffer
)
{

    /* Function is not supported by BF52xC1 driver */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************

    Function: adi_pdd_Control

        Senses or Configures BF52xC1 device registers

    Parameters:
        hPhysicalDevice - Physical Device Handle of the device to work on
        nCommand        - Command ID
        Value           - Command specific value

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully processed the given command

*********************************************************************/
static u32 adi_pdd_Control(
    ADI_DEV_PDD_HANDLE      hPhysicalDevice,
    u32                     nCommand,
    void                    *Value
)
{

    /* Return value - assume we're going to be successful */
    u32                                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the audio data port to work on */
    ADI_BF52XC1_DRIVER_DATA_PORT_INFO   *poDataPort = (ADI_BF52XC1_DRIVER_DATA_PORT_INFO *)hPhysicalDevice;
    /* Instance to access a BF52xC1 register */
    ADI_DEV_ACCESS_REGISTER             oAccessBF52xC1;
    /* Pointer to the device instance to work on */
    ADI_BF52XC1_DRIVER_DEF              *poDevice;
    /* Instance to set sample rate */
    ADI_BF52xC1_SAMPLE_RATE             oSampleRate;

/* Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given Physical Device Driver Handle */
    nResult = ValidatePDDHandle(hPhysicalDevice);

    /* IF (Given Physical Device Driver Handle is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* Get the address of device instance to work on */
        poDevice = &gaoBF52xC1Device[poDataPort->nAudioDevNumber];

        /* CASEOF (Command ID)  */
        switch (nCommand)
        {
            /* CASE (Control dataflow) */
            case (ADI_DEV_CMD_SET_DATAFLOW):

                /* IF (SPORT is Open) */
                if (poDataPort->hSport != NULL)
                {
                    /* IF (Enable Dataflow) */
                    if ((u32)Value == true)
                    {
                        /* IF (Both Transmit and Receive dataflow are inactive) */
                        if ((poDevice->oAudioTx.bIsActive == false) &&\
                            (poDevice->oAudioRx.bIsActive == false))
                        {
                            /*
                            ** Activate Audio CODEC digital interface
                            */

                            /* Register access to update BF52xC1 Active Control register */
                            oAccessBF52xC1.Address = (BF52xC1_REG_ACTIVE_CTRL << ADI_BF52xC1_REG_ADDR_SHIFT);
                            /* Update field to activate BF52xC1 Digital Audio Interface */
                            oAccessBF52xC1.Address |= 1;
                            /* Update BF52xC1 Active Control register */
                            nResult = adi_bf52xc1_DeviceAccess(poDevice, &oAccessBF52xC1);
                        }

                        /* IF (Successfully updated Active control register) */
                        if (nResult == ADI_DEV_RESULT_SUCCESS)
                        {
                            /* Configure SPORT Registers */
                            nResult = adi_bf52xc1_ConfigSport (poDevice, poDataPort);
                        }

                    } /* End of if (Enable Dataflow) */

                    /* IF (Successfully configured SPORT registers) */
                    if (nResult == ADI_DEV_RESULT_SUCCESS)
                    {
                        /* Enable/Disable SPORT dataflow */
                        nResult = adi_dev_Control(poDataPort->hSport,
                                                  ADI_DEV_CMD_SET_DATAFLOW,
                                                  Value);
                    }

                    /* IF (Successfully enabled/disabled SPORT) */
                    if (nResult == ADI_DEV_RESULT_SUCCESS)
                    {
                        /* Update dataflow status flag of this port */
                        poDataPort->bIsActive = (bool)((u32)Value);

                        /* IF (Both Transmit and Receive dataflow are inactive) */
                        if ((poDevice->oAudioTx.bIsActive == false) &&\
                            (poDevice->oAudioRx.bIsActive == false))
                        {
                            /*
                            ** De-activate Audio CODEC digital interface
                            */

                            /* Register access to update BF52xC1 Active Control register */
                            oAccessBF52xC1.Address = (BF52xC1_REG_ACTIVE_CTRL << ADI_BF52xC1_REG_ADDR_SHIFT);
                            /* Update BF52xC1 Active Control register */
                            nResult = adi_bf52xc1_DeviceAccess(poDevice, &oAccessBF52xC1);
                        }
                    }
                }
                /* ELSE (SPORT is not open yet) */
                else
                {
                    /* IF (Enable dataflow) */
                    if ((u32)Value == true)
                    {
                        /* report failure (Dataflow method undefined) */
                        nResult = ADI_DEV_RESULT_DATAFLOW_UNDEFINED;
                    }
                }

                break;

            /* CASE (Query for processor DMA support) */
            case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):

                /* BF52xC1 doesn't support DMA, but supports indirectly via SPORT */
                *((u32 *)Value) = FALSE;
                break;

            /* CASE (Set Dataflow method) */
            case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):

                /* IF (SPORT device connected to this port is not open yet) */
                if (poDataPort->hSport == NULL)
                {
                    /* Try to open the SPORT device */
                    nResult = adi_bf52xc1_SportOpen(poDataPort);
                }

                /* IF (Successfully opened SPORT) */
                if (nResult == ADI_DEV_RESULT_SUCCESS)
                {
                    /* Pass the dataflow method to the SPORT device */
                    nResult = adi_dev_Control(poDataPort->hSport, nCommand, Value);
                }
                break;

            /************************
            SPI related commands
            ************************/

            /* CASE (Set BF52xC1 driver to use SPI to access BF52xC1 hardware registers) */
            case (ADI_BF52xC1_CMD_SET_SPI_DEVICE_NUMBER):
                /* Update SPI device number */
                poDevice->nTwiSpiDevNumber      = (u8)((u32)Value);
                /* Update flag to use SPI for register access */
                poDevice->bUseTwiForRegAccess   = false;
                break;

            /* CASE (Set SPI Chipselect for BF52xC1) */
            case (ADI_BF52xC1_CMD_SET_SPI_CS):
                /* Store the chipselect value */
                poDevice->nTwiGAddrSpiCs = (u8)((u32)Value);
                break;

            /************************
            TWI related commands
            ************************/

            /* CASE (Set BF52xC1 driver to use TWI to access BF52xC1 hardware registers) */
            case (ADI_BF52xC1_CMD_SET_TWI_DEVICE_NUMBER):
                /* Update TWI device number */
                poDevice->nTwiSpiDevNumber      = (u8)((u32)Value);
                /* Update flag to use TWI for register access */
                poDevice->bUseTwiForRegAccess   = true;

                /* IF (TWI Global address is not valid) */
                if ((poDevice->nTwiGAddrSpiCs != 0x1A) &&\
                    (poDevice->nTwiGAddrSpiCs != 0x1B))
                {
                    /* Set default TWI Global Address for BF52xC1 */
                    poDevice->nTwiGAddrSpiCs = 0x1A;
                }
                break;

            /* CASE: Set TWI Global Address for BF52xC1 */
            case (ADI_BF52xC1_CMD_SET_TWI_GLOBAL_ADDR):

/* Debug build only - validate TWI Global Address */
#if defined(ADI_DEV_DEBUG)

                if (((u8)((u32)Value) != 0x1A) && ((u8)((u32)Value) != 0x1B))
                {
                    /* report failure (TWI Global address is invalid) */
                    nResult = ADI_BF52xC1_RESULT_TWI_GLOBAL_ADDRESS_INVALID;
                    break;
                }

#endif  /* ADI_DEV_DEBUG */

                /* Update TWI Global address for BF52xC1 */
                poDevice->nTwiGAddrSpiCs = (u8)((u32)Value);
                break;

            /****************************
            TWI and SPI related commands
            ****************************/

            /* CASE (Set TWI or SPI configuration table to be used to access BF52xC1 device registers) */
            case (ADI_BF52xC1_CMD_SET_TWI_SPI_CONFIG_TABLE):
                /* Store the configuration table address */
                poDevice->pSpiTwiConfigTable = (ADI_DEV_CMD_VALUE_PAIR*) Value;
                break;

            /************************
            SPORT related commands
            ************************/

            /* CASE: Set SPORT device number connected to BF52xC1 Digital Audio Interface port */
            case (ADI_BF52xC1_CMD_SET_SPORT_DEVICE_NUMBER):

                /* IF (SPORT device already open) */
                if (poDataPort->hSport != NULL)
                {
                    /* Close the SPORT device */
                    nResult = adi_dev_Close(poDataPort->hSport);

/* Debug build only - exit on error */
#if defined(ADI_DEV_DEBUG)
                    if (nResult != ADI_DEV_RESULT_SUCCESS)
                    {
                        break;
                    }
#endif  /* ADI_DEV_DEBUG */

                    /* Clear SPORT device handle */
                    poDataPort->hSport = NULL;
                }

                /* Update SPORT device number */
                poDataPort->nSportDevNumber = (u8)((u32)Value);

                /* Application should set dataflow method for this SPORT device, load data buffer(s) & Enable dataflow */

                break;

            /* CASE (Open/Close SPORT device connected to BF52xC1) */
            case (ADI_BF52xC1_CMD_OPEN_SPORT_DEVICE):

                /* IF (Open SPORT device) */
                if ((u32)Value == true)
                {
                    /* IF (SPORT device allocated to BF52xC1 is not open yet) */
                    if (poDataPort->hSport == NULL)
                    {
                        /* Open the SPORT device for BF52xC1 use */
                        nResult = adi_bf52xc1_SportOpen(poDataPort);
                    }
                }
                /* ELSE (Close SPORT device) */
                else
                {
                    /* IF (SPORT device already open) */
                    if (poDataPort->hSport != NULL)
                    {
                        /* Close the SPORT device */
                        nResult = adi_dev_Close(poDataPort->hSport);

/* for Debug build only - exit on error */
#if defined(ADI_DEV_DEBUG)
                        if (nResult != ADI_DEV_RESULT_SUCCESS)
                        {
                            break;
                        }
#endif  /* ADI_DEV_DEBUG */

                        /* Clear SPORT device handle */
                        poDataPort->hSport = NULL;
                    }
                }
                break;

            /* CASE  (Enable/Disable Auto-SPORT configuration mode) */
            case (ADI_BF52xC1_CMD_ENABLE_AUTO_SPORT_CONFIG):
                /* Update Automatic SPORT configuration flag */
                poDataPort->bIsAutoSportConfigEnabled = (u8)((u32)Value);
                break;

            /* CASE (Set BF52xC1 Sampling Control register to given ADC and DAC sample rate) */
            case (ADI_BF52xC1_CMD_SET_SAMPLE_RATE):

                /* Update BF52xC1 sampling control register */
                nResult = adi_bf52xc1_UpdateSampleRate(poDevice, (ADI_BF52xC1_SAMPLE_RATE*)Value);

                /* IF (Sample rate is not supported and
                       device not operated in bi-directional mode) */
                if ((nResult == ADI_BF52xC1_RESULT_SAMPLE_RATE_NOT_SUPPORTED) &&\
                    (poDataPort->eDirection != ADI_DEV_DIRECTION_BIDIRECTIONAL))
                {
                    /* IF (Application is trying to set output sample rate) */
                    if (poDataPort->eDirection == ADI_DEV_DIRECTION_OUTBOUND)
                    {
                        /* IF (Input port is not open yet) */
                        if (poDevice->oAudioRx.hSport == NULL)
                        {
                            /* Copy Output sample rate to local instance */
                            oSampleRate.DacSampleRate = ((ADI_BF52xC1_SAMPLE_RATE*)Value)->DacSampleRate;
                            /* Set Input sample rate same as Output */
                            oSampleRate.AdcSampleRate = oSampleRate.DacSampleRate;
                            /* Reset result code */
                            nResult = ADI_DEV_RESULT_SUCCESS;
                        }
                    }
                    /* ELSE (Application must be trying to set input sample rate) */
                    else
                    {
                        /* IF (Output port is not open yet) */
                        if (poDevice->oAudioTx.hSport == NULL)
                        {
                            /* Copy Input sample rate to local instance */
                            oSampleRate.AdcSampleRate = ((ADI_BF52xC1_SAMPLE_RATE*)Value)->AdcSampleRate;
                            /* Set Output sample rate same as Input */
                            oSampleRate.DacSampleRate = oSampleRate.AdcSampleRate;
                            /* Reset result code */
                            nResult = ADI_DEV_RESULT_SUCCESS;
                        }
                    }

                    /* IF (Result code was reset) */
                    if (nResult == ADI_DEV_RESULT_SUCCESS)
                    {
                        /* Try to update sample rate again */
                        nResult = adi_bf52xc1_UpdateSampleRate(poDevice, &oSampleRate);
                    }
                }

                break;

            /* other commands */
            default:
                /* Pass this nCommand to BF52xC1 Register Access Function */
                nResult = adi_bf52xc1_RegAccess(poDevice, nCommand, Value);

                /* IF (Command not supported by BF52xC1) */
                if (nResult == ADI_BF52xC1_RESULT_CMD_NOT_SUPPORTED)
                {
                    /* Pass this command to SPORT driver */
                    nResult = adi_dev_Control (poDataPort->hSport,nCommand, Value);
                }
                break;

        /* ENDCASE */
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(nResult);
}

/*********************************************************************

    Function: adi_bf52xc1_RegAccess

        Configures BF52xC1 hardware registers,
        Reads BF52xC1 register values from driver cache

    Parameters:
        poDevice - Pointer to BF52xC1 driver instance to work on
        nCommand - Command ID passed by the application
        Value    - Comand specific value

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully processed the given register access command
        Return codes from Device Access

*********************************************************************/
static u32 adi_bf52xc1_RegAccess(
    ADI_BF52XC1_DRIVER_DEF  *poDevice,
    u32                     nCommand,
    void                    *Value
)
{
    /* default return code */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;

    /* pointer to selective register access table */
    ADI_DEV_ACCESS_REGISTER         *pSelectiveAccess;
    /* pointer to block register access table */
    ADI_DEV_ACCESS_REGISTER_BLOCK   *pBlockAccess;
    /* pointer to register field access table */
    ADI_DEV_ACCESS_REGISTER_FIELD   *pFieldAccess;

    /* register access table to be passed to the Device Access Service */
    ADI_DEV_ACCESS_REGISTER         oAccessBF52xC1;

    u16 RegAddr,RegData,FieldData,i,CacheIndex;

    /* CASEOF (Device Access nCommand) */
    switch (nCommand)
    {
        /* CASE (Read a specific BF52xC1 Register) */
        case (ADI_DEV_CMD_REGISTER_READ):
        /* CASE (Read a table of BF52xC1 registers) */
        case (ADI_DEV_CMD_REGISTER_TABLE_READ):

            /* get the address of selective register access table submitted by the app */
            pSelectiveAccess = (ADI_DEV_ACCESS_REGISTER *) Value;

            /* access all registers in application provided access table until we reach the delimiter */
            while (pSelectiveAccess->Address != ADI_DEV_REGEND)
            {
                /* copy the register value from driver register cache */
                pSelectiveAccess->Data = poDevice->anBF52xC1Reg[pSelectiveAccess->Address];

                /* IF (the nCommand is to access a single register) */
                if (nCommand == ADI_DEV_CMD_REGISTER_READ)
                {
                    /* done with register access. exit this loop */
                    break;
                }
                /* move to next selective register access info */
                pSelectiveAccess++;
            }
            break;

        /* CASE (Configure a BF52xC1 Resigter) */
        case (ADI_DEV_CMD_REGISTER_WRITE):
        /* CASE (Configure a table BF52xC1 registers) */
        case (ADI_DEV_CMD_REGISTER_TABLE_WRITE):

            /* get the address of selective register access table submitted by the app */
            pSelectiveAccess = (ADI_DEV_ACCESS_REGISTER *) Value;

            /* access all registers in application provided access table until we reach the delimiter */
            while (pSelectiveAccess->Address != ADI_DEV_REGEND)
            {
                /*** Packet this register address and data as per BF52xC1 register configuration requirements ***/
                /* Shift the register address (Bit 15 to Bit 9 in BF52xC1 write packet holds the register address to configure) */
                oAccessBF52xC1.Address = ((pSelectiveAccess->Address << ADI_BF52xC1_REG_ADDR_SHIFT) & ADI_BF52xC1_REG_ADDR_MASK);
                /* Bit 8 to Bit 0 in BF52xC1 write packet holds the register data */
                oAccessBF52xC1.Address |= (pSelectiveAccess->Data & ADI_BF52xC1_REG_DATA_MASK);

                /* Pass the new register data to device access service which inturn configures BF52xC1 */
                if((nResult = adi_bf52xc1_DeviceAccess(poDevice,
                                                       &oAccessBF52xC1))
                                                    != ADI_DEV_RESULT_SUCCESS)
                {
                    break;
                }

                /* IF (the nCommand is to access a single register) */
                if (nCommand == ADI_DEV_CMD_REGISTER_WRITE)
                {
                    break;  /* done with register access. exit this loop */
                }
                /* move to next selective register access info */
                pSelectiveAccess++;
            }
            break;

        /* CASE (Read a specific BF52xC1 register field) */
        case (ADI_DEV_CMD_REGISTER_FIELD_READ):
        /* CASE (Read a table BF52xC1 register(s) field(s)) */
        case (ADI_DEV_CMD_REGISTER_FIELD_TABLE_READ):

            /* get the address of Field access table submitted by the app */
            pFieldAccess = (ADI_DEV_ACCESS_REGISTER_FIELD *) Value;

            /* access all registers in application provided access table until we reach the delimiter */
            while (pFieldAccess->Address != ADI_DEV_REGEND)
            {
                /* read register value from driver register cache & extract the field data we're interested in */
                FieldData = (poDevice->anBF52xC1Reg[pFieldAccess->Address] & pFieldAccess->Field);

                /* use the register field mask to determine the
                   shift count required to reach first bit of this register field */
                for (i = pFieldAccess->Field; i; i >>= 1)
                {
                    /* check if we've reached the first bit location of this field */
                    if (i & 1)
                    {
                        /* reached first bit of this field. exit this loop */
                        break;
                    }
                    /* shift 'FieldData' to report the exact field value back to application */
                    FieldData >>= 1;
                }

                /* pass this register field value to the application */
                pFieldAccess->Data = FieldData;

                /* IF (the nCommand is to access a single register) */
                if (nCommand == ADI_DEV_CMD_REGISTER_FIELD_READ)
                {
                    /* done with register access. exit this loop */
                    break;
                }

                /* move to next register field access info */
                pFieldAccess++;
            }

            break;

        /* CASE (Configure a specific BF52xC1 register field) */
        case (ADI_DEV_CMD_REGISTER_FIELD_WRITE):
        /* CASE (Configure a table BF52xC1 register(s) field(s)) */
        case (ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE):

            /* get the address of Field access table submitted by the app */
            pFieldAccess = (ADI_DEV_ACCESS_REGISTER_FIELD *) Value;

            /* access all registers in application provided access table until we reach the delimiter */
            while (pFieldAccess->Address != ADI_DEV_REGEND)
            {
                /* get the present register data from driver register cache */
                RegData = poDevice->anBF52xC1Reg[pFieldAccess->Address];
                /* Get the register field data passed by teh application */
                FieldData = pFieldAccess->Data;

                /* use the register field mask to determine the
                   shift count required to reach first bit of this register field */
                for (i = pFieldAccess->Field; i; i >>= 1)
                {
                    /* check if we've reached the first bit location of this field */
                    if (i & 1)
                    {
                        /* reached first bit of this field. exit this loop */
                        break;
                    }
                    /* shift the 'FieldData' until we reach its corresponding field location */
                    FieldData <<= 1;
                }
                /* clear the register field that we're about to update */
                RegData &= ~pFieldAccess->Field;
                /* update the register field with new value */
                RegData |= (FieldData & pFieldAccess->Field);

                /*** Packet this register address and data as per BF52xC1 register configuration requirements ***/
                /* Shift the register address (Bit 15 to Bit 9 in BF52xC1 write packet holds the register address to configure) */
                oAccessBF52xC1.Address = ((pFieldAccess->Address << ADI_BF52xC1_REG_ADDR_SHIFT) & ADI_BF52xC1_REG_ADDR_MASK);
                /* Bit 8 to Bit 0 in BF52xC1 write packet holds the register data */
                oAccessBF52xC1.Address |= RegData;

                /* Pass the new register data to device access service which inturn configures BF52xC1 */
                if ((nResult = adi_bf52xc1_DeviceAccess(poDevice,
                                                        &oAccessBF52xC1))
                                                     != ADI_DEV_RESULT_SUCCESS)
                {
                    break;
                }

                /* IF (the nCommand is to access a single register) */
                if (nCommand == ADI_DEV_CMD_REGISTER_FIELD_WRITE)
                {
                    /* done with register access. exit this loop */
                    break;
                }

                /* move to next register field access info */
                pFieldAccess++;
             }

             break;

        /* CASE (Read block of BF52xC1 registers starting from first given address) */
        case (ADI_DEV_CMD_REGISTER_BLOCK_READ):

            /* get the address of Block access table submitted by the app */
            pBlockAccess = (ADI_DEV_ACCESS_REGISTER_BLOCK *) Value;
            /* Load the register block start address */
            RegAddr = pBlockAccess->Address;

            /* Access all the registers requested by the application */
            for (i = 0; i < pBlockAccess->Count ; i++)
            {
                /* copy the register value from driver register cache */
                *(pBlockAccess->pData+i) = poDevice->anBF52xC1Reg[RegAddr];

                /* IF (Register address exceeds the limit) */
                if (RegAddr > BF52xC1_REG_ACTIVE_CTRL)
                {
                    /* no more registers left to read */
                    break;
                }
                /* ELSE (More registers to access) */
                else
                {
                    /* Move to next register address to access */
                    RegAddr++;
                }
            }

            break;

        /* CASE (Write to a block of BF52xC1 registers starting from first given address) */
        case (ADI_DEV_CMD_REGISTER_BLOCK_WRITE):

            /* get the address of Block access table submitted by the app */
            pBlockAccess = (ADI_DEV_ACCESS_REGISTER_BLOCK *) Value;
            /* Load the register block start address */
            RegAddr = pBlockAccess->Address;

            /* Access all the registers requested by the application */
            for (i = 0; i < pBlockAccess->Count ; i++)
            {
                /*** Packet this register address and data as per BF52xC1 register configuration requirements ***/
                /* Shift the register address (Bit 15 to Bit 9 in BF52xC1 write packet holds the register address to configure) */
                oAccessBF52xC1.Address = ((RegAddr << ADI_BF52xC1_REG_ADDR_SHIFT) & ADI_BF52xC1_REG_ADDR_MASK);
                /* Bit 8 to Bit 0 in BF52xC1 write packet holds the register data */
                oAccessBF52xC1.Address |= (*(pBlockAccess->pData+i) & ADI_BF52xC1_REG_DATA_MASK);

                /* Pass the new register data to device access service which inturn configures BF52xC1 */
                if ((nResult = adi_bf52xc1_DeviceAccess(poDevice,
                                                        &oAccessBF52xC1))
                                                     != ADI_DEV_RESULT_SUCCESS)
                {
                    break;
                }

                /* IF (Last register configured == Active Control register) */
                if (RegAddr == BF52xC1_REG_ACTIVE_CTRL)
                {
                    /* next register address to configure */
                    RegAddr = BF52xC1_REG_RESET;
                }
                /* ELSE IF (Register address exceeds the limit) */
                else if (RegAddr > BF52xC1_REG_RESET)
                {
                    /* no registers left to configure */
                    break;
                }
                else
                {
                    /* Move to next register address to access */
                    RegAddr++;
                }
            }

            break;

        /* other commands */
        default:

            /* Report failure (Command not supported) */
            nResult = ADI_BF52xC1_RESULT_CMD_NOT_SUPPORTED;
            break;

    /* ENDCASE */
    }

    return(nResult);
}

/*********************************************************************

    Function: adi_bf52xc1_DeviceAccess

        Updates a BF52xC1 hardware register using Device Access Service

    Parameters:
        poDevice        - Pointer to BF52xC1 driver instance to work on
        oAccessBF52xC1  - BF52xC1 register access info

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully accessed BF52xC1 register
        Return codes from Device Access

*********************************************************************/
static u32 adi_bf52xc1_DeviceAccess(
    ADI_BF52XC1_DRIVER_DEF      *poDevice,
    ADI_DEV_ACCESS_REGISTER     *oAccessBF52xC1
)
{
    /* default return code */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;
    /* create a device access data instance */
    ADI_DEVICE_ACCESS_REGISTERS ConfigBF52xC1;
    /* structure to hold Device type and access type to be used */
    ADI_DEVICE_ACCESS_SELECT    AccessSelect;
    /* BF52xC1 register access info for TWI based register access */
    ADI_DEV_ACCESS_REGISTER     DevAccessTable;

    /* IF (Client set to use SPI to access BF52xC1 registers) */
    if (poDevice->bUseTwiForRegAccess == false)
    {
        /* This device falls under special case SPI access mode */
        /* SPI Chip-select for BF52xC1 */
        AccessSelect.DeviceCS   = poDevice->nTwiGAddrSpiCs;
        /* No SPI Global address for BF52xC1 */
        AccessSelect.Gaddr_len  = ADI_DEVICE_ACCESS_LENGTH0;
        /* Register address = 2bytes (Register address & data merged as one packet) */
        AccessSelect.Raddr_len  = ADI_DEVICE_ACCESS_LENGTH2;
        /* Passing this as 0 sets Device access to consider BF52xC1 as special case SPI device */
        AccessSelect.Rdata_len  = ADI_DEVICE_ACCESS_LENGTH0;
        /* Use SPI to configure BF52xC1 registers */
        AccessSelect.AccessType = ADI_DEVICE_ACCESS_TYPE_SPI;

        /* Populate Device Access instance fields specific to SPI */
        /* No SPI Global address for BF52xC1    */
        ConfigBF52xC1.DeviceAddress  = 0;
        /* Command specific value */
        ConfigBF52xC1.Value          = (void*)oAccessBF52xC1;
    }
    /* ELSE (Client has choosen to use TWI to access BF52xC1 registers) */
    else
    {
        /* Don't care for TWI access */
        AccessSelect.DeviceCS   = 0;
        /* Don't care for TWI access */
        AccessSelect.Gaddr_len  = ADI_DEVICE_ACCESS_LENGTH0;
        /* Register address length */
        AccessSelect.Raddr_len  = ADI_DEVICE_ACCESS_LENGTH1;
        /* Register data length */
        AccessSelect.Rdata_len  = ADI_DEVICE_ACCESS_LENGTH1;
        /* Use TWI to configure BF52xC1 registers */
        AccessSelect.AccessType = ADI_DEVICE_ACCESS_TYPE_TWI;

        /* Split the BF52xC1 datapacket for TWI access mode */
        /* Bits 15:8 passed as address field */
        DevAccessTable.Address  = (oAccessBF52xC1->Address >> 8 & 0xFF);
        /* Bits 7:0 passed as data field */
        DevAccessTable.Data     = (oAccessBF52xC1->Address & 0xFF);

        /* Populate Device Access instance fields specific to TWI */
        /* TWI Address for BF52xC1 */
        ConfigBF52xC1.DeviceAddress  = poDevice->nTwiGAddrSpiCs;
        /* Command specific value */
        ConfigBF52xC1.Value          = (void*)&DevAccessTable;
    }

    /* IF (Device is opened in transmit/bi-directional mode) */
    if (poDevice->oAudioTx.eDirection != ADI_DEV_DIRECTION_UNDEFINED)
    {
        /* Device manager handle */
        ConfigBF52xC1.ManagerHandle = poDevice->oAudioTx.hDeviceManager;
        /* handle to the callback manager */
        ConfigBF52xC1.DCBHandle     = poDevice->oAudioTx.hDcbManager;
    }
    /* ELSE (Device must be opened for receive mode) */
    else
    {
        /* Device manager handle */
        ConfigBF52xC1.ManagerHandle = poDevice->oAudioRx.hDeviceManager;
        /* handle to the callback manager */
        ConfigBF52xC1.DCBHandle     = poDevice->oAudioRx.hDcbManager;
    }

    /* No client handle */
    ConfigBF52xC1.ClientHandle      = NULL;
    /* SPI/TWI Device number to use */
    ConfigBF52xC1.DeviceNumber      = poDevice->nTwiSpiDevNumber;
    /* No Callback from device access */
    ConfigBF52xC1.DeviceFunction    = NULL;
    /* nCommand ID to configure a register */
    ConfigBF52xC1.Command           = ADI_DEV_CMD_REGISTER_WRITE;
    /* Address of the last register BF52xC1 (with max data) */
    ConfigBF52xC1.FinalRegAddr      = ADI_BF52xC1_FINAL_REG_ADDRESS;
    /* No need to check for register field errors */
    ConfigBF52xC1.RegisterField     = NULL;
    /* No need to update for reserved bits */
    ConfigBF52xC1.ReservedValues    = NULL;
    /* Register address already validated. no need to repeat */
    ConfigBF52xC1.ValidateRegister  = NULL;
    /* SPI/TWI configuration table passed by the client */
    ConfigBF52xC1.ConfigTable       = poDevice->pSpiTwiConfigTable;
    /* Device Access type */
    ConfigBF52xC1.SelectAccess      = &AccessSelect;
    /* No Additional info */
    ConfigBF52xC1.pAdditionalinfo   = (void *)NULL;

    /* Updates driver register cache */
    nResult = adi_bf52xc1_UpdateDriverCache(poDevice, oAccessBF52xC1);

    /* IF (Updates driver register cache resulted in success) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* call device access to configure corresponding BF52xC1 register */
        nResult = adi_device_access (&ConfigBF52xC1);
    }

    /* return */
    return (nResult);
}

/*********************************************************************

    Function: adi_bf52xc1_UpdateDriverCache

        Updates driver register cache to keep in track of
        BF52xC1 hardware register changes

    Parameters:
        poDevice        - Pointer to BF52xC1 driver instance to work on
        oAccessBF52xC1  - BF52xC1 register access info

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully updated BF52xC1 register cache
        Return codes from Device Access

*********************************************************************/
static u32 adi_bf52xc1_UpdateDriverCache(
    ADI_BF52XC1_DRIVER_DEF      *poDevice,
    ADI_DEV_ACCESS_REGISTER     *oAccessBF52xC1
)
{
    u8  RegAddr;
    u16 u16Temp;
    /* default return code */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;

    /* Extract the register address from the given register access info */
    RegAddr = (u8) ((oAccessBF52xC1->Address & ADI_BF52xC1_REG_ADDR_MASK) >> ADI_BF52xC1_REG_ADDR_SHIFT);

    /* IF (Configure Reset Register) */
    if (RegAddr == BF52xC1_REG_RESET)
    {
        /* IF (soft-reset BF52xC1) */
        if (!(oAccessBF52xC1->Address & ADI_BF52xC1_REG_DATA_MASK))
        {
            /*
            **Initialise BF52xC1 register cache to default values
            */
            /* Left Line In Mute, 0dB */
            poDevice->anBF52xC1Reg[BF52xC1_REG_LEFT_ADC_VOL]    = 0x0097;
            /* Right Line In Mute, 0dB */
            poDevice->anBF52xC1Reg[BF52xC1_REG_RIGHT_ADC_VOL]   = 0x0097;
            /* Left HP volume = 0dB */
            poDevice->anBF52xC1Reg[BF52xC1_REG_LEFT_DAC_VOL]    = 0x0079;
            /* Right HP volume = 0dB */
            poDevice->anBF52xC1Reg[BF52xC1_REG_RIGHT_DAC_VOL]   = 0x0079;
            /* Enable Bypass, MIC Mute enabled */
            poDevice->anBF52xC1Reg[BF52xC1_REG_ANALOGUE_PATH]   = 0x000A;
            /* Enable DAC Soft mute */
            poDevice->anBF52xC1Reg[BF52xC1_REG_DIGITAL_PATH]    = 0x0008;
            /* Power-down all except ClkOut & Oscillator */
            poDevice->anBF52xC1Reg[BF52xC1_REG_POWER]           = 0x009F;
            /* I2S mode, 24-bits */
            poDevice->anBF52xC1Reg[BF52xC1_REG_DIGITAL_IFACE]   = 0x000A;
            /* Normal mode, 256fs */
            poDevice->anBF52xC1Reg[BF52xC1_REG_SAMPLING_CTRL]   = 0;
            /* BF52xC1 is Not Active */
            poDevice->anBF52xC1Reg[BF52xC1_REG_ACTIVE_CTRL]     = 0;

            /* Initialise Sport register cache to default values */
            poDevice->nTxCtrlReg1 = (ADI_SPORT_TFSR  | ADI_SPORT_TCKFE | ADI_SPORT_ITCLK | ADI_SPORT_ITFS);
            poDevice->nTxCtrlReg2 = (ADI_SPORT_TSFSE | ADI_SPORT_SLEN_24);
            poDevice->nRxCtrlReg1 = (ADI_SPORT_RFSR  | ADI_SPORT_RCKFE | ADI_SPORT_IRCLK | ADI_SPORT_IRFS);
            poDevice->nRxCtrlReg2 = (ADI_SPORT_RSFSE | ADI_SPORT_SLEN_24);
        }
    }
    /* ELSE IF (registers between ADC volume and Active Control) */
    else if (RegAddr <= BF52xC1_REG_ACTIVE_CTRL)
    {
        /* update driver register cache */
        poDevice->anBF52xC1Reg[RegAddr] = (oAccessBF52xC1->Address & ADI_BF52xC1_REG_DATA_MASK);

        /* extract BF52xC1 audio input format */
        u16Temp = (poDevice->anBF52xC1Reg[BF52xC1_REG_DIGITAL_IFACE] & BF52xC1_RFLD_IFACE_FORMAT);

        /*
        ** Update SPORT control register values cached by driver for
        ** Auto-SPORT configuration feature.
        ** Auto-SPORT Configuration is no not support when
        ** Audio CODEC is configured for Right-Justified or DSP mode
        */

        /* IF (Left Justified or I2S mode) */
        if ((u16Temp == BF52xC1_STEREO_MODE_LJ) ||\
            (u16Temp == BF52xC1_STEREO_MODE_I2S))
        {
            /* Calculate new SPORT register configuration value */
            poDevice->nTxCtrlReg1 = ADI_SPORT_TFSR;
            poDevice->nTxCtrlReg2 = ADI_SPORT_TSFSE;
            poDevice->nRxCtrlReg1 = ADI_SPORT_RFSR;
            poDevice->nRxCtrlReg2 = ADI_SPORT_RSFSE;

            /* IF (Audio Data Format is set to Left-Justified Mode) */
            if (u16Temp == BF52xC1_STEREO_MODE_LJ)
            {
                /* IF (DACLR Phase not Inverted) */
                if (!(poDevice->anBF52xC1Reg[BF52xC1_REG_DIGITAL_IFACE] & BF52xC1_RFLD_DAC_LR_POLARITY))
                {
                    /* Active Low TFS (Right channel DAC data when TFS is low) */
                    poDevice->nTxCtrlReg1 |= ADI_SPORT_LTFS;
                    /* Active Low RFS (Right channel DAC data when RFS is low) */
                    poDevice->nRxCtrlReg1 |= ADI_SPORT_LRFS;
                }
                /* ELSE (DACLR Phase Inverted) */
                    /* Active High TFS (Right channel DAC data when TFS is High) */
                    /* Active High RFS (Right channel DAC data when RFS is High) */

                /* IF (Bit Clock not Inverted) */
                if (!(poDevice->anBF52xC1Reg[BF52xC1_REG_DIGITAL_IFACE] & BF52xC1_RFLD_BCLK_INVERT))
                {
                    /* Drive Data & Internal Frame Syncs with falling edge of TSCLK. Sample External FS with rising edge of TSCLK */
                    poDevice->nTxCtrlReg1 |= ADI_SPORT_TCKFE;
                    /* Drive Data & Internal Frame Syncs with falling edge of RSCLK. Sample External FS with rising edge of RSCLK */
                    poDevice->nRxCtrlReg1 |= ADI_SPORT_RCKFE;
                }
                /* ELSE (Bit Clock is Inverted) */
                    /* Drive Data & Internal Frame Syncs with rising edge of TSCLK. Sample External FS with falling edge of TSCLK */
                    /* Drive Data & Internal Frame Syncs with rising edge of RSCLK. Sample External FS with falling edge of RSCLK */
            }
            /* ELSE (Audio Data Format is set to I2S Mode) */
            else
            {
                /* IF (DACLR Phase Inverted) */
                if (poDevice->anBF52xC1Reg[BF52xC1_REG_DIGITAL_IFACE] & BF52xC1_RFLD_DAC_LR_POLARITY)
                {
                    /* Active Low TFS (Right channel DAC data when TFS is low) */
                    poDevice->nTxCtrlReg1 |= ADI_SPORT_LTFS;
                    /* Active Low RFS (Right channel DAC data when RFS is low) */
                    poDevice->nRxCtrlReg1 |= ADI_SPORT_LRFS;
                }
                /* ELSE (DACLR Phase not Inverted) */
                    /* Active High TFS (Right channel DAC data when TFS is High) */
                    /* Active High RFS (Right channel DAC data when RFS is High) */

                /* IF (Bit Clock not Inverted) */
                if (!(poDevice->anBF52xC1Reg[BF52xC1_REG_DIGITAL_IFACE] & BF52xC1_RFLD_BCLK_INVERT))
                {
                    /* Drive Data & Internal Frame Syncs with falling edge of TSCLK. Sample External FS with rising edge of TSCLK */
                    poDevice->nTxCtrlReg1 |= ADI_SPORT_TCKFE;
                    /* Drive Data & Internal Frame Syncs with falling edge of RSCLK. Sample External FS with rising edge of RSCLK */
                    poDevice->nRxCtrlReg1 |= ADI_SPORT_RCKFE;
                }
                /* ELSE (Bit Clock is Inverted) */
                    /* Drive Data & Internal Frame Syncs with rising edge of TSCLK. Sample External FS with falling edge of TSCLK */
                    /* Drive Data & Internal Frame Syncs with rising edge of RSCLK. Sample External FS with falling edge of RSCLK */
            }

            /* extract Input Audio Data bit length */
            u16Temp = (poDevice->anBF52xC1Reg[BF52xC1_REG_DIGITAL_IFACE] & BF52xC1_RFLD_AUDIO_DATA_LEN);

            /* IF (Input Audio Data bit length is set to 16 bits) */
            if (u16Temp == BF52xC1_WORD_LENGTH_16BITS)
            {
                poDevice->nTxCtrlReg2 |= ADI_SPORT_SLEN_16;
                poDevice->nRxCtrlReg2 |= ADI_SPORT_SLEN_16;
            }
            /* ELSE IF (Input Audio Data bit length is set to 20 bits) */
            else if (u16Temp == BF52xC1_WORD_LENGTH_20BITS)
            {
                poDevice->nTxCtrlReg2 |= ADI_SPORT_SLEN_20;
                poDevice->nRxCtrlReg2 |= ADI_SPORT_SLEN_20;
            }
            /* ELSE IF (Input Audio Data bit length is set to 24 bits) */
            else if (u16Temp == BF52xC1_WORD_LENGTH_24BITS)
            {
                poDevice->nTxCtrlReg2 |= ADI_SPORT_SLEN_24;
                poDevice->nRxCtrlReg2 |= ADI_SPORT_SLEN_24;
            }
            /* ELSE (Input Audio Data bit length is set to 32 bits) */
            else
            {
                poDevice->nTxCtrlReg2 |= ADI_SPORT_SLEN_32;
                poDevice->nRxCtrlReg2 |= ADI_SPORT_SLEN_32;
            }

            /* IF (BF52xC1 set in slave mode) */
            if (!(poDevice->anBF52xC1Reg[BF52xC1_REG_DIGITAL_IFACE] & BF52xC1_RFLD_ENABLE_MASTER))
            {
                /* Enable SPORT Internal Clock Generation and Internal FS generation */
                poDevice->nTxCtrlReg1 |= (ADI_SPORT_ITCLK | ADI_SPORT_ITFS);
                poDevice->nRxCtrlReg1 |= (ADI_SPORT_IRCLK | ADI_SPORT_IRFS);
            }
        }
    }
    /* ELSE (invalid register, return error) */
    else
    {
        nResult = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
    }

    /* return */
    return (nResult);
}

/*********************************************************************

    Function: adi_bf52xc1_UpdateSampleRate

        Updates BF52xC1 Sampling rate register to match
        the given ADC,DAC Sample rates.

    Parameters:
        poDevice    - Pointer to BF52xC1 driver instance to work on
        pSampleRate - pointer to structure holding ADC/DAC Sample rates

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully updated BF52xC1 sampling rate
        ADI_BF52xC1_RESULT_SAMPLE_RATE_NOT_SUPPORTED
            - Given sampling rate is not supported
        Return codes from Device access

*********************************************************************/
static u32 adi_bf52xc1_UpdateSampleRate(
    ADI_BF52XC1_DRIVER_DEF      *poDevice,
    ADI_BF52xC1_SAMPLE_RATE     *pSampleRate
)
{
    u8  i,SRFieldValue;
    u16 RegData;
    u32 SamplingRate;
    /* default return code */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;
    /* structure to access one BF52xC1 register */
    ADI_DEV_ACCESS_REGISTER oAccessBF52xC1;

    /* Get the Sampling Control register value */
    RegData = poDevice->anBF52xC1Reg[BF52xC1_REG_SAMPLING_CTRL];
    /* Clear CLKDIV2 and SR fields */
    RegData &= ~(BF52xC1_RFLD_SAMPLE_RATE | BF52xC1_RFLD_CORECLK_DIV2);

    /*** Get the new SR field value from Sampling Rate Lookup Table ***/
    /* IF (ADC and DAC set to different sample rates) */
    if (pSampleRate->AdcSampleRate != pSampleRate->DacSampleRate)
    {
        /* This SR field value falls under Special case sampling rate */
        /* IF (ADC Sample Rate = 48kHz & DAC Sample Rate = 8kHz) */
        if ((pSampleRate->AdcSampleRate == 48000) &&\
            (pSampleRate->DacSampleRate == 8000))
        {
            /* SR Field value = 1 */
            SRFieldValue = 1;
        }
        /* ELSE IF (ADC Sample Rate = 8kHz & DAC Sample Rate = 48kHz) */
        else if ((pSampleRate->AdcSampleRate == 8000) &&\
                 (pSampleRate->DacSampleRate == 48000))
        {
            /* SR Field value = 2 */
            SRFieldValue = 2;
        }
        /* ELSE IF (ADC Sample Rate = 44.1kHz & DAC Sample Rate = 8.02kHz) */
        else if ((pSampleRate->AdcSampleRate == 44100) &&\
                 (pSampleRate->DacSampleRate == 8020))
        {
            /* SR Field value = 9 */
            SRFieldValue = 9;
        }
        /* ELSE IF (ADC Sample Rate = 8.02kHz & DAC Sample Rate = 44.1kHz) */
        else if ((pSampleRate->AdcSampleRate == 8020) &&\
                 (pSampleRate->DacSampleRate == 44100))
        {
            /* SR Field value = A */
            SRFieldValue = 0x0A;
        }
        /* ELSE (sampling rate combination not supported. return error */
        else
        {
            nResult = ADI_BF52xC1_RESULT_SAMPLE_RATE_NOT_SUPPORTED;
        }
    }
    /* ELSE (ADC and DAC sample rates are equal) */
    else
    {
        /* copy the given sampling rate to a temp location */
        SamplingRate = pSampleRate->AdcSampleRate;
        /* run thru SR mapping twice */
        i = 2;
        while (i--)
        {
            /* CASE OF (SamplingRate) */
            switch(SamplingRate)
            {
                /* CASE (96kHz) */
                case (96000):
                    /* SR Field value = 7 */
                    SRFieldValue = 7;
                    break;

                /* CASE (88.2kHz) */
                case (88200):
                    /* SR Field value = F */
                    SRFieldValue = 0x0F;
                    break;

                /* CASE (48kHz) */
                case (48000):
                    /* SR Field value = 0 */
                    SRFieldValue = 0;
                    break;

                /* CASE (44.1kHz) */
                case (44100):
                    /* SR Field value = 8 */
                    SRFieldValue = 8;
                    break;

                /* CASE (32kHz) */
                case (32000):
                    /* SR Field value = 6 */
                    SRFieldValue = 6;
                    break;

                /* CASE (8.02kHz) */
                case (8020):
                    /* SR Field value = 0x0B */
                    SRFieldValue = 0x0B;
                    break;

                /* CASE (8kHz) */
                case (8000):
                    /* SR Field value = 3 */
                    SRFieldValue = 3;
                    break;

                default:
                    /* IF (Temp sampling rate is same as client provided sampling rate) */
                    if (SamplingRate == pSampleRate->AdcSampleRate)
                    {
                        /* assuming that the given sampling rate can be supported by reducing core clock by half */
                        /* set Sampling Control register CLKDIV2 bit */
                        RegData |= (1 << ADI_BF52xC1_SHIFT_CLKDIV2);
                        /* dobule the temp sampling rate */
                        SamplingRate <<= 1;
                    }
                    else
                    {
                        /* Sampling rate not supported. return error */
                        nResult = ADI_BF52xC1_RESULT_SAMPLE_RATE_NOT_SUPPORTED;
                    }
                    break;

            } /* END CASE */

        } /* End of while */

    } /* End of if (ADC and DAC set to different sample rates) else case */

    /* IF (given sample rates are valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* update RegData with new SR field value */
        RegData |= (SRFieldValue << ADI_BF52xC1_SHIFT_SR);
        /* IF (USB mode enabled) */
        if (RegData & BF52xC1_RFLD_ENABLE_USB_MODE)
        {
            /* IF (set BOSR bit when Sampling rate is set to 88.2/44.1/22.05/8.02kHz) */
            if ((pSampleRate->AdcSampleRate == 88200) ||\
                (pSampleRate->AdcSampleRate == 44100) ||\
                (pSampleRate->AdcSampleRate == 22050) ||\
                (pSampleRate->AdcSampleRate == 8020))
            {
                RegData |= BF52xC1_RFLD_BOS_RATE;
            }
            /* ELSE (clear BOSR for other sampling rates) */
            else
            {
                RegData &= ~BF52xC1_RFLD_BOS_RATE;
            }
        }

        /*** Packet the Sample Rate register address and data as per BF52xC1 register configuration requirements ***/
        /* Shift the register address (Bit 15 to Bit 9 in BF52xC1 write packet holds the register address to configure) */
        oAccessBF52xC1.Address = (BF52xC1_REG_SAMPLING_CTRL << ADI_BF52xC1_REG_ADDR_SHIFT);
        /* Bit 8 to Bit 0 in BF52xC1 write packet holds the register data */
        oAccessBF52xC1.Address |= RegData;
        /* update BF52xC1 Sampling Control register */
        nResult = adi_bf52xc1_DeviceAccess(poDevice, &oAccessBF52xC1);
    }

    /* return */
    return (nResult);
}

/*********************************************************************

    Function: adi_bf52xc1_SportOpen

        Opens SPORT device allocated to BF52xC1 Audio Codec

    Parameters:
        poDataPort - Pointer to BF52xC1 SPORT Data port to open

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully opened SPORT Device
        Return codes from SPORT Driver

*********************************************************************/
static u32 adi_bf52xc1_SportOpen(
    ADI_BF52XC1_DRIVER_DATA_PORT_INFO  *poDataPort
)
{
    /* default return code */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;

    /* Open SPORT for inbound dataflow */
    nResult = adi_dev_Open(poDataPort->hDeviceManager,
                           &ADISPORTEntryPoint,
                           poDataPort->nSportDevNumber,
                           poDataPort,
                           &poDataPort->hSport,
                           poDataPort->eDirection,
                           poDataPort->hDmaManager,
                           NULL,
                           adi_bf52xc1_SportCallback);

    /* IF (Successfully opened SPORT device) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* IF (SPORT is opened in bi-directional mode) */
        if (poDataPort->eDirection == ADI_DEV_DIRECTION_BIDIRECTIONAL)
        {
            /* Copy the device handle information to receive port */
            gaoBF52xC1Device[poDataPort->nAudioDevNumber].oAudioRx.hSport = poDataPort->hSport;
        }
    }

    return (nResult);
}

/*********************************************************************

    Function: adi_bf52xc1_ConfigSport

        Configures SPORT device registers as per BF52xC1 operating conditions

    Parameters:
        poDevice    - Pointer to BF52xC1 device instace to work on
        poDataPort  - Pointer to BF52xC1 SPORT Data port to configure

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully configured SPORT Device registers
        Return codes from SPORT Driver

*********************************************************************/
static u32 adi_bf52xc1_ConfigSport(
    ADI_BF52XC1_DRIVER_DEF              *poDevice,
    ADI_BF52XC1_DRIVER_DATA_PORT_INFO   *poDataPort
)
{
    /* Audio data interface format */
    u16     eInterfaceFormat;
    /* default return code */
    u32     nResult = ADI_DEV_RESULT_SUCCESS;

    /* Extract BF52xC1 audio data interface format */
    eInterfaceFormat = (poDevice->anBF52xC1Reg[BF52xC1_REG_DIGITAL_IFACE] & BF52xC1_RFLD_IFACE_FORMAT);

    /* IF (Auto-SPORT Configuration is enabled) */
    if (poDataPort->bIsAutoSportConfigEnabled == true)
    {
        /* Auto-SPORT Configuration does not support Right-Justified or DSP mode */
        /* DSP & Right-Justified modes - SPORT configuration registers left unchanged */

        /* IF (BF52xC1 is set in Left Justified or I2S Mode) */
        if ((eInterfaceFormat == BF52xC1_STEREO_MODE_LJ) ||\
            (eInterfaceFormat == BF52xC1_STEREO_MODE_I2S))
        {
            /* IF (SPORT is opened for outbound or bi-directional dataflow) */
            if ((poDataPort->eDirection == ADI_DEV_DIRECTION_OUTBOUND) ||\
                (poDataPort->eDirection == ADI_DEV_DIRECTION_BIDIRECTIONAL))
            {
                /* Configure SPORT Transmit control register 1 */
                nResult = adi_dev_Control(poDevice->oAudioTx.hSport,
                                          ADI_SPORT_CMD_SET_TCR1,
                                          (void *)poDevice->nTxCtrlReg1);

                /* IF (Successfully configured TCR 1) */
                if (nResult == ADI_DEV_RESULT_SUCCESS)
                {
                    /* Configure SPORT Transmit control register 2 */
                    nResult = adi_dev_Control(poDevice->oAudioTx.hSport,
                                              ADI_SPORT_CMD_SET_TCR2,
                                             (void *)poDevice->nTxCtrlReg2);
                }

            } /* End of if (SPORT is opened for outbound or bi-directional dataflow) */

            /* IF (Successfully configured SPORT Tx Registers) */
            if (nResult == ADI_DEV_RESULT_SUCCESS)
            {
                /* IF (SPORT is opened for Inbound or bi-directional dataflow) */
                if ((poDataPort->eDirection == ADI_DEV_DIRECTION_INBOUND) ||\
                    (poDataPort->eDirection == ADI_DEV_DIRECTION_BIDIRECTIONAL))
                {
                    /* Configure SPORT Receive control register 1 */
                    nResult = adi_dev_Control(poDevice->oAudioRx.hSport,
                                              ADI_SPORT_CMD_SET_RCR1,
                                              (void *)poDevice->nRxCtrlReg1);

                    /* IF (Successfully configured RCR 1) */
                    if (nResult == ADI_DEV_RESULT_SUCCESS)
                    {
                        /* Configure SPORT Receive control register 2 */
                        nResult = adi_dev_Control(poDevice->oAudioRx.hSport,
                                                  ADI_SPORT_CMD_SET_RCR2,
                                                  (void *)poDevice->nRxCtrlReg2);
                    }

                } /* End of if (SPORT is opened for Inbound or bi-directional dataflow) */

            } /* End of if (Successfully configured SPORT Tx Registers) */

        } /* End of if (BF52xC1 is set in Left Justified or I2S Mode) */

    } /* End of if (Auto-SPORT Configuration is enabled) */

    return (nResult);
}

/*********************************************************************

    Function: adi_bf52xc1_SportCallback

        Callback from the SPORT Driver

    Parameters:
        hCallback   - Callback Handle passed by BF52xC1
        nEvent      - Callback Event
        pArgument   - Callback Argument

    Returns:
        None

*********************************************************************/
static void adi_bf52xc1_SportCallback(
    void    *hCallback,
    u32     nEvent,
    void    *pArgument
)
{
    /* Pointer to BF52xC1 device SPORT instance to which the callback belongs */
    ADI_BF52XC1_DRIVER_DATA_PORT_INFO  *poDataPort = (ADI_BF52XC1_DRIVER_DATA_PORT_INFO *)hCallback;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    /* IF (Given Physical Device Driver Handle is valid) */
    if (ValidatePDDHandle(hCallback) == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* Pass the callback to the Device Manager Callback */
        (gaoBF52xC1Device[poDataPort->nAudioDevNumber].pfCallback) (poDataPort->hDevice, nEvent, pArgument);

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

}

#if defined(ADI_DEV_DEBUG)  /* Debug build only */
/*********************************************************************

    Function: ValidatePDDHandle

        Validates the Physical Device Driver Handle

    Parameters:
        hPhysicalDevice - Physical Device Driver Handle to be validated

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Given Physical Device Driver Handle is valid
        ADI_DEV_RESULT_BAD_PDD_HANDLE
            - Handle is invalid

*********************************************************************/
static u32 ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE      hPhysicalDevice
)
{
    /* Return value - insure the client has provided a valid PDD Handle */
    u32     nResult = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    /* index */
    u32     i;

    /* Compare the given Physical Device Driver handle with Handle allocated to
       all BF52xC1 devices in the list */
    for (i = 0; i < ADI_BF52xC1_NUM_DEVICES; i++)
    {
        if ((hPhysicalDevice == (ADI_DEV_PDD_HANDLE)&gaoBF52xC1Device[i].oAudioTx) ||\
            (hPhysicalDevice == (ADI_DEV_PDD_HANDLE)&gaoBF52xC1Device[i].oAudioRx))
        {
            /* Given Physical Device Driver Handle is valid. quit this loop */
            nResult = ADI_DEV_RESULT_SUCCESS;
            break;
        }
    }

    return (nResult);
}

#endif /* ADI_DEV_DEBUG */

/*****/

/*
**
** EOF:
**
*/

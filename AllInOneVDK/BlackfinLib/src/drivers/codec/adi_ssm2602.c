/* $Revision: 3492 $
 * $Date: 2010-10-07 09:34:20 -0400 (Thu, 07 Oct 2010) $ 
******************************************************************************
Copyright (c), 2007-2009 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title: SSM2602 Audio CODEC driver

Description: This is the primary source file for SSM2602 Audio CODEC driver

*****************************************************************************/

/*=============  I N C L U D E S   =============*/

/* system service includes */
#include <services/services.h>
/* device manager includes */
#include <drivers/adi_dev.h>
/* SPORT driver includes */
#include <drivers/sport/adi_sport.h>
/* Device Access includes */
#include <drivers/deviceaccess/adi_device_access.h>
/* SSM2602 driver includes */
#include <drivers/codec/adi_ssm2602.h>

/*=============  D E F I N E S  =============*/

/*
**
**  Enumerations
**
*/

/* Number of SSM2602 devices in the system  */
#define ADI_SSM2602_NUM_DEVICES         (sizeof(gaoSSM2602Device)/sizeof(ADI_SSM2602_DRIVER_DEF))

/* TWI Global Address for SSM2602 when CSB pin is 0  */
#define ADI_SSM2602_CSB_LOW_TWI_ADDR    0x1AU
/* TWI Global Address for SSM2602 when CSB pin is 1  */
#define ADI_SSM2602_CSB_HIGH_TWI_ADDR   0x1BU

/*
** Macros to manipulate SSM2602 hardware register access
*/

/* Number of SSM2602 registers to cache */
#define ADI_SSM2602_NUM_REGS_TO_CACHE   13U
/* Address of the last register in SSM2602 */
#define ADI_SSM2602_FINAL_REG_ADDRESS   0x24FFU
/* SSM2602 Register Address mask */
#define ADI_SSM2602_REG_ADDR_MASK       0xFE00U
/* SSM2602 Register Data mask */
#define ADI_SSM2602_REG_DATA_MASK       0x01FFU
/* Shift Count to reach SSM2602 Register Address */
#define ADI_SSM2602_REG_ADDR_SHIFT      9U
/* Shift count to reach Sampling Rate field in Sampling control register */
#define ADI_SSM2602_SHIFT_SR            2U

/*
** Macros to manipulate SPORT Register fields
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
** SPORT word length (actual wordlength - 1)
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
** Stereo modes supported by SSM2602 hardware
**  SSM2602 driver Auto-SPORT configuration does not support
**  DSP and Right-Justified modes
*/
enum
{
    SSM2602_STEREO_MODE_RJ = 0,
    SSM2602_STEREO_MODE_LJ,
    SSM2602_STEREO_MODE_I2S,
    SSM2602_STEREO_MODE_DSP
};

/*
** List of data word lengths supported by SSM2602
*/
enum
{
    SSM2602_WORD_LENGTH_16BITS = 0x00,
    SSM2602_WORD_LENGTH_20BITS = 0x04,
    SSM2602_WORD_LENGTH_24BITS = 0x08,
    SSM2602_WORD_LENGTH_32BITS = 0x0C
};

/*
**
** Data Structures
**
*/

/*
** Structure to hold SSM2602 Audio Data Interface Port specific information
*/
typedef struct __AdiSSM2602DriverDataPortInfo
{

    /* true when dataflow is active, false otherwise */
    bool                            bIsActive;

    /* 'true' when automatic SPORT configuration enabled,
        'false' when disabled */
    bool                            bIsAutoSportConfigEnabled;

    /* SSM2602 device instance number to with this port belongs */
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

} ADI_SSM2602_DRIVER_DATA_PORT_INFO;

/*
** SSM2602 Driver instance structure
*/
typedef struct __AdiSSM2602DriverDef
{
    /* 'true' to use TWI to access device registers, 'false' to use SPI */
    bool                                bUseTwiForRegAccess;

    /* TWI/SPI device number to use to access SSM2602 registers */
    u8                                  nTwiSpiDevNumber;

    /* TWI global address / SPI chipselect for SSM2602 register access */
    u8                                  nTwiGAddrSpiCs;

    /* SPORT Transmit control register 1 value for Auto SPORT config */
    u16                                 nTxCtrlReg1;

    /* SPORT Transmit control register 2 value for Auto SPORT config */
    u16                                 nTxCtrlReg2;

    /* SPORT Receive control register 1 value for Auto SPORT config */
    u16                                 nRxCtrlReg1;

    /* SPORT Receive control register 2 value for Auto SPORT config */
    u16                                 nRxCtrlReg2;

    /* SSM2602 hardware registers cache */
    u16                                 anSSM2602Reg[ADI_SSM2602_NUM_REGS_TO_CACHE];

    /* SPI/TWI Configuration Table passed by the client */
    ADI_DEV_CMD_VALUE_PAIR              *pSpiTwiConfigTable;

    /* Callback function supplied by the Device Manager */
    ADI_DCB_CALLBACK_FN                 pfCallback;

    /* Instance to manage audio transmit or bi-directional data */
    ADI_SSM2602_DRIVER_DATA_PORT_INFO   oAudioTx;

    /* Instance to manage audio receive data */
    ADI_SSM2602_DRIVER_DATA_PORT_INFO   oAudioRx;

} ADI_SSM2602_DRIVER_DEF;

/*=============  D A T A  =============*/

/* Create SSM2602 device instance(s) */
static ADI_SSM2602_DRIVER_DEF   gaoSSM2602Device[] =
{
    /* SSM2602 Device 0 */
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
        /* Clear SSM2602 register cache */
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
            /* SSM2602 instance number */
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
            /* SSM2602 instance number */
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

/* Opens SSM2602 for specified data direction */
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

/* Closes a SSM2602 device */
static u32 adi_pdd_Close(
    ADI_DEV_PDD_HANDLE                  hPhysicalDevice
);

/* Queues Inbound buffers to SSM2602 SPORT */
static u32 adi_pdd_Read(
    ADI_DEV_PDD_HANDLE                  hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE                 eBufferType,
    ADI_DEV_BUFFER                      *pBuffer
);

/* Queues Outbound buffers to SSM2602 SPORT */
static u32 adi_pdd_Write(
    ADI_DEV_PDD_HANDLE                  hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE                 eBufferType,
    ADI_DEV_BUFFER                      *pBuffer
);

/* Sequentially read/writes data to a device
   Function not supported by SSM2602 */
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

/* Function to Access SSM2602 registers */
static u32 adi_ssm2602_RegAccess(
    ADI_SSM2602_DRIVER_DEF              *poDevice,
    u32                                 nCommand,
    void                                *Value
);

/* Writes to a SSM2602 hardware register using Device Access Service */
static u32 adi_ssm2602_DeviceAccess(
    ADI_SSM2602_DRIVER_DEF              *poDevice,
    ADI_DEV_ACCESS_REGISTER             *oAccessSSM2602
);

/* Function to update SSM2602 register cache */
static u32 adi_ssm2602_UpdateDriverCache(
    ADI_SSM2602_DRIVER_DEF              *poDevice,
    ADI_DEV_ACCESS_REGISTER             *oAccessSSM2602
);

/* Updates SSM2602 Sample Control register to match ADC/DAC sampling rate */
static u32 adi_ssm2602_UpdateSampleRate(
    ADI_SSM2602_DRIVER_DEF              *poDevice,
    ADI_SSM2602_SAMPLE_RATE             *poSampleRate
);

/* Function to open a SPORT device */
static u32 adi_ssm2602_SportOpen(
    ADI_SSM2602_DRIVER_DATA_PORT_INFO   *poDataPort
);

/* Function to Configure SPORT registers */
static u32 adi_ssm2602_ConfigSport(
    ADI_SSM2602_DRIVER_DEF              *poDevice,
    ADI_SSM2602_DRIVER_DATA_PORT_INFO   *poDataPort
);

/* Callback function passed to SPORT driver */
static void adi_ssm2602_SportCallback(
    void                                *hCallback,
    u32                                 nEvent,
    void                                *pArgument
);

/* Adjusts SSM2602 Sample Control register value to
   match MCLK frequency */
static u32 adi_ssm2602_AdjustToMclk(
    ADI_SSM2602_DEF                     *poDevice,
    u8                                  nMclkFreq
);

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)

/* Function to validate register access table   */
static u32 adi_ssm2602_ValidateRegAccess(
    ADI_SSM2602_DEF                     *poDevice,
    u32                                 nCommandID,
    void                                *Value
);

/* Validates Physical Device Driver Handle  */
static u32 adi_ssm2602_ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE                  hPhysicalDevice
);

#endif

/*
** Entry point to the SSM2602 device driver
*/

ADI_DEV_PDD_ENTRY_POINT     ADISSM2602EntryPoint =
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

        Opens SSM2602 Audio Device for use

    Parameters:
        hDeviceManager     - Device Manager handle
        nDeviceNumber      - SSM2602 device number to open
        hDevice            - Device Handle allocated by the Device Manager
        phPhysicalDevice   - Pointer to location to hold Physical Device Handle
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
     /* flag to indicate if device instance is to be reset to default or not */
    bool                                bResetInstance;
    /* Return value - assume we're going to be successful */
    u32                                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the device instance we will be working on */
    ADI_SSM2602_DRIVER_DEF              *poDevice;
    /* Pointer to the device data port we will be working on */
    ADI_SSM2602_DRIVER_DATA_PORT_INFO   *poDataPort;
    /* Exit critical region parameter */
    void                                *pExitCriticalArg;
    /* Instance to access a SSM2602 register */
    ADI_DEV_ACCESS_REGISTER             oAccessSSM2602;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* IF (Device number exceeds the number of devices supported) */
    if (nDeviceNumber >= ADI_SSM2602_NUM_DEVICES)
    {
        /* Report error (Invalid Device number) */
        nResult = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    }

    /* IF (the device number is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
#endif
        /* assume that the device client requesting for is
           already in use  */
        nResult = ADI_DEV_RESULT_DEVICE_IN_USE;
        /* SSM2602 device to work on */
        poDevice = &gaoSSM2602Device[nDeviceNumber];
        /* assume that the device instance has to be reset*/
        bResetInstance = true;

        /* Protect this section of code - entering a critical region */
        pExitCriticalArg = adi_int_EnterCriticalRegion(pCriticalRegionArg);

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

        /* Exit the critical region */
        adi_int_ExitCriticalRegion(pExitCriticalArg);

        /* IF (the device requested by the client is available for use) */
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
                poDevice->eDriverStatus       = false;
                /* By default, use SPI 0 to access SSM2602 registers */
                poDevice->nTwiSpiDevNumber          = 0;
                /* By default, no valid SPI chipselect is available to select SSM2602 */
                poDevice->nTwiGAddrSpiCs            = 0;
                /* No SPI or TWI configuration is available */
                poDevice->pSpiTwiConfigTable        = NULL;

                /* Auto-SPORT Configuration is enabled by default */
                poDevice->oAudioTx.bIsAutoSportConfigEnabled = true;
                poDevice->oAudioRx.bIsAutoSportConfigEnabled = true;

                /* Initialise SSM2602 register cache & SPORT register cache to default values */
                /* issue a dummy SSM2602 soft-reset within the driver */
                /* Shift the register address (Bit 15 to Bit 9 in SSM2602 write packet holds the register address to configure) */
                oAccessSSM2602.Address = ((SSM2602_REG_RESET << ADI_SSM2602_REG_ADDR_SHIFT) & ADI_SSM2602_REG_ADDR_MASK);
                /* Bit 8 to Bit 0 in SSM2602 write packet holds the register data (zero to reset SSM2602) */
                /* call update driver cache to reset SSM2602 register cache & SPORT register cache */
                adi_ssm2602_UpdateDriverCache(poDevice,&oAccessSSM2602);
            }

            /* Save physical device handle in the client supplied location */
            *phPhysicalDevice = (ADI_DEV_PDD_HANDLE *)poDataPort;
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(nResult);
}

/*********************************************************************

    Function: adi_pdd_Close

        Closes SSM2602 device

    Parameters:
        hPhysicalDevice - Physical Device handle to SSM2602 device instance

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully closed device
        Error code returned from PPI and SPI close functions

*********************************************************************/
static u32 adi_pdd_Close(
    ADI_DEV_PDD_HANDLE      hPhysicalDevice
)
{
    /* Return value - assume we're going to be successful */
    u32                                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the audio data port we're working on */
    ADI_SSM2602_DRIVER_DATA_PORT_INFO   *poDataPort = (ADI_SSM2602_DRIVER_DATA_PORT_INFO *)hPhysicalDevice;

/* for Debug build only - check for errors if required */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given Physical Device Handle */
    nResult = adi_ssm2602_ValidatePDDHandle(hPhysicalDevice);

    /* IF (given Physical Device Handle is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
#endif
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
                gaoSSM2602Device[poDataPort->nAudioDevNumber].oAudioTx.eDirection = ADI_DEV_DIRECTION_UNDEFINED;
                gaoSSM2602Device[poDataPort->nAudioDevNumber].oAudioRx.eDirection = ADI_DEV_DIRECTION_UNDEFINED;
                /* Clear the SPORT device handle */
                gaoSSM2602Device[poDataPort->nAudioDevNumber].oAudioTx.hSport = NULL;
                gaoSSM2602Device[poDataPort->nAudioDevNumber].oAudioRx.hSport = NULL;
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

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(nResult);
}

/*********************************************************************

    Function: adi_pdd_Read

        Queues Inbound buffers to SSM2602 SPORT

    Parameters:
        hPhysicalDevice - Physical Device handle to SSM2602 device instance
        eBufferType     - Buffer type to queue
        poTxBuffer      - Pointer to receive buffer

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully queued buffer to SPORT
        Error code returned by SPORT driver

*********************************************************************/
static u32  adi_pdd_Read(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poRxBuffer
)
{

    /* Return value - assume we're going to be successful */
    u32                                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the audio data port we're working on */
    ADI_SSM2602_DRIVER_DATA_PORT_INFO   *poDataPort = (ADI_SSM2602_DRIVER_DATA_PORT_INFO *)hPhysicalDevice;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given Physical Device Handle */
    nResult = adi_ssm2602_ValidatePDDHandle(hPhysicalDevice);

    /* IF (given Physical Device Handle is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* Submit the Inbound/Receive buffer to SPORT receive channel */
        nResult = adi_dev_Read(poDataPort->hSport, eBufferType, pBuffer);

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(nResult);
}
/*********************************************************************

    Function: adi_pdd_Write

        Queues Outbound buffers to SSM2602 SPORT

    Parameters:
        hPhysicalDevice - Physical Device handle to SSM2602 device instance
        eBufferType     - Buffer type to queue
        poTxBuffer      - Pointer to transmit buffer

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully queued buffer to SPORT
        Error code returned by SPORT driver

*********************************************************************/
static u32  adi_pdd_Write(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poTxBuffer
)
{

    /* Return value - assume we're going to be successful */
    u32                                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the audio data port we're working on */
    ADI_SSM2602_DRIVER_DATA_PORT_INFO   *poDataPort = (ADI_SSM2602_DRIVER_DATA_PORT_INFO *)hPhysicalDevice;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given Physical Device Handle */
    nResult = adi_ssm2602_ValidatePDDHandle(hPhysicalDevice);

    /* IF (given Physical Device Handle is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* Submit the Outbound/Transmit buffer to SPORT transmit channel */
        nResult = adi_dev_Write(poDataPort->hSport, eBufferType, pBuffer);

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(nResult);
}

/*********************************************************************

    Function: adi_pdd_SequentialIO

    Sequentially read/writes data to a device
        SSM2602 driver does not support this function

    Parameters:
        hPhysicalDevice - Physical Device Handle of the device to work on
        eBufferType     - Buffer type
        pBuffer         - Pointer to data buffer

    Returns:
        ADI_DEV_RESULT_NOT_SUPPORTED
            - Function not supported by SSM2602 driver

*********************************************************************/
static u32  adi_pdd_SequentialIO(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poSeqBuffer
)
{
    /* Function is not supported by SSM2602 driver */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************

    Function: adi_pdd_Control

        Sets or senses device specific parameter

    Parameters:
        hPhysicalDevice    - Physical Device handle to
                             SSM2602 device instance
        nCommandID         - Command ID
        Value      - Command specific value

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully processed command
        SPORT/SPI/TWI/SSM2602 Driver specific Error codes

*********************************************************************/
static u32  adi_pdd_Control(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    u32                         nCommandID,
    void                        *Value
)
{
    /* Return value - assume we're going to be successful */
    u32                                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the audio data port to work on */
    ADI_SSM2602_DRIVER_DATA_PORT_INFO   *poDataPort = (ADI_SSM2602_DRIVER_DATA_PORT_INFO *)hPhysicalDevice;
    /* Instance to access a SSM2602 register */
    ADI_DEV_ACCESS_REGISTER             oAccessSSM2602;
    /* Pointer to the device instance to work on */
    ADI_SSM2602_DRIVER_DEF              *poDevice;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given Physical Device Handle */
    nResult = adi_ssm2602_ValidatePDDHandle(hPhysicalDevice);

    /* IF (given Physical Device Handle is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
#endif
        /* Get the address of device instance to work on */
        poDevice = &gaoSSM2602Device[poDataPort->nAudioDevNumber];

        /* CASEOF (Command ID)  */
        switch (nCommandID)
        {
            /* CASE: Control dataflow */
            case ADI_DEV_CMD_SET_DATAFLOW:

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

                            /* Register access to update SSM2602 Active Control register */
                            oAccessSSM2602.Address = (SSM2602_REG_ACTIVE_CTRL << ADI_SSM2602_REG_ADDR_SHIFT);
                            /* Update field to activate SSM2602 Digital Audio Interface */
                            oAccessSSM2602.Address |= 1;
                            /* Update SSM2602 Active Control register */
                            nResult = adi_ssm2602_DeviceAccess(poDevice, &oAccessSSM2602);
                        }

                        /* IF (Successfully updated Active control register) */
                        if (nResult == ADI_DEV_RESULT_SUCCESS)
                        {
                            /* Configure SPORT Registers */
                            nResult = adi_ssm2602_ConfigSport (poDevice, poDataPort);
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

                            /* Register access to update SSM2602 Active Control register */
                            oAccessSSM2602.Address = (SSM2602_REG_ACTIVE_CTRL << ADI_SSM2602_REG_ADDR_SHIFT);
                            /* Update SSM2602 Active Control register */
                            nResult = adi_ssm2602_DeviceAccess(poDevice, &oAccessSSM2602);
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

            /* CASE: Query for processor DMA support */
            case ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT:

                /* SSM2602 doesn't support DMA, but supports indirectly via SPORT */
                *((u32 *)Value) = false;
                break;

            /* CASE: Set Dataflow method */
            case ADI_DEV_CMD_SET_DATAFLOW_METHOD:

                /* IF (SPORT device connected to this port is not open yet) */
                if (poDataPort->hSport == NULL)
                {
                    /* Try to open the SPORT device */
                    nResult = adi_SSM2602_SportOpen(poDataPort);
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

            /* CASE: Set SSM2602 driver to use SPI to access SSM2602 hardware registers */
            case ADI_SSM2602_CMD_SET_SPI_DEVICE_NUMBER:
                /* Update SPI device number */
                poDevice->nTwiSpiDevNumber      = (u8)((u32)Value);
                /* Update flag to use SPI for register access */
                poDevice->bUseTwiForRegAccess   = false;
                break;

            /* CASE: Set SPI Chipselect for SSM2602 */
            case ADI_SSM2602_CMD_SET_SPI_CS:
                /* Store the chipselect value */
                poDevice->nTwiGAddrSpiCs = (u8)((u32)Value);
                break;

            /************************
            TWI related commands
            ************************/

            /* CASE: Set SSM2602 driver to use TWI to access SSM2602 hardware registers */
            case ADI_SSM2602_CMD_SET_TWI_DEVICE_NUMBER:

                /* Update TWI device number */
                poDevice->nTwiSpiDevNumber      = (u8)((u32)Value);
                /* Update flag to use TWI for register access */
                poDevice->bUseTwiForRegAccess   = true;

                /* IF (TWI Global address is not valid) */
                if ((poDevice->nTwiGAddrSpiCs != ADI_SSM2602_CSB_LOW_TWI_ADDR) &&\
                    (poDevice->nTwiGAddrSpiCs != ADI_SSM2602_CSB_HIGH_TWI_ADDR))
                {
                    /* Set default TWI Global Address for SSM2602 */
                    poDevice->nTwiGAddrSpiCs = ADI_SSM2602_CSB_LOW_TWI_ADDR;
                }
                break;

            /* CASE: Set TWI Global Address for SSM2602 */
            case ADI_SSM2602_CMD_SET_TWI_GLOBAL_ADDR:

/* for Debug build only - validate TWI Global Address */
#if defined(ADI_DEV_DEBUG)

                /* IF (TWI Global address is invalid) */
                if (((u8)((u32)Value) != ADI_SSM2602_CSB_LOW_TWI_ADDR) &&\
                    ((u8)((u32)Value) != ADI_SSM2602_CSB_HIGH_TWI_ADDR))
                {
                    /* Report Failure (TWI Global address is invalid) */
                    nResult = ADI_SSM2602_RESULT_TWI_GLOBAL_ADDRESS_INVALID;
                    break;
                }

#endif  /* ADI_DEV_DEBUG */

                /* Update TWI Global address for SSM2602 */
                poDevice->nTwiGAddrSpiCs = (u8)((u32)Value);
                break;

            /****************************
            TWI and SPI related commands
            ****************************/

            /* CASE: Set TWI or SPI configuration table to be used to
                     access SSM2602 device registers */
            case ADI_SSM2602_CMD_SET_TWI_SPI_CONFIG_TABLE:

                /* store the configuration table address */
                poDevice->paSpiTwiConfigTable = (ADI_DEV_CMD_VALUE_PAIR*) Value;
                break;

            /************************
            SPORT related commands
            ************************/

            /* CASE: Set SPORT device number connected to
                     SSM2602 Digital Audio Interface port */
            case ADI_SSM2602_CMD_SET_SPORT_DEVICE_NUMBER:

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

            /* CASE: Open/Close SPORT device connected to SSM2602 */
            case ADI_SSM2602_CMD_OPEN_SPORT_DEVICE:

                /* IF (Open SPORT device) */
                if ((u32)Value == true)
                {
                    /* IF (SPORT device allocated to SSM2602 is not open yet) */
                    if (poDataPort->hSport == NULL)
                    {
                        /* Open the SPORT device for SSM2602 use */
                        nResult = adi_SSM2602_SportOpen(poDataPort);
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

            /* CASE: Enable/Disable Auto-SPORT configuration mode */
            case ADI_SSM2602_CMD_ENABLE_AUTO_SPORT_CONFIG:
                /* Update Automatic SPORT configuration flag */
                poDataPort->bIsAutoSportConfigEnabled = (u8)((u32)Value);
                break;

            /* CASE: Set SSM2602 Sampling Control register to
                     given ADC and DAC sample rate */
            case ADI_SSM2602_CMD_SET_SAMPLE_RATE:
                /* Update SSM2602 sampling control register */
                nResult = adi_SSM2602_UpdateSampleRate(poDevice, (ADI_SSM2602_SAMPLE_RATE*)Value);
                break;

            /* CASE: Pass MCLK (operating) frequency of SSM2602 */
            case ADI_SSM2602_CMD_PASS_MCLK_FREQ:
                /* Update SSM2602 sampling control register fields */
                nResult = adi_ssm2602_AdjustToMclk(poDevice,(u32)Value);
                break;

            /* DEFAULT: other commands */
            default:
                /* pass this Command ID to SSM2602 Register Access Function */
                nResult = adi_ssm2602_RegAccess(poDevice,nCommandID,Value);

                /* IF (Command not supported by SSM2602) */
                if (nResult == ADI_SSM2602_RESULT_CMD_NOT_SUPPORTED)
                {
                    /* Pass this command to SPORT driver */
                    nResult = adi_dev_Control (poDataPort->hSport,nCommand, Value);
                }
                break;

        } /* End of switch (nCommandID) cases */

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return(nResult);
}


/*********************************************************************

    Function: adi_ssm2602_RegAccess

        Configures SSM2602 hardware registers /
        Reads SSM2602 register values from driver cache

    Parameters:
        poDevice    - pointer to the driver instance to work on
        nCommandID  - Command ID passed by the application
        Value       - Command specific value

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully accessed SSM2602 register(s)
        SPI/TWI/Device Access/SSM2602 Driver specific Error codes

*********************************************************************/
static u32 adi_ssm2602_RegAccess(
    ADI_SSM2602_DRIVER_DEF  *poDevice,
    u32                     nCommand,
    void                    *Value
)
{
    /* Return code - assume we're going to be successfull */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;

    /* pointer to selective register access table */
    ADI_DEV_ACCESS_REGISTER         *poSelectiveAccess;
    /* pointer to block register access table */
    ADI_DEV_ACCESS_REGISTER_BLOCK   *poBlockAccess;
    /* pointer to register field access table */
    ADI_DEV_ACCESS_REGISTER_FIELD   *poFieldAccess;

    /* register access table to be passed to the Device Access Service */
    ADI_DEV_ACCESS_REGISTER         oAccessSSM2602;

    /* Variables for register access/register manipulation */
    u16 nRegAddr,nRegData,nFieldData,nIndex,nCacheIndex;

/* for Debug build only - Check for errors */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given register access table */
    nResult = adi_ssm2602_ValidateRegAccess(poDevice,nCommandID,Value);

    /* IF (the given register access table is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
#endif  /* ADI_DEV_DEBUG */

        /* CASEOF (Device Access Command) */
        switch (nCommandID)
        {
            /* CASE (Read a specific SSM2602 Register) */
            case ADI_DEV_CMD_REGISTER_READ:
            /* CASE (Read a table of SSM2602 registers) */
            case ADI_DEV_CMD_REGISTER_TABLE_READ:

                /* get the address of selective register access table submitted by the app */
                poSelectiveAccess = (ADI_DEV_ACCESS_REGISTER *) Value;

                /* access all registers in application provided access table until we reach the delimiter */
                while (poSelectiveAccess->Address != ADI_DEV_REGEND)
                {
                    /* IF (read ALC or Noise Gate register) */
                    if (poSelectiveAccess->Address > SSM2602_REG_RESET)
                    {
                        /* Index to the register in driver cache */
                        nCacheIndex = (poSelectiveAccess->Address - 6);
                    }
                    else
                    {
                        /* Index to the register in driver cache */
                        nCacheIndex = poSelectiveAccess->Address;
                    }

                    /* copy the register value from driver register cache */
                    poSelectiveAccess->Data = poDevice->aSSM2602Reg[nCacheIndex];

                    /* IF (the Command is to access a single register) */
                    if (nCommandID == ADI_DEV_CMD_REGISTER_READ)
                    {
                        /* done with register access. exit this loop */
                        break;
                    }
                    /* move to next selective register access info */
                    poSelectiveAccess++;
                }
                break;

            /* CASE (Configure a SSM2602 Resigter) */
            case ADI_DEV_CMD_REGISTER_WRITE:
            /* CASE (Configure a table SSM2602 registers) */
            case ADI_DEV_CMD_REGISTER_TABLE_WRITE:

                /* get the address of selective register access table submitted by the app */
                poSelectiveAccess = (ADI_DEV_ACCESS_REGISTER *) Value;

                /* access all registers in application provided access table until we reach the delimiter */
                while (poSelectiveAccess->Address != ADI_DEV_REGEND)
                {
                    /*************************************************
                    ** Packet this register address and data as per
                    ** SSM2602 register configuration requirements
                    *************************************************/
                    /* Shift the register address
                      (Bit 15 to Bit 9 in SSM2602 write packet holds the
                       register address to configure) */
                    oAccessSSM2602.Address = ((poSelectiveAccess->Address << ADI_SSM2602_REG_ADDR_SHIFT) & ADI_SSM2602_REG_ADDR_MASK);
                    /* Bit 8 to Bit 0 in SSM2602 write packet holds the register data */
                    oAccessSSM2602.Address |= (poSelectiveAccess->Data & ADI_SSM2602_REG_DATA_MASK);

                    /* Pass the new register data to device access service which inturn configures SSM2602 */
                    nResult = adi_ssm2602_DeviceAccess(poDevice,&oAccessSSM2602);

                    /* IF (Failed to access the device register(s)) */
                    if (nResult != ADI_DEV_RESULT_SUCCESS)
                    {
                        /* report error */
                        break;
                    }

                    /* IF (the Command is to access a single register) */
                    if (nCommandID == ADI_DEV_CMD_REGISTER_WRITE)
                    {
                        /* done with register access. exit this loop */
                        break;
                    }
                    /* move to next selective register access info */
                    poSelectiveAccess++;
                }
                break;

            /* CASE (Read a specific SSM2602 register field) */
            case ADI_DEV_CMD_REGISTER_FIELD_READ:
            /* CASE (Read a table SSM2602 register(s) field(s)) */
            case ADI_DEV_CMD_REGISTER_FIELD_TABLE_READ:

                /* get the address of Field access table submitted by the app */
                poFieldAccess = (ADI_DEV_ACCESS_REGISTER_FIELD *) Value;

                /* access all registers in application provided access table until we reach the delimiter */
                while (poFieldAccess->Address != ADI_DEV_REGEND)
                {
                    /* IF (read ALC 1/2 or Noise Gate register) */
                    if (poFieldAccess->Address > SSM2602_REG_RESET)
                    {
                        /* Index to the register in driver cache */
                        nCacheIndex = (poFieldAccess->Address - 6);
                    }
                    else
                    {
                        /* Index to the register in driver cache */
                        nCacheIndex = poFieldAccess->Address;
                    }

                    /* read register value from driver register cache &
                       extract the field data we're interested in */
                    nFieldData = (poDevice->aSSM2602Reg[nCacheIndex] & poFieldAccess->Field);

                    /* use the register field mask to determine the
                       shift count required to reach first bit of this register field */
                    for (nIndex = poFieldAccess->Field; nIndex; nIndex >>= 1)
                    {
                        /* IF (we've reached the first bit location of this field) */
                        if (nIndex & 1)
                        {
                            /* reached first bit of this field. exit this loop */
                            break;
                        }
                        /* shift 'nFieldData' to report the exact field value back to application */
                        nFieldData >>= 1;
                    }

                    /* pass this register field value to the application */
                    poFieldAccess->Data = nFieldData;

                    /* IF (the Command is to access a single register) */
                    if (nCommandID == ADI_DEV_CMD_REGISTER_FIELD_READ)
                    {
                        /* done with register access. exit this loop */
                        break;
                    }

                    /* move to next register field access info */
                    poFieldAccess++;
                }

                break;

            /* CASE (Configure a specific SSM2602 register field) */
            case ADI_DEV_CMD_REGISTER_FIELD_WRITE:
            /* CASE (Configure a table SSM2602 register(s) field(s)) */
            case ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE:

                /* get the address of Field access table submitted by the app */
                poFieldAccess = (ADI_DEV_ACCESS_REGISTER_FIELD *) Value;

                /* access all registers in application provided access table until we reach the delimiter */
                while (poFieldAccess->Address != ADI_DEV_REGEND)
                {
                    /* IF (configure ALC 1/2 or Noise Gate register) */
                    if (poFieldAccess->Address > SSM2602_REG_RESET)
                    {
                        /* Index to the register in driver cache */
                        nCacheIndex = (poFieldAccess->Address - 6);
                    }
                    else
                    {
                        /* Index to the register in driver cache */
                        nCacheIndex = poFieldAccess->Address;
                    }

                    /* get the present register data from driver register cache */
                    nRegData = poDevice->aSSM2602Reg[nCacheIndex];
                    /* Get the register field data passed by teh application */
                    nFieldData = poFieldAccess->Data;

                    /* use the register field mask to determine the
                       shift count required to reach first bit of this register field */
                    for (nIndex = poFieldAccess->Field; nIndex; nIndex >>= 1)
                    {
                        /* IF (we've reached the first bit location of this field) */
                        if (nIndex & 1)
                        {
                            break;  /* reached first bit of this field. exit this loop */
                        }
                        /* shift the 'nFieldData' until we reach its corresponding field location */
                        nFieldData <<= 1;
                    }

                    /* clear the register field that we're about to update */
                    nRegData &= ~poFieldAccess->Field;
                    /* update the register field with new value */
                    nRegData |= (nFieldData & poFieldAccess->Field);

                    /**********************************************
                    ** Packet this register address and data as per
                    ** SSM2602 register configuration requirements
                    **********************************************/
                    /* Shift the register address
                       (Bit 15 to Bit 9 in SSM2602 write packet holds the
                        register address to configure) */
                    oAccessSSM2602.Address = ((poFieldAccess->Address << ADI_SSM2602_REG_ADDR_SHIFT) & ADI_SSM2602_REG_ADDR_MASK);
                    /* Bit 8 to Bit 0 in SSM2602 write packet holds the register data */
                    oAccessSSM2602.Address |= nRegData;
                    /* Pass the new register data to
                       device access service to configure SSM2602 */
                    nResult = adi_ssm2602_DeviceAccess(poDevice,&oAccessSSM2602);

                    /* IF (Failed to update SSM2602 register(s)) */
                    if (nResult != ADI_DEV_RESULT_SUCCESS)
                    {
                        /* report error */
                        break;
                    }

                    /* IF (the Command is to access a single register) */
                    if (nCommandID == ADI_DEV_CMD_REGISTER_FIELD_WRITE)
                    {
                        /* done with register access. exit this loop */
                        break;
                    }
                    /* move to next register field access info */
                    poFieldAccess++;
                }

                break;

            /* CASE (Read block of SSM2602 registers starting from first given address) */
            case ADI_DEV_CMD_REGISTER_BLOCK_READ:

                /* get the address of Block access table submitted by the app */
                poBlockAccess = (ADI_DEV_ACCESS_REGISTER_BLOCK *) Value;
                /* Load the register block start address */
                nRegAddr = poBlockAccess->Address;

                /* Access all the registers requested by the application */
                for (nIndex = 0; nIndex < poBlockAccess->Count ; nIndex++)
                {
                    /* IF (Read ALC 1/2 or Noise Gate register) */
                    if (nRegAddr > SSM2602_REG_RESET)
                    {
                        /* Index to the register in driver cache */
                        nCacheIndex = (nRegAddr - 6);
                    }
                    else
                    {
                        /* Index to the register in driver cache */
                        nCacheIndex = nRegAddr;
                    }

                    /* copy the register value from driver register cache */
                    *(poBlockAccess->pData+nIndex) = poDevice->aSSM2602Reg[nCacheIndex];

                    /* IF (Last register read == Active Control register) */
                    if (nRegAddr == SSM2602_REG_ACTIVE_CTRL)
                    {
                        /* Reset register is the next register address to configure */
                        nRegAddr = SSM2602_REG_ALC_1;
                    }
                    /* ELSE IF (Register address exceeds the limit) */
                    else if (nRegAddr > SSM2602_REG_NOISE_GATE)
                    {
                        /* no registers left to read, exit this loop */
                        break;
                    }
                    /* ELSE (Move to next register address to access) */
                    else
                    {
                        nRegAddr++;
                    }
                }

                break;

            /* CASE (Write to a block of SSM2602 registers starting from first given address) */
            case(ADI_DEV_CMD_REGISTER_BLOCK_WRITE):

                /* get the address of Block access table submitted by the app */
                poBlockAccess = (ADI_DEV_ACCESS_REGISTER_BLOCK *) Value;
                /* Load the register block start address */
                nRegAddr = poBlockAccess->Address;

                /* Access all the registers requested by the application */
                for (nIndex = 0; nIndex < poBlockAccess->Count ; nIndex++)
                {
                    /**********************************************
                    ** Packet this register address and data as per
                    ** SSM2602 register configuration requirements
                    **********************************************/
                    /* Shift the register address
                       (Bit 15 to Bit 9 in SSM2602 write packet holds the
                        register address to configure) */
                    oAccessSSM2602.Address = ((nRegAddr << ADI_SSM2602_REG_ADDR_SHIFT) & ADI_SSM2602_REG_ADDR_MASK);
                    /* Bit 8 to Bit 0 in SSM2602 write packet
                       holds the register data */
                    oAccessSSM2602.Address |= (*(poBlockAccess->pData+nIndex) & ADI_SSM2602_REG_DATA_MASK);
                    /* Pass the new register data to
                       device access service to configures SSM2602 */
                    nResult = adi_ssm2602_DeviceAccess(poDevice,&oAccessSSM2602);

                    /* IF (Failed to update SSM2602 register(s)) */
                    if (nResult != ADI_DEV_RESULT_SUCCESS)
                    {
                        /* report error */
                        break;
                    }

                    /* IF (Last register configured == Active Control register) */
                    if (nRegAddr == SSM2602_REG_ACTIVE_CTRL)
                    {
                        /* Reset register is the next register address to configure */
                        nRegAddr = SSM2602_REG_RESET;
                    }
                    /* ELSE IF (Register address exceeds the limit) */
                    else if (nRegAddr > SSM2602_REG_NOISE_GATE)
                    {
                        /* no registers left to configure */
                        break;
                    }
                    /* ELSE (Move to next register address to access) */
                    else
                    {
                        nRegAddr++;
                    }
                }

                break;

            /* DEFAULT: other commands */
            default:
                /* Report failure (Command not supported) */
                nResult = ADI_SSM2602_RESULT_CMD_NOT_SUPPORTED;
                break;

        } /* End of switch (nCommandID) cases */

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return(nResult);
}


/*********************************************************************

    Function: adi_ssm2602_DeviceAccess

        Updates a SSM2602 hardware register using Device Access Service

    Parameters:
        poDevice        - Pointer to SSM2602 device instance to work on
        poAccessSSM2602 - Pointer to register access instance
                          holding info on SSM2602 register to access

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully accessed SSM2602 register
        SPI/TWI/Device Access/SSM2602 Driver specific Error codes

*********************************************************************/
static u32 adi_ssm2602_DeviceAccess(
    ADI_SSM2602_DRIVER_DEF      *poDevice,
    ADI_DEV_ACCESS_REGISTER     *oAccessSSM2602
)
{
    /* Return code - assume we're going to be successful */
    u32                         nResult = ADI_DEV_RESULT_SUCCESS;
    /* create a device access data instance */
    ADI_DEVICE_ACCESS_REGISTERS oConfigSSM2602;
    /* structure to hold Device type and access type to be used */
    ADI_DEVICE_ACCESS_SELECT    oAccessSelect;
    /* SSM2602 register access info for TWI based register access */
    ADI_DEV_ACCESS_REGISTER     oDevAccessTable;

    /* IF (Client set to use SPI to access SSM2602 registers) */
    if (poDevice->eDriverStatus == ADI_SSM2602_DRIVER_USE_SPI)
    {
        /*** This device falls under special case SPI access mode ***/

        /* SPI Chip-select for SSM2602 */
        oAccessSelect.DeviceCS   = poDevice->nTwiGAddrSpiCs;
        /* No SPI Global address for SSM2602 */
        oAccessSelect.Gaddr_len  = ADI_DEVICE_ACCESS_LENGTH0;
        /* Register address = 2bytes (Register address & data merged as one packet) */
        oAccessSelect.Raddr_len  = ADI_DEVICE_ACCESS_LENGTH2;
        /* Passing this as 0 sets Device access to consider
           SSM2602 as special case SPI device */
        oAccessSelect.Rdata_len  = ADI_DEVICE_ACCESS_LENGTH0;
        /* Use SPI to configure SSM2602 registers */
        oAccessSelect.AccessType = ADI_DEVICE_ACCESS_TYPE_SPI;

        /*** Populate Device Access instance fields specific to SPI ***/
        /* No SPI Global address for SSM2602    */
        oConfigSSM2602.DeviceAddress = 0;
        /* Command specific value */
        oConfigSSM2602.Value = (void *)poAccessSSM2602;
    }
    /* ELSE (Client has choosen to use TWI to access SSM2602 registers) */
    else
    {
        /* Don't care for TWI access */
        oAccessSelect.DeviceCS   = 0;
        /* Don't care for TWI Access */
        oAccessSelect.Gaddr_len  = ADI_DEVICE_ACCESS_LENGTH0;
        /* Register address length */
        oAccessSelect.Raddr_len  = ADI_DEVICE_ACCESS_LENGTH1;
        /* Register data length */
        oAccessSelect.Rdata_len  = ADI_DEVICE_ACCESS_LENGTH1;
        /* Use TWI to configure SSM2602 registers */
        oAccessSelect.AccessType = ADI_DEVICE_ACCESS_TYPE_TWI;

        /*** Split the SSM2602 datapacket for TWI access mode ***/
        /* Bits 15:8 passed as address field */
        oDevAccessTable.Address  = (poAccessSSM2602->Address >> 8 & 0xFF);
        /* Bits 7:0 passed as data field */
        oDevAccessTable.Data     = (poAccessSSM2602->Address & 0xFF);

        /*** Populate Device Access instance fields specific to TWI ***/
        /* TWI Address for SSM2602 */
        oConfigSSM2602.DeviceAddress = poDevice->nTwiGAddrSpiCs;
        /* Command specific Value */
        oConfigSSM2602.Value = (void*)&oDevAccessTable;
    }

    /* IF (Device is opened in transmit/bi-directional mode) */
    if (poDevice->oAudioTx.eDirection != ADI_DEV_DIRECTION_UNDEFINED)
    {
        /* Device manager handle */
        ConfigSSM2602.ManagerHandle = poDevice->oAudioTx.hDeviceManager;
        /* handle to the callback manager */
        ConfigSSM2602.DCBHandle     = poDevice->oAudioTx.hDcbManager;
    }
    /* ELSE (Device must be opened for receive mode) */
    else
    {
        /* Device manager handle */
        ConfigSSM2602.ManagerHandle = poDevice->oAudioRx.hDeviceManager;
        /* handle to the callback manager */
        ConfigSSM2602.DCBHandle     = poDevice->oAudioRx.hDcbManager;
    }

    /*** Populate Device access instance ***/

    /* No client handle */
    oConfigSSM2602.ClientHandle         = NULL;
    /* SPI/TWI Device number to use */
    oConfigSSM2602.DeviceNumber         = poDevice->nSpiTwiDevNumber;
    /* No Callback from device access */
    oConfigSSM2602.DeviceFunction       = NULL;
    /* Command ID to configure a register */
    oConfigSSM2602.Command              = ADI_DEV_CMD_REGISTER_WRITE;
    /* Address of the last register SSM2602 (with max data) */
    oConfigSSM2602.FinalRegAddr         = ADI_SSM2602_FINAL_REG_ADDRESS;
    /* No need to check for register field errors */
    oConfigSSM2602.RegisterField        = NULL;
    /* No need to update for reserved bits */
    oConfigSSM2602.ReservedValues       = NULL;
    /* Register address already validated. no need to repeat */
    oConfigSSM2602.ValidateRegister     = NULL;
    /* SPI/TWI configuration table passed by the client */
    oConfigSSM2602.ConfigTable          = poDevice->paSpiTwiConfigTable;
    /* Device Access type */
    oConfigSSM2602.SelectAccess         = &oAccessSelect;
    /* No Additional info */
    oConfigSSM2602.pAdditionalinfo      = (void *)NULL;

    /* Updates driver register cache */
    nResult = adi_ssm2602_UpdateDriverCache(poDevice,poAccessSSM2602);

    /* IF (Updates driver register cache resulted in success) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* call device access to configure corresponding SSM2602 register */
        nResult = adi_device_access (&oConfigSSM2602);
    }

    /* return */
    return (nResult);
}


/*********************************************************************

    Function: adi_ssm2602_UpdateDriverCache

        Updates driver register cache to keep in track of
        SSM2602 hardware register changes

    Parameters:
        poDevice        - Pointer to SSM2602 driver instance to work on
        poAccessSSM2602 - Pointer to register access instance
                          holding info on SSM2602 register to access

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully updated SSM2602 register cache
        ADI_DEV_RESULT_INVALID_REG_ADDRESS
            - Supplied register address is invalid
       Error codes from SPORT configuration function

*********************************************************************/
static u32 adi_ssm2602_UpdateDriverCache(
    ADI_SSM2602_DRIVER_DEF      *poDevice,
    ADI_DEV_ACCESS_REGISTER     *poAccessSSM2602
)
{
    /* variables to manipulate register data & driver cache */
    u8  nRegAddr;
    u16 nTemp;
    /* return code - assume we're going to be successful */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;

    /* Extract the register address from the given register access info */
    nRegAddr = (u8) ((poAccessSSM2602->Address & ADI_SSM2602_REG_ADDR_MASK) >> ADI_SSM2602_REG_ADDR_SHIFT);

    /* IF (Configure Reset Register) */
    if (RegAddr == SSM2602_REG_RESET)
    {
        /* IF (soft-reset SSM2602) */
        if (!(oAccessSSM2602->Address & ADI_SSM2602_REG_DATA_MASK))
        {
            /*
            **Initialise SSM2602 register cache to default values
            */
            /* Left Line In Mute, 0dB */
            poDevice->anSSM2602Reg[SSM2602_REG_LEFT_ADC_VOL]    = 0x0097U;
            /* Right Line In Mute, 0dB */
            poDevice->anSSM2602Reg[SSM2602_REG_RIGHT_ADC_VOL]   = 0x0097U;
            /* Left HP volume = 0dB */
            poDevice->anSSM2602Reg[SSM2602_REG_LEFT_DAC_VOL]    = 0x0079U;
            /* Right HP volume = 0dB */
            poDevice->anSSM2602Reg[SSM2602_REG_RIGHT_DAC_VOL]   = 0x0079U;
            /* Enable Bypass, MIC Mute enabled */
            poDevice->anSSM2602Reg[SSM2602_REG_ANALOGUE_PATH]   = 0x000AU;
            /* Enable DAC Soft mute */
            poDevice->anSSM2602Reg[SSM2602_REG_DIGITAL_PATH]    = 0x0008U;
            /* Power-down all except ClkOut & Oscillator */
            poDevice->anSSM2602Reg[SSM2602_REG_POWER]           = 0x009FU;
            /* I2S mode, 24-bits */
            poDevice->anSSM2602Reg[SSM2602_REG_DIGITAL_IFACE]   = 0x000AU;
            /* Normal mode, 256fs */
            poDevice->anSSM2602Reg[SSM2602_REG_SAMPLING_CTRL]   = 0;
            /* SSM2602 is Not Active */
            poDevice->anSSM2602Reg[SSM2602_REG_ACTIVE_CTRL]     = 0;
            /* SSM2602 Automatic Level Control register 1 */
            poDevice->aSSM2602Reg[SSM2602_REG_ACTIVE_CTRL+1]   = 0x007BU;
            /* SSM2602 Automatic Level Control register 2 */
            poDevice->aSSM2602Reg[SSM2602_REG_ACTIVE_CTRL+2]   = 0x0032U;
            /* SSM2602 Noise Gate register */
            poDevice->aSSM2602Reg[SSM2602_REG_ACTIVE_CTRL+3]   = 0;

            /* Initialise Sport register cache to default values */
            poDevice->nTxCtrlReg1 = (ADI_SPORT_TFSR  | ADI_SPORT_TCKFE | ADI_SPORT_ITCLK | ADI_SPORT_ITFS);
            poDevice->nTxCtrlReg2 = (ADI_SPORT_TSFSE | ADI_SPORT_SLEN_24);
            poDevice->nRxCtrlReg1 = (ADI_SPORT_RFSR  | ADI_SPORT_RCKFE | ADI_SPORT_IRCLK | ADI_SPORT_IRFS);
            poDevice->nRxCtrlReg2 = (ADI_SPORT_RSFSE | ADI_SPORT_SLEN_24);
        }
    }
    /* ELSE IF (registers between ADC volume and Active Control) */
    else if (RegAddr <= SSM2602_REG_ACTIVE_CTRL)
    {
        /* update driver register cache */
        poDevice->anSSM2602Reg[RegAddr] = (oAccessSSM2602->Address & ADI_SSM2602_REG_DATA_MASK);

        /* extract SSM2602 audio input format */
        u16Temp = (poDevice->anSSM2602Reg[SSM2602_REG_DIGITAL_IFACE] & SSM2602_RFLD_IFACE_FORMAT);

        /*
        ** Update SPORT control register values cached by driver for
        ** Auto-SPORT configuration feature.
        ** Auto-SPORT Configuration is no not support when
        ** Audio CODEC is configured for Right-Justified or DSP mode
        */

        /* IF (Left Justified or I2S mode) */
        if ((u16Temp == SSM2602_STEREO_MODE_LJ) ||\
            (u16Temp == SSM2602_STEREO_MODE_I2S))
        {
            /* Calculate new SPORT register configuration value */
            poDevice->nTxCtrlReg1 = ADI_SPORT_TFSR;
            poDevice->nTxCtrlReg2 = ADI_SPORT_TSFSE;
            poDevice->nRxCtrlReg1 = ADI_SPORT_RFSR;
            poDevice->nRxCtrlReg2 = ADI_SPORT_RSFSE;

            /* IF (Audio Data Format is set to Left-Justified Mode) */
            if (u16Temp == SSM2602_STEREO_MODE_LJ)
            {
                /* IF (DACLR Phase not Inverted) */
                if (!(poDevice->anSSM2602Reg[SSM2602_REG_DIGITAL_IFACE] & SSM2602_RFLD_DAC_LR_POLARITY))
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
                if (!(poDevice->anSSM2602Reg[SSM2602_REG_DIGITAL_IFACE] & SSM2602_RFLD_BCLK_INVERT))
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
                if (poDevice->anSSM2602Reg[SSM2602_REG_DIGITAL_IFACE] & SSM2602_RFLD_DAC_LR_POLARITY)
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
                if (!(poDevice->anSSM2602Reg[SSM2602_REG_DIGITAL_IFACE] & SSM2602_RFLD_BCLK_INVERT))
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
            u16Temp = (poDevice->anSSM2602Reg[SSM2602_REG_DIGITAL_IFACE] & SSM2602_RFLD_AUDIO_DATA_LEN);

            /* IF (Input Audio Data bit length is set to 16 bits) */
            if (u16Temp == SSM2602_WORD_LENGTH_16BITS)
            {
                poDevice->nTxCtrlReg2 |= ADI_SPORT_SLEN_16;
                poDevice->nRxCtrlReg2 |= ADI_SPORT_SLEN_16;
            }
            /* ELSE IF (Input Audio Data bit length is set to 20 bits) */
            else if (u16Temp == SSM2602_WORD_LENGTH_20BITS)
            {
                poDevice->nTxCtrlReg2 |= ADI_SPORT_SLEN_20;
                poDevice->nRxCtrlReg2 |= ADI_SPORT_SLEN_20;
            }
            /* ELSE IF (Input Audio Data bit length is set to 24 bits) */
            else if (u16Temp == SSM2602_WORD_LENGTH_24BITS)
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

            /* IF (SSM2602 set in slave mode) */
            if (!(poDevice->anSSM2602Reg[SSM2602_REG_DIGITAL_IFACE] & SSM2602_RFLD_ENABLE_MASTER))
            {
                /* Enable SPORT Internal Clock Generation and Internal FS generation */
                poDevice->nTxCtrlReg1 |= (ADI_SPORT_ITCLK | ADI_SPORT_ITFS);
                poDevice->nRxCtrlReg1 |= (ADI_SPORT_IRCLK | ADI_SPORT_IRFS);
            }
        }
    }
    /* ELSE IF (registers between ALC control or Noise Gate register) */
    else if ((nRegAddr >= SSM2602_REG_ALC_1) && (nRegAddr <= SSM2602_REG_NOISE_GATE))
    {
        /* update driver register cache */
        poDevice->aSSM2602Reg[nRegAddr-6] = (poAccessSSM2602->Address & ADI_SSM2602_REG_DATA_MASK);
    }
    /* ELSE (invalid register, return error) */
    else
    {
        nResult = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
    }

    return (nResult);

}

/*********************************************************************

    Function: adi_ssm2602_AdjustToMclk

        Adjusts SSM2602 Sample Control register value to match MCLK frequency

    Parameters:
        poDevice    - Pointer to SSM2602 driver instance to work on
        nMclkFreq   - MCLK Frequency selected

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully adjusted Sample rate register
        PPI/SPI/Sharp LQ035Q1DH02 Driver specific Error codes

*********************************************************************/
static u32 adi_ssm2602_AdjustToMclk(
    ADI_SSM2602_DRIVER_DEF      *poDevice,
    u32                         nMclkFreq
)
{
    /* to manipulate SSM2602 Sampling Rate register value */
    u16                         nRegData;
    /* return code - assume we're going to be successful */
    u32                         nResult = ADI_DEV_RESULT_SUCCESS;
    /* structure to access one SSM2602 register */
    ADI_DEV_ACCESS_REGISTER     oAccessSSM2602;

    /* Look-up Table to update CLKDIV2, USB & BOSR fields in
       Sample Rate Register based on the given MCLK frequency */
    const static u8 anSRFieldLut[] =
    {
        /* MCLK = 11.2896 MHz (CLKDIV2 = 0, USB = 0, BOSR = 0) */
        0,
        /* MCLK = 12 MHz (CLKDIV2 = 0, USB = 1) */
        SSM2602_RFLD_ENABLE_USB_MODE,
        /* MCLK = 12.288 MHz (CLKDIV2 = 0, USB = 0, BOSR = 0) */
        0,
        /* MCLK = 16.9344 MHz (CLKDIV2 = 0, USB = 0, BOSR = 1) */
        SSM2602_RFLD_BOS_RATE,
        /* MCLK = 18.432 MHz (CLKDIV2 = 0, USB = 0, BOSR = 1) */
        SSM2602_RFLD_BOS_RATE,
        /* MCLK = 22.5792 MHz (CLKDIV2 = 1, USB = 0, BOSR = 0) */
        SSM2602_RFLD_CLKOUT_DIV2,
        /* MCLK = 24 MHz (CLKDIV2 = 1, USB = 1) */
        (SSM2602_RFLD_CLKOUT_DIV2 | SSM2602_RFLD_ENABLE_USB_MODE),
        /* MCLK = 24.576 MHz (CLKDIV2 = 1, USB = 0, BOSR = 0) */
        SSM2602_RFLD_CLKOUT_DIV2,
        /* MCLK = 33.8688 MHz (CLKDIV2 = 1, USB = 0, BOSR = 1) */
        (SSM2602_RFLD_CLKOUT_DIV2 | SSM2602_RFLD_BOS_RATE),
        /* MCLK = 36.864 MHz (CLKDIV2 = 1, USB = 0, BOSR = 1) */
        (SSM2602_RFLD_CLKOUT_DIV2 | SSM2602_RFLD_BOS_RATE)
    };

    /* Get the Sampling Control register value */
    nRegData = poDevice->aSSM2602Reg[SSM2602_REG_SAMPLING_CTRL];

    /* IF (MCLK value is valid) */
    if (nMclkFreq < (sizeof(anSRFieldLut)/sizeof(anSRFieldLut[0])))
    {
        /* Clear CLKDIV2 & USB fields in Sample rate register */
        nRegData &= ~(SSM2602_RFLD_CLKOUT_DIV2 |\
                      SSM2602_RFLD_ENABLE_USB_MODE);

        /* IF (MCLK is other than 12 MHz or 24MHz) */
        if (((ADI_SSM2602_MCLK_FREQ)nMclkFreq != ADI_SSM2602_MCLK_12) &&\
            ((ADI_SSM2602_MCLK_FREQ)nMclkFreq != ADI_SSM2602_MCLK_24))
        {
            /* Clear BOSR field in Sample rate register */
            nRegData &= ~SSM2602_RFLD_BOS_RATE;
        }

        /* Update CLKDIV2 & USB fields for selected MCLK value */
        nRegData |= anSRFieldLut[nMclkFreq];

        /* Shift the register address
           (Bit 15 to Bit 9 in SSM2602 write packet holds the
           register address to configure) */
        oAccessSSM2602.Address = ((SSM2602_REG_SAMPLING_CTRL << ADI_SSM2602_REG_ADDR_SHIFT) & ADI_SSM2602_REG_ADDR_MASK);
        /* Bit 8 to Bit 0 in SSM2602 write packet holds the register data */
        oAccessSSM2602.Address |= (nRegData & ADI_SSM2602_REG_DATA_MASK);
        /* Update Sample Rate register value in SSM2602 device and driver cache */
        nResult = adi_ssm2602_UpdateDriverCache(poDevice,&oAccessSSM2602);
    }
    /* ELSE (given MCLK must be invalid) */
    else
    {
        /* report error (MCLK invalid) */
        nResult = ADI_SSM2602_RESULT_MCLK_INVALID;
    }

    return (nResult);
}

/*********************************************************************

    Function: adi_ssm2602_UpdateSampleRate

        Updates SSM2602 Sampling rate register to match
        the given ADC,DAC Sample rates

    Parameters:
        poDevice        - Pointer to the driver instance
        poSampleRate    - Pointer to structure holding ADC/DAC Sample rates

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully updated sample rate
        ADI_SSM2602_RESULT_SAMPLE_RATE_NOT_SUPPORTED
            - Given sample rate combination is not supported by the device

*********************************************************************/
static u32 adi_ssm2602_UpdateSampleRate(
    ADI_SSM2602_DEF             *poDevice,
    ADI_SSM2602_SAMPLE_RATE     *poSampleRate
)
{
    /* Index to sample rate look-up table / Sample Rate Field Value */
    u8                          nIndexSRValue;
    /* to manipulate SSM2602 Sampling Rate register value */
    u16                         nRegData;
    /* return code - assume we're going to be successful */
    u32                         nResult = ADI_DEV_RESULT_SUCCESS;
    /* structure to access one SSM2602 register */
    ADI_DEV_ACCESS_REGISTER     oAccessSSM2602;

    /* Sampling Rate Look-up Table as provided in SSM2602 spec */
    const static u32 aSamplingRateLut[] =
    {
        /* Normal & USB Mode -> SR = 0, ADC = 48kHz, DAC = 48kHz */
        ADI_SSM2602_SR_48000,
        /* Normal & USB Mode -> SR = 1, ADC = 48kHz, DAC = 8kHz */
        /* Special Case */
        0,
        /* Normal & USB Mode -> SR = 2, ADC = 8kHz, DAC = 48kHz */
        /* Special Case */
        0,
        /* Normal & USB Mode -> SR = 3, ADC = 8kHz, DAC = 8kHz */
        ADI_SSM2602_SR_8000,
        /* Normal Mode  -> SR = 4, ADC = 12kHz, DAC = 12kHz */
        /* USB Mode     -> SR = 4 is invalid */
        ADI_SSM2602_SR_12000,
        /* Normal Mode  -> SR = 5, ADC = 16kHz, DAC = 16kHz */
        /* USB Mode     -> SR = 5 is invalid */
        ADI_SSM2602_SR_16000,
        /* Normal & USB Mode -> SR = 6, ADC = 32kHz, DAC = 32kHz */
        ADI_SSM2602_SR_32000,
        /* Normal & USB Mode -> SR = 7, ADC = 96kHz, DAC = 96kHz */
        ADI_SSM2602_SR_96000,
        /* Normal Mode  -> SR = 8, ADC = 44.1kHz, DAC = 44.1kHz */
        /* USB Mode     -> SR = 8, BOSR = 0, ADC = 12kHz, DAC = 12kHz */
        /* USB Mode     -> SR = 8, BOSR = 1, ADC = 44.1kHz, DAC = 44.1kHz */
        ADI_SSM2602_SR_44100,
        /* Normal & USB Mode -> SR = 9, ADC = 44.1kHz, DAC = 8.02kHz */
        /* Special Case */
        0,
        /* Normal Mode  -> SR = A, ADC = 8.02kHz, DAC = 44.1kHz */
        /* USB Mode     -> SR = A, BOSR = 0, ADC = 16kHz, DAC = 16kHz */
        /* USB Mode     -> SR = A, BOSR = 1, ADC = 8.02kHz, DAC = 44.1kHz */
        /* Special Case */
        0,
        /* Normal & USB Mode -> SR = B, ADC = 8.02kHz, DAC = 8.02kHz */
        ADI_SSM2602_SR_8020,
        /* Normal & USB Mode -> SR = C, ADC = 11.025kHz, DAC = 11.025kHz */
        ADI_SSM2602_SR_11025,
        /* Normal & USB Mode -> SR = D, ADC = 22.05kHz, DAC = 22.05kHz */
        ADI_SSM2602_SR_22050,
        /* Normal & USB Mode -> SR = E, ADC = 24kHz, DAC = 24kHz */
        ADI_SSM2602_SR_24000,
        /* Normal & USB Mode -> SR = F, ADC = 88.2kHz, DAC = 88.2kHz */
        ADI_SSM2602_SR_88200
    };

    /* Get the Sampling Control register value */
    nRegData = poDevice->aSSM2602Reg[SSM2602_REG_SAMPLING_CTRL];

    /*** Get the new SR field value from Sampling Rate Lookup Table ***/
    /* IF (ADC and DAC set to different sample rates) */
    if (poSampleRate->nAdcSampleRate != poSampleRate->nDacSampleRate)
    {
        /* This SR field value falls under Special case sampling rate */
        /* IF (ADC Sample Rate = 48kHz & DAC Sample Rate = 8kHz) */
        if ((poSampleRate->nAdcSampleRate == ADI_SSM2602_SR_48000) &&\
            (poSampleRate->nDacSampleRate == ADI_SSM2602_SR_8000))
        {
            /* SR Field value = 1 */
            nIndexSRValue = 0x01U;
        }
        /* ELSE IF (ADC Sample Rate = 8kHz & DAC Sample Rate = 48kHz) */
        else if ((poSampleRate->nAdcSampleRate == ADI_SSM2602_SR_8000) &&\
                 (poSampleRate->nDacSampleRate == ADI_SSM2602_SR_48000))
        {
            /* SR Field value = 2 */
            nIndexSRValue = 0x02U;
        }
        /* ELSE IF (ADC Sample Rate = 44.1kHz & DAC Sample Rate = 8.02kHz) */
        else if ((poSampleRate->nAdcSampleRate == ADI_SSM2602_SR_44100) &&\
                 (poSampleRate->nDacSampleRate == ADI_SSM2602_SR_8020))
        {
            /* SR Field value = 9 */
            nIndexSRValue = 0x09U;
        }
        /* ELSE IF (ADC Sample Rate = 8.02kHz & DAC Sample Rate = 44.1kHz) */
        else if ((poSampleRate->nAdcSampleRate == ADI_SSM2602_SR_8020) &&\
                 (poSampleRate->nDacSampleRate == ADI_SSM2602_SR_44100))
        {
            /* SR Field value = A */
            nIndexSRValue = 0x0AU;
        }
        /* ELSE (sampling rate combination not supported) */
        else
        {
            /* Report error */
            nResult = ADI_SSM2602_RESULT_SAMPLE_RATE_NOT_SUPPORTED;
        }
    }
    /* ELSE (ADC and DAC sample rates are same) */
    else
    {
        /* Assume that the given sampling rate is not supported */
        nResult = ADI_SSM2602_RESULT_SAMPLE_RATE_NOT_SUPPORTED;

        /* Get the SR value for the given sample rate */
        for (nIndexSRValue = 0;
             nIndexSRValue < (sizeof(aSamplingRateLut)/sizeof(aSamplingRateLut[0]));
             nIndexSRValue++)
        {
            /* IF (the ADC sample rate matches with look-up table entry) */
            if (poSampleRate->nAdcSampleRate == aSamplingRateLut[nIndexSRValue])
            {
                /* Given sample rate is supported */
                nResult = ADI_DEV_RESULT_SUCCESS;
                /* exit this loop */
                break;
            }
        }

        /* IF (Given sample rate is valid) */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* 'nIndexSRValue' holds the new SR field value */

            /* IF (USB mode enabled) */
            if (nRegData & SSM2602_RFLD_ENABLE_USB_MODE)
            {
                /* Clear BOSR bit */
                nRegData &= ~SSM2602_RFLD_BOS_RATE;

                /* IF (Sample rate is set to 12kHz) */
                if (poSampleRate->nAdcSampleRate == ADI_SSM2602_SR_12000)
                {
                    /* SR value must be set to 8 */
                    nIndexSRValue = 0x08U;
                }
                /* ELSE IF (Sample rate is set to 16kHz) */
                else if (poSampleRate->nAdcSampleRate == ADI_SSM2602_SR_16000)
                {
                    /* SR value must be set to A */
                    nIndexSRValue = 0x0AU;
                }
                /* ELSE IF (Sampling rate is 88.2/44.1/22.05/11.025/8.02kHz) */
                else if (nIndexSRValue > 0x07U)
                {
                    /* BOSR bit must be set for these sampling rates */
                    nRegData |= SSM2602_RFLD_BOS_RATE;
                }

            } /* End of if (USB mode enabled) */

        } /* End of if (Sample rate is valid) */

    } /* End of if (ADC and DAC sample rates are different) else case */

    /* IF (Given sample rates are valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Clear SR field */
        nRegData &= ~SSM2602_RFLD_SAMPLE_RATE;
        /* 'nIndexSRValue' holds the new SR value, update SR field */
        nRegData |= (nIndexSRValue << ADI_SSM2602_SHIFT_SR);

        /**********************************************
        ** Packet this register address and data as per
        ** SSM2602 register configuration requirements
        **********************************************/
        /* Shift the register address
           (Bit 15 to Bit 9 in SSM2602 write packet holds the
            register address to configure) */
        oAccessSSM2602.Address = (SSM2602_REG_SAMPLING_CTRL << ADI_SSM2602_REG_ADDR_SHIFT);
        /* Bit 8 to Bit 0 in SSM2602 write packet holds the register data */
        oAccessSSM2602.Address |= nRegData;
        /* update SSM2602 Sampling Control register */
        nResult = adi_ssm2602_DeviceAccess(poDevice,&oAccessSSM2602);
    }

    return (nResult);
}

/*********************************************************************

    Function: adi_ssm2602_SportOpen

        Opens SPORT device allocated to SSM2602 Audio Codec

    Parameters:
        poDataPort - Pointer to SSM2602 SPORT Data port to open

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully opened SPORT Device
        Return codes from SPORT Driver

*********************************************************************/
static u32 adi_ssm2602_SportOpen(
    ADI_SSM2602_DRIVER_DATA_PORT_INFO  *poDataPort
)
{
    /* Default return code */
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
                           adi_ssm2602_SportCallback);

    /* IF (Successfully opened SPORT device) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* IF (SPORT is opened in bi-directional mode) */
        if (poDataPort->eDirection == ADI_DEV_DIRECTION_BIDIRECTIONAL)
        {
            /* Copy the device handle information to receive port */
            gaoSSM2602Device[poDataPort->nAudioDevNumber].oAudioRx.hSport = poDataPort->hSport;
        }
    }

    return (nResult);
}

/*********************************************************************

    Function: adi_ssm2602_ConfigSport

        Configures SPORT device registers as per SSM2602 operating conditions

    Parameters:
        poDevice    - Pointer to SSM2602 device instace to work on
        poDataPort  - Pointer to SSM2602 SPORT Data port to configure

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully configured SPORT Device registers
        Return codes from SPORT Driver

*********************************************************************/
static u32 adi_ssm2602_ConfigSport(
    ADI_SSM2602_DRIVER_DEF              *poDevice,
    ADI_SSM2602_DRIVER_DATA_PORT_INFO   *poDataPort
)
{
    /* Audio data interface format */
    u16     eInterfaceFormat;
    /* default return code */
    u32     nResult = ADI_DEV_RESULT_SUCCESS;

    /* Extract SSM2602 audio data interface format */
    eInterfaceFormat = (poDevice->anSSM2602Reg[SSM2602_REG_DIGITAL_IFACE] & SSM2602_RFLD_IFACE_FORMAT);

    /* IF (Auto-SPORT Configuration is enabled) */
    if (poDataPort->bIsAutoSportConfigEnabled == true)
    {
        /* Auto-SPORT Configuration does not support Right-Justified or DSP mode */
        /* DSP & Right-Justified modes - SPORT configuration registers left unchanged */

        /* IF (SSM2602 is set in Left Justified or I2S Mode) */
        if ((eInterfaceFormat == SSM2602_STEREO_MODE_LJ) ||\
            (eInterfaceFormat == SSM2602_STEREO_MODE_I2S))
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

        } /* End of if (SSM2602 is set in Left Justified or I2S Mode) */

    } /* End of if (Auto-SPORT Configuration is enabled) */

    return (nResult);
}

/*********************************************************************

    Function: adi_ssm2602_SportCallback

        Callback from the SPORT Driver

    Parameters:
        hCallback   - Callback Handle passed by SSM2602
        nEvent      - Callback Event
        pArgument   - Callback Argument

    Returns:
        None

*********************************************************************/
static void adi_ssm2602_SportCallback(
    void    *hCallback,
    u32     nEvent,
    void    *pArgument
)
{

    /* Pointer to SSM2602 device SPORT instance to which the callback belongs */
    ADI_SSM2602_DRIVER_DATA_PORT_INFO  *poDataPort = (ADI_SSM2602_DRIVER_DATA_PORT_INFO *)hCallback;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    /* IF (The given Physical Device Handle is valid) */
    if (adi_ssm2602_ValidatePDDHandle(phClient) == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* Pass the callback to the Device Manager Callback */
        (gaoSSM2602Device[poDataPort->nAudioDevNumber].pfCallback) (poDataPort->hDevice, nEvent, pArgument);

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

}

#if defined(ADI_DEV_DEBUG)  /* Debug build only */

/*********************************************************************

    Function: adi_ssm2602_ValidateRegAccess

        Validates the given register access table

    Parameters:
        poDevice   - Pointer to the driver instance
        nCommandID - Command passed by the application
        Value      - Command specific value

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully validated given register access table
        Error code returned by SPORT driver

*********************************************************************/
static u32 adi_ssm2602_ValidateRegAccess(
    ADI_SSM2602_DEF         *poDevice,
    u32                     nCommandID,
    void                    *Value
)
{

    /* return code - assume we're going to be successful */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;

    /* pointer to selective register access table */
    ADI_DEV_ACCESS_REGISTER         *poSelectiveAccess;
    /* pointer to block register access table */
    ADI_DEV_ACCESS_REGISTER_BLOCK   *poBlockAccess;
    /* pointer to register field access table */
    ADI_DEV_ACCESS_REGISTER_FIELD   *poFieldAccess;

    /* CASEOF (Device Access Command) */
    switch (nCommandID)
    {
        /* CASE (Read a specific SSM2602 Register) */
        case ADI_DEV_CMD_REGISTER_READ:
        /* CASE (Read a table of SSM2602 registers) */
        case ADI_DEV_CMD_REGISTER_TABLE_READ:

            /* get the address of selective register access table submitted by the app */
            poSelectiveAccess = (ADI_DEV_ACCESS_REGISTER *) Value;

            /* validate all registers in application provided access table until we reach the delimiter */
            while (poSelectiveAccess->Address != ADI_DEV_REGEND)
            {
                /* IF (this is a write-only register) */
                if (poSelectiveAccess->Address == SSM2602_REG_RESET)
                {
                    /* report error (function not supported) */
                    nResult = ADI_DEV_RESULT_NOT_SUPPORTED;
                    break;
                }
                /* IF (the register address is not valid) */
                if (((poSelectiveAccess->Address > SSM2602_REG_ACTIVE_CTRL) &&
                    (poSelectiveAccess->Address < SSM2602_REG_RESET)) ||
                    (poSelectiveAccess->Address > SSM2602_REG_NOISE_GATE))
                {
                    /* report error (Invalid register) */
                    nResult = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
                    break;
                }
                /* IF (Command is to access a single register) */
                if (nCommandID == ADI_DEV_CMD_REGISTER_READ)
                {
                    /* exit this loop */
                    break;
                }
                /* move to next selective register access info */
                poSelectiveAccess++;
            }
            break;

        /* CASE (Configure a SSM2602 Resigter) */
        case ADI_DEV_CMD_REGISTER_WRITE:
        /* CASE (Configure a table SSM2602 registers) */
        case ADI_DEV_CMD_REGISTER_TABLE_WRITE:

            /* get the address of selective register access table submitted by the app */
            poSelectiveAccess = (ADI_DEV_ACCESS_REGISTER *) Value;

            /* validate all registers in application provided access table until we reach the delimiter */
            while (poSelectiveAccess->Address != ADI_DEV_REGEND)
            {
                /* IF (the register address is not valid) */
                if (((poSelectiveAccess->Address > SSM2602_REG_ACTIVE_CTRL) &&
                    (poSelectiveAccess->Address < SSM2602_REG_RESET)) ||
                    (poSelectiveAccess->Address > SSM2602_REG_NOISE_GATE))
                {
                    /* report error (Invalid register) */
                    nResult = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
                    break;
                }
                /* IF (Command is to access a single register) */
                if (nCommandID == ADI_DEV_CMD_REGISTER_WRITE)
                {
                    /* exit this loop */
                    break;
                }
                /* move to next selective register access info */
                poSelectiveAccess++;
            }
            break;

        /* CASE (Read a specific SSM2602 register field) */
        case ADI_DEV_CMD_REGISTER_FIELD_READ:
        /* CASE (Read a table SSM2602 register(s) field(s)) */
        case ADI_DEV_CMD_REGISTER_FIELD_TABLE_READ:

            /* get the address of Field access table submitted by the app */
            poFieldAccess = (ADI_DEV_ACCESS_REGISTER_FIELD *) Value;

            /* validate all registers in application provided access table until we reach the delimiter */
            while (poFieldAccess->Address != ADI_DEV_REGEND)
            {
                /* IF (this is a write-only register) */
                if (poFieldAccess->Address == SSM2602_REG_RESET)
                {
                    /* report error (function not supported) */
                    nResult = ADI_DEV_RESULT_NOT_SUPPORTED;
                    break;
                }
                /* IF (the register address is not valid) */
                if (((poFieldAccess->Address > SSM2602_REG_ACTIVE_CTRL) &&
                    (poFieldAccess->Address < SSM2602_REG_RESET)) ||
                    (poFieldAccess->Address > SSM2602_REG_NOISE_GATE))
                {
                    /* report error (Invalid register) */
                    nResult = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
                    break;
                }
                /* IF (Command is to access a single register) */
                if (nCommandID == ADI_DEV_CMD_REGISTER_FIELD_READ)
                {
                    /* exit this loop */
                    break;
                }
                /* move to next register field access info */
                poFieldAccess++;
            }
            break;

        /* CASE (Configure a specific SSM2602 register field) */
        case ADI_DEV_CMD_REGISTER_FIELD_WRITE:
        /* CASE (Configure a table SSM2602 register(s) field(s)) */
        case ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE:

            /* get the address of Field access table submitted by the app */
            poFieldAccess = (ADI_DEV_ACCESS_REGISTER_FIELD *) Value;

            /* validate all registers in application provided access table until we reach the delimiter */
            while (poFieldAccess->Address != ADI_DEV_REGEND)
            {
                /* IF (the register address is not valid) */
                if (((poFieldAccess->Address > SSM2602_REG_ACTIVE_CTRL) &&
                    (poFieldAccess->Address < SSM2602_REG_RESET)) ||
                    (poFieldAccess->Address > SSM2602_REG_NOISE_GATE))
                {
                    /* report error (Invalid register) */
                    nResult = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
                    break;
                }
                /* IF (Command is to access a single register) */
                if (nCommandID == ADI_DEV_CMD_REGISTER_WRITE)
                {
                    /* exit this loop */
                    break;
                }
                /* move to next register field access info */
                poFieldAccess++;
            }
            break;

        /* CASE (Read block of SSM2602 registers starting from first given address) */
        case(ADI_DEV_CMD_REGISTER_BLOCK_READ):

            /* get the address of Block access table submitted by the app */
            poBlockAccess = (ADI_DEV_ACCESS_REGISTER_BLOCK *) Value;

            /* IF (there are any registers to access) */
            if (poBlockAccess->Count)
            {
                /* IF (this the block access starts with a write-only register) */
                if (poBlockAccess->Address == SSM2602_REG_RESET)
                {
                    /* report error (function not supported) */
                    nResult = ADI_DEV_RESULT_NOT_SUPPORTED;
                    break;
                }

                /* IF (the block access exceeds the readable register boundary) */
                if ((poBlockAccess->Address+(poBlockAccess->Count-1)) > (SSM2602_REG_ACTIVE_CTRL+3))
                {
                    /* report error (Invalid register) */
                    nResult = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
                    break;
                }
            }

            break;

        /* CASE (Write to a block of SSM2602 registers starting from first given address) */
        case(ADI_DEV_CMD_REGISTER_BLOCK_WRITE):

            /* get the address of Block access table submitted by the app */
            poBlockAccess = (ADI_DEV_ACCESS_REGISTER_BLOCK *) Value;

            /* IF (there are any registers to access) */
            if (poBlockAccess->Count)
            {
                /* IF (the block access exceeds the writeable register boundary) */
                if ((poBlockAccess->Address+(poBlockAccess->Count-1)) > (SSM2602_REG_ACTIVE_CTRL+4))
                {
                    /* report error (Invalid register) */
                    nResult = ADI_DEV_RESULT_INVALID_REG_ADDRESS;
                    break;
                }
            }
            break;

            /* DEFAULR: other commands */
            default:
                break;

        } /* End of switch case */

    return(nResult);
}

/*********************************************************************

    Function: adi_ssm2602_ValidatePDDHandle

        Validates the given Physical Device Driver Handle

    Parameters:
        hPhysicalDevice - Physical Device Driver Handle to validate

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully validated Physical Device Driver Handle
        ADI_DEV_RESULT_BAD_PDD_HANDLE
            - Physical Device Driver Handle is invalid

*********************************************************************/

static u32 adi_ssm2602_ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE      hPhysicalDevice
)
{
    /* Return value - insure the client has provided a valid PDD Handle */
    u32     nResult = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    /* index */
    u32     nIndex;

    /* compare the given Physical Device Driver Handle handle with Handles
       allocated to all SSM2602 devices in the list */
    for (nIndex = 0; nIndex < ADI_SSM2602_NUM_DEVICES; nIndex++)
    {
        if ((hPhysicalDevice == (ADI_DEV_PDD_HANDLE)&gaoSSM2602Device[i].oAudioTx) ||\
            (hPhysicalDevice == (ADI_DEV_PDD_HANDLE)&gaoSSM2602Device[i].oAudioRx))
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
** EOF: $
**
*/

/* $Revision: 3492 $
 * $Date: 2010-10-07 09:34:20 -0400 (Thu, 07 Oct 2010) $ 
******************************************************************************
Copyright (c), 2007-2009 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title: ADI Device Drivers - AD1980 Audio CODEC

Description:
    This is the primary source file for AD1980 AC'97 Audio CODEC driver

*****************************************************************************/

/*=============  I N C L U D E S   =============*/

/* system service includes */
#include <services/services.h>
/* device manager includes */
#include <drivers/adi_dev.h>
/* SPORT driver includes */
#include <drivers/sport/adi_sport.h>
/* AD1980 driver includes */
#include <drivers/codec/adi_ad1980.h>

/*=============  D E F I N E S  =============*/

/*
** AD1980 Record select register field mask for Phone Left Input channel
*/
#define AD1980_RECORD_PHONE_L_IN_MASK   0x0700U
/*
** AD1980 Record select register field mask for Line Left Input channel
*/
#define AD1980_RECORD_LINE_L_IN_MASK    0x0400U
/*
** AD1980 Record select register field mask for AUX Left Input channel
*/
#define AD1980_RECORD_AUX_L_IN_MASK     0x0300U
/*
** AD1980 Record select register field mask for CD Left Input channel
*/
#define AD1980_RECORD_CD_L_IN_MASK      0x0100U
/*
** AD1980 Record select register field mask for MIC 0 Left Input channel
*/
#define AD1980_RECORD_MIC_0_L_IN_MASK   0
/*
** AD1980 Record select register field mask for MIC 1 Left Input channel
*/
#define AD1980_RECORD_MIC_1_L_IN_MASK   0
/*
** Record select register field mask for Phone Right Input channel
*/
#define AD1980_RECORD_PHONE_R_IN_MASK   0x0007U
/*
** AD1980 Record select register field mask for Line Right Input channel
*/
#define AD1980_RECORD_LINE_R_IN_MASK    0x0004U
/*
** AD1980 Record select register field mask for AUX Right Input channel
*/
#define AD1980_RECORD_AUX_R_IN_MASK     0x0003U
/*
** AD1980 Record select register field mask for CD Right Input channel
*/
#define AD1980_RECORD_CD_R_IN_MASK      0x0001U
/*
** AD1980 Record select register field mask for MIC 0 Right Input channel
*/
#define AD1980_RECORD_MIC_0_R_IN_MASK   0
/*
** AD1980 Record select register field mask for MIC 1 Right Input channel
*/
#define AD1980_RECORD_MIC_1_R_IN_MASK   0

/*
**
** Enumerations
**
*/

/* number of AD1980 devices in the system  */
#define ADI_AD1980_NUM_DEVICES  (sizeof(gaoAD1980Device)/sizeof(ADI_AD1980_DEF))

/*
**
** Data Structures
**
*/

/*
** Structure to hold AD1980 Audio Data Interface Port specific information
*/
typedef struct __AdiAD1980DriverDataPortInfo
{
    /* AD1980 device instance number to with this port belongs */
    const u8            nAudioDevNumber;

    /* Data port direction */
    ADI_DEV_DIRECTION   eDirection;

} ADI_AD1980_DRIVER_DATA_PORT_INFO;

/*
** Structure to manage a AD1980 Device instance
*/
typedef struct __AdiAd1980Def
{

    /* SPORT Device Number connected to AD1980 */
    u8                                  nSportDevNumber;

    /* SPORT DMA bus width in bytes */
    u8                                  nSportDmaBusWidth;

    /* Reset Flag ID */
    ADI_FLAG_ID                         eResetFlag;

    /* Device Manager handle */
    ADI_DEV_MANAGER_HANDLE              hDeviceManager;

    /* Device instance Handle provided by Device Manager */
    ADI_DEV_DEVICE_HANDLE               hDevice;

    /* Handle to the DMA manager */
    ADI_DMA_MANAGER_HANDLE              hDmaManager;

    /* Deffered Callback handle */
    ADI_DCB_HANDLE                      hDcbManager;

    /* Callback function supplied by the Device Manager */
    ADI_DCB_CALLBACK_FN                 pfCallback;

    /* Handle to the underlying SPORT device driver */
    ADI_DEV_PDD_HANDLE                  hSport;

    /* critical region argument */
    void                                *pEnterCriticalArg;

    /* Pointer to a AC'97 driver instance */
    ADI_AC97_DRIVER_INSTANCE            *pAC97;

    /* Instance to manage audio transmit or bi-directional data */
    ADI_AD1980_DRIVER_DATA_PORT_INFO    oAudioTx;

    /* Instance to manage audio receive data */
    ADI_AD1980_DRIVER_DATA_PORT_INFO    oAudioRx;

} ADI_AD1980_DEF;

/*=============  D A T A  =============*/

/*
** AD1980 device instance(s)
*/
ADI_AD1980_DEF   gaoAD1980Device[] =
{
    /* AD1980 Device 0 */
    {
        /* SPORT 0 by default */
        0,
        /* SPORT DMA bus width - 16bit bus by default */
        2U,
        /* Flag ID undefined */
        ADI_FLAG_UNDEFINED,
        /* Device Manager Handle */
        NULL,
        /* Device Instance Handle */
        NULL,
        /* DMA Manager Handle */
        NULL,
        /* DCB Manager Handle */
        NULL,
        /* Device Manager Callback Function */
        NULL,
        /* SPORT Driver Handle */
        NULL,
        /* Critical region argument */
        NULL,
        /* AC'97 driver instance */
        NULL,
        /* Tx/Bi-directional port info */
        {
            /* AD1980 device instance number */
            0,
            /* Direction yet to be defined */
            ADI_DEV_DIRECTION_UNDEFINED
        },
        /* Rx port info */
        {
            /* AD1980 device instance number */
            0,
            /* Direction yet to be defined */
            ADI_DEV_DIRECTION_UNDEFINED
        },
    }
};

/*
**
**  Local Function Prototypes
**
*/

/* Opens AD1980 device */
static u32 adi_pdd_Open(
    ADI_DEV_MANAGER_HANDLE  hDeviceManager,
    u32                     DeviceNumber,
    ADI_DEV_DEVICE_HANDLE   hDevice,
    ADI_DEV_PDD_HANDLE      *pPDDHandle,
    ADI_DEV_DIRECTION       eDirection,
    void                    *pEnterCriticalArg,
    ADI_DMA_MANAGER_HANDLE  hDmaManager,
    ADI_DCB_HANDLE          hDcbManager,
    ADI_DCB_CALLBACK_FN     pfCallback
);

/* Closes a AD1980 device */
static u32 adi_pdd_Close(
    ADI_DEV_PDD_HANDLE      PDDHandle
);

/* Queues AD1980 ADC Interleaved buffer(s) */
static u32 adi_pdd_Read(
    ADI_DEV_PDD_HANDLE      PDDHandle,
    ADI_DEV_BUFFER_TYPE     BufferType,
    ADI_DEV_BUFFER          *pBuffer
);

/* Queues AD1980 DAC Interleaved buffer(s) */
static u32 adi_pdd_Write(
    ADI_DEV_PDD_HANDLE      PDDHandle,
    ADI_DEV_BUFFER_TYPE     BufferType,
    ADI_DEV_BUFFER          *pBuffer
);

/* Sequentially read/writes data to a device */
static u32 adi_pdd_SequentialIO(
    ADI_DEV_PDD_HANDLE      PDDHandle,
    ADI_DEV_BUFFER_TYPE     BufferType,
    ADI_DEV_BUFFER          *pBuffer
);

/* Sets or senses device specific parameter */
static u32 adi_pdd_Control(
    ADI_DEV_PDD_HANDLE      PDDHandle,
    u32                     Command,
    void                    *Value
);

/* Function to open and configure SPORT device connected to AD1980 */
static u32 adi_ad1980_SportConfig(
    ADI_AD1980_DEF          *poDevice
);

/* Callback function passed to SPORT driver */
static void adi_ad1980_SportCallback(
    void                    *phAd1980Driver,
    u32                     nEvent,
    void                    *pArgument
);

/* Callback function passed to AC'97 driver */
static void adi_ad1980_AC97Callback(
    void                    *phAd1980Driver,
    u32                     nEvent,
    void                    *pArgument
);

/* Processes AD1980 register access commands */
static u32 adi_ad1980_RegisterAccess (
    ADI_AD1980_DEF          *poDevice,
    u32                     nCommandID,
    void                    *pCommandValue,
    u8                      bUseLocalSemaphore
);

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)

/* Validates Physical Device Driver Handle  */
static u32 ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE      PDDHandle
);

#endif /* ADI_DEV_DEBUG */

/*
** AD1980 Driver Entry point for device manager
*/
ADI_DEV_PDD_ENTRY_POINT ADIAD1980EntryPoint =
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

        Opens a AD1980 device for use

    Parameters:
        hDeviceManager      - Device Manager handle
        DeviceNumber        - AD1980 Device number to open
        hDevice             - Device handle
        pPDDHandle          - pointer to PDD handle location
        eDirection          - data direction
        pEnterCriticalArg   - critical region storage location
        hDmaManager         - handle to the DMA manager
        hDcbManager         - callback handle
        pfCallback          - device manager callback function

    Return:
        ADI_DEV_RESULT_SUCCESS
            - Successfully opened Audio Device
        ADI_DEV_RESULT_BAD_DEVICE_NUMBER
            - Supplied device number is invalid
        ADI_DEV_RESULT_DEVICE_IN_USE
            - Supplied device number is already open and is in use
        Error code returned by Semaphore Service

*********************************************************************/
static u32 adi_pdd_Open(
    ADI_DEV_MANAGER_HANDLE  hDeviceManager,
    u32                     DeviceNumber,
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
    /* exit critical region parameter */
    void                                *pExitCriticalArg;
    /* pointer to the device we will be working on */
    ADI_AD1980_DEF                      *poDevice;
    /* Pointer to the device data port we will be working on */
    ADI_AD1980_DRIVER_DATA_PORT_INFO    *poDataPort;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* IF (Device Number is invlaid) */
    if (DeviceNumber >= ADI_AD1980_NUM_DEVICES)
    {
        /* report error (Invalid Device number) */
        nResult = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    }
    /* ELSE (Device number is valid) */
    else
    {

#endif /* ADI_DEV_DEBUG */

        /* insure the device the client wants is available  */
        nResult  = ADI_DEV_RESULT_DEVICE_IN_USE;
        /* AD1980 device to work on */
        poDevice = &gaoAD1980Device[DeviceNumber];
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

        /* Exit the critical region */
        adi_int_ExitCriticalRegion(pExitCriticalArg);

        /* IF (Successfully reserved AD1980 device for this client
               and/or created semaphore) */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* IF (Reset device instance) */
            if (bResetInstance == true)
            {
                /* Initialise device driver instance */
                /* Save the Device Manager handle */
                poDevice->hDeviceManager    = hDeviceManager;
                /* Save the Device handle */
                poDevice->hDevice           = hDevice;
                /* Save the DMA handle */
                poDevice->hDmaManager       = hDmaManager;
                /* Save the DCB handle */
                poDevice->hDcbManager       = hDcbManager;
                /* Save the Device Manager callback function handle */
                poDevice->pfCallback        = pfCallback;
                /* Pointer to critical region                   */
                poDevice->pEnterCriticalArg = pEnterCriticalArg;
                /* Clear the SPORT handle */
                poDevice->hSport            = NULL;
                /* Clear AC'97 driver instance address */
                poDevice->pAC97             = NULL;
                /* Invalidate Reset Flag ID */
                poDevice->eResetFlag        = ADI_FLAG_UNDEFINED;
                /* Use SPORT 0 */
                poDevice->nSportDevNumber   = 0U;
                /* 16-bit SPORT DMA */
                poDevice->nSportDmaBusWidth = 2U;
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

        Closes down a AD1980 device data port and/or instance

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
    /* Return value - assume we're going to be successful   */
    u32                                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we will be working on */
    ADI_AD1980_DEF                      *poDevice;
    /* Pointer to the audio data port we're working on */
    ADI_AD1980_DRIVER_DATA_PORT_INFO    *poDataPort = (ADI_AD1980_DRIVER_DATA_PORT_INFO *)hPhysicalDevice;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given PDDHandle */
    nResult = ValidatePDDHandle(hPhysicalDevice);

    /* Continue only if the given PDDHandle is valid */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {

#endif /* ADI_DEV_DEBUG */

        /* Get the address of device instance to work on */
        poDevice = &gaoAD1980Device[poDataPort->nAudioDevNumber];

        /* IF (Device opened for bi-directional mode) */
        if (poDataPort->eDirection == ADI_DEV_DIRECTION_BIDIRECTIONAL)
        {
            /* Mark both Tx and Rx port direction as undefined */
            poDevice->oAudioTx.eDirection = ADI_DEV_DIRECTION_UNDEFINED;
            poDevice->oAudioRx.eDirection = ADI_DEV_DIRECTION_UNDEFINED;
        }
        /* ELSE (Input and Output dataflow must be operated independently) */
        else
        {
            /* Mark the data port direction as undefined */
            poDataPort->eDirection = ADI_DEV_DIRECTION_UNDEFINED;
        }

        /* IF (Direction of both data ports is undefined) */
        if ((poDevice->oAudioTx.eDirection == ADI_DEV_DIRECTION_UNDEFINED) &&\
            (poDevice->oAudioRx.eDirection == ADI_DEV_DIRECTION_UNDEFINED))
        {
            /* IF (SPORT device is open) */
            if (poDevice->hSport)
            {
                /* Close the SPORT device */
                nResult = adi_dev_Close(poDevice->hSport);
                poDevice->hSport = NULL;
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

        Queues AD1980 ADC buffer(s)

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

    /* Return value - assume we're going to be successful   */
    u32                                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the audio data port we're working on */
    ADI_AD1980_DRIVER_DATA_PORT_INFO    *poDataPort = (ADI_AD1980_DRIVER_DATA_PORT_INFO *)hPhysicalDevice;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given PDDHandle */
    nResult = ValidatePDDHandle(hPhysicalDevice);

    /* IF (Given PDD handle is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {

#endif

        /* IF (AC'97 instance is valid) */
        if (gaoAD1980Device[poDataPort->nAudioDevNumber].pAC97 != NULL)
        {
            /* pass the read buffer to AC'97 driver */
            nResult = adi_ac97_Read(gaoAD1980Device[poDataPort->nAudioDevNumber].pAC97,
                                    eBufferType,
                                    pBuffer);
        }
        /* ELSE (AC'97 instance is not yet to be initialised) */
        else
        {
            /* report failure (AC'97 instance invalid) */
            nResult = ADI_AD1980_RESULT_AC97_INSTANCE_INVALID;
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(nResult);
}

/*********************************************************************

    Function: adi_pdd_Write

        Queues AD1980 DAC buffer(s)

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
    /* Return value - assume we're going to be successful   */
    u32                                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the audio data port we're working on */
    ADI_AD1980_DRIVER_DATA_PORT_INFO    *poDataPort = (ADI_AD1980_DRIVER_DATA_PORT_INFO *)hPhysicalDevice;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given PDDHandle */
    nResult = ValidatePDDHandle(hPhysicalDevice);

    /* IF (Given PDD handle is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {

#endif

        /* IF (AC'97 instance is valid) */
        if (gaoAD1980Device[poDataPort->nAudioDevNumber].pAC97 != NULL)
        {
            /* pass the write buffer to AC'97 driver */
            nResult = adi_ac97_Write(gaoAD1980Device[poDataPort->nAudioDevNumber].pAC97,
                                     eBufferType,
                                     pBuffer);
        }
        /* ELSE (AC'97 instance is not yet to be initialised) */
        else
        {
            /* report failure (AC'97 instance invalid) */
            nResult = ADI_AD1980_RESULT_AC97_INSTANCE_INVALID;
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    return(nResult);
}

/*********************************************************************

    Function: adi_pdd_SequentialIO

        Sequentially read/writes data to a device
        AD1980 driver does not support this function

    Parameters:
        hPhysicalDevice - Physical Device Handle of the device to work on
        eBufferType     - Buffer type
        pBuffer         - Pointer to data buffer

    Returns:
        ADI_DEV_RESULT_NOT_SUPPORTED
            - Function not supported by AD1980 driver

*********************************************************************/
static u32 adi_pdd_SequentialIO(
    ADI_DEV_PDD_HANDLE      hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE     eBufferType,
    ADI_DEV_BUFFER          *pBuffer
)
{
    /* this function is not supported by AD1980 driver */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************

    Function: adi_pdd_Control

        Senses or Configures AD1980 device registers

    Parameters:
        hPhysicalDevice - Physical Device Handle of the device to work on
        nCommand        - Command ID
        Value           - Command specific value

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully processed the given command
        DD/SS or AD1980 specific error code

*********************************************************************/
static u32 adi_pdd_Control(
    ADI_DEV_PDD_HANDLE      hPhysicalDevice,
    u32                     nCommand,
    void                    *Value
)
{
    /* u8 type to avoid casts/warnings etc.    */
    u8                                  u8Value;
    /* u32 type to avoid casts/warnings etc.    */
    u32                                 u32Value;
    /* Return value - assume we're going to be successful   */
    u32                                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we will be working on */
    ADI_AD1980_DEF                      *poDevice;
    /* Pointer to AD1980 Driver init instance */
    ADI_AD1980_INIT_DRIVER              *pInit;
    /* Pointer to the audio data port we're working on */
    ADI_AD1980_DRIVER_DATA_PORT_INFO    *poDataPort = (ADI_AD1980_DRIVER_DATA_PORT_INFO *)hPhysicalDevice;

    /* assign 8 & 32 bit values for the Value argument */
    u32Value = (u32)Value;
    u8Value = (u8)u32Value;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    /* Validate the given PDDHandle */
    nResult = ValidatePDDHandle(hPhysicalDevice);

    /* Continue only if the given PDDHandle is valid */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* Get the address of device instance to work on */
        poDevice = &gaoAD1980Device[poDataPort->nAudioDevNumber];

        /* CASEOF (Command ID)  */
        switch (nCommand)
        {
            /* CASE: Control dataflow */
            case (ADI_DEV_CMD_SET_DATAFLOW):
            /* CASE: Set Dataflow method */
            case (ADI_DEV_CMD_SET_DATAFLOW_METHOD):
                /* IF (AC'97 instance is invalid) */
                if (poDevice->pAC97 == NULL)
                {
                    /* return error (AC'97 instance is invalid) */
                    nResult = ADI_AD1980_RESULT_AC97_INSTANCE_INVALID;
                }
                /* ELSE (AC'97 instance is valid) */
                else
                {
                    /* IF (Data port opened for inbound dataflow) */
                    if (poDataPort->eDirection == ADI_DEV_DIRECTION_INBOUND)
                    {
                        /* IF (Command is to update dataflow status) */
                        if (nCommand == ADI_DEV_CMD_SET_DATAFLOW)
                        {
                            /* Change command to AC'97 specific command to
                               update Rx dataflow for this device */
                            nCommand = ADI_AC97_CMD_SET_RECEIVE_DATAFLOW;
                        }
                        /* ELSE (Command must be to set dataflow method) */
                        else
                        {
                            /* Change command to AC'97 specific command to
                               update Rx dataflow method for this device */
                            nCommand = ADI_AC97_CMD_SET_RECEIVE_DATAFLOW_METHOD;
                        }
                    }
                    /* ELSE IF (Data port opened for outbound dataflow) */
                    else if (poDataPort->eDirection == ADI_DEV_DIRECTION_OUTBOUND)
                    {
                        /* IF (Command is to update dataflow status) */
                        if (nCommand == ADI_DEV_CMD_SET_DATAFLOW)
                        {
                            /* Change command to AC'97 specific command to
                               update Tx dataflow for this device */
                            nCommand = ADI_AC97_CMD_SET_TRANSMIT_DATAFLOW;
                        }
                        /* ELSE (Command must be to set dataflow method) */
                        else
                        {
                            /* Change command to AC'97 specific command to
                               update Tx dataflow method for this device */
                            nCommand = ADI_AC97_CMD_SET_TRANSMIT_DATAFLOW_METHOD;
                        }
                    }
                    /* ELSE (Device is opened for bi-directional dataflow) */
                        /* Pass the command to AC'97 library with out any change */

                    /* Pass command to AC'97 library) */
                    nResult = adi_ac97_Control(poDevice->pAC97,
                                               nCommand,
                                               Value);

                }
                break;

            /* CASE (Initialise AD1980 driver) */
            case (ADI_AD1980_CMD_INIT_DRIVER):

                /* Address of AD1980 driver init structure */
                pInit = (ADI_AD1980_INIT_DRIVER *) Value;

                /* Initialise AC'97 Driver instance */
                nResult = adi_ac97_Init_Instance (Value);

                /* IF (Successfully initialised AC'97 driver instance) */
                if (nResult == ADI_DEV_RESULT_SUCCESS)
                {
                    /* IF (AC'97 Instance was already initialised) */
                    if (poDevice->pAC97 != NULL)
                    {
                        /* Close the SPORT device that is open */
                        nResult = adi_dev_Close(poDevice->hSport);
                    }
                }

                /* IF (Successfully closed the SPORT device opened previously) */
                if (nResult == ADI_DEV_RESULT_SUCCESS)
                {
                    /* save AC'97 driver instance handle */
                    poDevice->pAC97                     = pInit->pAC97;
                    /* AD1980 driver Callback function supplied to the AC'97 driver instance */
                    poDevice->pAC97->CodecCallback      = adi_ad1980_AC97Callback;
                    /* pointer to critical region */
                    poDevice->pAC97->pEnterCriticalArg  = poDevice->pEnterCriticalArg;
                    /* Save the SPORT Device Number */
                    poDevice->nSportDevNumber           = pInit->SportDevNumber;
                    /* Save the Reset Flag ID */
                    poDevice->eResetFlag                = pInit->ResetFlagId;

                    /* Open and Configure SPORT Device connected to AD1980 */
                    nResult = adi_ad1980_SportConfig (poDevice);
                }
                break;

            /* CASE (Update SPORT DMA Bus Width) */
            case (ADI_AC97_CMD_UPDATE_SPORT_DMA_BUS_WIDTH):

                /* IF (The DMA bus width is valid) */
                if ((u32)Value != 0)
                {
                    /* IF (we already have a SPORT device open) */
                    if (poDevice->hSport != NULL)
                    {
                        /* Close the SPORT device that is open */
                        nResult = adi_dev_Close(poDevice->hSport);

                        /* IF (Successfully closed SPORT device) */
                        if (nResult == ADI_DEV_RESULT_SUCCESS)
                        {
                            poDevice->hSport = NULL;
                            /* Update AC'97 instance with new DMA bus width */
                            nResult = adi_ac97_Control(poDevice->pAC97, nCommand, Value);
                        }

                        /* IF (Successfully updated AC'97 instance) */
                        if (nResult == ADI_DEV_RESULT_SUCCESS)
                        {
                            /* Open and Configure SPORT Device connected to AD1980 */
                            nResult = adi_ad1980_SportConfig (poDevice);
                        }
                    }
                    /* ELSE (AC'97 instance is yet to be initialised) */
                    else
                    {
                        /* Save the SPORT DMA bus width */
                        poDevice->nSportDmaBusWidth = (u32)Value;
                    }
                }
                break;

            /* CASE: Set Deferred Callback or Live callback to process AC'97 frames */
            case (ADI_AD1980_CMD_USE_DCB_TO_PROCESS_FRAMES):
                poDevice->hDcbManager = (ADI_DCB_HANDLE)Value;
                break;

            /* CASE: query for processor DMA support */
            case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):

                /* AD1980 doesn't support DMA, but supports indirectly via SPORT */
                *((u32 *)Value) = false;
                break;

            /* other commands - requires a valid AC'97 instance */
            default:
                /* IF (AC'97 instance is invalid) */
                if (poDevice->pAC97 == NULL)
                {
                    /* return error (AC'97 instance is invalid) */
                    nResult = ADI_AD1980_RESULT_AC97_INSTANCE_INVALID;
                }
                /* ELSE (AC'97 instance is valid) */
                else
                {
                    /* Pass this to register access control function
                       assuming this to be a register access command
                       from the upper level layer calling this driver */
                    nResult = adi_ad1980_RegisterAccess(poDevice, nCommand, Value, false);

                    /* IF (Command not supported by register access function) */
                    if (nResult == ADI_AD1980_RESULT_CMD_NOT_SUPPORTED)
                    {
                        /* try passing this command to AC'97 driver */
                        nResult = adi_ac97_Control(poDevice->pAC97, nCommand, Value);
                    }

                    /* IF (Command not supported by AC'97 driver) */
                    if (nResult == ADI_AC97_RESULT_CMD_NOT_SUPPORTED)
                    {
                        /* IF (SPORT driver is already open) */
                        if (poDevice->hSport != NULL)
                        {
                            /* try passing this command to SPORT */
                            nResult = adi_dev_Control(poDevice->hSport, nCommand, Value);
                        }

                    } /* End of if (Command not supported by AC'97 driver) */

                } /* End of if (AC'97 instance is invalid) */
                break;

        } /* End of switch (Command ID) cases */

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return(nResult);
}

/*********************************************************************

    Function: adi_ad1980_RegisterAccess

        Processes AD1980 register access commands

    Parameters:
        poDevice            - Pointer to AD1980 driver instance to work on
        nCommandID          - Command ID to process
        pCommandValue       - Command specific value
        bUseLocalSemaphore  - 'true' when call register access commands are
                               issued by a function, local to the driver,
                               that handles common Audio device commands
                              'false' when register access commands are
                              issued directly from the application

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully passed register access request to AC'97 driver
        ADI_AD1980_RESULT_CMD_NOT_SUPPORTED
            - Command ID not supported by this function
        ADI_AC97_RESULT_ADC_SAMPLE_RATE_NOT_SUPPORTED
            - Given ADC sample rate is not supported by the device/driver
        ADI_AC97_RESULT_DAC_SAMPLE_RATE_NOT_SUPPORTED
            - Given DAC Sample rate is not supported by the device/driver

*********************************************************************/
static u32 adi_ad1980_RegisterAccess (
    ADI_AD1980_DEF      *poDevice,
    u32                 nCommandID,
    void                *pCommandValue,
    u8                  bUseLocalSemaphore
)
{
    /* default return code - assume we're going to be successful */
    u32                         nResult = ADI_DEV_RESULT_SUCCESS;
    /* exit critical region parameter                       */
    void                        *pExitCriticalArg;
    /* Pointer to register access table */
    ADI_DEV_ACCESS_REGISTER     *poSelective;

    /* IF (Command is to access device register(s)) */
    if ((nCommandID >= ADI_DEV_CMD_REGISTER_READ) &&\
         (nCommandID <= ADI_DEV_CMD_REGISTER_BLOCK_WRITE))
    {
        /* IF (Command is to access a block of registers) */
        if ((nCommandID == ADI_DEV_CMD_REGISTER_BLOCK_READ) ||\
            (nCommandID == ADI_DEV_CMD_REGISTER_BLOCK_WRITE))
        {
            /* Block register access commands not supported by this driver */
            /* report error (command not supported) */
            nResult = ADI_AD1980_RESULT_CMD_NOT_SUPPORTED;
        }
        /* ELSE (Command other than block register access) */
        else
        {
            /* Protect this section of code - entering a critical region    */
            pExitCriticalArg = adi_int_EnterCriticalRegion(poDevice->pEnterCriticalArg);
            /* IF (register access is already in progress) */
            if (poDevice->pAC97->RegAccess.Cmd != 0)
            {
                /* Cmd field in AC'97 RegAccess structure is non-zero */
                /* means AC'97 is already processing a register access request */
                /* report error (Register access already in use) */
                nResult = ADI_AD1980_RESULT_REG_ACCESS_ALREADY_IN_USE;
            }
            /* ELSE (Register access is not in use) */
            else
            {
                /* Save the Command ID to AC'97 Register Access cmd field */
                poDevice->pAC97->RegAccess.Cmd = nCommandID;
            }
            /* Exit the critical region */
            adi_int_ExitCriticalRegion(pExitCriticalArg);

            /* IF (Register access is available for use) */
            if (nResult == ADI_DEV_RESULT_SUCCESS)
            {
                /* IF (command is to write to a register or
                                     write to a table of registers) */
                if ((nCommandID == ADI_DEV_CMD_REGISTER_WRITE) ||\
                    (nCommandID == ADI_DEV_CMD_REGISTER_TABLE_WRITE))
                {
                    /* Get address of given register access table */
                    poSelective = (ADI_DEV_ACCESS_REGISTER *)pCommandValue;

                    /* continue until we reach the end of register access table */
                    while (poSelective->Address != ADI_DEV_REGEND)
                    {
                        /* validate this register value */
                        nResult = adi_ac97_ValidateRegVal(poDevice->pAC97,
                                                          poSelective->Address,
                                                          poSelective->Data);

                        /* IF (register data is in valid (or)
                               configure a single register) */
                        if ((nResult != ADI_DEV_RESULT_SUCCESS) ||\
                            (nCommandID == ADI_DEV_CMD_REGISTER_WRITE))
                        {
                            /* exit this loop */
                            break;
                        }

                        /* move to next register access pair */
                        poSelective++;

                    } /* End of while (reached end of register access table) */

                } /* End of if (Command to write to register(s)) */

                /* IF (given Register data is valid) */
                if (nResult == ADI_DEV_RESULT_SUCCESS)
                {
                    /* initialise the AC97 register access info table */
                    poDevice->pAC97->RegAccess.pTable  = pCommandValue;
                    poDevice->pAC97->RegAccess.pAccess = (u16*)pCommandValue;

                    /* IF (write to AD1980 register(s)) */
                    if ((nCommandID >= ADI_DEV_CMD_REGISTER_WRITE) &&\
                        (nCommandID <= ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE))
                    {
                        /* Update AC'97 register access mode as to write to AD1980 */
                        poDevice->pAC97->RegAccess.Mode = ADI_AC97_CODEC_REG_ACCESS_WRITE;
                    }
                    /* ELSE (read AD1980 register(s) */
                    else
                    {
                        /* Update AC'97 register access mode as to read from AD1980 */
                        poDevice->pAC97->RegAccess.Mode = ADI_AC97_CODEC_REG_ACCESS_READ;
                    }

                } /* End of if (given Register data is valid) */

            } /* End of if (Register access is available for use) */

        } /* End of if (Command is to access a block of registers) else case */
    }
    /* ELSE (This is not a valid register access command) */
    else
    {
        /* report error (Command not supported) */
        nResult = ADI_AD1980_RESULT_CMD_NOT_SUPPORTED;
    }

    /* return */
    return(nResult);

}

/*********************************************************************

    Function: adi_ad1980_SportConfig

        Opens and Configures SPORT device allocated to AD1980 Audio Codec

    Parameters:
        poDevice         - Pointer to AD1980 device instance to work on

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully opened and configured SPORT device
        Error code returned from SPORT driver

*********************************************************************/
static u32 adi_ad1980_SportConfig(
    ADI_AD1980_DEF          *poDevice
)
{

    /* default return code */
    u32     nResult = ADI_DEV_RESULT_SUCCESS;

    /* SPORT Configuration Table */
    ADI_DEV_CMD_VALUE_PAIR  aoSportConfigTable[] =
    {
        /* clear any previous Tx errors */
        { ADI_SPORT_CMD_CLEAR_TX_ERRORS,    (void *)NULL                    },
        /* clear any previous Rx errors */
        { ADI_SPORT_CMD_CLEAR_RX_ERRORS,    (void *)NULL                    },
        /* SPORT in Circular mode       */
        { ADI_DEV_CMD_SET_DATAFLOW_METHOD,  (void *)ADI_DEV_MODE_CIRCULAR   },
        /* Enable Streaming mode        */
        { ADI_DEV_CMD_SET_STREAMING,        (void *)true                    },
        /* External Frame sync, MSB first, No TFS required  */
        { ADI_SPORT_CMD_SET_TCR1,           (void *)0                       },
        /* Secondary disabled   */
        { ADI_SPORT_CMD_SET_TCR2,           (void *)0                       },
        /* Tx Wordlength = 16 bits  */
        { ADI_SPORT_CMD_SET_TX_WORD_LENGTH, (void *)15                      },
        /* Internal RFS, Require RFS for every data */
        { ADI_SPORT_CMD_SET_RCR1,           (void *)0x0600                  },
        /* Secondary disabled   */
        { ADI_SPORT_CMD_SET_RCR2,           (void *)0                       },
        /* Rx Wordlength = 16 bits  */
        { ADI_SPORT_CMD_SET_RX_WORD_LENGTH, (void *)15                      },
        /* sets RFS to 48kHz (BIT_CLK for AC'97 = 12.288MHz) */
        { ADI_SPORT_CMD_SET_RFSDIV,         (void *)0xFF                    },
        /* MFD = 1, Enable Multi Channel, Enable Tx/Rx DMA packing */
        { ADI_SPORT_CMD_SET_MCMC2,          (void *)0x101C                  },
        /* 16 Channels */
        { ADI_SPORT_CMD_SET_MTCS0,          (void *)0xFFFF                  },
        /* 16 Channels */
        { ADI_SPORT_CMD_SET_MRCS0,          (void *)0xFFFF                  },
        /* Window Size = ((16 Channels/8) - 1) = 1 */
        { ADI_SPORT_CMD_SET_MCMC1,          (void *)0x1000                  },
        /* Enable error reporting */
        { ADI_DEV_CMD_SET_ERROR_REPORTING,  (void *)true                    },
        /* End of Configuration table */
        { ADI_DEV_CMD_END,                  (void *)NULL                    },
    };

    /* IF (Reset Flag ID is valid) */
    if (poDevice->eResetFlag != ADI_FLAG_UNDEFINED)
    {
        /* Issue a Hardware reset to AD1980 */
        nResult = adi_ac97_HwReset(poDevice->pAC97, poDevice->eResetFlag);
    }
    /* ELSE (Invalid reset flag) */
    else
    {
        /* Report failure (Invalid Reset Flag) */
        nResult = ADI_AD1980_RESULT_RESET_FLAG_ID_INVALID;
    }

    /* IF (Successfully Reset AD1980) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Open SPORT Device connected to AD1980 */
        nResult = adi_dev_Open(poDevice->hDeviceManager,
                               &ADISPORTEntryPoint,
                               poDevice->nSportDevNumber,
                               poDevice,
                               &poDevice->hSport,
                               ADI_DEV_DIRECTION_BIDIRECTIONAL,
                               poDevice->hDmaManager,
                               NULL,
                               adi_ad1980_SportCallback);
    }

    /* IF (Successfully opened SPORT device) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Configure SPORT Device */
        nResult = adi_dev_Control(poDevice->hSport,
                                  ADI_DEV_CMD_TABLE,
                                  (void *)&aoSportConfigTable[0]);
    }

    /* IF (Successfully Configured the SPORT driver) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Submit outbound buffer */
        nResult = adi_dev_Write (poDevice->hSport,
                                 ADI_DEV_CIRC,
                                 (ADI_DEV_BUFFER *)(&poDevice->pAC97->DacCircBuf));
    }

    /* IF (Successfully Submitted outbound buffer) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Submit inbound buffer */
        nResult = adi_dev_Read (poDevice->hSport,
                                ADI_DEV_CIRC,
                                (ADI_DEV_BUFFER *)(&poDevice->pAC97->AdcCircBuf));
    }

    /* IF (Successfully Opened and Configured SPORT Device) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Enable SPORT dataflow - this enables AC-Link to AD1980 */
        nResult = adi_dev_Control(poDevice->hSport,
                                  ADI_DEV_CMD_SET_DATAFLOW,
                                  (void *)true);
    }

    return (nResult);
}

/*********************************************************************

    Function:  adi_ad1980_SportCallback

        Callback from the SPORT Driver

    Parameters:
        phAd1980Driver  - Callback Handle supplied by the AD1980 driver
        nEvent          - Callback Event
        pArgument       - Callback Argument

    Returns:
        None

*********************************************************************/
static void adi_ad1980_SportCallback(
    void        *phAd1980Driver,
    u32         nEvent,
    void        *pArgument
)
{
    /* Pointer to AD1980 device instance */
    ADI_AD1980_DEF  *poDevice = (ADI_AD1980_DEF *)phAd1980Driver;

    /* CASEOF (callback event) */
    switch(nEvent)
    {
        /* CASE (sub-buffer complete */
        case ADI_DEV_EVENT_SUB_BUFFER_PROCESSED:

            /* IF (Deferred callback enabled) */
            if (poDevice->hDcbManager)
            {
                /* Post this information to DCB queue for AC'97 callback */
                adi_dcb_Post(poDevice->hDcbManager,
                             0,
                             poDevice->pAC97->AC97Callback,
                             (void *)poDevice->pAC97,
                             nEvent,
                             pArgument);
            }
            /* ELSE (Callbacks are live) */
            else
            {
                /* Pass the information to AC'97 driver */
                (poDevice->pAC97->AC97Callback) ((void *)poDevice->pAC97, nEvent, pArgument);
            }
            break;

        default:
            /* Pass the callback to the Device Manager Callback */
            (poDevice->pfCallback)(poDevice->hDevice, nEvent, pArgument);
            break;
    }

    /* return */

}

/*********************************************************************

    Function:  adi_ad1980_AC97Callback

        Callback from the AC'97 Driver

    Parameters:
        phAd1980Driver  - Callback Handle supplied by the AD1980 driver
        nEvent          - Callback Event
        pArgument       - Callback Argument

    Returns:
        None

*********************************************************************/
static void adi_ad1980_AC97Callback(
    void        *phAd1980Driver,
    u32         nEvent,
    void        *pArgument
)
{

    /* Pointer to AD1980 device instance */
    ADI_AC97_DRIVER_INSTANCE    *pAC97 = (ADI_AC97_DRIVER_INSTANCE *)phAd1980Driver;
    /* Index to AD1980 driver instance */
    u32                         nIndex;

    /* Find AD1980 device instance to which this AC'97 driver instance belong */
    for (nIndex = 0;
         nIndex < sizeof(gaoAD1980Device) / sizeof(gaoAD1980Device[0]);
         nIndex++)
    {
        /* IF (Found a device instance to which the given AC'97 instance belongs) */
        if (gaoAD1980Device[nIndex].pAC97 == pAC97)
        {
            /* Pass the event to Device Manager callback */
            (gaoAD1980Device[nIndex].pfCallback)(gaoAD1980Device[nIndex].hDevice,
                                                 nEvent,
                                                 pArgument);

        } /* End of if (Found a device instance to which
                        the given AC'97 instance belongs) */

    } /* End of for (Find AD1980 device instance to which
                     the given AC'97 driver instance belongs) */

}

#if defined(ADI_DEV_DEBUG)  /* Debug build only */
/*********************************************************************

    Function:       ValidatePDDHandle

    Description:    Validates a PDD handle

*********************************************************************/

static u32 ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of a EPPI device  */
)
{
    /* Return value - insure the client has provided a valid PDD Handle */
    u32     nResult = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    /* index */
    u32     i;

    /* compare the given PDD handle with PDDHandle allocated to all AD1980 devices in the list    */
    for (i = 0; i < ADI_AD1980_NUM_DEVICES; i++)
    {
        if ((PDDHandle == (ADI_DEV_PDD_HANDLE)&gaoAD1980Device[i].oAudioTx) ||\
            (PDDHandle == (ADI_DEV_PDD_HANDLE)&gaoAD1980Device[i].oAudioRx))
        {
            /* Given PDDHandle is valid. quit this loop */
            nResult = ADI_DEV_RESULT_SUCCESS;
            break;
        }
    }
    /* return */
    return (nResult);
}

#endif

/*****/

/*
**
** EOF:
**
*/

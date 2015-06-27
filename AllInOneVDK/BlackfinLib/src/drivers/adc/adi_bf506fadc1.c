/* $Revision: 3492 $
 * $Date: 2010-10-07 09:34:20 -0400 (Thu, 07 Oct 2010) $ 
******************************************************************************
Copyright (c), 2009 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title: ADI ADI Device Drivers - BF506F ADC

Description:

    This is the primary source file for ADSP-BF506F ADC driver

*****************************************************************************/

/*=============  I N C L U D E S   =============*/

/* System Service includes  */
#include <services/services.h>
/* device manager includes */
#include <drivers/adi_dev.h>
/* SPORT driver includes */
#include <drivers/sport/adi_sport.h>
/* ADSP-BF506F ADC driver includes */
#include <drivers/adc/adi_bf506fadc1.h>

/*=============  D E F I N E S  =============*/

/* Number of ADSP-BF506F ADC devices in the system  */
#define ADI_BF506ADC1_NUM_DEVICES       (sizeof(goaBf506fadc1Device)/sizeof(ADI_BF506ADC1_DEF))

/* Default ACLK Freq (20MHz) */
#define BF506ADC1_DEFAULT_ACLK          (20000000U)

/*
** SPORT Rx Control Register 1 configuration value to use when
** BF506F ADC is controlled by ACM
** External Rx clock, zero fill, MSB first, External RX frame sync (RFS) select,
** Requires RFS for every data word, Active low RFS, Late RFS,
** Drive convertor on falling edge of RSCLK and sample data (into SPORT) on rising edge
*/
#define ADI_SPORT_RCR1_ACM_CTRL         0x7400U

/*
** SPORT Rx Control Register 1 configuration value to use when
** BF506F ADC is controlled by GPIO Flag pins
** Internal Rx clock, zero fill, MSB first, Internal RX frame sync (RFS) select,
** Requires RFS for every data word, Active low RFS, Late RFS,
** Drive convertor on falling edge of RSCLK and sample data (into SPORT) on rising edge
*/
#define ADI_SPORT_RCR1_GPIO_CTRL        0x7602U

/* Num ACLKs to issue partial power-down */
#define ADC_PARTIAL_POWER_DN_NUM_ACLK   14U
/* Num ACLKs to issue full power-down */
#define ADC_FULL_POWER_DN_NUM_ACLK      32U
/* Num ACLKs to power-up ADC */
#define ADC_POWER_UP_NUM_ACLK           32U
/* Num SPORT word-length to power down ADC (8bits) */
#define ADC_POWER_DN_NUM_SLEN           7U
/* Num SPORT word-length to power up ADC (14bits) */
#define ADC_POWER_UP_NUM_SLEN           13U

/*
** SPORT word length (actual wordlength - 1)
*/
/* 8 bits to power-down ADC */
#define ADI_SPORT_SLEN_PDN_8            7U
/* 14 bits */
#define ADI_SPORT_SLEN_14               13U
/* 16 bits */
#define ADI_SPORT_SLEN_16               15U
/* 32 bits */
#define ADI_SPORT_SLEN_32               31U

/* Power-up/Power-down RFS Frequency (100kHz) */
#define ADC_POWER_RFS_FREQ              100000
#define ten2powr8                       100000000
/* min quiet period, teps = t2 + t8 + tquiet ~70ns */
#define tepsmin                         7

/*==============  D A T A   S T R U C T U R E S  ===============*/

/* BF506FADC1 Driver instance structure */
typedef struct __AdiBf506fadc1Def
{
    /* 'true' when device is already in use */
    bool                            bIsDevInUse;

    /* 'true' when dataflow is already enabled */
    bool                            bIsDataflowEnabled;

    /* ADC Control select - 'true' to use ACM, 'false' to use Flag pins */
    bool                            bUseAcm;

    /* ACM Device number to use */
    u8                              nAcmDevNumber;

    /* SPORT Device number to use */
    u8                              nSportDevNumber;

    /* SPORT RCLKDIV value to calculate operating mode delays */
    u16                             nSportRclkDiv;

    /* ADC data mode */
    ADI_BF506FADC1_DATA_MODE        eCurDataMode;

    /* ADC Operating mode */
    ADI_BF506FADC1_OP_MODE          eCurOpMode;

    /* Instance to hold Flag pins connected to ADC Control */
    ADI_BF506FADC1_CTRL_FLAG_PINS   oFlagPins;

    /* Instance to hold value of Flag pins (ADC control signals) */
    ADI_BF506FADC1_CTRL_FLAG_STATE  oFlagState;

    /* Sample Rate / ACLK Frequency */
    u32                             nCurSampleRateAclk;

    /* ACM Device Handle */
    ADI_ACM_DEV_HANDLE              hAcm;

    /* SPORT Device Handle */
    ADI_DEV_DEVICE_HANDLE           hSport;

    /* Device Manager Handle */
    ADI_DEV_MANAGER_HANDLE          hDevManager;

    /* Device instance handle provided by Device Manager*/
    ADI_DEV_DEVICE_HANDLE           hDevice;

    /* DMA Manager Handle */
    ADI_DMA_MANAGER_HANDLE          hDmaManager;

    /* DCB Manager Handle */
    ADI_DCB_HANDLE                  hDcbManager;

    /* Application / Device Manager callback function */
    ADI_DCB_CALLBACK_FN             pfCallback;

    /* Pointer to critical region */
    void                            *pEnterCriticalArg;

} ADI_BF506ADC1_DEF;

/*=============  D A T A  =============*/

/* Create BF506ADC1 device instance(s) */
static ADI_BF506ADC1_DEF   goaBf506fadc1Device[] =
{
    /* BF506ADC1 Device 0 */
    {
        /* Device Not in use */
        false,
        /* Dataflow disabled by default */
        false,
        /* Use ACM by default */
        true,
        /* ACM 0 */
        0,
        /* SPORT 0 */
        0,
        /* RCLKDIV */
        0,
        /* Data Mode - Receive Data only from Data port A
                       with no extended zeros (14-bits) */
        ADI_BF506FADC1_DATA_DOUT_A_ONLY,
        /* Operating mode - Power-down */
        ADI_BF506ADC1_MODE_POWERDOWN,
        /* No Flag pins, use ACM instead as default */
        {
            ADI_FLAG_UNDEFINED,
            ADI_FLAG_UNDEFINED,
            ADI_FLAG_UNDEFINED,
            ADI_FLAG_UNDEFINED,
            ADI_FLAG_UNDEFINED,
        },
        /* Flag state - all cleared */
        {
            false,
            false,
            false,
            false,
            false,
        },
        /* Default ACLK Frequency */
        BF506ADC1_DEFAULT_ACLK,
        /* Device Handles */
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
};

/*
**
**  Local Function Prototypes
**
*/

/* Opens BF506ADC1 for specified data direction */
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

/* Closes a BF506ADC1 device */
static u32 adi_pdd_Close(
    ADI_DEV_PDD_HANDLE                  hPhysicalDevice
);

/* Queues Inbound buffers to BF506ADC1 SPORT */
static u32 adi_pdd_Read(
    ADI_DEV_PDD_HANDLE                  hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE                 eBufferType,
    ADI_DEV_BUFFER                      *pBuffer
);

/* Queues Outbound buffers to SPORT - Function not supported by BF506ADC1 */
static u32 adi_pdd_Write(
    ADI_DEV_PDD_HANDLE                  hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE                 eBufferType,
    ADI_DEV_BUFFER                      *pBuffer
);

/* Sequentially read/writes data to a device
   Function not supported by BF506ADC1 */
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

/* Opens ACM device for ADC use */
static u32 Bf506fadc1_OpenAcm (
    ADI_BF506ADC1_DEF                   *poDevice
);

/* Updates ACM configuration for ADC use */
static u32 Bf506fadc1_UpdateAcmConfig (
    ADI_BF506ADC1_DEF                   *poDevice
);

/* Opens the SPORT device */
static u32 Bf506fadc1_OpenSport (
    ADI_BF506ADC1_DEF                   *poDevice
);

/* Updates SPORT register configuration */
static u32 Bf506fadc1_UpdateSportConfig (
    ADI_BF506ADC1_DEF                   *poDevice
);

/* Opens/Closes ADC Control device */
static u32 Bf506fadc1_OpenAdcCtrlDev (
    ADI_BF506ADC1_DEF                   *poDevice,
    bool                                bOpen
);

/* Configures selected GPIO pin for ADC control */
static u32 Bf506fadc1_ConfigFlag (
    ADI_FLAG_ID                         eFlagID,
    bool                                bState
);

/* Apply ADC operating mode */
static u32 Bf506fadc1_ApplyOpMode (
    ADI_BF506ADC1_DEF                   *poDevice,
    ADI_BF506FADC1_OP_MODE              eNewOpMode
);

/* Applies the given Sample Rate/ACLK Freq */
static u32 Bf506fadc1_ApplySampleRateAclk (
    ADI_BF506ADC1_DEF                   *poDevice,
    bool                                bUseAcm,
    u32                                 nNewSampleRateAclk
);

/* Callback passed to SPORT device */
static void Bf506fadc1_SportCallback (
    void                                *hAdcDevice,
    u32                                 nEvent,
    void                                *pArg
);

/* Calculates SPORT word length based on current data mode */
static void GetSportWordLength (
    ADI_BF506ADC1_DEF   *poDevice,
    u32                 *pWordLength
);

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)

/* Validates Physical Device Driver Handle  */
static u32 Bf506fadc1_ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE                  hPhysicalDevice
);

#endif

/*
** Entry point to the BF506ADC1 device driver
*/
ADI_DEV_PDD_ENTRY_POINT     ADI_BF506ADC1_EntryPoint =
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

        Opens BF506ADC1 Audio Device for use

    Parameters:
        hDeviceManager     - Device Manager handle
        nDeviceNumber      - BF506ADC1 device number to open
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
    /* Return value - assume we're going to be successful */
    u32                     nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the device instance we will be working on */
    ADI_BF506ADC1_DEF       *poDevice;
    /* Exit critical region parameter */
    void                    *pExitCriticalArg;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    /* IF (Device number exceeds the number of devices supported) */
    if (nDeviceNumber >= ADI_BF506ADC1_NUM_DEVICES)
    {
        /* Report failure (Invalid Device number) */
        nResult = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;
    }
    /* ELSE IF (Direction other than inbound) */
    else if (eDirection != ADI_DEV_DIRECTION_INBOUND)
    {
        /* Report Failure (Direction not supported) */
        nResult = ADI_DEV_RESULT_DIRECTION_NOT_SUPPORTED;
    }
    /* ELSE (Success, try to reserve this device for the client) */
    else

#endif
    {
        /* assume that the device client requesting for is already in use */
        nResult = ADI_DEV_RESULT_DEVICE_IN_USE;
        /* BF506ADC1 device to work on */
        poDevice = &goaBf506fadc1Device[nDeviceNumber];

        /* Protect this section of code - entering a critical region */
        pExitCriticalArg = adi_int_EnterCriticalRegion(pCriticalRegionArg);

        /* IF (Device not in use) */
        if (poDevice->bIsDevInUse == false)
        {
            /* Reserve this device for this client */
            poDevice->bIsDevInUse = true;
            nResult = ADI_DEV_RESULT_SUCCESS;
        }

        /* Exit the critical region */
        adi_int_ExitCriticalRegion(pExitCriticalArg);

        /* IF (the device requested by the client is available for use) */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* Dataflow disabled by default */
            poDevice->bIsDataflowEnabled        = false;
            /* By default, use ACM to control ADC */
            poDevice->bUseAcm                   = true;
            /* ACM Device Number to use */
            poDevice->nAcmDevNumber             = 0;
            /* SPORT Device Number to use */
            poDevice->nSportDevNumber           = 0;
            poDevice->nSportRclkDiv             = 0;
            /* Data Mode - DOUT A only */
            poDevice->eCurDataMode              = ADI_BF506FADC1_DATA_DOUT_A_ONLY;
            /* Operating mode - Powerdown */
            poDevice->eCurOpMode                = ADI_BF506ADC1_MODE_POWERDOWN;
            /* No Flag pins */
            poDevice->oFlagPins.eA0Flag         = ADI_FLAG_UNDEFINED;
            poDevice->oFlagPins.eA1Flag         = ADI_FLAG_UNDEFINED;
            poDevice->oFlagPins.eA2Flag         = ADI_FLAG_UNDEFINED;
            poDevice->oFlagPins.eRangeSelFlag   = ADI_FLAG_UNDEFINED;
            poDevice->oFlagPins.eLogicSelFlag   = ADI_FLAG_UNDEFINED;
            /* ACLK Freq to default */
            poDevice->nCurSampleRateAclk        = BF506ADC1_DEFAULT_ACLK;
            /* Clear ACM Device handle */
            poDevice->hAcm                      = NULL;
            /* Clear the SPORT Device handle */
            poDevice->hSport                    = NULL;
            /* Save the Device Manager handle */
            poDevice->hDevManager               = hDeviceManager;
            /* Save the Device handle */
            poDevice->hDevice                   = hDevice;
            /* Save the DMA Manager handle */
            poDevice->hDmaManager               = hDmaManager;
            /* Save the DCB Manager handle */
            poDevice->hDcbManager               = hDcbManager;
            /* Save Device Manager supplied callback function */
            poDevice->pfCallback                = pfDMCallback;
            /* Save critical region arg */
            poDevice->pEnterCriticalArg         = pCriticalRegionArg;

            /* Save the physical device handle in the client supplied location */
            *phPhysicalDevice = (ADI_DEV_PDD_HANDLE *)poDevice;
        }
    }

    return (nResult);
}

/*********************************************************************

    Function: adi_pdd_Close

        Closes BF506ADC1 device

    Parameters:
        hPhysicalDevice - Physical Device handle to BF506ADC1 device instance

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully closed device
        Error code returned from SPORT and ACM close functions

*********************************************************************/
static u32 adi_pdd_Close(
    ADI_DEV_PDD_HANDLE      hPhysicalDevice
)
{
    /* Return value - assume we're going to be successful */
    u32                 nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the BF506ADC1 device we're working on */
    ADI_BF506ADC1_DEF   *poDevice = (ADI_BF506ADC1_DEF *)hPhysicalDevice;

/* for Debug build only - check for errors if required */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given Physical Device Handle */
    nResult = Bf506fadc1_ValidatePDDHandle(hPhysicalDevice);

    /* IF (given Physical Device Handle is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)

#endif

    {
        /* IF (The SPORT device is open) */
        if (poDevice->hSport != NULL)
        {
            /* Close the SPORT device */
            nResult = adi_dev_Close(poDevice->hSport);
            poDevice->hSport = NULL;
        }

        /* IF (Successfully closed SPORT device) */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* IF (ACM Device is open) */
            if (poDevice->hAcm != NULL)
            {
                /* Close the ACM device */
                nResult = (u32) adi_acm_Close(poDevice->hAcm);
            }
        }

        /* IF (Successfully closed ACM device) */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* Mark this device as not in use */
            poDevice->bIsDevInUse = false;
        }

    }

    return (nResult);
}

/*********************************************************************

    Function: adi_pdd_Read

        Queues Inbound buffers to BF506ADC1 SPORT

    Parameters:
        hPhysicalDevice    - Physical Device handle to BF506ADC1 device instance
        eBufferType        - Buffer type to queue
        poTxBuffer         - Pointer to receive buffer

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
    u32 nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the BF506ADC1 device we're working on */
    ADI_BF506ADC1_DEF   *poDevice = (ADI_BF506ADC1_DEF *)hPhysicalDevice;

/* for Debug build only - check for errors if required */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given Physical Device Handle */
    nResult = Bf506fadc1_ValidatePDDHandle(hPhysicalDevice);

    /* IF (given Physical Device Handle is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)

#endif

    {
        /* pass the read operation to SPORT driver */
        nResult = adi_dev_Read(poDevice->hSport, eBufferType, poRxBuffer);
    }

    return (nResult);
}

/*********************************************************************

    Function: adi_pdd_Write

        Queues Outbound buffers to SPORT
        Function not supported by BF506ADC1

*********************************************************************/
static u32  adi_pdd_Write(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *pBuffer
)
{
    /* Report Failure (Function not supported) */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************

    Function: adi_pdd_SequentialIO

        Sequentially read/writes data to a device
        Function not supported by BF506ADC1

*********************************************************************/
static u32  adi_pdd_SequentialIO(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    ADI_DEV_BUFFER_TYPE         eBufferType,
    ADI_DEV_BUFFER              *poSeqBuffer
)
{
    /* Report Failure (Function not supported) */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************

    Function: adi_pdd_Control

        Sets or senses device specific parameter

    Parameters:
        hPhysicalDevice - Physical Device handle to
                          BF506ADC1 device instance
        nCommandID      - Command ID
        Value           - Command specific value

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully processed command
        SPORT/ACM/BF506ADC1 Driver/Service specific Error codes

*********************************************************************/
static u32  adi_pdd_Control(
    ADI_DEV_PDD_HANDLE          hPhysicalDevice,
    u32                         nCommandID,
    void                        *Value
)
{
    /* Return value - assume we're going to be successful */
    u32                             nResult = ADI_DEV_RESULT_SUCCESS;
    /* Pointer to the BF506ADC1 device we're working on */
    ADI_BF506ADC1_DEF               *poDevice = (ADI_BF506ADC1_DEF *)hPhysicalDevice;
    /* Pointer to instance that holds ADC Control flag pins info */
    ADI_BF506FADC1_CTRL_FLAG_PINS   *poFlagInfo;
    /* Pointer to instance that holds ADC control pin value/state */
    ADI_BF506FADC1_CTRL_FLAG_STATE  *poFlagState;

/* for Debug build only - check for errors if required */
#if defined(ADI_DEV_DEBUG)

    /* Validate the given Physical Device Handle */
    nResult = Bf506fadc1_ValidatePDDHandle(hPhysicalDevice);

    /* IF (given Physical Device Handle is valid) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)

#endif

    {
        /* CASEOF (Command ID)  */
        switch (nCommandID)
        {
            /* CASE (Enable/Disable dataflow) */
            case ADI_DEV_CMD_SET_DATAFLOW:

                /* IF (Enable dataflow) */
                if ((u32)Value)
                {
                    /* Open ADC Control device */
                    if ((nResult = Bf506fadc1_OpenAdcCtrlDev(poDevice, true)) != ADI_DEV_RESULT_SUCCESS)
                    {
                        /* Failed to open ADC Control device, exit */
                        break;
                    }

                    /* Open and configure SPORT */
                    if ((nResult = Bf506fadc1_OpenSport(poDevice)) != ADI_DEV_RESULT_SUCCESS)
                    {
                        /* Failed to open/configure SPORT device, exit */
                        break;
                    }
                }

                /* IF (Issued dataflow status is different to the current state) */
                if (poDevice->bIsDataflowEnabled != (bool)Value)
                {
                    /* IF (Enable dataflow) */
                    if ((u32) Value)
                    {
                        /* IF (Use ACM to control ADC) */
                        if (poDevice->bUseAcm == true)
                        {
                            /* Enable ACM */
                            nResult = (u32) adi_acm_Control (poDevice->hAcm,
                                                             ADI_ACM_CMD_ENABLE_ACM,
                                                             (void *) true);
                        }

                        /* IF (Failed to enable ACM) */
                        if (nResult != ADI_DEV_RESULT_SUCCESS)
                        {
                            break;
                        }

                        /* Enable SPORT */
                        nResult = adi_dev_Control (poDevice->hSport,
                                                   ADI_DEV_CMD_SET_DATAFLOW,
                                                   (void *) true);
                    }
                    /* ELSE (Disable dataflow) */
                    else
                    {
                        /* IF (Sport handle is valid) */
                        if (poDevice->hSport != NULL)
                        {
                            nResult = adi_dev_Control (poDevice->hSport,
                                                       ADI_DEV_CMD_SET_DATAFLOW,
                                                       (void *) false);
                        }

                        /* IF (Use ACM to control ADC and
                               we've a valid ACM handle and
                               we've successfully disabled SPORT) */
                        if ((poDevice->bUseAcm == true) &&\
                            (poDevice->hAcm != NULL) &&\
                            (nResult == ADI_DEV_RESULT_SUCCESS))
                        {
                            /* Disable ACM */
                            nResult = (u32) adi_acm_Control (poDevice->hAcm,
                                                             ADI_ACM_CMD_ENABLE_ACM,
                                                             (void *) false);
                        }

                    } /* End of if (Enable dataflow) else case */

                    /* IF (Successfully applied new dataflow status) */
                    if (nResult == ADI_DEV_RESULT_SUCCESS)
                    {
                        /* Update dataflow status flag */
                        poDevice->bIsDataflowEnabled = (bool)Value;
                    }

                } /* End of if (Issued dataflow status is different to the current state) */
                break;

            /* CASE (query for processor DMA support) */
            case ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT:
                /* DMA not supported at this level (DMA it is hadled at SPORT driver level) */
                *((u32 *)Value) = false;
                break;

            /* CASE (Set ACM Device number to control ADC) */
            case ADI_BF506ADC1_CMD_SET_ACM_DEV_NUMBER:

                /* Save the ACM Device number */
                poDevice->nAcmDevNumber = (u8)((u32)Value);
                /* Update active control dev flag to use ACM to control ADC */
                poDevice->bUseAcm = true;

                /* IF (ACM is not open yet) */
                if (poDevice->hAcm == NULL)
                {
                    /* nothing to do as we've already saved the device number
                       quit and return */
                    break;
                }
                /* ELSE (ACM is already open) */
                else
                {
                    /* Close ACM */
                    nResult = (u32) adi_acm_Close (poDevice->hAcm);
                }

                /* IF (Failed to close ACM) */
                if (nResult != ADI_DEV_RESULT_SUCCESS)
                {
                    /* quit and return */
                    break;
                }
                /* ELSE (Continue to open ACM) */
                else
                {
                    /* Clear ACM Handle */
                    poDevice->hAcm = NULL;
                    /* Open/Close ADC Control device */
                    nResult = Bf506fadc1_OpenAdcCtrlDev(poDevice, (bool)Value);
                }
                break;

            /* CASE (Open/Close the device used to control ADC (ACM or GPIO Flags)) */
            case ADI_BF506ADC1_CMD_OPEN_ADC_CTRL_DEV:

                /* Open/Close ADC Control device */
                nResult = Bf506fadc1_OpenAdcCtrlDev(poDevice, (bool)Value);
                break;

            /* CASE (Set SPORT Device number connected to ADC) */
            case ADI_BF506ADC1_CMD_SET_SPORT_DEV_NUMBER:

                /* Save the SPORT Device number */
                poDevice->nSportDevNumber = (u8)((u32)Value);

                /* IF (SPORT is not open yet) */
                if (poDevice->hSport == NULL)
                {
                    /* nothing to do as we've already saved the device number
                       quit and return */
                    break;
                }
                /* ELSE (SPORT is already open) */
                else
                {
                    /* Close SPORT */
                    nResult = adi_dev_Close (poDevice->hSport);
                }

                /* IF (Failed to close SPORT) */
                if (nResult != ADI_DEV_RESULT_SUCCESS)
                {
                    /* quit and return */
                    break;
                }
                /* ELSE (Continue to open SPORT) */
                else
                {
                    /* Clear SPORT Device handle */
                    poDevice->hSport = NULL;
                    /* Open and configure SPORT */
                    nResult = Bf506fadc1_OpenSport (poDevice);
                }
                break;

            /* CASE (Open/Close SPORT device connected to ADC) */
            case ADI_BF506ADC1_CMD_OPEN_SPORT_DEV:

                /* IF (Open SPORT deivce) */
                if ((bool) Value == true)
                {
                    /* Open and configure SPORT */
                    nResult = Bf506fadc1_OpenSport (poDevice);
                }
                /* ELSE (Close SPORT Device) */
                else
                {
                    /* IF (SPORT Device is open) */
                    if (poDevice->hSport != NULL)
                    {
                        /* Close SPORT Device */
                        adi_dev_Close (poDevice->hSport);
                    }
                }
                break;

            /* CASE (Set Flag IDs of all ADC control signals and
                     configures the driver to control ADC via Flag pins) */
            case ADI_BF506ADC1_CMD_SET_ADC_CTRL_FLAG_PINS:

                /* type-cast to ADC Control Flag info */
                poFlagInfo = (ADI_BF506FADC1_CTRL_FLAG_PINS *)Value;

                /* IF (we're switching from ACM to GPIO to control ADC) */
                if (poDevice->bUseAcm == true)
                {
                    /* IF (ACM is already open) */
                    if (poDevice->hAcm != NULL)
                    {
                        /* Close ACM */
                        adi_acm_Close (poDevice->hAcm);
                        poDevice->hAcm = NULL;
                    }
                }

                /* Update active control dev flag to use GPIO Flags */
                poDevice->bUseAcm = false;

                /* Save the Control flag pin info */
                poDevice->oFlagPins.eA0Flag         = poFlagInfo->eA0Flag;
                poDevice->oFlagPins.eA1Flag         = poFlagInfo->eA1Flag;
                poDevice->oFlagPins.eA2Flag         = poFlagInfo->eA2Flag;
                poDevice->oFlagPins.eRangeSelFlag   = poFlagInfo->eRangeSelFlag;
                poDevice->oFlagPins.eLogicSelFlag   = poFlagInfo->eLogicSelFlag;
                break;

            /* CASE (Set the value/state of ADC control signals when
                     the driver is set to controlled ADC via Flag pins) */
            case ADI_BF506ADC1_CMD_SET_ADC_CTRL_FLAG_STATE:

                /* type-cast to ADC Control Flag pin value/state */
                poFlagState = (ADI_BF506FADC1_CTRL_FLAG_STATE *)Value;

                /* Save the Control flag pin value/state */
                poDevice->oFlagState.bSetA0         = poFlagState->bSetA0;
                poDevice->oFlagState.bSetA1         = poFlagState->bSetA1;
                poDevice->oFlagState.bSetA2         = poFlagState->bSetA2;
                poDevice->oFlagState.eSetRangeSel   = poFlagState->eSetRangeSel;
                poDevice->oFlagState.eSetLogicSel   = poFlagState->eSetLogicSel;
                break;

            /* CASE (Set Serial data mode to receive ADC data) */
            case ADI_BF506ADC1_CMD_SET_SERIAL_DATA_MODE:

                /* IF (The supplied data mode is different to the current) */
                if (poDevice->eCurDataMode != (ADI_BF506FADC1_DATA_MODE) Value)
                {
                    /* Save the supplied date mode */
                    poDevice->eCurDataMode = (ADI_BF506FADC1_DATA_MODE) Value;

                    /* IF (Use ACM to control ADC) */
                    if (poDevice->bUseAcm == true)
                    {
                        /* Update ACM Config */
                        nResult = Bf506fadc1_UpdateAcmConfig(poDevice);
                    }
                    /* ELSE (Use GPIO to control ACM) */
                    else
                    {
                        /* Update SPORT Configuration */
                        nResult = Bf506fadc1_UpdateSportConfig (poDevice);
                    }
                }
                break;

            /* CASE (Set ADC Operating mode) */
            case ADI_BF506ADC1_CMD_SET_OPERATING_MODE:

                /* Apply this operating mode */
                nResult = Bf506fadc1_ApplyOpMode (poDevice, (ADI_BF506FADC1_OP_MODE) Value);
                break;

            /* CASE (Set ADC sampling rate / ACLK frequency) */
            case ADI_BF506ADC1_CMD_SET_SAMPLE_RATE_ACLK_FREQ:

                /* IF (Sample Rate/ACLK Freq is valid) */
                if ((u32) Value > 0)
                {
                    /* Apply the given sample rate/ACLK Freq */
                    nResult = Bf506fadc1_ApplySampleRateAclk (poDevice,
                                                              poDevice->bUseAcm,
                                                              (u32) Value);
                }
                /* ELSE (Sample Rate/ACLK Freq invalid) */
                else
                {
                    /* Report Failure (Sample Rate / ACLK Freq is invalid) */
                    nResult = ADI_BF506ADC1_RESULT_SAMPLE_RATE_ACLK_INVALID;
                }
                break;

            /* Commands other than BF506 ADC1 driver specific */
            default:

                /* IF (This is ACM Specific command) */
                if ((nCommandID & ADI_ACM_ENUMERATION_START) == ADI_ACM_ENUMERATION_START)
                {
                    /* IF (ACM is enabled) */
                    if (poDevice->hAcm != NULL)
                    {
                        /* Pass this command to ACM */
                        nResult = (u32) adi_acm_Control (poDevice->hAcm,
                                                         (ADI_ACM_COMMAND) nCommandID,
                                                         Value);
                    }
                    /* ELSE (ACM is not opened yet) */
                    else
                    {
                        /* Report failure (Command not supported) */
                        nResult = ADI_BF506ADC1_RESULT_CMD_NOT_SUPPORTED;
                    }
                }
                /* ELSE (Assume command is device driver specific) */
                else
                {
                    /* IF (SPORT Device is already open) */
                    if (poDevice->hSport != NULL)
                    {
                        /* Pass this command to SPORT device */
                        nResult = adi_dev_Control (poDevice->hSport,
                                                   nCommandID,
                                                   Value);
                    }
                    /* ELSE (SPORT is not opened yet) */
                    else
                    {
                        /* Report failure (Command not supported) */
                        nResult = ADI_BF506ADC1_RESULT_CMD_NOT_SUPPORTED;
                    }
                }

                break;

        } /* End Case of (eCommandID) */

    } /* Debug build only */

    return (nResult);
}

/*********************************************************************

    Function: Bf506fadc1_OpenAcm

        Opens ACM device for ADC use

    Parameters:
        poDevice - Pointer to ADC Device instance to work on

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully opened ACM
        ACM service specific error codes

*********************************************************************/
static u32 Bf506fadc1_OpenAcm (
    ADI_BF506ADC1_DEF   *poDevice
)
{

    /* Return value - assume we're going to be successful */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;

    /* Open the new ACM device */
    nResult = (u32) adi_acm_Open (poDevice->nAcmDevNumber,
                                  poDevice->pEnterCriticalArg,
                                  &poDevice->hAcm);

    /* IF (Successfully Opened ACM) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* By default, ACM is configured such that
           - Active low CS
           - ADC Clock (ACLK) polarity after CS gets activated (falling edge)
           - Setup Clock cycles to zero (means 1 SCLK)
           - Hold Cycles to zero */

        /* Update ACM Config */
        nResult = Bf506fadc1_UpdateAcmConfig(poDevice);
    }

    return (nResult);
}

/*********************************************************************

    Function: Bf506fadc1_UpdateAcmConfig

        Updates ACM configuration for ADC use

    Parameters:
        poDevice - Pointer to ADC Device instance to work on

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully opened ACM
        ACM service specific error codes

*********************************************************************/
static u32 Bf506fadc1_UpdateAcmConfig (
    ADI_BF506ADC1_DEF   *poDevice
)
{

    /* Return value - assume we're going to be successful */
    u32                     nResult = ADI_DEV_RESULT_SUCCESS;
    /* ADC word length */
    u32                     nWordLength;
    /* ACM Configuration table */
    ADI_ACM_CMD_VALUE_PAIR  aoAcmConfig[] =
    {
        /* Number of Zero cycles */
        { ADI_ACM_CMD_SET_ZERO_CYCLES,          (void *) 2                          },
        /* External peripheral select */
        { ADI_ACM_CMD_SET_EXT_PERIPHERAL_SEL,   (void *) poDevice->nSportDevNumber  },
        /* CS Width in ACLKs */
        { ADI_ACM_CMD_SET_CS_WIDTH,             NULL                                },
        /* End of config table */
        ADI_ACM_CMD_END
    };

    /* IF (ACM is open) */
    if(poDevice->hAcm != NULL)
    {
        /* Get word length */
        GetSportWordLength (poDevice, &nWordLength);
        /* Convert word length to number of ACLKs equivelent */
        aoAcmConfig[2].Value = (void *) (nWordLength+1);

        /* Update ACM Configuration */
        nResult = (u32) adi_acm_Control(poDevice->hAcm,
                                        ADI_ACM_CMD_TABLE,
                                        (void *) &aoAcmConfig[0]);

        /* IF (Successfully updated ACM configuration) */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* Update SPORT Configuration */
            nResult = Bf506fadc1_UpdateSportConfig (poDevice);
        }
    }

    return (nResult);
}

/*********************************************************************

    Function: Bf506fadc1_OpenSport

        Opens the SPORT device

    Parameters:
        poDevice - Pointer to ADC Device instance to work on

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully opened SPORT
        SPORT driver specific error codes

*********************************************************************/
static u32 Bf506fadc1_OpenSport (
    ADI_BF506ADC1_DEF   *poDevice
)
{

    /* Return code - assume we're going to be successful */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;

    /* IF (SPORT device is not open yet) */
    if (poDevice->hSport == NULL)
    {
        /* IF (Successfully opened SPORT driver) */
        if ((nResult = adi_dev_Open(poDevice->hDevManager,
                                    &ADISPORTEntryPoint,
                                    poDevice->nSportDevNumber,
                                    poDevice,
                                    &poDevice->hSport,
                                    ADI_DEV_DIRECTION_INBOUND,
                                    poDevice->hDmaManager,
                                    poDevice->hDcbManager,
                                    Bf506fadc1_SportCallback))
                                 == ADI_DEV_RESULT_SUCCESS)
        {
            /* IF (Use ACM as ADC control device) */
            if (poDevice->bUseAcm == true)
            {
                /* Update ACM Configuration */
                nResult = Bf506fadc1_UpdateAcmConfig (poDevice);
            }
            /* ELSE (ACM is not open or used) */
            else
            {
                /* Update SPORT Device configuration */
                nResult = Bf506fadc1_UpdateSportConfig(poDevice);
            }

        } /* End of if (Successfully opened SPORT driver) */

    }

    return (nResult);
}

/*********************************************************************

    Function: Bf506fadc1_UpdateSportConfig

        Updates SPORT register configuration

    Parameters:
        poDevice - Pointer to ADC Device instance to work on

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully updated SPORT register configuration
        SPORT driver specific error codes

*********************************************************************/
static u32 Bf506fadc1_UpdateSportConfig (
    ADI_BF506ADC1_DEF   *poDevice
)
{

    /* Return code - assume we're going to be successful */
    u32                     nResult = ADI_DEV_RESULT_SUCCESS;
    u32                     nWordlength;
    /* Configuration table for SPORT */
    ADI_DEV_CMD_VALUE_PAIR  aoSportConfig[] =
    {
        /* SPORT RCR1 config */
        { ADI_SPORT_CMD_SET_RCR1,                   NULL            },
        /* SPORT Rx wordlength */
        { ADI_SPORT_CMD_SET_RX_WORD_LENGTH,         NULL            },
        /* SPORT Secondary Rx enable/disable */
        { ADI_SPORT_CMD_SET_RX_SECONDARY_ENABLE,    NULL            },
        /* End of config table */
        ADI_DEV_CMD_END
    };

    /* IF (SPORT device is open) */
    if (poDevice->hSport != NULL)
    {
        /* IF (Use ACM to control ADC) */
        if (poDevice->bUseAcm == true)
        {
            /* SPORT RCR1 value for ACM controlled ADC */
            aoSportConfig[0].Value = (void *) ADI_SPORT_RCR1_ACM_CTRL;
        }
        /* ELSE (Use GPIO Flags to control ADC) */
        else
        {
            /* SPORT RCR1 value for GPIO Flag controlled ADC */
            aoSportConfig[0].Value = (void *) ADI_SPORT_RCR1_GPIO_CTRL;
        }

        /* Calculate SPORT Word length */
        GetSportWordLength (poDevice, &nWordlength);
        /* Save wordlength */
        aoSportConfig[1].Value = (void *) nWordlength;

        /*
        ** Check if Secondary channel is required or not
        */

        /* IF (Current data mode is set to have seperate Rx channels) */
        if ((poDevice->eCurDataMode == ADI_BF506FADC1_DATA_DOUT_A_B_SEPERATE_RX) ||\
            (poDevice->eCurDataMode == ADI_BF506FADC1_DATA_DOUT_A_B_SEPERATE_RX_XTND_ZERO))
        {
            /* Enable secondary channel */
            aoSportConfig[2].Value = (void *) true;
        }
        /* ELSE (Current data mode requires only one seperate Rx channel) */
        else
        {
            /* Disable secondary channel */
            aoSportConfig[2].Value = (void *) false;
        }

        /* IF (Dataflow is already enabled) */
        if (poDevice->bIsDataflowEnabled == true)
        {
            /* Disable SPORT Dataflow */
            adi_dev_Control (poDevice->hSport,
                             ADI_DEV_CMD_SET_DATAFLOW,
                             (void *) false);

            /* Update dataflow status flag */
            poDevice->bIsDataflowEnabled = false;
        }

        /* Update SPORT settings */
        nResult = adi_dev_Control (poDevice->hSport,
                                   ADI_DEV_CMD_TABLE,
                                   (void *) &aoSportConfig[0]);

        /* IF (Successfully update SPORT register configuration) */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* Apply current sample rate cached with in the driver */
            nResult = Bf506fadc1_ApplySampleRateAclk (poDevice,
                                                      poDevice->bUseAcm,
                                                      poDevice->nCurSampleRateAclk);
        }

    }

    return (nResult);
}

/*********************************************************************

    Function: Bf506fadc1_OpenAdcCtrlDev

        Opens/Closes ADC Control device

    Parameters:
        poDevice - Pointer to ADC Device instance to work on
        bOpen    - Flag to indicate whether to open or close device

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully opened/closed ADC Control device
        ACM/SPORT/Flag service specific error codes

*********************************************************************/
static u32 Bf506fadc1_OpenAdcCtrlDev (
    ADI_BF506ADC1_DEF   *poDevice,
    bool                bOpen
)
{
    /* Return code - assume we're going to be successful */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;

    /* IF (Open ADC Control device) */
    if (bOpen == true)
    {
        /* IF (Use ACM to control ADC) */
        if (poDevice->bUseAcm == true)
        {
            /* IF (ACM is not open yet) */
            if (poDevice->hAcm == NULL)
            {
                /* Open ACM Device */
                nResult = Bf506fadc1_OpenAcm (poDevice);
            }
        }
        /* ELSE (Use GPIO Flags to control ADC) */
        else
        {
            /* Open and configure A0 Flag */
            nResult = Bf506fadc1_ConfigFlag (poDevice->oFlagPins.eA0Flag,
                                             poDevice->oFlagState.bSetA0);

            /* IF (Succfully opened and configured A0 Flag) */
            if (nResult == ADI_DEV_RESULT_SUCCESS)
            {
                /* Open and configure A1 Flag */
                nResult = Bf506fadc1_ConfigFlag (poDevice->oFlagPins.eA1Flag,
                                                 poDevice->oFlagState.bSetA1);
            }

            /* IF (Succfully opened and configured A1 Flag) */
            if (nResult == ADI_DEV_RESULT_SUCCESS)
            {
                /* Open and configure A2 Flag */
                nResult = Bf506fadc1_ConfigFlag (poDevice->oFlagPins.eA2Flag,
                                                 poDevice->oFlagState.bSetA2);
            }

            /* IF (Succfully opened and configured Range Select Flag) */
            if (nResult == ADI_DEV_RESULT_SUCCESS)
            {
                /* Open and configure Range Select Flag */
                nResult = Bf506fadc1_ConfigFlag (poDevice->oFlagPins.eRangeSelFlag,
                                                 poDevice->oFlagState.eSetRangeSel);
            }

            /* IF (Succfully opened and configured Logic Select Flag) */
            if (nResult == ADI_DEV_RESULT_SUCCESS)
            {
                /* Open and configure Logic Select Flag */
                nResult = Bf506fadc1_ConfigFlag (poDevice->oFlagPins.eLogicSelFlag,
                                                 poDevice->oFlagState.eSetLogicSel);
            }

        }  /* End of if (Use ACM to control ADC) else case */

    }
    /* ELSE (Close ADC Control device) */
    else
    {
        /* IF (Use ACM to control ADC) */
        if (poDevice->bUseAcm == true)
        {
            /* IF (ACM device is open) */
            if (poDevice->hAcm != NULL)
            {
                /* Close ACM Device */
                nResult = adi_acm_Close (poDevice->hAcm);

                /* IF (Successfully closed ACM) */
                if (nResult == ADI_DEV_RESULT_SUCCESS)
                {
                    poDevice->hAcm = NULL;
                }

            } /* End of if (ACM device is open) */

        }
        /* ELSE (Use GPIO Flags to control ADC) */
            /* Nothing to do here */

    }

    return (nResult);
}

/*********************************************************************

    Function: Bf506fadc1_ConfigFlag

        Configures selected GPIO pin for ADC control

    Parameters:
        eFlagID - Flag ID to open and set as output
        bState  - Flag state (set or clear)

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully opened and configured GPIO Flag
        Flag service specific error codes

*********************************************************************/
static u32 Bf506fadc1_ConfigFlag (
    ADI_FLAG_ID     eFlagID,
    bool            bState
)
{
    /* Return code */
    u32 nResult;

    /* Open this flag for GPIO use */
    nResult = (u32) adi_flag_Open(eFlagID);

    /* IF (Successfully opened Flag for GPIO use) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Configure flag as output */
        nResult = (u32) adi_flag_SetDirection (eFlagID, ADI_FLAG_DIRECTION_OUTPUT);
    }

    /* IF (Successfully configured Flag direction as output) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* IF (Set flag) */
        if (bState == true)
        {
            /* Set this flag */
            nResult = (u32) adi_flag_Set (eFlagID);
        }
        /* ELSE (Clear flag) */
        else
        {
            /* Clear this flag */
            nResult = (u32) adi_flag_Clear (eFlagID);
        }
    }

    return (nResult);

}

/*********************************************************************

    Function: Bf506fadc1_ApplyOpMode

        Apply ADC operating mode

    Parameters:
        eFlagID - Flag ID to open and set as output
        bState  - Flag state (set or clear)

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully applied given operating mode
        Flag service specific error codes

*********************************************************************/
static u32 Bf506fadc1_ApplyOpMode (
    ADI_BF506ADC1_DEF           *poDevice,
    ADI_BF506FADC1_OP_MODE      eNewOpMode
)
{
    /* Return code */
    u32             nResult = ADI_DEV_RESULT_SUCCESS;
    /* Location to store current ACLK Freq/Sample Rate settings */
    u32             nAclkFreqSampleRate;
    /* Location to hold processor clock values */
    u32             nTemp, nFcclk, nFsclk;
    /* Number of ACLKs to wait */
    u32             nNumAclks;
    /* Delay counter */
    volatile u32    nDelay = 0;
    /* Additional delay counter */
    volatile u32    nAddDelay = 0;
    /* Configuration table for SPORT */
    ADI_DEV_CMD_VALUE_PAIR  aoSportConfig[] =
    {
        /* SPORT RCR1 config */
        { ADI_SPORT_CMD_SET_RCR1,           (void *) ADI_SPORT_RCR1_GPIO_CTRL   },
        /* SPORT Rx wordlength */
        { ADI_SPORT_CMD_SET_RX_WORD_LENGTH, NULL                                },
        /* Enable SPORT dataflow */
        { ADI_DEV_CMD_SET_DATAFLOW,         (void *) true                       },
        /* End of config table */
        ADI_DEV_CMD_END
    };

    /* IF (The supplied operating mode is different to current) */
    if (poDevice->eCurOpMode != eNewOpMode)
    {
        /* IF (SPORT Device is not open yet) */
        if (poDevice->hSport == NULL)
        {
            /* Report Failure (Command can not be processed at current stage) */
            nResult = ADI_BF506ADC1_RESULT_CANNOT_PROCESS_CMD;
        }
        /* ELSE (SPORT device is open) */
        else
        {
            /* Disable SPORT Dataflow */
            nResult = adi_dev_Control (poDevice->hSport,
                                       ADI_DEV_CMD_SET_DATAFLOW,
                                       (void *) false);
        }

        /* IF (Successfully disabled SPORT dataflow) */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* Get current CCLK */
            nResult = (u32) adi_pwr_GetFreq(&nFcclk,&nFsclk,&nTemp);
        }

        /* IF (Successfully obtainded current CCLK freq) */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* Save the current ACLK Freq/Sample rate */
            nAclkFreqSampleRate = poDevice->nCurSampleRateAclk;

            /* Apply sample rate such that SPORT is configured to generate ACLK & CS */
            nResult = Bf506fadc1_ApplySampleRateAclk (poDevice,
                                                      false,
                                                      ADC_POWER_RFS_FREQ);
            /* Restore ACLK Freq/Sample rate */
            poDevice->nCurSampleRateAclk = nAclkFreqSampleRate;
        }

        /* IF (Successfully applied sample rate) */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* IF (New operating mode is to partially power-down ADC) */
            if (eNewOpMode == ADI_BF506ADC1_MODE_PARTIAL_POWERDOWN)
            {
                /*
                ** Configure SPORT to read 8 bits (issue 8 ACLKs and a CS pulse)
                ** Run SPORT with above settings for one CS pulse and disable dataflow
                */

                /* SPORT wordlength to partially power-down ADC */
                aoSportConfig[1].Value = (void *) ADC_POWER_DN_NUM_SLEN;
                /* Number of ACLKs to wait */
                nNumAclks = ADC_PARTIAL_POWER_DN_NUM_ACLK;
                /* No need for additional delay */
            }
            /* ELSE IF (New operating mode is to completely power-down ADC) */
            else if (eNewOpMode == ADI_BF506ADC1_MODE_POWERDOWN)
            {
                /*
                ** Configure SPORT to read 8 bits (issue 8 ACLKs and a CS pulse)
                ** Run SPORT with above settings for two CS pulse and disable dataflow
                */

                /* SPORT wordlength to partially power-down ADC */
                aoSportConfig[1].Value = (void *) ADC_POWER_DN_NUM_SLEN;
                /* Number of ACLKs to wait */
                nNumAclks = ADC_FULL_POWER_DN_NUM_ACLK;
                /* No need for additional delay */
            }
            /* ELSE (New operating mode is to power-up ADC) */
            else
            {
                /*
                ** Configure SPORT to read 14 bits (issue 14 ACLKs and a CS pulse)
                ** Run SPORT with above settings for one CS pulse and disable dataflow
                ** wait for additional time until ADC completely powers-up
                */

                /* SPORT wordlength to power-up ADC */
                aoSportConfig[1].Value = (void *) ADC_POWER_UP_NUM_SLEN;
                /* Number of ACLKs to wait */
                nNumAclks = ADC_POWER_UP_NUM_ACLK;

                /*
                ** Calcaulate additional Delay in terms of cclks
                **
                ** ADC Requires maximum of 1.5ms to exit from
                ** complete power-down to normal mode
                ** convert 1.5ms to ns (1500000ns) as cclk is in ns
                ** Additional delay = (15 * (cclk / 100000))
                */
                nAddDelay = (15U * (nFcclk / 100000U));
            }

            /*
            ** Calculate the total delay to wait until
            ** the selected power status is reached
            ** Delay in terms of SPORT Rx Clock cycles
            */
            nDelay = (nNumAclks * (2 * (poDevice->nSportRclkDiv + 1) * (nFcclk/nFsclk)));

            /* Update SPORT settings */
            nResult = adi_dev_Control (poDevice->hSport,
                                       ADI_DEV_CMD_TABLE,
                                       (void *) &aoSportConfig[0]);
        }

        /* IF (Successfully configured SPORT) */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* Save the operating mode */
            poDevice->eCurOpMode = eNewOpMode;

            /* Wait until the delay expires */
            while (nDelay != 0)
            {
                nDelay--;
            }

            /* Disable SPORT Dataflow */
            nResult = adi_dev_Control (poDevice->hSport,
                                       ADI_DEV_CMD_SET_DATAFLOW,
                                       (void *) false);

            /* Wait until the additional delay expires */
            while (nAddDelay != 0)
            {
                nAddDelay--;
            }
        }

        /* IF (Successfully applied operating mode) */
        if (nResult == ADI_DEV_RESULT_SUCCESS)
        {
            /* Update SPORT Configuration as per the current Data Mode */
            nResult = Bf506fadc1_UpdateSportConfig(poDevice);
        }

    }

    return (nResult);
}

/*********************************************************************

    Function: Bf506fadc1_ApplySampleRateAclk

        Applies the given Sample Rate/ACLK Freq

    Parameters:
        poDevice            - Pointer to ADC Device instance to work on
        bUseAcm             - Flag to indicate whether to use ACM or not
        nNewSampleRateAclk  - Sample Rate/ACLK Freq to apply

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully applied Sample Rate/ACLK Freq
        ACM service specific error codes

*********************************************************************/
static u32 Bf506fadc1_ApplySampleRateAclk (
    ADI_BF506ADC1_DEF   *poDevice,
    bool                bUseAcm,
    u32                 nNewSampleRateAclk
)
{
    /* Return value - assume we're going to be successful */
    u32 nResult = ADI_DEV_RESULT_SUCCESS;
    /* Location to hold SPORT Rx Frame sync div (RFSDIV) &
       Rx Clock div (RCLKDIV) values calculated from the new sampling rate */
    u16 nRxFsDiv,nRxClkDiv;
    /* Location to hold processor clock values */
    u32 nTemp, nFsclk;
    /* Configuration table for SPORT */
    ADI_DEV_CMD_VALUE_PAIR  aoSportRxClkConfig[] =
    {
        /* SPORT RFS Divisor */
        { ADI_SPORT_CMD_SET_RFSDIV,     (void *) 0 },
        /* SPORT RCLK Divisor */
        { ADI_SPORT_CMD_SET_RCLKDIV,    (void *) 0 },
        /* End of config table */
        ADI_DEV_CMD_END
    };

    /* IF (Use ACM to control ADC) */
    if (bUseAcm == true)
    {
        /* IF (ACM is open) */
        if (poDevice->hAcm != NULL)
        {
            /* Apply this ACLK frequency */
            nResult = adi_acm_Control(poDevice->hAcm,
                                      ADI_ACM_CMD_SET_ACLK_FREQ,
                                      (void *) nNewSampleRateAclk);

            /* IF (Failed to derive ACLK from current SCLK) */
            if (nResult == (u32) ADI_ACM_RESULT_CANNOT_DERIVE_ACLK)
            {
                /* Report Failure (ACLK Freq not supported) */
                nResult = ADI_BF506ADC1_RESULT_SAMPLE_RATE_ACLK_NOT_SUPPORTED;
            }

            /* IF (SPORT is open) */
            if (poDevice->hSport != NULL)
            {
                /* Clear SPORT RFSDIV & RCLKDIV */
                adi_dev_Control (poDevice->hSport,
                                 ADI_DEV_CMD_TABLE,
                                 (void *) &aoSportRxClkConfig[0]);
            }

        } /* End of if (ACM is open) */

    }
    /* ELSE (Use GPIO flags to control ADC) */
    else
    {
        /* IF (SPORT is open) */
        if (poDevice->hSport != NULL)
        {
            /* Get current SCLK */
            nResult = (u32) adi_pwr_GetFreq(&nTemp,&nFsclk,&nTemp);

            /* IF (Successfully obtained SCLK) */
            if (nResult == ADI_DEV_RESULT_SUCCESS)
            {
                /* Calculate SPORT wordlength */
                GetSportWordLength (poDevice, &nTemp);

                /*
                ** Calculate the SPORTx_RFSDIV value from the requested sampling rate
                ** and taking into account the minimum total 'quiet' time between last
                ** sampled bit and the next falling edge of the /CS signal
                ** Make sure RFS is calculated to receive 'N' number of bits per samples
                ** where 'N' is the word length calculated from current data mode
                */
                nRxFsDiv = (tepsmin * nNewSampleRateAclk + nTemp * ten2powr8)/(ten2powr8 - tepsmin * nNewSampleRateAclk) + 1;

                /* Now we estimate the SPORTx_RCLKDIV value */
                nRxClkDiv = nFsclk/(2 * nNewSampleRateAclk * (nRxFsDiv+1));

                /* IF (The sample rate can be supported) */
                if (nRxClkDiv > 0)
                {
                    nRxClkDiv--;

                    /* Ensure the actual sampling rate is no greater than that requested */
                    if (nFsclk > (nNewSampleRateAclk * 2 * (nRxFsDiv + 1) * (nRxClkDiv + 1)))
                    {
                        nRxClkDiv++;
                    }

                    /* Update SPORT RxClk configuration table */
                    aoSportRxClkConfig[0].Value = (void *) nRxFsDiv;
                    aoSportRxClkConfig[1].Value = (void *) nRxClkDiv;
                    /* Save RCLK Div */
                    poDevice->nSportRclkDiv = nRxClkDiv;

                    /* Update SPORT configuration */
                    nResult = adi_dev_Control (poDevice->hSport,
                                               ADI_DEV_CMD_TABLE,
                                               (void *) &aoSportRxClkConfig[0]);
                }
                /* ELSE (Sample rate can't be supported) */
                else
                {
                    /* Report Failure (Sample rate not supported) */
                    nResult = ADI_BF506ADC1_RESULT_SAMPLE_RATE_ACLK_NOT_SUPPORTED;
                }

            } /* End of if (Successfully obtained SCLK) */

        } /* End of if (SPORT is open) */

    }

    /* IF (Successfully applied Sample rate/ ACLK Freq) */
    if (nResult == ADI_DEV_RESULT_SUCCESS)
    {
        /* Update the current sample rate */
        poDevice->nCurSampleRateAclk = nNewSampleRateAclk;
    }

    return (nResult);
}

/*********************************************************************

    Function: Bf506fadc1_SportCallback

        Callback from SPORT device

    Parameters:
        hAdcDevice  - Handle passed to SPORT driver
        nEvent      - Callback Event ID
        pArg        - Callback argument

    Returns:
        None

*********************************************************************/
static void Bf506fadc1_SportCallback (
    void    *hAdcDevice,
    u32     nEvent,
    void    *pArg
)
{
    ADI_BF506ADC1_DEF   *poDevice = (ADI_BF506ADC1_DEF *)hAdcDevice;

/* Debug build only */
#if defined(ADI_DEV_DEBUG)

    /* verify the DM handle */
    if (Bf506fadc1_ValidatePDDHandle(hAdcDevice) == ADI_DEV_RESULT_SUCCESS)

#endif
    {
        /*
        ** Simply pass the callback to the
        ** Device Manager Callback without interpretation
        */
        (poDevice->pfCallback)(poDevice->hDevice, nEvent, pArg);
    }
}

/*********************************************************************

    Function: GetSportWordLength

        Calculates SPORT word length based on current data mode

    Parameters:
        poDevice    - Pointer to ADC Device instance to work on
        pWordLength - Location to store word length

    Returns:
        None

*********************************************************************/
static void GetSportWordLength (
    ADI_BF506ADC1_DEF   *poDevice,
    u32                 *pWordLength
)
{
    /* IF (Current data mode is set to share Rx between two ports) */
    if (poDevice->eCurDataMode == ADI_BF506FADC1_DATA_DOUT_A_B_SHARED_RX)
    {
        /* SPORT should support two channels (32-bits) */
        *pWordLength = ADI_SPORT_SLEN_32;
    }
    /* ELSE IF (Current data mode requires extended zeros) */
    else if ((poDevice->eCurDataMode == ADI_BF506FADC1_DATA_DOUT_A_ONLY_XTND_ZERO) ||\
             (poDevice->eCurDataMode == ADI_BF506FADC1_DATA_DOUT_A_B_SEPERATE_RX_XTND_ZERO))
    {
        /* SPORT should support 16-bit data */
        *pWordLength = ADI_SPORT_SLEN_16;
    }
    /* ELSE (No extended zeros) */
    else
    {
        /* SPORT should support 14-bit data */
        *pWordLength = ADI_SPORT_SLEN_14;
    }
}

/* Debug build only */
#if defined(ADI_DEV_DEBUG)
/*********************************************************************

    Function: Bf506fadc1_ValidatePDDHandle

        Validates the given Physical Device Driver Handle

    Parameters:
        hPhysicalDevice - Physical Device Driver Handle to validate

    Returns:
        ADI_DEV_RESULT_SUCCESS
            - Successfully validated Physical Device Driver Handle
        ADI_DEV_RESULT_BAD_PDD_HANDLE
            - Physical Device Driver Handle is invalid

*********************************************************************/
static u32 Bf506fadc1_ValidatePDDHandle(
    ADI_DEV_PDD_HANDLE      hPhysicalDevice
)
{
    /* Return value - insure the client has provided a valid PDD Handle */
    u32     nResult = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    /* index */
    u32     nIndex;

    /* compare the given Physical Device Driver Handle handle with Handles
       allocated to all SSM2603 devices in the list */
    for (nIndex = 0; nIndex < ADI_BF506ADC1_NUM_DEVICES; nIndex++)
    {
        if (hPhysicalDevice == (ADI_DEV_PDD_HANDLE)&goaBf506fadc1Device[nIndex])
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

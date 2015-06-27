/* $Revision: 3492 $
 * $Date: 2010-10-07 09:34:20 -0400 (Thu, 07 Oct 2010) $ 
******************************************************************************
Copyright (c), 2009 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title: ADI System Services - ADC Control Module (ACM)

Description:

    This is the primary source file for ADC Control Module (ACM)

    ACM is supported for

        ADSP-BF504, ADSP-BF504F, ADSP-BF506F

*****************************************************************************/

/*=============  I N C L U D E S   =============*/

/* System Service includes  */
#include <services/services.h>
/* ACM MMR definition includes */
#include "adi_acm_reg.h"

/* ACM is available only for MoY class devices*/
#if !defined(__ADSP_MOY__)
#error "ADC Control Module (ACM) is not supported for this processor"
#endif

/*=============  D E F I N E S  =============*/

/* Number of ACM devices in the system */
#define     ADI_ACM_NUM_DEVICES                 (sizeof(gaoAcmDevice)/sizeof(ADI_ACM_DEF))
/* Maximum limit for event number */
#define     ADI_ACM_MAX_EVENT_NUM               (15u)
/* Maximum limit for external peripheral select value */
#define     ADI_ACM_MAX_EXT_PERIPHERAL_SEL      (1u)
/* Maximum limit for Timer trigger select value */
#define     ADI_ACM_MAX_TMR_TRIGGER_SEL         (3u)
/* Maximum limit for ADC Channel select value */
#define     ADI_ACM_MAX_ADC_CHANNEL_SEL         (7u)
/* Maximum limit for ACM Clock divisor value */
#define     ADI_ACM_MAX_ACM_CLKDIV              (0xFFu)
/* Maximum limit for ACM Setup cycles */
#define     ADI_ACM_MAX_ACM_SETUP_CYCLES        (256u)
/* Maximum limit for ACM Chip Select/Start Conversion (CS) width */
#define     ADI_ACM_MAX_ACM_CS_WIDTH            (256u)
/* Maximum limit for ACM Hold Cycles */
#define     ADI_ACM_MAX_ACM_HOLD_CYCLES         (15u)
/* Maximum limit for ACM Zero Cycles */
#define     ADI_ACM_MAX_ACM_ZERO_CYCLES         (15u)

/*==============  D A T A   S T R U C T U R E S  ===============*/

/*
** Structure to handle an ACM device instance
*/
typedef struct __AdiAcmDef
{

    /* 'true' when device is already in use, 'false' otherwise */
    bool                    bDeviceInUse;
    /* Application supplied callback function */
    ADI_DCB_CALLBACK_FN     pfCallback;
    /* Callback parameter for this device supplied by application */
    void                    *pCallbackParam;
    /* DCB Manager Handle */
    ADI_DCB_HANDLE          hDcbManager;
    /* Peripheral ID for ACM event completion (status) interrupts */
    ADI_INT_PERIPHERAL_ID   eAcmStatusIntID;
    /* Peripheral ID for  ACM event miss (error) interrupts */
    ADI_INT_PERIPHERAL_ID   eAcmErrorIntID;

} ADI_ACM_DEF;

/*=============  D A T A  =============*/

/*****************
 Moy  (ADSP-BF50x)
******************/

#if defined(__ADSP_MOY__)

/* Create ACM device instance(s) */
static ADI_ACM_DEF   gaoAcmDevice[] =
{
    {
        /* Device not in use */
        false,
        /* Callbacks not enabled */
        NULL,
        /* Callback Parameter */
        NULL,
        /* Live callbacks */
        NULL,
        /* Peripheral ID for ACM event completion (status) interrupts */
        ADI_INT_ACM,
        /* Peripheral ID for ACM event miss (error) interrupts */
        ADI_INT_ACM_STATUS
    },
};

#endif

/*
** Local Function Prototypes
*/

/* Resets ACM Registers */
static void ResetAcmRegs (
    ADI_ACM_DEF         *poDevice
);

/* Processes the given ADC Specific control command */
static ADI_ACM_RESULT  ProcessControlCommand (
    ADI_ACM_DEF         *poDevice,
    ADI_ACM_COMMAND     eCommandID,
    void                *Value
);

/* Configure all fields related to an event */
static ADI_ACM_RESULT   ConfigEventParams (
    ADI_ACM_DEF             *poDevice,
    ADI_ACM_EVENT_PARAMS    *poConfigParams
);

/* Posts ACM callback to application or DCB manager */
static void PostAcmCallback(
    ADI_ACM_DEF     *poDevice,
    ADI_ACM_EVENT   eEventID,
    u8              nEventNumber
);

/* ACM event completion (status) Interrupt handler */
static ADI_INT_HANDLER(AcmStatusIntHandler);

/* ACM event miss (error) Interrupt handler */
static ADI_INT_HANDLER(AcmErrorIntHandler);

/* Static functions for Moy class devices */
#if defined(__ADSP_MOY__)

/* Configures the general purpose ports for Moy class devices for ACM usage */
static ADI_ACM_RESULT SetPortControl (
    ADI_ACM_DEF         *poDevice
);

#endif /* __ADSP_MOY__ */

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

/* Validates ACM Device handle */
static ADI_ACM_RESULT   ValidateDeviceHandle(
    ADI_ACM_DEV_HANDLE      hAcmDevice
);

#endif /* ADI_SSL_DEBUG */

/*=============  C O D E  =============*/

/*********************************************************************

    Function: adi_acm_Open

        Opens an ADC Control Module (ACM) device

    Parameters:
        nDeviceNumber       - Physical ACM Device number to open
        pCriticalRegionArg  - Critical region parameter
        phAcmDevice         - Pointer to location to store handle
                              to this ACM device

*********************************************************************/
ADI_ACM_RESULT  adi_acm_Open (
    u32                     nDeviceNumber,
    void                    *pCriticalRegionArg,
    ADI_ACM_DEV_HANDLE      *phAcmDevice
)
{
    /* Result code */
    ADI_ACM_RESULT      eResult = ADI_ACM_RESULT_SUCCESS;
    /* Pointer to the device instance we will be working on */
    ADI_ACM_DEF         *poDevice;
    /* Exit critical region parameter */
    void                *pExitCriticalArg;

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

    /* IF (Device number exceeds the number of devices supported) */
    if (nDeviceNumber >= ADI_ACM_NUM_DEVICES)
    {
        /* Report error (Invalid Device number) */
        eResult = ADI_ACM_RESULT_BAD_DEVICE_NUMBER;
    }

    /* IF (the device number is valid) */
    if (eResult == ADI_ACM_RESULT_SUCCESS)

#endif

    {
        /* assume that the device client requesting for is
           already in use  */
        eResult = ADI_ACM_RESULT_ALREADY_IN_USE;
        /* ACM device instance to work on */
        poDevice = &gaoAcmDevice[nDeviceNumber];

        /* Protect this section of code - entering a critical region */
        pExitCriticalArg = adi_int_EnterCriticalRegion(pCriticalRegionArg);

        /* IF (Device is not in use) */
        if (poDevice->bDeviceInUse == false)
        {
            /* Reserve the device for this client */
            poDevice->bDeviceInUse = true;
            eResult = ADI_ACM_RESULT_SUCCESS;
        }

        /* Exit the critical region */
        adi_int_ExitCriticalRegion(pExitCriticalArg);

        /* IF (Successfully reserved device for this client) */
        if (eResult == ADI_ACM_RESULT_SUCCESS)
        {
            /* Clear Callback function */
            poDevice->pfCallback        = NULL;
            /* Clear Callback parameter */
            poDevice->pCallbackParam    = NULL;
            /* Clear DCB Manager Handle */
            poDevice->hDcbManager       = NULL;

            /* Reset ACM registers */
            ResetAcmRegs(poDevice);

            /* Pass ACM device handle */
            *phAcmDevice = (ADI_ACM_DEV_HANDLE *) poDevice;
        }
        /* ELSE (Device is already in use) */
        else
        {
            phAcmDevice = NULL;
        }
    }

    return (eResult);
}

/*********************************************************************

    Function: adi_acm_Close

        Closes an ADC Control Module (ACM) device

    Parameters:
        hAcmDevice - Handle to an active ACM device to close

*********************************************************************/
ADI_ACM_RESULT  adi_acm_Close (
    ADI_ACM_DEV_HANDLE      hAcmDevice
)
{
    /* Result code */
    ADI_ACM_RESULT      eResult = ADI_ACM_RESULT_SUCCESS;
    /* Pointer to the device instance we will be working on */
    ADI_ACM_DEF         *poDevice;

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

    /* IF (Device handle is valid) */
    eResult = ValidateDeviceHandle (hAcmDevice);

    /* IF (the device handle is valid) */
    if (eResult == ADI_ACM_RESULT_SUCCESS)

#endif

    {
        /* Get device instance to close */
        poDevice = (ADI_ACM_DEF *) hAcmDevice;

        /* Reset ACM registers */
        ResetAcmRegs(poDevice);

        /* Un-hook all ACM interrupt sources */
        adi_acm_Control (hAcmDevice,
                         ADI_ACM_CMD_SET_CALLBACK_FN,
                         NULL);

        /* Mark ACM device as not in use */
        poDevice->bDeviceInUse = false;
    }

    return (eResult);
}

/*********************************************************************

    Function: ResetAcmRegs

        Resets ACM registers

    Parameters:
        poDevice - ACM Device instance to reser

    Returns:
        None

*********************************************************************/
static void ResetAcmRegs (
    ADI_ACM_DEF     *poDevice
)
{

    /* loop variable */
    u8  i;
    /* Clear ACM Control register */
    ACM_CTRL_REG_SET_VAL(0);
    /* Disable all interrupts */
    ACM_IMASK_REG_DISABLE_ALL_INT;
    ACM_EMASK_REG_DISABLE_ALL_INT;
    /* Clear event conversion/miss status registers */
    ACM_ES_REG_CLEAR_ALL_EVENT_DONE;
    ACM_ES_REG_CLEAR_ALL_EVENT_MISSED;

    /* Clear Timing configuration registers */
    ACM_TC0_REG_SET_VAL(0);
    ACM_TC1_REG_SET_VAL(0);

    /* FOR (all events) */
    for (i = 0;
         i < ADI_ACM_MAX_EVENT_NUM;
         i++)
    {
        /* Clear Event Control Register */
        ACM_ERx_REG_SET_VAL (i, 0);
        /* Clear Event Time Register */
        ACM_ETx_REG_SET_VAL (i, 0);
    }
}

/*********************************************************************

    Function: adi_acm_Control

        Set/Sense ADC Control Module (ACM) device specific settings

    Parameters:
        hAcmDevice - Handle to an active ACM device to set/sense
        nCommandID - ACM specific command ID to process
        Value      - Command specific value

*********************************************************************/
ADI_ACM_RESULT  adi_acm_Control (
    ADI_ACM_DEV_HANDLE      hAcmDevice,
    ADI_ACM_COMMAND         eCommandID,
    void                    *Value
)
{
    /* Result code */
    ADI_ACM_RESULT          eResult = ADI_ACM_RESULT_SUCCESS;
    /* Pointer to the device instance we will be working on */
    ADI_ACM_DEF             *poDevice = (ADI_ACM_DEF *) hAcmDevice;
    /* pointer to command pair */
    ADI_ACM_CMD_VALUE_PAIR  *pPair;

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

    /* IF (Device handle is valid) */
    eResult = ValidateDeviceHandle (hAcmDevice);

    /* IF (the device handle is valid) */
    if (eResult == ADI_ACM_RESULT_SUCCESS)

#endif /* ADI_SSL_DEBUG */

    {

        /* CASE OF (Command ID) */
        switch (eCommandID)
        {
            /* CASE (Process a command table) */
            case ADI_ACM_CMD_TABLE:

                /* avoid casts */
                pPair = (ADI_ACM_CMD_VALUE_PAIR *)Value;

                /* Process each command pair */
                while ((pPair->eCommandID != ADI_ACM_CMD_END) &&\
                       (eResult == ADI_ACM_RESULT_SUCCESS))
                {
                    /* Process this command pair */
                    eResult = ProcessControlCommand (poDevice, pPair->eCommandID, pPair->Value);
                    /* Move to next command pair */
                    pPair++;
                }
                break;

            /* CASE (Process a command pair) */
            case ADI_ACM_CMD_PAIR:

                /* avoid casts */
                pPair = (ADI_ACM_CMD_VALUE_PAIR *)Value;
                /* process the command pair */
                eResult = ProcessControlCommand(poDevice, pPair->eCommandID, pPair->Value);
                break;

            /* Other commands */
            default:
                /* Process this command */
                eResult = ProcessControlCommand (poDevice, eCommandID, Value);
                break;
        }

    }

    return (eResult);
}

/*********************************************************************

    Function: ProcessControlCommand

        Processes the given ADC Specific control command

    Parameters:
        poDevice    - Pointer to ACM Device instance to work on
        nCommandID  - ACM specific command ID to process
        Value       - Command specific value

*********************************************************************/
static ADI_ACM_RESULT  ProcessControlCommand (
    ADI_ACM_DEF         *poDevice,
    ADI_ACM_COMMAND     eCommandID,
    void                *Value
)
{
    /* loop variable */
    u16                     i;
    /* Pointer to an array of event numbers */
    u8                      (*pEventArray)[] = NULL;
    /* Variable to get SCLK/CCLK */
    u32                     nfsclk, nTemp1, nTemp2;
    /* Result code */
    ADI_ACM_RESULT          eResult = ADI_ACM_RESULT_SUCCESS;
    /* Pointer to Timer configuration instance */
    ADI_ACM_TMR_CONFIG      *poTmrConfig;
    /* Pointer to event list instance */
    ADI_ACM_EVENT_LIST      *poEventList;
    /* Pointer to event status instance */
    ADI_ACM_EVENT_STAT      *poEventStat;
    /* Pointer to event config instance */
    ADI_ACM_EVENT_CONFIG    *pEventConfig;
    /* Pointer to an array of event configuration values */
    ADI_ACM_EVENT_CONFIG    (*pEventConfigArray)[] = NULL;
    /* Pointer to an array of event parameter instance */
    ADI_ACM_EVENT_PARAMS    (*pEventParams)[] = NULL;

    /* CASE OF (Command ID) */
    switch (eCommandID)
    {
        /* CASE (End of command table) */
        case ADI_ACM_CMD_END:
            break;

        /*
        ** ACM Register Configuration commands
        */

        /* CASE (Set ACM Control register value) */
        case ADI_ACM_CMD_SET_CTRL_REG:

            /* Configure ACM control register to given value */

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* IF (Enable ACM) */
            if ((u16)((u32)Value) & ACM_CTRL_REG_ACM_EN_FLD_MASK)
            {
                /* IF (Interrupt(s) enabled with out a valid callback function */
                if (((ACM_IMASK_REG_GET_VAL != 0) || (ACM_EMASK_REG_GET_VAL != 0)) &&\
                    (poDevice->pfCallback == NULL))
                {
                    /* Report error (Callback function is invalid) */
                    eResult = ADI_ACM_RESULT_CALLBACK_FN_INALID;
                    break;
                }
            }

#endif /* ADI_SSL_DEBUG */

/* Moy has a 16-bit ACM control register */
#if defined (__ADSP_MOY__)

            /* IF (Enable ACM) */
            if ((u16)((u32)Value) & ACM_CTRL_REG_ACM_EN_FLD_MASK)
            {
                /* Configure port pins for ACM use */
                eResult = SetPortControl(poDevice);
            }

            /* Update ACM Control register */
            ACM_CTRL_REG_SET_VAL((u16)((u32)Value));

#endif /* __ADSP_MOY__ */

            break;

        /* CASE (Set ACM Event Control register value) */
        case ADI_ACM_CMD_SET_EVENT_CTRL_REG:

            /* Type-cast to event config instance */
            pEventConfig = (ADI_ACM_EVENT_CONFIG *)Value;

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* Validate Event ID */
            if (pEventConfig->nEventNumber > ADI_ACM_MAX_EVENT_NUM)
            {
                /* Report Failure (Event number is invalid) */
                eResult = ADI_ACM_RESULT_EVENT_NUMBER_INVALID;
                break;
            }

#endif /* ADI_SSL_DEBUG */

            /* Configure ACM Event control register to given value */
            ACM_ERx_REG_SET_VAL(pEventConfig->nEventNumber,
                                (u16)pEventConfig->nConfigValue);

            break;

        /* CASE (Set ACM Event Time value register value) */
        case ADI_ACM_CMD_SET_EVENT_TIME_REG:

            /* Type-cast to event config instance */
            pEventConfig = (ADI_ACM_EVENT_CONFIG *)Value;

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* Validate Event ID */
            if (pEventConfig->nEventNumber > ADI_ACM_MAX_EVENT_NUM)
            {
                /* Report Failure (Event number is invalid) */
                eResult = ADI_ACM_RESULT_EVENT_NUMBER_INVALID;
                break;
            }

#endif /* ADI_SSL_DEBUG */

            /* Configure ACM Event Time value register */
            ACM_ETx_REG_SET_VAL(pEventConfig->nEventNumber,
                                pEventConfig->nConfigValue);

            break;

        /* CASE (Configure a a table of event numbers and its
                 corresponding control registers) */
        case ADI_ACM_CMD_SET_EVENT_CTRL_REG_TABLE:
        /* CASE (Configure a a table of event numbers and its
                 corresponding time value registers) */
        case ADI_ACM_CMD_SET_EVENT_TIME_REG_TABLE:

            /* Convert value to pointer to event config array */
            pEventConfigArray = (void *) &((ADI_ACM_EVENT_CONFIG_TABLE *)Value)->paConfig;

            /* FOR (all entries in the table) */
            for (i = 0;
                 i < ((ADI_ACM_EVENT_CONFIG_TABLE *)Value)->nNumEntries;
                 i++)
            {

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

                /* Validate Event ID */
                if ((*pEventConfigArray)[i].nEventNumber > ADI_ACM_MAX_EVENT_NUM)
                {
                    /* Report Failure (Event number is invalid) */
                    eResult = ADI_ACM_RESULT_EVENT_NUMBER_INVALID;
                    break;
                }

#endif /* ADI_SSL_DEBUG */

                /* IF (Configure Event control register) */
                if (eCommandID == ADI_ACM_CMD_SET_EVENT_CTRL_REG_TABLE)
                {
                    /* Configure ACM Event control register to given value */
                    ACM_ERx_REG_SET_VAL((*pEventConfigArray)[i].nEventNumber,
                                        (u16)(*pEventConfigArray)[i].nConfigValue);
                }
                /* ELSE (Configure Event Time register) */
                else
                {
                    /* Configure ACM Event Time value register */
                    ACM_ETx_REG_SET_VAL((*pEventConfigArray)[i].nEventNumber,
                                        (*pEventConfigArray)[i].nConfigValue);
                }
            }

            break;

        /*
        ** ACM Register Query commands
        */

        /* CASE (Get ACM Status register value) */
        case ADI_ACM_CMD_GET_STAT_REG:

            /* Get ACM Status register value */
            *((u32 *)Value) = ACM_STAT_REG_GET_VAL;

            break;

        /*
        ** ACM Register Field configuration commands
        */

        /* CASE (Enable/Disable ACM) */
        case ADI_ACM_CMD_ENABLE_ACM:

            /* IF (Enable ACM) */
            if ((bool)Value == true)
            {

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

                /* IF (Interrupt(s) enabled with out a valid callback function */
                if (((ACM_IMASK_REG_GET_VAL != 0) || (ACM_EMASK_REG_GET_VAL != 0)) &&\
                    (poDevice->pfCallback == NULL))
                {
                    /* Report error (Callback function is invalid) */
                    eResult = ADI_ACM_RESULT_CALLBACK_FN_INALID;
                    break;
                }

#endif /* ADI_SSL_DEBUG */

                /* Configure port pins for ACM use */
                eResult = SetPortControl(poDevice);
                /* Enable ACM */
                ACM_CTRL_FLD_ACM_ENABLE;
            }
            /* ELSE (Disable ACM) */
            else
            {
                ACM_CTRL_FLD_ACM_DISABLE;
            }

            break;

        /* CASE (Enable selected event) */
        case ADI_ACM_CMD_ENABLE_EVENT:

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* Validate Event ID */
            if ((u32)Value > ADI_ACM_MAX_EVENT_NUM)
            {
                /* Report Failure (Event number is invalid) */
                eResult = ADI_ACM_RESULT_EVENT_NUMBER_INVALID;
                break;
            }

#endif /* ADI_SSL_DEBUG */

            /* Enable event */
            ACM_ERx_REG_ENABLE_EVENT((u8)((u32)Value));

            break;

        /* CASE (Disable selected event) */
        case ADI_ACM_CMD_DISABLE_EVENT:

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* Validate Event ID */
            if ((u32)Value > ADI_ACM_MAX_EVENT_NUM)
            {
                /* Report Failure (Event number is invalid) */
                eResult = ADI_ACM_RESULT_EVENT_NUMBER_INVALID;
                break;
            }

#endif /* ADI_SSL_DEBUG */

            /* Disable event */
            ACM_ERx_REG_DISABLE_EVENT((u8)((u32)Value));

            break;

        /* CASE (Configure ACM Timer specific fields) */
        case ADI_ACM_CMD_CONFIG_TMR:

            /* Type-cast to timer configuration instance */
            poTmrConfig = (ADI_ACM_TMR_CONFIG *)Value;

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* Validate Timer Number */
            if (poTmrConfig->nTmrNumber > 1)
            {
                /* Report failure (Invalid timer number) */
                eResult = ADI_ACM_RESULT_TIMER_NUMBER_INVALID;
                break;
            }

            /* Validate Timer Trigger Select */
            if (poTmrConfig->nTriggerSelect > ADI_ACM_MAX_TMR_TRIGGER_SEL)
            {
                /* Report failure (Invalid timer configuration) */
                eResult = ADI_ACM_RESULT_TIMER_CONFIG_INVALID;
                break;
            }

            /* Validate Timer Trigger Polarity */
            if (poTmrConfig->nTriggerPolarity > 1)
            {
                /* Report failure (Invalid timer configuration) */
                eResult = ADI_ACM_RESULT_TIMER_CONFIG_INVALID;
                break;
            }

#endif /* ADI_SSL_DEBUG */

            /* IF (ACM is already enabled) */
            if (ACM_CTRL_FLD_IS_ACM_ENABLED)
            {
                /* Report Failure (Changing Timer configuration is not permitted) */
                eResult = ADI_ACM_RESULT_COMMAND_NOT_PERMITTED;
                break;
            }

            /* IF (Configure Timer 0) */
            if (poTmrConfig->nTmrNumber == 0)
            {
                /* IF (Enable Timer 0) */
                if (poTmrConfig->bEnable == true)
                {
                    ACM_CTRL_FLD_TMR0_ENABLE;
                }
                /* ELSE (Disable Timer 0) */
                else
                {
                    ACM_CTRL_FLD_TMR0_DISABLE;
                }

                /* Set Trigger select */
                ACM_CTRL_FLD_TGR0_SET (poTmrConfig->nTriggerSelect);

                /* IF (Trigger on raising edge) */
                if (poTmrConfig->nTriggerPolarity == 0)
                {
                    ACM_CTRL_FLD_TGR0_RAISE;
                }
                /* ELSE (Trigger must be on falling edge) */
                else
                {
                    ACM_CTRL_FLD_TGR0_FALL;
                }
            }
            /* ELSE (Configure Timer 1) */
            else
            {
                /* IF (Enable Timer 1) */
                if (poTmrConfig->bEnable == true)
                {
                    ACM_CTRL_FLD_TMR1_ENABLE;
                }
                /* ELSE (Disable Timer 1) */
                else
                {
                    ACM_CTRL_FLD_TMR1_DISABLE;
                }

                /* Set Trigger select */
                ACM_CTRL_FLD_TGR1_SET (poTmrConfig->nTriggerSelect);

                /* IF (Trigger on raising edge) */
                if (poTmrConfig->nTriggerPolarity == 0)
                {
                    ACM_CTRL_FLD_TGR1_RAISE;
                }
                /* ELSE (Trigger must be on falling edge) */
                else
                {
                    ACM_CTRL_FLD_TGR1_FALL;
                }
            }

            break;

        /* CASE (Set Chip select / Start Conversion polarity) */
        case ADI_ACM_CMD_SET_CS_POL:

            /* IF (Active low CS) */
            if ((u32)Value == 0)
            {
                ACM_CTRL_FLD_CS_LOW;
            }
            /* ELSE (CS must be active high) */
            else
            {
                ACM_CTRL_FLD_CS_HIGH;
            }

            break;

        /* CASE (Set ADC Clock (ACLK) polarity after CS gets activated) */
        case ADI_ACM_CMD_SET_ADC_CLK_POL:

            /* IF (ACLK starts with Falling edge) */
            if ((u32)Value == 0)
            {
                ACM_CTRL_FLD_CLK_POL_FALL;
            }
            /* ELSE (ACLK must start with Raising edge) */
            else
            {
                ACM_CTRL_FLD_CLK_POL_RAISE;
            }

            break;

        /* CASE (Set External Peripheral Select) */
        case ADI_ACM_CMD_SET_EXT_PERIPHERAL_SEL:

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* Validate External Peripheral Select */
            if ((u32)Value > ADI_ACM_MAX_EXT_PERIPHERAL_SEL)
            {
                eResult = ADI_ACM_RESULT_EXT_PERIPHERAL_INVALID;
                break;
            }

#endif /* ADI_SSL_DEBUG */

            /* Configure External Peripheral select */
            ACM_CTRL_FLD_EPS_SET ((u32)Value);

            break;

        /* CASE (Enable event completion interrupt for given event number) */
        case ADI_ACM_CMD_ENABLE_EVENT_COMPLETE_INT:

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* IF (ACM is enabled and we don't have a valid callback function) */
            if (ACM_CTRL_FLD_IS_ACM_ENABLED && poDevice->pfCallback == NULL)
            {
                /* Report error (Callback function is invalid) */
                eResult = ADI_ACM_RESULT_CALLBACK_FN_INALID;
                break;
            }

#endif /* ADI_SSL_DEBUG */

            /* Enable interrupt for event completion */
            ACM_IMASK_REG_ENABLE_INT((u8)((u32)Value));

            break;

        /* CASE (Enable event missed interrupt for given event number) */
        case ADI_ACM_CMD_ENABLE_EVENT_MISS_INT:

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* IF (ACM is enabled and we don't have a valid callback function) */
            if (ACM_CTRL_FLD_IS_ACM_ENABLED && poDevice->pfCallback == NULL)
            {
                /* Report error (Callback function is invalid) */
                eResult = ADI_ACM_RESULT_CALLBACK_FN_INALID;
                break;
            }

#endif /* ADI_SSL_DEBUG */

            /* Enable interrupt for event miss */
            ACM_EMASK_REG_ENABLE_INT((u8)((u32)Value));

            break;

        /* CASE (Disable event completion interrupt for given event number) */
        case ADI_ACM_CMD_DISABLE_EVENT_COMPLETE_INT:

            /* Disable interrupt for event completion */
            ACM_IMASK_REG_DISABLE_INT((u8)((u32)Value));
            break;

        /* CASE (Disable event missed interrupt for given event number) */
        case ADI_ACM_CMD_DISABLE_EVENT_MISS_INT:

            /* Disable interrupt for event miss */
            ACM_EMASK_REG_DISABLE_INT((u8)((u32)Value));
            break;

        /* CASE (Configure all fields related to an Event) */
        case ADI_ACM_CMD_SET_EVENT_PARAMS:

            /* Configure all fields related to an event with given value */
            eResult = ConfigEventParams (poDevice, (ADI_ACM_EVENT_PARAMS *)Value);
            break;

        /* CASE (Configure a table of Events and its related fields) */
        case ADI_ACM_CMD_SET_EVENT_PARAMS_TABLE:

            /* Convert event parameter table pointer to an array */
            pEventParams = (void *) ((ADI_ACM_EVENT_PARAMS_TABLE *)Value)->paEventParams;

            /* FOR (all entries in the table) */
            for (i = 0;
                 i < ((ADI_ACM_EVENT_PARAMS_TABLE *)Value)->nNumEntries;
                 i++)
            {
                /* Configure all fields related to an event with given value */
                eResult = ConfigEventParams (poDevice, &(*pEventParams)[i]);

                /* IF (Failed to configure event fields) */
                if (eResult != ADI_ACM_RESULT_SUCCESS)
                {
                    break;
                }
            }
            break;

        /* CASE (Set ACM Clock frequency) */
        case ADI_ACM_CMD_SET_ACLK_FREQ:

            /* IF (Failed to get SCLK) */
            if (adi_pwr_GetFreq (&nTemp1, &nfsclk, &nTemp1) != ADI_PWR_RESULT_SUCCESS)
            {
                /* Report failure (Failed to get SCLK) */
                eResult = ADI_ADC_RESULT_GET_SCLK_FAILED;
            }
            /* ELSE (Successfully got SCLK) */
            else
            {

/* For Moy processor family */
#if defined (__ADSP_MOY__)

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

                /* Validate ACLK Frequency */
                if ((u32)Value > (nfsclk >> 1))
                {
                    /* Report Failure (ACLK is out of range and can't be supported) */
                    eResult = ADI_ACM_RESULT_ACLK_OUT_OF_RANGE;
                    break;
                }

#endif /* ADI_SSL_DEBUG */

#endif /* __ADSP_MOY__ */

                /* Calculate ADC Clock Divisor */
                i = ((nfsclk / ((u32)Value << 1)) - 1);

                /* Check if exact ACLK frequency can be derived from SCLK */
                if ((nfsclk / ((i+1) << 1)) != (u32)Value)
                {

                    /* Report Failure (ACLK can't be derived from present SCLK) */
                    eResult = ADI_ACM_RESULT_CANNOT_DERIVE_ACLK;
                    /* Make sure the divisor is set such that ACLK is
                       less than or equal to value issued by application */
                    i++;
                }

            } /* End of IF (Failed to get SCLK) else case */

            /* Update ACLK Divisor */
            ACM_TC0_REG_SET_CKDIV_FLD_VAL (i);

            break;

        /* CASE (Set ACM Clock Divisor) */
        case ADI_ACM_CMD_SET_ACLK_DIVISOR:

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* Validate Clock Divisor */
            if ((u32)Value > ADI_ACM_MAX_ACM_CLKDIV)
            {
                /* Report Failure (Clock Divisor Invalid) */
                eResult = ADI_ACM_RESULT_CLKDIV_INVALID;
                break;
            }

#endif /* ADI_SSL_DEBUG */

            /* Update ACLK Divisor */
            ACM_TC0_REG_SET_CKDIV_FLD_VAL ((u32)Value);

            break;

        /* CASE (Set ACM Setup Cycles) */
        case ADI_ACM_CMD_SET_SETUP_CYCLES:

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* Validate Setup Cycles */
            if ((u32)Value > ADI_ACM_MAX_ACM_SETUP_CYCLES)
            {
                /* Report Failure (Setup Cycles Invalid) */
                eResult = ADI_ACM_RESULT_SETUP_CYCLES_INVALID;
                break;
            }

#endif /* ADI_SSL_DEBUG */

            /* Update Setup Cycles */
            ACM_TC0_REG_SET_SC_FLD_VAL ((u32)Value - 1);

            break;

        /* CASE (Set Chip Select/Start Conversion (CS) Width) */
        case ADI_ACM_CMD_SET_CS_WIDTH:

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* Validate CS Width */
            if ((u32)Value > ADI_ACM_MAX_ACM_CS_WIDTH)
            {
                /* Report Failure (CS Width Invalid) */
                eResult = ADI_ACM_RESULT_CS_WIDTH_INVALID;
                break;
            }

#endif /* ADI_SSL_DEBUG */

            /* Update CS Width */
            ACM_TC1_REG_SET_CSW_FLD_VAL ((u32)Value - 1);

            break;

        /* CASE (Set Hold Cycles count) */
        case ADI_ACM_CMD_SET_HOLD_CYCLES:

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* Validate Hold Cycles */
            if ((u32)Value > ADI_ACM_MAX_ACM_HOLD_CYCLES)
            {
                /* Report Failure (Hold Cycle Invalid) */
                eResult = ADI_ACM_RESULT_HOLD_CYCLES_INVALID;
                break;
            }

#endif /* ADI_SSL_DEBUG */

            /* Update Hold Cycle count */
            ACM_TC1_REG_SET_HC_FLD_VAL ((u32)Value);

            break;

        /* CASE (Set Zero Cycles count) */
        case ADI_ACM_CMD_SET_ZERO_CYCLES:

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* Validate Zero Cycles */
            if ((u32)Value > ADI_ACM_MAX_ACM_ZERO_CYCLES)
            {
                /* Report Failure (Zero Cycle Invalid) */
                eResult = ADI_ACM_RESULT_ZERO_CYCLES_INVALID;
                break;
            }

#endif /* ADI_SSL_DEBUG */

            /* Update Zero Cycle count */
            ACM_TC1_REG_SET_ZC_FLD_VAL ((u32)Value);

            break;

        /*
        ** ACM Register Field Query commands
        */

        /* CASE (Get status of a particular event number) */
        case ADI_ACM_CMD_GET_EVENT_STAT:

            /* Type-cast to event stat */
            poEventStat = (ADI_ACM_EVENT_STAT *)Value;

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

            /* Validate Event ID */
            if (poEventStat->nEventNumber > ADI_ACM_MAX_EVENT_NUM)
            {
                /* Report Failure (Event number invalid) */
                eResult = ADI_ACM_RESULT_EVENT_NUMBER_INVALID;
                break;
            }

#endif /* ADI_SSL_DEBUG */

            /* Check event done status */
            poEventStat->bIsCompleted = ACM_ES_REG_IS_EVENT_DONE(poEventStat->nEventNumber);
            /* Check event miss status */
            poEventStat->bIsMissed = ACM_MS_REG_WAS_EVENT_MISSED(poEventStat->nEventNumber);
            /* Clear event done and miss status sticky bits */
            ACM_ES_REG_CLEAR_EVENT_DONE(poEventStat->nEventNumber);
            ACM_ES_REG_CLEAR_EVENT_MISSED(poEventStat->nEventNumber);

            /* IF (This is the current event) */
            if (poEventStat->nEventNumber == ACM_STAT_REG_GET_CURRENT_EVENT)
            {
                poEventStat->bIsCurrent = true;
            }
            /* ELSE (this is not current event) */
            else
            {
                poEventStat->bIsCurrent = false;
            }

            break;

        /* CASE (Query if all events related to current trigger is completed) */
        case ADI_ACM_CMD_GET_EVENT_COMPLETE_STAT:

            /* Get event completion status bit value */
            *((bool *)Value) = ACM_STAT_REG_IS_EVENT_COMPLETEED;

            break;

        /* CASE (Query if any events related to current trigger was missed) */
        case ADI_ACM_CMD_GET_EVENT_MISS_STAT:

            /* Get event miss status bit value */
            *((bool *)Value) = ACM_STAT_REG_IS_EVENT_MISSED;

            break;

        /* CASE (Get a list of event numbers that are completed/done) */
        case ADI_ACM_CMD_GET_EVENT_COMPLETE_LIST:
        /* CASE (Get a list of event numbers that were missed) */
        case ADI_ACM_CMD_GET_EVENT_MISS_LIST:

            /* Type-cast to event list */
            poEventList = (ADI_ACM_EVENT_LIST *)Value;
            /* Initialise the event list count */
            poEventList->nNumEntries = 0;
            /* Convert pointer to pointer to an array */
            pEventArray = (void *) & (poEventList->paEventList);

            /* For all events supported */
            for (i = 0;
                 i < ADI_ACM_MAX_EVENTS;
                 i++)
            {
                /* IF (Command is to get missed event list) */
                if (eCommandID == ADI_ACM_CMD_GET_EVENT_MISS_LIST)
                {
                    /* Check if the event number corresponding to the count was missed */
                    if (ACM_MS_REG_WAS_EVENT_MISSED(i) == true)
                    {
                        /* Save this event number to application provided location */
                        (* pEventArray)[poEventList->nNumEntries++] = i;
                        /* Clear event miss status bit corresponding to this event number */
                        ACM_ES_REG_CLEAR_EVENT_MISSED(i);
                    }
                }
                /* ELSE (Command is to get completed event list) */
                else
                {
                    /* Check if the event number corresponding to the count is done */
                    if (ACM_ES_REG_IS_EVENT_DONE(i) == true)
                    {
                        /* Save this event number to application provided location */
                        (* pEventArray)[poEventList->nNumEntries++] = i;
                        /* Clear event completion status bit corresponding to this event number */
                        ACM_ES_REG_CLEAR_EVENT_DONE(i);
                    }
                }
            }
            break;

        /*
        ** Callback control commands
        */

        /* CASE (Set Application callback function) */
        case ADI_ACM_CMD_SET_CALLBACK_FN:

            /* Get the ACM event completion (status) interrupt IVG */
            adi_int_SICGetIVG(poDevice->eAcmStatusIntID, &nTemp1);
            /* Get the ACM event miss (error) interrupt IVG */
            adi_int_SICGetIVG(poDevice->eAcmErrorIntID, &nTemp2);

            /* IF (Application is trying to issue a new callback function
                   or disable interrupts) */
            if (poDevice->pfCallback != (ADI_DCB_CALLBACK_FN )Value)
            {
                /* IF (we already have a valid callback function) */
                if (poDevice->pfCallback != NULL)
                {
                    /* Unhook ACM Status Interrupt Handler */
                    if (adi_int_CECUnhook(nTemp1, AcmStatusIntHandler, poDevice) != ADI_INT_RESULT_SUCCESS)
                    {
                        /* Report Failure (Failed to unhook ACM interrupt) */
                        eResult = ADI_ACM_RESULT_INT_UNHOOK_FAILED;
                    }

                    /* Unhook ACM Error Interrupt Handler */
                    if (adi_int_CECUnhook(nTemp2, AcmErrorIntHandler, poDevice) != ADI_INT_RESULT_SUCCESS)
                    {
                        /* Report Failure (Failed to unhook ACM interrupt) */
                        eResult = ADI_ACM_RESULT_INT_UNHOOK_FAILED;
                    }
                }
            }

            /* IF (Failed to unhook interrupts) */
            if (eResult != ADI_ACM_RESULT_SUCCESS)
            {
                break;
            }
            /* ELSE IF (Application is trying to disable callbacks/interrupts) */
            else if ((ADI_DCB_CALLBACK_FN )Value == NULL)
            {
                /* Disable all interrupts */
                ACM_IMASK_REG_DISABLE_ALL_INT;
                ACM_EMASK_REG_DISABLE_ALL_INT;
            }
            /* ELSE (Application must be trying to enable callbacks/interrupts) */
            else
            {
                /* Enable ACM Status interrupt through the SIC */
                if (adi_int_SICEnable(poDevice->eAcmStatusIntID) != ADI_INT_RESULT_SUCCESS)
                {
                    /* Report Failure (Failed to hook ACM interrupt) */
                    eResult = ADI_ACM_RESULT_INT_HOOK_FAILED;
                    break;
                }

                /* Enable ACM Error interrupt through the SIC */
                if (adi_int_SICEnable(poDevice->eAcmErrorIntID) != ADI_INT_RESULT_SUCCESS)
                {
                    /* Report Failure (Failed to hook ACM interrupt) */
                    eResult = ADI_ACM_RESULT_INT_HOOK_FAILED;
                    break;
                }

                /* Hook ACM Status Interrupt Handler with nesting enabled */
                if (adi_int_CECHook(nTemp1, AcmStatusIntHandler, poDevice, true) != ADI_INT_RESULT_SUCCESS)
                {
                    /* Report Failure (Failed to hook ACM interrupt) */
                    eResult = ADI_ACM_RESULT_INT_HOOK_FAILED;
                    break;
                }

                /* Hook ACM Error Interrupt Handler with nesting enabled */
                if (adi_int_CECHook(nTemp2, AcmErrorIntHandler, poDevice, true) != ADI_INT_RESULT_SUCCESS)
                {
                    /* Unhook ACM Status Interrupt Handler */
                    adi_int_CECUnhook(nTemp1, AcmStatusIntHandler, poDevice);
                    /* Report Failure (Failed to hook ACM interrupt) */
                    eResult = ADI_ACM_RESULT_INT_HOOK_FAILED;
                    break;
                }

            } /* End of IF (we already have a valid callback function) */

            /* Update application callback function */
            poDevice->pfCallback = (ADI_DCB_CALLBACK_FN) Value;

            break;

        /* CASE (Set Application callback parameter) */
        case ADI_ACM_CMD_SET_CALLBACK_PARAM:

            /* Update callback parameter */
            poDevice->pCallbackParam = Value;

            break;

        /* CASE (Set DCB Manager handle) */
        case ADI_ACM_CMD_SET_DCB_MANAGER_HANDLE:

            /* Update DCB Manager handle */
            poDevice->hDcbManager = (ADI_DCB_HANDLE) Value;

            break;

        /* DEFAULT (Command must be invalid) */
        default:

            /* Report Failure (Command Invalid) */
            eResult = ADI_ACM_RESULT_COMMAND_INVALID;
            break;

    } /* End CASE OF (Command ID) */

    return (eResult);

}

/*********************************************************************

    Function: ConfigEventParams

        Configure all fields related to an event

    Parameters:
        poDevice        - ACM Device Instance to work on
        poConfigParams  - Pointer to Event - parameter instance that holds
                          configuration settings

*********************************************************************/
static ADI_ACM_RESULT   ConfigEventParams (
    ADI_ACM_DEF             *poDevice,
    ADI_ACM_EVENT_PARAMS    *poConfigParams
)
{
    /* Result code */
    ADI_ACM_RESULT  eResult = ADI_ACM_RESULT_SUCCESS;

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)

    /* Validate Event ID */
    if (poConfigParams->nEventNumber > ADI_ACM_MAX_EVENT_NUM)
    {
        /* Report Failure (Event number invalid) */
        eResult = ADI_ACM_RESULT_EVENT_NUMBER_INVALID;
    }
    /* Validate ADC Channel Select */
    else if (poConfigParams->nAdcCannelSelect > ADI_ACM_MAX_ADC_CHANNEL_SEL)
    {
        /* Report Failure (Event parameter select is invalid) */
        eResult = ADI_ACM_RESULT_EVENT_PARAM_INVALID;
    }


    /* IF (Successfully validated the Event - parameter instance */
    if (eResult == ADI_ACM_RESULT_SUCCESS)

#endif /* ADI_SSL_DEBUG */
    {

        /* Set ADC Channel select for selected event */
        ACM_ERx_REG_SET_ADC_CH_SEL (poConfigParams->nEventNumber,
                                    poConfigParams->nAdcCannelSelect);

        /* IF (Clear ADC Range) */
        if (poConfigParams->nRange == 0)
        {
            ACM_ERx_REG_CLEAR_ADC_RANGE (poConfigParams->nEventNumber);
        }
        /* ELSE (Set ADC Range) */
        else
        {
            ACM_ERx_REG_SET_ADC_RANGE (poConfigParams->nEventNumber);
        }

        /* IF (Clear ADC Logic) */
        if (poConfigParams->nLogic == 0)
        {
            ACM_ERx_REG_CLEAR_ADC_LOGIC (poConfigParams->nEventNumber);
        }
        /* ELSE (Set ADC Logic) */
        else
        {
            ACM_ERx_REG_SET_ADC_LOGIC (poConfigParams->nEventNumber);
        }

        /* Configure ACM Event Time value register */
        ACM_ETx_REG_SET_VAL(poConfigParams->nEventNumber,
                            poConfigParams->nTime);

        /* IF (Enable Event) */
        if (poConfigParams->bEnable == true)
        {
            /* Enable this event */
            ACM_ERx_REG_ENABLE_EVENT(poConfigParams->nEventNumber);
        }
        /* ELSE (Disable Event) */
        else
        {
            /* Disable event */
            ACM_ERx_REG_DISABLE_EVENT(poConfigParams->nEventNumber);
        }
    }

    return (eResult);

}

/*********************************************************************

    Function:   AcmStatusIntHandler

        Processes ACM event completion (status) interrupts

*********************************************************************/
static ADI_INT_HANDLER(AcmStatusIntHandler)
{

    /* Loop variable */
    u8              i;
    /* Pointer to ACM Device Instance */
    ADI_ACM_DEF     *poDevice = (ADI_ACM_DEF *) ClientArg;
    /* Location to hold event complete status register value */
    u32             nCompleteStatusReg;

    /* Get Event Complete status register value */
    nCompleteStatusReg = ACM_ES_REG_GET_VAL;
    /* Ignore the status of all events whose interrupts are masked */
    nCompleteStatusReg &= ACM_IMASK_REG_GET_VAL;

    /* IF (We've an event completion to report) */
    if (nCompleteStatusReg != 0)
    {
        /* For all events supported by ACM */
        for (i = 0;
             i < ADI_ACM_MAX_EVENTS;
             i++)
        {
            /* IF (Event complete) */
            if (nCompleteStatusReg & 1)
            {
                /* Post event complete callback to Applicaiton */
                PostAcmCallback (poDevice, ADI_ACM_EVENT_EVENT_COMPLETE, i);
                /* Clear Event Complete status bit */
                ACM_ES_REG_CLEAR_EVENT_DONE (i);
            }

            /* Shift to next event status */
            nCompleteStatusReg >>= 1;

        } /* End of For (all events supported by ACM) */

    } /* End of if (We've an event completion / miss to report) */

    return(ADI_INT_RESULT_PROCESSED);

}

/*********************************************************************

    Function:   AcmErrorIntHandler

        Processes ACM event miss (error) interrupts

*********************************************************************/
static ADI_INT_HANDLER(AcmErrorIntHandler)
{

    /* Loop variable */
    u8              i;
    /* Pointer to ACM Device Instance */
    ADI_ACM_DEF     *poDevice = (ADI_ACM_DEF *) ClientArg;
    /* Location to hold event miss status register value */
    u32             nMissStatusReg;

    /* Get Event Miss status register value */
    nMissStatusReg = ACM_MS_REG_GET_VAL;
    /* Ignore the status of all events whose interrupts are masked */
    nMissStatusReg &= ACM_EMASK_REG_GET_VAL;

    /* IF (We've an event miss to report) */
    if (nMissStatusReg != 0)
    {
        /* For all events supported by ACM */
        for (i = 0;
             i < ADI_ACM_MAX_EVENTS;
             i++)
        {
            /* IF (Event Missed) */
            if (nMissStatusReg & 1)
            {
                /* Post event miss callback to Applicaiton */
                PostAcmCallback (poDevice, ADI_ACM_EVENT_EVENT_MISS, i);
                /* Clear Event Miss status bit */
                ACM_ES_REG_CLEAR_EVENT_MISSED (i);
            }

            /* Shift to next event status */
            nMissStatusReg >>= 1;

        } /* End of For (all events supported by ACM) */

    } /* End of if (We've an event completion / miss to report) */

    return(ADI_INT_RESULT_PROCESSED);

}

/*********************************************************************

    Function:   PostAcmCallback

        Posts ACM callback to application or DCB manager

*********************************************************************/
static void PostAcmCallback(
    ADI_ACM_DEF     *poDevice,
    ADI_ACM_EVENT   eEventID,
    u8              nEventNumber
)
{

    /* IF (we've a valid callback function) */
    if (poDevice->pfCallback != NULL)
    {
        /* IF (DCB Enabled) */
        if (poDevice->hDcbManager != NULL)
        {
            /* Queue callback to DCB Manager */
            adi_dcb_Post (poDevice->hDcbManager,
                          0,
                          poDevice->pfCallback,
                          poDevice->pCallbackParam,
                          eEventID,
                          (void *) nEventNumber);
        }
        /* ELSE (Callbacks are live) */
        else
        {
            poDevice->pfCallback (poDevice->pCallbackParam,
                                  eEventID,
                                  (void *) nEventNumber);
        }

    } /* End of IF (we've a valid callback function) */

}

/* Static functions for Moy class devices */
#if defined(__ADSP_MOY__)
/*********************************************************************

    Function: SetPortControl

        Configures the general purpose ports for Moy class devices for
        ACM usage.

    Parameters:
        poDevice - ACM Device Instance to work on

*********************************************************************/
static ADI_ACM_RESULT SetPortControl (
    ADI_ACM_DEF         *poDevice
)
{
    /* Result code */
    ADI_ACM_RESULT  eResult = ADI_ACM_RESULT_SUCCESS;
    /* Port control directives */
    ADI_PORTS_DIRECTIVE     aeAcmPortDirectives [] =
    {
        ADI_PORTS_DIRECTIVE_ACM_A0,
        ADI_PORTS_DIRECTIVE_ACM_A1,
        ADI_PORTS_DIRECTIVE_ACM_A2,
        ADI_PORTS_DIRECTIVE_ACM_RANGE,
        ADI_PORTS_DIRECTIVE_ACM_SE_DIFF
    };

    /* IF (Failed to configure ports for ACM use) */
    if (adi_ports_Configure(aeAcmPortDirectives,
                            (sizeof (aeAcmPortDirectives)/
                             sizeof (aeAcmPortDirectives[0]))) != ADI_PORTS_RESULT_SUCCESS)
    {
        /* Report Failure (Port control error) */
        eResult = ADI_ACM_RESULT_PORT_CONTROL_FAILED;
    }

    return (eResult);
}

#endif /* __ADSP_MOY__ */

/* for Debug build only - check for errors if required  */
#if defined(ADI_SSL_DEBUG)
/*********************************************************************

    Function: ValidateDeviceHandle

        Validates ACM Device handle

    Parameters:
        hAcmDevice - ACM Device handle to validate

*********************************************************************/
static ADI_ACM_RESULT   ValidateDeviceHandle(
    ADI_ACM_DEV_HANDLE      hAcmDevice
)
{
    /* Loop variable */
    u8              i;
    /* Return value - insure the client has provided a valid Handle */
    ADI_ACM_RESULT  eResult = ADI_ACM_RESULT_BAD_HANDLE;

    /* Compare the given handle with handles allocated to all ACM devices in the list */
    for (i = 0;
         i < ADI_ACM_NUM_DEVICES;
         i++)
    {
        if (hAcmDevice == (ADI_ACM_DEV_HANDLE) &gaoAcmDevice[i])
        {
            /* Given Deviec Handle is valid. quit this loop */
            eResult = ADI_ACM_RESULT_SUCCESS;
            break;
        }
    }

    return (eResult);
}

#endif /* ADI_SSL_DEBUG */

/*****/

/*
**
** EOF:
**
*/

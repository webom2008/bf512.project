/* $Revision: 3492 $
 * $Date: 2010-10-07 09:34:20 -0400 (Thu, 07 Oct 2010) $ 
******************************************************************************
Copyright (c), 2002-2009 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title: Sharp LQ043T1DG01 LCD driver

Description: This is the primary source file for Sharp LQ043T1DG01 LCD driver

*****************************************************************************/

/*=============  I N C L U D E S   =============*/

/* system service includes */
#include <services/services.h>
/* Device manager includes */
#include <drivers/adi_dev.h>
/* EPPI driver includes */
#include <drivers/eppi/adi_eppi.h>
/* SHARP LQ043T1DG01 LCD driver includes */
#include <drivers/lcd/sharp/adi_lq043t1dg01.h>

/*=============  D E F I N E S  =============*/

/*
** Number of SHARP LQ043T1DG01 LCD(s) in the system
*/
#define ADI_NUM_LQ043T1DG01_DEVICES                 (sizeof(_gaLq043t1dg01Device)/\
                                                     sizeof(ADI_LQ043T1DG01))

/*
** SHARP LQ043T1DG01 LCD operating frequency (8MHz)
*/
#define ADI_LQ043T1DG01_FREQ                        8000000U

/*
** Default PPI Device Number to use
*/
#define ADI_LQ043T1DG01_PPI_DEV_DEFAULT             0U

/*
** Default PPI Device Number to use
*/
#define ADI_LQ043T1DG01_DISP_TIMER_DEFAULT          ADI_TMR_GP_TIMER_2

/*
** Default EPPI Control register configuration mode to use
*/
#define ADI_LQ043T1DG01_EPPI_CTRL_REG_MODE_DEFAULT  ADI_LQ043T1DG01_EPPI_CTRL_REG_AUTO_CONFIG_DISABLED

/*
** Enumeration for DISP signal status
*/
typedef enum
{
    ADI_LQ043T1DG01_DISP_START_IN_PROGRESS,     /* DISP start generation under progress */
    ADI_LQ043T1DG01_DISP_ASSERTED,              /* DISP signal in high state            */
    ADI_LQ043T1DG01_DISP_END_IN_PROGRESS,       /* DISP end generation under progress   */
    ADI_LQ043T1DG01_DISP_DISABLED,              /* DISP signal is disabled              */
} ADI_LQ043T1DG01_DISP;

/*
** Sharp LQ043T1DG01 LCD device structure
*/
typedef struct
{

    /* 'true' when device is in use, 'false' when device is free */
    bool                                bIsDeviceInUse;

    /* 'true' when Timer & Flag ID for DISP Signal generation is set
       'false' otherwise */
    bool                                bIsDispTimerFlagSet;

    /*PPI Device Number to use */
    u8                                  nPpiDeviceNumber;

    /* Device Manager Handle */
    ADI_DEV_MANAGER_HANDLE              hDeviceManager;

    /* Handle to the underlying PPI device driver */
    ADI_DEV_PDD_HANDLE                  hPpiDevice;

    /* Device Handle allocated by the Device Manager */
    ADI_DEV_DEVICE_HANDLE               hDevice;

    /* DMA Manager Handle */
    ADI_DMA_MANAGER_HANDLE              hDmaManager;

    /* Deferred Callback Manager Handle */
    ADI_DCB_HANDLE                      hDcbManager;

    /* Pointer to callback function supplied by the Device Manager */
    ADI_DCB_CALLBACK_FN                 pfDMCallback;

    /* Timer ID to generate DISP signal */
    u32                                 nDispTimerID;

    /* Flag ID connected to DISP signal */
    ADI_FLAG_ID                         eDispFlagID;

    /* DISP signal status */
    ADI_LQ043T1DG01_DISP                eDispStatus;

    /* EPPI Control Register Mode */
    ADI_LQ043T1DG01_EPPI_CTRL_REG_MODE  eEppiCtrlRegMode;

} ADI_LQ043T1DG01;

/*=============  D A T A  =============*/

/*
** Sharp LQ043T1DG01 LCD Device specific data
*/
static ADI_LQ043T1DG01 _gaLq043t1dg01Device[] =
{
    {
        /* Device Not in use */
        false,
        /* DISP signal Flag and Timer not set */
        false,
        /* Default PPI device to use */
        ADI_LQ043T1DG01_PPI_DEV_DEFAULT,
        /* Device Manager handle */
        NULL,
        /* PPI device driver handle */
        NULL,
        /* Device Handle */
        NULL,
        /* DMA Manager Handle */
        NULL,
        /* DCB Manager Handle */
        NULL,
        /* Callback function */
        NULL,
        /* Timer ID to generate Disp */
        ADI_LQ043T1DG01_DISP_TIMER_DEFAULT,
        /* Disp signal Flag not defined */
        ADI_FLAG_UNDEFINED,
        /* DISP signal is disabled by default */
        ADI_LQ043T1DG01_DISP_DISABLED,
        /* Default EPPI Control register configuration mode */
        ADI_LQ043T1DG01_EPPI_CTRL_REG_MODE_DEFAULT
    },
};

/*********************************************************************

Static functions

*********************************************************************/

static u32 adi_pdd_Open(                       /* Opens LQ043T1DG01 device                 */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* Device Manager handle                    */
    u32                     DeviceNumber,       /* LQ043T1DG01 Device number to open        */
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* Device handle                            */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,        /* pointer to PDD handle location           */
    ADI_DEV_DIRECTION       Direction,          /* data direction                           */
    void                    *pEnterCriticalArg, /* critical region storage location         */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager                */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle                          */
    ADI_DCB_CALLBACK_FN     DMCallback          /* device manager callback function         */
);

static u32 adi_pdd_Close(                      /* Closes a LQ043T1DG01 device              */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of the device to close        */
);

static u32 adi_pdd_Read(                       /* Reads data/queues inbound buffer to EPPI */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a LQ043T1DG01 device       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);

static u32 adi_pdd_Write(                      /*Writes data/queues outbound buffer to EPPI*/
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a LQ043T1DG01 device       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);

static u32 adi_pdd_SequentialIO(               /* Sequentially read/writes data to a device*/
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a LQ043T1DG01 device       */
    ADI_DEV_BUFFER_TYPE     BufferType,         /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer            /* pointer to buffer                        */
);

static u32 adi_pdd_Control(                    /* Sets or senses device specific parameter */
    ADI_DEV_PDD_HANDLE      PDDHandle,          /* PDD handle of a EPPI device              */
    u32                     Command,            /* Command ID                               */
    void                    *Value              /* Command specific value                   */
);

/* Function to open a EPPI device */
static u32 PpiOpen(
    ADI_LQ043T1DG01         *pDevice            /* Pointer to the device we're working on   */
);

/* Function to close a EPPI device */
static u32 PpiClose(
    ADI_LQ043T1DG01         *pDevice            /* Pointer to the device we're working on   */
);

/* EPPI Callback function */
static void PpiCallbackFunction(
    void                    *DeviceHandle,      /* Device Handle                            */
    u32                     Event,              /* Callback event                           */
    void                    *pArg               /* Callback Argument                        */
);

/* Configures given Timer to generate DISP signal start/end */
static u32 DispTimerConfig(
    ADI_LQ043T1DG01         *pDevice,           /* Pointer to the device we're working on   */
    u8                      StartEndFlag        /* TRUE for DISP start, FALSE for DISP end  */
);

/* Callback for DISP generation Timer */
static void DispTmrCallback(
    void                    *DeviceHandle,      /* Device Handle                            */
    u32                     Event,              /* Callback event                           */
    void                    *pArg               /* Callback Argument                        */
);

/* Debug Build only */
#if defined(ADI_DEV_DEBUG)

static u32 ValidatePDDHandle(                  /* Validates Physical Device Driver Handle  */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of a LQ043T1DG01 device       */
);

#endif

/*********************************************************************

Entry point for device manager

*********************************************************************/

ADI_DEV_PDD_ENTRY_POINT     ADILQ043T1DG01EntryPoint = {
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control,
    adi_pdd_SequentialIO
};

/*=============  C O D E  =============*/

/*********************************************************************
*
*   Function:       adi_pdd_Open
*
*   Description:    Opens a LQ043T1DG01 device for use
*
*********************************************************************/

static u32 adi_pdd_Open(                       /* Open LQ043T1DG01 device          */
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,      /* Device Manager handle            */
    u32                     DeviceNumber,       /* LQ043T1DG01 Device number to open*/
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,       /* Device handle                    */
    ADI_DEV_PDD_HANDLE      *pPDDHandle,        /* pointer to PDD handle location   */
    ADI_DEV_DIRECTION       Direction,          /* data direction                   */
    void                    *pEnterCriticalArg, /* critical region storage location */
    ADI_DMA_MANAGER_HANDLE  DMAHandle,          /* handle to the DMA manager        */
    ADI_DCB_HANDLE          DCBHandle,          /* callback handle                  */
    ADI_DCB_CALLBACK_FN     DMCallback          /* device manager callback function */
)
{
    /* Return value - assume we're going to be successful   */
    u32                 Result = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we're working on               */
    ADI_LQ043T1DG01    *pDevice;
    /* exit critical region parameter                       */
    void                *pExitCriticalArg;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)

    if (DeviceNumber >= ADI_NUM_LQ043T1DG01_DEVICES)    /* check the device number */
    {
        Result = ADI_DEV_RESULT_BAD_DEVICE_NUMBER;      /* Invalid Device number */
    }

    /*  Continue only if the device number is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* insure the device the client wants is available  */
        Result  = ADI_DEV_RESULT_DEVICE_IN_USE;
        /* LQ043T1DG01 device we're working on */
        pDevice = &_gaLq043t1dg01Device[DeviceNumber];

        /* Protect this section of code - entering a critical region    */
        pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);
        /* Check the device usage status*/
        if (pDevice->bIsDeviceInUse == FALSE)
        {
            /* Device is not in use. Reserve the device for this client */
            pDevice->bIsDeviceInUse = TRUE;
            Result = ADI_DEV_RESULT_SUCCESS;
        }
        /* Exit the critical region */
        adi_int_ExitCriticalRegion(pExitCriticalArg);

        /* Continue only when the EPPI device is available for use  */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            /* This LQ043T1DG01 device has been reserved for this client */

            /* Configure this LQ043T1DG01 device instance */
            /* DISP Timer & Flag ID are yet to be set */
            pDevice->bIsDispTimerFlagSet    = false;
            /* Use default PPI Device */
            pDevice->nPpiDeviceNumber       = ADI_LQ043T1DG01_PPI_DEV_DEFAULT;
            /* Clear the PPI device Handle */
            pDevice->hPpiDevice             = NULL;
            /* Store the Manager handle */
            pDevice->hDeviceManager         = ManagerHandle;
            /* and store the Device handle */
            pDevice->hDevice                = DeviceHandle;
            /* and store the DMA Manager handle */
            pDevice->hDmaManager            = DMAHandle;
            /* and store the DCallback Manager handle */
            pDevice->hDcbManager            = DCBHandle;
            /* and callback function */
            pDevice->pfDMCallback           = DMCallback;
            /* Timer ID to generate Disp */
            pDevice->nDispTimerID           = ADI_LQ043T1DG01_DISP_TIMER_DEFAULT;
            /* Disp signal Flag not defined */
            pDevice->eDispFlagID            = ADI_FLAG_UNDEFINED;
            /* DISP signal disabled by default */
            pDevice->eDispStatus            = ADI_LQ043T1DG01_DISP_DISABLED;
            /* Default EPPI Control register configuration mode */
            pDevice->eEppiCtrlRegMode       = ADI_LQ043T1DG01_EPPI_CTRL_REG_MODE_DEFAULT;

            /* save the physical device handle in the client supplied location */
            *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pDevice;
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Close
*
*   Description:    Closes down a EPPI device
*
*********************************************************************/

static u32 adi_pdd_Close(                      /* Closes a EPPI device                     */
    ADI_DEV_PDD_HANDLE      PDDHandle           /* PDD handle of the EPPI device to close   */
)
{
    /* Return value - assume we're going to be successful   */
    u32                 Result = ADI_DEV_RESULT_SUCCESS;
    /* pointer to the device we're working on   */
    ADI_LQ043T1DG01    *pDevice;
    /* a temp location   */
    volatile u32        i;  /* made this volatile to fix TAR 33651 */

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    /* Validate the given PDDHandle */
    Result = ValidatePDDHandle(PDDHandle);

    /* Continue only if the given PDDHandle is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* avoid casts */
        pDevice = (ADI_LQ043T1DG01 *)PDDHandle;

        /* IF (DISP signal is in asserted state) */
        if(pDevice->eDispStatus == ADI_LQ043T1DG01_DISP_ASSERTED)
        {
            /* disable LCD dataflow */
            Result = adi_pdd_Control (PDDHandle,
                                      ADI_DEV_CMD_SET_DATAFLOW,
                                      (void *)false);
        }

        /* IF (DISP end generation in progress) */
        if (pDevice->eDispStatus == ADI_LQ043T1DG01_DISP_END_IN_PROGRESS)
        {
            /* Terminating LCD dataflow abnormally will lock the LCD and this can be
               fixed only power-cycling the hardware */
            /* to avoid any lock up - wait for DISP end generation until i reaches 0 */
            i = 0xFFFFFF;
            /* wait here until DISP status goes to ADI_LQ043T1DG01_DISP_DISABLED */
            while (i)
            {
                if (pDevice->eDispStatus == ADI_LQ043T1DG01_DISP_DISABLED)
                {
                    break;
                }
                i--;
            }
        }

        /* IF (DISP signal is not disabled yet) */
        if (pDevice->eDispStatus != ADI_LQ043T1DG01_DISP_DISABLED)
        {
            /* terminate LCD abnormally */
            /* stop DISP end generation - Remove DISP timer callback & close it */
            adi_tmr_RemoveCallback(pDevice->nDispTimerID);
            adi_tmr_Close(pDevice->nDispTimerID);

            /* disable EPPI dataflow */
            Result = adi_pdd_Control (pDevice->hPpiDevice,
                                      ADI_DEV_CMD_SET_DATAFLOW,
                                      (void *)false);

            pDevice->eDispStatus = ADI_LQ043T1DG01_DISP_DISABLED;
        }

        /* continue only if EPPI dataflow is disabled */
        if (Result == ADI_DEV_RESULT_SUCCESS)
        {
            /* mark this EPPI device as closed */
            pDevice->bIsDeviceInUse = FALSE;
            /* unhook the EPPI Error interrupt handler from the system */
            Result = adi_pdd_Control(PDDHandle,
                                     ADI_DEV_CMD_SET_ERROR_REPORTING,
                                     (void *)false);
        }

/* for Debug build only */
#if defined(ADI_DEV_DEBUG)
    }
#endif

    /* return */
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Read
*
*   Description:    Never called as LQ043T1DG01 is an outbound device
*
*********************************************************************/
static u32 adi_pdd_Read(               /* Reads data/queues inbound buffer to EPPI */
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle of a LQ043T1DG01 device       */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer                        */
)
{

    /* Return error as LQ043T1DG01 is an outbound device */
    return(ADI_DEV_RESULT_FAILED);
}

/*********************************************************************
*
*   Function:       adi_pdd_Write
*
*   Description:    Pass this to EPPI device
*
*********************************************************************/
static u32 adi_pdd_Write(              /*Writes data/queues outbound buffer to EPPI*/
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle of a LQ043T1DG01 device       */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer                        */
)
{
    /*  Pointer to the device instance we're working on */
    ADI_LQ043T1DG01    *pDevice;
    /* assume we're going to be success */
    u32                 Result = ADI_DEV_RESULT_SUCCESS;
    /* avoid casts */
    pDevice         = (ADI_LQ043T1DG01 *)PDDHandle;

/* Debug build only */
#if defined(ADI_DEV_DEBUG)
    /* Validate the PDD handle */
    if ((Result = ValidatePDDHandle(PDDHandle)) == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* IF (EPPI is already open) */
        if (pDevice->hPpiDevice)
        {
            /* pass the request on to EPPI */
            Result = adi_dev_Write (pDevice->hPpiDevice, BufferType, pBuffer);
        }
        /* ELSE (return error) */
        else
        {
            Result = ADI_LQ043T1DG01_RESULT_PPI_STATE_INVALID;
        }
/* Debug Build Only */
#ifdef ADI_DEV_DEBUG
    }
#endif

    /* return */
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_SequentialIO
*
*   Description:    LQ043T1DG01 driver doesnot support this function
*
*********************************************************************/
static u32 adi_pdd_SequentialIO(       /* Sequentially read/writes data to a device*/
    ADI_DEV_PDD_HANDLE      PDDHandle,  /* PDD handle of a LQ043T1DG01 device       */
    ADI_DEV_BUFFER_TYPE     BufferType, /* Buffer type                              */
    ADI_DEV_BUFFER          *pBuffer    /* pointer to buffer                        */
)
{

    /* this function is not supported by LQ043T1DG01 */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************
*
*   Function:       adi_pdd_Control
*
*   Description:    Senses or Configures LQ043T1DG01 driver or
*                   EPPI device registers
*
*********************************************************************/
static u32 adi_pdd_Control(            /* Sets or senses device specific parameter */
    ADI_DEV_PDD_HANDLE  PDDHandle,      /* PDD handle of a EPPI device              */
    u32                 Command,        /* Command ID                               */
    void                *Value          /* Command specific value                   */
)
{
    /*  Pointer to the device instance we're working on */
    ADI_LQ043T1DG01    *pDevice;
    u32                 Result;         /* return value                             */
    u32                 u32Value;       /* u32 type to avoid casts/warnings etc.    */
    u8                  u8Value;        /* u8 type to avoid casts/warnings etc.     */

    /* assume we're going to be successful */
    Result          = ADI_DEV_RESULT_SUCCESS;
    /* avoid casts */
    pDevice         = (ADI_LQ043T1DG01 *)PDDHandle;
    /* assign 16 and 32 bit values for the Value argument */
    u32Value        = (u32)Value;
    u8Value         = (u8)u32Value;

/* for Debug build only - check for errors if required  */
#if defined(ADI_DEV_DEBUG)
    /* Validate the given PDDHandle */
    Result = ValidatePDDHandle(PDDHandle);

    /* Continue only if the given PDDHandle is valid */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* CASEOF (Command ID)  */
        switch (Command)
        {
            /* CASE: control dataflow */
            case (ADI_DEV_CMD_SET_DATAFLOW):

                /* IF (Timer & Flag ID already set for this device) */
                if (pDevice->bIsDispTimerFlagSet)
                {
                    /* IF (Enable dataflow) */
                    if (u8Value)
                    {
                        /* IF (DISP signal is not in disabled state, return error) */
                        if (pDevice->eDispStatus != ADI_LQ043T1DG01_DISP_DISABLED)
                        {
                            Result = ADI_LQ043T1DG01_DISP_GENERATION_ALREADY_IN_PROGRESS;
                            break;
                        }

                        /* IF (Automatic configuration of
                               EPPI control register is enabled */
                        if (pDevice->eEppiCtrlRegMode != ADI_LQ043T1DG01_EPPI_CTRL_REG_AUTO_CONFIG_DISABLED)
                        {
                            /* Configure EPPI Control register */
                            if ((Result = adi_dev_Control(pDevice->hPpiDevice,
                                                          ADI_EPPI_CMD_SET_CONTROL_REG,
                                                          (void *)pDevice->eEppiCtrlRegMode))
                                                        != ADI_DEV_RESULT_SUCCESS)
                            {
                                break;  /* exit on error */
                            }
                        }

                        /* Enable EPPI dataflow */
                        if ((Result = adi_dev_Control(pDevice->hPpiDevice,
                                                      ADI_DEV_CMD_SET_DATAFLOW,
                                                      (void *)true))
                                                   != ADI_DEV_RESULT_SUCCESS)
                        {
                            break;  /* exit on error */
                        }

                        /* Configure Timer to generate DISP signal start */
                        if ((Result = DispTimerConfig(pDevice,TRUE)) != 0)
                        {
                            break;  /* exit on error */
                        }

                        /* update eDispStatus as DISP start generation in progress */
                        pDevice->eDispStatus = ADI_LQ043T1DG01_DISP_START_IN_PROGRESS;

                        /* Enable Timer to generate DISP start */
                        Result = adi_tmr_GPControl(pDevice->nDispTimerID,ADI_TMR_GP_CMD_ENABLE_TIMER, (void *)TRUE);
                    }
                    else    /* disable LCD dataflow */
                    {
                        /* IF (DISP signal is in asserted state) */
                        if (pDevice->eDispStatus == ADI_LQ043T1DG01_DISP_ASSERTED)
                        {
                            /* Deactivate DISP signal by clearing the corresponding DISP flag */
                            if ((Result = adi_flag_Clear(pDevice->eDispFlagID)) != ADI_FLAG_RESULT_SUCCESS)
                            {
                                break;  /* exit on error */
                            }
                            /* Call Timer service to open the given DISP Timer */
                            if ((Result = adi_tmr_Open(pDevice->nDispTimerID)) != ADI_TMR_RESULT_SUCCESS)
                            {
                                break;  /* exit on error */
                            }
                            /* Configure Timer to generate DISP signal end */
                            if ((Result = DispTimerConfig(pDevice,FALSE)) != ADI_TMR_RESULT_SUCCESS)
                            {
                                break;  /* exit on error */
                            }
                            /* update eDispStatus as DISP end generation in progress */
                            pDevice->eDispStatus = ADI_LQ043T1DG01_DISP_END_IN_PROGRESS;
                            /* Enable timer to generate DISP end */
                            Result = adi_tmr_GPControl(pDevice->nDispTimerID,ADI_TMR_GP_CMD_ENABLE_TIMER, (void *)TRUE);
                        }
                    }
                }
                else
                {
                    /* return error, as no valid Timer/Flag ID is avaliable for DISP generation */
                    Result = ADI_LQ043T1DG01_RESULT_DISP_TIMER_FLAG_INVALID;
                }
                break;

            /* CASE: query for processor DMA support */
            case (ADI_DEV_CMD_GET_PERIPHERAL_DMA_SUPPORT):

                /* EPPI uses DMA but LQ043T1DG01  doesn't */
                *((u32 *)Value) = FALSE;
                break;

            /* CASE: Set Timer ID to generate DISP signal for the LCD &
                     Flag ID to which the DISP signal is connected to */
            case (ADI_LQ043T1DG01_CMD_SET_DISP_TIMER_FLAG):

                /* IF (the application has already set a Timer/Flag ID for this device) */
                if (pDevice->bIsDispTimerFlagSet)
                {
                    /* close the timer already opened by this driver */
                    if ((Result = adi_tmr_Close(pDevice->nDispTimerID)) != ADI_TMR_RESULT_SUCCESS)
                    {
                        break;  /* exit on error */
                    }
                    /* No need to worry about Flag as we have't installed any callbacks to it */
                    pDevice->bIsDispTimerFlagSet = FALSE; /* Clear the Done flag */
                }

                /* Save the Timer & Flag ID passed by the client */
                pDevice->nDispTimerID = ((ADI_LQ043T1DG01_TIMER_FLAG *)Value)->DispTimerId;
                pDevice->eDispFlagID  = ((ADI_LQ043T1DG01_TIMER_FLAG *)Value)->DispFlagId;

                /* Call Flag service to open this Flag for LQ043T1DG01 */
                if ((Result = adi_flag_Open(pDevice->eDispFlagID)) != ADI_FLAG_RESULT_SUCCESS)
                {
                    break;  /* exit on error */
                }
                /* Set this flag as Output */
                if ((Result = adi_flag_SetDirection(pDevice->eDispFlagID,ADI_FLAG_DIRECTION_OUTPUT)) != ADI_FLAG_RESULT_SUCCESS)
                {
                    break;  /* exit on error */
                }
                /* Clear this flag */
                if ((Result = adi_flag_Clear(pDevice->eDispFlagID)) != ADI_FLAG_RESULT_SUCCESS)
                {
                    break;  /* exit on error */
                }

                /* Call Timer service to open the given DISP Timer */
                if ((Result = adi_tmr_Open(pDevice->nDispTimerID)) == ADI_TMR_RESULT_SUCCESS)
                {
                    /* Mark bIsDispTimerFlagSet as TRUE */
                    pDevice->bIsDispTimerFlagSet = TRUE;
                }
                break;

            /* CASE: Set EPPI Device Number to use */
            case (ADI_LQ043T1DG01_CMD_SET_EPPI_DEV_NUMBER):

                /* save the EPPI Device Number passed by the client */
                pDevice->nPpiDeviceNumber = u8Value;
                break;

            /* CASE: Open/Close EPPI Device connected to LQ043T1DG01 LCD */
            case (ADI_LQ043T1DG01_CMD_SET_OPEN_EPPI_DEVICE):

                /* IF (Open the EPPI device) */
                if (u8Value)
                {
                    Result = PpiOpen(pDevice);
                }
                /* ELSE (Close the EPPI device) */
                else
                {
                    Result = PpiClose(pDevice);
                }
                break;

            /* CASE: Set EPPI Control Register configuration mode */
            case (ADI_LQ043T1DG01_CMD_SET_EPPI_CTRL_REG_MODE):

                /* save EPPI Control Register configuration mode passed by the client */
                pDevice->eEppiCtrlRegMode = (ADI_LQ043T1DG01_EPPI_CTRL_REG_MODE) Value;
                break;

            /* CASE (Not a LQ043T1DG01 driver specific command. Pass it to EPPI driver */
            default:

                /* IF (EPPI device is already open) */
                if (pDevice->hPpiDevice)
                {
                    /* pass the unknown command to EPPI */
                    Result = adi_dev_Control(pDevice->hPpiDevice, Command, Value);
                }
                /* ELSE (Command not supported, return error) */
                else
                {
                    Result = ADI_LQ043T1DG01_RESULT_CMD_NOT_SUPPORTED;
                }
                break;
        }

/* Debug Build Only */
#ifdef ADI_DEV_DEBUG
    }
#endif
    return(Result);
}

/*********************************************************************
*
*   Function:       PpiOpen
*
*   Description:    Opens EPPI device for LQ043T1DG01
*
*********************************************************************/
static u32 PpiOpen(
    ADI_LQ043T1DG01         *pDevice            /* Pointer to the device we're working on   */
)
{
    /* assume we're going to be success */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* Open the EPPI driver */
    Result = adi_dev_Open(pDevice->hDeviceManager,
                          &ADIEPPIEntryPoint,
                          pDevice->nPpiDeviceNumber,
                          pDevice,
                          &pDevice->hPpiDevice,
                          ADI_DEV_DIRECTION_OUTBOUND,
                          pDevice->hDmaManager,
                          pDevice->hDcbManager,
                          PpiCallbackFunction);

    /* Continue only when the EPPI driver open is a success */
    if (Result == ADI_DEV_RESULT_SUCCESS)
    {
        /* table of EPPI driver configuration values */
        ADI_DEV_CMD_VALUE_PAIR EppiLcdConfig[]=
        {
            /* LCD Clock (EPPI Clock Out) - 8MHz                                            */
            { ADI_EPPI_CMD_SET_CLOCK_FREQ,              (void *)ADI_LQ043T1DG01_FREQ        },
            /* Samples per Line - 480 (active data) + 45 (padding)                          */
            { ADI_EPPI_CMD_SET_SAMPLES_PER_LINE,        (void *)525                         },
            /* Lines per Frame - 272 (active data) + 14 (padding)                           */
            { ADI_EPPI_CMD_SET_LINES_PER_FRAME,         (void *)286                         },
            /* FS1 (Hsync) Width (Typical)                                                  */
            { ADI_EPPI_CMD_SET_FS1_WIDTH,               (void *)41                          },
            /* FS1 (Hsync) Period (Typical)                                                 */
            { ADI_EPPI_CMD_SET_FS1_PERIOD,              (void *)525                         },
            /* Horizontal Delay clock after assertion of Hsync (Typical)                    */
            { ADI_EPPI_CMD_SET_HORIZONTAL_DELAY,        (void *)43                          },
            /* # active data to transfer after Horizontal Delay clock                       */
            { ADI_EPPI_CMD_SET_HORIZONTAL_TX_COUNT,     (void *)480                         },
            /* FS2 (Vsync) Width    = FS1 (Hsync) Period * 10                               */
            { ADI_EPPI_CMD_SET_FS2_WIDTH,               (void *)5250                        },
            /* FS2 (Vsync) Period   = FS1 (Hsync) Period * Lines per Frame                  */
            { ADI_EPPI_CMD_SET_FS2_PERIOD,              (void *)150150                      },
            /* Vertical Delay after assertion of Vsync (2 Lines)                            */
            { ADI_EPPI_CMD_SET_VERTICAL_DELAY,          (void *)12                          },
            /* # active lines to transfer after Vertical Delay clock                        */
            { ADI_EPPI_CMD_SET_VERTICAL_TX_COUNT,       (void *)272                         },
            /* End of Configuration table                                                   */
            { ADI_DEV_CMD_END,                          NULL                                }
        };

        /* Configure EPPI device for LQ043T1DG01 */
        Result = adi_dev_Control(pDevice->hPpiDevice, ADI_DEV_CMD_TABLE, (void*)EppiLcdConfig);
    }

    /* return */
    return (Result);
}

/*********************************************************************
*
*   Function:       PpiClose
*
*   Description:    Closes EPPI device used by LQ043T1DG01
*
*********************************************************************/
static u32 PpiClose(
    ADI_LQ043T1DG01         *pDevice            /* Pointer to the device we're working on   */
)
{
    /* assume we're going to be success */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* Check if any EPPI device is open */
    /* if so, close the present EPPI device in use */
    if (pDevice->hPpiDevice != NULL)
    {
        /* close EPPI device */
        if ((Result = adi_dev_Close(pDevice->hPpiDevice))== ADI_DEV_RESULT_SUCCESS)
        {
            /* Mark EPPI Handle as NULL indicating that the device is closed */
            pDevice->hPpiDevice = NULL;
        }
    }

    /* return */
    return (Result);
}

/*********************************************************************

    Function:       PpiCallbackFunction

    Description:    Callback for EPPI driver

*********************************************************************/
static void PpiCallbackFunction(
    void        *DeviceHandle,      /* Device Handle     */
    u32         Event,              /* Callback event    */
    void        *pArg               /* Callback Argument */
)
{
    /*  Pointer to the device instance we're working on */
    ADI_LQ043T1DG01    *pDevice;
    pDevice = (ADI_LQ043T1DG01 *)DeviceHandle;

/* Debug build only */
#if defined(ADI_DEV_DEBUG)
    /* Validate the PDD handle */
    if (ValidatePDDHandle(DeviceHandle) == ADI_DEV_RESULT_SUCCESS)
    {
#endif

        /* simply pass the callback along to the Device Manager Callback */
        (pDevice->pfDMCallback)(pDevice->hDevice,Event,pArg);

/* Debug Build Only */
#ifdef ADI_DEV_DEBUG
    }
#endif
}

/*********************************************************************

    Function:       DispTimerConfig

    Description:    Configures the given Timer to generate
                    DISP signal start/end

    For DISP Start - with EPPI Internally generated clock:

        Timer Period = (EPPI VSYNC Width * (SClk/ADI_LQ043T1DG01_FREQ),
        where VSYNC Width = 10 * HSYNC period
        an extra 5 HSYNC period boundary is provided to accomodate the Frequency difference

    For DISP Start - with EPPI Externally generated clock:

        Timer Period = EPPI VSYNC Width
        where VSYNC Width = 10 * HSYNC period
        an extra 5 HSYNC period boundary is provided

    For DISP End - with EPPI Internally generated clock:

        Timer Period = (10 Frames * SClk/ADI_LQ043T1DG01_FREQ)
        where 10 Frames = 10 * VSYNC Period * HSYNC Period

    For DISP Period - with EPPI Externally generated clock:

        Timer Width = 10 Frames
        where 10 Frames = 10 * VSYNC Period * HSYNC Period

*********************************************************************/
static u32 DispTimerConfig(
    ADI_LQ043T1DG01     *pDevice,       /* Pointer to the device we're working on   */
    u8                  StartEndFlag    /* TRUE for DISP start, FALSE for DISP end  */
)
{
    /* assume we're going to be successful */
    u32 Result = ADI_TMR_RESULT_SUCCESS;
    u32 Sclk,u32Temp=0;
    u32 MultiplyFactor;

    /* Timer configuration common for DISP start & end */
    static ADI_TMR_GP_CMD_VALUE_PAIR    DispTmrConfig[] =
    {
        { ADI_TMR_GP_CMD_SET_TIMER_MODE,        (void *)1           },  /* PWM mode                         */
        { ADI_TMR_GP_CMD_SET_COUNT_METHOD,      (void *)TRUE        },  /* count to end of period           */
        { ADI_TMR_GP_CMD_SET_INTERRUPT_ENABLE,  (void *)TRUE        },  /* Enable Timer interrupt           */
        { ADI_TMR_GP_CMD_SET_OUTPUT_PAD_DISABLE,(void *)TRUE        },  /* Disable Timer ouput pad          */
        { ADI_TMR_GP_CMD_RUN_DURING_EMULATION,  (void *)TRUE        },  /* Run timer during emulation mode  */
        { ADI_TMR_GP_CMD_SET_WIDTH,             (void *)10          },
        { ADI_TMR_GP_CMD_END,                   NULL                },
    };

    /* Timer Clock config - EPPI0_CLK as source when EPPI clock is externally generated */
    static ADI_TMR_GP_CMD_VALUE_PAIR    DispTmrClkExternal[] =
    {
        { ADI_TMR_GP_CMD_SET_INPUT_SELECT,      (void *)TRUE        },  /* input from EPPI clock            */
        { ADI_TMR_GP_CMD_SET_CLOCK_SELECT,      (void *)TRUE        },  /* use PWM clock                    */
        { ADI_TMR_GP_CMD_END,                   NULL                },
    };

    /* Timer Clock config - EPPI0_CLK as source when EPPI clock is externally generated */
    static ADI_TMR_GP_CMD_VALUE_PAIR    DispTmrClkInternal[] =
    {
        { ADI_TMR_GP_CMD_SET_CLOCK_SELECT,      (void *)FALSE       },  /* use SCLK clock                   */
        { ADI_TMR_GP_CMD_END,                   NULL                },
    };

    /* Configure Timer with common configuration table for DISP start/end */
    if ((Result = adi_tmr_GPControl(pDevice->nDispTimerID, ADI_TMR_GP_CMD_TABLE, DispTmrConfig))  == ADI_TMR_RESULT_SUCCESS)
    {
        /* Get EPPI Configuration register value */
        if ((Result = adi_dev_Control(pDevice->hPpiDevice, ADI_EPPI_CMD_GET_CONTROL_REG, (void *)&u32Temp)) == ADI_DEV_RESULT_SUCCESS)
        {
            /* extract EPPI Clock generation info */
            /* IF (Clock is internaly generated */
            if (u32Temp && 0x0020)
            {
                /* EPPI clock is generated internally */
                /* generate the Multiplying factor for Timer Period */
                /* Get the SCLK value to calculate Timer Period Multiplying Factor for DISP generation */
                if ((Result = adi_pwr_GetFreq(&u32Temp, &Sclk, &u32Temp)) == ADI_PWR_RESULT_SUCCESS)
                {
                    MultiplyFactor = (Sclk/ADI_LQ043T1DG01_FREQ);
                    /* Configure Timer clock source */
                    Result = adi_tmr_GPControl(pDevice->nDispTimerID, ADI_TMR_GP_CMD_TABLE, DispTmrClkInternal);
                }
            }
            else
            {
                /* EPPI Clock is generated externally */
                MultiplyFactor = 1;
                /* Configure Timer clock source */
                Result = adi_tmr_GPControl(pDevice->nDispTimerID, ADI_TMR_GP_CMD_TABLE, DispTmrClkExternal);
            }
        }
    }

    /* continue only if Multiply factor calculation results in success */
    if (Result == 0)
    {
        /* Calculate Timer Period for this DISP signal generation */
        /* IF (this is for DISP signal start generation) */
        if (StartEndFlag)
        {
            u32Temp = (15 * 525 * MultiplyFactor);
        }
        /* ELSE (config timer for DISP end) */
        else
        {
            /* Timer period = Hsync * Vsync * number of frames */
            u32Temp = (525 * 286 * 10 * MultiplyFactor);
        }

        /* configure Timer period register */
        if ((Result = adi_tmr_GPControl(pDevice->nDispTimerID, ADI_TMR_GP_CMD_SET_PERIOD, (void *)u32Temp)) == ADI_TMR_RESULT_SUCCESS)
        {

            /* Install callback for this timer
               - Parameters (Timer ID, Wakeup flag, Client argument,
                             DCB Manger Handle, Callback function */
            Result = adi_tmr_InstallCallback(pDevice->nDispTimerID,
                                             true,
                                             pDevice,
                                             pDevice->hDcbManager,
                                             DispTmrCallback);

        }
    }
    /* return */
    return (Result);
}

/*********************************************************************

    Function:       DispTmrCallback

    Description:    Callback for DISP generation Timer

*********************************************************************/
static void DispTmrCallback(
    void    *DeviceHandle,      /* Device Handle */
    u32     Event,              /* Callback event */
    void    *pArg               /* Callback Argument */
)
{
    /*  Pointer to the device instance we're working on */
    ADI_LQ043T1DG01    *pDevice;
    pDevice = (ADI_LQ043T1DG01 *)DeviceHandle;

    /* assume we're going to be successful */
    u32 Result = ADI_DEV_RESULT_SUCCESS;

    /* CASEOF (Event) */
    switch(Event)
    {
        /* CASE: Timer expired */
        case (ADI_TMR_EVENT_TIMER_EXPIRED):
            /* Remove callback & close this timer */
            adi_tmr_RemoveCallback(pDevice->nDispTimerID);
            adi_tmr_Close(pDevice->nDispTimerID);

            /* IF (this callback is for DISP end generation) */
            if (pDevice->eDispStatus == ADI_LQ043T1DG01_DISP_END_IN_PROGRESS)
            {
                /* Disable EPPI Dataflow */
                if ((Result = adi_dev_Control(pDevice->hPpiDevice, ADI_DEV_CMD_SET_DATAFLOW, (void *) FALSE)) == ADI_DEV_RESULT_SUCCESS)
                {
                    /* update DISP status */
                    pDevice->eDispStatus = ADI_LQ043T1DG01_DISP_DISABLED;
                }
            }
            /* ELSE IF (this callback is for DISP start generation) */
            else if (pDevice->eDispStatus == ADI_LQ043T1DG01_DISP_START_IN_PROGRESS)
            {
                /* Assert DISP signal Flag pin */
                if((Result = adi_flag_Set(pDevice->eDispFlagID)) == ADI_FLAG_RESULT_SUCCESS)
                {
                    /* update DISP status */
                    pDevice->eDispStatus = ADI_LQ043T1DG01_DISP_ASSERTED;
                }
            }
            break;

        default:
            break;
    }
    /* return */
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
    u32     Result = ADI_DEV_RESULT_BAD_PDD_HANDLE;
    /* index */
    u32     i;

    /* compare the given PDD handle with PDDHandle allocated to all EPPI devices in the list    */
    for (i = 0; i < ADI_NUM_LQ043T1DG01_DEVICES; i++)
    {
        if (PDDHandle == (ADI_DEV_PDD_HANDLE)&_gaLq043t1dg01Device[i])
        {
            /* Given PDDHandle is valid. quit this loop */
            Result = ADI_DEV_RESULT_SUCCESS;
            break;
        }
    }
    /* return */
    return (Result);
}

#endif /* ADI_DEV_DEBUG */

/*****/

/*
**
** EOF:
**
*/

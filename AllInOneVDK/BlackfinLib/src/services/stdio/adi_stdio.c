/* $Revision: 3682 $
 * $Date: 2010-11-01 16:56:32 -0400 (Mon, 01 Nov 2010) $ 
**************************************************************************** 
Copyright (c), 2008-2009 - Analog Devices Inc. All Rights Reserved. 
This software is proprietary & confidential to Analog Devices, Inc. 
and its licensors. 
****************************************************************************

Title: STDIO Service 

Description: This file contains the STDIO service public API functions
             implementation. 
*****************************************************************************/

/*=============  I N C L U D E S   =============*/

/* IF MISRA diagnostics enabled, suppress selected MISRA rules for this file as necessary */
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_5_1 :"Most identifiers require more than 31 characters for readability. Rule disabled to avoid truncation")
#pragma diag(suppress:misra_rule_5_6 :"Most functions share same parameter name(s) for code readability")
#pragma diag(suppress:misra_rule_5_7 :"Most functions, structures and unions share same identifier name(s) for code readability")
#pragma diag(suppress:misra_rule_19_7:"Allow use of macros for bitwise operations")
#pragma diag(suppress:misra_rule_17_4:"Allow array indexing on pointers")
#pragma diag(suppress:misra_rule_20_9:"Stdio class devices leverage input/output library <stdio.h> for POSIX functions")
#endif /* _MISRA_RULES */

#include <stdio.h>                      /* STDIO includes                 */
#include <string.h>                     /* String operations includes     */
#include <services/stdio/adi_stdio.h>   /* STDIO service includes         */
#include <osal/adi_osal.h>              /* OSAL includes                  */
#include "adi_stdio_common.h"           /* Private STDIO service includes */

/*=============  D A T A  =============*/

/*
** Structure to store the STDIO service information
*/
typedef struct adiStdioService
{
    /* Pointer to the array of devices */
    ADI_STDIO_DEVICE_INFO      *paStdioDevice;
    /* Device Manger Handle */
    ADI_DEV_MANAGER_HANDLE      hDeviceMgr;
    /* DMA Manager Handle */
    ADI_DMA_MANAGER_HANDLE      hDMAMgr;
    /* DCB Manager Handle */
    ADI_DCB_HANDLE              hDCBMgr;
    /* Memory Pool Handle */
    int32_t                     hHeapIndex;
    /* Handle to the Mutex */
    ADI_OSAL_MUTEX_HANDLE       hMutex;
    /* Handle to NULL device device */
    ADI_STDIO_DEVICE_HANDLE     hNULLDevice;
    /* Handle to JTAG device device */
    ADI_STDIO_DEVICE_HANDLE     hJTAGDevice;
    /* Handle to STDIN device */
    ADI_STDIO_DEVICE_HANDLE     hSTDINDevice;
    /* Handle to STDOUT device */
    ADI_STDIO_DEVICE_HANDLE     hSTDOUTDevice;
    /* Handle to STDERR device */
    ADI_STDIO_DEVICE_HANDLE     hSTDERRDevice;
    /* Default STDIN File stream */
    FILE                        oSTDINFile;
    /* Default STDOUT File stream */
    FILE                        oSTDOUTFile;
    /* Default STDERR File stream */
    FILE                        oSTDERRFile;

    /* STDIN File pointer */
    FILE                       *pSTDINFile;
    /* STDOUT File pointer */
    FILE                       *pSTDOUTFile;
    /* STDErr File pointer */
    FILE                       *pSTDERRFile;

} ADI_STDIO_SERVICE, *ADI_STDIO_HANDLE;

/* Handle to STDIO service */
static ADI_STDIO_HANDLE ghStdioService = NULL;

/* Table which contains the pointers to the device entries */
ADI_STDIO_DEVICE_ENTRY *_adi_stdio_pDeviceEntries[ADI_STDIO_DEVICE_TYPE_MAX];

/* 
**  Table to store device information
*/
static ADI_STDIO_DEVICE_INFO gaDeviceInfo[ADI_STDIO_MAX_DEVICES];


/*=============  C O D E  =============*/

/*
**   Local function prototypes
*/


/* Disables selected STDIO stream by redirecting the output to
   the NULL device*/
static ADI_STDIO_RESULT  DisableStream(
    ADI_STDIO_STREAM_TYPE     eStreamType
);

/* Function to submit read/write buffer to selected Stdio device*/
static int32_t  StdioAccess (
    ADI_STDIO_DEVICE_INFO    *pStdioDevice,
    uint8_t                  *psDataBuffer,
    int32_t                   nElementCount,
    bool                      bReadFlag
);

/* Function to open the stdio device */
static ADI_STDIO_RESULT  OpenDevice (
    uint32_t                  nDeviceType,
    uint32_t                  nPhysicalDevNum,
    ADI_STDIO_DEVICE_HANDLE  *phStdioDevice
);


/********************************************************************************************
  Function: adi_stdio_Init

      Initializes the STDIO service

  Parameters:
      hDeviceMgr          - Handle to Device Manager
      hDMAMgr             - Handle to DMA Manager
      hDCBMgr             - Handle to DCB Manager
      phDefaultDevice     - Pointer to the default STDIO device (JTAG)

  Returns:
      ADI_STDIO_RESULT_SUCCESS
          - Successfully initialized STDIO service
      ADI_STDIO_RESULT_INVALID_HANDLE
          - One or more of the given handles is invalid
      ADI_STDIO_RESULT_SERVICE_ALREADY_INITIALIZED
          - STDIO service is already initialized
      ADI_STDIO_RESULT_MUTEX_FAILURE
          - Failed to create the required mutex
**********************************************************************************************/
ADI_STDIO_RESULT  adi_stdio_Init (
    ADI_DEV_MANAGER_HANDLE    const hDeviceMgr,
    ADI_DMA_MANAGER_HANDLE    const hDMAMgr,
    ADI_DCB_HANDLE            const hDCBMgr,
    ADI_STDIO_DEVICE_HANDLE        *phDefaultDevice
)
{
    /* Return value, assume that we will be successful */
    ADI_STDIO_RESULT           eResult = ADI_STDIO_RESULT_SUCCESS;
    /* Loop variable */
    uint32_t                 i;
    /* Pointer to STDIO device */
    ADI_STDIO_DEVICE_INFO    *pStdioDevice;
    /* Stdio Service Object */
    static ADI_STDIO_SERVICE goStdioService;
    /* OSAL configuration object */
    ADI_OSAL_CONFIG          OSConfig = {
                             ADI_OSAL_PRIO_INHERIT_AUTO,    /* Use the priority inheritance defined by OS */
                             8,                             /* Number of Thread slots */
                             0,                             /* System timer period (use OS defined value) */
                             NULL,                          /* Pointer to heap (use regular malloc) */
                             0 };                           /* Size of heap */
    /* OSAL Return code */
    uint32_t                 nOSALResult;

    /* IF(service is initialized) */
    if(ghStdioService != NULL)
    {
        /* Report error (STDIO service is already initialized) */
        eResult = ADI_STDIO_RESULT_SERVICE_ALREADY_INITIALIZED;
    }
    /* IF (Device Manager Handle is passed as NULL) */
    else if(NULL == hDeviceMgr)
    {
        /* Report Error (Invalid Device Manager Handle) */
        eResult = ADI_STDIO_RESULT_INVALID_HANDLE;
    }
    /**********************************************************************
    **
    ** Not required to validate DMA and DCB Manger handles as they are not 
    ** required when not using DMA and Deferred Callbacks
    **
    ***********************************************************************/
    /* IF (Pointer to the Handle of the default device is NULL) */
    else if(NULL == phDefaultDevice)
    {
        /* Report error (The given pointer to default device handle is invalid) */
        eResult = ADI_STDIO_RESULT_INVALID_POINTER;
    }
    /*
    **  Calling OSAL Init from the here prevents application to change the OSAL configuration
    **  again. This call should be eventually moved to the application.
    */
    /* IF (OSAL init returns other than Success and already initialized error) */
    else if( ((nOSALResult = adi_osal_Init(&OSConfig)) != ADI_OSAL_SUCCESS) &&
             (nOSALResult != ADI_OSAL_FAILED) )
    {
        /* Report error (Failed to initialize OSAL) */
        eResult = ADI_STDIO_RESULT_FAILED;   
    }
    /* ELSE IF(Not able to create Mutex) */
    else if((adi_osal_MutexCreate(&goStdioService.hMutex)) != ADI_OSAL_SUCCESS)
    {
        /* Report error (Unable to create the required Mutex) */
        eResult = ADI_STDIO_RESULT_MUTEX_FAILURE;
    }
    else
    {
        /* Initialize the stdio service handle */
        ghStdioService = &goStdioService;

        /* Initialize the heap index*/
        ghStdioService->hHeapIndex       = 0;
        /* Initialize device handles */
        ghStdioService->hNULLDevice      = NULL;
        ghStdioService->hJTAGDevice      = NULL;
        ghStdioService->hSTDINDevice     = NULL;
        ghStdioService->hSTDOUTDevice    = NULL;
        ghStdioService->hSTDERRDevice    = NULL;

        /* Initialize the file pointers */
        ghStdioService->pSTDINFile       = NULL;
        ghStdioService->pSTDOUTFile      = NULL;
        ghStdioService->pSTDERRFile      = NULL;

        /* Save the given handles */
        ghStdioService->hDeviceMgr       = hDeviceMgr;
        ghStdioService->hDMAMgr          = hDMAMgr;
        ghStdioService->hDCBMgr          = hDCBMgr;

        /* Make a local copy of the standard file streams */
        memcpy(&ghStdioService->oSTDINFile, stdin, sizeof(FILE));
        memcpy(&ghStdioService->oSTDOUTFile, stdout, sizeof(FILE));
        memcpy(&ghStdioService->oSTDERRFile, stderr, sizeof(FILE));

        /* Store the memory passed for creating devices */
        ghStdioService->paStdioDevice  = gaDeviceInfo;
        /* Clear the Device info pointer table */
        memset(&_adi_stdio_pDeviceEntries[0U], 0, sizeof(_adi_stdio_pDeviceEntries));
        
        pStdioDevice = ghStdioService->paStdioDevice;

        /* Initialize the Libio device id in stdio device */
        for(i = 0U;  i < ADI_STDIO_MAX_DEVICES; i++)
        {
            /* Set the LIBIO device ID */
            pStdioDevice->nLibioDeviceId     = (i + ADI_STDIO_ENUMERATION_START);
            /* Clear all the flags */
            pStdioDevice->nFlags             = 0U;
            /* Move to the next device */
            pStdioDevice++;
        }

        /* Register NULL device with the service */
        adi_stdio_RegisterNULL();

        /* Register JTAG device with the service */
        adi_stdio_RegisterJTAG();

        /*Open the NULL device for internal use */
        eResult = OpenDevice(ADI_STDIO_DEVICE_TYPE_NULL,   
                             0U,                           
                             &ghStdioService->hNULLDevice); 

        /* IF (NULL device was opened successfully) */
        if(ADI_STDIO_RESULT_SUCCESS == eResult)
        {
            /* Open the JTAG device */
            eResult = OpenDevice(ADI_STDIO_DEVICE_TYPE_JTAG,   
                                 0U,                           
                                 &ghStdioService->hJTAGDevice); 
        }

        /* Open the JTAG device */
        if(ADI_STDIO_RESULT_SUCCESS == eResult)
        {
            /* 
               Handle JTAG as special case, as it will be registered with LIBIO by default 
            */
            pStdioDevice = ghStdioService->hJTAGDevice;
            /* Set the LIBIO device ID */
            pStdioDevice->nLibioDeviceId = (uint32_t)PRIMIO;
            /* Set the flag to indicate that it is registered with LIBIO*/
            SET_REGISTERED_WITH_LIBIO_FLAG(pStdioDevice->nFlags);
            /* Return the default handle (JTAG) */
            *phDefaultDevice = ghStdioService->hJTAGDevice;
        }
    }
    return (eResult);
}



/********************************************************************************************
  Function: adi_stdio_Terminate

      Terminates the STDIO service

  Parameters:
      None
  Returns:
      ADI_STDIO_RESULT_SUCCESS
          - Successfully terminated STDIO service
      ADI_STDIO_RESULT_SERVICE_NOT_INITIALIZED
          - STDIO service is not initialized
**********************************************************************************************/
ADI_STDIO_RESULT adi_stdio_Terminate (
    void
)
{
    /* Return Value */
    ADI_STDIO_RESULT eResult = ADI_STDIO_RESULT_SUCCESS;

    /* IF (STDIO service is not initialized) */
    if(NULL == ghStdioService)
    {
        /* Return error (Service not yet initialized) */
        eResult = ADI_STDIO_RESULT_SERVICE_NOT_INITIALIZED;
    }
    else
    {
        /*
        ** Before terminating the service redirect all streams 
        ** back to the default device
        */
        adi_stdio_RedirectStream(ghStdioService->hJTAGDevice, 
                                 ADI_STDIO_STREAM_ALL_CONSOLE_IO);

        /* Unitialize the Stdio service handle */
        ghStdioService = NULL;
    }

    return (eResult);
}


/********************************************************************************************

  Function: adi_stdio_OpenDevice

      Opens the given physical device for STDIO

  Parameters:
      eDeviceType         - STDIO device type (JTAG/UART/NULL..)
      nPhysicalDevNum     - Physical device number to open
      phStdioDevice       - Pointer to STDIO device handle.

  Returns:
     ADI_STDIO_RESULT_SUCCESS
         - Successfully opened the given device.
     ADI_STDIO_RESULT_SERVICE_NOT_INITIALIZED
         - STDIO service is not yet initialized
     ADI_STDIO_RESULT_INVALID_POINTER
         - Given pointer to the STDIO device handle is invalid
     ADI_STDIO_RESULT_NO_STDIO_DEVICES
         - No free STDIO devices
     ADI_STDIO_RESULT_DEVICE_FAILED
         - Failed to open the given physical device
     ADI_STDIO_RESULT_INSUFFICIENT_MEMORY
         - Failed to allocated memory required for the device
     ADI_STDIO_RESULT_INVALID_DEVICE_TYPE
         - Given device type is invalid
     ADI_STDIO_RESULT_DEVICE_NOT_REGISTERED
         - Given device type is not registered with the service
********************************************************************************************/
ADI_STDIO_RESULT  adi_stdio_OpenDevice (
    ADI_STDIO_DEVICE_TYPE        eDeviceType,
    uint32_t                     nPhysicalDevNum,
    ADI_STDIO_DEVICE_HANDLE   *phStdioDevice
)
{
    /* Return value*/
    ADI_STDIO_RESULT           eResult = ADI_STDIO_RESULT_SUCCESS;
    
    /* IF (STDIO service is not initialized) */
    if(NULL == ghStdioService)
    {
        /* Return error (Service not yet initialized) */
        eResult = ADI_STDIO_RESULT_SERVICE_NOT_INITIALIZED;
    }
    /* ELSE IF (Given device type is valid) */
    else if((eDeviceType < ADI_STDIO_DEVICE_TYPE_UART) || 
            (eDeviceType >= ADI_STDIO_DEVICE_TYPE_MAX))
    {
        /* Report error (Given device type is not valid) */
        eResult = ADI_STDIO_RESULT_INVALID_DEVICE_TYPE;
    }
    /* ELSE IF (pointer to stdio device handle is NULL) */
    else if(NULL == phStdioDevice)
    {
        /* Report error (Invalid pointer) */
        eResult = ADI_STDIO_RESULT_INVALID_POINTER;
    }
    else
    {
        /* Lock Mutex */
        adi_osal_MutexPend(ghStdioService->hMutex, ADI_OSAL_TIMEOUT_FOREVER);

        eResult = OpenDevice(eDeviceType,
                             nPhysicalDevNum,
                             phStdioDevice);

        /* Release Mutex */
        adi_osal_MutexPost(ghStdioService->hMutex);
    }
    return (eResult);
}

/********************************************************************************************

  Function: adi_stdio_CloseDevice

      Closes the given STDIO device

  Parameters:
     hStdioDevice      - Handle to the STDIO device

  Returns:
     ADI_STDIO_RESULT_SUCCESS
         - Successfully closed the given device.
     ADI_STDIO_RESULT_SERVICE_NOT_INITIALIZED
         - STDIO service is not yet initialized
     ADI_STDIO_RESULT_INVALID_HANDLE
         - Handle to the device device is not valid
     ADI_STDIO_RESULT_DEVICE_FAILED
         - Failed to close the given physical device      
**********************************************************************************************/

ADI_STDIO_RESULT adi_stdio_CloseDevice(
    ADI_STDIO_DEVICE_HANDLE    const hStdioDevice
)
{
    /* Return Value */
    ADI_STDIO_RESULT           eResult = ADI_STDIO_RESULT_SUCCESS;
    /* Pointer to STDIO device */
    ADI_STDIO_DEVICE_INFO     *pStdioDevice;
    
    /* IF (STDIO service is not initialized) */
    if(NULL == ghStdioService)
    {
        /* Report error (STDIO service not yet initialized) */
        eResult = ADI_STDIO_RESULT_SERVICE_NOT_INITIALIZED;
    }
    /* IF (Handle to STDIO device is NULL) */
    else if (NULL == hStdioDevice)
    {
        /* Report error (Invalid STDIO device handle) */
        eResult = ADI_STDIO_RESULT_INVALID_HANDLE;
    }
    /* ELSE (STDIO device handle is valid) */
    else
    {
        /* Get pointer to STDIO device from the given device handle */
        pStdioDevice = (ADI_STDIO_DEVICE_INFO *) hStdioDevice;

        /*
          Before closing redirect streams directed to this device to NULL device 
        */

        adi_osal_MutexPend(ghStdioService->hMutex, ADI_OSAL_TIMEOUT_FOREVER);
        
        /* IF (This device is used for STDIN) */
        if(ghStdioService->hSTDINDevice == hStdioDevice)
        {
            /* Redirect STDIN to default device */
            eResult = adi_stdio_RedirectStream(ghStdioService->hJTAGDevice,
                                               ADI_STDIO_STREAM_STDIN);
        }
        
        /* IF (STDIN is disabled successfully and if this device is used for STDOUT) */
        if((ADI_STDIO_RESULT_SUCCESS == eResult) &&
           (ghStdioService->hSTDOUTDevice == hStdioDevice))
        {
            
            /* Redirect STDOUT to default device */
            eResult = adi_stdio_RedirectStream(ghStdioService->hJTAGDevice,
                                               ADI_STDIO_STREAM_STDOUT);
        }

        /* IF (STDOUT is disabled successfully and if this device is used for STDERR) */
        if((ADI_STDIO_RESULT_SUCCESS == eResult) &&
           (ghStdioService->hSTDERRDevice == hStdioDevice))
        {
            /* Redirect STDOUT to default device */
            eResult = adi_stdio_RedirectStream(ghStdioService->hJTAGDevice,
                                               ADI_STDIO_STREAM_STDERR);
        }
        
        /* IF (Successfully redirected the STDERR stream) */
        if(ADI_STDIO_RESULT_SUCCESS == eResult)
        {
            /* Close the physical device */
            eResult = pStdioDevice->pDeviceEntry->pfDeviceClose(pStdioDevice);
            
            /* Destroy the semaphores that are created */
            if(pStdioDevice->hOutputSemaphore != NULL)
            {
                adi_osal_SemDestroy(pStdioDevice->hOutputSemaphore);
            }
            
            if(pStdioDevice->hInputSemaphore != NULL)
            {           
                adi_osal_SemDestroy(pStdioDevice->hInputSemaphore);
            }   

            /* Free the memory allocated for the STDIO Device Entry */
            _adi_stdio_MemFree((void *)pStdioDevice->pDeviceEntry);

            /* IF (Device closed successfully) */
            if(ADI_STDIO_RESULT_SUCCESS == eResult)
            {
                /* Mark the device as free */
                CLEAR_DEVICE_IN_USE_FLAG(pStdioDevice->nFlags);
            }
        }
        
        adi_osal_MutexPost(ghStdioService->hMutex);

    } /* End of ELSE (STDIO device handle is valid) */

    return (eResult);
}



/********************************************************************************************
  Function: adi_stdio_RedirectStream

      Re-directs selected stream to a STDIO device

  Parameters:
      hStdioDevice    - Handle to STDIO device to which the 
                        stream to be redirected
      eStreamType     - Stream type to re-direct

  Returns:
      ADI_STDIO_RESULT_SUCCESS
          - Successfully redirected stream
      ADI_STDIO_RESULT_SERVICE_NOT_INITIALIZED
          - STDIO service is not yet initialized
      ADI_STDIO_RESULT_INVALID_HANDLE
          - Given handle is invalid
      ADI_STDIO_RESULT_STREAM_NOT_SUPPORTED
          - Invalid stream type or
            Stream type not supported by the selected device
      ADI_STDIO_RESULT_REDIRECT_FAILED
          - Failed to re-direct stream
      ADI_STDIO_RESULT_DEVICE_FAILED
          - STDIO device/hardware initialisation failed
      ADI_STDIO_RESULT_SEMAPHORE_FAILURE
          - Failed to create semaphore
      ADI_STDIO_RESULT_DEVTAB_REGISTER_FAILED
          - Failed to add entry into LIBIO device table entry
**********************************************************************************************/
ADI_STDIO_RESULT  adi_stdio_RedirectStream (
    ADI_STDIO_DEVICE_HANDLE   hStdioDevice,
    ADI_STDIO_STREAM_TYPE       eStreamType
)
{
    /* Return value */
    ADI_STDIO_RESULT           eResult = ADI_STDIO_RESULT_SUCCESS;
    /* Pointer to STDIO device information */
    ADI_STDIO_DEVICE_INFO     *pStdioDevice;
    /* Pointer to Device Info */
    ADI_STDIO_DEVICE_ENTRY    *pStdioDeviceEntry;
    /* Current IO Device */
    int32_t                    nCurrentIODevice;
	/* LibIO Default device ID */
	uint32_t nLibioDefaultDeviceId;
	
    /* IF (STDIO service is not initialized) */
    if(NULL == ghStdioService)
    {
        /* Report error (STDIO service not yet initialized) */
        eResult = ADI_STDIO_RESULT_SERVICE_NOT_INITIALIZED;
    }
    /* IF (STDIO device is not valid) */
    else if(NULL == hStdioDevice)
    {
        /* Report error (Invalid STDIO device handle) */
        eResult = ADI_STDIO_RESULT_INVALID_HANDLE;
    }
    /* ELSE (Input parameters are valid) */
    else
    {
        adi_osal_MutexPend(ghStdioService->hMutex, ADI_OSAL_TIMEOUT_FOREVER);
        
        /* Get pointer to STDIO instance using the STDIO device handle */
        pStdioDevice = (ADI_STDIO_DEVICE_INFO *) hStdioDevice;

        /* IF (Physical device handle is not valid) */
        if (NULL == pStdioDevice->hPhysicalDevice)
        {
            /* Report error (Invalid physical device handle) */
            eResult = ADI_STDIO_RESULT_INVALID_HANDLE;
        }
        /* ELSE (Physical device handle is valid) */
        else
        {
            /* IF (Semaphore is not yet created for input) */ 
            if (NULL == pStdioDevice->hInputSemaphore)
            {
                /* IF (Semaphore creation failed) */
                if ((adi_osal_SemCreate(&pStdioDevice->hInputSemaphore,
                                        0U)) != ADI_OSAL_SUCCESS)
                {
                    /* Report error (semaphore create failed) */
                    eResult = ADI_STDIO_RESULT_SEMAPHORE_FAILURE;
                }
            }
            
            /* IF (Semaphore is not yet created for output) */
            if((ADI_STDIO_RESULT_SUCCESS == eResult) &&
               (NULL == pStdioDevice->hOutputSemaphore))
            {

                /* IF (Semaphore creation failed) */
                if ((adi_osal_SemCreate(&pStdioDevice->hOutputSemaphore,
                                        0U)) != ADI_OSAL_SUCCESS)
                {
                    /* Report error (semaphore create failed) */
                    eResult = ADI_STDIO_RESULT_SEMAPHORE_FAILURE;
                }
            }
            
            pStdioDeviceEntry = pStdioDevice->pDeviceEntry;
            
            /* IF (Semaphore creation was successful and Device is not 
               registered with Libio) */
            if((ADI_STDIO_RESULT_SUCCESS == eResult) && 
               (IS_REGISTERED_WITH_LIBIO_FLAG_SET(pStdioDevice->nFlags) == 0U))
            {
                /* IF (Registering this device with LIBIO failed) */
                if(add_devtab_entry(&pStdioDeviceEntry->oLibioDevEntry) == -1)
                {
                    /* Report error (LIBIO device table registration failed) */
                    eResult = ADI_STDIO_RESULT_DEVTAB_REGISTER_FAILED;
                }
                else
                {
                    /* Set the flag to indicate that this device is registered with LIBIO */
                    SET_REGISTERED_WITH_LIBIO_FLAG(pStdioDevice->nFlags);
                }
            }
            
            /* IF (Successfully registered the device with LIBIO) */
            if(ADI_STDIO_RESULT_SUCCESS == eResult)
            {
				/* Get the LibIO default device ID, which is required to set it back */
				nLibioDefaultDeviceId = (uint32_t)get_default_io_device();

				/* Set this device as default IO Device */
                set_default_io_device(pStdioDevice->nLibioDeviceId);

                /* IF (Redirect STDIN console input stream or all the
                   console streams and if redirect is not to existing console
                   input device) */
                if(((ADI_STDIO_STREAM_STDIN == eStreamType )|| 
                    (ADI_STDIO_STREAM_ALL_CONSOLE_IO == eStreamType)) && 
                   (hStdioDevice != ghStdioService->hSTDINDevice))
                {
                    /* Before redirecting flush the input stream */
                    fflush(stdin);
                    
                    /* Close the previously opened input file stream */
                    if(ghStdioService->pSTDINFile != NULL)
                    {
                        fclose(ghStdioService->pSTDINFile);
                        ghStdioService->pSTDINFile = NULL;
                    }

                    /* IF (Redirect to JTAG device) */
                    if(hStdioDevice == ghStdioService->hJTAGDevice)
                    {
                        /* Just copy previously saved standard input stream to stdin stream */
                        memcpy(stdin, &ghStdioService->oSTDINFile, sizeof(FILE));
                    }
                    else
                    {
                        /* Open the Input stream file */
                        ghStdioService->pSTDINFile = fopen("stdin", "a+");

                        /* IF (file is oped successfully) */
                        if(ghStdioService->pSTDINFile != NULL)
                        {
                            /* Copy the newly opened file to standard input stream file */
                            memcpy(stdin, ghStdioService->pSTDINFile, sizeof(FILE));
                            /* disable buffering for console I/O */
                            setvbuf(stdin, NULL, _IONBF, 0U);
                        }
                        /* ELSE (File open failed) */
                        else
                        {
                            /* Report error (Redirect failed) */
                            eResult = ADI_STDIO_RESULT_REDIRECT_FAILED;
                        }
                    }

                    /* IF (Redirecting of input stream was successful) */
                    if(ADI_STDIO_RESULT_SUCCESS == eResult)
                    {
                        /* Save the current input device */
                        ghStdioService->hSTDINDevice = hStdioDevice;
                    }
                }


                /* IF (Redirect STDOUT console output stream or all the
                   console streams and if redirect is not to existing console
                   output device) */
                if( (ADI_STDIO_RESULT_SUCCESS == eResult) &&
                    ((ADI_STDIO_STREAM_STDOUT == eStreamType )|| 
                    (ADI_STDIO_STREAM_ALL_CONSOLE_IO == eStreamType)) && 
                   (hStdioDevice != ghStdioService->hSTDOUTDevice))
                {
                    /* Before redirecting flush the output stream */
                    fflush(stdout);

                    /* Close the previously opened output file stream */
                    if(ghStdioService->pSTDOUTFile != NULL)
                    {
                        fclose(ghStdioService->pSTDOUTFile);
                        ghStdioService->pSTDOUTFile = NULL;
                    }

                    
                    /* IF (Redirect to JTAG device) */
                    if(hStdioDevice == ghStdioService->hJTAGDevice)
                    {
                        /* Just copy previously saved standard output stream to stdout stream */
                        memcpy(stdout, &ghStdioService->oSTDOUTFile, sizeof(FILE));
                    }
                    else
                    {
                        /* Open the Output stream file */
                        ghStdioService->pSTDOUTFile = fopen("stdout", "a+");

                        /* IF (file is oped successfully) */
                        if(ghStdioService->pSTDOUTFile != NULL)
                        {
                            /* Copy the newly opened file to standard output stream file */
                            memcpy(stdout, ghStdioService->pSTDOUTFile, sizeof(FILE));
                            /* disable buffering for console I/O */
                            setvbuf(stdout, NULL, _IONBF, 0U);
                        }
                        /* ELSE (File open failed) */
                        else
                        {
                            /* Report error (Redirect failed) */
                            eResult = ADI_STDIO_RESULT_REDIRECT_FAILED;
                        }
                    }

                    /* IF (Redirecting of output stream was successful) */
                    if(ADI_STDIO_RESULT_SUCCESS == eResult)
                    {
                        /* Save the current output device */
                        ghStdioService->hSTDOUTDevice = hStdioDevice;
                    }
                }


                /* IF (Redirect STDERR console output stream or all the
                   console streams and if redirect is not to existing console
                   error device) */
                if( (ADI_STDIO_RESULT_SUCCESS == eResult) &&
                    ((ADI_STDIO_STREAM_STDERR == eStreamType )|| 
                    (ADI_STDIO_STREAM_ALL_CONSOLE_IO == eStreamType)) && 
                   (hStdioDevice != ghStdioService->hSTDERRDevice))
                {
                    /* Before redirecting flush the error stream */
                    fflush(stderr);

                    /* Close the previously opened error file stream */
                    if(ghStdioService->pSTDERRFile != NULL)
                    {
                        fclose(ghStdioService->pSTDERRFile);
                        ghStdioService->pSTDERRFile = NULL;
                    }
                    
                    /* IF (Redirect to JTAG device) */
                    if(hStdioDevice == ghStdioService->hJTAGDevice)
                    {
                        /* Just copy previously saved standard error stream to stderr stream */
                        memcpy(stderr, &ghStdioService->oSTDERRFile, sizeof(FILE));
                    }
                    else
                    {
                        /* Open the Error stream file */
                        ghStdioService->pSTDERRFile = fopen("stderr", "a+");

                        /* IF (file is oped successfully) */
                        if(ghStdioService->pSTDERRFile != NULL)
                        {
                            /* Copy the newly opened file to standard error stream file */
                            memcpy(stderr, ghStdioService->pSTDERRFile, sizeof(FILE));
                            /* disable buffering for console I/O */
                            setvbuf(stderr, NULL, _IONBF, 0U);
                        }
                        /* ELSE (File open failed) */
                        else
                        {
                            /* Report error (Redirect failed) */
                            eResult = ADI_STDIO_RESULT_REDIRECT_FAILED;
                        }
                    }

                    /* IF (Redirecting of error stream was successful) */
                    if(ADI_STDIO_RESULT_SUCCESS == eResult)
                    {
                        /* Save the current error device */
                        ghStdioService->hSTDERRDevice = hStdioDevice;
                    }
                }
				
				/* Set default device back after redirecting */
                set_default_io_device(nLibioDefaultDeviceId);

            } /* End of IF(Registration of LIBIO was successful)*/

        } /* End of ELSE (Physical device handle is valid) */

        adi_osal_MutexPost(ghStdioService->hMutex);

    }/* End of ELSE (Input parameters are valid) */

    return (eResult);
}

/********************************************************************************************

  Function: adi_stdio_DisableStream

      Disables selected STDIO stream by redirecting the output to
      the NULL device

  Parameters:
      eStreamType - Stream type to disable

  Returns:
      ADI_STDIO_RESULT_SUCCESS
          - Successfully disabled stream
      ADI_STDIO_RESULT_SEMAPHORE_FAILURE
          - Failed to create semaphore
      ADI_STDIO_RESULT_DEVTAB_REGISTER_FAILED
          - Failed to add entry into LIBIO device table entry
      ADI_STDIO_RESULT_SERVICE_NOT_INITIALIZED
          - Stdio service not initialized
********************************************************************************************/

ADI_STDIO_RESULT  adi_stdio_DisableStream (
    ADI_STDIO_STREAM_TYPE   eStreamType
)
{
    /* Return value */    
    ADI_STDIO_RESULT eResult = ADI_STDIO_RESULT_SUCCESS;

    /* IF (STDIO service is not initialized) */
    if(NULL == ghStdioService)
    {
        /* Report error (STDIO service not yet initialized) */
        eResult = ADI_STDIO_RESULT_SERVICE_NOT_INITIALIZED;
    }
    /* ELSE IF (Disable all the streams) */
    else if(ADI_STDIO_STREAM_ALL_CONSOLE_IO == eStreamType)
    {
        /* Disable STDIN stream */
        eResult = DisableStream(ADI_STDIO_STREAM_STDIN);
        
        /* IF (STDIN was successfully disabled) */
        if(ADI_STDIO_RESULT_SUCCESS == eResult)
        {
            /* Disable STDOUT stream */
            eResult = DisableStream(ADI_STDIO_STREAM_STDOUT);
        }
        
        /* IF (STDOUT was successfully disabled) */
        if(ADI_STDIO_RESULT_SUCCESS == eResult)
        {
            /* Disable STDERR stream */
            eResult = DisableStream(ADI_STDIO_STREAM_STDERR);
        }
    }
    /* ELSE (Disable individual streams) */
    else
    {
        /* Disable the requested stream type */
        eResult = DisableStream(eStreamType);
    }
    return (eResult);
}


/********************************************************************************************
  Function: adi_stdio_ControlDevice

      Sets/Gets the configuration parameters for given physical device

  Parameters:
      hStdioDevice        - Handle to the STDIO device
      nCommandID          - Command ID which needs to be set/get.
      pValue              - Pointer that holds the value to be set/get

  Returns:
     ADI_STDIO_RESULT_SUCCESS
         - Successfully closed the given device.
     ADI_STDIO_RESULT_INVALID_HANDLE
         - Handle to the STDIO device is not valid
     ADI_STDIO_RESULT_DEVICE_FAILED
         - Device failed to execute the given command
     ADI_STDIO_RESULT_COMMAND_NOT_SUPPORTED
         - Given command is not supported by the device
********************************************************************************************/
ADI_STDIO_RESULT adi_stdio_ControlDevice(
    ADI_STDIO_DEVICE_HANDLE          hStdioDevice,
    uint32_t                           nCommandID,
    void                        *const pValue
)
{
    /* Return Value */
    ADI_STDIO_RESULT           eResult = ADI_STDIO_RESULT_SUCCESS;
    /* Pointer to STDIO device */
    ADI_STDIO_DEVICE_INFO   *pStdioDevice;
    /* Flag to indicate if configuration has changed */
    bool bConfigChanged       = false;
    
    /* IF (STDIO service is not initialized) */
    if(NULL == ghStdioService)
    {
        /* Report error (STDIO service not yet initialized) */
        eResult = ADI_STDIO_RESULT_SERVICE_NOT_INITIALIZED;
    }
    /* IF (Handle to STDIO device is NULL) */
    else if (NULL == hStdioDevice)
    {
        /* Report error (Invalid STDIO device handle) */
        eResult = ADI_STDIO_RESULT_INVALID_HANDLE;
    }
    /* ELSE (Handle to STDIO device valid) */
    else
    {
        /* Get pointer to STDIO device from the given device handle */
        pStdioDevice = (ADI_STDIO_DEVICE_INFO *) hStdioDevice;

/* IF MISRA diagnostics enabled, suppress rule 1.1 and 11.3*/
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_1_1:"Allow void pointer to be typecasted to a bool value, required for consistency with other SS/DD APIs")
#pragma diag(suppress:misra_rule_11_3:"Allow void pointer to be typecasted to a bool value, required for consistency with other SS/DD APIs")
#endif /* _MISRA_RULES */

        adi_osal_MutexPend(ghStdioService->hMutex, ADI_OSAL_TIMEOUT_FOREVER);

        switch((ADI_STDIO_COMMAND)nCommandID)
        {
            /* CASE (Enable/Disable Unix mode line breaks) */
            case (ADI_STDIO_COMMAND_ENABLE_UNIX_MODE):
                if(true == (bool)pValue)
                {
                    SET_UNIX_MODE_FLAG(pStdioDevice->nFlags);
                }
                else
                {
                    CLEAR_UNIX_MODE_FLAG(pStdioDevice->nFlags);
                }
                bConfigChanged = true;
                break;

            /* CASE (Enable/Disable char echo) */
            case (ADI_STDIO_COMMAND_ENABLE_CHAR_ECHO):
                if(true == (bool)pValue)
                {
                    SET_CHAR_ECHO_FLAG(pStdioDevice->nFlags);
                }
                else
                {
                    CLEAR_CHAR_ECHO_FLAG(pStdioDevice->nFlags);
                }
                bConfigChanged = true;
                break;

            /* CASE (Get the physical device handle) */
            case (ADI_STDIO_COMMAND_GET_DEVICE_HANDLE):
                *(ADI_DEV_DEVICE_HANDLE *)pValue = pStdioDevice->hPhysicalDevice;
                break;

            default:
                /* Call the device specific configuration function */
                eResult = pStdioDevice->pDeviceEntry->pfDeviceControl(pStdioDevice, 
                                                                      nCommandID,
                                                                      pValue);
                break;

        } /* End of switch(nCommandID) */

        adi_osal_MutexPost(ghStdioService->hMutex);

/* IF MISRA diagnostics enabled, enable back rule 1.1 and 11.3*/
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */


        if((bConfigChanged == true) && (ADI_STDIO_RESULT_SUCCESS == eResult))
        {
            /* Current device mappings */
            ADI_STDIO_DEVICE_HANDLE hCurrSTDINDevice  = ghStdioService->hSTDINDevice;
            ADI_STDIO_DEVICE_HANDLE hCurrSTDOUTDevice = ghStdioService->hSTDOUTDevice;
            ADI_STDIO_DEVICE_HANDLE hCurrSTDERRDevice = ghStdioService->hSTDERRDevice;

            /*
            **  Redirect the streams mapped to this device to NULL device
            */

            /* IF (This instance is used for STDIN) */
            if(hCurrSTDINDevice == hStdioDevice)
            {
                /* Disable STDIN Stream */
                eResult = adi_stdio_DisableStream(ADI_STDIO_STREAM_STDIN);

                /* Redirect back STDIN to this device */
                if(ADI_STDIO_RESULT_SUCCESS == eResult)
                {
                    eResult = adi_stdio_RedirectStream(hStdioDevice, ADI_STDIO_STREAM_STDIN);
                }               
            }
            /* IF (Successfully disabled STDIN and this instance is used for STDERR) */
            if((ADI_STDIO_RESULT_SUCCESS == eResult) &&
               (hCurrSTDOUTDevice == hStdioDevice))
            {
                /* Disable STDOUT stream */
                eResult = adi_stdio_DisableStream(ADI_STDIO_STREAM_STDOUT);
                
                /* Redirect back STDOUT to this device */
                if(ADI_STDIO_RESULT_SUCCESS == eResult)
                {
                    eResult = adi_stdio_RedirectStream(hStdioDevice, ADI_STDIO_STREAM_STDOUT);
                }
            }
            /* IF (Successfully disabled STDOUT and this instance is used for STDERR) */
            if((ADI_STDIO_RESULT_SUCCESS == eResult) &&
               (hCurrSTDERRDevice == hStdioDevice))
            {
                /* Disable STDERR stream */
                eResult = adi_stdio_DisableStream(ADI_STDIO_STREAM_STDERR);
                
                /* Redirect back STDERR to this device */
                if(ADI_STDIO_RESULT_SUCCESS == eResult)
                {               
                    eResult = adi_stdio_RedirectStream(hStdioDevice, ADI_STDIO_STREAM_STDERR);
                }   
            }
            
        } /* End of IF (co figuration changed) */

    }  /* End of ELSE(STDIO device handle is valid) */

    return (eResult);
}


/*********************************************************************

    Function: _adi_stdio_GetStdioDevice

        Returns the STDIO device based on LibIO device number.

    Parameters:
        nLibIODeviceID  - LIBIO Device ID.

    Returns:
        pStdioDevice  - Pointer to STDIO device
*********************************************************************/
ADI_STDIO_DEVICE_INFO* _adi_stdio_GetStdioDevice(
    uint32_t                 nLibIODeviceID
)
{
    uint32_t          nDeviceNum;

    /* Get STDIO device number from the given LibIO DeviceID */
    nDeviceNum = (nLibIODeviceID - ADI_STDIO_ENUMERATION_START);

    /* return pointer to STDIO device */
    return(&ghStdioService->paStdioDevice[nDeviceNum]);
}



/*********************************************************************

    Function: adi_stdio_Callback

        Generic Stdio callback

    Parameters:
        hStdioDevice  - Handle to STDIO device
        nEvent          - Callback Event code
        pArgument       - Callback Argument

    Returns:
        None

*********************************************************************/
void _adi_stdio_Callback(
    void  *const hStdioDevice,
    u32    nEvent,
    void  *const pArgument
    )
{
    /* Console buffer that generated this callback */
    ADI_DEV_1D_BUFFER   *pConsoleBuffer;

    switch(nEvent)
    {
        case ((uint32_t)ADI_DEV_EVENT_BUFFER_PROCESSED):
            /* Get the address of console buffer
               that generated this callback */
            pConsoleBuffer = (ADI_DEV_1D_BUFFER *)pArgument;
            /* Post Console Data Semaphore */
            adi_osal_SemPost((ADI_OSAL_SEM_HANDLE)pConsoleBuffer->pAdditionalInfo);
            break;

        /* default (other events) */
        default:
            /* TODO: Implement logic to handle UART status error events */
            break;
    }
}



/*********************************************************************

    Function: adi_stdio_Write

        Writes to file (stdout/stderr) opened over selected device

    Parameters:
        pStdioDevice      - Pointer to STDIO device info
        psDataBuffer      - pointer to buffer containing output elements
        nElementCount     - number elements in output data buffer

    Returns: (return codes as per libio standard)
        -1 - Failed to write to a device
        zero - File (descriptor) is closed
        non-zero (number of bytes transmitted) - Success

*********************************************************************/
/* TODO: finalise section name after agreeing on naming conventions
#pragma default_section (CODE,"adi_bsp_code_slow_cache") */
int32_t _adi_stdio_Write (
    ADI_STDIO_DEVICE_INFO *pStdioDevice,
    uint8_t                 *psDataBuffer,
    int32_t                  nElementCount
)
{
    /* Number of bytes transmitted */
    int32_t     nTotalBytesSent, nNumBytesSent;

    /*
    ** New Line entry for DOS output data mode
    */
    static uint8_t  kganDosNewLine[] = {ADI_STDIO_CHAR_LINE_FEED,
                                        ADI_STDIO_CHAR_NEW_LINE};

    /* IF (we've a valid Transmit buffer) */
    if (psDataBuffer != NULL)
    {
        /* IF (Console output must be in Unix mode) */
        if (IS_UNIX_MODE_FLAG_SET(pStdioDevice->nFlags) == 1U)
        {

          /* Transmit all data */
            nTotalBytesSent = StdioAccess(pStdioDevice,
                                          psDataBuffer,
                                          nElementCount,
                                          false);
        }
        /* ELSE (Console output data should be in DOS mode) */
        else
        {
            /* IF (new line) */
            if (*psDataBuffer == ADI_STDIO_CHAR_NEW_LINE)
            {

/* Disable MISRA diagnostics as necessary */
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_5:"Type-cast required to pass he new line char array to console out device")
#endif /* _MISRA_RULES */

                /* Transmit new line entry (Dos version - "\r\n") */
                nNumBytesSent = StdioAccess(pStdioDevice,
                                            &kganDosNewLine[0],
                                            (int32_t)sizeof(kganDosNewLine),
                                            false);

/* IF MISRA diagnostics enabled, enable MISRA rule 11.5 */
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

                /* IF (Failed to Transmit new line) */
                if (nNumBytesSent < 0)
                {
                    /* return error */
                    nTotalBytesSent = -1;

                }
                else
                {
                    nTotalBytesSent = nElementCount;
                }
            }
            /* ELSE (other characters) */
            else
            {
                /* Transmit all data */
                nTotalBytesSent = StdioAccess(pStdioDevice,
                                              psDataBuffer,
                                              nElementCount,
                                              false);
            }

        } /* End of if (Console output data in Unix mode) else case */

    }
    /* ELSE (return error) */
    else
    {
        nTotalBytesSent = -1;
    }

    return (nTotalBytesSent);
}



/*********************************************************************

    Function: _adi_stdio_Read

        Reads from file (stdin) opened over selected device

    Parameters:
        pStdioDevice      - Pointer to STDIO device information
        psDataBuffer        - pointer to buffer to hold input elements
        nElementCount       - number elements to receive

    Returns: (return codes as per libio standard)
        -1 - Failed to read from a device
        zero - File (descriptor) is closed
        non-zero (number of bytes transmitted) - Success

*********************************************************************/
/* TODO: finalise section name after agreeing on naming conventions
#pragma default_section (CODE,"adi_bsp_code_slow_cache") */
int32_t _adi_stdio_Read (
    ADI_STDIO_DEVICE_INFO *pStdioDevice,
    uint8_t                 *psDataBuffer,
    int32_t                  nElementCount
)
{
    /* Number of bytes received */
    int32_t     nTotalBytesRead;

    /* IF (we've a valid Receive buffer) */
    if (psDataBuffer != NULL)
    {
        /* Read data from the device and fill the receive buffer */
        nTotalBytesRead = StdioAccess(pStdioDevice,
                                      psDataBuffer,
                                      nElementCount,
                                      true);

        /* IF (Successfully read data from console input device) */
        if (nTotalBytesRead > 0)
        {
            /* IF (Character Echo Enabled) */
            if (IS_CHAR_ECHO_FLAG_SET(pStdioDevice->nFlags) == 1U)
            {
                /* IF (Line Feed ('\r') entry) */
                if (*psDataBuffer == ADI_STDIO_CHAR_LINE_FEED)
                {
                    /* IF (Unix Output mode) */
                    if (IS_UNIX_MODE_FLAG_SET(pStdioDevice->nFlags) == 1U)
                    {
                        /* Echo this character */
                        printf("%c",*psDataBuffer);
                    }
                    /* ELSE (DOS Output mode) */
                    else
                    {
                        /* Echo new Line Entry */
                        printf("\n");
                    }
                }
                /* ELSE IF (Backspace key pressed) */
                else if (*psDataBuffer == ADI_STDIO_CHAR_BACKSPACE)
                {
                    /*
                       Don't echo this character and leave it to the
                       application to deal with it
                    */
                }
                /* ELSE (other characters) */
                else
                {
                    /* Echo this character */
                    printf("%c",*psDataBuffer);
                }

            } /* End of if (Character Echo Enabled) */

        } /* End of if (Successfully read data from console input device) */

    } /* End of if (we've a valid Receive buffer) */

    return (nTotalBytesRead);
}



/********************************************************************************************

  Function: OpenDevice

      Opens the given physical device for STDIO.

  Parameters:
      eDeviceType         - STDIO device type (JTAG/UART/NULL..)
      nPhysicalDevNum     - Physical device number to open
      phStdioDevice       - Pointer to STDIO device handle.

  Returns:
     ADI_STDIO_RESULT_SUCCESS
         - Successfully opened the given device.
     ADI_STDIO_RESULT_NO_STDIO_DEVICES
         - No free STDIO devices
     ADI_STDIO_RESULT_DEVICE_FAILED
         - Failed to open the given physical device
     ADI_STDIO_RESULT_INSUFFICIENT_MEMORY
         - Failed to allocated memory required for the device     
     ADI_STDIO_RESULT_DEVICE_NOT_REGISTERED
         - Given device type is not registered with the service
********************************************************************************************/
static ADI_STDIO_RESULT  OpenDevice (
    uint32_t                     nDeviceType,
    uint32_t                     nPhysicalDevNum,
    ADI_STDIO_DEVICE_HANDLE     *phStdioDevice
)
{    

    ADI_STDIO_RESULT           eResult = ADI_STDIO_RESULT_SUCCESS;
    /* Index variable */
    uint32_t                   i;
    /* Pointer to STDIO device */
    ADI_STDIO_DEVICE_INFO     *pStdioDevice;
    /* Pointer to STDIO device info structure */
    ADI_STDIO_DEVICE_ENTRY    *pStdioDeviceEntry;

    pStdioDevice = ghStdioService->paStdioDevice;

    /* Find a free STDIO device instance */
    for(i = 0U; i < ADI_STDIO_MAX_DEVICES; i++)
    {
        /* IF (The stdio device is free) */
        if(IS_DEVICE_IN_USE_FLAG_SET(pStdioDevice->nFlags) == 0U)
        {
            break;
        }
        pStdioDevice++;
    }
        
    /* IF (Free device not available) */
    if(i == ADI_STDIO_MAX_DEVICES)
    {
        /* Report error (No more STDIO devices available */
        eResult = ADI_STDIO_RESULT_NO_STDIO_DEVICES;
    }
    /* IF (Device registered with the service) */
    else if(NULL == _adi_stdio_pDeviceEntries[nDeviceType])
    {
        /* Report error (Device not registered with the service) */
        eResult = ADI_STDIO_RESULT_DEVICE_NOT_REGISTERED;
    } 
    else
    {
        eResult =  _adi_stdio_MemAlloc((void **)&pStdioDevice->pDeviceEntry, 
                                        sizeof(ADI_STDIO_DEVICE_ENTRY) 
                                       );

        if(eResult == ADI_STDIO_RESULT_SUCCESS)
        {
            /* Store the physical device entry info into the current device */
            *pStdioDevice->pDeviceEntry          = *_adi_stdio_pDeviceEntries[nDeviceType];
            /* Get the pointer to device entry info */
            pStdioDeviceEntry                    = pStdioDevice->pDeviceEntry;
            /* Open the physical device */
            eResult = pStdioDeviceEntry->pfDeviceOpen (pStdioDevice,
                                                       nPhysicalDevNum,
                                                       ghStdioService->hDeviceMgr,
                                                       ghStdioService->hDMAMgr,
                                                       ghStdioService->hDCBMgr);
        }
        /* IF (Device opened successfully) */
        if(ADI_STDIO_RESULT_SUCCESS == eResult)
        {
            /*
            ** Initialize the STDIO device members
            */
                

            /* pStdioDevice->nLibioDeviceId is not required to be initialized. 
               It is initialized during the STDIO init */

            /* Set the flag to indicate that this device is in use */
            SET_DEVICE_IN_USE_FLAG(pStdioDevice->nFlags);
            /* Set the Libio device Id*/
            pStdioDeviceEntry->oLibioDevEntry.DeviceID  = (int32_t)pStdioDevice->nLibioDeviceId;
               
            /* hDevice will be filled by the Device specific open function */

            /* Clear the input semaphore handle */
            pStdioDevice->hInputSemaphore               = NULL;
            /* Clear the output semaphore handle */
            pStdioDevice->hOutputSemaphore              = NULL;
            /* Return the STDIO device handle */
            *phStdioDevice                              = (ADI_STDIO_DEVICE_HANDLE) pStdioDevice;
        }
    }

    return eResult;
}


/*********************************************************************

    Function: StdioAccess

        Function to submit read/write buffer to selected Stdio device

    Parameters:
        pStdioDevice      - Pointer to STDIO device information
        psDataBuffer        - pointer to transmit/receive data buffer
        nElementCount       - number elements in receive/transmit buffer
        bReadFlag           - 'true' to read from Stdio device
                              'false' to write to Stdio device

    Returns: (return codes as per libio standard)
        -1 - Failed to access a device
        zero - File (descriptor) is closed
        non-zero (number of bytes transmitted) - Success

*********************************************************************/
/* TODO: finalise section name after agreeing on naming conventions
#pragma default_section (CODE,"adi_bsp_code_slow_cache") */

/* Disable MISRA diagnostics as necessary */
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_16_7:"psDataBuffer can not be a constant as it gets modified indirectly through DD/SS")
#endif /* _MISRA_RULES */

static int32_t  StdioAccess (
    ADI_STDIO_DEVICE_INFO *pStdioDevice,
    uint8_t                 *psDataBuffer,
    int32_t                  nElementCount,
    bool                     bReadFlag
)
{
    /* return code - assume file descriptor is closed */
    int32_t                 nResult = 0;
    /* DD/SS Result */
    uint32_t                nDDSSResult;
    /* 1D Tx/Rx buffer */
    ADI_DEV_1D_BUFFER       oDeviceAccessBuffer;
    /* OSAL Result */
    ADI_OSAL_STATUS         eOsalResult = ADI_OSAL_SUCCESS;

    /* IF (we've some data to transmit/receive) */
    if (nElementCount > 0)
    {
        /* IF (we've a valid device handle) */
        if (pStdioDevice->hPhysicalDevice != NULL)
        {
            /* set up Device Access Buffer */
            oDeviceAccessBuffer.Data                = (void*)psDataBuffer;
            oDeviceAccessBuffer.ElementWidth        = sizeof(uint8_t);
            oDeviceAccessBuffer.ElementCount        = (uint32_t)nElementCount;
            oDeviceAccessBuffer.CallbackParameter   = (void *)&oDeviceAccessBuffer;
            oDeviceAccessBuffer.pNext               = NULL;

            /*
            ** Queue Buffer to the selected device
            */

            /* IF (Read from selected device) */
            if (bReadFlag)
            {
                /* Pass Device Semaphore handle as additional info */
                oDeviceAccessBuffer.pAdditionalInfo     = (void *)pStdioDevice->hInputSemaphore;


                /* Disable MISRA diagnostics as necessary */
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4:"Object type to a different pointer type casting required for DD/SS")
#endif /* _MISRA_RULES */




                /* Queue read buffer to the console input device */
                nDDSSResult = adi_dev_Read(pStdioDevice->hPhysicalDevice,
                                           ADI_DEV_1D,
                                           (ADI_DEV_BUFFER *)&oDeviceAccessBuffer);
                
                /* IF MISRA diagnostics enabled, enable MISRA rule 11.4 */
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

            }
            /* ELSE (Write to selected device) */
            else
            {
                /* Pass Device Semaphore handle as additional info */
                oDeviceAccessBuffer.pAdditionalInfo     = (void *)pStdioDevice->hOutputSemaphore;

                /* Disable MISRA diagnostics as necessary */
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4:"Object type to a different pointer type casting required for DD/SS")
#endif /* _MISRA_RULES */

                /* Queue read buffer to the console input device */
                nDDSSResult = adi_dev_Write(pStdioDevice->hPhysicalDevice,
                                            ADI_DEV_1D,
                                            (ADI_DEV_BUFFER *)&oDeviceAccessBuffer);
                
                /* IF MISRA diagnostics enabled, enable MISRA rule 11.4 */
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

            }

            /* IF (Transmit/Receive buffer submission success) */
            if (nDDSSResult == (uint32_t)ADI_DEV_RESULT_SUCCESS)
            {
                /* Await for buffer completion */
                eOsalResult = adi_osal_SemPend((ADI_OSAL_SEM_HANDLE) oDeviceAccessBuffer.pAdditionalInfo,
                                               ADI_OSAL_TIMEOUT_FOREVER);
                
            }

            /* IF (Data transfer failed) */
            if ((nDDSSResult != (uint32_t)ADI_DEV_RESULT_SUCCESS) ||
                (eOsalResult != ADI_OSAL_SUCCESS) ||
                (oDeviceAccessBuffer.ProcessedElementCount == 0U))
            {
                /* return error */
                nResult = -1;
            }
            else
            {
                /* return number of bytes transfered/received over UART 1 */
                nResult += (int32_t)oDeviceAccessBuffer.ProcessedElementCount;
            }
        }
        /* ELSE (return error as we don't have a valid device handle) */
        else
        {
            nResult = -1;
        }

    } /* End of if (we've some data to transmit/receive) */

    return (nResult);
}

/* IF MISRA diagnostics enabled, enable MISRA rule 16.7 */
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

/********************************************************************************************

  Function: DisableStream

      Disables selected STDIO stream by redirecting the output to
      the NULL device

  Parameters:
      eStreamType - Stream type to disable

  Returns:
      ADI_STDIO_RESULT_SUCCESS
          - Successfully disabled stream
      ADI_STDIO_RESULT_INVALID_HANDLE
          - Given Device handle is invalid
      ADI_STDIO_RESULT_STDIO_STREAM_NOT_SUPPORTED
          - Invalid stream type or
            Stream type not supported by the selected device
      ADI_STDIO_RESULT_STDIO_REDIRECT_FAILED
          - Failed to re-direct stream
      ADI_STDIO_RESULT_STDIO_DEVICE_FAILED
          - STDIO device/hardware initialisation failed
********************************************************************************************/

static ADI_STDIO_RESULT  DisableStream(ADI_STDIO_STREAM_TYPE eStreamType)
{
    /* Return value */    
    ADI_STDIO_RESULT eResult = ADI_STDIO_RESULT_SUCCESS;

    switch(eStreamType)
    {
        /* CASE (STDIN Stream type) */
        case (ADI_STDIO_STREAM_STDIN):
            /* Redirect STDIN to NULL device */
            eResult = adi_stdio_RedirectStream(ghStdioService->hNULLDevice,
                                               ADI_STDIO_STREAM_STDIN);
            break;
        /* CASE (STDOUT Stream type) */
        case (ADI_STDIO_STREAM_STDOUT):
            /* Redirect STDOUT to NULL device */
            eResult = adi_stdio_RedirectStream(ghStdioService->hNULLDevice,
                                               ADI_STDIO_STREAM_STDOUT);
            break;
        /* CASE (STDERR Stream type) */
        case (ADI_STDIO_STREAM_STDERR):
            /* Redirect STDERR to NULL device */
            eResult = adi_stdio_RedirectStream(ghStdioService->hNULLDevice,
                                               ADI_STDIO_STREAM_STDERR);
            break;

        /* DEFAULT (other types) */
        default:
            eResult = ADI_STDIO_RESULT_STREAM_NOT_SUPPORTED;
            break;
    }

    return (eResult);
}


 /*********************************************************************
  Function: _adi_stdio_MemAlloc

      Call heap_malloc or malloc to allocate memory

  Parameters:
      nSize     - Required memory block size
      ppData    - Pointer to a location where the address of the 
                  allocated memory block will be written
                  
  Returns:
      ADI_STDIO_RESULT_SUCCESS
                - Successfully allocated memory block of given size
      ADI_STDIO_RESULT_INSUFFICIENT_MEMORY
                - Failed to allocate memory block of given size
*********************************************************************/
ADI_STDIO_RESULT _adi_stdio_MemAlloc( void **ppData, uint32_t nSize )
{
    /* Return code - assume we're going to be successful */
    ADI_STDIO_RESULT  eResult = ADI_STDIO_RESULT_SUCCESS;

    void* pMemory;
    /* pass request directly to heap_malloc. We don't need to check whether the
     * pool has been set to 0 or not. If it has heap_free is equivalent to free
     * which is what we want anyway
     */

    pMemory = heap_malloc(ghStdioService->hHeapIndex, nSize);

    if (pMemory != NULL)
    {
        *ppData = pMemory;
        eResult = ADI_STDIO_RESULT_SUCCESS;
    }
    else
    {
        eResult = ADI_STDIO_RESULT_INSUFFICIENT_MEMORY;
    }

    return eResult;
}

/*********************************************************************
  Function: _adi_stdio_MemFree

      Calls heap_free to free the given memory block

  Parameters:
      pData    - Pointer to the memory block which is to be freed
                  
  Returns:
      None
*********************************************************************/
void _adi_stdio_MemFree(void *pData)
{

 /* pass request directly to heap_free. We don't need to check whether the pool
    has been set to 0 or not. If it has heap_free is equivalent to free which
    is what we want anyway
  */

    heap_free(ghStdioService->hHeapIndex, pData);
}


/* IF MISRA diagnostics enabled, re-enable rules that are suppressed for this file 
      (5.1, 5.6, 5.7, 19.7, 17.4 and 20.9) */
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

/*
**
** EOF:
**
*/

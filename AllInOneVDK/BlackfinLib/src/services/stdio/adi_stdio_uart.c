/* $Revision: 3492 $
 * $Date: 2010-10-07 09:34:20 -0400 (Thu, 07 Oct 2010) $ 
**************************************************************************** 
Copyright (c), 2008-2009 - Analog Devices Inc. All Rights Reserved. 
This software is proprietary & confidential to Analog Devices, Inc. 
and its licensors. 
****************************************************************************

Title: STDIO Service 

Description: This file contains the STDIO service UART device specific 
             implementation
*****************************************************************************/

/* IF MISRA diagnostics enabled, suppress selected MISRA rules for this file as necessary */
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_5_1 :"Most identifiers require more than 31 characters for readability. Rule disabled to avoid truncation")
#pragma diag(suppress:misra_rule_5_6 :"Most functions share same parameter name(s) for code readability")
#pragma diag(suppress:misra_rule_5_7 :"Most functions, structures and unions share same identifier name(s) for code readability")
#pragma diag(suppress:misra_rule_6_3:"Allow basic data types for LIBIO APIs")
#pragma diag(suppress:misra_rule_19_7:"Allow use of macros for bitwise operations")
#pragma diag(suppress:misra_rule_17_4:"Allow array indexing on pointers")
#pragma diag(suppress:misra_rule_20_9:"Stdio class devices leverage input/output library <stdio.h> for POSIX functions")
#pragma diag(suppress:misra_rule_16_7:"Even though some pointer parameters are not used for this device they may be used by other device implementation. So for API consistency it is required them to keep them as non constant pointers ")
#endif /* _MISRA_RULES */

/*=============  I N C L U D E S   =============*/
#include <string.h>
#include <services/stdio/adi_stdio.h>
#include "adi_stdio_common.h"

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rules_all :"Disable MISRA checking for DD/SS")
#endif /* _MISRA_RULES */

#include <drivers/adi_dev.h>
#include <drivers/uart/adi_uart.h>

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */


/*==============  D E F I N E S  ===============*/

/* 
**  Default UART configuration Values
*/

/*
** UART Wordlength
*/
#define ADI_STDIO_DEFAULT_UART_WLEN             8U
/*
** UART Stop bits
*/
#define ADI_STDIO_DEFAULT_UART_STOP_BITS        1U
/*
** UART Auto-baud character
*/
#define ADI_STDIO_DEFAULT_UART_AUTOBAUD_CHAR    0x0AU
/*
** UART Baud rate
*/
#define ADI_STDIO_DEFAULT_UART_BAUD_RATE        56000U


/*
**  Local Functions declaration 
*/

/* Function open UART Device */
static ADI_STDIO_RESULT adi_stdio_Uart_DeviceOpen(
        ADI_STDIO_DEVICE_INFO                 *pStdioDevice,
        uint32_t                                 nPhysicalDevNum,
        ADI_DEV_MANAGER_HANDLE             const hDeviceMgr,
        ADI_DMA_MANAGER_HANDLE             const hDmaMgr,
        ADI_DCB_HANDLE                     const hDcbMgr
);

/* Function to Close UART Device */
static ADI_STDIO_RESULT adi_stdio_Uart_DeviceClose(
    ADI_STDIO_DEVICE_INFO               *const pStdioDevice
);

static ADI_STDIO_RESULT adi_stdio_Uart_DeviceControl(
    ADI_STDIO_DEVICE_INFO               *const pStdioDevice,
    uint32_t                                     nCommandID,
    void                                  *const pValue
);

/* UART Callback function */
static void adi_stdio_Uart_Callback(
    void        *hStdioDevice,
    u32          nEvent,
    void        *pArgument
);


/* Function to configure UART Device */
static ADI_STDIO_RESULT STDIO_Uart_DeviceConfig(
    ADI_STDIO_DEVICE_INFO               *const pStdioDevice
);

/*
** Local functions declaration required for registering with LIBIO
*/

/* Open a File over UART Stdio device */
static int32_t  Stdio_Uart_Init (
    struct DevEntry    *device
);


/* Open a File over UART Stdio device */
static int32_t  Stdio_Uart_Open (
    const char_t       *szFilePath,
    int32_t             nFileMode
);

/* Close a File opened over UART Stdio device */
static int32_t  Stdio_Uart_Close (
    int32_t             nFileDescriptor
);

/* Write to file opened over UART Stdio device */
static int32_t  Stdio_Uart_Write (
    int32_t             nFileDescriptor,
    uint8_t            *psDataBuffer,
    int32_t             nBufferSize
);

/* Read from a file opened over UART Stdio device */
static int32_t  Stdio_Uart_Read (
    int32_t             nFileDescriptor,
    uint8_t            *psDataBuffer,
    int32_t             nBufferSize
);

/* Seek to new position in a file opened over UART Stdio device */
static long  Stdio_Uart_Seek (
    int32_t             nFileDescriptor,
    long                nOffset,
    int32_t             nWhence
);


/*=============  D A T A  =============*/

/* 
   Structure containing UART Device specific implementation
   function pointers
*/    
ADI_STDIO_DEVICE_ENTRY adi_stdio_UART_DeviceEntry = {

    /* LIBIO Device Entry device for UART */
    {
        /* Initialize DeviceID to an invalid value, it will be set by the STDIO service */
        -1,
        /* Set Pointer to private data to NULL */
        NULL,
        /* Iinit function pointer*/
        &Stdio_Uart_Init,
        /* File Open function pointer */
        &Stdio_Uart_Open,
        /* File Close function pointer */
        &Stdio_Uart_Close,
        /* File Write function pointer */
        &Stdio_Uart_Write,
        /* File Read function pointer */
        &Stdio_Uart_Read,
        /* File Seek function pointer */
        &Stdio_Uart_Seek,
        /* stdinfd - don't claim file access */
        dev_not_claimed,
        /* stdoutfd - don't claim file access */
        dev_not_claimed,
        /* stderrfd - don't claim file access */
        dev_not_claimed,
    },
    /* Function pointers required by the STDIO service  */

    /* Pointer to Device Open function */
    &adi_stdio_Uart_DeviceOpen,
    /* Pointer to Device Close function */
    &adi_stdio_Uart_DeviceClose,
    /* Pointer to Device Control function */
    &adi_stdio_Uart_DeviceControl
};


/*=============  C O D E  =============*/

/*
**  Function Definition section
*/


/**********************************************************

  Function: adi_stdio_RegisterUART

      Registers the UART device with STDIO service

      This function is primarily helpful to avoid UART device related
      files being linked when not used.
      
  Parameters:
      None

  Returns:
      None
**********************************************************/
void adi_stdio_RegisterUART(void)
{
    /* Pass the device entry information to the STDIO service */
    _adi_stdio_pDeviceEntries[ADI_STDIO_DEVICE_TYPE_UART] = &adi_stdio_UART_DeviceEntry;
}

/**********************************************************
  Function: adi_stdio_Uart_DeviceOpen

      Opens the UART device for STDIO

  Parameters:
      pStdioDevice      - Pointer to the STDIO device
      nPhysicalDevNum     - Physical device number to open
      hDeviceMgr          - Handle to Device Manager
      hDmaMgr             - Handle to DMA Manager
      hDcbMgr             - Handle to DCB Manager

  Returns:
     ADI_STDIO_RESULT_SUCCESS
         - Successfully opened the given device.
     ADI_STDIO_RESULT_INVALID_POINTER
         - Pointer to STDIO device is not valid
     ADI_STDIO_RESULT_DEVICE_FAILED
         - Failed to open the given physical device
**********************************************************/
static ADI_STDIO_RESULT adi_stdio_Uart_DeviceOpen(
        ADI_STDIO_DEVICE_INFO           *pStdioDevice,
        uint32_t                           nPhysicalDevNum,
        ADI_DEV_MANAGER_HANDLE       const hDeviceMgr,
        ADI_DMA_MANAGER_HANDLE       const hDmaMgr,
        ADI_DCB_HANDLE               const hDcbMgr
)
{
    /* Return code */
    ADI_STDIO_RESULT  eResult = ADI_STDIO_RESULT_SUCCESS;
    /* DD/SS Configuration table */
    ADI_DEV_CMD_VALUE_PAIR  aoUartDriverConfig[8];

    /* IF (Pointer to STDIO device is not valid) */
    if(NULL == pStdioDevice)
    {
        /* Report error (Invalid pointer) */
        eResult = ADI_STDIO_RESULT_INVALID_POINTER;
    }
    /* ELSE (STDIO device is valid) */
    else
    {
        /* Open physical UART devcie */
        if((adi_dev_Open(hDeviceMgr,
                         &ADIUARTEntryPoint,
                         nPhysicalDevNum,
                         (void *) pStdioDevice,
                         &(pStdioDevice->hPhysicalDevice),
                         ADI_DEV_DIRECTION_BIDIRECTIONAL,
                         NULL,
                         hDcbMgr,
                         adi_stdio_Uart_Callback)
            )!= (uint32_t)ADI_DEV_RESULT_SUCCESS)
        {
            /* Report error (Failed to open a device) */
            eResult = ADI_STDIO_RESULT_DEVICE_FAILED;
        }
        
        /* Configure and start data flow */
        eResult = STDIO_Uart_DeviceConfig(pStdioDevice);

    } /* End of ELSE (STDIO device valid) */

    return (eResult);
}

/**********************************************************
  Function: adi_stdio_Uart_DeviceClose

      Closes UART device

  Parameters:
     pStdioDevice      - pointer to the STDIO device

  Returns:
     ADI_STDIO_RESULT_SUCCESS
         - Successfully closed the given device.
     ADI_STDIO_RESULT_INVALID_POINTER
         - Pointer to the STDIO device is not valid
     ADI_STDIO_RESULT_DEVICE_FAILED
         - Failed to close the given physical device      
**********************************************************/
static ADI_STDIO_RESULT adi_stdio_Uart_DeviceClose(
    ADI_STDIO_DEVICE_INFO          *const pStdioDevice
)
{
    /* Return code */
    ADI_STDIO_RESULT  eResult = ADI_STDIO_RESULT_SUCCESS;
    
    /* IF (Pointer to STDIO device is not valid) */
    if(NULL == pStdioDevice)
    {
        /* Report error (Invalid pointer) */
        eResult = ADI_STDIO_RESULT_INVALID_POINTER;
    }
    /* ELSE (STDIO device is valid) */
    else
    {
        /* IF (Failed to close UART) */
        if (adi_dev_Close(pStdioDevice->hPhysicalDevice) != (uint32_t)ADI_DEV_RESULT_SUCCESS)
        {
            /* Report error (failed to close Stdio device) */
            eResult = ADI_STDIO_RESULT_DEVICE_FAILED;
        }
    }
    
    return (eResult);
}



/*************************************************************
  Function: adi_stdio_Uart_DeviceControl

      Sets/Gets the configuration parameters for given physical device

  Parameters:
      pStdioDevice      - Pointer to the STDIO device
      nCommandID          - Command ID which needs to be set/get.
      pValue              - Pointer that holds the value to be set/get

  Returns:
     ADI_STDIO_RESULT_SUCCESS
         - Successfully configured the given device.
     ADI_STDIO_RESULT_INVALID_POINTER
         - Pointer to the STDIO device is not valid
     ADI_STDIO_RESULT_DEVICE_FAILED
         - Device failed to exicute the given command
     ADI_STDIO_RESULT_COMMAND_NOT_SUPPORTED
         - Given command is not supported by the device
****************************************************************/
ADI_STDIO_RESULT adi_stdio_Uart_DeviceControl(
    ADI_STDIO_DEVICE_INFO           *pStdioDevice,
    uint32_t                           nCommandID,
    void *const                        pValue
)
{
    /* Return code */
    ADI_STDIO_RESULT               eResult = ADI_STDIO_RESULT_SUCCESS;
    /* Physical device Command */
    uint32_t                       nDeviceCmd;
    /* Data to be passed to physical device */
    void                          *pDeviceData;

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_3 :"Allow void pointer to be typecasted to a bool value, requred for consistancy with other SS/DD APIs")
#endif /* _MISRA_RULES */

    
    /* IF (Given pointer to the STDIO device is NULL) */
    if(pStdioDevice == NULL)
    {
        /* Report error (Given pointer to STDIO device is invalid) */
        eResult = ADI_STDIO_RESULT_INVALID_POINTER;
    }
    else
    {
        /* IF (Able Disable data flow failed) */
        if((adi_dev_Control(pStdioDevice->hPhysicalDevice,
                            (uint32_t)ADI_DEV_CMD_SET_DATAFLOW,
                            (void *)false))
           != (uint32_t)ADI_DEV_RESULT_SUCCESS)
        {
            /* Report error (Failed to configure STDIO device) */
            eResult = ADI_STDIO_RESULT_DEVICE_FAILED;
        }
        else
        {
            /* Assume that we pass the same data passed from application to the physical device */
            pDeviceData = pValue;

            switch(nCommandID)
            {
                /* Set parity (none/odd/even)*/
                case (ADI_STDIO_COMMAND_SET_UART_PARITY_TYPE):
                    
                    /* IF (Parity type is None) */
                    if((ADI_STDIO_PARITY_TYPE) pValue ==  ADI_STDIO_PARITY_TYPE_NONE)
                    {
                        /* Set device command data to disable parity check */
                        nDeviceCmd  = (uint32_t)ADI_UART_CMD_ENABLE_PARITY;
                        pDeviceData = (void *) false;
                    }
                    else
                    {
                        /* Set device command to set the parity type */
                        nDeviceCmd = ADI_UART_CMD_SET_PARITY;

                        /* Set the parity type */
                        if((adi_dev_Control(pStdioDevice->hPhysicalDevice,
                                            nDeviceCmd,
                                            pDeviceData))
                           != (uint32_t)ADI_DEV_RESULT_SUCCESS)
                        {
                            /* Report error (Failed to configure STDIO device) */
                            eResult = ADI_STDIO_RESULT_DEVICE_FAILED;
                        }
                        else
                        {
                            /* Set the device command and data to enable parity check */
                            nDeviceCmd  = (uint32_t)ADI_UART_CMD_ENABLE_PARITY;
                            pDeviceData = (void *) true;
                        }
                    }
                    break;
                    /* Set data word length, valid word lengths are 5, 6, 7, 8 */
                case (ADI_STDIO_COMMAND_SET_UART_WORD_LENGTH):
                    nDeviceCmd  = (uint32_t)ADI_UART_CMD_SET_DATA_BITS;
                    break;
                    /* Set number of stop bits, valid values are 1 or 2 */
                case (ADI_STDIO_COMMAND_SET_UART_NUM_STOP_BITS):
                    nDeviceCmd  = (uint32_t)ADI_UART_CMD_SET_STOP_BITS;
                    break;
                    /* Set the charecter to be used in autobaud mode, valid values are any charecter */
                case (ADI_STDIO_COMMAND_SET_UART_AUTO_BAUD_CHAR):
                    nDeviceCmd  = (uint32_t)ADI_UART_CMD_SET_AUTOBAUD_CHAR;
                    break;

                    /* Enable auto baud detection */
                case (ADI_STDIO_COMMAND_ENABLE_AUTO_BAUD):
                    nDeviceCmd  = (uint32_t)ADI_UART_CMD_AUTOBAUD;
                    break;

                    /* Set the UART baud rate */
                case (ADI_STDIO_COMMAND_SET_UART_BAUD_RATE):
                    nDeviceCmd  = (uint32_t)ADI_UART_CMD_SET_BAUD_RATE;
                    break;
                    
                    /* All the other values are considered as invvalid commands */
                default:
                    eResult = ADI_STDIO_RESULT_COMMAND_NOT_SUPPORTED;
                    break;
                    
            }

            /* IF (Previouse commands were successful) */
            if(eResult == ADI_STDIO_RESULT_SUCCESS)
            {
                /* 
                **  Configure UART device 
                */

                if((adi_dev_Control(pStdioDevice->hPhysicalDevice,
                                    nDeviceCmd,
                                    pDeviceData))
                   != (uint32_t)ADI_DEV_RESULT_SUCCESS)
                {
                    /* Report error (Failed to configure STDIO device) */
                    eResult = ADI_STDIO_RESULT_DEVICE_FAILED;
                }
                else
                {
                    /* Enable data flow again */
                    if((adi_dev_Control(pStdioDevice->hPhysicalDevice,
                                        nDeviceCmd,
                                        (void *)true))
                       != (uint32_t)ADI_DEV_RESULT_SUCCESS)
                    {
                        /* Report error (Failed to configure STDIO device) */
                        eResult = ADI_STDIO_RESULT_DEVICE_FAILED;
                    }
                }
            }
        }
    }

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

    return eResult;
}


/**********************************************************
  Function: STDIO_Uart_DeviceConfig

     Configures the given physical device

  Parameters:
     pStdioDevice      - Pointer to the STDIO device

  Returns:
     ADI_STDIO_RESULT_SUCCESS
         - Successfully closed the given device.
     ADI_STDIO_RESULT_INVALID_POINTER
         - Pointer to the STDIO device is not valid
     ADI_STDIO_RESULT_DEVICE_FAILED
         - Failed to configure the given physical device
**********************************************************/
static ADI_STDIO_RESULT STDIO_Uart_DeviceConfig(
    ADI_STDIO_DEVICE_INFO           *const pStdioDevice
)
{
    /* Return code */
    ADI_STDIO_RESULT               eResult = ADI_STDIO_RESULT_SUCCESS;
    /* DD/SS Configuration table */
    ADI_DEV_CMD_VALUE_PAIR         aoUartDriverConfig[8];

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_3 :"Allow void pointer to be typecasted to a bool value, requred for consistancy with other SS/DD APIs")
#endif /* _MISRA_RULES */
    
    /* IF (Pointer to STDIO device is not valid) */
    if(NULL == pStdioDevice)
    {
        /* Report error (Invalid pointer) */
        eResult = ADI_STDIO_RESULT_INVALID_POINTER;
    }
    /* ELSE (STDIO device is valid) */
    else
    {
        /* IF (Pointer to STDIO device is not valid) */
        if(NULL == pStdioDevice)
        {
            /* Report error (Invalid pointer) */
            eResult = ADI_STDIO_RESULT_INVALID_POINTER;
        }
        /* ELSE (STDIO device is valid) */
        else
        {
            
            /*** Populate DD/SS configuration table to configure UART for STDIO operations ***/

            /* Dataflow method */            

            aoUartDriverConfig[0].CommandID  = (uint32_t)ADI_DEV_CMD_SET_DATAFLOW_METHOD;
            aoUartDriverConfig[0].Value      = (void *)ADI_DEV_MODE_CHAINED;
            /* Number of data bits */
            aoUartDriverConfig[1].CommandID  = (uint32_t)ADI_UART_CMD_SET_DATA_BITS;
            aoUartDriverConfig[1].Value      = (void *)ADI_STDIO_DEFAULT_UART_WLEN;
            /* Disable parity */
            aoUartDriverConfig[2].CommandID  = (uint32_t)ADI_UART_CMD_ENABLE_PARITY;
            aoUartDriverConfig[2].Value      = (void *)false;
            /* Number of stop bits */
            aoUartDriverConfig[3].CommandID  = (uint32_t)ADI_UART_CMD_SET_STOP_BITS;
            aoUartDriverConfig[3].Value      = (void *)ADI_STDIO_DEFAULT_UART_STOP_BITS;
            /* Baud rate */
            aoUartDriverConfig[4].CommandID  = (uint32_t)ADI_UART_CMD_SET_BAUD_RATE;
            aoUartDriverConfig[4].Value      = (void *)ADI_STDIO_DEFAULT_UART_BAUD_RATE;
            /* Enable error reporting */
            aoUartDriverConfig[5].CommandID  = (uint32_t)ADI_UART_CMD_SET_LINE_STATUS_EVENTS;
            aoUartDriverConfig[5].Value      = (void *)true;
            /* Enable dataflow */
            aoUartDriverConfig[6].CommandID  = (uint32_t)ADI_DEV_CMD_SET_DATAFLOW;
            aoUartDriverConfig[6].Value      = (void *)true;
            /* End of configuration table */
            aoUartDriverConfig[7].CommandID  = (uint32_t)ADI_DEV_CMD_END;

            /* Configure UART 1 device */
            if((adi_dev_Control(pStdioDevice->hPhysicalDevice,
                                (uint32_t)ADI_DEV_CMD_TABLE,
                                (void *)&aoUartDriverConfig[0]))
               != (uint32_t)ADI_DEV_RESULT_SUCCESS)
            {
                /* Report error (Failed to configure STDIO device) */
                eResult = ADI_STDIO_RESULT_DEVICE_FAILED;
            }
        
        } /* End of ELSE (STDIO device is valid) */

    } /* End of ELSE (STDIO device is valid) */

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */



    return (eResult);
}





/*********************************************************************

    Function: adi_stdio_Uart_Callback

        UART Stdio callback

    Parameters:
        hStdioDevice   - Handle to the STDIO device
        nEvent           - Callback Event code
        pArgument        - Callback Aurgument

    Returns:
        None

*********************************************************************/
static void adi_stdio_Uart_Callback(
    void        *hStdioDevice,
    u32          nEvent,
    void        *pArgument
    )
{
    /* CASE OF (nEvent) */
    switch (nEvent)
    {
        /* CASE (UART Auto buad complete) */
        case ((uint32_t)ADI_UART_EVENT_AUTOBAUD_COMPLETE):
            /* no action to be taken */
            break;

        /* default (other events) */
        default:
            /* Call the generic callback handler for other events */
            _adi_stdio_Callback(hStdioDevice, nEvent, pArgument);
            break;
    }
}



/* Open a File over UART Stdio device */
static int32_t  Stdio_Uart_Init (
    struct DevEntry     *device
)
{
    return 0;
}


/* Open a File over UART Stdio device */
static int32_t  Stdio_Uart_Open (
    const char_t        *szFilePath,
    int32_t             nFileMode
)
{
    uint32_t nLibioDeviceId;

/* IF MISRA diagnostics enabled, suppress rule 1.1 and 11.3*/
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_1_1:"Allow void pointer to be typecasted to a bool value, requred for consistancy with other SS/DD APIs")
#pragma diag(suppress:misra_rule_11_3:"Allow void pointer to be typecasted to a bool value, requred for consistancy with other SS/DD APIs")
#endif /* _MISRA_RULES */


    /* Get the LibIO device ID */
    nLibioDeviceId = (uint32_t)get_default_io_device();
    
    /* Based on LibIO Device Id return the STDIO service device.
       Pass the device information so that we get back the device information
       in further call from LIBIO 
    */
    return((int32_t)_adi_stdio_GetStdioDevice(nLibioDeviceId));

/* IF MISRA diagnostics enabled, enable back rule 1.1 and 11.3*/
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

}

/* Close a File opened over UART Stdio device */
static int32_t  Stdio_Uart_Close (
    int32_t             nFileDescriptor
)
{
    /* Return success */
    return (0);
}

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_3 :"Allow void pointer to be typecasted to a bool value, requred for consistancy with other SS/DD APIs")
#endif /* _MISRA_RULES */


/* Write to file opened over UART Stdio device */
static int32_t  Stdio_Uart_Write (
    int32_t             nFileDescriptor,
    uint8_t             *psDataBuffer,
    int32_t             nBufferSize
)
{
    /* We get pointer to STDIO device information as we have returned that when the
       file is opened */
    ADI_STDIO_DEVICE_INFO *pStdioDevice = (ADI_STDIO_DEVICE_INFO *) nFileDescriptor;

    /* Pass this to STDIO common write function */
    return(_adi_stdio_Write(pStdioDevice, 
                            psDataBuffer, 
                            nBufferSize));
}



/* Read from a file opened over UART Stdio device */
static int32_t  Stdio_Uart_Read (
    int32_t             nFileDescriptor,
    uint8_t             *psDataBuffer,
    int32_t             nBufferSize
)
{
    /* We get pointer to STDIO device information as we have returned that when the
       file is opened */
    ADI_STDIO_DEVICE_INFO *pStdioDevice = (ADI_STDIO_DEVICE_INFO *) nFileDescriptor;

    /* Pass this to STDIO common write function */
    return(_adi_stdio_Read(pStdioDevice, 
                           psDataBuffer, 
                           nBufferSize));
}

/* IF MISRA diagnostics enabled, enable back 11.3*/
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */


/* Seek to new position in a file opened over UART Stdio device */
static long  Stdio_Uart_Seek (
    int32_t             nFileDescriptor,
    long                nOffset,
    int32_t             nWhence
)
{
    /* Seek is not supported for this device return failure*/
    return (-1);
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


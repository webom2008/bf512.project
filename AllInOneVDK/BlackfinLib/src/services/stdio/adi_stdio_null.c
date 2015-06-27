/* $Revision: 3492 $
 * $Date: 2010-10-07 09:34:20 -0400 (Thu, 07 Oct 2010) $ 
**************************************************************************** 
Copyright (c), 2008-2009 - Analog Devices Inc. All Rights Reserved. 
This software is proprietary & confidential to Analog Devices, Inc. 
and its licensors. 
****************************************************************************

Title: STDIO Service 

Description: This file contains the STDIO service NULL device specific 
             implementation
*****************************************************************************/

/*=============  I N C L U D E S   =============*/
#include <string.h>
#include <services/stdio/adi_stdio.h>
#include "adi_stdio_common.h"


/* IF MISRA diagnostics enabled, suppress rules for this file as necessary*/
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_5_1 :"Most identifiers require more than 31 characters for readability. Rule disabled to avoid truncation")
#pragma diag(suppress:misra_rule_5_6 :"Most functions share same parameter name(s) for code readability")
#pragma diag(suppress:misra_rule_5_7 :"Most functions, structures and unions share same identifier name(s) for code readability")
#pragma diag(suppress:misra_rule_6_3:"Allow basic data types for LIBIO APIs")
#pragma diag(suppress:misra_rule_20_9:"Stdio class devices leverage input/output library <stdio.h> for POSIX functions")
#pragma diag(suppress:misra_rule_16_7:"Even though some pointer parameters are not used for this device they may be used by other device implementation. So for API consistency it is required them to keep them as non constant pointers ")
#endif /* _MISRA_RULES */

/*
**  Local Functions declaration 
*/

/* Function open NULL Device */
static ADI_STDIO_RESULT adi_stdio_Null_DeviceOpen(
        ADI_STDIO_DEVICE_INFO           *pStdioDevice,
        uint32_t                         nPhysicalDevNum,
        ADI_DEV_MANAGER_HANDLE     const hDeviceMgr,
        ADI_DMA_MANAGER_HANDLE     const hDmaMgr,
        ADI_DCB_HANDLE             const hDcbMgr
        );

/* Function to Close NULL Device */
static ADI_STDIO_RESULT adi_stdio_Null_DeviceClose(
    ADI_STDIO_DEVICE_INFO         *const pStdioDevice
);


/* Function to configure NULL Device */
static ADI_STDIO_RESULT adi_stdio_Null_DeviceControl(
    ADI_STDIO_DEVICE_INFO         *const pStdioDevice,
    uint32_t                             nCommandID,
    void                          *const pValue
);

/*
** Local functions declaration required for registering with LIBIO
*/

/* Open a File over NULL Stdio device */
static int32_t  Stdio_Null_Init (
    struct DevEntry     *device
);


/* Open a File over NULL Stdio device */
static int32_t  Stdio_Null_Open (
    const char_t        *szFilePath,
    int32_t             nFileMode
);

/* Close a File opened over NULL Stdio device */
static int32_t  Stdio_Null_Close (
    int32_t             nFileDescriptor
);

/* Write to file opened over NULL Stdio device */
static int32_t  Stdio_Null_Write (
    int32_t             nFileDescriptor,
    uint8_t            *psDataBuffer,
    int32_t             nBufferSize
);

/* Read from a file opened over NULL Stdio device */
static int32_t  Stdio_Null_Read (
    int32_t             nFileDescriptor,
    uint8_t            *psDataBuffer,
    int32_t             nBufferSize
);

/* Seek to new position in a file opened over NULL Stdio device */
static long  Stdio_Null_Seek (
    int32_t             nFileDescriptor,
    long                nOffset,
    int32_t             nWhence
);


/*=============  D A T A  =============*/

/* 
   Structure containing NULL Device specific implementation
   function pointers
*/    
ADI_STDIO_DEVICE_ENTRY adi_stdio_NULL_DeviceEntry = {

    /* LIBIO Device Entry device for NULL */
    {
        /* Initialize DeviceID to an invalid value, it will be set by the STDIO service */
        -1,
        /* Set Pointer to private data to NULL */
        NULL,
        /* Iinit function pointer*/
        &Stdio_Null_Init,
        /* File Open function pointer */
        &Stdio_Null_Open,
        /* File Close function pointer */
        &Stdio_Null_Close,
        /* File Write function pointer */
        &Stdio_Null_Write,
        /* File Read function pointer */
        &Stdio_Null_Read,
        /* File Seek function pointer */
        &Stdio_Null_Seek,
        /* stdinfd - don't claim file access */
        dev_not_claimed,
        /* stdoutfd - don't claim file access */
        dev_not_claimed,
        /* stderrfd - don't claim file access */
        dev_not_claimed,
    },
    /* Function pointers required by the STDIO service  */

    /* Pointer to Device Open function */
    &adi_stdio_Null_DeviceOpen,
    /* Pointer to Device Close function */
    &adi_stdio_Null_DeviceClose,
    /* Pointer to Device Control function */
    &adi_stdio_Null_DeviceControl
};


/*=============  C O D E  =============*/

/*
**  Function Definition section
*/


/**********************************************************
  Function: adi_stdio_RegisterNULL

      Registers the NULL device with STDIO service
      
      This function is primarily helpful to avoid NULL device related
      files being linked when not used.

  Parameters:
      None

  Returns:
      None
**********************************************************/
void adi_stdio_RegisterNULL(void)
{
    /* Pass the device entry information to the STDIO service */
    _adi_stdio_pDeviceEntries[ADI_STDIO_DEVICE_TYPE_NULL] = &adi_stdio_NULL_DeviceEntry;
}

/**********************************************************
  Function: adi_stdio_Null_DeviceOpen

      Opens the NULL device for STDIO

  Parameters:
      pStdioDevice      - Pointer to the STDIO device
      nPhysicalDevNum     - Physical device number to open
      hDeviceMgr          - Handle to Device Manager
      hDmaMgr             - Handle to DMA Manager
      hDcbMgr             - Handle to DCB Manager

  Returns:
     ADI_STDIO_RESULT_SUCCESS
**********************************************************/
static ADI_STDIO_RESULT adi_stdio_Null_DeviceOpen(
        ADI_STDIO_DEVICE_INFO           *pStdioDevice,
        uint32_t                         nPhysicalDevNum,
        ADI_DEV_MANAGER_HANDLE     const hDeviceMgr,
        ADI_DMA_MANAGER_HANDLE     const hDmaMgr,
        ADI_DCB_HANDLE             const hDcbMgr
)
{
/* IF MISRA diagnostics enabled, suppress 11.3*/
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_3:"Allow typecasting between integer and pointer")
#endif /* _MISRA_RULES */

    /* Just indicate that NULL device is initialized */
    pStdioDevice->hPhysicalDevice = (ADI_DEV_DEVICE_HANDLE) 1U;

/* IF MISRA diagnostics enabled, renable 11.3*/
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

    return (ADI_STDIO_RESULT_SUCCESS);
}

/**********************************************************
  Function: adi_stdio_Null_DeviceClose

      Closes NULL device

  Parameters:
     pStdioDevice      - pointer to the STDIO device

  Returns:
     ADI_STDIO_RESULT_SUCCESS
**********************************************************/
static ADI_STDIO_RESULT adi_stdio_Null_DeviceClose(
    ADI_STDIO_DEVICE_INFO          *const pStdioDevice
)
{
    return (ADI_STDIO_RESULT_SUCCESS);
}



/**********************************************************
  Function: adi_stdio_Null_DeviceControl

      Sets/Gets the configuration parameters for given physical device

  Parameters:
      pStdioDevice        - Pointer to the STDIO device
      nCommandID          - Command ID which needs to be set/get.
      pValue              - Pointer that holds the value to be set/get

  Returns:
     ADI_STDIO_RESULT_SUCCESS
         - Successfully configured the given device.
**********************************************************/

static ADI_STDIO_RESULT adi_stdio_Null_DeviceControl(
    ADI_STDIO_DEVICE_INFO        *const pStdioDevice,
    uint32_t                            nCommandID,
    void                         *const pValue    
)
{
    return (ADI_STDIO_RESULT_SUCCESS);
}


/* Open a File over NULL Stdio device */
static int32_t  Stdio_Null_Init (
    struct DevEntry     *device
)
{
    return 0;
}

/* IF MISRA diagnostics enabled, suppress rule 1.1 and 11.3*/
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_1_1:"Allow void pointer to be typecasted to a bool value, requred for consistancy with other SS/DD APIs")
#pragma diag(suppress:misra_rule_11_3:"Allow void pointer to be typecasted to a bool value, requred for consistancy with other SS/DD APIs")
#endif /* _MISRA_RULES */


/* Open a File over NULL Stdio device */
static int32_t  Stdio_Null_Open (
    const char_t        *szFilePath,
    int32_t             nFileMode
)
{
    uint32_t nLibioDeviceId;

    /* Get the LibIO device ID */
    nLibioDeviceId = (uint32_t) get_default_io_device();
    
    /* Based on LibIO Device Id return the STDIO service device.
       Pass the device information so that we get back the device information
       in further call from LIBIO 
    */
    return((int32_t)_adi_stdio_GetStdioDevice(nLibioDeviceId));
}

/* IF MISRA diagnostics enabled, enable back rule 1.1 and 11.3*/
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */


/* Close a File opened over NULL Stdio device */
static int32_t  Stdio_Null_Close (
    int32_t             nFileDescriptor
)
{
    /* Return success */
    return (0);
}

/* Write to file opened over NULL Stdio device */
static int32_t  Stdio_Null_Write (
    int32_t             nFileDescriptor,
    uint8_t             *psDataBuffer,
    int32_t             nBufferSize
)
{
    /* return back the number of bytes requested to be written */
    return(nBufferSize);
}

/* Read from a file opened over NULL Stdio device */
static int32_t  Stdio_Null_Read (
    int32_t             nFileDescriptor,
    uint8_t             *psDataBuffer,
    int32_t             nBufferSize
)
{
    /* return back the number of bytes requested to be read */
    return(nBufferSize);
}

/* Seek to new position in a file opened over NULL Stdio device */
static long  Stdio_Null_Seek (
    int32_t             nFileDescriptor,
    long                nOffset,
    int32_t             nWhence
)
{
    /* Seek is not supported for this device return failure*/
    return (-1);
}

/* IF MISRA diagnostics enabled, enable back rule 5.1, 5.6, 5.7, 6.3, 16.7 and 20.9 */
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */


/*
**
** EOF:
**
*/


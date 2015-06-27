/* $Revision: 3492 $
 * $Date: 2010-10-07 09:34:20 -0400 (Thu, 07 Oct 2010) $ 
**************************************************************************** 
Copyright (c), 2008-2009 - Analog Devices Inc. All Rights Reserved. 
This software is proprietary & confidential to Analog Devices, Inc. 
and its licensors. 
****************************************************************************

Title: STDIO Service 

Description: This file contains the STDIO service JTAG device specific 
             implementation

*****************************************************************************/

/*=============  I N C L U D E S   =============*/
#include <string.h>
#include <services/stdio/adi_stdio.h>
#include <device.h>
#include "adi_stdio_common.h"


/* IF MISRA diagnostics enabled, suppress 16.7*/
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_16_7:"Even though some pointer parameters are not used for this device they may be used by other device implementation. So for API consistency it is required them to keep them as non constant pointers ")
#endif /* _MISRA_RULES */


/*=============  C O D E  =============*/


/*
**  Local Functions declaration 
*/

/* Function open JTAG Device */
static ADI_STDIO_RESULT adi_stdio_Jtag_DeviceOpen(
        ADI_STDIO_DEVICE_INFO         *pStdioDevice,
        uint32_t                       nPhysicalDevNum,
        ADI_DEV_MANAGER_HANDLE         hDeviceMgr,
        ADI_DMA_MANAGER_HANDLE         hDmaMgr,
        ADI_DCB_HANDLE                 hDcbMgr
);

/* Function to Close JTAG Device */
static ADI_STDIO_RESULT adi_stdio_Jtag_DeviceClose(
    ADI_STDIO_DEVICE_INFO             *pStdioDevice
);


/* Function to configure JTAG Device */
static ADI_STDIO_RESULT adi_stdio_Jtag_DeviceControl(
    ADI_STDIO_DEVICE_INFO       *const pStdioDevice,
    uint32_t                           nCommandID,
    void                        *const pValue
);


/*=============  D A T A  =============*/
/* 
   Structure containing JTAG Device specific implementation
   function pointers and data
*/    
ADI_STDIO_DEVICE_ENTRY adi_stdio_JTAG_DeviceEntry = {

    /* As it is preregistered with LIBIO, it is not required 
       to supply the function pointers */
    {
        /* Initialize DeviceID to an invalid value, it will be set by the STDIO service */
        -1,
        /* Set Pointer to private data to JTAG */
        NULL,
        /* Iinit function pointer*/
        NULL,
        /* File Open function pointer */
        NULL,
        /* File Close function pointer */
        NULL,
        /* File Write function pointer */
        NULL,
        /* File Read function pointer */
        NULL,
        /* File Seek function pointer */
        NULL,
        /* stdinfd - don't claim file access */
        dev_not_claimed,
        /* stdoutfd - don't claim file access */
        dev_not_claimed,
        /* stderrfd - don't claim file access */
        dev_not_claimed
    },
    /* Function pointers required by the STDIO service  */

    /* Pointer to Device Open function */
    &adi_stdio_Jtag_DeviceOpen,
    /* Pointer to Device Close function */
    &adi_stdio_Jtag_DeviceClose,
    /* Pointer to Device Config function */
    &adi_stdio_Jtag_DeviceControl
};


/*
**  Function Definition section
*/


/**********************************************************
  Function: adi_stdio_RegisterJTAG

      Registers the JTAG device with STDIO service
      
      This function is primarily used to avoid JTAG related
      files being linked when not used.

  Parameters:
      None

  Returns:
      None
**********************************************************/
void adi_stdio_RegisterJTAG(void)
{
    /* Pass the device entry information to the STDIO service */
    _adi_stdio_pDeviceEntries[ADI_STDIO_DEVICE_TYPE_JTAG] = &adi_stdio_JTAG_DeviceEntry;
}

/**********************************************************
  Function: adi_stdio_Jtag_DeviceOpen

      Opens the JTAG device for STDIO

  Parameters:
      pStdioDevice        - Pointer to the STDIO device
      nPhysicalDevNum     - Physical device number to open
      hDeviceMgr          - Handle to Device Manager
      hDmaMgr             - Handle to DMA Manager
      hDcbMgr             - Handle to DCB Manager

  Returns:
     ADI_STDIO_RESULT_SUCCESS
**********************************************************/
static ADI_STDIO_RESULT adi_stdio_Jtag_DeviceOpen(
        ADI_STDIO_DEVICE_INFO           *pStdioDevice,
        uint32_t                         nPhysicalDevNum,
        ADI_DEV_MANAGER_HANDLE           hDeviceMgr,
        ADI_DMA_MANAGER_HANDLE           hDmaMgr,
        ADI_DCB_HANDLE                   hDcbMgr
)
{
/* IF MISRA diagnostics enabled, suppress 11.3*/
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_3:"Allow typecasting between integer and pointer")
#endif /* _MISRA_RULES */

    /* Just indicate that JTAG is initialized */
    pStdioDevice->hPhysicalDevice = (ADI_DEV_DEVICE_HANDLE) 1U;

/* IF MISRA diagnostics enabled, renable 11.3*/
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

    return (ADI_STDIO_RESULT_SUCCESS);
}

/**********************************************************
  Function: adi_stdio_Jtag_DeviceClose

      Closes JTAG device

  Parameters:
     pStdioDevice      - pointer to the STDIO device

  Returns:
     ADI_STDIO_RESULT_SUCCESS
**********************************************************/
static ADI_STDIO_RESULT adi_stdio_Jtag_DeviceClose(
    ADI_STDIO_DEVICE_INFO          *pStdioDevice
)
{
    return (ADI_STDIO_RESULT_SUCCESS);
}


/*************************************************************
  Function: adi_stdio_Jtag_DeviceControl

      Sets/Gets the configuration parameters for given physical device

  Parameters:
      pStdioDevice      - Pointer to the STDIO device
      nCommandID          - Command ID which needs to be set/get.
      pValue              - Pointer that holds the value to be set/get

  Returns:
     ADI_STDIO_RESULT_SUCCESS
         - Successfully configured the given device.
****************************************************************/
static ADI_STDIO_RESULT adi_stdio_Jtag_DeviceControl(
    ADI_STDIO_DEVICE_INFO          *const pStdioDevice,
    uint32_t                              nCommandID,
    void                           *const pValue
)
{
    return (ADI_STDIO_RESULT_SUCCESS);
}

/* IF MISRA diagnostics enabled, enable back rule 16.7*/
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */


/*
**
** EOF:
**
*/


/* $Revision: 3682 $
 * $Date: 2010-11-01 16:56:32 -0400 (Mon, 01 Nov 2010) $ 
******************************************************************************
Copyright (c), 2009 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title: ADI STDIO

Description: This is the common header file for all the STDIO device specific
             implementations.

*****************************************************************************/

#ifndef __ADI_STDIO_COMMON_H__
#define __ADI_STDIO_COMMON_H__

/*=============  I N C L U D E S   =============*/
#include <services/stdio/adi_stdio.h>
#include <osal/adi_osal.h>

/* disable misra diagnostics as necessary */
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rules_all :"Disable MISRA checking for LibIO")
#endif /* _MISRA_RULES */

/* PRIMIO includes */
#include <device.h>
#include <device_int.h>

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */


/*==============  D E F I N E S  ===============*/

/*
** Maximum number of STDIO devices; 4 devices are reserved for
** FILIO, PRIMIO, PCI_IO and CSIMIO (refer device.h)
*/
#define ADI_STDIO_MAX_DEVICES             (MAXDEV - 4U)

/*
** Default device types
*/

/* NULL Device */
#define ADI_STDIO_DEVICE_TYPE_NULL        0U
/* JTAG Device */
#define ADI_STDIO_DEVICE_TYPE_JTAG        1U




/*
** Hex Equivalent for backspace entry
*/
#define ADI_STDIO_CHAR_BACKSPACE           0x08U
/*
** Hex Equivalent for new line entry character
*/
#define ADI_STDIO_CHAR_NEW_LINE            0x0AU
/*
** Hex Equivalent for line feed character
*/
#define ADI_STDIO_CHAR_LINE_FEED           0x0DU


/*
**   Macros to set, clear and check flag bits in nFlags member 
**   of ADI_STDIO_DEVICE_INFO structure
**  
*/

/* Bit positions in nFlags */
#define DEVICE_IN_USE_FLAG_BIT             (0U)
#define REGISTERED_WITH_LIBIO_FLAG_BIT       (1U)
#define UNIX_MODE_FLAG_BIT                   (2U)
#define CHAR_ECHO_FLAG_BIT                   (3U)


/* Macros to set, clear and check flag bits */
#define SET_FLAG_BIT(x, y)    ((x) = ((x) |  (1UL << (y))))
#define CLEAR_FLAG_BIT(x, y)  ((x) = ((x) & ~(1UL << (y))))
#define CHECK_FLAG_BIT(x, y)  (((x) &  (1UL << (y))) >> (y))

/* Macros to set, clear and test Device in use flag */
#define SET_DEVICE_IN_USE_FLAG(x)          \
        SET_FLAG_BIT((x), DEVICE_IN_USE_FLAG_BIT)
#define CLEAR_DEVICE_IN_USE_FLAG(x)        \
        CLEAR_FLAG_BIT((x), DEVICE_IN_USE_FLAG_BIT)
#define IS_DEVICE_IN_USE_FLAG_SET(x)       \
        CHECK_FLAG_BIT((x),DEVICE_IN_USE_FLAG_BIT)


/* Macros to set, clear and test Registered with LIBIO flag */
#define SET_REGISTERED_WITH_LIBIO_FLAG(x)    \
        SET_FLAG_BIT((x), REGISTERED_WITH_LIBIO_FLAG_BIT)
#define CLEAR_REGISTERED_WITH_LIBIO_FLAG(x)  \
        CLEAR_FLAG_BIT((x), REGISTERED_WITH_LIBIO_FLAG_BIT)
#define IS_REGISTERED_WITH_LIBIO_FLAG_SET(x) \
        CHECK_FLAG_BIT((x),REGISTERED_WITH_LIBIO_FLAG_BIT)

/* Macros to set, clear and test Unix mode flag */
#define SET_UNIX_MODE_FLAG(x)                \
        SET_FLAG_BIT((x), UNIX_MODE_FLAG_BIT)
#define CLEAR_UNIX_MODE_FLAG(x)              \
        CLEAR_FLAG_BIT((x), UNIX_MODE_FLAG_BIT)
#define IS_UNIX_MODE_FLAG_SET(x)             \
        CHECK_FLAG_BIT((x), UNIX_MODE_FLAG_BIT)


/* Macros to set, clear and test charecter echo flag */
#define SET_CHAR_ECHO_FLAG(x)                \
        SET_FLAG_BIT((x), CHAR_ECHO_FLAG_BIT)
#define CLEAR_CHAR_ECHO_FLAG(x)              \
        CLEAR_FLAG_BIT((x), CHAR_ECHO_FLAG_BIT)
#define IS_CHAR_ECHO_FLAG_SET(x)             \
        CHECK_FLAG_BIT((x), CHAR_ECHO_FLAG_BIT)


#if defined(_LANGUAGE_C)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* C-specific definitions  */


/*=============  D A T A    T Y P E S   =============*/

/*
**
**  Data structures
**
*/

typedef struct adiStdioDeviceInfo ADI_STDIO_DEVICE_INFO;



/*
**  Function: adi_stdio_RegisterNULL
**
**      Registers the NULL device with STDIO service
**
**  Parameters:
**      None
**
**  Returns:
**      None
*/
void adi_stdio_RegisterNULL (
    void
);


/*
**  Function: adi_stdio_RegisterJTAG
**
**      Registers the JTAG device with STDIO service
**
**  Parameters:
**      None
**
**  Returns:
**      None
*/
void adi_stdio_RegisterJTAG (
    void
);


/*
**  Function: ADI_STDIO_DEVICE_OPEN_FUNCTION
**
**      Opens the given physical device for STDIO
**
**  Parameters:
**      pStdioDevice      - Pointer to the STDIO device
**      nPhysicalDevNum     - Physical device number to open
**      hDeviceMgr          - Handle to Device Manager
**      hDmaMgr             - Handle to DMA Manager
**      hDcbMgr             - Handle to DCB Manager
**      pDeviceConfig       - Device specific configuration
**
**  Returns:
**     ADI_STDIO_RESULT_SUCCESS
**         - Successfully opened the given device.
**     ADI_STDIO_RESULT_INVALID_POINTER
**         - Pointer to the STDIO device is not valid
**     ADI_STDIO_RESULT_DEVICE_FAILED
**         - Failed to open the given physical device
*/
typedef ADI_STDIO_RESULT (*ADI_STDIO_DEVICE_OPEN_FUNCTION)(
    ADI_STDIO_DEVICE_INFO           *pStdioDevice,
    uint32_t                           nPhysicalDevNum,
    ADI_DEV_MANAGER_HANDLE             const hDeviceMgr,
    ADI_DMA_MANAGER_HANDLE             const hDmaMgr,
    ADI_DCB_HANDLE                     const hDcbMgr
);


/*
**  Function: ADI_STDIO_DEVICE_CLOSE_FUNCTION
**
**      Closes the given physical device
**
**  Parameters:
**     pStdioDevice      - pointer to the STDIO device
**
**  Returns:
**     ADI_STDIO_RESULT_SUCCESS
**         - Successfully closed the given device.
**     ADI_STDIO_RESULT_INVALID_POINTER
**         - Pointer to the STDIO device is not valid
**     ADI_STDIO_RESULT_DEVICE_FAILED
**         - Failed to close the given physical device      
*/
typedef ADI_STDIO_RESULT (*ADI_STDIO_DEVICE_CLOSE_FUNCTION)(
    ADI_STDIO_DEVICE_INFO          *const pStdioDevice
);


/*
**  Function: ADI_STDIO_DEVICE_CONTROL_FUNCTION
**
**      Sets/Gets the configuration parameters for given physical device
**
**  Parameters:
**      pStdioDevice      - Pointer to the STDIO device
**      nCommandID          - Command ID which needs to be set/get.
**      pValue              - Pointer that holds the value to be set/get
**
**  Returns:
**     ADI_STDIO_RESULT_SUCCESS
**         - Successfully closed the given device.
**     ADI_STDIO_RESULT_INVALID_POINTER
**         - Pointer to the STDIO device is not valid
**     ADI_STDIO_RESULT_DEVICE_FAILED
**         - Device failed to exicute the given command
**     ADI_STDIO_RESULT_COMMAND_NOT_SUPPORTED
**         - Given command is not supported by the device
*/
typedef ADI_STDIO_RESULT (*ADI_STDIO_DEVICE_CONTROL_FUNCTION)(
    ADI_STDIO_DEVICE_INFO    *const  pStdioDevice,
    uint32_t                         nCommandID,
    void                     *const  pValue
);


/*
** Structure to store the STDIO device entry 
*/
typedef struct adiStdioDeviceEntry
{
    /* pointer to the Libio device entry */
    DevEntry                            oLibioDevEntry;
    /* pointer to function which opens the pysical device */
    ADI_STDIO_DEVICE_OPEN_FUNCTION      pfDeviceOpen;
    /* pointer to function which closes the pysical device */
    ADI_STDIO_DEVICE_CLOSE_FUNCTION     pfDeviceClose;
    /* pointer to function which configures the pysical device */
    ADI_STDIO_DEVICE_CONTROL_FUNCTION   pfDeviceControl;
} ADI_STDIO_DEVICE_ENTRY;



/*
** Structure to store the STDIO device information
*/
struct adiStdioDeviceInfo
{
    /* Physical Device information object */
    ADI_STDIO_DEVICE_ENTRY      *pDeviceEntry;
    /* LIBIO device ID, should be unique for each entry */
    uint32_t                     nLibioDeviceId;
    /* Pointer to private data */
    void                        *pPrivateData;                     
    /* Flags to indicate various status */
    uint32_t                     nFlags;
    /* Handle to the physical device driver */
    ADI_DEV_DEVICE_HANDLE        hPhysicalDevice;
    /* Handle to the input semaphore */
    ADI_OSAL_SEM_HANDLE          hInputSemaphore;
    /* Handle to the output semaphore */
    ADI_OSAL_SEM_HANDLE          hOutputSemaphore;
};


/*
**    Function: _adi_stdio_GetStdioDevice
**
**        Returns the STDIO device based on LibIO device number.
**
**    Parameters:
**        nLibIODeviceID  - LIBIO Device ID.
**
**    Returns:
**        pStdioDevice  - Pointer to STDIO device
*/
ADI_STDIO_DEVICE_INFO* _adi_stdio_GetStdioDevice(
    uint32_t                 nLibIODeviceID
);

/*
**    Function: _adi_stdio_Callback
**
**        Generic Stdio callback
**
**    Parameters:
**        hStdioDevice  - Handle to STDIO device
**        nEvent          - Callback Event code
**        pArgument       - Callback Aurgument
**
**    Returns:
**        None
*/
void _adi_stdio_Callback(
    void  *const hStdioDevice,
    u32          nEvent,
    void  *const pArgument
);


/*
**    Function: _adi_stdio_Write
**
**        Writes to file (stdout/stderr) opened over selected device
**
**    Parameters:
**        pStdioDevice      - Pointer to STDIO device info
**        psDataBuffer        - pointer to buffer containing output elements
**        nElementCount       - number elements in output data buffer
**
**    Returns: (return codes as per libio standard)
**        -1 - Failed to write to a device
**        zero - File (descriptor) is closed
**        non-zero (number of bytes transmitted) - Success
**
*/
int32_t _adi_stdio_Write (  ADI_STDIO_DEVICE_INFO *pStdioDevice,
    uint8_t                *psDataBuffer,
    int32_t                 nElementCount
);


/*
**    Function: _adi_stdio_Read
**
**        Reads from file (stdin) opened over selected device
**
**    Parameters:
**        pStdioDevice      - Pointer to STDIO device information
**        psDataBuffer        - pointer to buffer to hold input elements
**        nElementCount       - number elements to receive
**
**    Returns: (return codes as per libio standard)
**        -1 - Failed to read from a device
**        zero - File (descriptor) is closed
**        non-zero (number of bytes transmitted) - Success
*/
int32_t _adi_stdio_Read (
    ADI_STDIO_DEVICE_INFO   *pStdioDevice,
    uint8_t                 *psDataBuffer,
    int32_t                  nElementCount
);



/* 
**  Function: _adi_stdio_MemAlloc
**
**      Call Memory Manager or malloc function to allocate memory
**
**  Parameters:
**      nSize     - Required memory block size
**      ppData    - Pointer to a location where the address of the 
**                  allocated memory block will be written
**                  
**  Returns:
**      ADI_STDIO_RESULT_SUCCESS
**                - Successfully allocated memory block of given size
**      ADI_STDIO_RESULT_INSUFFICIENT_MEMORY
**                - Failed to allocate memory block of given size
*/
ADI_STDIO_RESULT _adi_stdio_MemAlloc (
    void     **ppData,
    uint32_t   nSize 
);


/*
**  Function: _adi_stdio_MemFree
**
**      Calls Memory Manager or free function to heap_free the given 
**      memory block
**
**  Parameters:
**      pData    - Pointer to the memory block which is to be freed
**                  
**  Returns:
**      None
*/
void _adi_stdio_MemFree (
    void *pData
);

/*=============  E X T E R N A L S  ============*/

/* 
   Array which contains the pointer to the device info for each stdio device type.
   The registration functions will fill the device info for registering the device
   with the stdio service.
*/
extern ADI_STDIO_DEVICE_ENTRY *_adi_stdio_pDeviceEntries[ADI_STDIO_DEVICE_TYPE_MAX];


/*=======  P U B L I C   P R O T O T Y P E S  ========*/
/*            (globally-scoped functions)             */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#else /* not _LANGUAGE_C */

/* Assembler-specific */

#endif /* not _LANGUAGE_C */


#endif /* __ADI_STDIO_COMMON_H__ */



/*
**
** EOF:
**
*/


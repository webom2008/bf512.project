/******************************************************************************

  Copyright (C), 2005-2014, CVTE.

 ******************************************************************************
  File Name     : AioLed.c
  Version       : Initial Draft
  Author        : qiuweibo
  Created       : 2015/6/20
  Last Modified :
  Description   : driver for aio led
  Function List :
  History       :
  1.Date        : 2015/6/20
    Author      : qiuweibo
    Modification: Created file

******************************************************************************/

#include "Config/includes.h"

/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

/* Device Driver Control Struct */
typedef struct {                                 /* LED device structure                    */
    /* Driver-Related Fields...                                                                 */
    u8                       DeviceNumber;    /* Holds SPI Device Number to use              */
    u8                       InUseFlag;          /* in use flag (in use when TRUE)              */
    void                    *pEnterCriticalArg;  /* critical region argument                    */
    ADI_DEV_MANAGER_HANDLE   ManagerHandle;      /* Device Manager Handle                       */
    ADI_DEV_DEVICE_HANDLE    DeviceHandle;       /* Device handle                               */
    ADI_DCB_HANDLE           DCBHandle;          /* callback handle                             */
    ADI_DCB_CALLBACK_FN      DMCallback;         /* Device manager's Callback function          */
    ADI_DEV_PDD_HANDLE	     Handle;             /* Handle to device (driver)               */
} LED_DEVICE_STRUCT;

/*----------------------------------------------*
 * internal variables                           *
 *----------------------------------------------*/
static LED_DEVICE_STRUCT LEDDevice[] = {
    { 
        0,                                      /* SPISSEL number                           */
        FALSE,                                  /* in use flag                              */
        NULL,                                   /* critical region argument                 */
        NULL,                                   /* Device manager handle                    */
        NULL,                                   /* Device handle                            */
        NULL,                                   /* callback handle                          */
        NULL,                                   /* Device manager Callback function         */
        NULL,                                   /* Handle to device (driver)            */
    },
};

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

static u32 adi_pdd_Open(                        // Open a device
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,          // device manager handle
    u32                     DeviceNumber,           // device number
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,           // device handle
    ADI_DEV_PDD_HANDLE      *pPDDHandle,            // pointer to PDD handle location
    ADI_DEV_DIRECTION       Direction,              // data direction
    void                    *pCriticalRegionArg,    // critical region imask storage location
    ADI_DMA_MANAGER_HANDLE  DMAHandle,              // handle to the DMA manager
    ADI_DCB_HANDLE          DCBHandle,              // callback handle
    ADI_DCB_CALLBACK_FN     DMCallback              // device manager callback function
);

static u32 adi_pdd_Close(       // Closes a device
    ADI_DEV_PDD_HANDLE PDDHandle    // PDD handle
);

static u32 adi_pdd_Read(        // Reads data or queues an inbound buffer to a device
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER *pBuffer         // pointer to buffer
);

static u32 adi_pdd_Write(       // Writes data or queues an outbound buffer to a device
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType, // buffer type
    ADI_DEV_BUFFER *pBuffer         // pointer to buffer
);

/* Sequentially read/writes data to a device*/
static u32 adi_pdd_SequentialIO(        /* Never called as UART doesnot support this function  */
    ADI_DEV_PDD_HANDLE      PDDHandle,
    ADI_DEV_BUFFER_TYPE     BufferType,
    ADI_DEV_BUFFER          *pBuffer
);

static u32 adi_pdd_Control(     // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE PDDHandle,   // PDD handle
    u32 Command,                    // command ID
    void *pArg                      // pointer to argument
);
/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/


/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/
ADI_DEV_PDD_ENTRY_POINT AIOLedEntryPoint =
{
    adi_pdd_Open,
    adi_pdd_Close,
    adi_pdd_Read,
    adi_pdd_Write,
    adi_pdd_Control,
    adi_pdd_SequentialIO
};


/*********************************************************************
*
*   Function:       adi_pdd_Open
*
*   Description:    Opens a UART for use
*
*********************************************************************/


static u32 adi_pdd_Open(                // Open a device
    ADI_DEV_MANAGER_HANDLE  ManagerHandle,          // device manager handle
    u32                     DeviceNumber,           // device number
    ADI_DEV_DEVICE_HANDLE   DeviceHandle,           // device handle
    ADI_DEV_PDD_HANDLE      *pPDDHandle,            // pointer to PDD handle location
    ADI_DEV_DIRECTION       Direction,              // data direction
    void                    *pEnterCriticalArg,     // enter critical region parameter
    ADI_DMA_MANAGER_HANDLE  DMAHandle,              // handle to the DMA manager
    ADI_DCB_HANDLE          DCBHandle,              // callback handle
    ADI_DCB_CALLBACK_FN     DMCallback              // client callback function
) 
{

    u32                    Result = ADI_DEV_RESULT_SUCCESS; /* Return value - assume we're going to be successful */
    LED_DEVICE_STRUCT     *pLEDDev;                         /* pointer to the device we're working on             */
    void                  *pExitCriticalArg;                /* exit critical region parameter                     */

    /* insure the device the client wants is available  */
    Result  = ADI_DEV_RESULT_DEVICE_IN_USE;

    pLEDDev = &LEDDevice[DeviceNumber];

    /* Protect this section of code - entering a critical region    */
    pExitCriticalArg = adi_int_EnterCriticalRegion(pEnterCriticalArg);

    /* Check the device usage status */
    if (pLEDDev->InUseFlag == FALSE) 
    {
        /* Device is not in use. Reserve the device for this client */
        pLEDDev->InUseFlag = TRUE;
        Result = ADI_DEV_RESULT_SUCCESS;
    }

    /* Exit the critical region */
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    if (Result == ADI_DEV_RESULT_SUCCESS) 
    {    
        pLEDDev->DeviceNumber = 0;
        /* Pointer to critical region */
        pLEDDev->pEnterCriticalArg  = pEnterCriticalArg;
        
        /* Pointer to Device manager handle */
        pLEDDev->ManagerHandle      = ManagerHandle;
        
        /* Pointer to Device handle */
        pLEDDev->DeviceHandle       = DeviceHandle;
        
        /* Pointer to Deffered Callback manager handle */
        pLEDDev->DCBHandle          = DCBHandle;
        
        /* Pointer to Device manager callback function  */
        pLEDDev->DMCallback         = DMCallback;
        
        /* save the physical device handle in the client supplied location */
        *pPDDHandle = (ADI_DEV_PDD_HANDLE *)pLEDDev;
        
        gpio_init(GPIOF, GPIO_Pin_6, Function_GPIO, GPIO_Mode_Out);
    }
    
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Close
*
*   Description:    Closes down a UART
*
*********************************************************************/


static u32 adi_pdd_Close(       // Closes a device
    ADI_DEV_PDD_HANDLE PDDHandle            // PDD handle
)
{
    u32                     Result = ADI_DEV_RESULT_SUCCESS; /* Return value - assume we're going to be successful   */
    LED_DEVICE_STRUCT     *pLEDDev;                         /* pointer to the device we're working on */

    /* avoid casts */
    pLEDDev = (LED_DEVICE_STRUCT *) PDDHandle;

    /* mark this device as closed */
    pLEDDev->InUseFlag = FALSE;

    /* return */
    return(Result);
}

/*********************************************************************
*
*   Function:       adi_pdd_Read
*
*   Description:    Provides buffers to store data when data is received
*                   from the UART
*
*********************************************************************/


static u32 adi_pdd_Read(            // Reads data or queues an inbound buffer to a device
    ADI_DEV_PDD_HANDLE  PDDHandle,      // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType,     // buffer type
    ADI_DEV_BUFFER      *pBuffer        // pointer to buffer
)
{
    /* not supported  */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************
*
*   Function:       adi_pdd_Write
*
*   Description:    Provides buffers containing data to be transmitted
*                   out through the UART
*
*********************************************************************/


static u32 adi_pdd_Write(       // Writes data or queues an outbound buffer to a device
    ADI_DEV_PDD_HANDLE  PDDHandle,          // PDD handle
    ADI_DEV_BUFFER_TYPE BufferType,         // buffer type
    ADI_DEV_BUFFER      *pBuffer            // pointer to buffer
)
{
    /* not supported  */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************
*
*   Function:       adi_pdd_SequentialIO
*
*   Description:    LED does not support this function
*
*********************************************************************/
static u32 adi_pdd_SequentialIO(        /* Never called as UART doesnot support this function  */
    ADI_DEV_PDD_HANDLE      PDDHandle,
    ADI_DEV_BUFFER_TYPE     BufferType,
    ADI_DEV_BUFFER          *pBuffer
){

    /* this function is not supported by  */
    return(ADI_DEV_RESULT_NOT_SUPPORTED);
}

/*********************************************************************
*
*   Function:       adi_pdd_Control
*
*   Description:    Configures the UART
*
*********************************************************************/


static u32 adi_pdd_Control(     // Sets or senses a device specific parameter
    ADI_DEV_PDD_HANDLE  PDDHandle,          // PDD handle
    u32                 Command,            // command ID
    void                *pVal              // command specific value
)
{
    LED_DEVICE_STRUCT          *pLEDDev;    /*pointer to the device we're working on*/
    u32                         Result;     /* return value                         */
    u32                         u32Value;
    
    /* assume we're going to be successful */
    Result  = ADI_DEV_RESULT_SUCCESS;
            
    /* avoid casts */
    pLEDDev     = (LED_DEVICE_STRUCT *)PDDHandle;
    
    /* assign 32 bit values for the pVal argument */
    u32Value    = (u32)pVal;
    
    // CASEOF (Command ID)
    switch (Command) 
    {

        case (AIO_LED_CMD_ON):
		    gpio_clear(GPIOF, GPIO_Pin_6);
            break;

        case (AIO_LED_CMD_OFF):
		    gpio_set(GPIOF, GPIO_Pin_6);
            break;

        default:
            // we don't understand this command
            Result = ADI_DEV_RESULT_NOT_SUPPORTED;
            break;
    }

    // return
    return(Result);
}


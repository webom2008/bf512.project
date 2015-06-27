/******************************************************************************

  Copyright (C), 2005-2014, CVTE.

 ******************************************************************************
  File Name     : StdioUart.c
  Version       : Initial Draft
  Author        : qiuweibo
  Created       : 2015/6/27
  Last Modified :
  Description   : Stdio Uart
  Function List :
  History       :
  1.Date        : 2015/6/27
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

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

static ADI_STDIO_DEVICE_HANDLE  stdioDevice;
static ADI_DEV_DEVICE_HANDLE    stdioUartHandle; 

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/

// UART configuration table
ADI_DEV_CMD_VALUE_PAIR UARTConfiguration[] = {
	{ ADI_DEV_CMD_SET_DATAFLOW_METHOD, 	(void *)ADI_DEV_MODE_CHAINED },
	{ ADI_UART_CMD_SET_DATA_BITS, 		(void *)8			         },
	{ ADI_UART_CMD_ENABLE_PARITY, 		(void *)FALSE	             },
	{ ADI_UART_CMD_SET_STOP_BITS, 		(void *)1			         },
	{ ADI_UART_CMD_SET_BAUD_RATE, 		(void *)230400		         },
	{ ADI_DEV_CMD_END, 		            NULL         		         },
};
    
static void StdioUartConfig(ADI_DEV_DEVICE_HANDLE UartHandle)
{
    u32 Result;
    
    // configure the UART driver, parameters are
    //      UART device driver handle
    //      command ID stating that we're passing in a command table
    //      address of the command table
    Result = adi_dev_Control(UartHandle,
                             ADI_DEV_CMD_TABLE, 
                             UARTConfiguration);
	while(ADI_DEV_RESULT_SUCCESS != Result)
	{
    }
}

void StdioUartInit(ADI_DCB_HANDLE dcbServer)
{
    u32 Result;
    
    adi_dma_SetMapping(ADI_DMA_PMAP_UART1_RX, ADI_DMA_DMA10); 		 
    adi_dma_SetMapping(ADI_DMA_PMAP_UART1_TX, ADI_DMA_DMA11);
    
    /*
    * Initialize STDIO Service
    */
    Result = adi_stdio_Init (
    			  adi_dev_ManagerHandle,	/* Device Manager Handle */
    			  adi_dma_ManagerHandle,    /* DMA Manager Handle */
    			  dcbServer,		        /* DCB Manager Handle */
    			  &stdioDevice);	        /* Pointer to UART STDIO Device handle */
    while (ADI_STDIO_RESULT_SUCCESS != Result)
    {
    }
    
    /*
    * Register the UART Device Type
    */
    adi_stdio_RegisterUART();

    /*
    * Open UART1 
    */
    Result = adi_stdio_OpenDevice(
        ADI_STDIO_DEVICE_TYPE_UART,   /* UART Device Type */
        1,                            /* Physical Device number */
        &stdioDevice);                /* Pointer to the handle */
    while (ADI_STDIO_RESULT_SUCCESS != Result)
    {
    }

    /*
    * Disable character echo on the UART Device
    */
    Result = adi_stdio_ControlDevice(
              stdioDevice,
              ADI_STDIO_COMMAND_ENABLE_CHAR_ECHO, 
              (void *) false);            /* false to disable */
    while (ADI_STDIO_RESULT_SUCCESS != Result)
    {
    }
    
    /*
    ** CR + LF (carriage return and line feed) is used as EOL
    */
    Result = adi_stdio_ControlDevice(
        stdioDevice,
        ADI_STDIO_COMMAND_ENABLE_UNIX_MODE, 
        (void *) false);      /* false to disable */

    /*
    ** UART related configuration commands
    */
    
    /* Set Parity type (valid argument is one of the prity type from ADI_STDIO_PARITY_TYPE) */
    Result = adi_stdio_ControlDevice(
        stdioDevice,                         /* UART Device Type */
        ADI_STDIO_COMMAND_SET_UART_PARITY_TYPE, /* Command ID */
        (void *)ADI_STDIO_PARITY_TYPE_NONE
        );
    while (ADI_STDIO_RESULT_SUCCESS != Result)
    {
    }

#if 0 /* not need to autobaud mode */
    /* Set the charecter to be used in autobaud mode, valid values are any charecter */
    Result = adi_stdio_ControlDevice(
          stdioDevice,                         /* UART Device Type */
          ADI_STDIO_COMMAND_SET_UART_AUTO_BAUD_CHAR, /* Command ID */
          (void *)'C'
         );
     
    /* Check if command is successfully executed */
    while (ADI_STDIO_RESULT_SUCCESS != Result)
    {
    // Failed to execute STDIO control command, take appropriate action here
    }

    /* Enable auto baud detection (no command specific arguments) */
    Result = adi_stdio_ControlDevice(
          stdioDevice,                         /* UART Device Type */
          ADI_STDIO_COMMAND_ENABLE_AUTO_BAUD, /* Command ID */
          (void *) NULL
         );
     
    /* Check if command is successfully executed */
    while (ADI_STDIO_RESULT_SUCCESS != Result)
    {
    // Failed to execute STDIO control command, take appropriate action here
    } 
#endif

    /* Set data word length, valid word lengths are 5, 6, 7, 8 */
//    Result = adi_stdio_ControlDevice(
//        stdioDevice,                         /* UART Device Type */
//        ADI_STDIO_COMMAND_SET_UART_WORD_LENGTH, /* Command ID */
//        reinterpret_cast<void *>(8u)
//        );
//    while (ADI_STDIO_RESULT_SUCCESS != Result)
//    {
//    }

    /* Set number of stop bits, valid values are 1 or 2 */
    Result = adi_stdio_ControlDevice(
        stdioDevice,                         /* UART Device Type */
        ADI_STDIO_COMMAND_SET_UART_NUM_STOP_BITS, /* Command ID */
        (void *)(1u)
        );
    while (ADI_STDIO_RESULT_SUCCESS != Result)
    {
    }

    /*
    ** Set STDIO Baud Rate  (115200 ~= 3200 floats/sec)
    */
    Result = adi_stdio_ControlDevice(
        stdioDevice,
        ADI_STDIO_COMMAND_SET_UART_BAUD_RATE, 
        (void *)(230400u)
        );

    Result = adi_stdio_RedirectStream (
        stdioDevice,
        ADI_STDIO_STREAM_ALL_CONSOLE_IO);

#if 0 /* It is not recommended to set any physical device configuration values directly from the application */
    /* Get the physical device handle */
    Result = adi_stdio_ControlDevice(
          stdioDevice,                         /* UART Device Type */
          ADI_STDIO_COMMAND_GET_DEVICE_HANDLE, /* Command ID */
          (void *) &stdioUartHandle
         );
     
    /* Check if command is successfully executed */
    while (ADI_STDIO_RESULT_SUCCESS != Result)
    {
    // Failed to execute STDIO control command, take appropriate action here
    }
    StdioUartConfig(stdioUartHandle);
#endif

#if 0 /* not need to terminate */
    /*
    ** Disable STDOUT stream alone
    */
    eResult = adi_stdio_DisableStream (
             ADI_STDIO_STREAM_STDOUT /* Stream type to be disabled */
    );
     
    /* Check if required stream disabled successfully */
    while (ADI_STDIO_RESULT_SUCCESS != eResult)
    {
    // Failed to disable STDOUT stream, take appropriate action here
    }

    /*
    ** Terminate STDIO Service
    */
    eResult = adi_stdio_Terminate();
     
    /* Check if STDIO service is terminated successfully */
    while (ADI_STDIO_RESULT_SUCCESS != eResult)
    {
    // Failed to terminate STDIO service take appropriate action here
    }

#endif
}


/******************************************************************************

  Copyright (C), 2005-2014, CVTE.

 ******************************************************************************
  File Name     : AIODrivers.c
  Version       : Initial Draft
  Author        : qiuweibo
  Created       : 2015/6/20
  Last Modified :
  Description   : AIO drivers
  Function List :
  History       :
  1.Date        : 2015/6/20
    Author      : qiuweibo
    Modification: Created file

******************************************************************************/
#define _AIO_DRI_C_
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
extern ADI_STDIO_DEVICE_HANDLE hSTDIOUART;

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/
static void StdioUartConfig(void)
{
    u32 eResult;
    ADI_DEV_DEVICE_HANDLE getDevHandle;
    
    /* Enable/Disable Unix mode line breaks. true = Enable, false = Disable */
    eResult = adi_stdio_ControlDevice(
          hSTDIOUART,                         /* UART Device Type */
          ADI_STDIO_COMMAND_ENABLE_UNIX_MODE, /* Command ID */
          (void *) true
         );
     
    /* Check if command is successfully executed */
    while (ADI_STDIO_RESULT_SUCCESS != eResult)
    {
    // Failed to execute STDIO control command, take appropriate action here
    }
    
    /* Enalbe/Disable Charecter echo. true = Enable, false = Disable */
    eResult = adi_stdio_ControlDevice(
          hSTDIOUART,                         /* UART Device Type */
          ADI_STDIO_COMMAND_ENABLE_CHAR_ECHO, /* Command ID */
          (void *) true
         );
     
    /* Check if command is successfully executed */
    while (ADI_STDIO_RESULT_SUCCESS != eResult)
    {
    // Failed to execute STDIO control command, take appropriate action here
    }
#if 0 /* It is not recommended to set any physical device configuration values directly from the application */
    /* Get the physical device handle */
    eResult = adi_stdio_ControlDevice(
          hSTDIOUART,                         /* UART Device Type */
          ADI_STDIO_COMMAND_GET_DEVICE_HANDLE, /* Command ID */
          (void *) &getDevHandle
         );
     
    /* Check if command is successfully executed */
    while (ADI_STDIO_RESULT_SUCCESS != eResult)
    {
    // Failed to execute STDIO control command, take appropriate action here
    }
#endif

    /*
    ** UART related configuration commands
    */
    
    /* Set Parity type (valid argument is one of the prity type from ADI_STDIO_PARITY_TYPE) */
    eResult = adi_stdio_ControlDevice(
          hSTDIOUART,                         /* UART Device Type */
          ADI_STDIO_COMMAND_SET_UART_PARITY_TYPE, /* Command ID */
          (void *) ADI_STDIO_PARITY_TYPE_NONE
         );
     
    /* Check if command is successfully executed */
    while (ADI_STDIO_RESULT_SUCCESS != eResult)
    {
    // Failed to execute STDIO control command, take appropriate action here
    }

    /* Set data word length, valid word lengths are 5, 6, 7, 8 */
    eResult = adi_stdio_ControlDevice(
          hSTDIOUART,                         /* UART Device Type */
          ADI_STDIO_COMMAND_SET_UART_WORD_LENGTH, /* Command ID */
          (void *)8
         );
     
    /* Check if command is successfully executed */
    while (ADI_STDIO_RESULT_SUCCESS != eResult)
    {
    // Failed to execute STDIO control command, take appropriate action here
    }

    /* Set number of stop bits, valid values are 1 or 2 */
    eResult = adi_stdio_ControlDevice(
          hSTDIOUART,                         /* UART Device Type */
          ADI_STDIO_COMMAND_SET_UART_NUM_STOP_BITS, /* Command ID */
          (void *)1
         );
     
    /* Check if command is successfully executed */
    while (ADI_STDIO_RESULT_SUCCESS != eResult)
    {
    // Failed to execute STDIO control command, take appropriate action here
    }

#if 0 /* not need to autobaud mode */
    /* Set the charecter to be used in autobaud mode, valid values are any charecter */
    eResult = adi_stdio_ControlDevice(
          hSTDIOUART,                         /* UART Device Type */
          ADI_STDIO_COMMAND_SET_UART_AUTO_BAUD_CHAR, /* Command ID */
          (void *)'C'
         );
     
    /* Check if command is successfully executed */
    while (ADI_STDIO_RESULT_SUCCESS != eResult)
    {
    // Failed to execute STDIO control command, take appropriate action here
    }

    /* Enable auto baud detection (no command specific arguments) */
    eResult = adi_stdio_ControlDevice(
          hSTDIOUART,                         /* UART Device Type */
          ADI_STDIO_COMMAND_ENABLE_AUTO_BAUD, /* Command ID */
          (void *) NULL
         );
     
    /* Check if command is successfully executed */
    while (ADI_STDIO_RESULT_SUCCESS != eResult)
    {
    // Failed to execute STDIO control command, take appropriate action here
    }
#endif 
    /* Set the UART baud rate */
    eResult = adi_stdio_ControlDevice(
          hSTDIOUART,                         /* UART Device Type */
          ADI_STDIO_COMMAND_SET_UART_BAUD_RATE, /* Command ID */
          (void *) 230400
         );
     
    /* Check if command is successfully executed */
    while (ADI_STDIO_RESULT_SUCCESS != eResult)
    {
    // Failed to execute STDIO control command, take appropriate action here
    }

}

static void InitStdioDevice(void)
{
    u32 eResult;

    /*
    ** Register the UART Device Type
    */
    adi_stdio_RegisterUART();
    
    /*
    ** Open the UART Device Type
    */
    eResult = adi_stdio_OpenDevice(
             ADI_STDIO_DEVICE_TYPE_UART, /* UART Device Type */
             1,                          /* Physical Device number */ 
             &hSTDIOUART                 /* Pointer to the handle */
           );
     
    /* Check if STDIO Device opened successfully */
    while (ADI_STDIO_RESULT_SUCCESS != eResult)
    {
    // Failed to open STDIO device type, take appropriate action here
    }

    StdioUartConfig();
    
    /* Redirect all STDIO streams to UART */
    eResult = adi_stdio_RedirectStream (
            hSTDIOUART,                /* UART Device Handle */
            ADI_STDIO_STREAM_ALL_CONSOLE_IO /* Stream Type        */
               );
     
    /* Check if streams are successfully redirected */
    while (ADI_STDIO_RESULT_SUCCESS != eResult)
    {
    // Failed to redirect STDIO streams, take appropriate action here
    }
    
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

void InitDeviceDrivers(void)
{
    //SysUartInit();  //Simple test for UART
    InitStdioDevice();
    
}



void ExitDeviceDrivers(void)
{
    
}


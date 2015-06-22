/******************************************************************************

  Copyright (C), 2005-2014, CVTE.

 ******************************************************************************
  File Name     : main.cpp
  Version       : Initial Draft
  Author        : qiuweibo
  Created       : 2015/6/22
  Last Modified :
  Description   :  System Services & Drivers, VDK program to blink the LEDs on 
				   the BF512 Data Acquistion Module.  An ADSP_BRODIE processor.
  Function List :
  History       :
  1.Date        : 2015/6/22
    Author      : qiuweibo
    Modification: Created file

******************************************************************************/
#pragma diag(push)
#pragma diag(suppress: 177,401,451,826,831,1462)

#include "VDK.h"

#pragma diag(pop)

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
#define  DCB_QUEUE   (ADI_DCB_QUEUE_SIZE)

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal variables                           *
 *----------------------------------------------*/
static u8					dcbQueueMemData [DCB_QUEUE];
static u32					num_dcb_entries;
static ADI_DCB_HANDLE   	dcbServer;

static const void           *myDmaID = (void *) 0x444D4131u;

static ADI_STDIO_DEVICE_HANDLE  stdioDevice;
static ADI_DEV_DEVICE_HANDLE    stdioUartHandle; 

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/

 
 /*  I hate  void *  parameters, but I'm stuck with them.  So, ... */
 
#define  TMR_PERIOD  reinterpret_cast<void *>(48u)  /* 1 MHz = 1 micro second */
#define  TMR_WIDTH   reinterpret_cast<void *>(3u)   /* Hdwre Ref 10-43, min ~10 cnts */
#define  TMR_MODE    reinterpret_cast<void *>(1u)    /* 1 = PWM */
#define  TMR_CLOCK   reinterpret_cast<void *>(false) /* SCLK. (true=PWM_CLK) */
#define  TMR_METHOD  reinterpret_cast<void *>(true)  /* count to end of PERIOD */
#define  TMR_OUTPUT  reinterpret_cast<void *>(false) /* Enable TMRx pin */
#define  TMR_IRQ     reinterpret_cast<void *>(false) /* Disable interrrupts */
#define  TMR_ENABLE  reinterpret_cast<void *>(true)  /* Start the timer */
#define  SENTINEL    reinterpret_cast<void *>(0u)    /* Mark End of Table */

static ADI_TMR_GP_CMD_VALUE_PAIR  tmr1_init[] = {	/* SCLK = 296 MHz */
	{ ADI_TMR_GP_CMD_SET_PERIOD,			  TMR_PERIOD }, 
	{ ADI_TMR_GP_CMD_SET_WIDTH, 			  TMR_WIDTH }, 
	{ ADI_TMR_GP_CMD_SET_TIMER_MODE,		  TMR_MODE },
	{ ADI_TMR_GP_CMD_SET_CLOCK_SELECT,		  TMR_CLOCK },	
	{ ADI_TMR_GP_CMD_SET_COUNT_METHOD,		  TMR_METHOD }, 
	{ ADI_TMR_GP_CMD_SET_OUTPUT_PAD_DISABLE,  TMR_OUTPUT },
	{ ADI_TMR_GP_CMD_SET_INTERRUPT_ENABLE,	  TMR_IRQ },  
	{ ADI_TMR_GP_CMD_ENABLE_TIMER,			  TMR_ENABLE },
	{ ADI_TMR_GP_CMD_END,					  SENTINEL } 
};

// UART configuration table
ADI_DEV_CMD_VALUE_PAIR UARTConfiguration[] = {
	{ ADI_DEV_CMD_SET_DATAFLOW_METHOD, 	(void *)ADI_DEV_MODE_CHAINED },
	{ ADI_UART_CMD_SET_DATA_BITS, 		(void *)8			         },
	{ ADI_UART_CMD_ENABLE_PARITY, 		(void *)FALSE	             },
	{ ADI_UART_CMD_SET_STOP_BITS, 		(void *)1			         },
	{ ADI_UART_CMD_SET_BAUD_RATE, 		(void *)230400		         },
	{ ADI_DEV_CMD_END, 		            NULL         		         },
};
    
static void myDmaHandler (void*pHandle, u32 u32Arg, void*pArg)
{
	if (u32Arg == reinterpret_cast<u32>(myDmaID)) return;
	if (pArg == NULL) return;
	if (pHandle == NULL) return;
	/*	  This function probably wants to post an event or message to	   */
	/* indicate that the scan is complete and the buffer may be processed. */
}

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

static void StdioUartInit(void)
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
        reinterpret_cast<void *>(1u)
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
        reinterpret_cast<void *>(230400u)
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


int  main (void)
{
    u32 Result;

    VDK::Initialize ();

    Result = adi_ssl_Init();

    Result = adi_tmr_Open (ADI_TMR_GP_TIMER_1) ;
    Result = adi_tmr_GPControl (ADI_TMR_GP_TIMER_1, 
    							 ADI_TMR_GP_CMD_TABLE, tmr1_init);
    Result = adi_dcb_Open (0u, &dcbQueueMemData, DCB_QUEUE,
    						&num_dcb_entries, &dcbServer);
    Result = adi_dma_Open (adi_dma_ManagerHandle,
    						ADI_DMA_DMA0,
    						&myDmaID,
    						&adi_dma_ManagerHandle,
    						ADI_DMA_MODE_SINGLE,
    						NULL,
    						&myDmaHandler);

    StdioUartInit();

    VDK::Run ();
    return (0);
}


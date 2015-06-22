/******************************************************************************

  Copyright (C), 2005-2014, CVTE.

 ******************************************************************************
  File Name     : SystemUart.c
  Version       : Initial Draft
  Author        : qiuweibo
  Created       : 2015/6/20
  Last Modified :
  Description   : system uart
  Function List :
  History       :
  1.Date        : 2015/6/20
    Author      : qiuweibo
    Modification: Created file

******************************************************************************/
#define _DRI_SYS_UART_
#include "Config/includes.h"
#include <drivers/uart/adi_uart.h>		// UART device driver includes

/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/
extern ADI_DEV_MANAGER_HANDLE DevMgrHandle;  // handle to device manager

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

#define NUM_BUFFERS 	     (2)	// number of buffers to use

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal variables                           *
 *----------------------------------------------*/

static u8                    Data[NUM_BUFFERS];   // storage for data sent and received
static ADI_DEV_1D_BUFFER     Buffer[NUM_BUFFERS]; // the actual buffers
static ADI_DEV_DEVICE_HANDLE UARTHandle;          // handle to the UART driver
/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/
static void CallbackFunction(void *ClientHandle, u32 Event, void *pArg); // callback function

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/

void SysUartInit(void)
{
	u32 i;      // counter
	u32 Result; // return code
	
    // UART configuration table
	ADI_DEV_CMD_VALUE_PAIR UARTConfiguration[] = {
		{ ADI_DEV_CMD_SET_DATAFLOW_METHOD, 	(void *)ADI_DEV_MODE_CHAINED },
		{ ADI_UART_CMD_SET_DATA_BITS, 		(void *)8			         },
		{ ADI_UART_CMD_ENABLE_PARITY, 		(void *)FALSE	             },
		{ ADI_UART_CMD_SET_STOP_BITS, 		(void *)1			         },
		{ ADI_UART_CMD_SET_BAUD_RATE, 		(void *)230400		         },
		{ ADI_DEV_CMD_END, 		            NULL         		         },
	};
    
	// open the UART driver, parameters are
	//      device manager handle
	//      entry point of device driver to open
	//      the device number (0th UART)
	//      client handle (callback will always be passed this value)
	//      location where UART device driver handle will be stored
	//      direction the device is to be opened for
	//      DMA manager handle (NULL cause we're not using DMA)
	//      DCB handle (NULL cause we want live callbacks)
	//      address of callback function
	Result = adi_dev_Open(DevMgrHandle, 
	                      &ADIUARTEntryPoint, 
	                      1, 
	                      (void *)0x12345678, 
	                      &UARTHandle, 
	                      ADI_DEV_DIRECTION_BIDIRECTIONAL, 
	                      NULL, 
	                      NULL, 
	                      CallbackFunction);
	while(ADI_DEV_RESULT_SUCCESS != Result){}
    
    // configure the UART driver, parameters are
    //      UART device driver handle
    //      command ID stating that we're passing in a command table
    //      address of the command table
    Result = adi_dev_Control(UARTHandle,
                             ADI_DEV_CMD_TABLE, 
                             UARTConfiguration);
	while(ADI_DEV_RESULT_SUCCESS != Result){}
    
	// create 1D buffers for the UART to process
	for (i = 0; i < NUM_BUFFERS; i++) {
		Buffer[i].Data              = &Data[i];     // address of the data
		Buffer[i].ElementCount      = 1;            // one element in the buffer
		Buffer[i].ElementWidth      = 1;            // element is one byte wide
		Buffer[i].CallbackParameter = &Buffer[i];   // pass the buffer address to the callback
		Buffer[i].pNext             = &Buffer[i+1]; // point to the next buffer in the chain
		Buffer[i].pAdditionalInfo   = (void *)0;    // (for this example, 0 means an inbound buffer
	}
	Buffer[NUM_BUFFERS - 1].pNext   = NULL;         // terminate the chain of buffers
	
	// give the buffers to the driver to fill with data, parameters are
	//      UART device driver handle
	//      1D buffers
	//      Address of first buffer
	Result = adi_dev_Read(UARTHandle,
	                      ADI_DEV_1D, 
	                      (ADI_DEV_BUFFER *)Buffer);
	while(ADI_DEV_RESULT_SUCCESS != Result){}
    
	// enable data flow, parameters are
	//      UART device driver handle
	//      SET_DATAFLOW command ID
	//      TRUE (enable dataflow)
	Result = adi_dev_Control(UARTHandle, 
	                         ADI_DEV_CMD_SET_DATAFLOW, 
	                         (void *)TRUE);
	while(ADI_DEV_RESULT_SUCCESS != Result){}
}


/*********************************************************************

	Function:		CallbackFunction

	Description:	Invoked when a buffer has been processed.  When an 
	                inbound buffer has been processed, meaning a character
	                has been received, the buffer is sent back to the terminal
	                by calling the adi_dev_Write() function.  When an outbound
	                buffer has been processed, meaning the character has been
	                sent to the terminal, the buffer is then ready for new data
	                and is sent to the adi_dev_Read() function.  
	                
	                Parameters:
	                    ClientHandle - value that was passed in adi_dev_Open()
	                    Event        - event that occured (see adi_dev.h).  In 
	                                   this example, event will always be a 
	                                   buffer processed event.
	           	        pArg         - event specific parameter (see adi_dev.h)
	           	                       In this example, pArg will always contain
	           	                       the buffer's address.

*********************************************************************/

static void CallbackFunction(void *ClientHandle, u32 Event, void *pArg) {
    
	ADI_DEV_1D_BUFFER *pBuffer;    // pointer to the buffer
	u32               Result;      // return code

	// point to the buffer
	pBuffer = (ADI_DEV_1D_BUFFER *)pArg;
	
	// CASEOF (event type)
	switch (Event) {
    	
    	// CASE (buffer processed)
    	case ADI_DEV_EVENT_BUFFER_PROCESSED:
    	
    	    // IF (an inbound buffer)
    	    if (pBuffer->pAdditionalInfo == 0) {
        	    
        	    // tag it as an outbound buffer and the only buffer in the chain
        	    pBuffer->pAdditionalInfo = (void *)1;
        	    pBuffer->pNext = NULL;
        	    
        	    // send the buffer out, parameters are
        	    //      UART device driver handle
        	    //      1D buffer
        	    //      buffer address
       	    	Result = adi_dev_Write(UARTHandle, 
       	    	                       ADI_DEV_1D, 
       	    	                       (ADI_DEV_BUFFER *)pBuffer);
       	    	
       	    // ELSE
	        } else {
    	        
    	        // tag it as an inbound buffer and the only buffer in the chain
        	    pBuffer->pAdditionalInfo = (void *)0;
        	    pBuffer->pNext = NULL;
        	    
        	    // fill the buffer with more data, parameters are
        	    //      UART device driver handle
        	    //      1D buffer
        	    //      buffer address
       	    	Result = adi_dev_Read(UARTHandle, 
       	    	                      ADI_DEV_1D, 
       	    	                      (ADI_DEV_BUFFER *)pBuffer);
       	    	
       	    // ENDIF
   	        }
   	        break;
   	        
   	// ENDCASE
	}
	
	// return
}
	   
	


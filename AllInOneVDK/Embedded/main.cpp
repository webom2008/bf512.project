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


/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/

 
 /*  I hate  void *  parameters, but I'm stuck with them.  So, ... */
 
#define  TMR_PERIOD  reinterpret_cast<void *>(48u)   /* 1 MHz = 1 micro second */
#define  TMR_WIDTH   reinterpret_cast<void *>(3u)    /* Hdwre Ref 10-43, min ~10 cnts */
#define  TMR_MODE    reinterpret_cast<void *>(1u)    /* 1 = PWM */
#define  TMR_CLOCK   reinterpret_cast<void *>(false) /* SCLK. (true=PWM_CLK) */
#define  TMR_METHOD  reinterpret_cast<void *>(true)  /* count to end of PERIOD */
#define  TMR_OUTPUT  reinterpret_cast<void *>(false) /* Enable TMRx pin */
#define  TMR_IRQ     reinterpret_cast<void *>(false) /* Disable interrrupts */
#define  TMR_ENABLE  reinterpret_cast<void *>(true)  /* Start the timer */
#define  SENTINEL    reinterpret_cast<void *>(0u)    /* Mark End of Table */

static ADI_TMR_GP_CMD_VALUE_PAIR  tmr1_init[] = {	 /* SCLK = 296 MHz */
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

static void myDmaHandler (void*pHandle, u32 u32Arg, void*pArg)
{
	if (u32Arg == reinterpret_cast<u32>(myDmaID)) return;
	if (pArg == NULL) return;
	if (pHandle == NULL) return;
	/*	  This function probably wants to post an event or message to	   */
	/* indicate that the scan is complete and the buffer may be processed. */
}


int  main (void)
{
    u32 Result;

    VDK::Initialize ();

    Result = adi_ssl_Init();

    // demo for timer
    Result = adi_tmr_Open (ADI_TMR_GP_TIMER_1) ;
    Result = adi_tmr_GPControl (ADI_TMR_GP_TIMER_1, 
    							 ADI_TMR_GP_CMD_TABLE, tmr1_init);
    
    Result = adi_dcb_Open (0u, &dcbQueueMemData, DCB_QUEUE,
    						&num_dcb_entries, &dcbServer);

    // demo for dma
    Result = adi_dma_Open (adi_dma_ManagerHandle,
    						ADI_DMA_DMA0,
    						&myDmaID,
    						&adi_dma_ManagerHandle,
    						ADI_DMA_MODE_SINGLE,
    						NULL,
    						&myDmaHandler);

    StdioUartInit(dcbServer);

    VDK::Run ();
    return (0);
}


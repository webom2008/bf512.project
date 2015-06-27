/* $Revision: 3578 $
 * $Date: 2010-10-19 09:36:04 -0400 (Tue, 19 Oct 2010) $
******************************************************************************
Copyright (c), 2008-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Operating System Abstraction Layer - OSAL for uCOS - Timing functions

Description:
              This file contains the Timing API's for the uCOS implementation of the OSAL

NOTE:
              All typedef with upper case (like INT8U,INT16U,INT32U) are uCOS
              specific. These typedefs are typically used for the variable
              which are passed as arguments to the OS function OR used for
              storing return value.

SUPRESSED MISRA RULES ARE:

        See adi_osal_ucos.c for the list of the MISRA Rules that have been suppressed for the uCOS OSAL.


*****************************************************************************/

/*=============  I N C L U D E S   =============*/

/*
 ALL MISRA rules are suppressed  for uCOS include file since ucos_ii.h is not MISRA compatible
*/
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rules_all)
#endif /*  _MISRA_RULES */
#include "ucos_ii.h"
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /*  _MISRA_RULES */

#include <osal/adi_osal.h>
#include "osal_ucos.h"
#include "../include/osal_common.h"



#pragma diag(push)
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exits (mostly for handling parameter error checking) increases the code readability and therefore maintenance")
/* Rule-5.1 indicates that all identifiers shall not rely on more than 31 characters of significance */
#pragma diag(suppress:misra_rule_5_1:"prefixes added in front of identifiers quickly increases their size. In order to keep the code self explanatory, and since ADI tools are the main targeted tools, this restriction is not really a necessity")
/* Rule-11.3 indicates that typecast of a integer value into pointer is invalid */
#pragma diag(suppress:misra_rule_11_3 : "typecasting is necessary everytimes a predefine value is written to a return pointer during error conditions")
/* Rule-16.2 indicates that  Functions shall not call themselves,either directly or indirectly */
#pragma diag(suppress:misra_rule_16_2 : "Since the OSAL is reentrant by nature (several thread could call the API) the compiler MISRA checker mistakes sometimes the reentrancy for recurrence")
/* Rule-2.1 indicates that Assembly language shall be encapsulated and isolated */
#pragma diag(suppress:misra_rule_2_1 : "In this case we use macros to isolate an assembly function, for readability reasons, it's been applied to the whole file and not around each macro call")


/*=============  D E F I N E S  =============*/

#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")




/*=============  D A T A  =============*/




/*=============  C O D E  =============*/




/*****************************************************************************

  Function: adi_osal_TickPeriodInMicroSec


  This function returns duration of a tick period in microsecond.

  Parameters:
     pnTickPeriod - pointer to a location to write the tick period in micro
     second(return value).

  Returns:
      ADI_OSAL_SUCCESS - Successfully returned the duration of the tick period
      in micro second.

  Notes:
      This function  helps to convert  time units to system ticks which is
      needed by the pend API's of message-Q,semaphore,mutex,event  and to
      put the task in "sleep" mode.

                                                        No. Microsecond in one second
      Duration of the tick period (in micro second) =  -------------------------------
                                                        No of ticks in one second.
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_TickPeriodInMicroSec(uint32_t *pnTickPeriod)
{
    *pnTickPeriod = _adi_osal_gnTickPeriod;
    return(ADI_OSAL_SUCCESS);
}



/*****************************************************************************
  Function: adi_osal_TimeTick


  This is a service to process a clock tick. This indicates to the OS
  that a tick period is completed.

  Parameters:
      None

  Returns:
      None

*****************************************************************************/

void adi_osal_TimeTick(void)
{
    PAUSE_PROFILING();
    OSTimeTick();
    RESUME_PROFILING();

    return;
}



/*****************************************************************************
  Function: adi_osal_GetCurrentTick


  This service returns the current value of the continuously incrementing
  timer tick counter. The counter increments once for every timer interrupt.

  Parameters:
     pnTicks  - pointer to a location to write the current value of tick counter.

  Returns:
     ADI_OSAL_SUCCESS - Successfully returned tick counter value  since
                       " OSTimeGet()" always  return the tick counter.

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_GetCurrentTick(uint32_t *pnTicks )
{
    PAUSE_PROFILING();
    *pnTicks = OSTimeGet();
    RESUME_PROFILING();

    return(ADI_OSAL_SUCCESS);
}



#pragma diag(pop)  /* global file supressions */

/*
**
** EOF: 
**
*/

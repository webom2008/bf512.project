/* $Revision: 3916 $
 * $Date: 2010-12-06 11:39:55 -0500 (Mon, 06 Dec 2010) $
******************************************************************************
Copyright (c), 2008-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Operating System Abstraction Layer - OSAL for uCOS - Critical section functions

Description:
              This file contains the critical section & scheduler locking API's for the uCOS
              implementation of the OSAL

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
#include <builtins.h>
#include "osal_ucos.h"

#pragma diag(push)
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exits (mostly for handling parameter error checking) increases the code readability and therefore maintenance")
/* Rule-2.1 indicates that Assembly language shall be encapsulated and isolated */
#pragma diag(suppress:misra_rule_2_1 : "In this case we use macros to isolate an assembly function, for readability reasons, it's been applied to the whole file and not around each macro call")


/*=============  D E F I N E S  =============*/

#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=internal")
#pragma file_attr(  "prefersMemNum=30")



/*=============  D A T A  =============*/



/*=============  C O D E  =============*/




/*****************************************************************************

  Function: adi_osal_IsSchedulerActive


  This function can be used determine whether scheduler is running.

  Parameters:
     None

  Returns:
     true if the scheduler is running, and false if not..
 

*****************************************************************************/

bool adi_osal_IsSchedulerActive(void)
{
    return(OSRunning == OS_TRUE);
}



/*****************************************************************************
  Function: adi_osal_SchedulerLock

  This function prevents rescheduling until adi_osal_SchedulerUnlock is called.
  The thread that calls this function continue execution even if higher
  priority threads are ready to run

  Parameters:
      None

  Return Values:
      None

*****************************************************************************/

void   adi_osal_SchedulerLock( void )
{
    PAUSE_PROFILING();
    OSSchedLock();
    RESUME_PROFILING();

#ifdef OSAL_DEBUG
    _adi_osal_gnSchedulerLockCnt++;
#endif /* OSAL_DEBUG */
    return;
}



/*****************************************************************************
  Function: adi_osal_SchedulerUnlock

  This service re-enables task scheduling.

  Parameters:
      None

  Return Values:
      None

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_SchedulerUnlock( void )
{
#ifdef OSAL_DEBUG

    if (_adi_osal_gnSchedulerLockCnt == 0u)
    {
        return (ADI_OSAL_FAILED);       /* if the Unlock function is called before the lock, return an error */
    }
    _adi_osal_gnSchedulerLockCnt--;             /* it must be done before unlocking */
#endif /* OSAL_DEBUG */

    PAUSE_PROFILING();
    OSSchedUnlock();                    /* uCOS OSSchedUnlock function takes care of nesting itself */
    RESUME_PROFILING();

    return (ADI_OSAL_SUCCESS);
}



/*****************************************************************************
  Function: adi_osal_EnterCriticalRegion

  Disables interrupts for the execution of a critical region of code.

  Parameters:
    none

  Return Values:
    ADI_OSAL_SUCCESS - If thread scheduling was enabled successfully
    ADI_OSAL_FAILED  - If the function call does not match a call to
                       adi_osal_SchedulerLock

*****************************************************************************/

void adi_osal_EnterCriticalRegion( void )
{
    
    /* accessing the global count variable needs to be protected from thread switches
       so cli() is called at the very begining. The value of the interrupts state
       is only saved the first time. */
    uint32_t istate = cli();

    if (0==_adi_osal_gnCriticalRegionNestingCnt)
    {
        /* only lock the interrupts the first time the function is called */
        _adi_osal_gnCriticalRegionState = istate;
    }

    _adi_osal_gnCriticalRegionNestingCnt++;


    return;
}



/*****************************************************************************
  Function: adi_osal_ExitCriticalRegion

  Re-enable the interrupts and restore the interrupt status.

  Parameters:
    none

  Return Values:
    none

*****************************************************************************/

void adi_osal_ExitCriticalRegion( void )
{
    _adi_osal_gnCriticalRegionNestingCnt--;

    /* when the last nesting level is reached, reenable the interrupts */
    if (_adi_osal_gnCriticalRegionNestingCnt <= 0)
    {
        _adi_osal_gnCriticalRegionNestingCnt = 0;
        sti(_adi_osal_gnCriticalRegionState);
    }

    return;
}

#pragma diag(pop)  /* global file supressions */

/*
**
** EOF: 
**
*/

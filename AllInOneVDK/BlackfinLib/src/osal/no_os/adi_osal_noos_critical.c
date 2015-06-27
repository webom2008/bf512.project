/* $Revision: 3916 $
 * $Date: 2010-12-06 11:39:55 -0500 (Mon, 06 Dec 2010) $ 
******************************************************************************
Copyright (c), 2008-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Operating System Abstraction Layer - OSAL for no-OS - Critical section functions

Description:
              This file contains the critical section & scheduler locking API's for the no-OS
              implementation of the OSAL

NOTE:
              Since no scheduler is present in the no-OS implementation, SchedulerLock and SchedulerUnlock
              functions should not be in theory present. However since they are typically used to protect
              global data from concurrent accesses, we decided to implement them used Critical sections. The
              difference with the critical sections is that they only block interrupts above a user-supplied
              threshold level. By default they do not do anything.



SUPRESSED MISRA RULES ARE:

        See adi_osal_noos.c for the list of the MISRA Rules that have been suppressed for the uCOS OSAL.


*****************************************************************************/



/*=============  I N C L U D E S   =============*/


#pragma diag(push)
/* Rule-16.2 indicates that  Functions shall not call themselves,either directly or indirectly */
/* cli/sti need could be reentrant */
#pragma diag(suppress:misra_rule_16_2 : "Since the OSAL is reentrant by nature (several thread could call the API) the compiler MISRA checker mistakes sometimes the reentrancy for recurrence")
#include <ccblkfn.h>                                                            /* for cli/sti */
#pragma diag(pop)

#include <osal/adi_osal.h>
#include "osal_noos.h"
#include "../include/osal_common.h"


#pragma diag(push)
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exits (mostly for handling parameter error checking) increases the code readability and therefore maintenance")
/* Rule-16.2 indicates that  Functions shall not call themselves,either directly or indirectly */
#pragma diag(suppress:misra_rule_16_2 : "Since the OSAL is reentrant by nature (several thread could call the API) the compiler MISRA checker mistakes sometimes the reentrancy for recurrence")


/*=============  D E F I N E S  =============*/

#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=internal")
#pragma file_attr(  "prefersMemNum=30")


/* since IMASK is a 16-bits value, 0xFFFFFFFF is invalid and can be used for testing */
#define INVALID_IMASK       (0xFFFFFFFFul)


/*=============  D A T A  =============*/

static uint32_t gnImaskState = INVALID_IMASK;


/*=============  C O D E  =============*/




/*****************************************************************************

  Function: adi_osal_IsSchedulerActive


  This function can be used determine whether scheduler is running. In the context
  of the no-OS, the scheduler is never active

  Parameters:
     None

  Returns:
     Normally true if the scheduler is running, and false if not. Here since 
     it is the no-OS version, it always returns False.

*****************************************************************************/

bool adi_osal_IsSchedulerActive(void)
{
    return(false);
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
void adi_osal_SchedulerLock( void )
{
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
        /* Replaced the built in cli() with inline assembly extension*/
        _adi_osal_gnCriticalRegionNestingCnt = 0;
        EXIT_CRITICAL_INTERNAL(_adi_osal_gnCriticalRegionState);

    }

    return;
}


#pragma diag(pop)  /* global file supressions */

/*
**
** EOF:
**
*/

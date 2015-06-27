/* $Revision: 5506 $
 * $Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $
******************************************************************************
Copyright (c), 2008-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Operating System Abstraction Layer - OSAL for non OS - Semaphore functions

Description:
              This file contains the Semaphore API's for the no OS implementation of the OSAL


SUPRESSED MISRA RULES ARE:

        See adi_osal_noos.c for the list of the MISRA Rules that have been suppressed for the no-OS OSAL.


*****************************************************************************/

/*=============  I N C L U D E S   =============*/

/* stdlib.h is included to find NULL definition */
#include <stdlib.h>
#include <osal/adi_osal.h>
#include "osal_noos.h"
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

/*=============  D E F I N E S  =============*/

#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")





/*=============  D A T A  =============*/



/*=============  C O D E  =============*/


/*****************************************************************************
  Function: adi_osal_SemCreate

  This service can be used to create a counting semaphore.

  Parameters:
      phSem         - Pointer to a location to write the returned semaphore ID
      nInitCount    - Initial value for the creation of a counting semaphore.
                      Semaphore will be created "unavailable" state if "nInitCount"
                      is equal to zero.
  Returns:
      ADI_OSAL_SUCCESS      - If semaphore is created successfully
      ADI_OSAL_FAILED       - If failed to create semaphore
      ADI_OSAL_BAD_COUNT    - The value specified in nInitCount is too large for uCOS
      ADI_OSAL_CALLER_ERROR - If the call is made from an invalid location (i.e an ISR)

  Note:
      phSem  set  to "ADI_OSAL_INVALID_SEM" if semaphore creation is failed.
 *****************************************************************************/

ADI_OSAL_STATUS adi_osal_SemCreate(ADI_OSAL_SEM_HANDLE *phSem, uint32_t nInitCount)
{
    ADI_OSAL_STATUS    eRetStatus;
    OSAL_SEMAPHORE    *pSem;

#ifdef OSAL_DEBUG
    if (_adi_osal_IsCurrentLevelISR())
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        return (ADI_OSAL_CALLER_ERROR);
    }
#endif /* OSAL_DEBUG */

    /* get memory for a semaphore object */
    if (ADI_OSAL_SUCCESS != _adi_osal_MemAlloc((void*) &pSem, sizeof(OSAL_SEMAPHORE)))
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        return (ADI_OSAL_MEM_ALLOC_FAILED);
    }

    pSem->nCount = nInitCount;
    pSem->bPendingS = false;
    pSem->bActiveS = true;

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the structure into an opaque pointer")
    *phSem = (ADI_OSAL_SEM_HANDLE) pSem;
#pragma diag(pop)

    return (ADI_OSAL_SUCCESS);
}


/*****************************************************************************
  Function: adi_osal_SemDestroy

  This function is used to delete a specified semaphore.

  Parameters:
      hSem      - The handle of the semaphore which need to be deleted

  Returns:
      ADI_OSAL_SUCCESS          - If semaphore is deleted successfully
      ADI_OSAL_FAILED           - If failed to delete semaphore
      ADI_OSAL_BAD_HANDLE       - If the specified semaphore handle is invalid
      ADI_OSAL_CALLER_ERROR     - If the call is made from an invalid location (i.e an ISR).
      ADI_OSAL_THREAD_PENDING   - A thread is pending on the specified semaphore

 *****************************************************************************/

ADI_OSAL_STATUS adi_osal_SemDestroy( ADI_OSAL_SEM_HANDLE const hSem )
{

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a pointer to a useful structure")
    OSAL_SEMAPHORE* pSemNative = (OSAL_SEMAPHORE*) hSem;
#pragma diag(pop)


#ifdef OSAL_DEBUG
    if (_adi_osal_IsCurrentLevelISR())
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ((hSem == ADI_OSAL_INVALID_SEM) ||
        (hSem == NULL ))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */

    if (pSemNative->bPendingS)
    {
        return ( ADI_OSAL_THREAD_PENDING );
    }

#ifdef OSAL_DEBUG
    /* even though the memory is getting freed, this flag is set so that future operations
       on that handle will fail */
    pSemNative->bActiveS = false;
#endif /* OSAL_DEBUG */


    _adi_osal_MemFree(pSemNative);

    return ( ADI_OSAL_SUCCESS );

}

/*****************************************************************************
  Function: adi_osal_SemPend

  This function is used to wait for access to a semaphore. If the specified semaphore
  is acquired, its count will be decremented.


  Parameters:
       hSem             - Handle of the semaphore to obtain
       nTimeoutInTicks  - Specify the number of system ticks after which obtaining the
                          semaphore will return.

            Valid timeouts are:

            ADI_OSAL_TIMEOUT_NONE     No wait. Results in an immediate return
                                      from this service regardless of whether
                                      or not it was successful
            ADI_OSAL_TIMEOUT_FOREVER  Wait option for calling task to suspend
                                      indefinitely  until a semaphore instance
                                      is obtained


  Returns:
      ADI_OSAL_SUCCESS    -  If semaphore acquired successfully
      ADI_OSAL_FAILED     -  An error occured while acquiring the semaphore
      ADI_OSAL_TIMEOUT    -  Failed to acquire the semaphore within the specified
                             time limit
      ADI_OSAL_BAD_HANDLE -  If the specified semaphore handle is invalid
      ADI_OSAL_BAD_TIME   -  If the specified time is invalid for uCOS
      ADI_OSAL_CALLER_ERROR -  If the function is invoked from an invalid location

 *****************************************************************************/

ADI_OSAL_STATUS  adi_osal_SemPend(ADI_OSAL_SEM_HANDLE const hSem, ADI_OSAL_TICKS nTimeoutInTicks)
{
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a pointer to a useful structure")
    OSAL_SEMAPHORE* pSemNative = (OSAL_SEMAPHORE*) hSem;
#pragma diag(pop)
    ADI_OSAL_STATUS eRetStatus = ADI_OSAL_FAILED;
    uint32_t nIntState;

#ifdef OSAL_DEBUG
     if (( nTimeoutInTicks != ADI_OSAL_TIMEOUT_NONE )   &&
         ( nTimeoutInTicks != ADI_OSAL_TIMEOUT_FOREVER))
     {
          return (ADI_OSAL_BAD_TIME);
     }

     if (_adi_osal_IsCurrentLevelISR())
     {
         return (ADI_OSAL_CALLER_ERROR);
     }

    if ((hSem == ADI_OSAL_INVALID_SEM) ||
        (hSem == NULL))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

    if (pSemNative->bActiveS == false)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */

    /* only change the pending status if the call is blocking */
    if(nTimeoutInTicks == ADI_OSAL_TIMEOUT_FOREVER)
    {
        ENTER_CRITICAL_INTERNAL(nIntState);
        pSemNative->bPendingS = true;
        EXIT_CRITICAL_INTERNAL(nIntState);
    }
    while ( eRetStatus == ADI_OSAL_FAILED )
    {
        ENTER_CRITICAL_INTERNAL(nIntState);

        /* we could have used the testset instruction, but the problem is that the
        location of pSemNative is then required to be in a particular memory */
        if (pSemNative->nCount > 0u)
        {
            pSemNative->nCount--;
            pSemNative->bPendingS = false;
            eRetStatus = ADI_OSAL_SUCCESS;
        }
        EXIT_CRITICAL_INTERNAL(nIntState);

        if(nTimeoutInTicks == ADI_OSAL_TIMEOUT_NONE)
        {
            /* If timeout value is None, the function should return
               the value of the semaphore immediately.
               So it should exit the waiting loop
             */
            break;
        }
    }

    return ( eRetStatus );
}




/*****************************************************************************
  Function: adi_osal_SemPost

  This function is used to release a semaphore.
  Its count will be incremented if the specified semaphore is released
  successfully

  Parameters:
      hSem - Handle of the semaphore to be released

  Returns:
      ADI_OSAL_SUCCESS     - If semaphore released successfully
      ADI_OSAL_FAILED      - an error occured while releasing the specified semaphore
      ADI_OSAL_BAD_HANDLE  - If the specified semaphore handle is invalid

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_SemPost(ADI_OSAL_SEM_HANDLE const hSem)
{
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a pointer to a useful structure")
    OSAL_SEMAPHORE* pSemNative = (OSAL_SEMAPHORE*) hSem;
#pragma diag(pop)
    ADI_OSAL_STATUS eRetStatus = ADI_OSAL_FAILED;
    uint32_t nIntState;

#ifdef OSAL_DEBUG
    if ((hSem == ADI_OSAL_INVALID_SEM) ||
        (hSem == NULL) ||
        (pSemNative->bActiveS == false))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */

    ENTER_CRITICAL_INTERNAL(nIntState);
    pSemNative->nCount++;
    EXIT_CRITICAL_INTERNAL(nIntState);

    return( ADI_OSAL_SUCCESS );
}

#pragma diag(pop) /* global misra deviations */
/*
**
** EOF: 
**
*/


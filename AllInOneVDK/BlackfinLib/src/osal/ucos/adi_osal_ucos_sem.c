/* $Revision: 5506 $
 * $Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $
******************************************************************************
Copyright (c), 2008-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Operating System Abstraction Layer - OSAL for uCOS - Semaphore functions

Description:
              This file contains the Semaphore API's for the uCOS implementation of the OSAL

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

#include <string.h>                                                             /* for strncpy */
#include <stdlib.h>                                                             /* for malloc/free */
#include <ccblkfn.h>                                                            /* for cli/sti */
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

ADI_OSAL_STATUS  adi_osal_SemCreate(ADI_OSAL_SEM_HANDLE *phSem, uint32_t nInitCount)
{
    ADI_OSAL_STATUS eRetStatus;
    OS_EVENT *pSemaphore;

    if (phSem == NULL)
    {
        return(ADI_OSAL_FAILED);
    }

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        return (ADI_OSAL_CALLER_ERROR);
    }

    /* Check because it will be converted to a 16-bit value */
    if (nInitCount > OSAL_MAX_UINT16)
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        return (ADI_OSAL_BAD_COUNT);
    }
#endif /* OSAL_DEBUG */

    PAUSE_PROFILING();
    pSemaphore = OSSemCreate((INT16U) nInitCount & OSAL_MAX_UINT16);
    RESUME_PROFILING();

    if(pSemaphore != NULL)
    {
        eRetStatus = ADI_OSAL_SUCCESS;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the uCOS type into an OSAL type")
        *phSem = (ADI_OSAL_SEM_HANDLE) pSemaphore;
#pragma diag(pop)
    }
    else
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        eRetStatus = ADI_OSAL_FAILED;
    }
    return(eRetStatus);
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

ADI_OSAL_STATUS  adi_osal_SemDestroy(ADI_OSAL_SEM_HANDLE const hSem)
{
    INT8U nErr;
    ADI_OSAL_STATUS eRetStatus;

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a pointer to a useful structure")
    OS_EVENT *hSemNative = (OS_EVENT *) hSem;
#pragma diag(pop)

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if(hSem == ADI_OSAL_INVALID_SEM)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

#endif /* OSAL_DEBUG */

    PAUSE_PROFILING();
    OSSemDel( hSemNative, OS_DEL_NO_PEND, &nErr );
    RESUME_PROFILING();
    switch ( nErr )
    {
        case OS_ERR_NONE:
            eRetStatus =  ADI_OSAL_SUCCESS;
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_EVENT_TYPE:
        case OS_ERR_PEVENT_NULL:
            eRetStatus =  ADI_OSAL_BAD_HANDLE;
            break;

        case  OS_ERR_DEL_ISR:
            eRetStatus= ADI_OSAL_CALLER_ERROR;
            break;
#endif /* OSAL_DEBUG */

        case OS_ERR_TASK_WAITING:
            eRetStatus= ADI_OSAL_THREAD_PENDING;
            break;

        case OS_ERR_INVALID_OPT:
        default:
            eRetStatus= ADI_OSAL_FAILED;
            break;

    } /*switch */

    return( eRetStatus );
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
            1....0xFFFFFFFE           Selecting a numeric value specifies the
                                      maximum time limit (in system ticks ) for
                                      obtaining a semaphore


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
    INT8U nErr;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a pointer to a useful structure")
    OS_EVENT* hSemNative = (OS_EVENT*) hSem;
#pragma diag(pop)


#ifdef OSAL_DEBUG
     if((nTimeoutInTicks > OSAL_MAX_TIMEOUT) && 
        (nTimeoutInTicks != ADI_OSAL_TIMEOUT_FOREVER))
     {
          return(ADI_OSAL_BAD_TIME);
     }

     if (CALLED_FROM_AN_ISR)
     {
         return (ADI_OSAL_CALLER_ERROR);
     }

    if(hSem == ADI_OSAL_INVALID_SEM)
    {
        return  (ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */



    if ( nTimeoutInTicks == ADI_OSAL_TIMEOUT_NONE )
    {

        PAUSE_PROFILING();
        if( 0u == OSSemAccept(hSemNative) )
        {
            eRetStatus = ADI_OSAL_FAILED;
        }
        else
        {
            eRetStatus = ADI_OSAL_SUCCESS;
        }
        RESUME_PROFILING();

    }
    else
    {
        if(nTimeoutInTicks == ADI_OSAL_TIMEOUT_FOREVER)
        {
            nTimeoutInTicks = 0u;
        }

        PAUSE_PROFILING();
        OSSemPend(hSemNative, (INT16U) nTimeoutInTicks,  &nErr);
        RESUME_PROFILING();
        switch (nErr)
        {
            case OS_ERR_NONE:
                eRetStatus = ADI_OSAL_SUCCESS;
                break;

#ifdef OSAL_DEBUG
            case OS_ERR_EVENT_TYPE:
            case OS_ERR_PEVENT_NULL:
                eRetStatus = ADI_OSAL_BAD_HANDLE;
                break;

            case OS_ERR_PEND_ISR:       /* FALLTHROUGH */
            case OS_ERR_PEND_LOCKED:

                eRetStatus = ADI_OSAL_CALLER_ERROR;
                break;
#endif /* OSAL_DEBUG */

            case OS_ERR_TIMEOUT:
                eRetStatus =  ADI_OSAL_TIMEOUT;
                break;

            default:
                eRetStatus = ADI_OSAL_FAILED;
                break;
        }
    }

    return(eRetStatus);
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
    INT8U nErrCode;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a pointer to a useful structure")
    OS_EVENT* hSemNative = (OS_EVENT*) hSem;
#pragma diag(pop)

#ifdef OSAL_DEBUG
    if(hSem == ADI_OSAL_INVALID_SEM)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */

    PAUSE_PROFILING();
    nErrCode = OSSemPost(hSemNative);
    RESUME_PROFILING();
    switch (nErrCode)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_EVENT_TYPE:
        case OS_ERR_PEVENT_NULL:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;

        case OS_ERR_SEM_OVF:
            eRetStatus = ADI_OSAL_COUNT_OVERFLOW;
            break;
#endif /* OSAL_DEBUG */

        default:
            eRetStatus  = ADI_OSAL_FAILED;
            break;

    }

  return(eRetStatus);
}

#pragma diag(pop)  /* global file supressions */

/*
**
** EOF:
**
*/

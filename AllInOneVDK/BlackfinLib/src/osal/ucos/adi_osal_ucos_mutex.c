/* $Revision: 5506 $
 * $Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $
******************************************************************************
Copyright (c), 2008-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Operating System Abstraction Layer - OSAL for uCOS - Mutex functions

Description:
              This file contains the Mutex API's for the uCOS implementation of the OSAL

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



/******

NOTE: PRIORITY INHERITANCE (CEILING) IS ALWAYS CONSIDERED TURNED OFF IN UCOS

******/

/*****************************************************************************
  Function: adi_osal_MutexCreate

  This function is used to create a mutex.
  Mutex behaviour is emulated by using binary semaphore with some onwership restrictions.

  Parameters:
      phMutexID  - Pointer to a location to write the returned mutex handle

  Returns:
      ADI_OSAL_SUCCESS      - If mutex is created successfully
      ADI_OSAL_FAILED       - If failed to create mutex
      ADI_OSAL_MEM_ALLOC_FAILED    - If there isn't sufficient memory to create the mutex
      ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid location

  Note:
    uCOS needs different priority levels for each mutex. Since the OSAL does not have
    a knowledge of each task in the system, its only option would be to reserve the
    top levels for usage by the mutex. Because of this requirement, there may be a
    case where the highest priority task may not get CPU control due to priority
    inheritance between two lower priority tasks. To avoid this situation, mutex
    functionality is replaced with a binary semaphore. When a Mutex is shared by
    more than one task, It is the responsibility of the application to ensure that
    the task with the highest priority should "acquire the mutex successfully"
    (Since it is a  binary semaphore).  This helps to prevent the highest priority
    task from getting starved."

     "phMutexID" points to NULL if mutex creation is failed.

 *****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexCreate(ADI_OSAL_MUTEX_HANDLE *phMutex)
{
    ADI_OSAL_STATUS eRetStatus;
    ADI_OSAL_MUTEX_INFO_PTR pMutexObj;

    if (phMutex == NULL)
    {
        return(ADI_OSAL_FAILED);
    }

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return (ADI_OSAL_CALLER_ERROR);
    }
#endif /* OSAL_DEBUG */

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the ADI_OSAL_MUTEX_INFO_PTR type in to a generic void** type required by malloc")
    /* a new mutex is created, we need to allocate some memory for it */
    if (ADI_OSAL_SUCCESS != _adi_osal_MemAlloc((void**) &pMutexObj, sizeof(ADI_OSAL_MUTEX_INFO)))
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return (ADI_OSAL_MEM_ALLOC_FAILED);
    }
#pragma diag(pop)

    /* the only drawback is that in case of a failure to create a semaphore object, there will be an extra
    alloc/free cycle. However, it will happen exceptionally and it is still better to avoid code duplication */
    eRetStatus = adi_osal_MutexCreateStatic(pMutexObj, sizeof(ADI_OSAL_MUTEX_INFO), phMutex);

    if (eRetStatus != ADI_OSAL_SUCCESS)
    {
        _adi_osal_MemFree(pMutexObj);
    }

    return (eRetStatus);
}



/*****************************************************************************
  Function: adi_osal_MutexCreateStatic

    THIS FUNCTION IS THE SAME AS adi_osal_MutexCreate except that memory is passed
    to it instead of relying on dynamic memory.

  Parameters:
      phMutexID  - Pointer to a location to write the returned mutex handle

  Returns:
      ADI_OSAL_SUCCESS      - If mutex is created successfully
      ADI_OSAL_FAILED       - If failed to create mutex
      ADI_OSAL_MEM_ALLOC_FAILED    - If there isn't sufficient memory to create the mutex
      ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid location

 *****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexCreateStatic(void* const pMutexObject, uint32_t nMutexObjSize, ADI_OSAL_MUTEX_HANDLE *phMutex)
{
    ADI_OSAL_STATUS eRetStatus;
    OS_EVENT*       hMtxNative;
    uint32_t        nAlignment;

    if (phMutex == NULL)
    {
        return(ADI_OSAL_FAILED);
    }

#ifdef OSAL_DEBUG

    if (CALLED_FROM_AN_ISR)
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ( nMutexObjSize < sizeof(ADI_OSAL_MUTEX_INFO) )
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return (ADI_OSAL_MEM_ALLOC_FAILED);
    }

#pragma diag(push)
#pragma diag(suppress:misra_rule_1_1 : "typecasting is not allowed otherwise, notified compiler team")
    nAlignment = ((uint32_t) (pMutexObject) & 0x3u);
#pragma diag(pop)
    if (nAlignment != 0u)
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return (ADI_OSAL_BAD_MEMORY);
    }

#endif /* OSAL_DEBUG */

    PAUSE_PROFILING();
    hMtxNative = OSSemCreate(INIT_COUNT_FOR_EMULATED_MUTEX);
    RESUME_PROFILING();
    if ( hMtxNative != NULL)
    {
        ADI_OSAL_MUTEX_INFO_PTR pMutex = (ADI_OSAL_MUTEX_INFO_PTR) pMutexObject;

        pMutex->hMutexID = hMtxNative;
        pMutex->hOwnerThread = NULL; /* the mutex is not owned by the thread creating it but by the
                                                           the thread acquiring it (MutexPend) */
        pMutex->nAcquisitionCount = 0u;                 /* initially the mutex is not acquired */

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the uCOS type in to an OSAL type")
        *phMutex = (ADI_OSAL_MUTEX_HANDLE) pMutex;
#pragma diag(pop)

        return (ADI_OSAL_SUCCESS);
    }
    else
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return (ADI_OSAL_CALLER_ERROR);
    }
}



/*****************************************************************************
  Function: adi_osal_MutexGetMutexObjSize

  This function returns the size of a mutex object. It can be used by the adi_osal_MutexCreateStatic
  function to determine what the object size should be for this particular RTOS implementation.

  Parameters:
      None

  Returns:
      size of a mutex object in bytes.

*****************************************************************************/

uint32_t adi_osal_MutexGetMutexObjSize(void)
{
    return ( sizeof(ADI_OSAL_MUTEX_INFO) );
}

/*****************************************************************************
  Function: adi_osal_MutexDestroyStatic

  This function is used to delete a mutex that has been allocated with
  adi_osal_MutexCreateStatic().

  Parameters:
      hMutexID      - Handle of the mutex to be deleted

  Returns:
      ADI_OSAL_SUCCESS      - If mutex is deleted successfully
      ADI_OSAL_FAILED       - If failed to delete mutex
      ADI_OSAL_BAD_HANDLE   - If the specified mutex handle is invalid
      ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid location
      ADI_OSAL_THREAD_PENDING  - one or more thread is pending on the specified mutex.
  Notes:
      Only owner is authorised to release the acquired mutex. But it
      can "destroyed" by  other task.

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexDestroyStatic(ADI_OSAL_MUTEX_HANDLE const hMutex)
{
    OS_EVENT        *pRetValue;
    INT8U           nErr;
    ADI_OSAL_STATUS eRetStatus = ADI_OSAL_BAD_HANDLE;

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle into a pointer to a useful structure")
    ADI_OSAL_MUTEX_INFO_PTR  pMutexInfo  = (ADI_OSAL_MUTEX_INFO_PTR) hMutex;
#pragma diag(pop)


#ifdef OSAL_DEBUG
    if(CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ((hMutex == NULL) || (hMutex == ADI_OSAL_INVALID_MUTEX ))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif  /* OSAL_DEBUG */

    /* check whether index is with in range and points to a valid mutex */
    if(pMutexInfo->hMutexID != NULL)
    {
        PAUSE_PROFILING();
        pRetValue = OSSemDel(pMutexInfo->hMutexID, OS_DEL_NO_PEND, &nErr);
        RESUME_PROFILING();
        switch (nErr)
        {
            case OS_ERR_NONE:
                if(NULL == pRetValue)
                {
                    eRetStatus = ADI_OSAL_SUCCESS;
                }
                else
                {
                    eRetStatus  = ADI_OSAL_FAILED;
                }
                break;

            case OS_ERR_EVENT_TYPE:
                break;

            case  OS_ERR_DEL_ISR:
                eRetStatus= ADI_OSAL_CALLER_ERROR;
                break;

            case OS_ERR_TASK_WAITING:
                  eRetStatus= ADI_OSAL_THREAD_PENDING;
                  break;

            case OS_ERR_INVALID_OPT:       /* FALLTHROUGH */
            case OS_ERR_PEVENT_NULL:
            default:
                eRetStatus  = ADI_OSAL_FAILED;
                break;
        } /* switch */
    }
    else
    {
        eRetStatus = ADI_OSAL_BAD_HANDLE;
    }

    return(eRetStatus);
}


/*****************************************************************************
  Function: adi_osal_MutexDestroy

  This function is used to delete a mutex.

  Parameters:
      hMutexID      - Handle of the mutex to be deleted

  Returns:
      ADI_OSAL_SUCCESS      - If mutex is deleted successfully
      ADI_OSAL_FAILED       - If failed to delete mutex
      ADI_OSAL_BAD_HANDLE   - If the specified mutex handle is invalid
      ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid location
      ADI_OSAL_THREAD_PENDING  - one or more thread is pending on the specified mutex.
  Notes:
      Only owner is authorised to release the acquired mutex. But it
      can "destroyed" by  other task.

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexDestroy(ADI_OSAL_MUTEX_HANDLE const hMutex)
{
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle into a pointer to a useful structure")
    ADI_OSAL_MUTEX_INFO_PTR  pMutexInfo  = (ADI_OSAL_MUTEX_INFO_PTR) hMutex;
#pragma diag(pop)

    eRetStatus = adi_osal_MutexDestroyStatic(hMutex);
    if (eRetStatus == ADI_OSAL_SUCCESS )
    {
        _adi_osal_MemFree(pMutexInfo);
    }
    
    return (eRetStatus);    
}


/*****************************************************************************
  Function: adi_osal_MutexPend

  This function is used to lock a mutex (acquire a resource)

  Parameters:
      hMutex            - Handle of the mutex which need to be acquired
      nTimeoutInTicks   - Specify the number of system ticks for acquiring the mutex

      Valid timeouts are:

        ADI_OSAL_TIMEOUT_NONE       -  No wait. Results in an immediate return
                                       from this service  regardless of whether
                                       or not it was successful

        ADI_OSAL_TIMEOUT_FOREVER    -  Wait option for calling task to suspend
                                       indefinitely until a specified  mutex is
                                       obtained

        1 ....0xFFFFFFFE            -  Selecting a numeric value specifies the
                                       maximum time limit (in system ticks) for
                                       obtaining specified mutex

  Returns:
      ADI_OSAL_SUCCESS      - If the specified mutex is locked successfully
      ADI_OSAL_FAILED       - If failed to lock the specified mutex
      ADI_OSAL_BAD_HANDLE   - If the specified mutex ID is invalid
      ADI_OSAL_TIMEOUT      - If the specified time limit expired.
      ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid location

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexPend(ADI_OSAL_MUTEX_HANDLE const hMutex, ADI_OSAL_TICKS nTimeoutInTicks)
{
    INT8U    nErr;
    ADI_OSAL_STATUS  eRetStatus;
    OS_TCB  *hCurrThread;
    bool bSchedulerStarted;

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle into a pointer to a useful structure")
    ADI_OSAL_MUTEX_INFO_PTR  pMutexInfo = (ADI_OSAL_MUTEX_INFO_PTR) hMutex;
#pragma diag(pop)

#ifdef OSAL_DEBUG
    if((nTimeoutInTicks > OSAL_MAX_TIMEOUT) && (nTimeoutInTicks != ADI_OSAL_TIMEOUT_FOREVER))
    {
        return(ADI_OSAL_BAD_TIME);
    }

    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ((hMutex == NULL) || (hMutex == ADI_OSAL_INVALID_MUTEX ))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

    /* check whether index is with in range and points to a valid mutex */
    if(pMutexInfo->hMutexID == NULL)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */


    /* Current thread handle is necessary if all following conditionals paths */
    /* The mutex owner is a uCOS thread pointer because all threads in the
     * application are uCOS threads but not all threads are OSAL threads 
     */

    hCurrThread = OSTCBCur;


    /* Nesting: check if the mutex is already acquired by the same owner */
    if (pMutexInfo->nAcquisitionCount > 0u)                     /* already owned ... */
    {
        if (pMutexInfo->hOwnerThread == hCurrThread)            /* ... by the same owner */
        {
#ifdef OSAL_DEBUG
            /* check for count overflow */
            if (pMutexInfo->nAcquisitionCount == 0xFFFFFFFFu)
            {
                return (ADI_OSAL_COUNT_OVERFLOW);
            }
#endif /* OSAL_DEBUG */

            pMutexInfo->nAcquisitionCount += 1u;
            return (ADI_OSAL_SUCCESS);
        }
        /* else, other owner than current thread --> current thread will pend
        this case is automatically taken care of in the code below */
    }
    /* else, the mutex needs to be acquired */

    if(nTimeoutInTicks == ADI_OSAL_TIMEOUT_NONE)
    {
        INT16U  nErrVal;
        PAUSE_PROFILING();
        nErrVal = OSSemAccept(pMutexInfo->hMutexID);
        RESUME_PROFILING();
        if ( 0u == nErrVal)
        {
            eRetStatus = ADI_OSAL_FAILED;
        }
        else
        {
            /* A binary semaphore is acquired successfully. Store the thread 
             * handle of the current thread. This information is needed to 
             * find the owner for subsequent operations.
             */
            pMutexInfo->hOwnerThread = hCurrThread;
            pMutexInfo->nAcquisitionCount += 1u;
            eRetStatus  = ADI_OSAL_SUCCESS;
        }
    }
    else
    {
        if(nTimeoutInTicks == ADI_OSAL_TIMEOUT_FOREVER)
        {
            nTimeoutInTicks =0u;
        }

        PAUSE_PROFILING();
        OSSemPend( pMutexInfo->hMutexID, (INT16U) nTimeoutInTicks,  &nErr);
        RESUME_PROFILING();
        switch (nErr)
        {
            case OS_ERR_NONE:
                /* A binary semaphore is acquired successfully. Store the taskID
                   of the current task.This information is needed to find the owner
                   when  releasing it.
                */
                pMutexInfo->hOwnerThread = hCurrThread;
                pMutexInfo->nAcquisitionCount += 1u;
                eRetStatus = ADI_OSAL_SUCCESS;
                break;

            case OS_ERR_TIMEOUT:
                eRetStatus = ADI_OSAL_TIMEOUT;
                break;

#ifdef OSAL_DEBUG
            case OS_ERR_EVENT_TYPE:
                eRetStatus = ADI_OSAL_BAD_HANDLE;
                break;

            case OS_ERR_PEND_ISR:       /* FALLTHROUGH */
            case OS_ERR_PEND_LOCKED:
                eRetStatus = ADI_OSAL_CALLER_ERROR;
                break;
#endif /* OSAL_DEBUG */

            case OS_ERR_PEVENT_NULL:
            default:
                eRetStatus = ADI_OSAL_FAILED;
                break;
        }
    }

    return (eRetStatus);
}



/*****************************************************************************
  Function: adi_osal_MutexPost

  This function is used to unlock a mutex.

  Parameters:
      hMutexID      - Handle of the mutex which needs to be unlocked

  Returns:
      ADI_OSAL_SUCCESS          - If mutex is un locked successfully
      ADI_OSAL_FAILED           - If failed unlock mutex
      ADI_OSAL_BAD_HANDLE       - If the specified mutex ID is invalid
      ADI_OSAL_NOT_MUTEX_OWNER  - If it is called from a task which does not own it.
      ADI_OSAL_CALLER_ERROR       - The function is invoked from an invalid location

  note:
      Mutex can be successfully released by its owner : Only the task which acquired it
      can release it. Any attempt to release it by non-owner will result in error.

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexPost(ADI_OSAL_MUTEX_HANDLE const hMutex)
{
    INT8U nErr;
    INT32U nErrCode;
    ADI_OSAL_STATUS eRetStatus;
    OS_TCB *hCurrThread;

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle into a pointer to a useful structure")
    ADI_OSAL_MUTEX_INFO_PTR  pMutexInfo  = (ADI_OSAL_MUTEX_INFO_PTR) hMutex;
#pragma diag(pop)

#ifdef OSAL_DEBUG
    /* Even though semaphore posting is valid within an ISR, Mutex posting is NOT.
       So we artificially restrict it here since we use binary semaphores to emulate
       mutexes */
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    /* check whether "pMutexInfo" is a valid "mutex" and that it is called from
       the thread which acquired it.
       Irrespective of priority, A mutex can be "released" only by the thread
       which owns it.
       This condition check is implemented in OSAL since mutex is replaced by the binary
       semaphore.
    */
    if ((hMutex == NULL) || (hMutex == ADI_OSAL_INVALID_MUTEX ))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

    if (pMutexInfo->hMutexID == NULL)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

#endif /* OSAL_DEBUG */


    /* Check if the current thread is the mutex owner */
    /* The mutex owner is a uCOS thread pointer because all threads in the
     * application are uCOS threads but not all threads are OSAL threads 
     */ 
    hCurrThread = OSTCBCur;

    if(pMutexInfo->hOwnerThread != hCurrThread)
    {
        return (ADI_OSAL_NOT_MUTEX_OWNER);
    }

/* Scheduler lock is necessary to keep the pMutexInfo structure in sync with the OS Post
       so that there isn't a case when the OSSemPost creates a reschedule with the pMutexInfo
       structure not yet updated*/
    PAUSE_PROFILING();
    OSSchedLock();
    RESUME_PROFILING();

    /* Ok, the current thread is the owner. Nesting need to be checked now */
    if (pMutexInfo->nAcquisitionCount > 1u)
    {
        pMutexInfo->nAcquisitionCount--;
        eRetStatus = ADI_OSAL_SUCCESS;
    }
    else
    {
        /* here: current thread is the owner and nAcquisitionCount is 1 */
        PAUSE_PROFILING();
        nErrCode =  (INT32U) OSSemPost(pMutexInfo->hMutexID);
        RESUME_PROFILING();
        switch (nErrCode)
        {
            case OS_ERR_NONE:
                pMutexInfo->hOwnerThread = NULL;
                pMutexInfo->nAcquisitionCount = 0u;
                eRetStatus = ADI_OSAL_SUCCESS;
                break;

    #ifdef OSAL_DEBUG
            case OS_ERR_EVENT_TYPE:
                eRetStatus = ADI_OSAL_BAD_HANDLE;
                break;
    #endif /* OSAL_DEBUG */
            case OS_ERR_PEVENT_NULL:           /* FALLTHROUGH */
            case OS_ERR_SEM_OVF:
            default :
                eRetStatus = ADI_OSAL_FAILED;
                break;
        } /*end of switch */
    }

    PAUSE_PROFILING();
    OSSchedUnlock();
    RESUME_PROFILING();

    return(eRetStatus);
}



#pragma diag(pop)  /* global file supressions */

/*
**
** EOF: 
**
*/

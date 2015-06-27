/* $Revision: 5506 $
 * $Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $
******************************************************************************
Copyright (c), 2008-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Operating System Abstraction Layer - OSAL for uCOS - Message Queues functions

Description:
              This file contains the Message Queue API's for the uCOS implmentation of the OSAL

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
  Function: adi_osal_MsgQueueCreate

  This function creates a message queue that is typically used for intertask
  communication. The Message is always a pointer which points to the base of
  a buffer which contains the actual message (indirect message). Hence the
  size of the message in the queue is always 4 bytes (a pointer).


  Parameters:
    phMsgQ    - Pointer to a location to write the returned message queue ID
    aMsgQ     - Buffer to be used to store the messages
    nMaxMsgs  - Maximum number of messages the queue can hold

  Returns:
   ADI_OSAL_SUCCESS         - If message queue is created successfully
   ADI_OSAL_FAILED          - If failed to create message queue
   ADI_OSAL_CALLER_ERROR    - If function is invoked from an invalid location

*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_MsgQueueCreate(ADI_OSAL_QUEUE_HANDLE *phMsgQ, void* aMsgQ[], uint32_t nMaxMsgs)
{
    ADI_OSAL_STATUS eRetStatus;
    OS_EVENT *pMessageQ;

#ifdef OSAL_DEBUG
    if (phMsgQ == NULL)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

    if (CALLED_FROM_AN_ISR)
    {
        *phMsgQ = ADI_OSAL_INVALID_QUEUE;
        return (ADI_OSAL_CALLER_ERROR);
    }

    /* message numbers are store in a 16-bit variable in uCOS */
    if (nMaxMsgs > OSAL_MAX_UINT16)
    {
        *phMsgQ = ADI_OSAL_INVALID_QUEUE;
        return (ADI_OSAL_BAD_COUNT);
    }

#endif /* OSAL_DEBUG */

    PAUSE_PROFILING();
    pMessageQ = OSQCreate(aMsgQ,(INT16U)nMaxMsgs );
    RESUME_PROFILING();
    if(pMessageQ != NULL)
    {
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the uCOS type into an OSAL type")
        *phMsgQ = (ADI_OSAL_QUEUE_HANDLE) pMessageQ;
#pragma diag(pop)
        eRetStatus  =   ADI_OSAL_SUCCESS;
    }
    else
    {
        *phMsgQ = ADI_OSAL_INVALID_QUEUE;
        return (ADI_OSAL_FAILED);
    }

    return(eRetStatus);
}



/*****************************************************************************
  Function: adi_osal_MsgQueueDestroy

  This function is used to delete the specified message queue

  Parameters:
     hMsgQ   -  handle of the message queue to be deleted



  Returns:
      ADI_OSAL_SUCCESS          - If message queue is deleted successfully
      ADI_OSAL_FAILED           - If failed to delete message queue
      ADI_OSAL_BAD_HANDLE       - If the specified message queue ID is invalid
      ADI_OSAL_CALLER_ERROR     - If function is invoked from an invalid location (i.e an ISR)
      ADI_OSAL_THREAD_PENDING   - if one or more thread is pending for the message on
                                  the specified queue.
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_MsgQueueDestroy(ADI_OSAL_QUEUE_HANDLE const hMsgQ)
{
    INT8U    nErr;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a native uCOS pointer")
    OS_EVENT *hMsgQNative = (OS_EVENT *) hMsgQ;
#pragma diag(pop)

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ((hMsgQ == ADI_OSAL_INVALID_QUEUE) || (hMsgQ == NULL))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif


    PAUSE_PROFILING();
    OSQDel(hMsgQNative, OS_DEL_NO_PEND, &nErr);
    RESUME_PROFILING();
    switch (nErr)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_EVENT_TYPE:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;

        case OS_ERR_DEL_ISR:
            eRetStatus = ADI_OSAL_CALLER_ERROR;
            break;
#endif /* OSAL_DEBUG */

        case OS_ERR_TASK_WAITING:
            eRetStatus= ADI_OSAL_THREAD_PENDING;
            break;

        case OS_ERR_INVALID_OPT:        /* FALLTHROUGH */
        case OS_ERR_PEVENT_NULL:
        default :
            eRetStatus = ADI_OSAL_FAILED;
            break;
    }

    return(eRetStatus);
}



/*****************************************************************************
  Function: adi_osal_MsgQueuePost

  This function is used to send a message to the specified message queue.


  Parameters:
      hMsgQ     - Handle of the message queue to use.
      pMsg      - Pointer to the message to send


  Returns:
      ADI_OSAL_SUCCESS      - If message queued successfully
      ADI_OSAL_FAILED       - If failed to queue the message
      ADI_OSAL_BAD_HANDLE   - If the specified message queue handle is invalid
      ADI_OSAL_QUEUE_FULL   - If queue is full
*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MsgQueuePost(ADI_OSAL_QUEUE_HANDLE const hMsgQ, void *pMsg)
{
    INT8U           nRetValue;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a native uCOS pointer")
    OS_EVENT *hMsgQNative = (OS_EVENT *) hMsgQ;
#pragma diag(pop)

    if ((hMsgQ == ADI_OSAL_INVALID_QUEUE) || (hMsgQ == NULL))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

    PAUSE_PROFILING();
    nRetValue = OSQPost(hMsgQNative, pMsg);
    RESUME_PROFILING();
    switch (nRetValue)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

        case OS_Q_FULL:
            eRetStatus = ADI_OSAL_QUEUE_FULL ;
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_EVENT_TYPE:     /* FALLTHROUGH */
        case OS_ERR_PEVENT_NULL:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;
#endif /* OSAL_DEBUG */

        default :
            eRetStatus =  ADI_OSAL_FAILED;
            break;
    }

    return(eRetStatus);
}



/*****************************************************************************
  Function: adi_osal_MsgQueuePend

  This function is used to receive a message from the specified message queue.

  Parameters:
      hMsgQ             -  handle of the Message queue to retrieve the message from
      pMsgPtr           -  Pointer to a location to store the message
      nTimeoutInTicks   -  Timeout in system ticks for retrieving the message.

      Valid timeouts are:

         ADI_OSAL_TIMEOUT_NONE     -   No wait. Results in an immediate return
                                       from this service regardless of whether
                                       or not it was successful

         ADI_OSAL_TIMEOUT_FOREVER  -   suspends the calling thread indefinitely
                                       until a message is obtained

         1 ... 0xFFFFFFFE          -   Selecting a numeric value specifies the
                                       maximum time limit (in system ticks) for
                                       obtaining a message from the queue

  Returns:
      ADI_OSAL_SUCCESS      - If message is received and copied to ppMsg buffer
                              and removed from queue.
      ADI_OSAL_FAILED       - If failed to get a message.
      ADI_OSAL_TIMEOUT      - If failed get message due to timeout.
      ADI_OSAL_BAD_HANDLE   - If the specified message queue ID   is invalid
      ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid location (i.e an ISR)
*****************************************************************************/

#pragma diag(push)
#pragma diag(suppress:misra_rule_17_5 : "In this case we need to allow 2 levels of pointer indirection because we need to write the value of a pointer in a memory location")
ADI_OSAL_STATUS  adi_osal_MsgQueuePend(ADI_OSAL_QUEUE_HANDLE const hMsgQ, void **ppMsg, ADI_OSAL_TICKS nTimeoutInTicks)
{
    INT8U nErr;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a native uCOS pointer")
    OS_EVENT *hMsgQNative = (OS_EVENT *) hMsgQ;
#pragma diag(pop)


#ifdef OSAL_DEBUG
    if((nTimeoutInTicks > OSAL_MAX_TIMEOUT) && (nTimeoutInTicks != ADI_OSAL_TIMEOUT_FOREVER))
    {
         return (ADI_OSAL_BAD_TIME);
    }

    if ((CALLED_FROM_AN_ISR) || (CALLED_IN_SCHED_LOCK_REGION))
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ((hMsgQ == ADI_OSAL_INVALID_QUEUE) || (hMsgQ == NULL))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */


    PAUSE_PROFILING();
    switch (nTimeoutInTicks)
    {
        case ADI_OSAL_TIMEOUT_NONE:
            *ppMsg = OSQAccept(hMsgQNative, &nErr);
            break;
        case ADI_OSAL_TIMEOUT_FOREVER:
            *ppMsg = OSQPend(hMsgQNative, 0u,  &nErr);
            break;
        default:
            *ppMsg = OSQPend(hMsgQNative, (INT16U)nTimeoutInTicks,  &nErr);
            break;
    }
    RESUME_PROFILING();
    /* Only one switch-case for error code from  both OSQPend and OSQAccept. */
    switch (nErr)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

        case OS_Q_EMPTY:
            /* the call that returned OS_Q_EMPTY will have written NULL to pMsgPtr */
            eRetStatus = ADI_OSAL_QUEUE_EMPTY;
            break;

        case OS_ERR_TIMEOUT:
            eRetStatus = ADI_OSAL_TIMEOUT;
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_EVENT_TYPE:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;

        case OS_ERR_PEND_ISR:           /* FALLTHROUGH */
        case OS_ERR_PEND_LOCKED:
            eRetStatus = ADI_OSAL_CALLER_ERROR;
            break;
#endif /*OSAL_DEBUG */

        case OS_ERR_PEVENT_NULL:
        default:
            eRetStatus = ADI_OSAL_FAILED;
            break;
    }

    return( eRetStatus );
}
#pragma diag(pop)  /* rule 17.5 (2 level of indirection) */

#pragma diag(pop)  /* global file supressions */

/*
**
** EOF: 
**
*/

/* $Revision: 5506 $
 * $Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $
******************************************************************************
Copyright (c), 2008-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Operating System Abstraction Layer - OSAL for uCOS - Events functions

Description:
              This file contains the Events API's for the uCOS implementation of the OSAL

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
/* Rule-11.3 indicates that typecast of a integer value into pointer is invalid */
#pragma diag(suppress:misra_rule_11_3 : "typecasting is necessary everytimes a predefine value is written to a return pointer during error conditions")
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exits (mostly for handling parameter error checking) increases the code readability and therefore maintenance")
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

static ADI_OSAL_STATUS OsalPostFlags( ADI_OSAL_EVENT_HANDLE const hEventGroup,
                                      ADI_OSAL_EVENT_FLAGS nEventFlags,
                                      uint8_t nOption );




/*****************************************************************************
  Function: adi_osal_EventGroupCreate

  This function creates an event queue. This is typically used to synchronize threads
  with events that happen in the system.


  Parameters:
    phEventGroup - Pointer to a location to write the returned event group handle

  Returns:
   ADI_OSAL_SUCCESS         - If message queue is created successfully
   ADI_OSAL_FAILED          - If failed to create message queue
   ADI_OSAL_CALLER_ERROR    - If function is invoked from an invalid location

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_EventGroupCreate(ADI_OSAL_EVENT_HANDLE *phEventGroup)
{
    OS_FLAG_GRP* pEventNative;
    INT8U nErr;
    ADI_OSAL_STATUS eRetStatus;

    if (phEventGroup == NULL)
    {
        return(ADI_OSAL_FAILED);
    }

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        *phEventGroup = ADI_OSAL_INVALID_EVENT_GROUP;
        return (ADI_OSAL_CALLER_ERROR);
    }
#endif /* OSAL_DEBUG */

    /* Flags are initially all created as unset (0x0) */
    PAUSE_PROFILING();
    pEventNative = OSFlagCreate(0x0u, &nErr);
    RESUME_PROFILING();
    switch (nErr)
    {
        case OS_ERR_NONE:
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a pointer to a useful structure")
            *phEventGroup = (ADI_OSAL_EVENT_HANDLE) pEventNative;
#pragma diag(pop)
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

        case OS_ERR_CREATE_ISR:
            *phEventGroup = ADI_OSAL_INVALID_EVENT_GROUP;
            eRetStatus = ADI_OSAL_CALLER_ERROR;
            break;

        case OS_ERR_FLAG_GRP_DEPLETED:
            *phEventGroup = ADI_OSAL_INVALID_EVENT_GROUP;
            eRetStatus = ADI_OSAL_MEM_ALLOC_FAILED;
            break;

        default:
            *phEventGroup = ADI_OSAL_INVALID_EVENT_GROUP;
            eRetStatus = ADI_OSAL_FAILED;
            break;
    }
    return (eRetStatus);
}



/*****************************************************************************
  Function: adi_osal_EventGroupDestroy

  This function is used to delete the specified Event group

  Parameters:
    hEventGroup - handle of the event group to be deleted

  Returns:
   ADI_OSAL_SUCCESS         - If message queue is created successfully
   ADI_OSAL_FAILED          - If failed to create message queue
   ADI_OSAL_CALLER_ERROR    - If function is invoked from an invalid location
   ADI_OSAL_THREAD_PENDING  - Destruction of the Event Group is not possible because
                              threads are pending on events from that group
   ADI_OSAL_BAD_HANDLE      - Invalid event flag group ID

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_EventGroupDestroy(ADI_OSAL_EVENT_HANDLE const hEventGroup)
{

    INT8U nErr;
    ADI_OSAL_STATUS eRetStatus;

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a native uCOS pointer")
    OS_FLAG_GRP* hEventNative = (OS_FLAG_GRP*) hEventGroup;
#pragma diag(pop)


#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ((hEventGroup == NULL) || (hEventGroup == ADI_OSAL_INVALID_EVENT_GROUP))
    {
        return(ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */

    PAUSE_PROFILING();
    OSFlagDel(hEventNative, OS_DEL_NO_PEND, &nErr);
    RESUME_PROFILING();
    switch(nErr)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;
        case OS_ERR_TASK_WAITING:
            eRetStatus = ADI_OSAL_THREAD_PENDING;
            break;
#ifdef OSAL_DEBUG
        case OS_ERR_DEL_ISR:
            eRetStatus = ADI_OSAL_CALLER_ERROR;
            break;
        case OS_ERR_FLAG_INVALID_PGRP:
        case OS_ERR_EVENT_TYPE:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;
#endif /* OSAL_DEBUG */
        case OS_ERR_INVALID_OPT:
        default:
            eRetStatus = ADI_OSAL_FAILED;
            break;
    }

    return (eRetStatus);
}



/*****************************************************************************
  Function: adi_osal_EventPend

  This function is used to receive event flags

  Parameters:
      hEventGroup      - handle of the event group to use
      nRequestedEvents - Specifies requested event flags.
      eGetOption       - Specifies whether all bits need to be set/cleared OR
                         any of the bits to be set.
      nTimeoutInTicks    - Timeout for the event flag in system ticks.
      pnReceivedEvents - Pointer to destination of where the retrieved event
                         flags are placed.


         The following options are valid for setting flag eGetOption.

            ADI_OSAL_EVENT_FLAG_SET_ANY  - check any of the bits specified by
                                           the nRequestedEvents is set
            ADI_OSAL_EVENT_FLAG_SET_ALL  - check all the bits specified by the
                                           nRequestedEvents are set.

         Valid options for nTimeoutInTicks  are:

           ADI_OSAL_TIMEOUT_NONE     -  No wait. Results in an immediate return
                                        from this service  regardless of whether
                                        or not it was successful
           ADI_OSAL_TIMEOUT_FOREVER  -  Wait option for calling task to suspend
                                        indefinitely until the required flags are
                                        set.
           1 ... 0XFFFFFFFE          -  Selecting a numeric value specifies the
                                        maximum time limit (in system ticks) for
                                        set required event flags

  Returns:
      ADI_OSAL_SUCCESS    -  If there is no error while retrieving the event flags.
                             This does not indicate event flag condition - the user
                             must read the flags separately.
      ADI_OSAL_FAILED     -  Error occured while retrieving event flags.
      ADI_OSAL_BAD_HANDLE -  Invalid event flag group ID
      ADI_OSAL_TIMEOUT     - If failed get specified event flag(s) due to timeout.
      ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid location (i.e an ISR)
      ADI_OSAL_BAD_OPTION  - "eGetOption" specifies a wrong options.
*****************************************************************************/

ADI_OSAL_STATUS adi_osal_EventPend (ADI_OSAL_EVENT_HANDLE const hEventGroup,
                                    ADI_OSAL_EVENT_FLAGS        nRequestedEvents,
                                    ADI_OSAL_EVENT_FLAG_OPTION  eGetOption,
                                    ADI_OSAL_TICKS              nTimeoutInTicks,
                                    ADI_OSAL_EVENT_FLAGS        *pnReceivedEvents)
{
    INT8U           nErr;
    INT8U           nWaitOption = OS_FLAG_WAIT_SET_ALL;
    OS_FLAGS        nRetValue;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a native uCOS pointer")
    OS_FLAG_GRP     *hEventNative = (OS_FLAG_GRP*) hEventGroup;
#pragma diag(pop)


#ifdef OSAL_DEBUG
    if ( (nTimeoutInTicks > OSAL_MAX_TIMEOUT) &&
         (nTimeoutInTicks != ADI_OSAL_TIMEOUT_FOREVER) )
    {
         return (ADI_OSAL_BAD_TIME);
    }

    if ( (nRequestedEvents & ADI_RESERVED_EVENT_BIT) == ADI_RESERVED_EVENT_BIT )
    {
        return (ADI_OSAL_BAD_EVENT);
    }

    if ((CALLED_FROM_AN_ISR) || (CALLED_IN_SCHED_LOCK_REGION))
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ( (eGetOption != ADI_OSAL_EVENT_FLAG_ANY) &&
         (eGetOption != ADI_OSAL_EVENT_FLAG_ALL) )
    {
        return (ADI_OSAL_BAD_OPTION);
    }

    if ((hEventGroup == NULL) || (hEventGroup == ADI_OSAL_INVALID_EVENT_GROUP))
    {
        return(ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */



    if(eGetOption == ADI_OSAL_EVENT_FLAG_ANY)
    {
        nWaitOption = OS_FLAG_WAIT_SET_ANY;
    }

    PAUSE_PROFILING();
    switch (nTimeoutInTicks)
    {
        case ADI_OSAL_TIMEOUT_NONE:
            nRetValue = OSFlagAccept(hEventNative, (OS_FLAGS) nRequestedEvents, nWaitOption, &nErr);
            break;
        case ADI_OSAL_TIMEOUT_FOREVER:
            nRetValue = OSFlagPend(hEventNative, (OS_FLAGS) nRequestedEvents, nWaitOption, 0u, &nErr);
            break;
        default:
            nRetValue = OSFlagPend(hEventNative, (OS_FLAGS) nRequestedEvents, nWaitOption, (INT16U) nTimeoutInTicks, &nErr);
            break;
    }
    RESUME_PROFILING();

    switch (nErr)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

        case OS_ERR_TIMEOUT:
            eRetStatus = ADI_OSAL_TIMEOUT;
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_EVENT_TYPE:                     /* FALLTHROUGH */
        case OS_ERR_FLAG_INVALID_PGRP:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;

        case OS_ERR_PEND_ISR:
            eRetStatus = ADI_OSAL_CALLER_ERROR;
            break;

        case OS_ERR_FLAG_WAIT_TYPE:
            eRetStatus = ADI_OSAL_BAD_OPTION;
            break;

#endif /* OSAL_DEBUG */

        default:
            eRetStatus = ADI_OSAL_FAILED;
            break;
    }

    /* uC/OS also sets its return value (nRetValue) to be 0 in case of error */
    *pnReceivedEvents = (uint32_t) nRetValue;

    return( eRetStatus );
}



/*****************************************************************************
  Function: OsalPostFlags

  This internal function is used by the adi_osal_EventSet and adi_osal_EventClear APIs
  It is a separate function in order to avoid API

   Parameters:
      hEventGroup      - handle of the event group to use
      nEventFlags      - Specifies the event flags to set.
                         'ORed' into the current event flags.
      nOption          - uCOS option OS_FLAG_SET or OS_FLAG_CLR

  Returns:
      same as adi_osal_EventSet and adi_osal_EventClear APIs

*****************************************************************************/

static ADI_OSAL_STATUS OsalPostFlags( ADI_OSAL_EVENT_HANDLE const hEventGroup,
                                      ADI_OSAL_EVENT_FLAGS nEventFlags,
                                      uint8_t nOption )
{
    INT8U nErr;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a native uCOS pointer")
    OS_FLAG_GRP *hEventNative = (OS_FLAG_GRP*) hEventGroup;
#pragma diag(pop)

#ifdef OSAL_DEBUG
    if((nEventFlags & ADI_RESERVED_EVENT_BIT) == ADI_RESERVED_EVENT_BIT)
    {
        return(ADI_OSAL_BAD_EVENT);
    }

    if ((hEventGroup == NULL) || (hEventGroup == ADI_OSAL_INVALID_EVENT_GROUP))
    {
        return(ADI_OSAL_BAD_HANDLE);
    }
#endif /* OSAL_DEBUG */

    /* Only SET operations are allowed in the adi_osal_EventSet API */
    PAUSE_PROFILING();
    OSFlagPost(hEventNative, (OS_FLAGS) nEventFlags, (INT8U) nOption, &nErr);
    RESUME_PROFILING();
    switch (nErr)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

#ifdef OSAL_DEBUG
        case OS_ERR_EVENT_TYPE:
        case OS_ERR_FLAG_INVALID_PGRP:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;
#endif /* OSAL_DEBUG */

        case OS_ERR_FLAG_INVALID_OPT:
        default:
            eRetStatus = ADI_OSAL_FAILED;
            break;
    }

    return( eRetStatus );
}



/*****************************************************************************
  Function: adi_osal_EventSet

  This function is used to set one or more event flags.

   Parameters:
      hEventGroup      - handle of the event group to use
      nEventFlags      - Specifies the event flags to set.
                         'ORed' into the current event flags.

  Returns:
      ADI_OSAL_SUCCESS    - If the event flag(s) are posted successfully.
      ADI_OSAL_FAILED     - If failed to post the event flags.
      ADI_OSAL_BAD_HANDLE - Invalid event flag group ID
      ADI_OSAL_BAD_EVENT  - the events presented in the nEventFlags variable are
                            not valid

*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_EventSet( ADI_OSAL_EVENT_HANDLE const hEventGroup,
                                     ADI_OSAL_EVENT_FLAGS nEventFlags)
{
    return (OsalPostFlags(hEventGroup, nEventFlags, OS_FLAG_SET));
}



/*****************************************************************************
  Function: adi_osal_EventClear

  This service helps to  clear  event flag(s).

  Parameters:
      hEventGroup       - Handle of the event group to use
      nEventFlags       - Specifies the event flags to cleared.

  Returns:
      ADI_OSAL_SUCCESS    - If the event flag(s) are cleared successfully.
      ADI_OSAL_FAILED     - If failed to clear the event flags.
      ADI_OSAL_BAD_HANDLE - Invalid event flag group
  NOTE:
     There will be one eventgroup for the entire system. "ghEventGroupID"
     is a pointer to event group.
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_EventClear(ADI_OSAL_EVENT_HANDLE const hEventGroup, ADI_OSAL_EVENT_FLAGS nEventFlags)
{
    return (OsalPostFlags(hEventGroup, nEventFlags, OS_FLAG_CLR));
}




#pragma diag(pop)  /* global file supressions */

/*
**
** EOF: 
**
*/

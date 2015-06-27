/* $Revision: 3578 $
 * $Date: 2010-10-19 09:36:04 -0400 (Tue, 19 Oct 2010) $
******************************************************************************
Copyright (c), 2008-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Operating System Abstraction Layer - OSAL for uCOS - TLS functions

Description:
              This file contains the Thread Local Storage API's for the uCOS
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

#include <string.h>                                                             /* for strncpy */
#include <stdlib.h>                                                             /* for malloc/free */
#include <ccblkfn.h>                                                            /* for cli/sti */
#include <osal/adi_osal.h>
#include "osal_ucos.h"
#include "../include/osal_common.h"



#pragma diag(push)
/* FIXME : To be  removed once the bug is fixed in the compiler.exe "
   PR51984 -  MISRA Rule 17.4 on pre-release Compiler.
*/
/* #pragma diag(suppress:misra_rule_17_4:"Array indexing shall be the only allowed form  of pointer arithmetic") */
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

/* Rule-16.7 indicates that an argument which is a pointer argument should be
 * const if the pointer is not used to modify the object. This is a common API
 * even if the argument is not used in this implementation it could be in
 * others
 */
#pragma diag(suppress:misra_rule_16_7 : "sometimes the pointer argument is not used but cannot be declared const")


/*=============  D E F I N E S  =============*/

#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")




/*=============  D A T A  =============*/




/*=============  C O D E  =============*/


/*****************************************************************************
  Function: adi_osal_ThreadSlotAcquire

  This function allocate slot (i.e memory location of a word size ) from the
  thread local storage buffer and returns the slot number.

  Parameters:
      pnThreadSlotNum - Pointer to return the slot number if free slot is found.

  Returns:
      ADI_OSAL_SUCCESS  - successfully  allocated the slot.
      ADI_OSAL_FAILED   - Failed to allocate the slot.
      ADI_OSAL_CALLER_ERROR  -  If function is invoked from an invalid location (i.e an ISR)

  Note:
     "pnThreadSlotNum"  will be set to  ADI_OSAL_INVALID_THREAD_SLOT   if
      failed to find a free slot.
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_ThreadSlotAcquire(ADI_OSAL_TLS_SLOT_NUM     *pnThreadSlotNum,
                                            ADI_OSAL_TLS_CALLBACK_PTR pTerminateCallbackFunc)
{
    uint8_t  nSlotId;
    uint32_t nSlotMarker;
    uint32_t nTestSlot;
    ADI_OSAL_STATUS  eRetStatus = ADI_OSAL_FAILED;

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }
#endif /* OSAL_DEBUG */

    /* Lock the scheduler since it might be accessing from the other task */
    PAUSE_PROFILING();
    OSSchedLock();
    RESUME_PROFILING();

    /* search the work for a cleared bit */
    for (nSlotId=0u; nSlotId < _adi_osal_gnNumSlots; nSlotId++)
    {
        nSlotMarker = ((uint32_t) 1 << (nSlotId));
        nTestSlot = (_adi_osal_gaThreadLocalBuffStatusRegister & nSlotMarker);
        if ( nTestSlot == 0u)
        {
            /* a free slot has been found : set the bit in the state word and save the callback */
            _adi_osal_gaThreadLocalBuffStatusRegister |= nSlotMarker;
            _adi_osal_gaTLSCallbackTable[nSlotId] = pTerminateCallbackFunc;
            *pnThreadSlotNum = nSlotId;

            eRetStatus = ADI_OSAL_SUCCESS;
            break;
        }
    }
    if ( eRetStatus != ADI_OSAL_SUCCESS )
    {
        /* only write result in case of failure */
        *pnThreadSlotNum = ADI_OSAL_INVALID_THREAD_SLOT;
    }

    PAUSE_PROFILING();
    OSSchedUnlock();
    RESUME_PROFILING();

    return(eRetStatus);
}



/*****************************************************************************
  Function: adi_osal_ThreadSlotRelease

  This function frees the specified slot in the local storage buffer.
  This function mark corresponding bit in the global status register
  (which maintain the status of each slot) as free

  Parameters:
      nThreadSlotNum - slot number which need to be freed.
  Rturns:
      ADI_OSAL_SUCCESS           - successfully  freed the slot.
      ADI_OSAL_FAILED            - Tried to free a slot which was not allocated.
      ADI_OSAL_BAD_SLOT_NUM  - Specified slot number is invalid.
      ADI_OSAL_CALLER_ERROR  -  If function is invoked from an invalid location (i.e an ISR)
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_ThreadSlotRelease(ADI_OSAL_TLS_SLOT_NUM nThreadSlotNum)
{

    ADI_OSAL_STATUS eRetStatus;
    uint32_t nSlotState;
    uint32_t nSlotMarker;

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }
#endif /* OSAL_DEBUG */

    if(nThreadSlotNum < _adi_osal_gnNumSlots )
    {
        nSlotMarker = (uint32_t)1 << nThreadSlotNum;

        nSlotState = nSlotMarker & _adi_osal_gaThreadLocalBuffStatusRegister;
        /* Lock scheduler since other task may try to free same slot */
        PAUSE_PROFILING();
        OSSchedLock();
        RESUME_PROFILING();

        /* Check for the bit status : it should be set (allocated) */
        if(nSlotState == nSlotMarker)
        {
            _adi_osal_gaThreadLocalBuffStatusRegister &= ~(nSlotMarker);

            /* remove the callback from the callback table */
            _adi_osal_gaTLSCallbackTable[nSlotState] = NULL;

            eRetStatus = ADI_OSAL_SUCCESS;
        }
        else
        {
            eRetStatus = ADI_OSAL_FAILED;
        }

        PAUSE_PROFILING();
        OSSchedUnlock();
        RESUME_PROFILING();
    }
    else
    {
         eRetStatus =  ADI_OSAL_BAD_SLOT_NUM;
    }
    return(eRetStatus);
}



/*****************************************************************************
  Function: adi_osal_SetThreadSlotValue

  This function stores the a value in the specified slot number.


  Parameters:
      nThreadSlotNum - slot number of the Thread Local Buffer in
                       which "nSlotValue" to be stored.
      pSlotValue     - Value  to be stored.

  Returns:
      ADI_OSAL_SUCCESS           - successfully  stored the value in the specified slot.
      ADI_OSAL_FAILED            - Trying to  store the value in the slot which is not acquired
      ADI_OSAL_BAD_SLOT_NUM  - Specified slot number is invalid.
      ADI_OSAL_CALLER_ERROR  -  If function is invoked from an invalid location (i.e an ISR)

*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_ThreadSlotSetValue(ADI_OSAL_TLS_SLOT_NUM nThreadSlotNum,  ADI_OSAL_SLOT_VALUE  SlotValue)
{
    ADI_OSAL_STATUS eRetStatus;
    uint32_t nSlotState;
    ADI_OSAL_THREAD_HANDLE hCurrThread;
    ADI_OSAL_SLOT_VALUE (*aCurrThreadSlotArray)[];                              /* so that the pointer can be accessed as an array */
    ADI_OSAL_THREAD_INFO_PTR hThreadNode;

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }
#endif /* OSAL_DEBUG */

    if (nThreadSlotNum < _adi_osal_gnNumSlots)
    {
        adi_osal_ThreadGetHandle(&hCurrThread);
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a pointer to a useful structure")
        hThreadNode = (ADI_OSAL_THREAD_INFO_PTR) hCurrThread;
#pragma diag(pop)

        aCurrThreadSlotArray = hThreadNode->pThreadLocalStorageBuffer;

        /* Check for the status of slot */
        nSlotState  = (uint32_t)1 << nThreadSlotNum;
        nSlotState = nSlotState & _adi_osal_gaThreadLocalBuffStatusRegister;
        if ( nSlotState != 0u )
        {

#pragma diag(push)
#pragma diag(suppress:misra_rule_17_1 : "The variable cannot be defined as a finite array because the size is not know at compile time")
            (*aCurrThreadSlotArray)[nThreadSlotNum] = SlotValue;
#pragma diag(pop)

            eRetStatus = ADI_OSAL_SUCCESS;
        }
        else
        {
            eRetStatus = ADI_OSAL_FAILED;
        }
    }
    else
    {
         eRetStatus =  ADI_OSAL_BAD_SLOT_NUM;
    }
    return(eRetStatus);
}



/*****************************************************************************

  Function: adi_osal_ThreadSlotGetValue

  This function gets a value for the specified slot number of the
  current thread.

  Parameters:
      nThreadSlotNum -  slot number of the Thread Local Buffer from which
                        which the data need to retrieved.
      pSlotValue     - pointer to store the retrieved value from Thread Local
                        Storage.
  Returns:
      ADI_OSAL_SUCCESS      - successfully  retrieved data from the specified slot.
      ADI_OSAL_FAILED       - Trying to  retrieve data a slot which is not acquired.
      ADI_OSAL_BAD_SLOT_NUM - Specified slot number is invalid.
      ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid location (i.e an ISR)
  Note:
     "pnSlotValue"  will be set to ADI_OSAL_INVALID_THREAD_SLOT if
      failed to retrieve the data from the specified   slot.

*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_ThreadSlotGetValue(ADI_OSAL_TLS_SLOT_NUM nThreadSlotNum, ADI_OSAL_SLOT_VALUE *pSlotValue)
{
    ADI_OSAL_THREAD_HANDLE hCurrThread;
    ADI_OSAL_THREAD_INFO_PTR hThreadNode;

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        *pSlotValue = (ADI_OSAL_SLOT_VALUE) ADI_OSAL_INVALID_THREAD_SLOT;
        return (ADI_OSAL_CALLER_ERROR);
    }
#endif /* OSAL_DEBUG */

    adi_osal_ThreadGetHandle(&hCurrThread);

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a pointer to a useful structure")
    hThreadNode = (ADI_OSAL_THREAD_INFO_PTR) hCurrThread;
#pragma diag(pop)

    return( _adi_osal_ThreadSlotGetValueThread(hThreadNode, nThreadSlotNum, pSlotValue) );
}



/*****************************************************************************

  Function: _adi_osal_ThreadSlotGetValueThread

  This function gets a value for the specified slot number of a specific
  thread.

  Parameters:
      pThreadInfo    - pointer to the thread info structure
      nThreadSlotNum - slot number of the Thread Local Buffer from which
                       which the data need to retrieved.
      pSlotValue     - pointer to store the retrieved value from Thread Local
                        Storage.
  Returns:
      ADI_OSAL_SUCCESS      - successfully  retrieved data from the specified slot.
      ADI_OSAL_FAILED       - Trying to  retrieve data a slot which is not acquired.
      ADI_OSAL_BAD_SLOT_NUM - Specified slot number is invalid.
      ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid location (i.e an ISR)
  Note:
     "pnSlotValue"  will be set to ADI_OSAL_INVALID_THREAD_SLOT if
      failed to retrieve the data from the specified slot.

*****************************************************************************/

ADI_OSAL_STATUS  _adi_osal_ThreadSlotGetValueThread(ADI_OSAL_THREAD_INFO_PTR const pThreadInfo,
                                                    ADI_OSAL_TLS_SLOT_NUM nThreadSlotNum,
                                                    ADI_OSAL_SLOT_VALUE *pSlotValue)
{
    ADI_OSAL_STATUS eRetStatus;
    uint32_t nSlotState;
    ADI_OSAL_THREAD_HANDLE hCurrThread;
    ADI_OSAL_THREAD_INFO_PTR hThreadNode;
    ADI_OSAL_SLOT_VALUE (*aCurrThreadSlotArray)[];


    if(nThreadSlotNum < _adi_osal_gnNumSlots)
    {

        aCurrThreadSlotArray = pThreadInfo->pThreadLocalStorageBuffer;

        /* Check for the status of slot */
        nSlotState = (uint32_t) 1u << nThreadSlotNum;
        nSlotState= nSlotState & _adi_osal_gaThreadLocalBuffStatusRegister;

        if ( nSlotState != 0u )
        {

#pragma diag(push)
#pragma diag(suppress:misra_rule_17_1 : "The variable cannot be defined as a finite array because the size is not know at compile time")
            *pSlotValue =  (*aCurrThreadSlotArray)[nThreadSlotNum];
#pragma diag(pop)

             eRetStatus = ADI_OSAL_SUCCESS;
        }
        else /* slot not allocated */
        {
            eRetStatus = ADI_OSAL_FAILED;
            *pSlotValue = (ADI_OSAL_SLOT_VALUE) ADI_OSAL_INVALID_THREAD_SLOT;

        }
    }
    else /* invalid slot number */
    {
         eRetStatus =  ADI_OSAL_BAD_SLOT_NUM;
         *pSlotValue = (ADI_OSAL_SLOT_VALUE) ADI_OSAL_INVALID_THREAD_SLOT;
    }

    return(eRetStatus);
}



#pragma diag(pop)  /* global file supressions */

/*
**
** EOF: 
**
*/

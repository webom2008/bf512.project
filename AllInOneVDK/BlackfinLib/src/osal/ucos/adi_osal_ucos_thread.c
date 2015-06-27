/* $Revision: 5506 $
 * $Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $
******************************************************************************
Copyright (c), 2008-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Operating System Abstraction Layer - OSAL for uCOS - Thread functions

Description:
              This file contains the Thread API's for the uCOS implmentation of the OSAL

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
#pragma diag(suppress:misra_rule_17_4:"Array indexing shall be the only allowed form  of pointer arithmetic")
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
  Function: adi_osal_TaskCreate

  Creates a task and runs it.

  Parameters:
     pTaskAttr - Pointer to the (pre-initialized) ADI_OSAL_THREAD_ATTR structure
     phTaskID  - Pointer to a location to return the task handle if the task is
                  created successfully

  Returns:
      ADI_OSAL_SUCCESS              -  if task creation is successful.
      ADI_OSAL_FAILED               -  if task creation fails.
      ADI_OSAL_MAX_THREADS_EXCEEDED -  maximum number of tasks supported has been exceeded
      ADI_OSAL_PRIORITY_IN_USE      -  if a task already exists with the requested priority.
      ADI_OSAL_BAD_PRIORITY         -  if the specified priority is invalid.
      ADI_OSAL_CALLER_ERROR           -  If function is invoked from an invalid location (i.e an ISR)
      ADI_OSAL_BAD_STACK_ADDR       -  Stact base pointer is not word aligned.
*****************************************************************************/

ADI_OSAL_STATUS adi_osal_ThreadCreate(ADI_OSAL_THREAD_HANDLE *phThread, const ADI_OSAL_THREAD_ATTR *pThreadAttr)
{
    INT8U    nErr;
    INT8U    nRetValue;
#pragma diag(push)
#pragma diag(suppress:misra_rule_6_3 : "uCOS definition gives a MISRA error, in theory OS_STK should by INT32U and not 'unsigned int'")
    OS_STK   *pStackStart;
    OS_STK   *pStackTop;
#pragma diag(pop)
    uint32_t nStkSize;
    uint32_t nAlignment;
    ADI_OSAL_STATUS eRetStatus;
    ADI_OSAL_THREAD_INFO_PTR hThreadNode;
    uint32_t  nAssignedPrio;
    ADI_OSAL_STATUS eHeapResult;

#pragma diag(push)
#pragma diag(suppress:misra_rule_13_7 : "while checking compile time errors, having boolean operation that result is an invariant is necessary")
    /* check that typecasting pThreadAttr->pStackBase is valid */
    COMPILE_TIME_ASSERT((sizeof(void*)==sizeof(uint32_t)), pointer_size_mismatch);
#pragma diag(pop)

    if (phThread == NULL)
    {
        return(ADI_OSAL_FAILED);
    }

    /* verify that the given structure is not NULL before starting to access the fields */
    if (pThreadAttr == NULL)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        return(ADI_OSAL_FAILED);
    }

#ifdef OSAL_DEBUG
#pragma diag(push)
#pragma diag(suppress:misra_rule_1_1 : "typecasting is not allowed otherwise, notified compiler team")
    nAlignment = ((uint32_t) (pThreadAttr->pStackBase) & 0x3u);   /* an address is 32-bits on a Blackfin processor */
#pragma diag(pop)
    if((nAlignment) !=0u)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
         return(ADI_OSAL_BAD_STACK_ADDR);
    }

    if (CALLED_FROM_AN_ISR)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        return(ADI_OSAL_CALLER_ERROR);
    }

    if (pThreadAttr->pThreadFunc == NULL)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        return(ADI_OSAL_BAD_THREAD_FUNC);
    }

    if ((pThreadAttr->nStackSize == 0u) || ((pThreadAttr->nStackSize & 0x3u) != 0u))
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        return (ADI_OSAL_BAD_STACK_SIZE);
    }

    if (strlen(pThreadAttr->szThreadName) > ADI_OSAL_MAX_THREAD_NAME)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        return ( ADI_OSAL_BAD_THREAD_NAME);
    }
#endif /* OSAL_DEBUG */

/* It is assumed here that the OS is setup to have its stack to grow downward (OS_STK_GROWTH==1)
    which is the way it is setup in the Blackfin port.
    We also make the assumption that OS_STK is fixed and won't change on the user's machine which is true
    because it's also part of the Blackfin port */
#ifndef __ADSPBLACKFIN__
#warning "the following code makes some assumptions that the code runs on a blackfin because of its stack operations"
#endif
   /*Convert the stack size in bytes to elements of OS_STK */
    nStkSize = pThreadAttr->nStackSize/sizeof(OS_STK);


#pragma diag(push)
#pragma diag(suppress:misra_rule_17_1 : "Allow pointer arithmetic on non-array types")
    pStackStart = (OS_STK*) pThreadAttr->pStackBase;
    pStackTop =   &pStackStart[nStkSize-1u];
#pragma diag(pop)


    /*  OS reserve some priority level for itself. uCOS reserves
        priority level 0,1,2,3.  ADI_OSAL_UCOS_BASE_PRIO is defined to offset
        OS reserved priorities.
    */
    nAssignedPrio= pThreadAttr->nPriority + ADI_OSAL_UCOS_BASE_PRIO;

    PAUSE_PROFILING();
    /* Because the OSAL operates on thread handles rather than priority we need to maintain
       a mapping. This is done via the ADI_OSAL_THREAD_INFO structure.
       Create a object of type ADI_OSAL_THREAD_INFO which will serve as the handle */
       eHeapResult = _adi_osal_MemAlloc((void*) &hThreadNode, sizeof(ADI_OSAL_THREAD_INFO));
    RESUME_PROFILING();
    if (ADI_OSAL_SUCCESS != eHeapResult)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        return (ADI_OSAL_MEM_ALLOC_FAILED);
    }
    else
    {
        ADI_OSAL_SLOT_VALUE *pTLSBuffer;

        /* save information about the thread into its structure */
        hThreadNode->nThreadPrio = (uint8_t) nAssignedPrio;

        PAUSE_PROFILING();
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the local structure type in to a generic void** type required by malloc")
        eHeapResult = _adi_osal_MemAlloc((void**) &pTLSBuffer, sizeof(ADI_OSAL_SLOT_VALUE) * _adi_osal_gnNumSlots);
#pragma diag(pop)
        RESUME_PROFILING();
        if (ADI_OSAL_SUCCESS != eHeapResult )
        {
            *phThread = ADI_OSAL_INVALID_THREAD;
            
            /* Free the previously allocated memory because of a failure case */
            _adi_osal_MemFree(hThreadNode);
            return (ADI_OSAL_MEM_ALLOC_FAILED);
        }
        hThreadNode->pThreadLocalStorageBuffer = pTLSBuffer;
    }

    /* Creating a task  by calling native OS call.*/

    /* The scheduler needs to be locked so that the TaskCreation and the Name assignement
       can be done atomically. It is important because the OSTaskNameSet takes the priority
       as an argument, and the priority could have been changed within the task itself which
       starts right after OSTaskCreateExt is called */
    PAUSE_PROFILING();
    OSSchedLock();
    RESUME_PROFILING();


    PAUSE_PROFILING();
    /* the OSTCBExtPtr TCB entry (pext) is used for storing the pointer to the ADI_OSAL_THREAD_INFO structure so that
       it can be found when given the Thread Priority (uCOS task handle) */
    nRetValue = OSTaskCreateExt (pThreadAttr->pThreadFunc,
                                 pThreadAttr->pParam,
                                 pStackTop,
                                 (INT8U)  (nAssignedPrio & 0xFFu),
                                 (INT16U) nAssignedPrio,
                                 pStackStart,
                                 nStkSize,
                                 (void *)hThreadNode,
                                 (OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR) ) ;
    RESUME_PROFILING();

    switch (nRetValue)
    {
        case  OS_ERR_NONE:
            /* set the Name of the task. Maximum characters in the
              name of the task  allowed is "OS_TASK_NAME_SIZE".
              If it exceeds, it will be truncated to OS_TASK_NAME_SIZE*/
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting to the type expexted by uC/OS is necessary, also necessary for the return value")
#pragma diag(suppress:misra_rule_11_5 : "the API between the OSAL and uCOS force a const --> non-const conversion.")
/* converting from a const to non-const variable could be a risk, but since the data is getting 
   copied anyway it's not an issue here.*/
/* Supression since  typecasting "szTaskName" which is char_t to INT8U */
            PAUSE_PROFILING();
            OSTaskNameSet((INT8U) nAssignedPrio,(INT8U*) pThreadAttr->szThreadName, &nErr);
            RESUME_PROFILING();
            if(nErr == OS_ERR_NONE)
            {
                /* The ADI_OSAL_THREAD_INFO structure is used as the thread handle */
               *phThread = (ADI_OSAL_THREAD_HANDLE) hThreadNode;
                eRetStatus = ADI_OSAL_SUCCESS;
            }
            else
            {
                eRetStatus = ADI_OSAL_FAILED;
            }
            break;
#pragma diag(pop)

        /* Priority of the specified task is already exists */
        case  OS_ERR_PRIO_EXIST:
            eRetStatus= ADI_OSAL_PRIORITY_IN_USE;
            break;
        /* Specified priority is out of range */
        case  OS_ERR_PRIO_INVALID:
            eRetStatus = ADI_OSAL_BAD_PRIORITY;
            break;
        /* Not enough memory to create task */
        case  OS_ERR_TASK_NO_MORE_TCB:
            eRetStatus = ADI_OSAL_MEM_ALLOC_FAILED;
            break;
        default:
            eRetStatus = ADI_OSAL_FAILED;
            break;
    }

    PAUSE_PROFILING();
    OSSchedUnlock();
    RESUME_PROFILING();
    
    if(eRetStatus != ADI_OSAL_SUCCESS)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        if (hThreadNode != NULL)
        {
            _adi_osal_MemFree(hThreadNode->pThreadLocalStorageBuffer);
            _adi_osal_MemFree(hThreadNode);
        }
    }

    return(eRetStatus);
}



/*****************************************************************************
  Function: adi_osal_TaskDestroy


  To delete the task (hence can no longer be scheduled)

  Parameters:
     hTaskID - Handle of the task to be deleted

  Returns:
     ADI_OSAL_SUCCESS         -  If successfully removed the task from the system.
     ADI_OSAL_FAILED          -  If failed to delete the task
     ADI_OSAL_DESTROY_SELF    -  Attempt to destroy itself which is invalid
     ADI_OSAL_BAD_HANDLE      -  If the specified task ID is invalid
     ADI_OSAL_CALLER_ERROR      -  If function is invoked from an invalid location (i.e an ISR)

*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_ThreadDestroy(ADI_OSAL_THREAD_HANDLE const hThread)
{

        INT8U                   nErrCode;
        ADI_OSAL_THREAD_HANDLE  hCurrThread;
        ADI_OSAL_STATUS         eRetStatus;
        INT8U                   nPrioTask;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a useful type")
        ADI_OSAL_THREAD_INFO_PTR hThreadNode = (ADI_OSAL_THREAD_INFO_PTR) hThread;
#pragma diag(pop)



        eRetStatus = ADI_OSAL_FAILED;

#ifdef OSAL_DEBUG
        /* check validity of the handle */
        if ((hThreadNode==NULL) || (hThread==ADI_OSAL_INVALID_THREAD))
        {
            return (ADI_OSAL_BAD_HANDLE);
        }

        if (hThreadNode->pThreadLocalStorageBuffer==NULL)
        {
            return (ADI_OSAL_BAD_HANDLE);
        }
#endif

        eRetStatus = adi_osal_ThreadGetHandle(&hCurrThread);
        if (eRetStatus != ADI_OSAL_SUCCESS)
        {
            return (ADI_OSAL_FAILED);
        }

        if (hCurrThread == hThread)
        {
            return (ADI_OSAL_DESTROY_SELF);
        }

        nPrioTask = (INT8U) (hThreadNode->nThreadPrio);

        PAUSE_PROFILING();
        nErrCode= OSTaskDel(nPrioTask);
        RESUME_PROFILING();

        switch (nErrCode)
        {
            /* Deleted task successfully and mark the corresponding task */
            case  OS_ERR_NONE:

                /* TLS Destruction callback needs to be called */
                /* first, let's check if any slots have been allocated */
                if (_adi_osal_gaThreadLocalBuffStatusRegister != 0u)
                {
                    uint32_t i;

                    /* for each allocated slots, call the callback */
                    for (i=0u; i < ADI_OSAL_MAX_THREAD_SLOTS; i++)
                    {
#if (ADI_OSAL_MAX_THREAD_SLOTS>32u)
#error "ASSERT ERROR : ADI_OSAL_MAX_THREAD_SLOTS must be less than 32 because it is used as a bitmask to store TLS state"
#endif
                        /* loop through registered slots */
                        if ((_adi_osal_gaThreadLocalBuffStatusRegister & ((uint32_t) 1 << i)) != 0u)
                        {
                            ADI_OSAL_SLOT_VALUE  nSlotValue;
                            ADI_OSAL_STATUS      eRetVal;

                            /* NULL is valid and indicate to skip the callback */
                            if (NULL != _adi_osal_gaTLSCallbackTable[i])
                            {
                                /* Thread is allocated and callback is set, find the value in the slot */
                                /* here we need to use the function _adi_osal_ThreadSlotGetValueThread because we
                                   don't want the value from the current Thread */
                                eRetVal = _adi_osal_ThreadSlotGetValueThread(hThreadNode, i, &nSlotValue);
                                if (eRetVal == ADI_OSAL_SUCCESS)
                                {
                                    _adi_osal_gaTLSCallbackTable[i](nSlotValue);
                                }
                                /* else, slot value is not available : go to next slot */
                            } /* else, callback is set to NULL : just skip */
                        } /* else, slot not registered : ignore */
                    }
                } /* else, no slots at all have been registered, no TLS operations are necessary */

                /* free the ADI_OSAL_THREAD_INFO structure and the TLS Buffer */

                _adi_osal_MemFree(hThreadNode->pThreadLocalStorageBuffer);
                _adi_osal_MemFree(hThreadNode);

                eRetStatus =  ADI_OSAL_SUCCESS;
                break;

            /* Not to be called from ISR */
            case  OS_ERR_TASK_DEL_ISR:
                eRetStatus= ADI_OSAL_CALLER_ERROR;
                break;

            /* Invalid priority of the task */
            case OS_ERR_PRIO_INVALID:
            case OS_ERR_TASK_DEL:
            case OS_ERR_TASK_NOT_EXIST:
                eRetStatus= ADI_OSAL_BAD_HANDLE;
                break;

            case OS_ERR_TASK_DEL_IDLE:
            default:
                eRetStatus= ADI_OSAL_FAILED;
                break;

        }

        return(eRetStatus);
}



/*****************************************************************************
  Function: adi_osal_ThreadGetHandle

  Returns the handle of the current task.


  Parameters:
    phThread - pointer to a location to write the current thread
               handle upon successful.
               "phThread" will be set to "ADI_OSAL_INVALID_THREAD_ID"
               if not  successful.

  Returns:
    None

*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_ThreadGetHandle(ADI_OSAL_THREAD_HANDLE *phThread)
{
    if (OSRunning == OS_FALSE)
    {

/* a typecast is necessary here because the thread handle type is incomplete
 * and is just an abstract pointer, the real structure (the type of
 * _adi_osal_oStartupVirtualThread) cannot be used directly because it is
 * hidden from the public interface. */

#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4)
        *phThread  = (ADI_OSAL_THREAD_HANDLE) &_adi_osal_oStartupVirtualThread;
#pragma diag(pop)
        return (ADI_OSAL_SUCCESS);
    }

    /* NOTE: Initially this was implemented using the OSTaskQuery() function however, while profiling it, we noticed that
             calling that function takes 1400 cycles (?!) mostly due to memcopy. Since we only need one value from the TCB
             of the current task, we just need to access it directly.
             However IT DOES MAKE OUR CODE SUSCEPTIBLE TO INTERNAL MICRIUM BUG FIXES and backwards compatibility is not
             guaranteed.
             Also note that OSTCBCur is valid because if the OS is not running the previous condition is true
     */
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_3)
        /* WARNING: This is assuming that OS_TASK_CREATE_EXT_EN uCOS configuration is turned ON
                    the assumption is easily verified because the code won't link if it is turned off,
                    because OSTaskCreateExt will be defined out */
        *phThread  =(ADI_OSAL_THREAD_HANDLE)(OSTCBCur->OSTCBExtPtr);
#pragma diag(pop)

    return(ADI_OSAL_SUCCESS);
}



/*****************************************************************************
  Function: adi_osal_ThreadGetName

  Returns the name of the currently executing thread

  Parameters:
        none

  Returns:
      ADI_OSAL_FAILED       -  Not supported by this OS, or could not be enabled
      ADI_OSAL_SUCCESS      -  Successfully enabled
      ADI_OSAL_BAD_HANDLE   -  The specified thread handle is invalid.

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_ThreadGetName(ADI_OSAL_THREAD_HANDLE const hThread,
                                       char_t *pTaskName,
                                       uint32_t nNumBytesToCopy)
{
    char_t *sThreadName = NULL;
    INT8U nErr;
    INT8U nUcosNameSize;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a useful type")
        ADI_OSAL_THREAD_INFO_PTR hThreadNode = (ADI_OSAL_THREAD_INFO_PTR) hThread;
#pragma diag(pop)
    INT16U nUcosVersion = OSVersion();

#ifdef OSAL_DEBUG
    /* check validity of the handle */
    if ((hThreadNode==NULL) || (hThread==ADI_OSAL_INVALID_THREAD))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
    if (hThreadNode->pThreadLocalStorageBuffer==NULL)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }
#endif

   /* In version 2.92 or later the task stores a pointer and GetName returns it
    * so we don't  need to allocate memory for this variable because the uCOS
    * code does not copy the string to it, it simply sets the pointer 
    *
    * In older versions the maximum Task name size (OSTaskNameSize) is only set
    * in uCOS in debug mode so we cannot use it. We use UCHAR_MAX as the max
    * length because the return value is an INT8U.  
    *
    * Note that the prototype of OSTaskNameGet has changed and the pointer is
    * different.
    */

    #pragma diag(push)
    #pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the char_t type into INT8U type")
    if (nUcosVersion < 292u) 
    {

    /* allocate memory to get the task name based on the maximum task name size
     */

        if (ADI_OSAL_SUCCESS != _adi_osal_MemAlloc((void*) &sThreadName, UCHAR_MAX))
        {
            return (ADI_OSAL_MEM_ALLOC_FAILED);
        }
        PAUSE_PROFILING();
        nUcosNameSize = OSTaskNameGet(hThreadNode->nThreadPrio, 
                                      (void*) sThreadName, &nErr);
        RESUME_PROFILING();

     }
     else
     {
        PAUSE_PROFILING();
        nUcosNameSize = OSTaskNameGet(hThreadNode->nThreadPrio, 
                                      (void*) &sThreadName, &nErr);
        RESUME_PROFILING();
     }

#pragma diag(pop)

    switch(nErr)
    {
        case OS_ERR_NONE:
            strncpy(pTaskName, sThreadName, nNumBytesToCopy - 1u);
            /* We zero-terminate the string in either the last element or 
               where the uCOS name finished
             */
            if (nUcosNameSize < (nNumBytesToCopy) )
            {
                pTaskName[nUcosNameSize + 1u] ='\0';

            }
            else
            {
                pTaskName[nNumBytesToCopy] ='\0';
            }

            eRetStatus = ADI_OSAL_SUCCESS;
            break;
        case OS_ERR_TASK_NOT_EXIST:
        case OS_ERR_PRIO_INVALID:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;
        case OS_ERR_NAME_GET_ISR:
            eRetStatus = ADI_OSAL_CALLER_ERROR;
            break;
        case OS_ERR_PNAME_NULL:
        default:
            eRetStatus = ADI_OSAL_FAILED;
            break;
    }

    if (nUcosVersion < 292u) 
    {
        _adi_osal_MemFree(sThreadName);
    }

    return (eRetStatus);
}



/*****************************************************************************
  Function: adi_osal_ThreadEnableCycleCounting

  Start counting processor cycles spent in each thread

  Parameters:
        none

  Returns:
      ADI_OSAL_FAILED       -  Not supported by this OS, or could not be enabled
      ADI_OSAL_SUCCESS      -  Successfully enabled

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_ThreadEnableCycleCounting(void)
{
    /* Thread cycle counting is supported by uC/OS but it is not available in
    the Blackfin port */
    return(ADI_OSAL_FAILED);
}



/*****************************************************************************
  Function: adi_osal_ThreadGetCycleCount

  Start counting processor cycles spent in each thread

  Parameters:
        none

  Returns:
      ADI_OSAL_FAILED       -  Not supported by this OS, or could not be enabled
      ADI_OSAL_SUCCESS      -  Successfully enabled

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_ThreadGetCycleCount(uint64_t *pnCycles)
{
    /* Thread cycle counting is supported by uC/OS but it is not available in
    the Blackfin port */

    pnCycles=pnCycles;      /* avoid misra error about argument not used */
    return(ADI_OSAL_FAILED);
}



/*****************************************************************************
  Function: adi_osal_ThreadGetPrio

  Returns the priority of a given thread.

  Parameters:
      pnTaskPrio - pointer to a location to write the thread priority.

  Returns:
      ADI_OSAL_FAILED       -  If failed to return the priority of the current task
      ADI_OSAL_SUCCESS      -  If successfully returns priority of the current task
      ADI_OSAL_BAD_HANDLE   -  If the specified thread handle is invalid

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_ThreadGetPrio(ADI_OSAL_THREAD_HANDLE const hThread, ADI_OSAL_PRIORITY *pnThreadPrio)
{
        OS_TCB          CurrTCB;
        ADI_OSAL_STATUS eRetStatus;
        INT8U           nPrio;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a useful type")
        ADI_OSAL_THREAD_INFO_PTR hThreadNode = (ADI_OSAL_THREAD_INFO_PTR) hThread;
#pragma diag(pop)

        /*  Not allowed from an ISR */
#ifdef OSAL_DEBUG
        if (CALLED_FROM_AN_ISR)
        {
            *pnThreadPrio = (ADI_OSAL_PRIORITY) ADI_OSAL_INVALID_PRIORITY;
            return (ADI_OSAL_CALLER_ERROR);
        }
        /* check validity of the handle */
        if ((hThreadNode==NULL) || (hThread==ADI_OSAL_INVALID_THREAD))
        {
            return (ADI_OSAL_BAD_HANDLE);
        }
        if (hThreadNode->pThreadLocalStorageBuffer==NULL)
        {
            return (ADI_OSAL_BAD_HANDLE);
        }
#endif /* OSAL_DEBUG */

        /* the priority is stored within the local structure, so it is not necessary to access the
          TCB in order to get the priority (maybe it could be done as a debug step)? */
        nPrio  = hThreadNode->nThreadPrio;

        if (nPrio >=  (INT8U) ADI_OSAL_UCOS_BASE_PRIO)
        {
             nPrio = nPrio - (INT8U) ADI_OSAL_UCOS_BASE_PRIO;
             *pnThreadPrio = nPrio;
             return (ADI_OSAL_SUCCESS);
        }
        *pnThreadPrio = (ADI_OSAL_PRIORITY) ADI_OSAL_INVALID_PRIORITY;
        return (ADI_OSAL_FAILED);
}



/*****************************************************************************
  Function: adi_osal_ThreadSetPrio

  To change the priority of the specified task.

  Parameters:
      hThread       - Handle of the thread whose priority is to be changed.
      nNewPriority  - New desired priority.


  Returns:
      ADI_OSAL_SUCCESS          -  If successfully changed the priority of the
                                   specified task
      ADI_OSAL_FAILED           -  If failed to change the priority of the task
      ADI_OSAL_PRIORITY_IN_USE  -  If failed due to presence of a task with the
                                   specified priority.
      ADI_OSAL_BAD_PRIORITY     -  Failed because specified priority is invalid
      ADI_OSAL_CALLER_ERROR     -  If function is invoked from an invalid location (i.e an ISR)

*****************************************************************************/

/* MISRA RULE 16.7 is supressed to have a common API. i.e
 * Content of hTaskID may be modified in other OS */

#pragma diag(push)
#pragma diag(suppress:misra_rule_16_7)
ADI_OSAL_STATUS adi_osal_ThreadSetPrio(ADI_OSAL_THREAD_HANDLE const hThread, ADI_OSAL_PRIORITY nNewPriority)
{
    INT8U           nNewPrio;
    INT8U           nOldPrio;
    INT8U           nRetValue;
    ADI_OSAL_STATUS eRetStatus;
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a useful type")
    ADI_OSAL_THREAD_INFO_PTR hThreadNode = (ADI_OSAL_THREAD_INFO_PTR) hThread;
#pragma diag(pop)


#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }
#endif /* OSAL_DEBUG */

#ifdef OSAL_DEBUG
        /* check validity of the handle */
        if ((hThreadNode==NULL) || (hThread==ADI_OSAL_INVALID_THREAD))
        {
            return (ADI_OSAL_BAD_HANDLE);
        }
        if ((hThreadNode->pThreadLocalStorageBuffer==NULL) ||
            (hThreadNode->nThreadPrio < ADI_OSAL_UCOS_BASE_PRIO) )
        {
            return (ADI_OSAL_BAD_HANDLE);
        }
#endif

    nOldPrio = (INT8U) hThreadNode->nThreadPrio;
    nNewPrio = (INT8U) (nNewPriority + ADI_OSAL_UCOS_BASE_PRIO);

    if ((nNewPrio <  (INT8U) ADI_OSAL_UCOS_BASE_PRIO))
    {
        return(ADI_OSAL_BAD_PRIORITY);
    }

    PAUSE_PROFILING();
    nRetValue= OSTaskChangePrio(nOldPrio, nNewPrio);
    RESUME_PROFILING();
    switch (nRetValue)
    {
        case  OS_ERR_NONE:
            /* update local structure */
            hThreadNode->nThreadPrio = nNewPrio;
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

        case OS_ERR_PRIO_EXIST:
            eRetStatus = ADI_OSAL_PRIORITY_IN_USE;
            break;
#ifdef OSAL_DEBUG
        case OS_ERR_PRIO_INVALID:
            eRetStatus = ADI_OSAL_BAD_PRIORITY;
            break;
        case OS_ERR_PRIO:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;
#endif /* OSAL_DEBUG */
        default :
            eRetStatus = ADI_OSAL_FAILED;
            break;
    }
    return(eRetStatus);
}
#pragma diag(pop)   /* End of MISRA supression 16.7 */



/*****************************************************************************
  Function: adi_osal_TaskSleep

  function description:
      Put calling task to sleep for specified time in system ticks.

  Parameters:
      nTimeInTicks - Specify the amount of time in system ticks.

  Returns:
      ADI_OSAL_CALLER_ERROR  -  If function is invoked from an invalid location (i.e an ISR)
      ADI_OSAL_SUCCESS - If successfully completed the "sleep" period.
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_ThreadSleep(ADI_OSAL_TICKS nTimeInTicks)
{
        ADI_OSAL_STATUS eRetStatus;

        if (nTimeInTicks == 0u)
        {
            return ADI_OSAL_SUCCESS;
        }

#ifdef OSAL_DEBUG
        if ((CALLED_FROM_AN_ISR) || (CALLED_IN_SCHED_LOCK_REGION))
        {
            return (ADI_OSAL_CALLER_ERROR);
        }
#endif /* OSAL_DEBUG */

        if((nTimeInTicks <= OSAL_MAX_TIMEOUT))
        {
            PAUSE_PROFILING();
            OSTimeDly((INT16U) nTimeInTicks);
            RESUME_PROFILING();

            eRetStatus = ADI_OSAL_SUCCESS;
        }
        else
        {
             eRetStatus = ADI_OSAL_BAD_TIME;
        }
        return(eRetStatus);
}




#pragma diag(pop)  /* global file supressions */

/*
**
** EOF: 
**
*/

/*
***************************************************************************
Copyright (c), 2009-2010 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
***************************************************************************

Title:   UCOS-specific definitions

         This file contains the uCOS specific defines.

*****************************************************************************/

#ifndef __OSAL_UCOS_H__
#define __OSAL_UCOS_H__


#ifdef _MISRA_RULES
#pragma diag(push)
/* Rule-5.1 indicates that all identifiers shall not rely on more than 31 characters of significance */
#pragma diag(suppress:misra_rule_5_1:"prefixes added in front of identifiers quickly increases their size. In order to keep the code self explanatory, and since ADI tools are the main targeted tools, this restriction is not really a necessity")
#endif /* _MISRA_RULES */


/*=============  I N C L U D E S   =============*/

#include <adi_types.h>
#include <services/services.h>													/* for Memory Manager */
#include "../include/osal_common.h"

/*==============  D E F I N E S  ===============*/
#if defined(_LANGUAGE_C)


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
  Patch version of  OSAL for uCOS.  Please refer "adi_osal.h" for
  ADI_OSAL_MAJOR_VER and ADI_OSAL_MINOR_VER since these fields are
  also used for the code-base version
*/
#define ADI_OSAL_PATCH_VER              3u


#ifdef DEBUG
/* in case we ever want to turn on the checks in 'release' mode */
#define OSAL_DEBUG
#endif /* DEBUG */


/* Semaphores are used in place of Mutexes in this implementation */
#define INIT_COUNT_FOR_EMULATED_MUTEX   1u

/* Build option to determine if the memory manager will be built in or if only the default malloc/free will be used
   Set to 0 to disable the memory manager.
   NOTE that this is a BUILD option. Once the library is built, it cannot be changed.
   By default set it to 1 */
#define BUILD_WITH_SSL_MEM_MGR          1


/*
    Defines the minimum number of bytes that the user has to pass during OSAL Initialization
    to create the OSAL heap.
    For now in uCOS, there are no real requirements, so we just check that it is not 0
*/
#define OSAL_MINIMUM_HEAP_SIZE          ((uint32_t) 0u)


 /* "OSIntNesting" is incremented before entering an ISR and decremented after
    executing it. If its value is greater than '0', it means that the processor is
    running in an ISR context. */
#define CALLED_FROM_AN_ISR              (OSIntNesting != 0u)

/* The scheduler and critical region locks counters can be used to verify if the user calls a function that can potentially
   create a deadlock situation */

#define CALLED_IN_SCHED_LOCK_REGION     ((_adi_osal_gnSchedulerLockCnt != 0u) || (_adi_osal_gnCriticalRegionNestingCnt != 0))


/*
    uC/OS reserves the first 3 priority levels, so we offset them in the OSAL
    (see note #3 in the uCOS Reference Manual under OSTaskCreate)
*/
#define ADI_OSAL_UCOS_BASE_PRIO         3u

/*
    uC/OS maximum lowest priority is 255 (as of v2.85). It is really defined by the build option OS_LOWEST_PRIO.
    This macro is defined for error checking purposes
*/
#define UCOS_ABSOLUTE_MAXIMUM_PRIORITY  (0xFFu)


/* 
    Timeout fields are 16-bit number, so the maximum timeout is the maximum 16-bit number range
*/
#define OSAL_MAX_TIMEOUT           (OSAL_MAX_UINT16)


/*
    The current code has been developed and tested with uC/OS version 2.86
*/
#define COMPATIBLE_OS_VERSION           (286u)


/*  Structure:  ADI_OSAL_THREAD_INFO
    This structure provides a mapping between thread handle and thread priority
    it also stores the thread local storage since uCOS does not provide it.
*/
typedef struct __AdiOsalThreadInfo
{
    /* Variable : pThreadLocalStorageBuffer
       Pointer to the Thread Local Storage */
    void  *pThreadLocalStorageBuffer;

    /* Variable : nThreadPrio
       Stores the thread  priority (since it can change), ucos only support 255 priorities, so 8 bits are enough */
    uint8_t nThreadPrio;
} ADI_OSAL_THREAD_INFO, *ADI_OSAL_THREAD_INFO_PTR;


typedef struct __AdiOsalMutexInfo
{
    /* Variable : hMutexID
       Stores the mutex  handle return from uCOS
    */
    OS_EVENT*               hMutexID;

    /* Variable : hOwnerThread
       Holds the task handle which acquires the mutex. This is a OS_TCB* 
       because we don't know whether the thread was created with OSAL or not.
    */
    OS_TCB*         hOwnerThread;

    /* Variable : nAcquisitionCount
       counter for mutex ownership.
    */
    uint32_t                nAcquisitionCount;

    /* Variable : bDynAlloc
        indicates if this structure has been dynamically allocated.
    */
    bool                    bDynAlloc;

} ADI_OSAL_MUTEX_INFO, *ADI_OSAL_MUTEX_INFO_PTR;



/*
    This definition is used to allocate enough memory for the worth case TCB structure size
    The 20 words have been computed using the worth case definition based on ucos v2.85
    This value is verified in the Init function.
*/
#define ADI_OSAL_UCOS_MAX_TCB_SIZE          ((uint16_t) (20u*4u + (uint16_t) ADI_OSAL_MAX_THREAD_NAME))



/*  Utility Macro to do compile time assertion */
#pragma diag(push)
#pragma diag(suppress:misra_rule_19_13:"## is used so that identifier names are unique")
#pragma diag(suppress:misra_rule_19_7:"The gains from this compile time check macro outweight the suppression of the Adv rules ")
#pragma diag(suppress:misra_rule_19_4:"In this case we don't want this to take space in the code, so it is defined as a typedef")

#define COMPILE_TIME_ASSERT(condition, name) typedef uint8_t name##_failed[(condition)?1:-1]

/* the do ... while(0) construct is a misra requirement from Misra Rule 19.4 */
#if defined(PROFILING) && (PROFILING==1)
#define PAUSE_PROFILING()  do { asm volatile ("%0 = syscfg; bitclr(%0, 1); syscfg = %0;" ::"?d"(0)); } while(0)
#define RESUME_PROFILING() do { asm volatile ("%0 = syscfg; bitset(%0, 1); syscfg = %0;" ::"?d"(0)); } while(0)
#else
#define PAUSE_PROFILING()   do {} while(0)
#define RESUME_PROFILING()  do {} while(0)
#endif

#pragma diag(pop)


/*=============  E X T E R N A L S  =============*/

/* Local global variables that are shared across files */
extern uint32_t _adi_osal_gnNumSlots;
extern uint32_t _adi_osal_gaThreadLocalBuffStatusRegister;
extern ADI_OSAL_TLS_CALLBACK_PTR _adi_osal_gaTLSCallbackTable[ADI_OSAL_MAX_THREAD_SLOTS];
extern uint32_t _adi_osal_gnTickPeriod;
extern ADI_OSAL_ISR_INFO _adi_osal_gIsrInfoTable[ADI_MAX_NUM_IVG_LEVELS];
extern ADI_OSAL_THREAD_INFO _adi_osal_oStartupVirtualThread;
extern uint32_t _adi_osal_gnSchedulerLockCnt;
extern bool _adi_osal_gbSchedulerActive;
extern int32_t _adi_osal_gnCriticalRegionNestingCnt;
extern uint32_t _adi_osal_gnCriticalRegionState;


/* Definitions of the uC/OS debug variables */
extern INT16U const OSDebugEn;
extern INT16U const OSLowestPrio;
extern INT16U const OSTicksPerSec;
extern INT16U const OSStkWidth;
extern INT16U const OSTaskNameSize;
extern INT16U const OSTCBSize;
extern INT16U const OSFlagWidth;


/* external code */
extern  void   _adi_osal_UnregisteredInterrupt(void);
extern  void*  _adi_osal_apNonNestedEvtAddressbuffer[ADI_MAX_NUM_IVG_LEVELS];
extern  void*  _adi_osal_apNestedEvtAddressbuffer[ADI_MAX_NUM_IVG_LEVELS];
extern  ADI_OSAL_STATUS  _adi_osal_MemAlloc(void** ppData, uint32_t nSize);
extern  void   _adi_osal_MemFree(void* pData);
extern  ADI_OSAL_STATUS _adi_osal_ThreadSlotGetValueThread(ADI_OSAL_THREAD_INFO_PTR const pThreadInfo,
                                                           ADI_OSAL_TLS_SLOT_NUM nThreadSlotNum,
                                                           ADI_OSAL_SLOT_VALUE *pSlotValue);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#else /* assembly language specific macros and declarations*/


#endif  /* if !defined(_LANGUAGE_C) */


#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */



#endif /*__OSAL_UCOS_H__ */

/*
**
** EOF: 
**
*/

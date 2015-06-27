/* $Revision: 3578 $
 * $Date: 2010-10-19 09:36:04 -0400 (Tue, 19 Oct 2010) $
***************************************************************************
Copyright (c), 2009-2010 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
***************************************************************************

Title:   specific definitions non OS environment.

         This file contains the non OS specific definitions.

*****************************************************************************/
#ifndef __OSAL_NOOS_H__
#define __OSAL_NOOS_H__


/*=============  I N C L U D E S   =============*/


#if defined(__GNUC__) || defined(_LANGUAGE_C)

#pragma diag(push)
#pragma diag(suppress:misra_rule_5_1:"This module gains clarity by using some identifier names longer than 31 characters.")


#include <adi_types.h>
#include <osal/adi_osal.h>
#include <services/services.h>													/* for Memory Manager */
#include "../include/osal_common.h"
#include <ccblkfn.h>                                                            /* for cli/sti */



/*==============  D E F I N E S  ===============*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
  Patch version of  OSAL for uCOS.  Please refer "adi_osal.h" for
  ADI_OSAL_MAJOR_VER and ADI_OSAL_MINOR_VER since these fields are
  also used for the code-base version
*/
#define ADI_OSAL_PATCH_VER                      2u


#ifdef DEBUG
/* in case we ever want to turn on the checks in 'release' mode */
#define OSAL_DEBUG
#endif /* DEBUG */


/* Build option to determine if the memory manager will be built in or if only the default malloc/free will be used
   Set to 0 to disable the memory manager.
   NOTE that this is a BUILD option. Once the library is built, it cannot be changed.
   By default set it to 1 */
#define BUILD_WITH_SSL_MEM_MGR                   1

/*
    Defines the minimum number of bytes that the user has to pass during OSAL Initialization
    to create the OSAL heap.
    For now in no-OS, there are no real requirements, so we just check that it is not 0
*/
#define OSAL_MINIMUM_HEAP_SIZE               (0ul)

/*
    This preprocessor constant is used to indicate is the OSAL has been initialized.
   This is used instead of boolean flag in order to minimize the risk of match when
   the memory is uninitialized after processor boot
*/
#define OSAL_INITIALIZED                (0xAD10ul)


#pragma diag(push)
#pragma diag(suppress:misra_rule_19_7:"Inline function doesn't work very well here.")

#define ENTER_CRITICAL_INTERNAL(var)    do { (var) = cli(); } while(0)
#define EXIT_CRITICAL_INTERNAL(var)     do { sti(var); } while(0)

#pragma diag(pop)



/*******************************************************************************

       Data Structures semaphore management.

 ********************************************************************************/

typedef struct osal_sem_semaphore {
    bool               bPendingS;           /* Indicate if a function is pending on the semaphore. */
    bool               bActiveS;            /* Indicates if the semaphore is active, this avoid operations on deleted semaphores */
    volatile uint32_t  nCount;             /* count for the semaphore  */
} OSAL_SEMAPHORE, *OSAL_SEMAPHORE_PTR;


/* external data (shared across OSAL files) */
extern  uint32_t     _adi_osal_gnSchedulerLockCnt;
extern  int32_t      _adi_osal_gnCriticalRegionNestingCnt;
extern  uint32_t     _adi_osal_gnCriticalRegionState;
extern  uint32_t     _adi_osal_gnTickPeriod;
extern  uint32_t     _adi_osal_gnTickCnt;
extern  uint32_t     _adi_osal_gnNumSlots;
extern  uint32_t     _adi_osal_gaThreadLocalBuffStatusRegister;
extern  ADI_OSAL_TLS_CALLBACK_PTR _adi_osal_gaTLSCallbackTable[ADI_OSAL_MAX_THREAD_SLOTS];
extern  ADI_OSAL_SLOT_VALUE       _adi_osal_gaTLSSlotTable[ADI_OSAL_MAX_THREAD_SLOTS];
extern  ADI_OSAL_ISR_INFO         _adi_osal_gIsrInfoTable[ADI_MAX_NUM_IVG_LEVELS];

/* external code (shared across OSAL files) */
extern  ADI_OSAL_STATUS  _adi_osal_MemAlloc(void** ppData, uint32_t nSize);
extern  void   _adi_osal_MemFree(void* pData);
extern  void*  _adi_osal_apNonNestedEvtAddressbuffer[ADI_MAX_NUM_IVG_LEVELS];
extern  void*  _adi_osal_apNestedEvtAddressbuffer[ADI_MAX_NUM_IVG_LEVELS];
extern  void _adi_osal_UnregisteredInterrupt( void );
extern  bool _adi_osal_AcquireGlobalLock( void );
extern  void _adi_osal_ReleaseGlobalLock( void );
extern  bool _adi_osal_IsCurrentLevelISR( void );


#ifdef __cplusplus
}
#endif /* __cplusplus */

#pragma diag(pop)

#else /* assembly language specific macros and declarations*/

#endif  /* if !defined(__GNUC__) && !defined(_LANGUAGE_C) */

#endif /*__OSAL_NOOS_H__ */
/*
**
** EOF: 
**
*/


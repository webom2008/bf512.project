/* $Revision: 3916 $
 * $Date: 2010-12-06 11:39:55 -0500 (Mon, 06 Dec 2010) $
******************************************************************************
Copyright (c), 2008-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Operating System Abstraction Layer - OSAL for no-OS - Mutex functions

Description:
              This file contains the Mutex API's for the no OS implementation of the OSAL


IMPORTANT NOTE: 

  Mutexes in the No-OS implementation assume that the "thread" is running at 
  level 15. Since only the mutex owner can clear the mutex, and that pending is 
  only allowed in non-ISR levels, it comes dow that only level15 can use it. 
  That makes the Mutex functionality irrelevant. 
  So mutexes are implemented as all returning success.

  ASSUMPTIONS:
    - all levels above level 15 are considered ISRs
    - only non-ISR levels can do mutex operations (so only L15)
  

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


/* since returning NULL means error, this value is used to define a "valid" mutex handle. */
#define DUMMY_MUTEX_HANDLE  ((ADI_OSAL_MUTEX_HANDLE) 0xAD10ul)

/*=============  D A T A  =============*/




/*=============  C O D E  =============*/



/*****************************************************************************

  Function: adi_osal_MutexCreate

    Return successful mutex allocation, since the return value of NULL means 
    allocation error, another value must be chosen

  Parameters:
      phMutexID  - Pointer to a location to write the returned mutex handle

  Returns:
      ADI_OSAL_SUCCESS      - If mutex is created successfully (always)

  Note:
      ASSUMPTIONS:
        - all levels above level 15 are considered ISRs
        - only non-ISR levels can do mutex operations (so only L15)
  


 *****************************************************************************/
ADI_OSAL_STATUS adi_osal_MutexCreate(ADI_OSAL_MUTEX_HANDLE *phMutex)
{
     return (adi_osal_MutexCreateStatic(NULL, 0u, phMutex));
}



/*****************************************************************************
  Function: adi_osal_MutexCreateStatic

    (See comment for adi_osal_MutexCreate)

  Parameters:
      pMutexObject - adresss of a memory location to use to create a mutex.
      nMutexIBjSize - size of the pMutexObject given
      phMutexID  - Pointer to a location to write the returned mutex handle

  Returns:
      ADI_OSAL_SUCCESS      - If mutex is created successfully (always)
    
 *****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexCreateStatic(void* const pMutexObject, uint32_t nMutexObjSize, ADI_OSAL_MUTEX_HANDLE *phMutex)
{
#ifdef OSAL_DEBUG
    if(_adi_osal_IsCurrentLevelISR())
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return ADI_OSAL_CALLER_ERROR;
    }
#endif  /* OSAL_DEBUG */
    
    *phMutex = DUMMY_MUTEX_HANDLE;
    return ADI_OSAL_SUCCESS;
}


/*****************************************************************************
  Function: adi_osal_MutexGetMutexObjSize

  This function returns the size of a mutex object. It can be used by the adi_osal_MutexCreateStatic
  function to determine what the object size should be for this particular RTOS implementation.

  Parameters:
      None

  Returns:
      size of a mutex object in bytes, here 0.

*****************************************************************************/

uint32_t adi_osal_MutexGetMutexObjSize(void)
{
    return ( 0ul );
}



/*****************************************************************************
  Function: adi_osal_MutexDestroy

  This function is used to delete a mutex. 

  Parameters:
      hMutexID      - Handle of the mutex to be deleted

  Returns:
      ADI_OSAL_SUCCESS      - If mutex is deleted successfully
      ADI_OSAL_BAD_HANDLE   - If the specified mutex handle is invalid
      ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid location

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexDestroy(ADI_OSAL_MUTEX_HANDLE const hMutex)
{
#ifdef OSAL_DEBUG
    if(_adi_osal_IsCurrentLevelISR())
    {
        return ADI_OSAL_CALLER_ERROR;
    }

    if ((hMutex == NULL) || (hMutex == ADI_OSAL_INVALID_MUTEX ))
    {
        return ADI_OSAL_BAD_HANDLE;
    }
#endif  /* OSAL_DEBUG */

    return  ADI_OSAL_SUCCESS ;

}



/*****************************************************************************
  Function: adi_osal_MutexDestroyStatic

  This function is used to delete a mutex that has being created by 
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
    return (adi_osal_MutexDestroy(hMutex));    
}




/*****************************************************************************
  Function: adi_osal_MutexPend

  This function is used to lock a mutex (acquire a resource), always succeeds 
  in release mode.

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

  Returns:
      ADI_OSAL_SUCCESS      - If the specified mutex is locked successfully
      ADI_OSAL_BAD_HANDLE   - If the specified mutex ID is invalid
      ADI_OSAL_TIMEOUT      - If the specified time limit expired.
      ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid location

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexPend(ADI_OSAL_MUTEX_HANDLE const hMutex, ADI_OSAL_TICKS nTimeoutInTicks)
{

#ifdef OSAL_DEBUG
    if (_adi_osal_IsCurrentLevelISR())
    {
        return ADI_OSAL_CALLER_ERROR;
    }

    if ((hMutex == NULL) ||
        (hMutex == ADI_OSAL_INVALID_MUTEX ))
    {
        return ADI_OSAL_BAD_HANDLE;
    }
#endif /* OSAL_DEBUG */

    return ADI_OSAL_SUCCESS;
}



/*****************************************************************************
  Function: adi_osal_MutexPost

  This function is used to unlock a mutex, always succeeds 
  in release mode.

  Parameters:
      hMutexID      - Handle of the mutex which needs to be unlocked

  Returns:
      ADI_OSAL_SUCCESS          - If mutex is un locked successfully
      ADI_OSAL_BAD_HANDLE       - If the specified mutex ID is invalid
      ADI_OSAL_CALLER_ERROR       - The function is invoked from an invalid location

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexPost(ADI_OSAL_MUTEX_HANDLE const hMutex)
{

#ifdef OSAL_DEBUG
    if (_adi_osal_IsCurrentLevelISR())
    {
        return ADI_OSAL_CALLER_ERROR;
    }

    if ((hMutex == NULL) ||
        (hMutex == ADI_OSAL_INVALID_MUTEX ))
    {
        return ADI_OSAL_BAD_HANDLE;
    }

#endif /* OSAL_DEBUG */

    return ADI_OSAL_SUCCESS;
}



#pragma diag(pop)  /* global file supressions */

/*
**
** EOF: 
**
*/


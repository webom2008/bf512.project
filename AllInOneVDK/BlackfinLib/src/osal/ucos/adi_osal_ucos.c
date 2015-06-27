/* $Revision: 5506 $
 * $Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $
******************************************************************************
Copyright (c), 2008-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Operating System Abstraction Layer - OSAL for uCOS

Description:
              This file contains the API's designed to abstract uCOS the
              operating system from the user.

NOTE:
              All typedef with upper case (like INT8U,INT16U,INT32U) are uCOS
              specific. These typedefs are typically used for the variable
              which are passed as arguments to the OS function OR used for
              storing return value.

SUPRESSED MISRA RULES ARE:

            1.1  [R]    :  "All code shall conform to ISO 9899:1990"
            2.1  [R]    :  "Assembly language shall be encapsulated and isolated"
            5.1  [R]    :  "Identifier shall not rely on the significance of more than 31 characters"
            5.7  [A]    :  "No identifier name should be reused"
            11.1 [R]    :  "Conversions shall not be performed between a pointer to function and
                            any type other than an integral type"
            11.3 [A]    :  "A cast should not be performed between a pointer type and an integral type"
            11.4 [A]    :  "A cast should not be performed between a pointer to object type and a
                            different pointer to object type"
            14.7 [R]    :  "a function shall have a single point of exit at the end of the function"
            16.7 [A]    :  "A pointer parameter in a function prototype should be declared as
                            pointer to const if the pointer is not used to modify the addressed
                            object."
            17.4 [R]    :  "Array indexing shall be the only allowed form of pointer arithmetic"
            17.5 [A]    :  "The declaration of objects should contain no more than 2 levels of pointer indirection"



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
#include "os_cpu.h"
#ifdef _MISRA_RULES
#pragma diag(pop)

#pragma diag(suppress:misra_rule_20_8:"This source needs to use signals")
#pragma diag(suppress:misra_rule_20_4:"This source needs to use malloc")
#endif /*  _MISRA_RULES */

#include <string.h>                                                             /* for strncpy */
#include <stdlib.h>                                                             /* for malloc/free */
#include <ccblkfn.h>                                                            /* for cli/sti */
#include <signal.h>                                                             /* for raise */
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


/* "version.h" contain macro "ADI_BUILD_VER"  which gives the
    build version for OSAL for uCOS. This is a generated file.
*/
#include "version.h"

/*=============  D E F I N E S  =============*/

#pragma diag(push)
#pragma diag(suppress:misra_rule_13_7 : "while checking compile time errors, having boolean operation that result is an invariant is necessary")
COMPILE_TIME_ASSERT(((uint32_t) sizeof(ADI_OSAL_MUTEX_INFO) < (uint32_t) ADI_OSAL_MAX_MUTEX_SIZE_CHAR), verify_size_mutex_object);
COMPILE_TIME_ASSERT((OS_LOWEST_PRIO > 63), verify_ucos_lowest_prio_config);
#pragma diag(pop)

/* priority of the virtual startup "thread" */
#define STARTUP_PRIORITY    0u
#define ADI_OSAL_INVALID_HEAP_INDEX (-1)

#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")


/*=============  D A T A  =============*/
/* TODO: These declarations need to be discussed for multicore environment*/

/*
    variable    : _adi_osal_gnNumSlots
    stores the number of TLS slots requested by the user during initialization.
*/
uint32_t _adi_osal_gnNumSlots;


/*
   variable     : _adi_osal_gaThreadLocalBuffStatusRegister
   buffer to hold the status thread's local storage  slot.
   Bits are used to represent the status of the slot. Bit 0 corresponds to slot
   number 0 and slot number 30 corresponds bit number 30. Bit 31 is reserved.
   A slot is free if the corresponding bit is set and a slot is acquired if the
   corresponding bit is cleared. Initially all the slot bits are set.
*/
uint32_t _adi_osal_gaThreadLocalBuffStatusRegister;


/*
    variable: _adi_osal_gaTLSCallbackTable
    Hold the callback structure for each Thread Local Storage Slot. The Callback are
    per slot, all threads that are using that slot are using the same callback.
    Make the array the maximum supported size (ADI_OSAL_MAX_THREAD_SLOTS)
*/
ADI_OSAL_TLS_CALLBACK_PTR _adi_osal_gaTLSCallbackTable[ADI_OSAL_MAX_THREAD_SLOTS];


/*
   variable     : gIsrInfoTable
   This table holds the information of ISRs such as function to be called.
 */
#pragma section("L1_data")
ADI_OSAL_ISR_INFO _adi_osal_gIsrInfoTable[ADI_MAX_NUM_IVG_LEVELS];


/*
   variable     : _adi_osal_gnTickPeriod
   defines the length of system ticks in microseconds
 */
uint32_t _adi_osal_gnTickPeriod;


/*
    variable    : _gnHeapIndex
    Heap index that is used for memory operations within the OSAL. 
    This variable is only valid after the call to adi_osal_Init
*/
static int32_t _gnHeapIndex = ADI_OSAL_INVALID_HEAP_INDEX;

/*
    variable    : _gpHeapMemory
    Pointer to the memory area to be used for the heap
    this variable is unused after the call to OSStart, it is only used to pass information from
    Init to OSStart
*/
static uint32_t*  _gpHeapMemory;

/*
    variable    : _gnHeapMemorySize
    Size of the memory area to be used for the heap
    this variable is unused after the call to OSStart, it is only used to pass information from
    Init to OSStart
*/
static uint32_t  _gnHeapMemorySize;

/*
    variable    : _adi_osal_gnSchedulerLockCnt
    Indicates if the code is within a Scheduler lock section. It is only used in debug mode
    to check if Unlock is called with Lock being called first. This needs to be a counter to allow
    for nesting
*/
#ifdef OSAL_DEBUG
uint32_t _adi_osal_gnSchedulerLockCnt;
#endif /* OSAL_DEBUG */


/*
   variable     : _adi_osal_gbSchedulerActive
   A boolean flag to indicate whether scheduler is active.
*/
bool _adi_osal_gbSchedulerActive;


/*
   variable     : _adi_osal_gnCriticalRegionNestingCnt
   This variable is a counter which is incremented when adi_osal_EnterCriticalRegion()
   is called and decremented when adi_osal_ExitCriticalRegion is called.
   it is implemented using "cli" and "sti" instructions respectively rather than
   the OS API.
*/
#pragma section("L1_data")
int32_t _adi_osal_gnCriticalRegionNestingCnt;


/*
    variable    : _adi_osal_gnCriticalRegionState
    Holds the state of the interrupt mask as of the first call to adi_osal_EnterCriticalRegion
*/
#pragma section("L1_data")
uint32_t _adi_osal_gnCriticalRegionState;


/*
    variable    : _adi_osal_oStartupVirtualThread
    This thread is not a real thread, but is active until the OS starts. It will allow the TLS functions to operate
    until the OS takes over.
*/
ADI_OSAL_THREAD_INFO _adi_osal_oStartupVirtualThread;


/*=============  C O D E  =============*/


/*****************************************************************************
  Function: adi_osal_GetVersion
     This API function call is used to access the code-base version information.
     The code-base version differs for each target operating system of the OSAL
     although the major and minor revs are the same for all OS variants.

  Parameters:
      pVersion - the location to store the retrieved version information.

  Returns:
     ADI_OSAL_SUCCESS - if able to successfully return the version
     ADI_OSAL_FAILED  - in the unlikely event that the version information
                        could not be obtained.
  Note:
    Version number is mentioned in the format major.minor.patch.build.
    For example,Version "1.0.2.2022" means

    => major  = 1.
    => minor  = 0.
    => patch  = 2.
    => build  = 2022.

    Members of structure ADI_OSAL_VERSION_PTR are also declared in above order.
*****************************************************************************/

ADI_OSAL_STATUS adi_osal_GetVersion(ADI_OSAL_VERSION *pVersion)
{
    pVersion->nMajor = ADI_OSAL_MAJOR_VER;
    pVersion->nMinor = ADI_OSAL_MINOR_VER;
    pVersion->nPatch = ADI_OSAL_PATCH_VER;
    pVersion->nBuild = ADI_BUILD_VER;
    return(ADI_OSAL_SUCCESS);
}



/*****************************************************************************
  Function: adi_osal_Init

   This function initializes the internal OSAL data structure. It should be
   called during the system startup.

  Parameters:
      pConfig - pointer to a ADI_OSAL_CONFIG data structure that contains the
      OSAL configuration options.

  Returns:
      ADI_OSAL_SUCCESS - Initialization is done successfully.
      ADI_OSAL_FAILED  - OSAL was already initialised
      ADI_OSAL_OS_ERROR- The version of OSAL is not compatible with the uCOS 
            version
      ADI_OSAL_BAD_SLOT_NUM - Number of thread local storage slots specified
            greater than the maximum allowed.
      ADI_OSAL_BAD_PRIO_INHERIT - Priority inheritance specified when it is not
            supported

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_Init( const ADI_OSAL_CONFIG *pConfig)
{
    uint32_t i;             /* indexing through interrupts table */
    /*
        global variable    : gbOsalInitialized
    */
    static bool gbOsalInitialized;  /* A boolean flag to record if the OSAL has already been initialized */

    /*
        global variable    : gaBackgroundThreadTLSBuffer
    Since the number of slots is not known at built time and this "thread" TLS functionality needs to be available
    before OSStart, memory for the maximum number of slots is preallocated.
    */
    static ADI_OSAL_SLOT_VALUE gaBackgroundThreadTLSBuffer[ADI_OSAL_MAX_THREAD_SLOTS];



    /* Check if already initialized */
    if (gbOsalInitialized)
    {
        return (ADI_OSAL_FAILED);
    }

    /* Checks that the version of uC/OS is compatible with this version of OSAL */
    if (OSVersion() < COMPATIBLE_OS_VERSION)
    {

        /* TODO: another thing that could be done is to check the OS settings.
        It can be done if OSDebugEn == 1 */

        return (ADI_OSAL_OS_ERROR);
    }

#if 0

    /* Fix for SAR-50710 "OSAL on uCOS forces uCOS to have debug enabled" We
     * cannot say to all customers that they must have debug on so we remove
     * all the requirements for this being the case. This may mean that we
     * cannot check certain error conditions as happened below
     */

    /* The only ways to check these error conditions are:
     * - use OSFlagWidth/OSTCBSize which forces the application to have uCOS 
     *   debug enabled
     * - Rebuild OSAL with their own ucos_ii.h header file
     * For this reason, we don't do the check 
     */

    /* the OSAL requires the events groups to be 31 bits, uCOS allows several
     * options: 8, 16, or 32 bits.
     * we need to verify here that the users have chosen the right option (32
     * bits) */

    if (OSFlagWidth != sizeof(uint32_t))
    {
        return (ADI_OSAL_OS_ERROR);
    }

    /* check that the ADI_OSAL_UCOS_MAX_TCB_SIZE value is big enough for the
     * current TCB definition
     * Since the OS_TCB definition is variable depending on the configuration
     * options, we need to have a value defined for the worst case.
     */

    if (OSTCBSize > ADI_OSAL_UCOS_MAX_TCB_SIZE)
    {
        return (ADI_OSAL_OS_ERROR);
    }

#endif

    /* checks that arguments are all valid */
    if (NULL != pConfig)
    {
        if (pConfig->eEnablePrioInherit == ADI_OSAL_PRIO_INHERIT_ENABLED)
        {
            /* priority inheritance is always disabled in uCOS */
            return(ADI_OSAL_BAD_PRIO_INHERIT);
        }
        if (pConfig->nNumTLSSlots > (uint8_t) ADI_OSAL_MAX_THREAD_SLOTS)
        {
            return (ADI_OSAL_BAD_SLOT_NUM);
        }
        if (pConfig->nSysTimerPeriodInUsec == (uint32_t) 0)
        {
            /* automatically sets the tick period based on the uC/OS settings */
            #if 0
            /* this uCOS variable only exists when uCOS is in debug mode so 
               we cannot rely on it
             */
            _adi_osal_gnTickPeriod = USEC_PER_SEC / OSTicksPerSec;
            #else
            _adi_osal_gnTickPeriod = ADI_OSAL_DEFAULT_SYS_TIMER_PERIOD;
            #endif
        } else {
            _adi_osal_gnTickPeriod = pConfig->nSysTimerPeriodInUsec;
        }
        _adi_osal_gnNumSlots = pConfig->nNumTLSSlots;


        /* Since the memory manager has a dependency on the OSAL functions, it
        needs to be called after the OSInit function has been called. It therefore needs to happen
        in the adi_osal_OSStart function. This is why we need to store the memory information here */
        _gpHeapMemory = pConfig->pHeap;
        _gnHeapMemorySize = pConfig->nHeapSizeBytes;


    } else
    {
        /* use default settings when pConfig is NULL */
        _adi_osal_gnNumSlots   = ADI_OSAL_DEFAULT_NUM_TLS_SLOTS;
        _adi_osal_gnTickPeriod = ADI_OSAL_DEFAULT_SYS_TIMER_PERIOD;
        _gpHeapMemory = NULL;
        _gnHeapMemorySize = 0u;
    }

    /* Initially the scheduler os not started */
    _adi_osal_gbSchedulerActive = false;

    /* Initially the scheduler is not locked */
#ifdef OSAL_DEBUG
    _adi_osal_gnSchedulerLockCnt = 0u;
#endif /* OSAL_DEBUG */

    /* Initially not in a critical region */
    _adi_osal_gnCriticalRegionNestingCnt = 0;

    /* It is not really necessary to set this value. It has debug value though
       If ExitCriticalRegion is called before EnterCriticalRegion, it will be quite obvious
       since all interrupts will be blocked */
    _adi_osal_gnCriticalRegionState = 0u;

    /* All local storage slots are available : Initialize the "_adi_osal_gaThreadLocalBuffStatusRegister" to
       all "available" state. i.e clear all bits. When Slots are taken, their related bit is set.
       if  _adi_osal_gnNumSlots is equal to n , then "n" LSBs are cleared and stored in global
       status register(_adi_osal_gaThreadLocalBuffStatusRegister) the other bits are considered as "taken"
    */
     _adi_osal_gaThreadLocalBuffStatusRegister =(0XFFFFFFFF) << _adi_osal_gnNumSlots;


    /* Set all callbacks to invalid */
    for ( i = 0u; i < ADI_OSAL_MAX_THREAD_SLOTS; i++ )
    {
        _adi_osal_gaTLSCallbackTable[i] = NULL;
    }

    /* create the thread that represent the current excution as a thread until the OS actually starts */
    _adi_osal_oStartupVirtualThread.pThreadLocalStorageBuffer = &gaBackgroundThreadTLSBuffer;
    _adi_osal_oStartupVirtualThread.nThreadPrio = STARTUP_PRIORITY;


    /* Old versions of the code used to reset all interrupts because it was
       assumed that OSAL would manage all interrupts. This is no
       longer the expected behaviour. The CRT already sets all interrupts to a
       known value so we leave them as they are. This is a fix for SAR-50480.
       We still set up the OSAL internal pointers so we know which ISRs have
       been set up with OSAL so we can uninstall them
    */

    for( i = 0u ; i < (uint32_t)(ADI_MAX_NUM_IVG_LEVELS); i++ )
    {
        _adi_osal_gIsrInfoTable[i].pIsrFunction = &_adi_osal_UnregisteredInterrupt; 
    }

    gbOsalInitialized = true;

    return( ADI_OSAL_SUCCESS);
}



/*****************************************************************************
  Function: adi_osal_OsStart

    Initializes uCOS, allocate resource required by the OS, create the start thread
    invoke the startup callback and starts the scheduler.

    WARNING: This function should never return.

  Parameters:
      pOSInitData   - private data optionally passed to the OS. Ignored for uCOS.
      pStartCB      - Startup Callback function
      pStartThread  - Startup Thread

  Return Values:
      ADI_OSAL_MEM_ALLOC_FAILED - Error initializing dynamic memory heap
      ADI_OSAL_CALLBACK_FAILED  - Callback pStartCB returned an error
      ADI_OSAL_FAILED           - Start Thread could not be properly created
      ADI_OSAL_START_FAILED     - OS returns an error upon start.

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_OSStart(const void *pOSInitData,
                                 const ADI_OSAL_CALLBACK_ATTR *pStartCB,
                                 const ADI_OSAL_THREAD_ATTR *pStartThread)
{
    ADI_OSAL_STATUS eResult;
    ADI_OSAL_THREAD_HANDLE hStartupThread;      /* it is not necessary to keep this handle since
                                                   there are no ways to shutdown the OS. No operations
                                                   other than creation are done on this thread.
                                                   So local storage is sufficient */


    /*
        1. Initializes the OS

        it must be called before OSStart. After this call, Most OS functions can
        be invoked.
    */
    PAUSE_PROFILING();
    OSInit();
    RESUME_PROFILING();

    /*
        2. Create a memory pool for OSAL's dynamic memory allocations

        it must be called after OSInit because it uses Mutexes.
    */
    if (NULL != _gpHeapMemory) {
        int32_t nHeapUserId;

        if (_gnHeapMemorySize <= OSAL_MINIMUM_HEAP_SIZE)
        {
            return (ADI_OSAL_MEM_ALLOC_FAILED);
        }

        /* Create a heap with the information provided. The user ID might be in
         * use by application code. For this reason, if heap_install fails OSAL
         * will change the user ID 10 times before giving up and returning an
         * error 
         */

        nHeapUserId = INT_MAX ; 
        do
        {
            _gnHeapIndex = heap_install(_gpHeapMemory,_gnHeapMemorySize, nHeapUserId);
            nHeapUserId --;
        } while ( (ADI_OSAL_INVALID_HEAP_INDEX == _gnHeapIndex)  && ( (INT_MAX -10 )< nHeapUserId ));

        if (ADI_OSAL_INVALID_HEAP_INDEX == _gnHeapIndex )
        {
                return (ADI_OSAL_MEM_ALLOC_FAILED);
        }
    }


    /*
        3. Invoke the Startup Callback
    */
    /* NULL is valid and indicate to skip the callback */
    if (NULL != pStartCB)
    {
        if (ADI_OSAL_SUCCESS != pStartCB->pCallbackFunc(pStartCB->pParam))
        {
            return(ADI_OSAL_CALLBACK_FAILED);
        }
    }

    /*
        4. Create the Startup thread that was passed by the user
    */
    if (pStartThread != NULL)
    {
        eResult = adi_osal_ThreadCreate(&hStartupThread, pStartThread);
        if (eResult != ADI_OSAL_SUCCESS)
        {
            /* failure results from ThreadCreate are mapping 1-to-1 except CALLER_ERROR which we know is fine
               and PRIORITY_IN_USE which is also fine since this is creating the first thread */
            return(eResult);
        }
    }
    else
    {
        return(ADI_OSAL_FAILED);
    }

    /*
        5. Start the OS
    */
    /* set the "scheduler flag" here since next statement will start the OS execution and won't return */
    _adi_osal_gbSchedulerActive = true;

    /* The following call with start the OS and the Startup thread will become active */
    OSStart();      /* THIS CALL DOES NOT RETURN!!! */


    /* WARNING: Execution should never enter here */

        /* Infinite loop.*/
        raise(SIGEMU);        /*  throws an exception if emulator is connected */
    
    return (ADI_OSAL_START_FAILED);
}



/*****************************************************************************
  Function: _adi_osal_UnregisteredInterrupt
    This function address is filled in the  adi_osal_gIsrInfoTable[ ] as a default value.
  Parameters:
    none
  Returns:
    none

*****************************************************************************/

void  _adi_osal_UnregisteredInterrupt(void )
{
  raise(SIGEMU);
}



/*****************************************************************************
   Function: _adi_osal_MemAlloc

    This function is abstracting which memory manager to use for dynamic memory allocations

   Parameters:
     ppData - Pointer to the allocated memory and size of memory required
     nSize  - Size of memory to be allocated

   Returns:
      ADI_OSAL_FAILED  - The memory could not be allocated

*****************************************************************************/

ADI_OSAL_STATUS _adi_osal_MemAlloc(void** ppData, uint32_t nSize)
{
    void* pMemory;

    /* pass it directly to heap_malloc if a heap was created or malloc
     * otherwise */
    if (_gnHeapIndex != ADI_OSAL_INVALID_HEAP_INDEX)
    {
        pMemory = heap_malloc(_gnHeapIndex, nSize);
    }
    else
    {
        pMemory = malloc(nSize);
    }

    if (pMemory != NULL)
    {
        *ppData = pMemory;
        return (ADI_OSAL_SUCCESS);
    }
    else
    {
        return (ADI_OSAL_FAILED);
    }
}



/*****************************************************************************
   Function: _adi_osal_MemFree

    This function is abstracting which memory manager to use to free dynamic memory

   Parameters:
     same as free

   Returns:
     None

*****************************************************************************/

void  _adi_osal_MemFree(void* pData)
{
    if (_gnHeapIndex != ADI_OSAL_INVALID_HEAP_INDEX)
    /* pass it directly to heap_free*/
    {
        heap_free(_gnHeapIndex, pData);
    }
    else
    /* use regular malloc/free with the system heap */
    {
        free(pData);
    }
}


#pragma diag(pop)  /* global file supressions */

/*
**
** EOF: 
**
*/

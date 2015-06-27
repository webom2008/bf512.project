/* $Revision: 5506 $
 * $Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $
******************************************************************************
Copyright (c), 2009-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:      OSAL for No-OS

Description:
              Operating System Abstraction Layer for the environment where
              there is no OS(Operating System) running.

NOTE:
              The Memory Manager Service is currently used in the no-OS implementation
              Eventually we may want to remove it in order to remove the circular dependency 
              during initialization : OSAL needs to initialize the memory manager for 
              allocating its internal structures and the memory manager need to initialize 
              the OSAL to use the mutex functionality.
              Maybe using regular heap_malloc would work (unless malloc also depends on OSAL
              init) or the OSAL need to have its own internal dynamic memory allocation 
              scheme.
              

SUPRESSED MISRA RULES ARE:

            1.1  [R]    :  "All code shall conform to ISO 9899:1990"
            5.1  [R]    :  "Identifier shall not rely on the significance of more than 31 characters"
            5.7  [A]    :  "No identifier name should be reused"
            11.1 [R]    :  "Conversions shall not be performed between a pointer to function and
                           any type other than an integral type"
            11.3 [A]    :  "A cast should not be performed between a pointer type and an integral type"
            11.4 [A]    :  "A cast should not be performed between a pointer to object type and a
                           different pointer to object type"
            14.7 [R]    :  "a function shall have a single point of exit at the end of the function"
            16.2 [R]    :  "Functions shall not call themselves, either directly or indirectly"
            17.5 [A]    :  "The declaration of objects should contain no more than 2 levels of pointer indirection"
			20.8 [R]    :  "The signal handling facilities of <signal.h> shall not be used. "
			20.4 [R]    :  "Dynamic heap memory allocation shall not be used." 


*****************************************************************************/

/*=============  I N C L U D E S   =============*/


#pragma diag(push)
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exits (mostly for handling parameter error checking) increases the code readability and therefore maintenance")
/* Rule-16.2 indicates that  Functions shall not call themselves,either directly or indirectly */
#pragma diag(suppress:misra_rule_16_2 : "Since the OSAL is reentrant by nature (several thread could call the API) the compiler MISRA checker mistakes sometimes the reentrancy for recurrence")
/* Rule-5.1 indicates that all identifiers shall not rely on more than 31 characters of significance */
#pragma diag(suppress:misra_rule_5_1:"prefixes added in front of identifiers quickly increases their size. In order to keep the code self explanatory, and since ADI tools are the main targeted tools, this restriction is not really a necessity")
/* Rule-20.8 The signal handling facilities of <signal.h> shall not be used. */
#pragma diag(suppress:misra_rule_20_8:"This source needs to use signals")
/* Rule-20.4 Dynamic heap memory allocation shall not be used. */
#pragma diag(suppress:misra_rule_20_4:"This source needs to use malloc")



#include <stdlib.h>                                                             /* for NULL definition */
#include <blackfin.h>                                                           /* for pIPEND definition */
#include <signal.h>                                                             /* for raise */
#include <ccblkfn.h>                                                            /* for cli/sti */
#include <osal/adi_osal.h>
#include "../include/osal_common.h"
#include "osal_noos.h"

/* "version.h" contains the macro "ADI_OSAL_SVN_REV" which gives the build version
   for OSAL. It is a generated file.
*/
#include "version.h"


/*=============  D E F I N E S  =============*/


#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#define ADI_OSAL_INVALID_HEAP_INDEX (-1)


/*=============  D A T A  =============*/

/*
   variable     : gIsrInfoTable
   This table holds the information of ISRs such as function to be called.
 */
 #pragma section("L1_data")
ADI_OSAL_ISR_INFO _adi_osal_gIsrInfoTable[ADI_MAX_NUM_IVG_LEVELS];

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
    per slot.
    Make the array the maximum supported size (ADI_OSAL_MAX_THREAD_SLOTS)
*/
ADI_OSAL_TLS_CALLBACK_PTR _adi_osal_gaTLSCallbackTable[ADI_OSAL_MAX_THREAD_SLOTS];

/*
    variable: _adi_osal_gaTLSSlotsTable
    Hold the TLS values for each slot. Since in the noOS implementation there are
    only ONE 'thread', this array can be allocated statically.
    Make the array the maximum supported size (ADI_OSAL_MAX_THREAD_SLOTS)
*/
ADI_OSAL_SLOT_VALUE _adi_osal_gaTLSSlotTable[ADI_OSAL_MAX_THREAD_SLOTS];

/*
    variable    : _adi_osal_gnNumSlots
    stores the number of TLS slots requested by the user during initialization.
*/
uint32_t _adi_osal_gnNumSlots;

/*
   variable     : _adi_osal_gnTickPeriod
   defines the length of system ticks in microseconds
 */
uint32_t _adi_osal_gnTickPeriod;


/*
   variable     : _adi_osal_gnTickCnt
   System ticks counter. Each tick represents '_adi_osal_gnTickPeriod' microseconds.
 */
uint32_t _adi_osal_gnTickCnt;


/*
    variable    : _gnHeapIndex
    Heap index that is used for memory operations within the OSAL. 
    This variable is only valid after the call to adi_osal_Init
*/
static int32_t _gnHeapIndex = ADI_OSAL_INVALID_HEAP_INDEX;

/*
    variable    : _adi_osal_gnSchedulerLockCnt
    This variable is only used in debug mode and indicates if an error condition
    when a Unlock is called before Lock. The Lock and Unlock functions themselves
    don't do anything in the no-os implementation
*/
#ifdef OSAL_DEBUG
uint32_t _adi_osal_gnSchedulerLockCnt;
#endif /* OSAL_DEBUG */

/*
    variable    : gbGlobalLock
    Variable that prevents reentrancy. It is checked and set when entering a function
    and if it is already taken (true), the function is trying to reenter, it should return
    an error
*/
#pragma section("L1_data")
static volatile bool gbGlobalLock;



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

ADI_OSAL_STATUS adi_osal_GetVersion(ADI_OSAL_VERSION* pVersion)
{
    pVersion->nMajor = ADI_OSAL_MAJOR_VER;
    pVersion->nMinor = ADI_OSAL_MINOR_VER;
    pVersion->nPatch = ADI_OSAL_PATCH_VER;
    pVersion->nBuild = ADI_BUILD_VER;
    return(ADI_OSAL_SUCCESS);
}


/*****************************************************************************
  Function: adi_osal_Init

   Init function for OSAL. This function will be called from system startup.
   adi_osal_Init() must be called just after the start-up since this function calls


  Parameters:
      pOSInit - Pointer to ADI_OSAL_OS_INIT_ATTR structure.

  Returns:
      ADI_OSAL_SUCCESS - Initialization is done successfully.
      ADI_OSAL_FAILED  - OSAL has already been initialized
      ADI_OSAL_BAD_SLOT_NUM - Number of thread local storage slots specified 
            greater than the maximum allowed.
      ADI_OSAL_BAD_PRIO_INHERIT - Priority inheritance specified when it is not 
            supported
      ADI_OSAL_MEM_ALLOC_FAILED - Either the size of the heap specified is smaller than
            the minimum or the memory manager failed.

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_Init( const ADI_OSAL_CONFIG *pConfig )
{

    uint32_t   i;                                                               /* indexing through interrupts table */
    uint32_t*  pHeapMemory;                                                     /* pointer to the area of memory to use for the heap */
    uint32_t   nHeapMemorySize;                                                 /* size of memory pointed by pHeapMemory */
    /*
        global variable    : gnOsalInitialized
    */
    static uint32_t gnOsalInitialized;  /* A boolean flag to record if the OSAL has already been initialized, it's implemented
                                           using a variable with a known value to allow for cases where the global variable is
                                           uninitialized  */

    /*
        global variable    : gaTLSBuffer
    Since the number of slots is not known at built time and this "thread" TLS functionality needs to be available
    even though there will not be any threads, memory for the maximum number of slots is preallocated.
    */
    static ADI_OSAL_SLOT_VALUE gaTLSBuffer[ADI_OSAL_MAX_THREAD_SLOTS];

    /* Check if already initialized */
    if (gnOsalInitialized == OSAL_INITIALIZED)
    {
        return (ADI_OSAL_FAILED);
    }


    /* checks that arguments are all valid */
    if (NULL != pConfig)
    {
        if (pConfig->eEnablePrioInherit == ADI_OSAL_PRIO_INHERIT_ENABLED)
        {
            /* priority inheritance is not available in the noos version */
            return(ADI_OSAL_BAD_PRIO_INHERIT);
        }

        if (pConfig->nNumTLSSlots > (uint8_t) ADI_OSAL_MAX_THREAD_SLOTS)
        {
            return (ADI_OSAL_BAD_SLOT_NUM);
        }

        if (pConfig->nSysTimerPeriodInUsec == (uint32_t) 0)
        {
            /* automatically sets the tick period based on the "OS" settings : considered not supported */
            _adi_osal_gnTickPeriod = 0u;
        } else {
            _adi_osal_gnTickPeriod = pConfig->nSysTimerPeriodInUsec;
        }
        _adi_osal_gnNumSlots = pConfig->nNumTLSSlots;

        /* Since the memory manager has a dependency on the OSAL functions, it
        needs to be called only after the adi_osal_Init function has been called. It therefore needs to happen
        in the adi_osal_OSStart function. This is why we need to store the memory information here */
        pHeapMemory = pConfig->pHeap;
        nHeapMemorySize = pConfig->nHeapSizeBytes;


    } else
    {
        /* use default settings when pConfig is NULL */
        _adi_osal_gnNumSlots   = ADI_OSAL_DEFAULT_NUM_TLS_SLOTS;
        _adi_osal_gnTickPeriod = ADI_OSAL_DEFAULT_SYS_TIMER_PERIOD;
        pHeapMemory = NULL;
        nHeapMemorySize = 0u;
    }

    if (NULL != pHeapMemory) {
        int32_t nHeapUserId;

        if (nHeapMemorySize <= OSAL_MINIMUM_HEAP_SIZE)
        {
            return (ADI_OSAL_MEM_ALLOC_FAILED);
        }

        /* Create a heap with the information provided. The user ID might be in
         * use by application code. For this reason, if heap_install fails OSAL
         * will change the user ID 10 times before giving up and returning an
         * error 
         */

        nHeapUserId = INT_MAX;
        do
        {
            _gnHeapIndex = heap_install(pHeapMemory,nHeapMemorySize, nHeapUserId);
            nHeapUserId --;
        } while ( (ADI_OSAL_INVALID_HEAP_INDEX == _gnHeapIndex)  && ( (INT_MAX -10 )< nHeapUserId ));

        if (ADI_OSAL_INVALID_HEAP_INDEX == _gnHeapIndex )
        {
                return (ADI_OSAL_MEM_ALLOC_FAILED);
        }
    }

    /* Ticks start at value 0 */
    _adi_osal_gnTickCnt = 0u;

    /* in the no-OS, the scheduler is never active */
    _adi_osal_gbSchedulerActive = false;

    /* Initially the scheduler is not locked */
#ifdef OSAL_DEBUG
    _adi_osal_gnSchedulerLockCnt = 0u;
#endif /* OSAL_DEBUG */

    /* Initially not in a critical region */
    _adi_osal_gnCriticalRegionNestingCnt = 0;

    /* initially global lock is not taken */
    gbGlobalLock = false;

    /* It is not really necessary to set this value. It has debug value though
       If ExitCriticalRegion is called before EnterCriticalRegion, it will be quite obvious
       since all interrupts will be blocked */
    _adi_osal_gnCriticalRegionState = 0u;

    /* All local storage slots are available : Initialize the "_adi_osal_gaThreadLocalBuffStatusRegister" to
       all "available" state. i.e clear all bits.
       if  _adi_osal_gnNumSlots is equal to n , then "n" n LSBs are set and stored in global
       status register(_adi_osal_gaThreadLocalBuffStatusRegister).
    */
     _adi_osal_gaThreadLocalBuffStatusRegister =(0xFFFFFFFF) << _adi_osal_gnNumSlots;


    /* Set all callbacks to invalid */
    for ( i = 0u; i < ADI_OSAL_MAX_THREAD_SLOTS; i++ )
    {
        _adi_osal_gaTLSCallbackTable[i] = NULL;
        _adi_osal_gaTLSSlotTable[i] = NULL;
    }


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


    gnOsalInitialized = OSAL_INITIALIZED;

    return( ADI_OSAL_SUCCESS);
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
  raise(SIGEMU);        /*  throws an exception if the emulator is connected */
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



/*****************************************************************************
   Function: _adi_osal_IsCurrentLevelISR

    This function returns the current interrupt level

   Parameters:
       None

   Returns:
       true if the current execution ISR level is less than 15, false if the level is 15

*****************************************************************************/
bool _adi_osal_IsCurrentLevelISR( void )
{
    /* Look for the highest interrupt currently pending in the core, for this the
       register IPEND is read and some of the levels are masked off (EMU, GBLDIS) */
    uint32_t nIPEND = *pIPEND;

    /* skip Level 0, 4 & 15 : Emulator, Global Disable, and application level*/
    uint32_t nMask  = ~((0x1ul << 0) | (0x1ul << 4) | (0x1ul << 15));

    nIPEND = nIPEND & nMask ;  
    
    return (nIPEND != 0ul);
}


/*****************************************************************************
   Function: _adi_osal_AcquireGlobalLock

    This function returns true if the lock was acquired, and false if the lock
    is not available

   Parameters:
       None

   Returns:
       true if the lock was acquired, and false if the lock
       is not available

*****************************************************************************/
bool _adi_osal_AcquireGlobalLock( void )
{
    uint32_t nImask;
    bool     bRetval;

    ENTER_CRITICAL_INTERNAL(nImask);
    bRetval = gbGlobalLock;
    bRetval = (bRetval==false);
    if (bRetval)
    {
        gbGlobalLock = true;
    }
    EXIT_CRITICAL_INTERNAL(nImask);

    return (bRetval);
}

/*****************************************************************************
   Function: _adi_osal_ReleaseGlobalLock

    This function sets the lock back to false.

   Parameters:
       None

   Returns:
       true if the lock was acquired, and false if the lock
       is not available

*****************************************************************************/
void _adi_osal_ReleaseGlobalLock( void )
{
    gbGlobalLock = false;
}

#pragma diag(pop)  /* global file supressions */

/*
**
** EOF: 
**
*/


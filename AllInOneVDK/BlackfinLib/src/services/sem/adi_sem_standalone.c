/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_sem_standalone.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			This file contains the standalone code for the semaphore service

*********************************************************************************/

#include <services/services.h>


/*******************************************************************************

    Data Structures

********************************************************************************/
    
typedef struct adi_sem_semaphore {
    volatile u32                Count;              // count for the semaphore 
    ADI_SEM_PENDING_FUNCTION    PendingFunction;    // function to call when pending
    ADI_SEM_POSTING_FUNCTION    PostingFunction;    // function to call when posting
    struct adi_sem_semaphore    *pNext;             // next semaphore
} ADI_SEM_SEMAPHORE;

typedef struct {
    u8                  InitializedFlag;    // initialized flag (FALSE - not initialized, TRUE - initialized)
    void                *pCriticalRegionArg;// parameter for critical region function
    u32                 Count;              // semaphore count
    ADI_SEM_SEMAPHORE   *pActiveList;       // list of active semaphore objects
    ADI_SEM_SEMAPHORE   *pFreeList;         // list of free semaphore objects
    ADI_SEM_SEMAPHORE   *pSemaphores;       // starting address of all the semaphores
} ADI_SEM_INSTANCE_DATA;

ADI_SEM_INSTANCE_DATA adi_sem_InstanceData;	// address of instance data for the semaphore service


/*******************************************************************************

    Static functions

********************************************************************************/

#ifdef ADI_SSL_DEBUG

ADI_SEM_RESULT ValidateSemaphoreHandle(ADI_SEM_HANDLE Handle);     // validates a semaphore handle

#endif




/*********************************************************************

    Function:       adi_sem_Init

    Description:    Initializes the semaphore service

*********************************************************************/
ADI_SEM_RESULT adi_sem_Init(        // initializes the semaphore service
    void            *pMemory,           // pointer to memory
    const size_t    MemorySize,         // size of memory (in bytes)
    u32             *pMaxSemaphores,    // number of semaphores that can be supported
    void            *pCriticalRegionArg // parameter for critical region function
){
    
    ADI_SEM_RESULT      Result;             // result
	void                *pExitCriticalArg;  // return value from EnterCritical
	u32                 i;                  // counter
	ADI_SEM_SEMAPHORE   *pSemaphore;        // pointer to a semaphore
    
    // ever the optimist
    Result = ADI_SEM_RESULT_SUCCESS;
    
    // insure we're not being initialized repeatedly
#if defined(ADI_SSL_DEBUG)
	if (ADI_SEM_SEMAPHORE_MEMORY != sizeof(ADI_SEM_SEMAPHORE)) {
		Result = ADI_SEM_RESULT_BAD_MEMORY_SIZE;
	}
    if (adi_sem_InstanceData.InitializedFlag == TRUE) {
        Result = ADI_SEM_RESULT_ALREADY_INITIALIZED;
    }
#endif

    // IF (no errors) 
    if (Result == ADI_SEM_RESULT_SUCCESS) {
        
        // mark that we're initialized
        pExitCriticalArg = adi_int_EnterCriticalRegion(pCriticalRegionArg);
        adi_sem_InstanceData.InitializedFlag = TRUE;
        adi_int_ExitCriticalRegion(pExitCriticalArg);

        // populate the rest of the instance data
        adi_sem_InstanceData.pCriticalRegionArg = pCriticalRegionArg;
        adi_sem_InstanceData.Count = MemorySize/sizeof(ADI_SEM_SEMAPHORE);
        adi_sem_InstanceData.pSemaphores = pMemory;
        adi_sem_InstanceData.pFreeList = pMemory;
        adi_sem_InstanceData.pActiveList = NULL;
        
        // tell the client how many semaphores we can support
        *pMaxSemaphores = adi_sem_InstanceData.Count;
        
        // make the free list
        for (i = adi_sem_InstanceData.Count, pSemaphore = adi_sem_InstanceData.pFreeList; i; i--, pSemaphore++) {
            pSemaphore->pNext = pSemaphore + 1;
        }
        if (adi_sem_InstanceData.Count) {
            pSemaphore--;
            pSemaphore->pNext = NULL;
        }
        
    // ENDIF
    }
    
    // return
    return(Result);
}



/*********************************************************************

    Function:       adi_sem_Terminate

    Description:    Terminates the semaphore service

*********************************************************************/

ADI_SEM_RESULT adi_sem_Terminate(void) {   // terminates the semaphore service
    
    ADI_SEM_RESULT Result;      // result
    
    // ever the optimist
    Result = ADI_SEM_RESULT_SUCCESS;
    
    // insure we've been initialized
#if defined(ADI_SSL_DEBUG)
    if (adi_sem_InstanceData.InitializedFlag == FALSE) {
        Result = ADI_SEM_RESULT_NOT_INITIALIZED;
    }
#endif

    // IF (no errors) 
    if (Result == ADI_SEM_RESULT_SUCCESS) {
        
        // mark us as uninitialized
        adi_sem_InstanceData.InitializedFlag = FALSE;
        
    // ENDIF
    }
    
    // return
    return(Result);
}



/*********************************************************************

    Function:       adi_sem_Create

    Description:    Creates a semaphore

*********************************************************************/

ADI_SEM_RESULT adi_sem_Create(  
    u32                     Count,          // intial count
    ADI_SEM_HANDLE          *pHandle,       // location where the handle to the semaphore is stored
    ADI_SEM_OS_ATTRIBUTES   *pOSAttributes  // pointer to the semaphore's attributes
) {
    
    ADI_SEM_RESULT      Result;             // result
	void                *pExitCriticalArg;  // return value from EnterCritical
	ADI_SEM_SEMAPHORE   *pSemaphore;        // pointer to a semaphore
    
    // ever the optimist
    Result = ADI_SEM_RESULT_SUCCESS;
    
    // insure we've been initialized
#if defined(ADI_SSL_DEBUG)
    if (adi_sem_InstanceData.InitializedFlag == FALSE) {
        Result = ADI_SEM_RESULT_NOT_INITIALIZED;
    }
#endif

    // IF (no errors) 
    if (Result == ADI_SEM_RESULT_SUCCESS) {
        
        // try and allocate a semaphore
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_sem_InstanceData.pCriticalRegionArg);
        if ((pSemaphore = adi_sem_InstanceData.pFreeList) != NULL) {
            adi_sem_InstanceData.pFreeList = pSemaphore->pNext;
        } else {
            Result = ADI_SEM_RESULT_NONE_AVAILABLE;
        }
        adi_int_ExitCriticalRegion(pExitCriticalArg);
        
        // IF (we allocated one) 
        if (Result == ADI_SEM_RESULT_SUCCESS) {
            
            // initialize the count
            pSemaphore->Count = Count;
            
            // initialize the pending function if provided
            if (pOSAttributes) {
                pSemaphore->PendingFunction = pOSAttributes->PendingFunction;
                pSemaphore->PostingFunction = pOSAttributes->PostingFunction;
            } else {
                pSemaphore->PendingFunction = NULL;
                pSemaphore->PostingFunction = NULL;
            }
            
            // give the caller the handle to the semaphore
            *pHandle = pSemaphore;
            
        // ENDIF
        }
        
    // ENDIF
    }
    
    // return
    return (Result);
}



        
/*********************************************************************

    Function:       adi_sem_Delete

    Description:    Deletes a semaphore

*********************************************************************/

ADI_SEM_RESULT adi_sem_Delete(  
    ADI_SEM_HANDLE      Handle  // semaphore handle
) {
    
    ADI_SEM_RESULT      Result;             // result
	void                *pExitCriticalArg;  // return value from EnterCritical
	ADI_SEM_SEMAPHORE   *pSemaphore;        // pointer to a semaphore
    
    // ever the optimist
    Result = ADI_SEM_RESULT_SUCCESS;
    
    // avoid casts
    pSemaphore = (ADI_SEM_SEMAPHORE *)Handle;
    
    // insure we've been initialized and the handle is valid
#if defined(ADI_SSL_DEBUG)
    if (adi_sem_InstanceData.InitializedFlag == FALSE) {
        Result = ADI_SEM_RESULT_NOT_INITIALIZED;
    } else {
        Result = ValidateSemaphoreHandle(Handle);
    }
#endif

    // IF (no errors)
    if (Result == ADI_SEM_RESULT_SUCCESS) {
        
        // put the semaphore back in the free list
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_sem_InstanceData.pCriticalRegionArg);
        if (adi_sem_InstanceData.pFreeList == NULL) {
            adi_sem_InstanceData.pFreeList = pSemaphore;
            pSemaphore->pNext = NULL;
        } else {
            pSemaphore->pNext = adi_sem_InstanceData.pFreeList;
            adi_sem_InstanceData.pFreeList = pSemaphore;
        }
        adi_int_ExitCriticalRegion(pExitCriticalArg);

    // ENDIF
    }
    
    // return
    return (Result);
}




/*********************************************************************

    Function:       adi_sem_Pend

    Description:    Acquires a semaphore

*********************************************************************/

ADI_SEM_RESULT adi_sem_Pend(  
    ADI_SEM_HANDLE  Handle,     // semaphore handle
    u32             Timeout     // timeout
) {
    
    ADI_SEM_RESULT      Result;             // result
	void                *pExitCriticalArg;  // return value from EnterCritical
	ADI_SEM_SEMAPHORE   *pSemaphore;        // pointer to a semaphore
    
    // ever the optimist
    Result = ADI_SEM_RESULT_SUCCESS;
    
    // avoid casts
    pSemaphore = (ADI_SEM_SEMAPHORE *)Handle;
    
    // insure we've been initialized, the handle is valid
#if defined(ADI_SSL_DEBUG)
    if (adi_sem_InstanceData.InitializedFlag == FALSE) {
        Result = ADI_SEM_RESULT_NOT_INITIALIZED;
    } else {
        if ((Timeout != ADI_SEM_TIMEOUT_FOREVER) && (Timeout != ADI_SEM_TIMEOUT_IMMEDIATE)) {
            Result = ADI_SEM_RESULT_BAD_TIMEOUT_MODE;
        } else {
            Result = ValidateSemaphoreHandle(Handle);
        }
    }
#endif

    // IF (no errors)
    if (Result == ADI_SEM_RESULT_SUCCESS) {
        
        // LOOP 
        while (1) {
            
            // try and acquire the semaphore
            Result = ADI_SEM_RESULT_NOT_ACQUIRED;
            if (pSemaphore->Count) {
                pExitCriticalArg = adi_int_EnterCriticalRegion(adi_sem_InstanceData.pCriticalRegionArg);
                if (pSemaphore->Count) {
                    pSemaphore->Count--;
                    Result = ADI_SEM_RESULT_SUCCESS;
                }
                adi_int_ExitCriticalRegion(pExitCriticalArg);
            }
            
            // exit the loop if we got the semaphore
            if (Result == ADI_SEM_RESULT_SUCCESS) {
                break;
            }
            
            // exit the loop if the caller wanted to return immediately, even if not acquired
            if (Timeout == ADI_SEM_TIMEOUT_IMMEDIATE) {
                break;
            }
            
            // call the pending function if one was provided
            if (pSemaphore->PendingFunction) {
                (pSemaphore->PendingFunction)(Handle);
            }
                
        // ENDLOOP
        }
        
    // ENDIF
    }
    
    // return
    return (Result);
}

        
        
        
/*********************************************************************

    Function:       adi_sem_Post

    Description:    Posts a semaphore

*********************************************************************/

ADI_SEM_RESULT adi_sem_Post(  
    ADI_SEM_HANDLE  Handle      // semaphore handle
) {
    
    ADI_SEM_RESULT      Result;             // result
	void                *pExitCriticalArg;  // return value from EnterCritical
	ADI_SEM_SEMAPHORE   *pSemaphore;        // pointer to a semaphore
    
    // ever the optimist
    Result = ADI_SEM_RESULT_SUCCESS;
    
    // avoid casts
    pSemaphore = (ADI_SEM_SEMAPHORE *)Handle;
    
    // insure we've been initialized, the handle is valid
#if defined(ADI_SSL_DEBUG)
    if (adi_sem_InstanceData.InitializedFlag == FALSE) {
        Result = ADI_SEM_RESULT_NOT_INITIALIZED;
    } else {
        Result = ValidateSemaphoreHandle(Handle);
    }
#endif

    // IF (no errors)
    if (Result == ADI_SEM_RESULT_SUCCESS) {
        
        // post the semaphore
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_sem_InstanceData.pCriticalRegionArg);
        pSemaphore->Count++;
        adi_int_ExitCriticalRegion(pExitCriticalArg);

        // call the posting function if one was provided
        if (pSemaphore->PostingFunction) {
            (pSemaphore->PostingFunction)(Handle);
        }
                
    // ENDIF
    }
    
    // return
    return (Result);
}
    
                
        
        
/*********************************************************************

    Function:       adi_sem_Query

    Description:    Queries a semaphore count

*********************************************************************/

ADI_SEM_RESULT adi_sem_Query(  
    ADI_SEM_HANDLE  Handle,     // semaphore handle
    u32             *pCount     // location where the semaphore count is stored
) {
    
    ADI_SEM_RESULT      Result;             // result
	void                *pExitCriticalArg;  // return value from EnterCritical
	ADI_SEM_SEMAPHORE   *pSemaphore;        // pointer to a semaphore
    
    // ever the optimist
    Result = ADI_SEM_RESULT_SUCCESS;
    
    // avoid casts
    pSemaphore = (ADI_SEM_SEMAPHORE *)Handle;
    
    // insure we've been initialized, the handle is valid
#if defined(ADI_SSL_DEBUG)
    if (adi_sem_InstanceData.InitializedFlag == FALSE) {
        Result = ADI_SEM_RESULT_NOT_INITIALIZED;
    } else {
        Result = ValidateSemaphoreHandle(Handle);
    }
#endif

    // IF (no errors)
    if (Result == ADI_SEM_RESULT_SUCCESS) {
        
        // get the count
        *pCount = pSemaphore->Count;

    // ENDIF
    }
    
    // return
    return (Result);
}




#ifdef ADI_SSL_DEBUG

/*********************************************************************

    Function:       ValidateSemaphoreHandle

    Description:    Validates a semaphore handle

*********************************************************************/
ADI_SEM_RESULT ValidateSemaphoreHandle(     // validates a semaphore handle
    ADI_SEM_HANDLE  Handle      // semaphore handle
) {
    
    ADI_SEM_RESULT      Result;             // result
	ADI_SEM_SEMAPHORE   *pSemaphore;        // pointer to a semaphore
	u32                 i;                  // counter
    
    // be a pessimist
    Result = ADI_SEM_RESULT_BAD_HANDLE;
    
    // find the semaphore
    for (i = adi_sem_InstanceData.Count, pSemaphore = adi_sem_InstanceData.pSemaphores; i; i--, pSemaphore++) {
        if (pSemaphore == (ADI_SEM_SEMAPHORE *)Handle) {
            Result = ADI_SEM_RESULT_SUCCESS;
            break;
        }
    }
    
    // return
    return (Result);
}
    
    
    
    
#endif

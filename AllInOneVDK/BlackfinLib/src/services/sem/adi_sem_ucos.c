/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_sem_ucos.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			This file contains the uC/OS-II code for the semaphore service

*********************************************************************************/

#include <services/services.h>

#include "../rtos/adi_rtos.h"           // uC/OS-II API

/*******************************************************************************

    Data Structures

********************************************************************************/
    
typedef struct adi_sem_semaphore {
} ADI_SEM_SEMAPHORE;

typedef struct {
    u8                  InitializedFlag;    // initialized flag (FALSE - not initialized, TRUE - initialized)
} ADI_SEM_INSTANCE_DATA;

ADI_SEM_INSTANCE_DATA adi_sem_InstanceData;	// address of instance data for the semaphore service


/*******************************************************************************

    Static functions

********************************************************************************/

#ifdef ADI_SSL_DEBUG


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
    
    ADI_SEM_RESULT Result;  // return code
    
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
        
        // initialize the service
        adi_sem_InstanceData.InitializedFlag = TRUE;
        
    // ENDIF
    }
    
    // return
    return (Result);
}



/*********************************************************************

    Function:       adi_sem_Terminate

    Description:    Terminates the semaphore service

*********************************************************************/

ADI_SEM_RESULT adi_sem_Terminate(void) {   // terminates the semaphore service
    
    ADI_SEM_RESULT Result;  // return code
    
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
        
        // terminate the service
        adi_sem_InstanceData.InitializedFlag = FALSE;
        
    // ENDIF
    }
    
    // return
    return (Result);
}



/*********************************************************************

    Function:       adi_sem_Create

    Description:    Creates a semaphore

*********************************************************************/

ADI_SEM_RESULT adi_sem_Create(  
    u32                     Count,          // initial count
    ADI_SEM_HANDLE          *pHandle,       // location where the handle to the semaphore is stored
    ADI_SEM_OS_ATTRIBUTES   *pOSAttributes  // pointer to the semaphore's attributes
) {
    
    ADI_SEM_RESULT      Result;             // result
    
    // uC/OS-II doesn't use an error code for this function so always return success
    Result = ADI_SEM_RESULT_SUCCESS;
    
    // insure we've been initialized
#if defined(ADI_SSL_DEBUG)
    if (adi_sem_InstanceData.InitializedFlag == FALSE) {
        Result = ADI_SEM_RESULT_NOT_INITIALIZED;
    }
#endif

    // IF (no errors) 
    if (Result == ADI_SEM_RESULT_SUCCESS) {
        
        // create it
        *pHandle = (ADI_SEM_HANDLE)OSSemCreate(Count);
        
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
    
    // uC/OS-II doesn't support this function so always return success for compatibility reasons
    Result = ADI_SEM_RESULT_SUCCESS;
    
    // insure we've been initialized 
#if defined(ADI_SSL_DEBUG)
    if (adi_sem_InstanceData.InitializedFlag == FALSE) {
        Result = ADI_SEM_RESULT_NOT_INITIALIZED;
    }
#endif

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
    
    ADI_SEM_RESULT  Result;     // result
    u8              OSResult;   // return value from the RTOS
    
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
        
        // IF (the caller doesn't want to suspend)
        if (Timeout == ADI_SEM_TIMEOUT_IMMEDIATE) {
            
            // use OSSemAccept to get the semaphore
            if (OSSemAccept(Handle) == 0) {
                Result = ADI_SEM_RESULT_NOT_ACQUIRED;
            }
            
        // ELSE 
        } else {
            
            // use OSSemPend for timeouts, OSSemAccept 
            OSSemPend(Handle, Timeout, &OSResult);
        
            // convert the return code
            if (OSResult != OS_NO_ERR) {
                Result = ADI_SEM_RESULT_OS_ERROR;
            }
            
        // ENDIF
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
    
    ADI_SEM_RESULT  Result;     // result
    u8              OSResult;   // return value from the RTOS
    
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
        
        // post the semaphore to uC/OS-II
        OSResult = OSSemPost(Handle);
        
        // convert the return code
        if (OSResult != OS_NO_ERR) {
            Result = ADI_SEM_RESULT_OS_ERROR;
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
    
    ADI_SEM_RESULT  Result;     // result
    u8              OSResult;   // return value from the RTOS
    OS_SEM_DATA     Count;      // count from uC/OS-II
    
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
        
        // query the semaphore count
        OSResult = OSSemQuery(Handle, &Count);
        
        // convert the error and store the value in the caller's location
        if (OSResult == OS_NO_ERR) {
            *pCount = Count.OSCnt;
        } else {
            Result = ADI_SEM_RESULT_OS_ERROR;
        }
        
    // ENDIF
    }
    
    // return
    return (Result);
}





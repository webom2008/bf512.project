/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_sem_vdk.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
			This file contains the VDK code for the semaphore service

*********************************************************************************/

#include <services/services.h>

#include "../rtos/adi_rtos.h"        // VDK API




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
        
        // create it (watch for the case of no attributes)
        if (pOSAttributes != NULL) {
            *pHandle = (ADI_SEM_HANDLE)VDK_CreateSemaphore(Count, pOSAttributes->inMaxCount, pOSAttributes->inInitialDelay, pOSAttributes->inPeriod);
        } else {
            *pHandle = (ADI_SEM_HANDLE)VDK_CreateSemaphore(Count, UINT_MAX, 1, 0);
        }
        
        // check for error
        if (*pHandle == (ADI_SEM_HANDLE)UINT_MAX) {
            Result = ADI_SEM_RESULT_OS_ERROR;
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

        // destroy it
        VDK_DestroySemaphore((VDK_SemaphoreID)Handle);
        
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
    
    ADI_SEM_RESULT  Result;     // result
    
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
        
        // convert the timeout value to a VDK enumeration
        if (Timeout == ADI_SEM_TIMEOUT_IMMEDIATE) {
            Timeout = kDoNotWait;
        }
        
        // acquire the semaphore
        if (VDK_PendSemaphore((VDK_SemaphoreID)Handle, Timeout) == FALSE) {
            Result = ADI_SEM_RESULT_NOT_ACQUIRED;
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
    u32             CurrentIVG; // current interrupt level we're running at
    
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
                
        // post the semaphore to the VDK taking into account if we're running from ISR context
    	if ((adi_int_GetCurrentIVGLevel(&CurrentIVG) == ADI_INT_RESULT_NOT_ASSERTED) || (CurrentIVG > 13)) {
            VDK_PostSemaphore((VDK_SemaphoreID)Handle);
        } else {
            VDK_C_ISR_PostSemaphore((VDK_SemaphoreID)Handle);
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
        if ((*pCount = VDK_GetSemaphoreValue((VDK_SemaphoreID)Handle)) == UINT_MAX) {
            Result = ADI_SEM_RESULT_OS_ERROR;
        }
        
    // ENDIF
    }
    
    // return
    return (Result);
}





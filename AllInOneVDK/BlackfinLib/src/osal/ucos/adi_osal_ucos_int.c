/* $Revision: 3578 $
 * $Date: 2010-10-19 09:36:04 -0400 (Tue, 19 Oct 2010) $
******************************************************************************
Copyright (c), 2008-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Operating System Abstraction Layer - OSAL for uCOS - Interrupt functions

Description:
              This file contains the Interrupts handling API's for the uCOS
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
#include "os_cpu.h"
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
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exits (mostly for handling parameter error checking) increases the code readability and therefore maintenance")
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
  Function: adi_osal_ISRInstall

    This function is used to register a function which is called from
    "ISR entry function". Each IVG level has its own entry function.
     OSAL takes care of storing and restoring the processor register before
     and after execution of the registered function respectively.

  Parameters:
     eIvgLevel      - IVG level for which the function will be installed
     pISRFunc       - Pointer to the function to be called
     bAllowNesting  - true: nesting of ISR allowed
                      false: no nesting allowed
  Returns:
      ADI_OSAL_SUCCESS          - if ISR is installed successfully
      ADI_OSAL_BAD_IVG_LEVEL    - Invalid IVG level
      ADI_OSAL_IVG_LEVEL_IN_USE - Specified IVG level is already registered with
                                  a different function
      ADI_OSAL_CALLER_ERROR     - If function is invoked from an invalid location (i.e an ISR)

  NOTE:
      No function can be registered for "emulator exception" and "Exception".
      This function should not be called from ISR.
*****************************************************************************/

ADI_OSAL_STATUS adi_osal_ISRInstall(ADI_OSAL_IVG eIvgLevel,
                                    ADI_OSAL_ISR_PTR pISRFunc,
                                    bool bAllowNesting)
{
    ADI_OSAL_STATUS eRetStatus=ADI_OSAL_BAD_IVG_LEVEL;

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if (((eIvgLevel >= ADI_OSAL_INVALID_IVG) ||
         (eIvgLevel < ADI_OSAL_IVG_HWERR)) &&
        (eIvgLevel != ADI_OSAL_IVG_NMI))
    {
        return (ADI_OSAL_BAD_IVG_LEVEL);
    }

#endif /* OSAL_DEBUG */

   /* Lock the scheduler since other task may access the adi_osal_gIsrInfoTable and
    Event vector table which are global resource */
    PAUSE_PROFILING();
    OSSchedLock();
    RESUME_PROFILING();

    if (_adi_osal_gIsrInfoTable[eIvgLevel].pIsrFunction == &_adi_osal_UnregisteredInterrupt)
    {
        _adi_osal_gIsrInfoTable[eIvgLevel].pIsrFunction = pISRFunc;
        PAUSE_PROFILING();
        OS_CPU_RegisterHandler((INT8U) eIvgLevel, (FNCT_PTR) pISRFunc, (BOOLEAN) bAllowNesting);
        RESUME_PROFILING();
        eRetStatus = ADI_OSAL_SUCCESS;
    }
    else  /* ISR already registered for this level */
    {
        if(_adi_osal_gIsrInfoTable[eIvgLevel].pIsrFunction != pISRFunc)
        {
            eRetStatus=ADI_OSAL_IVG_LEVEL_IN_USE;
        }
        else
        /*"else" is the case where user trying to register function whose
           address is same as content of "adi_osal_gIsrInfoTable[eIvgLevel].pIsrFunction".
           returns ADI_OSAL_SUCCESS.
        */
        {
            eRetStatus = ADI_OSAL_SUCCESS;
        }
    }

    PAUSE_PROFILING();
    OSSchedUnlock();
    RESUME_PROFILING();

    return (eRetStatus);
}



/*****************************************************************************
   Function: adi_osal_ISRUninstall

    This service can be used to un-install function which was registred  for an
    an ISR  of IVG level "eIvgLevel".

   Parameters:
     eIvgLevel - IVG level to which ISR to be uninstalled.
   Returns:
      ADI_OSAL_SUCCESS    - If ISR is uninstalled successfully.
      ADI_OSAL_BAD_IVG_LEVEL     - Invalid IVG level.
      ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid location (i.e an ISR)
Note:
    No function can be un-registered for "emulator exception" and "Exception".
      This function should not be called from ISR.
*****************************************************************************/

ADI_OSAL_STATUS adi_osal_ISRUninstall( ADI_OSAL_IVG eIvgLevel )
{

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if (((eIvgLevel >= ADI_OSAL_INVALID_IVG) ||
         (eIvgLevel < ADI_OSAL_IVG_HWERR)) &&
        (eIvgLevel != ADI_OSAL_IVG_NMI))
    {
        return (ADI_OSAL_BAD_IVG_LEVEL);
    }

#endif /* OSAL_DEBUG */

    /*   Lock the scheduler since other task may access the adi_osal_gIsrInfoTable
         which is a global resource.
    */
    PAUSE_PROFILING();
    OSSchedLock();
    RESUME_PROFILING();

    _adi_osal_gIsrInfoTable[eIvgLevel].pIsrFunction =  &_adi_osal_UnregisteredInterrupt;
    PAUSE_PROFILING();
    OS_CPU_RegisterHandler((INT8U) eIvgLevel, (FNCT_PTR) &_adi_osal_UnregisteredInterrupt, (BOOLEAN) NOT_NESTED);
    OSSchedUnlock();
    RESUME_PROFILING();

    return( ADI_OSAL_SUCCESS );
}

#pragma diag(pop)  /* global file supressions */

/*
**
** EOF: 
**
*/

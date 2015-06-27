/* $Revision: 3492 $
 * $Date: 2010-10-07 09:34:20 -0400 (Thu, 07 Oct 2010) $ 
******************************************************************************
Copyright (c), 2008-2010 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Operating System Abstraction Layer - OSAL for no-OS - Interrupt functions

Description:
              This file contains the Interrupts handling API's for the no-OS
              implementation of the OSAL


SUPRESSED MISRA RULES ARE:

        See adi_osal_noos.c for the list of the MISRA Rules that have been suppressed for the no-OS OSAL.


*****************************************************************************/

/*=============  I N C L U D E S   =============*/

#include <stdlib.h>                                                             /* for NULL definition */
#include <osal/adi_osal.h>
#include "osal_noos.h"
#include "../include/osal_common.h"



#pragma diag(push)
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exits (mostly for handling parameter error checking) increases the code readability and therefore maintenance")
/* Rule-16.2 indicates that  Functions shall not call themselves,either directly or indirectly */
#pragma diag(suppress:misra_rule_16_2 : "Since the OSAL is reentrant by nature (several thread could call the API) the compiler MISRA checker mistakes sometimes the reentrancy for recurrence")
/* Rule-5.1 indicates that all identifiers shall not rely on more than 31 characters of significance */
#pragma diag(suppress:misra_rule_5_1:"prefixes added in front of identifiers quickly increases their size. In order to keep the code self explanatory, and since ADI tools are the main targeted tools, this restriction is not really a necessity")


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
    if (_adi_osal_IsCurrentLevelISR())
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

   /* Lock the scheduler since other "threads" may access the adi_osal_gIsrInfoTable and
    Event vector table which are global resource */
    if (_adi_osal_AcquireGlobalLock() != true)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if (_adi_osal_gIsrInfoTable[eIvgLevel].pIsrFunction == &_adi_osal_UnregisteredInterrupt)
    {
        _adi_osal_gIsrInfoTable[eIvgLevel].pIsrFunction = pISRFunc;
/*RULE 11.1 is supressed since _adi_osal_apNestedEvtAddressbuffer is extern with datatype "void*"
while it is typecasted to ex_handler_fn.
Registering an ISR entry point for the specified IVG level
*/
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_1)

        if ( bAllowNesting == true )
        {
            register_handler ((interrupt_kind)eIvgLevel,(ex_handler_fn)_adi_osal_apNestedEvtAddressbuffer[eIvgLevel]);
        }
        else
        {
            register_handler ((interrupt_kind)eIvgLevel,(ex_handler_fn)_adi_osal_apNonNestedEvtAddressbuffer[eIvgLevel]);
        }
#pragma diag(pop)
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
    _adi_osal_ReleaseGlobalLock();

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
    uint32_t nIntState;

#ifdef OSAL_DEBUG
    if (_adi_osal_IsCurrentLevelISR())
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
    ENTER_CRITICAL_INTERNAL(nIntState);
    _adi_osal_gIsrInfoTable[eIvgLevel].pIsrFunction =  &_adi_osal_UnregisteredInterrupt;
    EXIT_CRITICAL_INTERNAL(nIntState);

    return( ADI_OSAL_SUCCESS );
}




#pragma diag(pop)  /* global file supressions */

/*
**
** EOF:
**
*/


/*********************************************************************************

Copyright(c) 2006-2010 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

Description:
            This source code is used when the interrupt manager runs in uCOS mode.

*********************************************************************************/


#include <rtos\adi_rtos.h>

/*********************************************************************

Static data

*********************************************************************/

/* saved value of IMASK is placed in L1 for Moab and Kookaburra/Mockingbird parts,
 * otherwise it goes where it goes!
*/
#if defined(__ADSP_MOAB__) || defined(__ADSP_KOOKABURRA__) || defined(__ADSP_MOCKINGBIRD__)
#pragma section ("L1_data")
#endif
static ADI_INT_IMASK SavedIMask;

static u32              CriticalRegionUsageCount = 0;   // critical region usage counter


/*********************************************************************

    Function:       adi_int_EnterCriticalRegion

    Description:    Protects a critical region of code.

                    This function takes whatever action is necessary to
                    protect a critical region of code.

                    In the standalone version of this function, the IMASK
                    register is copied to a holding location.  Interrupts are
                    then disabled and the function returns to the caller.

                    This function, and the sister adi_int_ExitCriticalRegion
                    function employ a usage counter so that if the functions
                    are called in a nesting manner, only at the outermost
                    calls is IMASK saved and restored.

                    In the standalone version, the pArg parameter is not
                    used.  However, to insure compatibility with versions of
                    this function in other operating environments, the value
                    that is returned from this function should always
                    be passed to the adi_int_ExitCriticalRegion function.

*********************************************************************/

void *adi_int_EnterCriticalRegion(      // protects a critical region of code
    void *pArg                              // pointer to enter critical data
)
{
    ADI_INT_IMASK tmpIMask = cli();

    // IF (we're not already in a critical region)
    if (CriticalRegionUsageCount++ == 0) {
        // save IMASK and disable all interrupts
        SavedIMask = tmpIMask;
    // ENDIF
    }

    // return
    return((void *)SavedIMask);

}




/*********************************************************************

    Function:       adi_int_ExitCriticalRegion

    Description:    Unprotects a critical region of code.  See notes in
                    adi_int_EnterCriticalRegion.

                    In the standalone version of this function, the IMASK
                    register value that was copied to a holding location
                    by the adi_int_EnterCriticalRegion function is restored
                    to IMASK.

                    This function, and the sister adi_int_EnterCriticalRegion
                    function employ a usage counter so that if the functions
                    are called in a nesting manner, only at the outermost
                    calls is IMASK saved and restored.

                    In the standalone version, the pArg parameter is not
                    used.  However, to insure compatibility with versions of
                    this function in other operating environments, the value
                    that is returned from the adi_int_EnterCriticalRegion
                    function should always be passed to this function.


*********************************************************************/

void adi_int_ExitCriticalRegion(    // unprotects a critical region of code
    void *pArg                          // pointer to exit critical region data
)
{

    // IF (after decrementing the usage count, this is the outermost ExitCritical call
    if ((--CriticalRegionUsageCount) == 0) {

        // reenable interrupts
        sti(SavedIMask);

    // ENDIF
    }

}




/*********************************************************************

    Function:       adi_int_SetIMaskBits

    Description:    Sets bits in the IMASK register.

                    This function enables the given events in the CEC
                    IMASK register.

*********************************************************************/

void adi_int_SetIMaskBits(      // sets bits in the IMASK register
    ADI_INT_IMASK BitsToSet         // bits that are to be set
)
{

    void    *pExitCriticalArg;  // parameter for exit critical


    // IF (we're already in a critical region)
    if (CriticalRegionUsageCount) {

        // update the saved copy of IMask
        SavedIMask |= BitsToSet;

    // ELSE
    } else {

        // protect us
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_int_InstanceData.pEnterCriticalArg);
        // update the saved copy of IMask
        SavedIMask |= BitsToSet;

        // unprotect us
        adi_int_ExitCriticalRegion(pExitCriticalArg);

    // ENDIF
    }

}



/*********************************************************************

    Function:       adi_int_ClearIMaskBits

    Description:    Clears bits in the IMASK register.

                    This function disables the given events in the CEC
                    IMASK register.

*********************************************************************/

void adi_int_ClearIMaskBits(    // clears bits in the IMASK register
    ADI_INT_IMASK BitsToClear       // bits that are to be cleared
)
{

    void    *pExitCriticalArg;  // parameter for exit critical

    // IF (we already in a critical region)
    if (CriticalRegionUsageCount) {

        // update the saved copy of IMask
        SavedIMask &= ~BitsToClear;

    // ELSE
    } else {

        // protect us
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_int_InstanceData.pEnterCriticalArg);
        // update the saved copy of IMask
        SavedIMask &= ~BitsToClear;

        // unprotect us
        adi_int_ExitCriticalRegion(pExitCriticalArg);

    // ENDIF
    }

}

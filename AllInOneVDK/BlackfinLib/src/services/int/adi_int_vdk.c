/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_int_vdk.c,v $
$Revision: 4 $
$Date: 2009-07-28 11:25:38 -0400 (Tue, 28 Jul 2009) $

Description:
            This source code is used when the interrupt manager runs in VDK mode.

*********************************************************************************/


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

                    This function should mask interrupts and/or grabs a semaphore to protect
                    a region of code. Note that in single core systems,
                    typically only an IMASK is used.  In multi-core systems,
                    the use of IMASK may not be sufficient as masking interrupts
                    does not stop a different core from accessing the critical
                    region.  As such, multi-core systems typically use both
                    interrupt masking, to prevent the given core from accessing
                    the critical region, and the semaphore, to prevent other
                    cores from accessing the region.  Also note that because
                    even in multi-core devices, when the interrupt manager
                    needs to protect a section of code, because each core has
                    its own SIC and CEC, only IMASK is needed.

                    For single core devices, the default behavior of these routines
                    simply disables and enables interrupts.

                    The value that is returned from this function should always
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

    Description:    Restores IMASK and/or returns a semaphore to unprotect
                    a region of code.  See notes in adi_int_EnterCriticalRegion.

                    The value that is passed into this function should always be
                    the value returned from the adi_int_EnterCriticalRegion function.

                    This function takes a pointer to an ADI_INT_CRITICAL_REGION_DATA
                    structure.  The IMask value within the structure is the IMask
                    value that will be restored into IMask.

*********************************************************************/

void adi_int_ExitCriticalRegion(    // unprotects a critical region of code
    void *pArg                          // pointer to exit critical region data
)
{

    // IF (after decrementing the usage count, this is the outermost ExitCritical call
    if ((--CriticalRegionUsageCount) == 0) {
        // reenable interrupts
        sti(SavedIMask);
        ssync();
    // ENDIF
    }
}




/*********************************************************************

    Function:       adi_int_SetIMaskBits

    Description:    Sets bits in the IMASK register.

                    This function enables the given events in the CEC
                    IMASK register.

*********************************************************************/

void adi_int_SetIMaskBits(      // protects a critical region of code
    ADI_INT_IMASK IMask             // pointer to enter critical data
)
{

    void    *pExitCriticalArg;  // parameter for exit critical

    // IF (we're already in a critical region)
    if (CriticalRegionUsageCount) {

        // update the saved copy of IMask
        SavedIMask |= IMask;

    // ELSE
    } else {

        // protect us
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_int_InstanceData.pEnterCriticalArg);
        // update the saved copy of IMask
        SavedIMask |= IMask;

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

void adi_int_ClearIMaskBits(        // protects a critical region of code
    ADI_INT_IMASK IMask             // pointer to enter critical data
)
{

    void    *pExitCriticalArg;  // parameter for exit critical

    // IF (we already in a critical region)
    if (CriticalRegionUsageCount) {

        // update the saved copy of IMask
        SavedIMask &= ~IMask;

    // ELSE
    } else {

        // protect us
        pExitCriticalArg = adi_int_EnterCriticalRegion(adi_int_InstanceData.pEnterCriticalArg);
        // update the saved copy of IMask
        SavedIMask &= ~IMask;

        // unprotect us
        adi_int_ExitCriticalRegion(pExitCriticalArg);

    // ENDIF
    }

}



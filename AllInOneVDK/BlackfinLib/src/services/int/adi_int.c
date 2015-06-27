/*********************************************************************************

Copyright(c) 2004-2010 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$Revision: 5506 $
$Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $

Description:
            This is the source code for the interrupt manager.

            Note that one of the following macros must be defined.  These macros
            are used to include the proper implementation code for the specified
            operating environment:

                o ADI_SSL_STANDALONE    - no RTOS (standalone operating environment)
                o ADI_SSL_VDK           - VDK operating environment
                o ADI_SSL_THREADX       - ThreadX operating environment
                o ADI_SSL_INTEGRITY     - Integrity operating environment
                o ADI_SSL_UCOS          - uCOS-II operating environment

*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/
#include "adi_int_module.h"



/*********************************************************************

Processor specific defines

*********************************************************************/

/********************
    Edinburgh
********************/

#if defined(__ADSP_EDINBURGH__)     // register addresses for BF531, BF532, BF533

#define ADI_INT_EVT         ((u32 *)0xffe02000)                     // address of event vector table
#define ADI_INT_CEC_IMASK   ((volatile ADI_INT_IMASK *)0xffe02104)  // address of first CEC IMASK register
#define ADI_INT_CEC_IPEND   ((volatile ADI_INT_IMASK *)0xffe02108)  // address of first CEC IPEND register
#define ADI_INT_SIC_IMASK0  ((volatile u32 *)0xffc0010c)            // address of first SIC IMASK register
#define ADI_INT_SIC_IAR0    ((volatile u32 *)0xffc00110)            // address of first SIC IAR register
#define ADI_INT_SIC_ISR0    ((volatile u32 *)0xffc00120)            // address of first SIC ISR register
#define ADI_INT_SIC_IWR0    ((volatile u32 *)0xffc00124)            // address of first SIC IWR register

#endif




/********************
    Braemar
********************/

#if defined(__ADSP_BRAEMAR__)   // register addresses for BF534, BF536, BF537

#define ADI_INT_EVT         ((u32 *)0xffe02000)                     // address of event vector table
#define ADI_INT_CEC_IMASK   ((volatile ADI_INT_IMASK *)0xffe02104)  // address of first CEC IMASK register
#define ADI_INT_CEC_IPEND   ((volatile ADI_INT_IMASK *)0xffe02108)  // address of first CEC IPEND register
#define ADI_INT_SIC_IMASK0  ((volatile u32 *)0xffc0010c)            // address of first SIC IMASK register
#define ADI_INT_SIC_IAR0    ((volatile u32 *)0xffc00110)            // address of first SIC IAR register
#define ADI_INT_SIC_ISR0    ((volatile u32 *)0xffc00120)            // address of first SIC ISR register
#define ADI_INT_SIC_IWR0    ((volatile u32 *)0xffc00124)            // address of first SIC IWR register

#endif



/********************
    Teton-Lite
********************/

#if defined(__ADSPBF561__)          // register addresses for BF561

#define ADI_INT_EVT         ((u32 *)0xffe02000)                     // address of event vector table
#define ADI_INT_CEC_IMASK   ((volatile ADI_INT_IMASK *)0xffe02104)  // address of first CEC IMASK register
#define ADI_INT_CEC_IPEND   ((volatile ADI_INT_IMASK *)0xffe02108)  // address of first CEC IPEND register
#define ADI_INT_SICA_IMASK0 ((volatile u32 *)0xffc0010c)            // address of first SICA IMASK register
#define ADI_INT_SICA_IAR0   ((volatile u32 *)0xffc00124)            // address of first SICA IAR register
#define ADI_INT_SICA_ISR0   ((volatile u32 *)0xffc00114)            // address of first SICA ISR register
#define ADI_INT_SICA_IWR0   ((volatile u32 *)0xffc0011c)            // address of first SICA IWR register
#define ADI_INT_SICA_IWR1   ((volatile u32 *)0xffc00120)            // address of second SICA IWR register
#define ADI_INT_SICB_IMASK0 ((volatile u32 *)0xffc0110c)            // address of first SICB IMASK register
#define ADI_INT_SICB_IAR0   ((volatile u32 *)0xffc01124)            // address of first SICB IAR register
#define ADI_INT_SICB_ISR0   ((volatile u32 *)0xffc01114)            // address of first SICB ISR register
#define ADI_INT_SICB_IWR0   ((volatile u32 *)0xffc0111c)            // address of first SICB IWR register
#define ADI_INT_SICB_IWR1   ((volatile u32 *)0xffc01120)            // address of second SICB IWR register

#define ADI_INT_SIC_IMASK0  ((*ADI_SRAM_BASE_ADDR_MMR == ADI_COREA)?(ADI_INT_SICA_IMASK0):(ADI_INT_SICB_IMASK0))
#define ADI_INT_SIC_IAR0    ((*ADI_SRAM_BASE_ADDR_MMR == ADI_COREA)?(ADI_INT_SICA_IAR0):(ADI_INT_SICB_IAR0))
#define ADI_INT_SIC_ISR0    ((*ADI_SRAM_BASE_ADDR_MMR == ADI_COREA)?(ADI_INT_SICA_ISR0):(ADI_INT_SICB_ISR0))
#define ADI_INT_SIC_IWR0    ((*ADI_SRAM_BASE_ADDR_MMR == ADI_COREA)?(ADI_INT_SICA_IWR0):(ADI_INT_SICB_IWR0))
#define ADI_INT_SIC_IWR1    ((*ADI_SRAM_BASE_ADDR_MMR == ADI_COREA)?(ADI_INT_SICA_IWR1):(ADI_INT_SICB_IWR1))

#endif



/********************
    Stirling
********************/

#if defined(__ADSP_STIRLING__)  // register addresses for BF538, BF539

#define ADI_INT_EVT         ((u32 *)0xffe02000)                     // address of event vector table
#define ADI_INT_CEC_IMASK   ((volatile ADI_INT_IMASK *)0xffe02104)  // address of first CEC IMASK register
#define ADI_INT_CEC_IPEND   ((volatile ADI_INT_IMASK *)0xffe02108)  // address of first CEC IPEND register
#define ADI_INT_SIC_IMASK0  ((volatile u32 *)0xffc0010c)            // address of first SIC IMASK register
#define ADI_INT_SIC_IAR0    ((volatile u32 *)0xffc00110)            // address of first SIC IAR register
#define ADI_INT_SIC_ISR0    ((volatile u32 *)0xffc00120)            // address of first SIC ISR register
#define ADI_INT_SIC_IWR0    ((volatile u32 *)0xffc00124)            // address of first SIC IWR register
#define ADI_INT_SIC_IWR1    ((volatile u32 *)0xffc00130)            /* address of second Interrupt Wakeup register */

#endif



/********************
    Moab
********************/

#if defined(__ADSP_MOAB__)  // register addresses for BF54x

#define ADI_INT_EVT         ((u32 *)0xffe02000)                     // address of event vector table
#define ADI_INT_CEC_IMASK   ((volatile ADI_INT_IMASK *)0xffe02104)  // address of first CEC IMASK register
#define ADI_INT_CEC_IPEND   ((volatile ADI_INT_IMASK *)0xffe02108)  // address of first CEC IPEND register
#define ADI_INT_SIC_IMASK0  ((volatile u32 *)0xffc0010c)            // address of first SIC IMASK register
#define ADI_INT_SIC_IAR0    ((volatile u32 *)0xffc00130)            // address of first SIC IAR register
#define ADI_INT_SIC_ISR0    ((volatile u32 *)0xffc00118)            // address of first SIC ISR register
#define ADI_INT_SIC_IWR0    ((volatile u32 *)0xffc00124)            // address of first SIC IWR register
#define ADI_INT_SIC_IWR1    ((volatile u32 *)0xffc00128)            /* address of second Interrupt Wakeup register */
#define ADI_INT_SIC_IWR2    ((volatile u32 *)0xffc0012C)            /* address of third Interrupt Wakeup register */

#endif


/**************************************
    Kookaburra/Mockingbird/Brodie/Moy
**************************************/

/* register addresses for BF52x, BF51x & BF50x */
#if defined(__ADSP_KOOKABURRA__)  ||\
    defined(__ADSP_MOCKINGBIRD__) ||\
    defined(__ADSP_BRODIE__)      ||\
    defined(__ADSP_MOY__)

#define ADI_INT_EVT         ((u32 *)0xffe02000)                     // address of event vector table
#define ADI_INT_CEC_IMASK   ((volatile ADI_INT_IMASK *)0xffe02104)  // address of first CEC IMASK register
#define ADI_INT_CEC_IPEND   ((volatile ADI_INT_IMASK *)0xffe02108)  // address of first CEC IPEND register
#define ADI_INT_SIC_IMASK0  ((volatile u32 *)0xffc0010c)            // address of first SIC IMASK register
#define ADI_INT_SIC_IAR0    ((volatile u32 *)0xffc00110)            // address of first SIC IAR register
#define ADI_INT_SIC_ISR0    ((volatile u32 *)0xffc00120)            // address of first SIC ISR register
#define ADI_INT_SIC_IWR0    ((volatile u32 *)0xffc00124)            // address of first SIC IWR register
#define ADI_INT_SIC_IWR1    ((volatile u32 *)0xffc00164)            /* address of second Interrupt Wakeup register */

#endif

/**********
    Delta
***********/

/* register addresses for BF90x*/
#if defined(__ADSP_DELTA__)

#define ADI_INT_EVT         ((u32 *)0xffe02000)                     // address of event vector table
#define ADI_INT_CEC_IMASK   ((volatile ADI_INT_IMASK *)0xffe02104)  // address of first CEC IMASK register
#define ADI_INT_CEC_IPEND   ((volatile ADI_INT_IMASK *)0xffe02108)  // address of first CEC IPEND register
#define ADI_INT_SIC_IMASK0  ((volatile u32 *)0xffc0010c)            // address of first SIC IMASK register
#define ADI_INT_SIC_IAR0    ((volatile u32 *)0xffc00110)            // address of first SIC IAR register
#define ADI_INT_SIC_ISR0    ((volatile u32 *)0xffc00120)            // address of first SIC ISR register
#define ADI_INT_SIC_IWR0    ((volatile u32 *)0xffc00124)            // address of first SIC IWR register

#endif



ADI_INT_INSTANCE_DATA adi_int_InstanceData; // address of instance data for the interrupt manager


/*********************************************************************

ISR address tables...

*********************************************************************/


extern  void
    adi_int_NestingISR_IVG0(),
    adi_int_NestingISR_IVG1(),
    adi_int_NestingISR_IVG2(),
    adi_int_NestingISR_IVG3(),
    adi_int_NestingISR_IVG4(),
    adi_int_NestingISR_IVG5(),
    adi_int_NestingISR_IVG6(),
    adi_int_NestingISR_IVG7(),
    adi_int_NestingISR_IVG8(),
    adi_int_NestingISR_IVG9(),
    adi_int_NestingISR_IVG10(),
    adi_int_NestingISR_IVG11(),
    adi_int_NestingISR_IVG12(),
    adi_int_NestingISR_IVG13(),
    adi_int_NestingISR_IVG14(),
    adi_int_NestingISR_IVG15();


void (*adi_int_NestingISR_Table[])() =
{
    adi_int_NestingISR_IVG0,
    adi_int_NestingISR_IVG1,
    adi_int_NestingISR_IVG2,
    adi_int_NestingISR_IVG3,
    adi_int_NestingISR_IVG4,
    adi_int_NestingISR_IVG5,
    adi_int_NestingISR_IVG6,
    adi_int_NestingISR_IVG7,
    adi_int_NestingISR_IVG8,
    adi_int_NestingISR_IVG9,
    adi_int_NestingISR_IVG10,
    adi_int_NestingISR_IVG11,
    adi_int_NestingISR_IVG12,
    adi_int_NestingISR_IVG13,
    adi_int_NestingISR_IVG14,
    adi_int_NestingISR_IVG15,
};


extern void
    adi_int_NonNestingISR_IVG0(),
    adi_int_NonNestingISR_IVG1(),
    adi_int_NonNestingISR_IVG2(),
    adi_int_NonNestingISR_IVG3(),
    adi_int_NonNestingISR_IVG4(),
    adi_int_NonNestingISR_IVG5(),
    adi_int_NonNestingISR_IVG6(),
    adi_int_NonNestingISR_IVG7(),
    adi_int_NonNestingISR_IVG8(),
    adi_int_NonNestingISR_IVG9(),
    adi_int_NonNestingISR_IVG10(),
    adi_int_NonNestingISR_IVG11(),
    adi_int_NonNestingISR_IVG12(),
    adi_int_NonNestingISR_IVG13(),
    adi_int_NonNestingISR_IVG14(),
    adi_int_NonNestingISR_IVG15();


void (*adi_int_NonNestingISR_Table[])() =
{
    adi_int_NonNestingISR_IVG0,
    adi_int_NonNestingISR_IVG1,
    adi_int_NonNestingISR_IVG2,
    adi_int_NonNestingISR_IVG3,
    adi_int_NonNestingISR_IVG4,
    adi_int_NonNestingISR_IVG5,
    adi_int_NonNestingISR_IVG6,
    adi_int_NonNestingISR_IVG7,
    adi_int_NonNestingISR_IVG8,
    adi_int_NonNestingISR_IVG9,
    adi_int_NonNestingISR_IVG10,
    adi_int_NonNestingISR_IVG11,
    adi_int_NonNestingISR_IVG12,
    adi_int_NonNestingISR_IVG13,
    adi_int_NonNestingISR_IVG14,
    adi_int_NonNestingISR_IVG15,
};



/*********************************************************************

Static functions

*********************************************************************/

#ifdef ADI_SSL_DEBUG
                                    // debug routine to check the IVG number
static ADI_INT_RESULT CheckIVG(u32 IVG) {
#ifdef ADI_SSL_UCOS
    if (IVG == 14)
    {
        return (ADI_INT_RESULT_INVALID_IVG);
    }
#endif

    if (IVG < num_interrupt_kind) {
        return (ADI_INT_RESULT_SUCCESS);
    }
    return (ADI_INT_RESULT_INVALID_IVG);
}

static ADI_INT_RESULT CheckPeripheralID(ADI_INT_PERIPHERAL_ID PeripheralID) {
    if (ADI_INT_GET_UNUSED_BITS(PeripheralID) == 0) {
        return (ADI_INT_RESULT_SUCCESS);
    }
    return (ADI_INT_RESULT_INVALID_PERIPHERAL_ID);
}

static ADI_INT_RESULT ValidateInterruptManager(void) {
    if (&adi_int_InstanceData == NULL) return(ADI_INT_RESULT_NOT_INITIALIZED);
    return (ADI_INT_RESULT_SUCCESS);
}

#endif


/*********************************************************************

Include operating environment specific functionality

*********************************************************************/

#if defined(ADI_SSL_STANDALONE)
#include "adi_int_standalone.c"
#endif

#if defined(ADI_SSL_VDK)
#include "adi_int_vdk.c"
#endif

#if defined(ADI_SSL_THREADX)
#include "adi_int_threadx.c"
#endif


#if defined(ADI_SSL_INTEGRITY)
#include "adi_int_integrity.c"
#endif

#if defined(ADI_SSL_UCOS)
#include "adi_int_ucos.c"
#endif

/*********************************************************************

    Function:       adi_int_Init

    Description:    Initializes the interrupt manager.  This function
                    populates the register addresses and initializes
                    table entries etc.

*********************************************************************/

ADI_INT_RESULT adi_int_Init(        // Initializes the interrupt manager
    void            *pMemory,           // pointer to memory
    const size_t    MemorySize,         // size of the memory (in bytes)
    u32             *pMaxEntries,       // number of secondary handlers supported
    void            *pEnterCriticalArg  // parameter for enter critical region
) {

    int i;                              // generic counter
    ADI_INT_HANDLER_ENTRY *pEntry;      // pointer to a handler entry

    // debug
#if defined(ADI_SSL_DEBUG)
    if (ADI_INT_SECONDARY_MEMORY != sizeof(ADI_INT_HANDLER_ENTRY)) {
        return (ADI_INT_RESULT_BAD_SECONDARY_MEMORY_SIZE);
    }
#endif

    // initialize the primary handler table
    for (i = 0, pEntry = adi_int_InstanceData.PrimaryTable; i < ADI_INT_IVG_COUNT; i++, pEntry++) {
        pEntry->Handler = NULL;
        pEntry->pNext = NULL;
    }

    // save the enter critical region parameter
    adi_int_InstanceData.pEnterCriticalArg = pEnterCriticalArg;

    // determine how many secondary handlers we can support and notify the application of such
    *pMaxEntries = MemorySize/(sizeof(ADI_INT_HANDLER_ENTRY));

    // initialize the secondary handler table
    if (*pMaxEntries) {
        pEntry = adi_int_InstanceData.FreeSecondaryList = (ADI_INT_HANDLER_ENTRY*)pMemory;
        for (i = *pMaxEntries; i; i--, pEntry++) {
            pEntry->pNext = pEntry + 1;
        }
        (pEntry - 1)->pNext = NULL;
    } else {
        adi_int_InstanceData.FreeSecondaryList = NULL;
    }

    // return
    return (ADI_INT_RESULT_SUCCESS);
}

/*********************************************************************

    Function:       adi_int_Terminate

    Description:    Unhooks all interrupt handlers and terminates the
                    interrupt manager.

*********************************************************************/

ADI_INT_RESULT adi_int_Terminate(   // Hooks the given interrupt handler into the handler chain
    void
) {

    u32                     IVG;                // IVG
    ADI_INT_HANDLER_ENTRY   *pPrimary;          // pointer to a handler entry
    ADI_INT_RESULT          Result;             // return value

    // assume success
    Result = ADI_INT_RESULT_SUCCESS;

    // debug
#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidateInterruptManager()) != ADI_INT_RESULT_SUCCESS) return (Result);
#endif

    // unhook all handlers
    for (IVG = 0, pPrimary = &adi_int_InstanceData.PrimaryTable[0]; IVG < ADI_INT_IVG_COUNT; IVG++, pPrimary++) {
        while (pPrimary->Handler != NULL) {
            if ((Result = adi_int_CECUnhook(IVG, pPrimary->Handler, pPrimary->ClientArg)) != ADI_INT_RESULT_SUCCESS) {
                return (Result);
            }
        }
    }

    // prevent the memory from being used again
    adi_int_InstanceData.FreeSecondaryList = NULL;

    // return
    return(Result);
}


/*********************************************************************

    Function:       adi_int_CECHook

    Description:    Hooks the given interrupt handler into the chain of
                    handlers for the given IVG.  Note that the NestingFlag
                    parameter for a given IVG level is only used when the
                    primary handler is installed on the IVG.

*********************************************************************/

ADI_INT_RESULT adi_int_CECHook(     // Hooks the given interrupt handler into the handler chain
    u32                 IVG,            // IVG level to hook into
    ADI_INT_HANDLER_FN  Handler,        // handler function address
    void                *ClientArg,     // client argument supplied to handler
    u32                 NestingFlag     // nesting enable flag (TRUE/FALSE)
) {

    u32                     Hooked;             // hooked flag
    ADI_INT_HANDLER_ENTRY   *pPrimary;          // pointer to the primary handler entry
    ADI_INT_HANDLER_ENTRY   *pEntry;            // pointer to a handler entry
    ADI_INT_RESULT          Result;             // return value
    void                    *pExitCriticalArg;  // parameter for exit critical

    // debug
#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidateInterruptManager()) != ADI_INT_RESULT_SUCCESS) return (Result);
    if ((Result = CheckIVG(IVG)) != ADI_INT_RESULT_SUCCESS) return (Result);
#if defined(ADI_SSL_VDK)
    if ((IVG == ADI_INT_IVG_EXCEPTION) || (IVG == ADI_INT_IVG_14) || (IVG == ADI_INT_IVG_15)) {
        return (ADI_INT_RESULT_DISALLOWED_BY_RTOS);
    }
#endif
#endif

    // ever the optimist
    Result = ADI_INT_RESULT_SUCCESS;

    // point to the primary handler entry for this IVG
    pPrimary = &adi_int_InstanceData.PrimaryTable[IVG];

    // protect us while we update the interrupt handler chain
    pExitCriticalArg = adi_int_EnterCriticalRegion(adi_int_InstanceData.pEnterCriticalArg);

    // IF (no handler is installed on this IVG, the given handler becomes the primary handler)
    if (pPrimary->Handler == NULL) {

        // store the handler address and parameter in the entry and increment the usage count
        pPrimary->Handler = Handler;
        pPrimary->ClientArg = ClientArg;
        pPrimary->UsageCount = 1;

        // install the ISR into the EVT for this IVG
        if (NestingFlag) {
            *(ADI_INT_EVT + IVG) = (u32)adi_int_NestingISR_Table[IVG];
        } else {
            *(ADI_INT_EVT + IVG) = (u32)adi_int_NonNestingISR_Table[IVG];
        }

        // enable the event in the CEC IMASK register
        adi_int_SetIMaskBits(1 << IVG);

    // ELSE
    } else {

        // update the usage counter if the handler with client arg is already in the chain
        Hooked = FALSE;
        for (pEntry = pPrimary; pEntry; pEntry = pEntry->pNext) {
            if ((pEntry->Handler == Handler) && (pEntry->ClientArg == ClientArg)) {
                pEntry->UsageCount++;
                Hooked = TRUE;
                break;
            }
        }

        // IF (the handler is not already in the chain)
        if (Hooked == FALSE) {

            // IF (a secondary entry is available)
            if (pEntry = adi_int_InstanceData.FreeSecondaryList) {

                // remove it from the free list
                adi_int_InstanceData.FreeSecondaryList = pEntry->pNext;

                // populate the secondary handler entry
                pEntry->Handler = Handler;
                pEntry->ClientArg = ClientArg;
                pEntry->UsageCount = 1;

                // insert the secondary handler entry into the chain
                pEntry->pNext = pPrimary->pNext;
                pPrimary->pNext = pEntry;

            // ELSE
            } else {

                // return an error
                Result = ADI_INT_RESULT_NO_MEMORY;

            // ENDIF
            }

        // ENDIF
        }

    // ENDIF
    }

    // unprotect us
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // return
    return(Result);
}




/*********************************************************************

    Function:       adi_int_CECUnhook

    Description:    Unhooks the given interrupt handler from the chain of
                    handlers for the given IVG.

*********************************************************************/

ADI_INT_RESULT adi_int_CECUnhook(   // Unhooks the given interrupt handler from the handler chain
    u32                 IVG,            // IVG level to unhook from
    ADI_INT_HANDLER_FN  Handler,        // handler function address
    void                *ClientArg      // client argument supplied to handler
) {

    ADI_INT_IMASK           IMask;              // storage for IMASK
    ADI_INT_HANDLER_ENTRY   *pPrimary;          // pointer to a handler entry
    ADI_INT_HANDLER_ENTRY   *pSecondary;        // pointer to a handler entry
    ADI_INT_HANDLER_ENTRY   *pPrevious;         // pointer to a handler entry
    ADI_INT_RESULT          Result;             // return code
    void                    *pExitCriticalArg;  // parameter for exit critical

    // debug
#if defined(ADI_SSL_DEBUG)
    if ((Result = ValidateInterruptManager()) != ADI_INT_RESULT_SUCCESS) return (Result);
    if ((Result = CheckIVG(IVG)) != ADI_INT_RESULT_SUCCESS) return (Result);
#endif

    // be a pessimist
    Result = ADI_INT_RESULT_HANDLER_NOT_FOUND;

    // point to the primary handler entry for this IVG
    pPrimary = &adi_int_InstanceData.PrimaryTable[IVG];

    // protect us while we update the interrupt handler chain
    pExitCriticalArg = adi_int_EnterCriticalRegion(adi_int_InstanceData.pEnterCriticalArg);

    //IF (there is a primary handler)
    if (pPrimary->Handler != NULL) {

        // IF (we're unhooking the primary handler on the IVG)
        if ((pPrimary->Handler == Handler) && (pPrimary->ClientArg == ClientArg)) {

            // decrement the usage counter
            pPrimary->UsageCount--;

            // IF (the handler is no longer needed)
            if (pPrimary->UsageCount == 0) {

                // IF (there is a secondary handler on the IVG)
                if (pSecondary = pPrimary->pNext) {

                    // put the secondary handler into the primary handler slot
                    *pPrimary = *pSecondary;

                    // free the secondary handler
                    pSecondary->pNext = adi_int_InstanceData.FreeSecondaryList;
                    adi_int_InstanceData.FreeSecondaryList = pSecondary;

                // ELSE
                } else {

                    // disable the event in the CEC IMASK register
                    adi_int_ClearIMaskBits(1 << IVG);

                    // remove the ISR from the EVT
                    *(ADI_INT_EVT + IVG) = (u32)NULL;

                    // remove the handler from the primary table
                    pPrimary->Handler = NULL;

                // ENDIF
                }

            // ENDIF
            }

            // indicate success
            Result = ADI_INT_RESULT_SUCCESS;

        // ELSE
        } else {

            // we know we're unhooking a secondary handler entry so let's
            // find the entry we're removing and the entry immediately preceding it in the chain
            pPrevious = pPrimary;
            pSecondary = pPrimary->pNext;

            // WHILE (there are more secondary handlers to check)
            while (pSecondary) {

                // IF (this is the handler we're looking for)
                if ((pSecondary->Handler == Handler) && (pSecondary->ClientArg == ClientArg)) {

                    // decrement the usage count
                    pSecondary->UsageCount--;

                    // IF (the handler is no longer needed)
                    if (pSecondary->UsageCount == 0) {

                        // remove it from the chain
                        pPrevious->pNext = pSecondary->pNext;

                        // free the secondary handler
                        pSecondary->pNext = adi_int_InstanceData.FreeSecondaryList;
                        adi_int_InstanceData.FreeSecondaryList = pSecondary;

                    // ENDIF
                    }

                    // indicate success
                    Result = ADI_INT_RESULT_SUCCESS;

                    // we're done
                    break;

                // ENDIF
                }

                // point to the next handler in the chain
                pPrevious = pSecondary;
                pSecondary = pSecondary->pNext;

            // ENDWHILE
            }

        // ENDIF
        }

    // ENDIF
    }

    // unprotect us
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // return
    return(Result);
}





/*********************************************************************

    Function:       adi_int_SICEnable

    Description:    Enables passing of interrupts from a peripheral to
                    the core CEC.

*********************************************************************/

ADI_INT_RESULT adi_int_SICEnable(           // Enables a SIC interrupt to be passed to the CEC
    const ADI_INT_PERIPHERAL_ID PeripheralID    // peripheral ID
){


    // debug
#if defined(ADI_SSL_DEBUG)
    ADI_INT_RESULT Result;
    if ((Result = CheckPeripheralID(PeripheralID)) != ADI_INT_RESULT_SUCCESS) return (Result);
#endif

    // enable the interrupt
    *((ADI_INT_SIC_IMASK0 + ADI_INT_GET_SIC_IMASK_OFFSET(PeripheralID))) |= ADI_INT_GET_SIC_IMASK_ISR_IWR_MASK(PeripheralID);

    // return
    return (ADI_INT_RESULT_SUCCESS);
}


/*********************************************************************

    Function:       adi_int_SICDisable

    Description:    Disables passing of interrupts from a peripheral to
                    the core CEC.

*********************************************************************/

ADI_INT_RESULT adi_int_SICDisable(          // Disables a SIC interrupt from being passed to the CEC
    const ADI_INT_PERIPHERAL_ID PeripheralID    // peripheral ID
){

    // debug
#if defined(ADI_SSL_DEBUG)
    ADI_INT_RESULT Result;
    if ((Result = CheckPeripheralID(PeripheralID)) != ADI_INT_RESULT_SUCCESS) return (Result);
#endif

    // disable the interrupt
    *(ADI_INT_SIC_IMASK0 + ADI_INT_GET_SIC_IMASK_OFFSET(PeripheralID)) &= ~ADI_INT_GET_SIC_IMASK_ISR_IWR_MASK(PeripheralID);

    // return
    return (ADI_INT_RESULT_SUCCESS);
}

/*********************************************************************

    Function:       adi_int_SICSetIVG

    Description:    Sets the IVG number to which a peripheral is mapped

*********************************************************************/

ADI_INT_RESULT adi_int_SICSetIVG(           // Sets the IVG number to which a peripheral is mapped
    const ADI_INT_PERIPHERAL_ID PeripheralID,   // peripheral ID
    const u32                   IVG             // IVG number
){

    u32 Mask;               // mask value
    u32 Nibble;             // nibble containing IVG
    volatile u32 *pReg;     // register to change

    // debug
#if defined(ADI_SSL_DEBUG)
    ADI_INT_RESULT Result;
    if ((Result = CheckPeripheralID(PeripheralID)) != ADI_INT_RESULT_SUCCESS) return (Result);
    if ((Result = CheckIVG(IVG)) != ADI_INT_RESULT_SUCCESS) return (Result);
#endif

    // get the nibble for this peripheral
    Nibble = ADI_INT_GET_SIC_IAR_NIBBLE(PeripheralID);

    // get the register address
    pReg = ADI_INT_SIC_IAR0 + ADI_INT_GET_SIC_IAR_OFFSET(PeripheralID);

    // set the IVG number
    Mask = ~(0xf << (Nibble << 2));
    *pReg = (*pReg & Mask) | ((IVG - 7) << (Nibble << 2));

    // return
    return (ADI_INT_RESULT_SUCCESS);
}


/*********************************************************************

    Function:       adi_int_SICGetIVG

    Description:    Gets the IVG number to which a peripheral is mapped

*********************************************************************/

ADI_INT_RESULT adi_int_SICGetIVG(           // Gets the IVG number to which a peripheral is mapped
    const ADI_INT_PERIPHERAL_ID PeripheralID,   // peripheral ID
    u32                         *pIVG           // IVG number
){

    u32 Mask;               // mask value
    u32 Nibble;             // nibble containing IVG
    volatile u32 *pReg;     // register to change

    // debug
#if defined(ADI_SSL_DEBUG)
    ADI_INT_RESULT Result;
    if ((Result = CheckPeripheralID(PeripheralID)) != ADI_INT_RESULT_SUCCESS) return (Result);
#endif

    // get the nibble for this peripheral
    Nibble = ADI_INT_GET_SIC_IAR_NIBBLE(PeripheralID);

    // get the register address
    pReg = ADI_INT_SIC_IAR0 + ADI_INT_GET_SIC_IAR_OFFSET(PeripheralID);

    // get the IVG number and store it
    *pIVG = ((*pReg >> (Nibble << 2)) & 0xf) + 7;

    // return
    return (ADI_INT_RESULT_SUCCESS);
}


/*********************************************************************

    Function:       adi_int_SICWakeup

    Description:    Enables/disables a peripheral interrupt from waking
                    up the core.

*********************************************************************/

ADI_INT_RESULT adi_int_SICWakeup(           // Enables/disables a SIC interrupt from waking up the core
    const ADI_INT_PERIPHERAL_ID PeripheralID,   // peripheral ID
    u32                         WakeupFlag      // wakeup enable flag (TRUE/FALSE)
){

    u32 Mask;               // mask value
    volatile u32 *pReg;     // register to change

    // debug
#if defined(ADI_SSL_DEBUG)
    ADI_INT_RESULT Result;
    if ((Result = CheckPeripheralID(PeripheralID)) != ADI_INT_RESULT_SUCCESS) return (Result);
#endif

    // get the mask for this peripheral
    Mask = ADI_INT_GET_SIC_IMASK_ISR_IWR_MASK(PeripheralID);

    // get the register address
    pReg = ADI_INT_SIC_IWR0 + ADI_INT_GET_SIC_IWR_OFFSET(PeripheralID);

    // set the bit accordingly
    if (WakeupFlag) {
        *pReg |= Mask;
    } else {
        *pReg &= ~Mask;
    }

    // return
    return (ADI_INT_RESULT_SUCCESS);
}







/*********************************************************************

    Function:       adi_int_SICGlobalWakeup

    Description:    Enables/disables all peripheral interrupts from
                    waking up the core.

*********************************************************************/

ADI_INT_RESULT adi_int_SICGlobalWakeup(
    u32  WakeupFlag,        /* wakeup enable flag (TRUE/FALSE) */
    pADI_INT_WAKEUP_REGISTER SaveIWR   /* save/restore register value */
){

    u32 RegValue;
    volatile u32 *pReg;     /* register to change */


    /* the ADSP-BF54X has three registers */

    if (WakeupFlag == TRUE)
    {
        pReg = ADI_INT_SIC_IWR0;
        /* restore the value of the register */
        *pReg = SaveIWR->RegIWR0;

#if !( defined(__ADSP_EDINBURGH__)  || defined(__ADSP_BRAEMAR__) || defined(__ADSP_DELTA__))
        pReg = ADI_INT_SIC_IWR1;
        /* restore the value of the register */
        *pReg = SaveIWR->RegIWR1;
#endif

#if defined(__ADSP_MOAB__)
        pReg = ADI_INT_SIC_IWR2;
        /* restore  the value of the register */
        *pReg = SaveIWR->RegIWR2;
#endif

    }
    else
    {
        /* disable all wakeups */
        RegValue = 0x0;

        pReg = ADI_INT_SIC_IWR0;
        /* save the value of the register */
        SaveIWR->RegIWR0 = *pReg;
        /* zero the register */
        *pReg = RegValue;

#if !( defined(__ADSP_EDINBURGH__)  || defined(__ADSP_BRAEMAR__)  || defined(__ADSP_DELTA__))
        pReg = ADI_INT_SIC_IWR1;
        /* save the value of the register */
        SaveIWR->RegIWR1 = *pReg;
        /* zero the register */
        *pReg = RegValue;
#endif

#if defined(__ADSP_MOAB__)
        pReg = ADI_INT_SIC_IWR2;
        /* save the value of the register */
        SaveIWR->RegIWR2 = *pReg;
        /* zero the register */
        *pReg = RegValue;
#endif


    }
    return (ADI_INT_RESULT_SUCCESS);

}


/*********************************************************************

    Function:       adi_int_SICInterruptAsserted

    Description:    Determines if a peripheral is asserting an interrupt.

*********************************************************************/

ADI_INT_RESULT adi_int_SICInterruptAsserted(    // Determines if peripheral is asserting an interrupt
    const ADI_INT_PERIPHERAL_ID PeripheralID        // peripheral ID
) {

    // debug
#if defined(ADI_SSL_DEBUG)
    ADI_INT_RESULT Result;
    if ((Result = CheckPeripheralID(PeripheralID)) != ADI_INT_RESULT_SUCCESS) return (Result);
#endif

    // interrogate the appropriate SIC ISR register and return the result
    if (*(ADI_INT_SIC_ISR0 + ADI_INT_GET_SIC_ISR_OFFSET(PeripheralID)) & ADI_INT_GET_SIC_IMASK_ISR_IWR_MASK(PeripheralID)) {
        return (ADI_INT_RESULT_ASSERTED);
    }

    // return
    return (ADI_INT_RESULT_NOT_ASSERTED);
}


/*********************************************************************

    Function:       adi_int_GetCurrentIVGLevel

    Description:    Senses the IVG level at which the processor is
                    currently running.  Returns ADI_INT_RESULT_NOT_ASSERTED
                    if no interrupt is active.

*********************************************************************/
ADI_INT_RESULT adi_int_GetCurrentIVGLevel(
    u32 *pIVG                             // location where the current IVG level is stored
) {

    ADI_INT_RESULT          Result;             // return code
    u32                     i;                  // counter
    ADI_INT_IMASK           Mask;               // mask
    ADI_INT_IMASK           IPend;              // IPend
    void                    *pExitCriticalArg;  // parameter for exit critical

    // assume no interrupt is active
    Result = ADI_INT_RESULT_NOT_ASSERTED;

    // get the current value of IPEND
    pExitCriticalArg = adi_int_EnterCriticalRegion(adi_int_InstanceData.pEnterCriticalArg);
    IPend = *ADI_INT_CEC_IPEND;
    adi_int_ExitCriticalRegion(pExitCriticalArg);

    // determine the highest priority bit that's set and store the corresponding IVG level in the caller's location
    for (i = 0, Mask = 1; i < 16; i++, Mask <<= 1) {
        if (IPend & Mask) {
            *pIVG = i;
            Result = ADI_INT_RESULT_SUCCESS;
            break;
        }
    }

    // return
    return (Result);
}


/************************************************************************

* Function:    adi_int_GetLibraryDetails

* Description: Accepts a pointer to a ADI_INT_LIBRARY_DETAILS structure.
               Returns the details about the library in the ADI_INT_LIBRARY_DETAILS
               structure.

************************************************************************/

ADI_INT_RESULT adi_int_GetLibraryDetails(ADI_INT_LIBRARY_DETAILS *pLibraryDetails)
{
    ADI_INT_LIBRARY_DETAILS *p;

    p = pLibraryDetails;

#if defined(__ADSP_DELTA__)
    p->Processor = ADI_INT_LIBRARY_PROC_BF592;
#elif defined(__ADSP_MOY__)
    p->Processor = ADI_INT_LIBRARY_PROC_BF506F;
#elif defined(__ADSP_BRODIE__)
    p->Processor = ADI_INT_LIBRARY_PROC_BF512;
#elif defined(__ADSPBF526__)
    p->Processor = ADI_INT_LIBRARY_PROC_BF526;
#elif defined(__ADSPBF527__)
    p->Processor = ADI_INT_LIBRARY_PROC_BF527;
#elif defined(__ADSPBF533__)
    p->Processor = ADI_INT_LIBRARY_PROC_BF533;
#elif defined(__ADSPBF537__)
    p->Processor = ADI_INT_LIBRARY_PROC_BF537;
#elif defined(__ADSPBF538__)
    p->Processor = ADI_INT_LIBRARY_PROC_BF538;
#elif defined(__ADSPBF548__) || defined(__ADSPBF548M__)
    p->Processor = ADI_INT_LIBRARY_PROC_BF548;
#elif defined(__ADSPBF561__)
    p->Processor = ADI_INT_LIBRARY_PROC_BF561;
#else
    p->Processor = ADI_INT_LIBRARY_PROC_UNKNOWN;
#endif


#if defined(ADI_SSL_DEBUG)
    p->Config = ADI_INT_LIBRARY_CONFIG_DEBUG;
#else
    p->Config = ADI_INT_LIBRARY_CONFIG_RELEASE;
#endif

#if defined(ADI_SSL_AUTO)
    p->ChipRev = ADI_INT_LIBRARY_CHIP_REV_AUTO;
#else
#if defined(ADI_SSL_CHIP_REV)
    p->ChipRev = ADI_SSL_CHIP_REV;
#else
    p->ChipRev = ADI_INT_LIBRARY_CHIP_REV_NONE;         // since 0 is a valid chip rev, we need a special value
#endif  // not ADI_SSL_CHIP_REV
#endif  // not ADI_SSL_AUTO

#if defined(ADI_SSL_WORKAROUNDS)
    p->Workarounds = ADI_INT_LIBRARY_WRKRNDS_ENABLED;
#else
    p->Workarounds = ADI_INT_LIBRARY_WRKRNDS_DISABLED;
#endif

#if defined(ADI_SSL_VDK)
    p->Os = ADI_INT_LIBRARY_OS_VDK;
#elif defined(ADI_SSL_UCOS)
    p->Os = ADI_INT_LIBRARY_OS_UCOS;
#else
    p->Os = ADI_INT_LIBRARY_OS_STANDALONE;
#endif

    return ADI_INT_RESULT_SUCCESS;
}

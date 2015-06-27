/*********************************************************************************

Copyright(c) 2004-2010 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$Revision: 3626 $
$Date: 2010-10-25 09:37:28 -0400 (Mon, 25 Oct 2010) $

Description:
            This is the source code for the interrupt manager's ISR functions.

*******************************************************************/
#include <sys/anomaly_macros_rtl.h>
#include "adi_int_module.h"
.import "adi_int_module.h";

#ifdef ADI_SSL_UCOS

#include "..\rtos\os_cpu.h"

//******************************************************************
//    External Globals for uCOS
//******************************************************************

.extern _OS_CPU_ISR_Entry;
.extern _OS_CPU_ISR_Exit;
.extern _OSIntExit;
#endif /* ADI_SSL_UCOS */


#if  WA_05000283
#define WORKAROUND_05000283()                                           \
    [--SP] = P0;                                                        \
    [--SP] = ASTAT;                                                     \
    CC = R0 == R0;   /* always true                            */       \
    P0.L = 0x0014;   /* MMR space - CHIPID                     */       \
    P0.H = 0xffc0;                                                      \
    IF CC JUMP 4;                                                       \
    R0 =  [ P0 ];    /* bogus MMR read that is speculatively   */       \
                     /* read and killed - never executed       */       \
    ASTAT = [SP++];                                                     \
    P0 = [SP++];

#else 
#define WORKAROUND_05000283()
#endif /* WA_05000283 */





/* *****************************************************************
*
*    Reference to Interrupt Manager reference data
*
*******************************************************************/
.section constdata;

.extern _adi_int_InstanceData;
.type _adi_int_InstanceData,STT_OBJECT;

.section program;


/*********************************************************************

    Function:       adi_int_NestingISR

    Description:    The nesting ISR.  This function immediately
                    invokes the primary handler then executes any secondary
                    handlers that may be in the chain.

                    On entry R0 contains the address of the primary handler
                    entry for the required IVG level.

                    The return value from each handler is no longer tested
                    so all handlers in the chain are invoked regardless of
                    whether or not the handler indicates it processed the
                    interrupt.

                    Pops R0 before returning from interrupt.

*********************************************************************/
#define Result R0
// The P3 & P4 regs are used as they are call-preserved in C - If a handler
// uses them it is resposible for maintaining there values
#define pEntry P3
#define pNextEntry P4

__STARTFUNC(_adi_int_NestingISR)

    /* Anomaly 05000428 does not apply here because the memory read is from MMR space */
    .MESSAGE/SUPPRESS 5517;
    WORKAROUND_05000283()
    .MESSAGE/POP 5517;

#if defined(ADI_SSL_UCOS)
    /* Save the RETS before making the call */
    [ -- SP ] = RETS ;
    
	/* Save R0 */
	P1 = R0;
	
    /* this function takes care of incrementing OSIntNesting and saving */
    /* processor context.                                               */
    R0 = NESTED;
    CALL _OS_CPU_ISR_Entry;

	/* Restore R0 */
	R0 = P1;
	
    /* Setup C Run-Time stack */
    LINK 0;
    SP += -12 ;/* make space for outgoing arguments when calling C-functions */
#else
    ADI_INT_PROLOG(__ISR, __NESTED)
#endif

    // assign location of Primary Handler entry
    pEntry = R0;

    // load Primary Handler address and client arg value
    nop; nop; nop; nop; // unavoidable delay - sorry!
    P1 = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,Handler)];
    R0 = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,ClientArg)];

    // look ahead to the next entry to fill nop slot
    pNextEntry = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,pNext)];
    nop;

    // and call Primary Handler
    CALL (P1);

._adi_int_NestingISR.secondary_loop_start:

    // check result code and exit if previous handler in chain
    // processed the interrupt
//  CC = Result == 0;
//  IF CC jump ._adi_int_NestingISR.exit (bp);

    // otherwise check for next handler in chain and exit if none
    CC = pNextEntry == 0;
    IF CC jump ._adi_int_NestingISR.exit (bp);

    // load handler address and client arg value
    P1 = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,Handler)];
    R0 = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,ClientArg)];

    // look ahead to the next entry to fill nop slot
    pNextEntry = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,pNext)];
    nop;

    // and call Secondary Handler
    CALL (P1);

    // loop round secondary handlers
    jump ._adi_int_NestingISR.secondary_loop_start;

._adi_int_NestingISR.exit:

#if defined(ADI_SSL_UCOS)
    SP     += -4;                               /* Disable interrupts by this artificial pop   */
    RETI    = [ SP++ ];                         /* of RETI register. Restore context need to   */
                                                /* be done while interrupts are disabled       */
    CALL.X _OSIntExit;
    /* Destroy the C Run-Time Stack created */
    UNLINK;
    /* Jump to processor context restore routine */
    /* This routine will RTI out of this interrupt level */
    JUMP.X _OS_CPU_ISR_Exit;
#else
#if WA_05000428
    nop;        // workaround anomaly 05-00-0428


#endif
    ADI_INT_EPILOG(__ISR, __NESTED)
#endif

__ENDFUNC(_adi_int_NestingISR)

/*********************************************************************

    Function:       adi_int_NonNestingISR

    Description:    The non-nesting ISR.  This function immediately
                    invokes the primary handler then executes any secondary
                    handlers that may be in the chain.

                    On entry R0 contains the address of the primary handler
                    entry for the required IVG level.

                    The return value from each handler is no longer tested
                    so all handlers in the chain are invoked regardless of
                    whether or not the handler indicates it processed the
                    interrupt.

                    Pops R0 before returning from interrupt.

*********************************************************************/

__STARTFUNC(_adi_int_NonNestingISR)

    /* Anomaly 05000428 does not apply here because the memory read is from MMR space */
    .MESSAGE/SUPPRESS 5517;
    WORKAROUND_05000283()
    .MESSAGE/POP 5517;

#if defined(ADI_SSL_UCOS)
    /* Save the RETS before making the call */
    [ -- SP ] = RETS ;

	/* Save R0 */
	P1 = R0;

    /* this function takes care of incrementing OSIntNesting and saving */
    /* processor context.                                               */
    R0 = NESTED;
    CALL _OS_CPU_ISR_Entry;

	/* Restore R0 */
	R0 = P1;

	
    /* Setup C Run-Time stack */
    LINK 0;
    SP += -12 ;/* make space for outgoing arguments when calling C-functions */
#else
    ADI_INT_PROLOG(__ISR, __NON_NESTED)
#endif

    // assign location of Primary Handler entry
    pEntry = R0;

    // load Primary Handler address and client arg value
    nop; nop; nop; nop;
    P1 = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,Handler)];
    R0 = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,ClientArg)];

    // look ahead to the next entry to fill nop slot
    pNextEntry = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,pNext)];
    nop;
    // and call handler
    CALL (P1);

._adi_int_NonNestingISR.secondary_loop_start:

    // check result code and exit if processed
//  CC = Result == 0;
//  IF CC jump ._adi_int_NonNestingISR.exit (bp);

    // otherwise check for next handler in chain and exit if none
    CC = pNextEntry == 0;
    IF CC jump ._adi_int_NonNestingISR.exit (bp);

    // load handler address and client arg value
    P1 = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,Handler)];
    R0 = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,ClientArg)];

    // look ahead to the next entry to fill nop slot
    pNextEntry = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,pNext)];
    nop;

    // and call Secondary Handler
    CALL (P1);

    // loop round secondary handlers
    jump ._adi_int_NonNestingISR.secondary_loop_start;

._adi_int_NonNestingISR.exit:

#if defined(ADI_SSL_UCOS)
    /* Call OSIntExit at the end of an ISR */
    CALL.X _OSIntExit;
    /* Destroy the C Run-Time Stack created */
    UNLINK;
    /* Jump to processor context restore routine */
    /* This routine will RTI out of this interrupt level */
    JUMP.X _OS_CPU_ISR_Exit;
#else
#if  WA_05000428
    nop;        // workaround anomaly 05-00-0428
#endif
    ADI_INT_EPILOG(__ISR, __NON_NESTED)
#endif

__ENDFUNC(_adi_int_NonNestingISR)

/*********************************************************************

    Function:       adi_int_NestingEXC

    Description:    The nesting Exception ISR. This function immediately
                    invokes the primary handler then executes any secondary
                    handlers that may be in the chain.

                    On entry R0 contains the address of the primary handler
                    entry for the required IVG level.

                    The return value from each handler is no longer tested
                    so all handlers in the chain are invoked regardless of
                    whether or not the handler indicates it processed the
                    interrupt.

                    Pops R0 before returning from interrupt.

*********************************************************************/
#define Result R0
// The P3 & P4 regs are used as they are call-preserved in C - If a handler
// uses them it is resposible for maintaining there values
#define pEntry P3
#define pNextEntry P4

__STARTFUNC(_adi_int_NestingEXC)

    /* For uCOS, also use the standalone Exception management (Int manager) */
    ADI_INT_PROLOG(__EXC, __NESTED)

#if  WA_05000283
    // workaround for anomaly 05-00-0283
    cc = r0 == r0; // always true
    p0.l = 0x0014;
    p0.h = 0xffc0; // MMR space CHIPID
    if cc jump _adi_int_NestingEXC_skip_05_000_283; // always skip MMR access, however, MMR access will be fetched and killed
#if WA_05000428
 // suppress assembler warning for 05-00-0428 as speculative MMR access are safe
.MESSAGE/SUPPRESS 5517 FOR 2 LINES;
#endif
    r0 = [p0]; // bogus mmr read
    _adi_int_NestingEXC_skip_05_000_283:  // continue with EXC
#endif

// assign location of Primary Handler entry
    pEntry = R0;

    // load Primary Handler address and client arg value
    nop; nop; nop; nop; // unavaoidable delay - sorry!
    P1 = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,Handler)];
    R0 = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,ClientArg)];

    // look ahead to the next entry to fill nop slot
    pNextEntry = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,pNext)];
    nop;

    // and call Primary Handler
    CALL (P1);

._adi_int_NestingEXC.secondary_loop_start:

    // check result code and exit if previous handler in chain
    // processed the interrupt
//  CC = Result == 0;
//  IF CC jump ._adi_int_NestingEXC.exit (bp);

    // otherwise check for next handler in chain and exit if none
    CC = pNextEntry == 0;
    IF CC jump ._adi_int_NestingEXC.exit (bp);

    // load handler address and client arg value
    P1 = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,Handler)];
    R0 = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,ClientArg)];

    // look ahead to the next entry to fill nop slot
    pNextEntry = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,pNext)];
    nop;

    // and call Secondary Handler
    CALL (P1);

    // loop round secondary handlers
    jump ._adi_int_NestingEXC.secondary_loop_start;

._adi_int_NestingEXC.exit:

#if WA_05000428
    nop;        // workaround anomaly 05-00-0428
#endif
    ADI_INT_EPILOG(__EXC, __NESTED)

__ENDFUNC(_adi_int_NestingEXC)

/*********************************************************************

    Function:       adi_int_NonNestingEXC

    Description:    The non-nesting Exception ISR. This function immediately
                    invokes the primary handler then executes any secondary
                    handlers that may be in the chain.

                    On entry R0 contains the address of the primary handler
                    entry for the required IVG level.

                    The return value from each handler is no longer tested
                    so all handlers in the chain are invoked regardless of
                    whether or not the handler indicates it processed the
                    interrupt.

                    Pops R0 before returning from interrupt.

*********************************************************************/

__STARTFUNC(_adi_int_NonNestingEXC)

    /* For uCOS, also use the standalone Exception management (Int manager) */
    ADI_INT_PROLOG(__EXC, __NON_NESTED)

#if  WA_05000283
    // workaround for anomaly 05-00-0283
    cc = r0 == r0; // always true
    p0.l = 0x0014;
    p0.h = 0xffc0; // MMR space CHIPID
    if cc jump _adi_int_NonNestingEXC_skip_05_000_283; // always skip MMR access, however, MMR access will be fetched and killed

#if WA_05000428
 // suppress assembler warning for 05-00-0428 as speculative MMR access are safe
.MESSAGE/SUPPRESS 5517 FOR 2 LINES;
#endif
    r0 = [p0]; // bogus mmr read
    _adi_int_NonNestingEXC_skip_05_000_283:  // continue with EXC
#endif

    // assign location of Primary Handler entry
    pEntry = R0;

    // load Primary Handler address and client arg value
    nop; nop; nop; nop;
    P1 = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,Handler)];
    R0 = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,ClientArg)];

    // look ahead to the next entry to fill nop slot
    pNextEntry = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,pNext)];
    nop;
    // and call handler
    CALL (P1);

._adi_int_NonNestingEXC.secondary_loop_start:

    // check result code and exit if processed
//  CC = Result == 0;
//  IF CC jump ._adi_int_NonNestingEXC.exit (bp);

    // otherwise check for next handler in chain and exit if none
    CC = pNextEntry == 0;
    IF CC jump ._adi_int_NonNestingEXC.exit (bp);

    // load handler address and client arg value
    P1 = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,Handler)];
    R0 = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,ClientArg)];

    // look ahead to the next entry to fill nop slot
    pNextEntry = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,pNext)];
    nop;

    // and call Secondary Handler
    CALL (P1);

    // loop round secondary handlers
    jump ._adi_int_NonNestingEXC.secondary_loop_start;

._adi_int_NonNestingEXC.exit:

#if  WA_05000428
    nop;        // workaround anomaly 05-00-0428
#endif
    ADI_INT_EPILOG(__EXC, __NON_NESTED)

__ENDFUNC(_adi_int_NonNestingEXC)

/*********************************************************************

    Function:       adi_int_NestingNMI

    Description:    The nesting NMI.  This function immediately
                    invokes the primary handler then executes any secondary
                    handlers that may be in the chain.

                    On entry R0 contains the address of the primary handler
                    entry for the required IVG level.

                    The return value from each handler is no longer tested
                    so all handlers in the chain are invoked regardless of
                    whether or not the handler indicates it processed the
                    interrupt.

                    Pops R0 before returning from interrupt.

*********************************************************************/
#define Result R0
// The P3 & P4 regs are used as they are call-preserved in C - If a handler
// uses them it is resposible for maintaining there values
#define pEntry P3
#define pNextEntry P4

__STARTFUNC(_adi_int_NestingNMI)

    /* Anomaly 05000428 does not apply here because the memory read is from MMR space */
    .MESSAGE/SUPPRESS 5517;
    WORKAROUND_05000283()
    .MESSAGE/POP 5517;

#if defined(ADI_SSL_UCOS)
    /* Save the RETS before making the call */
    [ -- SP ] = RETS ;
    
	/* Save R0 */
	P1 = R0;
	
    /* this function takes care of incrementing OSIntNesting and saving */
    /* processor context.                                               */
    R0 = NESTED;
    CALL _OS_CPU_ISR_Entry;

	/* Restore R0 */
	R0 = P1;
	
    /* Setup C Run-Time stack */
    LINK 0;
    SP += -12 ;/* make space for outgoing arguments when calling C-functions */
#else
    ADI_INT_PROLOG(__NMI, __NESTED)
#endif

    // assign location of Primary Handler entry
    pEntry = R0;

    // load Primary Handler address and client arg value
    nop; nop; nop; nop; // unavoidable delay - sorry!
    P1 = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,Handler)];
    R0 = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,ClientArg)];

    // look ahead to the next entry to fill nop slot
    pNextEntry = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,pNext)];
    nop;

    // and call Primary Handler
    CALL (P1);

._adi_int_NestingNMI.secondary_loop_start:

    // otherwise check for next handler in chain and exit if none
    CC = pNextEntry == 0;
    IF CC jump ._adi_int_NestingNMI.exit (bp);

    // load handler address and client arg value
    P1 = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,Handler)];
    R0 = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,ClientArg)];

    // look ahead to the next entry to fill nop slot
    pNextEntry = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,pNext)];
    nop;

    // and call Secondary Handler
    CALL (P1);

    // loop round secondary handlers
    jump ._adi_int_NestingNMI.secondary_loop_start;

._adi_int_NestingNMI.exit:

#if defined(ADI_SSL_UCOS)
    SP     += -4;                               /* Disable interrupts by this artificial pop   */
    RETI    = [ SP++ ];                         /* of RETI register. Restore context need to   */
                                                /* be done while interrupts are disabled       */
    CALL.X _OSIntExit;
    /* Destroy the C Run-Time Stack created */
    UNLINK;
    /* Jump to processor context restore routine */
    /* This routine will RTI out of this interrupt level */
    JUMP.X _OS_CPU_ISR_Exit;
#else
#if WA_05000428
    nop;        // workaround anomaly 05-00-0428


#endif
    ADI_INT_EPILOG(__NMI, __NESTED)
#endif

__ENDFUNC(_adi_int_NestingNMI)

/*********************************************************************

    Function:       adi_int_NonNestingNMI

    Description:    The non-nesting NMI.  This function immediately
                    invokes the primary handler then executes any secondary
                    handlers that may be in the chain.

                    On entry R0 contains the address of the primary handler
                    entry for the required IVG level.

                    The return value from each handler is no longer tested
                    so all handlers in the chain are invoked regardless of
                    whether or not the handler indicates it processed the
                    interrupt.

                    Pops R0 before returning from interrupt.

*********************************************************************/

__STARTFUNC(_adi_int_NonNestingNMI)

    /* Anomaly 05000428 does not apply here because the memory read is from MMR space */
    .MESSAGE/SUPPRESS 5517;
    WORKAROUND_05000283()
    .MESSAGE/POP 5517;

#if defined(ADI_SSL_UCOS)
    /* Save the RETS before making the call */
    [ -- SP ] = RETS ;

	/* Save R0 */
	P1 = R0;

    /* this function takes care of incrementing OSIntNesting and saving */
    /* processor context.                                               */
    R0 = NESTED;
    CALL _OS_CPU_ISR_Entry;

	/* Restore R0 */
	R0 = P1;

	
    /* Setup C Run-Time stack */
    LINK 0;
    SP += -12 ;/* make space for outgoing arguments when calling C-functions */
#else
    ADI_INT_PROLOG(__NMI, __NON_NESTED)
#endif

    // assign location of Primary Handler entry
    pEntry = R0;

    // load Primary Handler address and client arg value
    nop; nop; nop; nop;
    P1 = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,Handler)];
    R0 = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,ClientArg)];

    // look ahead to the next entry to fill nop slot
    pNextEntry = [pEntry + offsetof(ADI_INT_HANDLER_ENTRY,pNext)];
    nop;
    // and call handler
    CALL (P1);

._adi_int_NonNestingNMI.secondary_loop_start:

    // otherwise check for next handler in chain and exit if none
    CC = pNextEntry == 0;
    IF CC jump ._adi_int_NonNestingNMI.exit (bp);

    // load handler address and client arg value
    P1 = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,Handler)];
    R0 = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,ClientArg)];

    // look ahead to the next entry to fill nop slot
    pNextEntry = [pNextEntry + offsetof(ADI_INT_HANDLER_ENTRY,pNext)];
    nop;

    // and call Secondary Handler
    CALL (P1);

    // loop round secondary handlers
    jump ._adi_int_NonNestingNMI.secondary_loop_start;

._adi_int_NonNestingNMI.exit:

#if defined(ADI_SSL_UCOS)
    /* Call OSIntExit at the end of an ISR */
    CALL.X _OSIntExit;
    /* Destroy the C Run-Time Stack created */
    UNLINK;
    /* Jump to processor context restore routine */
    /* This routine will RTI out of this interrupt level */
    JUMP.X _OS_CPU_ISR_Exit;
#else
#if  WA_05000428
    nop;        // workaround anomaly 05-00-0428
#endif
    ADI_INT_EPILOG(__NMI, __NON_NESTED)
#endif

__ENDFUNC(_adi_int_NonNestingNMI)

/*********************************************************************

	Function:		adi_int_NestingISR_IVG0 - adi_int_NestingISR_IVG15

	Description:	The nesting ISRs for each IVG level that are used  
					to populate the EVT. The function pushes R0 on the  
					stack, then changes it to the Address of the primary 
					handler entry in the instance data and then jumps  
					to the main ISR, whose prolog does not push R0, but 
					does pop it before returning from interrupt.
					
					IMPORTANT: THESE ROUTINES MUST BE MAINTAINED IN THIS 
					ORDER AS THEY ARE INDEXED BY THE CODE IN adi_int.c
					
*********************************************************************/

.adi_int_isrblock:	

ADI_INT_ISR_FUNCTION(_adi_int_NestingISR_IVG0,0,__NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NestingISR_IVG1,1,__NESTED)
ADI_INT_NMI_FUNCTION(_adi_int_NestingISR_IVG2,2,__NESTED)
ADI_INT_EXC_FUNCTION(_adi_int_NestingISR_IVG3,3,__NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NestingISR_IVG4,4,__NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NestingISR_IVG5,5,__NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NestingISR_IVG6,6,__NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NestingISR_IVG7,7,__NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NestingISR_IVG8,8,__NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NestingISR_IVG9,9,__NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NestingISR_IVG10,10,__NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NestingISR_IVG11,11,__NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NestingISR_IVG12,12,__NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NestingISR_IVG13,13,__NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NestingISR_IVG14,14,__NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NestingISR_IVG15,15,__NESTED)

/*********************************************************************

	Function:		adi_int_NonNestingISR_IVG0 - adi_int_NonNestingISR_IVG15

	Description:	The non-nesting ISRs for each IVG level that are used  
					to populate the EVT. The function pushes R0 on the  
					stack, then changes it to the Address of the primary 
					handler entry in the instance data and then jumps  
					to the main ISR, whose prolog does not push R0, but 
					does pop it before returning from interrupt.
					
					IMPORTANT: THESE ROUTINES MUST BE MAINTAINED IN THIS 
					ORDER AS THEY ARE INDEXED BY THE CODE IN adi_int.c
					
*********************************************************************/

ADI_INT_ISR_FUNCTION(_adi_int_NonNestingISR_IVG0,0,__NON_NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NonNestingISR_IVG1,1,__NON_NESTED)
ADI_INT_NMI_FUNCTION(_adi_int_NonNestingISR_IVG2,2,__NON_NESTED)
ADI_INT_EXC_FUNCTION(_adi_int_NonNestingISR_IVG3,3,__NON_NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NonNestingISR_IVG4,4,__NON_NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NonNestingISR_IVG5,5,__NON_NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NonNestingISR_IVG6,6,__NON_NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NonNestingISR_IVG7,7,__NON_NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NonNestingISR_IVG8,8,__NON_NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NonNestingISR_IVG9,9,__NON_NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NonNestingISR_IVG10,10,__NON_NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NonNestingISR_IVG11,11,__NON_NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NonNestingISR_IVG12,12,__NON_NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NonNestingISR_IVG13,13,__NON_NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NonNestingISR_IVG14,14,__NON_NESTED)
ADI_INT_ISR_FUNCTION(_adi_int_NonNestingISR_IVG15,15,__NON_NESTED)

.adi_int_isrblock.end:


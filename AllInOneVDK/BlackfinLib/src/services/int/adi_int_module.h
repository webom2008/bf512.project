/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  

$RCSfile: adi_int_module.h,v $
$Revision: 3626 $
$Date: 2010-10-25 09:37:28 -0400 (Mon, 25 Oct 2010) $

Description:
			Interrupt Manager Header for use by module functions only

*********************************************************************************/

#ifndef __ADI_INT_MODULE_H__
#define __ADI_INT_MODULE_H__

#include <services/services.h>

#define ADI_INT_IVG_COUNT	16			// number of IVG levels

#if !defined(_LANGUAGE_ASM)

typedef struct ADI_INT_HANDLER_ENTRY {		// handler table entry
	ADI_INT_HANDLER_FN 				Handler;	// secondary handler
	void							*ClientArg;	// parameter to pass to the handler
	u32                             UsageCount; // indicates how many times the handler (with same ClientArg) has been hooked
	struct ADI_INT_HANDLER_ENTRY	*pNext;		// pointer to next handler in the chain
} ADI_INT_HANDLER_ENTRY;

typedef struct ADI_INT_INSTANCE_DATA {					// data structure for all interrupt manager data memory usage
    void					*pEnterCriticalArg;				// parameter for enter critical region 
	ADI_INT_HANDLER_ENTRY 	*FreeSecondaryList;				// pointer to a list of free secondary entries
	ADI_INT_HANDLER_ENTRY	PrimaryTable[ADI_INT_IVG_COUNT];// primary handler table
} ADI_INT_INSTANCE_DATA;

#else

// Assembler section

/******************************************************************

	Utility Macros to make definition of assembler functions more
	legible
	
*******************************************************************/

#define __NESTED 1 
#define __NON_NESTED 0
#define __STATIC 1
#define __NON_STATIC 0
#define __ISR 0
#define __EXC 1
#define __NMI 2
#define __STARTFUNC(Name) \
	.align 2; \
	Name:

#define __ENDFUNC(Name) \
.##Name##.end:\
	.global Name;\
	.type Name,STT_FUNC;		

/******************************************************************

	Macros to perform the context saving required for the prolog and
	epilog of an ISR. These are imbalanced such that the prolog does not
	push R0 but R0 is popped in the epilog. This is to enable us to 
	have one small (outer) ISR entry (whose address is placed in the EVT)
	for each IVG (32 in total) and just one	(inner) ISR for each of 
	nesting and non-nesting, which are jumped from by outer ISR. Return
	to the outer ISR is not required. The Outer routine saves off R0
	before assigning it to the required address of the primary handler 
	entry in the instance data. In this way we have a completely 
	deterministic approach to determining the IVG level that requires
	processing whilst keeping code size to a minimum.
	
	Macros are added to cater for exception handling
	
*******************************************************************/
#define ADI_INT_PROLOG(Type, Nested) \
.IF (Type == __ISR && Nested == __NESTED);\
		[--SP] = RETI;\
.ELIF (Type == __EXC && Nested == __NESTED);\
		[--SP] = RETX;\
.ENDIF;\
		[--SP] = ASTAT;\
		[--SP] = FP;\
		[--SP] = (R7:1,P5:0);\
		[--SP] = I0;\
		[--SP] = I1;\
		[--SP] = I2;\
		[--SP] = I3;\
		[--SP] = B0;\
		[--SP] = B1;\
		[--SP] = B2;\
		[--SP] = B3;\
		[--SP] = L0;\
		[--SP] = L1;\
		[--SP] = L2;\
		[--SP] = L3;\
		[--SP] = M0;\
		[--SP] = M1;\
		[--SP] = M2;\
		[--SP] = M3;\
		R1.L = A0.x;\
		[--SP] = R1;\
		R1 = A0.w;\
		[--SP] = R1;\
		R1.L = A1.x;\
		[--SP] = R1;\
		R1 = A1.w;\
		[--SP] = R1;\
		[--SP] = LC0;\
		R3 =   0;\
		LC0 = R3 ;\
		[--SP] = LC1;\
		R3 =   0;\
		LC1 = R3 ;\
		[--SP] = LT0;\
		[--SP] = LT1;\
		[--SP] = LB0;\
		[--SP] = LB1;\
		SP +=  -12;\
		L0 =  0 (X);\
		L1 =  0 (X);\
		L2 =  0 (X);\
		L3 =  0 (X);\
		link  20;

#define ADI_INT_EPILOG(Type, Nested) \
		unlink;\
		SP +=  12;\
		LB1 = [SP++];\
		LB0 = [SP++];\
		LT1 = [SP++];\
		LT0 = [SP++];\
		LC1 = [SP++];\
		LC0 = [SP++];\
		R0 = [SP++];\
		A1 = R0 ;\
		R0 = [SP++];\
		A1.x = R0.L;\
		R0 = [SP++];\
		A0 = R0 ;\
		R0 = [SP++];\
		A0.x = R0.L;\
		M3 = [SP++];\
		M2 = [SP++];\
		M1 = [SP++];\
		M0 = [SP++];\
		L3 = [SP++];\
		L2 = [SP++];\
		L1 = [SP++];\
		L0 = [SP++];\
		B3 = [SP++];\
		B2 = [SP++];\
		B1 = [SP++];\
		B0 = [SP++];\
		I3 = [SP++];\
		I2 = [SP++];\
		I1 = [SP++];\
		I0 = [SP++];\
		(R7:1,P5:0) = [SP++];\
		FP = [SP++];\
		ASTAT = [SP++];\
.IF (Type == __ISR && Nested == __NESTED);\
		RETI = [SP++];\
.ELIF (Type == __EXC && Nested == __NESTED);\
		RETX = [SP++];\
.ENDIF;\
		P1 = [SP++]; \
		R0 = [SP++]; \
.IF (Type == __ISR);\
		RTI;\
.ELIF (Type == __EXC);\
		RTX;\
.ELIF (Type == __NMI);\
		RTN;\
.ENDIF;\
		NOP;\
		NOP;\
		NOP;
						
/******************************************************************

	Macro to calculate the address of the primary handler entry for  
	a given IVG level.
	
*******************************************************************/

#define ADI_INT_PRIMARY_ENTRY_ADDRESS(IVG) \
	_adi_int_InstanceData + (offsetof(ADI_INT_INSTANCE_DATA,PrimaryTable) + (IVG*sizeof(ADI_INT_HANDLER_ENTRY)))

#endif

#if defined(_LANGUAGE_ASM)

/******************************************************************

	Macro to define the main ISR entry point for each IVG and for 
	both nesting and non-nesting cases and exception handling.
	
*******************************************************************/

#define ADI_INT_ISR_FUNCTION(Name,IVG,Nested) \
	__STARTFUNC(Name) \
		[--SP] = R0; \
		[--SP] = P1; \
		R0.L = ADI_INT_PRIMARY_ENTRY_ADDRESS(IVG); \
		R0.H = ADI_INT_PRIMARY_ENTRY_ADDRESS(IVG); \
		.IF (Nested == __NESTED); \
			jump.x _adi_int_NestingISR; \
		.ELSE; \
			jump.x _adi_int_NonNestingISR; \
		.ENDIF; \
	__ENDFUNC(Name)

#define ADI_INT_EXC_FUNCTION(Name,IVG,Nested) \
	__STARTFUNC(Name) \
		[--SP] = R0; \
		[--SP] = P1; \
		R0.L = ADI_INT_PRIMARY_ENTRY_ADDRESS(IVG); \
		R0.H = ADI_INT_PRIMARY_ENTRY_ADDRESS(IVG); \
		.IF (Nested == __NESTED); \
			jump.x _adi_int_NestingEXC; \
		.ELSE; \
			jump.x _adi_int_NonNestingEXC; \
		.ENDIF; \
	__ENDFUNC(Name)

#define ADI_INT_NMI_FUNCTION(Name,IVG,Nested) \
	__STARTFUNC(Name) \
		[--SP] = R0; \
		[--SP] = P1; \
		R0.L = ADI_INT_PRIMARY_ENTRY_ADDRESS(IVG); \
		R0.H = ADI_INT_PRIMARY_ENTRY_ADDRESS(IVG); \
		.IF (Nested == __NESTED); \
			jump.x _adi_int_NestingNMI; \
		.ELSE; \
			jump.x _adi_int_NonNestingNMI; \
		.ENDIF; \
	__ENDFUNC(Name)
	
#else


#endif


#endif //__ADI_INT_MODULE_H__

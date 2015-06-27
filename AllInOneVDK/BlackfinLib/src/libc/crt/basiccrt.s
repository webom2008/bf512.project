/* Copyright (C) 2000-2010 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
// basic startup code which
// - turns the cycle counter on
// - loads up FP & SP (both supervisor and user)
// - initialises the device drivers (FIOCRT)
// - calls monstartup to set up the profiling routines (PROFCRT)
// - calls the C++ startup (CPLUSCRT)
// - initialises argc/argv (FIOCRT/normal)
// - calls _main
// - calls _exit (which calls monexit to dump accumulated prof data (PROFCRT))
// - defines dummy IO routines (!FIOCRT)

#include <sys/platform.h>
#include <cplb.h>
#include <sys/anomaly_macros_rtl.h>

#define IVBh (EVT0 >> 16)
#define IVBl (EVT0 & 0xFFFF)
#define UNASSIGNED_VAL 0x8181
#define UNASSIGNED_FILL 0
// just IVG15
#define INTERRUPT_BITS 0x400

	.section program;
	.align 2;
	.file_attr requiredForROMBoot;	

start:
   /* The assembler warning for anomaly 05-00-0312, issued when interrupts are 
   ** enabled and an an SSYNC, CSYNC or loop register load is used, is not 
   ** needed till interrupts are enabled. So we suppress it.
   */
   .MESSAGE/SUPPRESS 5515;   

#if WA_05000109
	// Avoid Anomaly ID 05000109. 
#	define SYSCFG_VALUE 0x30
	R1 = SYSCFG_VALUE;
	SYSCFG = R1;
#endif
#if WA_05000229
   // Avoid Anomaly 05-00-0229: DMA5_CONFIG and SPI_CTL not cleared on reset.
   R1 = 0x400;
#if defined(__ADSPBF538__) || defined(__ADSPBF539__)
   P0.L = SPI0_CTL & 0xFFFF;
   P0.H = SPI0_CTL >> 16;
   W[P0] = R1.L;
#else
   P0.L = SPI_CTL & 0xFFFF;
   P0.H = SPI_CTL >> 16;
   W[P0] = R1.L;
#endif
   P0.L = DMA5_CONFIG & 0xFFFF;
   P0.H = DMA5_CONFIG >> 16;
   R1 = 0;
   W[P0] = R1.L;
#endif

#if defined(__ADSPBF504F__) || defined(__ADSPBF506F__)
   /* Enable FLASH */
   R1 = (FLASH_ENABLE | FLASH_UNPROTECT);
   P0.L = FLASH_CONTROL_SET & 0xFFFF;
   P0.H = FLASH_CONTROL_SET >> 16;
   W[P0] = R1.L;
#endif

	// Zap loop counters to zero, to make sure that
	// hw loops are disabled - it could be really baffling
	// if the counters and bottom regs are set, and we happen
	// to run into them.
	R7 = 0;
	LC0 = R7;
	LC1 = R7;

	// Clear the DAG Length regs too, so that it's safe to
	// use I-regs without them wrapping around.
	L0 = R7;
	L1 = R7;
	L2 = R7;
	L3 = R7;

	// Zero ITEST_COMMAND and DTEST_COMMAND
	// (in case they have crud in them and
	// does a write somewhere when we enable cache)
	I0.L = (ITEST_COMMAND & 0xFFFF);
	I0.H = (ITEST_COMMAND >> 16);
	I1.L = (DTEST_COMMAND & 0xFFFF);
	I1.H = (DTEST_COMMAND >> 16);
	R7 = 0;
	[I0] = R7;
	[I1] = R7;
	CSYNC;

	// Initialise the Event Vector table.
	P0.H = IVBh;
	P0.L = IVBl;

	// Install __unknown_exception_occurred in EVT so that 
	// there is defined behaviour.
	P0 += 2*4;		// Skip Emulation and Reset
	P1 = 13;
	R1.L = __unknown_exception_occurred;
	R1.H = __unknown_exception_occurred;
	LSETUP (.ivt,.ivt) LC0 = P1;
.ivt:	[P0++] = R1;
	// Set IVG15's handler to be the start of the mode-change
	// code. Then, before we return from the Reset back to user
	// mode, we'll raise IVG15. This will mean we stay in supervisor
	// mode, and continue from the mode-change point., but at a
	// much lower priority.
	P1.H = supervisor_mode;
	P1.L = supervisor_mode;
	[P0] = P1;

	// Initialise the stack.
	// Note: this points just past the end of the section.
	// First write should be with [--SP].
	SP.L=ldf_stack_end;
	SP.H=ldf_stack_end;
	usp = sp;

	// We're still in supervisor mode at the moment, so the FP
	// needs to point to the supervisor stack.
	FP = SP;

	// And make space for incoming "parameters" for functions
	// we call from here:
	SP += -12;

	R0 = INTERRUPT_BITS;
	R0 <<= 5;	// Bits 0-4 not settable.
	CALL.X __install_default_handlers;
	R4 = R0;		// Save modified list

	R0 = SYSCFG;		// Enable the Cycle counter
	BITSET(R0,1);
	SYSCFG = R0;

#if defined(__AD6900__) || defined(__AD6902__) || defined(__AD6903__) || \
    defined(__AD6904__) || defined(__AD6905__) || defined(__AD6723__) || \
    defined(__MT6906__)
/////////////////////////////////////////////////////////////////
// Initialize the EBUS arbitration registers
	P0.H =EBS_ARBCR0  >> 16;
	P0.L =EBS_ARBCR0 & 0xffff;
	R0.H =0x0; 
	R0.L =0x9618;
	W[P0] = R0;
	SSYNC;
	
	P0.L =EBS_ARBCR1 & 0xffff;
	R0.H =0x0; 
	R0.L =0x1061;
	W[P0] = R0;
	SSYNC;
	
	P0.L =EBS_ARBCR2 & 0xffff;
	R0.H =0x0; 
	R0.L =0x1186;
	W[P0] = R0;
	SSYNC;

/////////////////////////////////////////////////////////////////
// Initialize peripheral behavior in emulation mode
	P0.L = DSP_HALT & 0xffff;
	P0.H = DSP_HALT >> 16;
	R0 = 0x10;
	W[P0] = R0;	
	P0.L = DSP_REL & 0xffff;
	P0.H = DSP_REL >> 16;
	R0 = 0xFF;
	W[P0] = R0;
#endif

#if defined(__AD6531__) || defined (__AD6532__) || defined (__AD6901__)
/////////////////////////////////////////////////////////////////
// Initialize peripheral behavior in emulation mode
	P0.L = DSP_REL & 0xffff;	// DSP_REL[0]=1 to restart DMA after exiting emulation mode
	P0.H = DSP_REL >> 16;
	R0 = W[P0];
	BITSET(R0,0);
	R1 = R0.L(Z);
	W[P0] = R1;
#endif

#if WA_05000137 || WA_05000162
	// Avoid anomaly #05000137/05000162

	// Set the port preferences of DAG0 and DAG1 to be
	// different; this gives better performance when
	// performing dual-dag operations on SDRAM.
	P0.L = DMEM_CONTROL & 0xFFFF;
	P0.H = DMEM_CONTROL >> 16;
	R0 = [P0];
	BITSET(R0, 12);
	BITCLR(R0, 13);
	[P0] = R0;
	CSYNC;
#endif

	// Reinitialise data areas in RAM from ROM, if MemInit's
	// been used.
	CALL.X _mi_initialize;

	// Write the cplb exception handler to the EVT if approprate and
	// initialise the CPLBs if they're needed. couldn't do
	// this before we set up the stacks.
	P2.H = ___cplb_ctrl;
	P2.L = ___cplb_ctrl;
	R0 = CPLB_ENABLE_ANY_CPLBS;
	R6 = [P2];
	R0 = R0 & R6;
	CC = R0;
	IF !CC JUMP no_cplbs;
	P1.H = _cplb_hdr;
	P1.L = _cplb_hdr;
	P0.H = IVBh;
	P0.L = IVBl;
   // Write exception handler into EVT3. In applications using VDK this will
   // replaced later by a VDK specific exception handler.
	[P0+12] = P1;
	R0 = R6;
	CALL.X _cplb_init;
no_cplbs:

   /* We are about to enable interrupts so stop suppressing the assembler 
   ** warning for 05-00-0312.
   */
   .MESSAGE/RESTORE 5515;   

	//  Enable interrupts
	STI R4;		// Using the mask from default handlers
	RAISE 15;

	// Move the processor into user mode.
	P0.L=still_interrupt_in_ipend;
	P0.H=still_interrupt_in_ipend;
	RETI=P0;

still_interrupt_in_ipend:
	rti;	// keep doing 'rti' until we've 'finished' servicing all
		// interrupts of priority higher than IVG15. Normally one
		// would expect to only have the reset interrupt in IPEND
		// being serviced, but occasionally when debugging this may
		// not be the case - if restart is hit when servicing an 
		// interrupt.
		//
		// When we clear all bits from IPEND, we'll enter user mode,
		// then we'll automatically jump to supervisor_mode to start 
		// servicing IVG15 (which we will 'service' for the whole
		// program, so that the program is in supervisor mode.
		//
		// Need to do this to 'finish' servicing the reset interupt.

supervisor_mode:
	[--SP] = RETI;	// re-enables the interrupt system

	R0.L = UNASSIGNED_VAL;
	R0.H = UNASSIGNED_VAL;
#if UNASSIGNED_FILL
	R2=R0;
	R3=R0;
	R4=R0;
	R5=R0;
	R6=R0;
	R7=R0;
	P0=R0;
	P1=R0;
	P2=R0;
	P3=R0;
	P4=R0;
	P5=R0;
#endif
	// Push a RETS and Old FP onto the stack, for sanity.
	[--SP]=R0;
	[--SP]=R0;
	// Make sure the FP is sensible.
	FP = SP;

	// And leave space for incoming "parameters"
	SP += -12;

#ifdef PROFCRT
	CALL.X monstartup; // initialise profiling routines
#endif  /* PROFCRT */
#ifdef __AD6532__
	CALL.X ___start_hsl; // initialise High Speed Logger
#endif  /* __AD6532__ */
#if defined(CPLUSCRT) || defined(_ADI_THREADS)
	R0=0;		// load up R0 and R1 and call _main()
	R1=R0;
	CALL.X ___ctorloop;
#endif  /* CPLUSCRT || ADI_THREADS*/
#if !defined(_ADI_THREADS) 
#ifdef FIOCRT
	// FILE IO provides access to real command-line arguments.
	CALL.X __getargv;
	r1.l=__Argv; 
	r1.h=__Argv;
#else
	// Default to having no arguments and a null list.
	R0=0;
	R1.L=argv;
	R1.H=argv;
#endif /* FIOCRT */
#endif /* _ADI_THREADS */

	// At long last, call the application program.
	CALL.X _main;

#if !defined(_ADI_THREADS) 
	CALL.X _exit;	// passing in main's return value
#endif

.start.end:		// To keep the linker happy.


	.global start;
	.type start,STT_FUNC;
	.extern _main;
	.type _main,STT_FUNC;
#if !defined(_ADI_THREADS)
	.extern _exit;
	.type _exit,STT_FUNC;
#endif
#if defined(__ADSPBF561__) || defined(__ADSPBF566__)
	.extern __mc_data_initialise;
	.type __mc_data_initialise,STT_FUNC;
#endif
#ifdef FIOCRT
	.extern __getargv;
	.type __getargv,STT_FUNC;
	.extern __Argv;
#else
	// If File IO support isn't provided, then
	// we provide dummy versions of the device-handling
	// functions, so that the exception handlers don't rely
	// on the file IO library
	.align 2;
_dev_open:
_dev_close:
_dev_write:
_dev_read:
_dev_seek:
_dev_dup:
#if WA_05000371
	NOP;
	NOP;
	NOP;
#endif 
	R0 = -1;
	RTS;
._dev_open.end:
._dev_close.end:
._dev_write.end:
._dev_read.end:
._dev_seek.end:
._dev_dup.end:

	.global _dev_open;
	.type _dev_open,STT_FUNC;
	.global _dev_close;
	.type _dev_close,STT_FUNC;
	.global _dev_write;
	.type _dev_write,STT_FUNC;
	.global _dev_read;
	.type _dev_read,STT_FUNC;
	.global _dev_seek;
	.type _dev_seek,STT_FUNC;
	.global _dev_dup;
	.type _dev_dup,STT_FUNC;
#endif /* FIOCRT */


	.section data1;
#ifndef FIOCRT
	// With no FILE IO support, we cannot fetch arguments
	// from the command line, so we provide a null list.
	.align 4;
argv:
	.byte4=0;				// argv[0]==0
#endif /* !FIOCRT */
	.align 4;

.extern ldf_stack_end;
.extern _mi_initialize;
.type _mi_initialize, STT_FUNC;

.extern _cplb_hdr;
.extern _cplb_init;
.extern ___cplb_ctrl;
.extern __install_default_handlers;
.extern __unknown_exception_occurred;

#ifdef PROFCRT
.extern monstartup;
#endif
#ifdef __AD6532__
.extern  ___start_hsl;
#endif  /* __AD6532__ */

#if defined(CPLUSCRT) || defined(_ADI_THREADS)
.section ctor;
	.align 4;
___ctor_table:
   /* This is a NULL terminated table of functions pointers for C++ constructor
   ** calls before main(). The LDF should contiguously map this ctor section 
   ** first, then others and then ctorl. The ctorl section contains the NULL
   ** terminator.
   ** We need to define some data at the start of the section here so that the 
   ** start label is placed correctly by the linker. The data could be 
   ** anything as it is unused. We set it to the address of the ctor end 
   ** section label to force that section end label to always be required. 
   ** This avoids unknowingly not having the ctorl defined NULL terminator 
   ** linked and the various unexpected results that could cause.
   */
	.byte4=___ctor_end; 
.global ___ctor_table;
.type ___ctor_table,STT_OBJECT;
.extern ___ctorloop, ___ctor_end;

.section .gdt;
   .align 4;
___eh_gdt:
   .byte4=0;
.global ___eh_gdt;
.type ___eh_gdt,STT_OBJECT;
#endif  /* CPLUSCRT */

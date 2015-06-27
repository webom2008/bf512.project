/*
** Read "data" from L1 Instruction memory.
** Copyright (C) 2004-2011 Analog Devices, Inc. All Rights Reserved.
**
** void *_l1_memcpy(void *datap, const void *instrp, size_t n);
**
** This function is equivalent to memcpy(), except that it can
** read from Blackfin's L1 Instruction memory, which ordinarily
** would raise an exception. The function copies n bytes from
** instrp to datap, and returns datap. It has some restrictions
** beyond that of normal memcpy:
** - instrp must be aligned on an 8-byte boundary.
** - datap must be aligned on a 4-byte boundary.
** - n must be a multiple of 8.
** - instrp must be within L1 Instr memory
** - instrp+n-1 may not be outside of the L1 Instr memory.
** - For multi-core processors (561), instrp must correspond to
** - the core calling the function.
** If any of these restrictions are broken, NULL is returned.
*/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libName=libc;
.file_attr FuncName=__l1_memcpy;
.file_attr libFunc=__l1_memcpy;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
#endif

#include <sys/platform.h>
#include <sys/anomaly_macros_rtl.h>

#ifndef LO
#define LO(_x) (_x & 0xFFFF)
#endif
#ifndef HI
#define HI(_x) (_x >> 16)
#endif

#ifdef __ADSPBF51x__
#define L1INSTRBANKB	0xFFA04000
#if WA_05000123
// We use registers that are clobbered by the workaround for this anomaly.
// The anomaly should never be enabled for this part.
#error Function requires WA_05000123 be disabled when compiling for BF51x
#endif
#endif

.section noncache_code;   // cannot be defined in L1 cache memory
.align 2;

__l1_memcpy:
	// If n == 0, then there's nothing to do.
	CC = R2 == 0;
	IF CC JUMP .finished;

#if defined(__ADSPLPBLACKFIN__) && !defined(__SILICON_REVISION__) && \
    !defined(__ADSPBF561__)
	[--SP] = (R7:4);
#endif

	// If instrp or n is not 8-byte-aligned,
	// or if datap is not 4-byte-aligned, fail.
	R3 = R0 << 1;
	R3 = R3 | R1;
	R3 = R3 | R2;
	R3 <<= 29;
	CC = R3 == 0;
	IF !CC JUMP .bad;

	// Is len negative? (which would be too big)
	CC = R2 < 0;
	IF CC JUMP .bad;

	// Is len a multiple of 8?
	R3 = 8 (Z);
	CC = R3 <= R2 (IU);
	IF !CC JUMP .bad;

	// Is instrp within the range of L1 Instr?

	P0 = R0;	// Save for later
	P1 = R0;

#if !defined(__ADSPLPBLACKFIN__)
   // Is len just too big?
   R3 = 0x4000 (Z);
   CC = R2 <= R3 (IU);
   IF !CC JUMP .bad;

// 0xFFA00000 - 0xFFA03FFF  Code SRAM (16K)
	R3.L = LO(0xFFA00000);
	R3.H = HI(0xFFA00000);
	CC = R3 <= R1 (IU);
	IF !CC JUMP .bad;	// Too low
	R3.L = LO(0xFFA04000);
	R3.H = HI(0xFFA04000);
	R0 = R1 + R2;
	CC = R0 <= R3 (IU);
	IF !CC JUMP .bad;	// Too high
   // otherwise fall through to .instrp_okay
#else
#ifdef CHECKS_DONE
# undef CHECKS_DONE
#endif

#if defined(__ADSPBF561__)
  /* there is a specific 561 library build so no need to worry about 
  ** revision none issues but we do have to check two cores.
  */
.check_bf56x_coreb:
// 0xFF610000 - 0xFF613FFF  Code SRAM / cache (16K)
// 0xFF604000 - 0xFF60FFFF  Reserved
// 0xFF600000 - 0xFF603FFF  Code SRAM (16K)
   // Is len just too big?
   R3 = 0x4000 (Z);
   CC = R2 <= R3 (IU);
   IF !CC JUMP .bad;
   
   R3.L = LO(0xFF600000);  // L1 instruction memory base for core B
   R3.H = HI(0xFF600000);
   CC = R3 <= R1 (IU);
   IF !CC JUMP .bad;       // Too low
   R3.L = LO(0xFF614000);  // L1 instruction memory end+1 for core B
   R3.H = HI(0xFF614000);
   R0 = R1 + R2; 
   CC = R0 <= R3 (IU);
   P2.L = LO(0xFF610000);  // L1 instr cache B address
   P2.H = HI(0xFF610000);
   // set the value to be written to DTEST_COMMAND 
	R0.L = 0x0004;
	R0.H = 0x0100;
   IF CC JUMP .instrp_okay;  // okay
   // fall thru to check core A at .check_generic
#else
/* This is the support for the bf532 named libraries */

#if !defined(__SILICON_REVISION__) && !defined(__ADSPBF561__)
   /* The bf532 library build for silicon revision none need to diffentiate
   ** a couple of part families to perform the right checks. This is done at
   ** runtime.
   */
   // unsigned int partno = ((*(unsigned int *)0xFFC00014)&0x0FFFF000)>>12;
   P2.L = 0x0014;
   P2.H = 0xFFC0;
   R3 = [P2];
   R7.L = 0xF000;
   R7.H = 0x0FFF;
   R3 = R3 & R7;
   R5 = R3 >> 12;

   // partno==0x2800 for BF50x parts
   R6 = 0x2800;
   CC = R5 == R6;
   if CC jump .check_bf50x;
   // partno==0x2802 for BF59x parts
   R6 = 0x2802;
   CC = R5 == R6;
   if CC jump .check_bf59x;
   JUMP .check_generic;
#endif

#if defined(__ADSPBF59x__) || !defined(__SILICON_REVISION__)
.check_bf59x:
// 0xFFA10000 - 0xFFA1FFFF  L1 Instruction ROM (64K)
// 0xFFA00000 - 0xFFA07FFF  L1 Instruction SRAM (32K)
   // Is len just too big? In this case the comparison is not with the
   // size of the but the maximum size of the data memory blocks as the
   // ROM is bigger. This also happens to be the same size as the instruction
   // SRAM block so is good for both forms of copies.
   R3 = 0x8000 (Z);
   CC = R2 <= R3 (IU);
   IF !CC JUMP .bad;

	// check if instrp is in the ROM section : 0xFFA10000 - 0xFFA1FFFF
	R3.L = 0x0000;
	R3.H = 0xFFA1;
	CC = R1 < R3 (IU);
	IF CC JUMP .check_59x_sram;	// instrp is too low for ROM, might be L1 SRAM
	R3.H = 0xFFA2;
	R0 = R1 + R2;
	CC = R0 <= R3 (IU);
	IF !CC JUMP .bad;	      // too high for ROM, bad 

   // set the value to be written to DTEST_COMMAND to read ROM
   //  1 WRITE/READ == 0(read)
   //  2 ONE        == 1(must be set)
   //  24:22 REGION bit 2 for instruction access bit 0 for ROM
	R0.L = 0x0004; 
	R0.H = 0x0140;

   // don't ever need to use the ITEST_COMMAND code for reading ROM
   // so we set the address at which that support to kicks in
   // to an impossible one to reach.
   P2.L = LO(0xFFA20000);
   P2.H = HI(0xFFA20000);
   
	JUMP .instrp_okay;	   // okay

.check_59x_sram:
   R3.H = HI(0xFFA00000);  // L1 instruction memory base
   // assert: R3.L == 0x0000
   CC = R3 <= R1 (IU);
   IF !CC JUMP .bad;       // Too low
   R3.L = LO(0xFFA08000);  // L1 instruction memory end+1
   // assert: R3.H == 0xFFA0
   R0 = R1 + R2;
   CC = R0 <= R3 (IU);
   IF !CC JUMP .bad;       // Too high

   // set the value to be written to DTEST_COMMAND to read ROM
   //  1:WRITE/READ == 0(read)
   //  2:ONE        == 1(must be set)
   // 24:REGION bit 2 for instruction access
	R0.L = 0x0004;  // read, array access
	R0.H = 0x0100;

   P2.L = LO(0xFFA04000);  // L1 instr range accessible via ITEST_COMMAND
   P2.H = HI(0xFFA04000);
   
#if !defined(__SILICON_REVISION__)
   JUMP .instrp_okay;      // okay
#else
#define CHECKS_DONE
#endif
#endif

#if defined(__ADSPBF50x__) || !defined(__SILICON_REVISION__)
.check_bf50x:
// 0xFFA04000 - 0xFFA07FFF  L1 instruction SRAM/CACHE (16K)
// 0xFFA00000 - 0xFFA03FFF  L1 instruction Bank A SRAM (16K)
   // Is len just too big?
   R3 = 0x8000 (Z);
   CC = R2 <= R3 (IU);
   IF !CC JUMP .bad;

   R3.L = LO(0xFFA00000);  // L1 instruction memory base
   R3.H = HI(0xFFA00000);
   CC = R3 <= R1 (IU);
   IF !CC JUMP .bad;       // Too low
   R3.L = LO(0xFFA08000);  // L1 instruction memory end+1
   // assert: R3.H == 0xFFA0
   R0 = R1 + R2;
   CC = R0 <= R3 (IU);
   IF !CC JUMP .bad;       // Too high

   P2.L = LO(0xFFA04000);  // L1 instr cache 
   P2.H = HI(0xFFA04000);

   // set the value to be written to DTEST_COMMAND 
   R0.L = 0x0004;
   R0.H = 0x0100;

#if !defined(__SILICON_REVISION__)
   JUMP .instrp_okay;      // okay
#else
#define CHECKS_DONE
#endif
#endif
#endif

#if !defined(CHECKS_DONE)
.check_generic:
// 0xFFA10000 - 0xFFA13FFF  Code SRAM / cache (16K)
// 0xFFA00000 - 0xFFA0FFFF  Code SRAM (64K)
   // Is len just too big?
   R3.L = LO(0x140000);
   R3.H = HI(0x140000);
   CC = R2 <= R3 (IU);
   IF !CC JUMP .bad;

	R3.L = LO(0xFFA00000);
	R3.H = HI(0xFFA00000);
	CC = R3 <= R1 (IU);
	IF !CC JUMP .bad;	// Too low
	R3.L = LO(0xFFA14000);
	R3.H = HI(0xFFA14000);
	R0 = R1 + R2;
	CC = R0 <= R3 (IU);
	IF !CC JUMP .bad;	// Too high
	P2.L = LO(0xFFA10000);
	P2.H = HI(0xFFA10000);

   // set the value to be written to DTEST_COMMAND 
	R0.L = 0x0004;
	R0.H = 0x0100;
#endif
#endif

.instrp_okay:

	// Our inputs are okay.

#if !defined(__ADSPLPBLACKFIN__) || defined(__SILICON_REVISION__) || \
    defined(__ADSPBF561__)
	[--SP] = (R7:4);   // push preserved registers if not done already above
#endif

#if !defined(__ADSPLPBLACKFIN__)

	// Begin by saving the control registers

	P2.L = LO(DTEST_COMMAND);
	P2.H = HI(DTEST_COMMAND);
	I3.L = LO(ITEST_DATA1);
	I3.H = HI(ITEST_DATA1);
	I2.L = LO(ITEST_DATA0);
	I2.H = HI(ITEST_DATA0);
	I1.L = LO(IMEM_CONTROL);
	I1.H = HI(IMEM_CONTROL);
	I0.L = LO(ITEST_COMMAND);
	I0.H = HI(ITEST_COMMAND);
	R7 = [I3];
	R6 = [I2];
	R5 = [I1];
	R4 = [I0];
	R3 = [P2];
	[--SP] = (R7:3);

	// Zero ITEST_COMMAND and DTEST_COMMAND
	// (in case they have crud in them and
	// do a write somewhere when we enable cache)
	R7 = 0;
	[I0] = R7;
	[P2] = R7;
	CSYNC;

	// Switch into cache mode.

	R3 = (ENIM | IMC) (Z); // Enable L1, Cache mode
	// but first disable interrupts in case there are ISRs in L1.
	CLI R5;
	[--SP] = R5;
	[I1] = R3;
	CSYNC;

	R2 >>= 3;		// Looping in multiples of 8
	P2 = R2;
	R5 = 0x3F8 (Z);
	R6 = 0x3000 (Z);
	R7 = 0xC00 (Z);
	LSETUP(.ls, .le) LC0 = P2;
.ls:	// Compute the command from the physical address
	R2 = R1 & R5;		// Bits 9:3 as Index and Double-word index
	R4 = R1 & R6;		// Bits 13:12 as sub-bank
	R4 <<= 4;
	R2 = R2 | R4;
	R4 = R1 & R7;		// Bits 11:10 as Way
	R4 <<= 16;
	R2 = R2 | R4;
	BITSET(R2, 2);		// Select Data array
	BITTGL(R2, 27);		// Way bits are inverted!
	BITTGL(R2, 26);

	// Issue the command:
	[I0] = R2;
	CSYNC;

	// Read the resulting data back.
	R0 = [I2];
	R3 = [I3];
	CSYNC;

	// And write into the dest buffer.
	[P0++] = R0;
	[P0++] = R3;

	// Move to next instrp address
.le:	R1 += 8;

	R0 = [SP++];	// Pop saved IMASK
	// Restore the control registers
	(R7:3) = [SP++];
	[I2] = R6;
	[I3] = R7;
	[I0] = R4;
	[I1] = R5;
	CSYNC;
	STI R0;			// Re-enable interrupts

	P2.L = LO(DTEST_COMMAND);
	P2.H = HI(DTEST_COMMAND);

	[P2] = R3;
	CSYNC;


#else	/* __ADSPLPBLACKFIN__ */

	// Save all of the Instr and Data control registers.

	// First Instr registers
	I3.L = LO(ITEST_DATA1);
	I3.H = HI(ITEST_DATA1);
	I2.L = LO(ITEST_DATA0);
	I2.H = HI(ITEST_DATA0);
	I1.L = LO(IMEM_CONTROL);
	I1.H = HI(IMEM_CONTROL);
	I0.L = LO(ITEST_COMMAND);
	I0.H = HI(ITEST_COMMAND);
	R7 = [I3];
	R6 = [I2];
	R5 = [I1];
	R3 = R3 - R3 (NS) || R4 = [I0];
	[--SP] = (R7:4);

	CLI R7;        // disable interrupts before ITEST_COMMAND till after CSYNC
	// Zero ITEST_COMMAND (in case it contains a command
	// that will overwrite something)
	[I0] = R3;
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	// suppress assembler warning for 05-00-0312 as interrupts are now
	// disabled which means the code is safe.
	.MESSAGE/SUPPRESS 5515 FOR 2 LINES; 
#endif
	CSYNC;
	STI R7;        // reenable interrupts

	// Then Data registers
	I3.L = LO(DTEST_DATA1);
	I3.H = HI(DTEST_DATA1);
	I2.L = LO(DTEST_DATA0);
	I2.H = HI(DTEST_DATA0);
	I1.L = LO(DMEM_CONTROL);
	I1.H = HI(DMEM_CONTROL);
	I0.L = LO(DTEST_COMMAND);
	I0.H = HI(DTEST_COMMAND);
	R7 = [I3];
	R6 = [I2];
	R5 = [I1];
	R4 = [I0];
	[--SP] = (R7:4, P5:3);

	I1.L = LO(IMEM_CONTROL);
	I1.H = HI(IMEM_CONTROL);

	R2 >>= 3;		// Divide loop count by 8

	CLI R7;        // disable interrupts before ITEST_COMMAND till after CSYNC
	// Zero DTEST_COMMAND (in case it contains a command
	// that will overwrite something)
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	// suppress assembler warning for 05-00-0312 as interrupts are now
	// disabled which means the code is safe.
	.MESSAGE/SUPPRESS 5515; 
#endif
	[I0] = R3;
	CSYNC;
	LC0 = R2;
	STI R7;        // reenable interrupts
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	// restore assembler warning for 05-00-0312 now that interupts are enabled
	.MESSAGE/RESTORE 5515;   
#endif

	// If instrp is in the instr-only area of L1, then
	// we're starting on the Instr regs

	R5 = P2;
	CC = R1 < R5 (IU);
	IF !CC JUMP .reading_via_itest_command;

#if defined(__ADSPBF59x__)
   R5 = 0xC7F8 (Z);  // ADR[15:14] and ADR[10:3] bits
#else
	R5 = 0x47F8 (Z);  // here to deal with 05-00-0244
#endif
	R7 = 0x800 (Z);

	// Configure the SRAM/Cache to be in SRAM mode (write a zero).
	[I1] = R3;

#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	CLI R6;
#endif
	CSYNC;
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	STI R6;
#endif
	R6 = 0x3000 (Z);
#if !defined(__ADSPBF561__) && !defined(__ADSPBF50x__) && \
    !defined(__ADSPBF59x__)
	R3.L = 0x8000;  // Bank B start address requiring us to set DTESTCOMMAND:23
	R3.H = 0xFFA0;
#endif
#if defined(__ADSPBF51x__) && defined(__SILICON_REVISION__)
#if __SILICON_REVISION__ == 0xffff
    //BF51x changed memory map between 0.0 and 0.1  check silicon revision
    P4.L = 0x0014;
    P4.H = 0xFFC0;
    R4 = [P4];
    R4 >>= 28;
    P4=0;
    CC=R4==0;
    IF CC JUMP .loop_start; //0.0 silicon revision, so jump to start of loop
#endif
#if __SILICON_REVISION__ == 0xFFFF || __SILICON_REVISION__ > 0x0
    R2.L = LO(L1INSTRBANKB);
    R2.H = HI(L1INSTRBANKB);
    R4=R2-R1;
#if __SILICON_REVISION__ != 0xffff
    P4=0;
#endif
    CC=R4<=0;
    IF CC JUMP .pre_loop_start; //if we're already in BankB jump to pre loop
    //else we're in BankA so calculate how many loops before we hit BankB
    R4>>=3;   //calculate number of iterations before BankB
    R2=LC0;
    CC=R2<R4;
    IF CC JUMP .loop_start; //if we'll never hit BankB, just go.
    LC0=R4;   //loops in BankA
    R4=R2-R4; //loops in BankB
    P4=R4;
    JUMP .loop_start;
.second_iteration_start:
    //we jump back here when we move from BankA to BankB
    LC0=P4;
    P4=0;
.pre_loop_start:
    //set the value to be written to DTEST_COMMAND to enable bank B
    R0.L = 0x4004;
    R0.H = 0x0180;
.loop_start:
#endif
#endif
	LSETUP(.ils, .ile) LC0;
.ils:	
	// For each iteration, check whether we've run into
	// the SRAM/Cache area that gets accessed using ITEST_COMMAND. 
   // If so, we'll need to stop this loop.
	R4 = P2;
	CC = R1 < R4 (IU);
	IF !CC JUMP .reading_via_itest_command;

	R2 = R1 & R5;		// Bit 14, Index and Double-word Index
	R4 = R1 & R6;		// Sub-bank
	R4 <<= 4;
	R2 = R2 | R4;
	R4 = R1 & R7;		// Way
	R4 <<= 15;
	R2 = R2 | R4;
	R2 = R2 | R0;		// Select Data Array, Instr Access

#if !defined(__ADSPBF561__) && !defined(__ADSPBF50x__) && \
    !defined(__ADSPBF59x__)
            // DTEST_COMMAND:23 is always 0 for BF561 and BF59x parts instr mem
            // and reserved for BF50x parts
	CC = R3 <= R1 (IU);	// Select block
	R4 = CC;
	R4 <<= 23;
	R2 = R2 | R4;
#endif

#if WA_05000123
	P4 = R2;		// Save command value
	P3 = R3;		// free some dregs
	P5 = I0;		// move I0

.readloop:
	[P5] = P4;		// Issue the command
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	CLI R2;
#endif
	CSYNC;
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	STI R2;
#endif
	R2 = [I2];		// Read DTEST_DATA0
	R4 = [I3];		// Read DTEST_DATA1
	[P5] = P4;		// Re-issue the command
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	CLI R3;
#endif
	CSYNC;
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	STI R3;
#endif
	R3 = [I2];		// Read DTEST_DATA0
	CC = R2 == R3; // Values consistent?
	IF !CC JUMP .readloop;

#if WA_05000428 || WA_05000245
   // 050000428: Speculative read from L2 by Core B may cause a write 
   //            to L2 to fail
   // 050000245: False Hardware Error from an Access in the Shadow of 
   //            a Conditional Branch
   // Suppress the assembler warnings associated with the following
   // instruction because it is loading DTEST_DATA1 so is not an L2 read
   // or an invalid access so is safe from both anomalies.
.MESSAGE/SUPPRESS 5517,5508 FOR 2 LINES;
#endif
   R3 = [I3];     // Read DTEST_DATA1, ea5517 and ea5508 might be suppressed

	CC = R4 == R3; // Values consistent?
	IF !CC JUMP .readloop;

	R3 = P3;
#else 
	CLI R4;        // disable interrupts before DTEST_COMMAND till after CSYNC
	[I0] = R2;		// Issue the command
	// suppress assembler warning for 05-00-0312 as interrupts are now
	// disabled which means the code is safe.
	.MESSAGE/SUPPRESS 5515; 
   CSYNC;
	STI R4;        // reenable interrupts
	R2 = [I2];
	R4 = [I3];
#endif
 
	[P0++] = R2;		// Instruction data is in R2 and R4
	[P0++] = R4;            // ... so store in datap.
.ile:	R1 += 8;		// Advance instrp.
#if defined(__ADSPBF51x__) && \
    (defined(__ADI_LIB_BUILD__) || __SILICON_REVISION__ > 0x0)
    CC=P4==0;
    IF !CC JUMP .second_iteration_start;
#endif

	// If we fall off the end of the loop, we've read all
	// the data we need to read.
	JUMP .done_reading;

.reading_via_itest_command:

	// We're going to be reading from the SRAM/Cache part
	// of L1 Instruction memory now, which requires a
	// different configuration. 

	// Switch into Cache mode
	// disabling interrupts first
	CLI R0;
	// suppress assembler warning for 05-00-0312 as interrupts are now
	// disabled which means the code is safe.
	.MESSAGE/SUPPRESS 5515; 
	R3 = (IMC) (Z);
	[I1] = R3;
	CSYNC;

	// Now set up the control register addresses.

	I3.L = LO(ITEST_DATA1);
	I3.H = HI(ITEST_DATA1);
	I2.L = LO(ITEST_DATA0);
	I2.H = HI(ITEST_DATA0);
	I0.L = LO(ITEST_COMMAND);
	I0.H = HI(ITEST_COMMAND);

	R5 = 0x3F8 (Z);
	R6 = 0x3000 (Z);
	R7 = 0xC00 (Z);
	LSETUP (.dls, .dle) LC0;
.dls:
	// Construct the address command from the instrp address
	R2 = R1 & R5;  // Index and Double-word index : ADR[9:3]
	R4 = R1 & R6;  // sub-bank : ADR[13:12]
	R4 <<= 4;
	R2 = R2 | R4;
	R4 = R1 & R7;  // Way (and no inverting on LP cores) : ADR[11:10]
	R4 <<= 16;
	R2 = R2 | R4;
	BITSET(R2, 2); // Data array

	// Issue command - interrupts are disabled already
	[I0] = R2;
	CSYNC;

	// Read instruction data
	R2 = [I2];
	R4 = [I3];

	// Write to datap
	[P0++] = R2;
	[P0++] = R4;

	// Advance instrp
.dle:	R1 += 8;

	// Re-enable interrupts
	STI R0;
	// restore assembler warning for 05-00-0312 now that interupts are enabled
	.MESSAGE/RESTORE 5515;   

.done_reading:
	// Now read all the data we need to read, so restore
	// the control registers.

	// First Data registers
	I3.L = LO(DTEST_DATA1);
	I3.H = HI(DTEST_DATA1);
	I2.L = LO(DTEST_DATA0);
	I2.H = HI(DTEST_DATA0);
	I1.L = LO(DMEM_CONTROL);
	I1.H = HI(DMEM_CONTROL);
	I0.L = LO(DTEST_COMMAND);
	I0.H = HI(DTEST_COMMAND);
	(R7:4, P5:3) = [SP++];
	[I3] = R7;
	[I2] = R6;
#if WA_05000125
	CLI R6;
	.align 8;
	[I1] = R5;
	// suppress assembler warning for 05-00-0312 for ssync because interrupts
	// are disabled.
	.MESSAGE/SUPPRESS 5515 FOR 1 LINES;  
	SSYNC;
	STI R6;
#else
	[I1] = R5;
#endif
   CLI R3;        // disable interrupts before DTEST_COMMAND till after CSYNC
	[I0] = R4;
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	// suppress assembler warning for 05-00-0312 as interrupts are now
	// disabled which means the code is safe.
	.MESSAGE/SUPPRESS 5515 FOR 2 LINES; 
#endif
   CSYNC;
   STI R3;        // reenable interrupts
   
	// Then Instr registers
	I3.L = LO(ITEST_DATA1);
	I3.H = HI(ITEST_DATA1);
	I2.L = LO(ITEST_DATA0);
	I2.H = HI(ITEST_DATA0);
	I1.L = LO(IMEM_CONTROL);
	I1.H = HI(IMEM_CONTROL);
	I0.L = LO(ITEST_COMMAND);
	I0.H = HI(ITEST_COMMAND);
	(R7:4) = [SP++];
	[I3] = R7;
	[I2] = R6;

#if WA_05000125
	CLI R0;
.align 8;
	[I1] = R5;
	// suppress assembler warning for 05-00-0312 for ssync because interrupts
	// are disabled.
	.MESSAGE/SUPPRESS 5515 FOR 1 LINES;  
	SSYNC;
	STI R0;
#else
	[I1] = R5;
#endif

   CLI R3;        // disable interrupts before ITEST_COMMAND till after CSYNC
	[I0] = R4;
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	// suppress assembler warning for 05-00-0312 as interrupts are now
	// disabled which means the code is safe.
	.MESSAGE/SUPPRESS 5515 FOR 2 LINES; 
#endif
	CSYNC;
   STI R3;        // reenable interrupts

#endif	/* __ADSPLPBLACKFIN__ */

	// Return datap
	R0 = P1;
	(R7:4) = [SP++];
.finished:
	RTS;
.bad:
#if defined(__ADSPLPBLACKFIN__) && !defined(__SILICON_REVISION__) && \
    !defined(__ADSPBF561__)
	(R7:4) = [SP++];
#endif
	R0 = 0;
	RTS;

.__l1_memcpy.end:
.global __l1_memcpy;
.type __l1_memcpy, STT_FUNC;

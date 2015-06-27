/* Copyright (C) 2007-2008 Analog Devices, Inc. All Rights Reserved. */
.file_attr libName=libevent;
.file_attr FuncName=_flush_data_cache;
.file_attr libFunc=_flush_data_cache;
.file_attr libFunc=flush_data_cache;
.file_attr FuncName=__dcplb_flush;
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";

#include <cplb.h>
#include <sys/anomaly_macros_rtl.h>       // include WA_ macro definitions
#if !defined(ENICPLB_P)
   // ENICPLB_P not defined for BF535
#  define ENICPLB_P 1
#endif

.extern ___page_prefix_table;
.type ___page_prefix_table, STT_OBJECT;

.section cplb_code; /* non cached and CPLB locked */
.align 2;

// Flush all of the dirty DCPLBs. This is intended to
// be called by external functions, and not necessarily
// in an interrupt handler.
// void flush_data_cache(void)

_flush_data_cache:
	[--SP] = (R7:7, P5:4);
	LINK 12;

	P1.L = ___page_prefix_table;
	P1.H = ___page_prefix_table;
	P5.H = DCPLB_ADDR0 >> 16;
	P5.L = DCPLB_ADDR0 & 0xFFFF;
	P4.H = DCPLB_DATA0 >> 16;
	P4.L = DCPLB_DATA0 & 0xFFFF;
	R7 = (CPLB_VALID | CPLB_L1_CHBL | CPLB_DIRTY) (Z);
	P0 = __CPLB_TABLE_SIZE;
	LSETUP(.fstart, .fend) LC0 = P0;
.fstart:	R0 = [P5++];
		R1 = [P4++];
		CC = BITTST(R1, 14);	// Is it write-through?
		IF CC JUMP .skip;	// If so, ignore it.
		R2 = R1 & R7;		// Is it a dirty, cached page?
		CC = R2 == R7;
		IF !CC JUMP .skip;	// If not, ignore it.
		// Extract page size from the CPLB (bits 17:16 or 18:16), and 
		// scale to give an offset into the page_prefix tables.
		R2 = R1 << 14;
		R2 >>= 30;
		R2 <<= 2;
		P0 = R2;
		P0 = P1 + P0;
		R2 = [P0];	// This is the address prefix mask

              // R0 = page, R1 = data, R2 = page prefix
		CALL.X __dcplb_flush;  
#if 0
#warning CPLB flushing clears CPLB dirty bit
		BITCLR(R1,7);
		[P4-4]=R1;
#endif
.fend:
.skip:		NOP;

	UNLINK;
	(R7:7, P5:4) = [SP++];
	RTS;
._flush_data_cache.end:
.global _flush_data_cache;
.type _flush_data_cache, STT_FUNC;


__dcplb_flush:
	// This is an internal function to flush all pending
	// writes in the cache associated with a particular DCPLB.
	// R0 contains the page's start address, and R1 contains
	// the CPLB's data field. R2 contains an index into the
	// page prefix table.
	// WARNING: this function does *not* follow the C ABI; it
	// is defined to not clobber any registers, so must save
	// everything it modifies (apart from ASTAT).
	[--SP] = (R7:0, P5:0);
	[--SP] = LC0;
	[--SP] = LT0;
	[--SP] = LB0;
	[--SP] = LC1;
	[--SP] = LT1;
	[--SP] = LB1;
	[--SP] = I0;

	// If it's a 1K or 4K page, then it's quickest to
	// just systematically flush all the addresses in
	// the page, regardless of whether they're in the
	// cache, or dirty. If it's a 1M or 4M page (and 16K/64K
	// pages on certain parts), there are too many addresses,
	// and we have to search the cache for lines corresponding
	// to the page.

	CC = BITTST(R1, 17);	// 1MB or 4MB

	P4.L = DMEM_CONTROL & 0xFFFF;
	P4.H = DMEM_CONTROL >> 16;
	R5 = [P4];

#if !defined(__ADSPLPBLACKFIN__)
	// Ensure DCPLBs are enabled so the flushing instructions operate on
	// BF535 compatible cores. They will not be if the callee is cplb_mgr.
	// The state of DMEM_CONTROL:ENDCPLB is restored before return.
	BITSET(R5,ENDCPLB_P);
	[P4] = R5;
	SSYNC;
#endif			
	IF !CC JUMP flush_whole_page;

	[--SP] = I1;
#if WA_05000123
	[--SP] = I2;
#endif
	R7 = R2;	// This is the address prefix mask

	P2.L = DTEST_COMMAND & 0xFFFF;
	P2.H = DTEST_COMMAND >> 16;
	P3.L = DTEST_DATA0 & 0xFFFF;
	P3.H = DTEST_DATA0 >> 16;
#if WA_05000123
	P5.L = DTEST_DATA1 & 0xFFFF;
	P5.H = DTEST_DATA1 >> 16;
#endif
	// Inner loop iterations = number of cache line sets per way
	P0 = 64;

	// Iterations of second innermost loop = number of sub-banks
	P1 = 4;

	// Initialise sub-bank deposit foregrounds (sub-bank bits in upper
	// halves initialised to zero).
	R3 = ((16<<8)|2);	// Command foreground
	R4 = ((12<<8)|2);	// Address foreground

	// Initialise command value to perform a read of data tag array,
	// starting with way, bank, sub-bank, line select bits all 0.
	R6 = 0;

	// If DCBS = 1, address bit 23 is used to determine cache bank
	// so only one bank is of interest, which can be determined from
	// the page start address. Set the bank select bit in the command
	// value appropriately.
	// If DCBS = 0, address bit 14 is used to determine cache bank
	// so addresses in 64K, 1M, or 4M pages could be cached in either
	// bank A or B.

	I0 = R5;		// Save DMEM_CONTROL value for later
	CC = BITTST(R5,4);	// Check DCBS
	IF !CC JUMP .bankloopstart;
	
	CC = BITTST(R0, 23);
	R5 = R6;
	BITSET(R5, 23);

	// If address bit 23 is set then address alisases to bank B, otherwise
	// bank A. 
	IF CC R6 = R5;	// Set bank select bit in command

	// Examine cache lines and flush those which are dirty and have
	// data in the page of interest
	//
	// In pseudo-code the algorithm is:
	//
	// for (data bank A and/or data bank B) {
	//   for (way 0 and way 1) {
	//     for (sub-bank 0 to 3) {
	//       for (cache lines 0 to 63) {
	//         read_cache_line_tag(line)
	//         if (line valid and dirty) {
	//           if (line tag matches page start address) {
	//             flush_address()
	//           }
	//         }
       //       }
	//     }
       //   }
       // } 
	//
	// Within the loops, registers have the following uses:
	//
	// R0 has page start address
	// R1 incremented within inner loop by 32 to give line select bits
	// R2 used as temporary
	// R3 contains sub-bank deposit foreground for command value 
	// R4 contains sub-bank deposit foreground for address 
	// R5 used as temporary
	// R6 contains command bits built up during execution of loops 
	// R7 has page address mask
	// P0 is line count
	// P1 is sub-bank count
	// P2 contains DTEST_COMMAND address
	// P3 contains DTEST_DATA0 address
	// P4 used as temporary
	// P5 has DTEST_DATA1 address (for WA_05000123)
	// I0 has DMEM_CONTROL value
	// I1 used as temporary (for WA_05000123)
	// I2 used as temporary (for WA_05000123)

.bankloopstart:

	R1 = 0;		// Initialise line select bits

.wayloopstart:

	LSETUP(.subbankloopstart,.subbankloopend) LC0 = P1;
.subbankloopstart:

	LSETUP(.lineloopstart,.lineloopend) LC1 = P0;
.lineloopstart: 
	// Deposit line bits into command
	R5 = R6 | R1;

#if WA_05000123
	// Workaround the anomaly by preceding writes to DTEST_COMMAND with a
	// CSYNC, and repeatedly read both the DTEST_DATAx registers until
	// consistent data is returned on two consecutive reads.
	I1 = R0;	// Free some data regs
	I2 = R1;
	P4 = R5;	// Save command value
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
       CLI R0;
       // suppress assembler warning for 05-00-0312 as interrupts are now
       // disabled which means the code is safe.
       .MESSAGE/SUPPRESS 5515;
#endif
	CSYNC;
	[P2] = P4;	// Write command value
	CSYNC;
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
       STI R0;
       // restore assembler warning for 05-00-0312
       .MESSAGE/RESTORE 5515;
#endif
	R0 = [P3];           // Read DTEST_DATA0
	R1 = [P5];           // Read DTEST_DATA1
.readloop:
	R5 = R0;
	R2 = R1;
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
       CLI R0;
       // suppress assembler warning for 05-00-0312 as interrupts are now
       // disabled which means the code is safe.
       .MESSAGE/SUPPRESS 5515;
#endif
	CSYNC;
	[P2] = P4;	// Write command value again
	CSYNC;
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
       STI R0;
       // restore assembler warning for 05-00-0312
       .MESSAGE/RESTORE 5515;
#endif
	R0 = [P3];           // Read DTEST_DATA0
	CC = R0 == R5;	// Values consistent?
	IF !CC JUMP .readloop;
       // The following load from P5 is safe for 05000428 because it is not
       // from L2 so suppress the assemble rwarning.
       .MESSAGE/SUPPRESS 5517 FOR 1 LINES;
	R1 = [P5];           // Read DTEST_DATA1
	CC = R1 == R2;	// Values consistent?
	IF !CC JUMP .readloop;
	// R5 has correct value of DTEST_DATA0
	R0 = I1;	// Restore regs
	R1 = I2;
#else
	// Write command into DTEST_COMMAND 
	[P2] = R5;
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
       CLI R5;
#endif
       CSYNC;
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
       STI R5;
#endif

	// Read DTEST_DATA0 
	R5 = [P3];
#endif

	CC = BITTST(R5, 0);		// Check if line valid
	IF !CC JUMP .lineloopend;	// and skip if not.
	CC = BITTST(R5, 1);		// Check if line dirty
	IF !CC JUMP .lineloopend;	// and skip if not.

	// Compare the tag bits with the page start address
	R2 = R5 & R7;
	CC = R2 == R0;
	IF !CC JUMP .lineloopend;

	// Produce address to be flushed from the 18 MSBs and bit 11 returned in
	// the tag, and sub-bank bits built up in R4 within the outer loops,
	// and the line select bits in R1
	R5 >>= 11;		// Extract tag bits only
	R5 <<= 11;
	R5 = R5 | R1;		// OR in line select bits
#if defined(__WORKAROUND_DREG_COMP_LATENCY)
	NOP;
#endif
	R5 = DEPOSIT(R5, R4);// Deposit sub-bank bits into correct position
	P4 = R5;
	FLUSHINV[P4];		// And flush the addresss
.lineloopend:
	R1 += 32;		// Increment to next line

	R2.L = R4.H << 0;	// Copy sub-bank select bits
	R2 += 1;		// and increment to next sub-bank
	R4.H = R2.L << 0;	// Copy back to sub-bank foreground reg

	// Deposit sub-bank bits into command value
	R3.H = R2.L << 0;
	R1 = 0;		// Re-set line select bits
.subbankloopend:
	R6 = DEPOSIT(R6,R3);

	// If way bit set command value, both ways already flushed, jump out
	CC = BITTST(R6,26);
	IF CC JUMP .wayloopend;
	// Move on to second way by setting way bits in command value
	BITSET(R6,26);
	JUMP .wayloopstart;
.wayloopend:

	// If the bank select bit is 14, and we've not already
	// flushed bank B, then reset the command value appropriately
	// and start bankloop again.
	R5 = I0;			// Load saved DMEM_CONTROL value
	CC = BITTST(R5, 4);		// Check DCBS bit
	IF CC JUMP .bankloopend;	// Only one bank to flush, finished
	CC = BITTST(R6, 23);		// Check command bank select bit
	IF CC JUMP .bankloopend;	// Already flushed bank B, finished
	// Bank B needs flushing, re-set command value
	R6 = 0;
	BITSET(R6, 23);
	JUMP .bankloopstart;

.bankloopend:
	R1 = 0;	// Zero DTEST_COMMAND
	[P2] = R1;

#if WA_05000123
	I2 = [SP++];
#endif
	I1 = [SP++];
.finished:
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
       CLI R0;
#endif
	SSYNC;		// Ensure the data gets out to mem.
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
       STI R0;
#endif

#if !defined(__ADSPLPBLACKFIN__)
	P4.L = DMEM_CONTROL & 0xFFFF;
	P4.H = DMEM_CONTROL >> 16;
	// Restore the state of DMEM_CONTROL on entering the function
	R5 = I0;
	[P4] = R5;
	SSYNC;
#endif			
	// Finished. Restore context.
	I0 = [SP++];
	LB1 = [SP++];
	LT1 = [SP++];
	LC1 = [SP++];
	LB0 = [SP++];
	LT0 = [SP++];
	LC0 = [SP++];
	(R7:0, P5:0) = [SP++];
	RTS;

flush_whole_page:
	// It's a 1K or 4K page, so quicker to just flush the
	// entire page.
	P1 = 32;		// For 1K pages
	P2 = P1 << 2;		// For 4K pages
	P0 = R0;		// Start of page
	CC = BITTST(R1, 16);	// Whether 1K or 4K
	IF CC P1 = P2;
#if WA_05000096
	P1 += -1;		// Unroll one iteration
	LSETUP (.sall, .eall) LC0 = P1;
	FLUSHINV [P0++];	// because CSYNC can't end loops.
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
       CLI R0;
       // suppress assembler warning for 05-00-0312 as interrupts are now
       // disabled which means the code is safe.
       .MESSAGE/SUPPRESS 5515;
#endif
.sall:	CSYNC;
.eall:	FLUSHINV [P0++];
	CSYNC;
#if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
       STI R0;
       // restore assembler warning for 05-00-0312
       .MESSAGE/RESTORE 5515;
#endif
#else
	LSETUP (.all, .all) LC0 = P1;
.all:	FLUSHINV [P0++];
#endif
	JUMP .finished;
.__dcplb_flush.end:
.global __dcplb_flush; 
.type __dcplb_flush, STT_FUNC;

/*
vi:ts=7
*/

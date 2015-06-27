/* Copyright (C) 2003-2009 Analog Devices, Inc. All Rights Reserved. */
//****************************************************************************
// void cache_invalidate(int caches)
// caches has bits set to mean
//   bit 0 - invalidate instruction cache
//   bit 1 - invalidate data cache A
//   bit 2 - invalidate data cache B. (ignored for ADSP-BF50x)
//****************************************************************************
// This library function can be called to invalidate instruction and
// data caches. It is designed to be called from a user application
// and for startup on the BF535, whose invalid bits are random coming
// out of RESET.
// 
// If called from a user application, dirty lines must first be flushed.
// This is not NOT done here.
//
// To perform the invalidation, the SRAM block in question must first be
// enabled, and as cache, when any current SRAM contents become inaccessible.
// In particular, if this code happens to be there it is like cutting off
// your own legs.
// 

.file_attr libName=libevent;
.file_attr FuncName=_cache_invalidate;
.file_attr libFunc=_cache_invalidate;
.file_attr libFunc=cache_invalidate;
.file_attr FuncName=_icache_invalidate;
.file_attr libFunc=_icache_invalidate;
.file_attr libFunc=icache_invalidate;
.file_attr FuncName=_dcache_invalidate;
.file_attr libFunc=_dcache_invalidate;
#if !defined(__ADSPBF50x__) /* The ADSP-BF50x part only have data Bank A */
.file_attr FuncName=_dcache_invalidate_both;
.file_attr libFunc=_dcache_invalidate_both;
.file_attr libFunc=dcache_invalidate_both;
#endif
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";

#include <cplb.h>
#include <sys/anomaly_macros_rtl.h> // include WA_ macro definitions
 
.section program;
// We cannot be sure that that is not currently an SRAM block about to
// be reconfigured as cache and invalidated, but if there is no L2/L3
// memory what is being cached? So we expect this code to be in L2/L3.

.global _cache_invalidate;
.type _cache_invalidate, STT_FUNC;
.global _icache_invalidate;
.type _icache_invalidate, STT_FUNC;
.global _dcache_invalidate;
.type _dcache_invalidate, STT_FUNC;
#if !defined(__ADSPBF50x__) /* The ADSP-BF50x part only have data Bank A */
.global _dcache_invalidate_both;
.type _dcache_invalidate_both, STT_FUNC;
#endif

.align 2;

_cache_invalidate:
    //********************************************
    //** Icache or DcacheA or DcacheB Invalidation
    //** or any combination thereof
    //********************************************
    // R0 has bits
    //   CPLB_ENABLE_ICACHE_P,CPLB_ENABLE_DCACHE_P,CPLB_ENABLE_DCACHE2_P
    // set as required
	LINK 0;
	[--SP] = R7;

	R7 = R0;
	CC = BITTST(R7,CPLB_ENABLE_ICACHE_P);
	IF !CC JUMP .no_icache;
	CALL.X _icache_invalidate;
.no_icache:
#if    defined(__ADSPLPBLACKFIN__) \
    && !defined(__ADSPBF50x__)  /* The ADSP-BF50x part only have data Bank A */
	// Check whether both bits are set, and call the
	// more efficient invalidation routine
	R0 = (CPLB_ENABLE_DCACHE|CPLB_ENABLE_DCACHE2) (Z);
	R1 = R0 & R7;
	CC = R0 == R1;
	IF !CC JUMP .not_both;
	CALL.X _dcache_invalidate_both;
	JUMP .finished;
.not_both:
#endif
	// Check for each bit, and call a per-cache
	// invalidation routine
	CC = BITTST(R7,CPLB_ENABLE_DCACHE_P);
	IF !CC JUMP .no_dcache_a;
	R0 = 0;         // specifies bank A
	CALL.X _dcache_invalidate;
.no_dcache_a:
#if !defined(__ADSPBF50x__) /* The ADSP-BF50x part only have data Bank A */
	CC = BITTST(R7,CPLB_ENABLE_DCACHE2_P);
	IF !CC JUMP .no_dcache_b;
	R0 = 0;
	BITSET(R0, 23); // specifies bank B
	CALL.X _dcache_invalidate;
#endif
.no_dcache_b:
.finished:
	R7 = [SP++];
	UNLINK;
	RTS;
._cache_invalidate.end:

.align 2;
_icache_invalidate:
    //***********************************
    //** Icache Invalidation
    //***********************************
#ifdef __ADSPLPBLACKFIN__
	// On BF531/2/3, entire cache can be invalidated by clearing the IMC bit in
	// IMEM_CONTROL.

	P0.L = (IMEM_CONTROL & 0xFFFF);
	P0.H = (IMEM_CONTROL >> 16);
    // Save the existing value of IMEM_CONTROL
	R1 = [P0];

	R0 = R1;	// Clear IMC bit.
	BITCLR(R0, IMC_P);

#if WA_05000125 || defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	CLI R2;              // workaround anomaly ID 05000125 & 05-00-0312
	SSYNC;		// SSYNC required before invalidating cache.
	.align 8;
	[P0] = R0;
	.MESSAGE/SUPPRESS 5515 FOR 1 LINES; 
	CSYNC;		// And a CSYNC after each operation.
	STI R2;
#else
	[P0] = R0;
	SSYNC;
#endif

	// Write saved IMEM_CONTROL value to restore IMEM configuration.
 	// (Working around anomalies when writing to IMEM_CONTROL).
#if WA_05000125 || defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	CLI R2;              // workaround anomaly ID 05000125 & 05-00-0312
	.align 8;
	[P0] = R1;
	.MESSAGE/SUPPRESS 5515 FOR 1 LINES; 
	SSYNC;
	STI R2;
#else
	[P0] = R1;
	SSYNC;
#endif

#else

#if WA_05000125 || defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
/* neither 05-00-0312 or 05-00-0125 should be required for no 10x core parts */
#   error if 05-00-0312 or 05-00-0125  workarounds are required changes needed
#endif
	[--SP] = (R7:4, P5:5);
    // Enable, and as cache. This will already be the case if this is a
    // user request, but for use at RESET it will be enabled as SRAM.
	P0.L = (IMEM_CONTROL & 0xFFFF);
	P0.H = (IMEM_CONTROL >> 16);
    // Save the current settings of IMEM_CONTROL, so that we don't
    // clobber it by invalidation.
	R7 = [P0];
#if defined(__ADSPLPBLACKFIN__)
	R0 = (IMC);
#else
	R0 = (IMC | ENIM);
#endif
	[P0]=R0;
	SSYNC;
	I0.L = (ITEST_COMMAND & 0xFFFF);
	I0.H = (ITEST_COMMAND >> 16);
    // When the TEST_COMMAND register is written, the tag or line
    // contents for the line it addresses is read or written to/from the
    // TEST_DATA0/TEST_DATA1 registers respectively.
    //
    // To invalidate a line we write zero to the tag for the line.
    //   
	I1.L = (ITEST_DATA0 & 0xFFFF);
	I1.H = (ITEST_DATA0 >> 16);
	R3=0;
	[I1]=R3;

    // I-CACHE:
    //   4 sub-banks, 
    //   each sub-bank has 4 ways, 
    //   each way has 32 lines,
    //  
	R2=32;	    // line index increment
	BITSET(R3,16);   // sub-bank increment
	P2=4;	    // number of sub-banks, outer loop iterations
                    // (Should be 2 for D-cache)
	P5=R2;      // number of lines in each way
	R4=2;       // initial value for ITEST_COMMAND for way 0
		        // write to tag for sub-bank 0, way 0, line 0
	R5.H=0x400;	// initial value for ITEST_COMMAND for way 1
	R5.L=2;		// write to tag for sub-bank 0, way 1, line 0
	LSETUP(lbl0a,lbl3a) LC1=P2; // for each of 4 sub-banks
lbl0a:  R0=R4;
	R1=R5;
	LSETUP(lbl1a,lbl2a) LC0=P5; // for each of 32 lines
lbl1a:	R0 = R0+|+R2 || [I0]=R0;// way 0
lbl2a:	R1 = R1+|+R2 || [I0]=R1;// way 1
	R4=R4+R3;   // next sub-bank way 0
lbl3a:  R5=R5+R3;   // next sub-bank way 1
    // and now do it all again fors ways 2 and 3
	R4.H=0x800;
	R4.L=2;  // initial value for ITEST_COMMAND for way 2                
	R5.H=0xc00;
	R5.L=2;  // initial value for ITEST_COMMAND for way 3
	LSETUP(lbl0b,lbl3b) LC1=P2;
lbl0b:	R0=R4;
	R1=R5;
	LSETUP(lbl1b,lbl2b) LC0=P5;
lbl1b:	R0 = R0+|+R2 || [I0]=R0;
lbl2b:	R1 = R1+|+R2 || [I0]=R1;
	R4=R4+R3;
lbl3b:  R5=R5+R3;

    // Restore the previous value of IMEM_CONTROL
	CSYNC;

	/*
	** The emulator can not function correctly if a write command is left
	** in ITEST_COMMAND or DTEST_COMMAND AND access to cache memory is 
	** needed by the emulator. To avoid such problems, write a zero.
	*/
	R0=0;
	[I0]=R0;

	// Restore the previous value of IMEM_CONTROL
	CSYNC;   // CSYNC require after write to DTEST_COMMAND
	[P0] = R7;
	SSYNC;
	(R7:4, P5:5) = [SP++];
#endif
	RTS;
._icache_invalidate.end:

#if !defined(__ADSPBF50x__) /* The ADSP-BF50x part only have data Bank A */
.align 2;
_dcache_invalidate_both:
    //***********************************
    //** DCACHE+DCACHE2 Invalidation
    //***********************************

#ifdef __ADSPLPBLACKFIN__
	// On BF531/2/3, entire cache can be invalidated by clearing the DMC bits in
	// DMEM_CONTROL (apart from BF532 rev 0.0 - anomaly #15).

	P0.L = (DMEM_CONTROL & 0xFFFF);
	P0.H = (DMEM_CONTROL >> 16);
    // Save the existing value of DMEM_CONTROL
	R1 = [P0];
 
	// Clear bits DMC[1:0]
	R0 = R1;
	BITCLR(R0, DMC0_P);
	BITCLR(R0, DMC1_P);

#if WA_05000125 || defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	CLI R2;              // workaround anomaly ID 05000125 & 05-00-0312
	SSYNC;		// SSYNC required before invalidating cache.
	.align 8;
	[P0] = R0;
	.MESSAGE/SUPPRESS 5515 FOR 1 LINES; 
	SSYNC;		// And a SSYNC after each operation.
	STI R2;
#else
	[P0] = R0;
	SSYNC;
#endif

	// Write saved DMEM_CONTROL value to restore DMEM configuration.
 	// (Working around anomalies when writing to DMEM_CONTROL).
#if WA_05000125 || defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	CLI R2;              // workaround anomaly ID 05000125 & 05-00-0312
	.align 8;
	[P0] = R1;
	.MESSAGE/SUPPRESS 5515 FOR 1 LINES; 
	SSYNC;
	STI R2;
#else
	[P0] = R1;
	SSYNC;
#endif

#else
	LINK 12;
	R0 = 0 (Z);					// First invalidate CACHE A
	CALL.X _dcache_invalidate;
	BITSET(R0, CPLB_INVALIDATE_B_P);	// Then invalidate CACHE B
	CALL.X _dcache_invalidate;
	UNLINK;
#endif /* __ADSPLPBLACKFIN__ */
	RTS;
._dcache_invalidate_both.end:
#endif /* !__ADSPBF50x__ */

.align 2;
_dcache_invalidate:
    //***********************************
    //** DCACHE Invalidation
    //***********************************

    // R0 has bit 23 set to specify bank A or bank B
    // Which corresponds to the Data Bank Access bit in DTEST_COMMAND
	[--SP] = (R7:5);

	P0.L = (DMEM_CONTROL & 0xFFFF);
	P0.H = (DMEM_CONTROL >> 16);
    // Save the existing value of DMEM_CONTROL, so that we
    // don't clobber it by invalidation.
	R6 = [P0];
	R1 = (ACACHE_BCACHE | ENDM);
    // Enable both as cache although we are going to invalidate only 1.
    // This is because if only 1 bank is configured as cache it must be
    // bank A. However if a user has both banks as cache and wishes to
    // invalidate only bank B, we cannot enable it alone as cache.
    // Bank A must already be cache in this case, so no SRAM data is lost.
    // At RESET both will currently be enabled as SRAM.

	// Write saved DMEM_CONTROL value to restore DMEM configuration.
 	// (Working around anomalies when writing to DMEM_CONTROL).
#if WA_05000125 || defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	CLI R5;              // workaround anomaly ID 05000125 & 05-00-0312
	.align 8;
	[P0]=R1;
	.MESSAGE/SUPPRESS 5515 FOR 1 LINES; 
	SSYNC;
	STI R5;
#else
	[P0]=R1;
	SSYNC;
#endif
	I0.L = (DTEST_COMMAND & 0xFFFF);
	I0.H = (DTEST_COMMAND >> 16);
	I1.L = (DTEST_DATA0 & 0xFFFF);
	I1.H = (DTEST_DATA0 >> 16);

	R3=0;
	[I1]=R3;

    //  D-CACHE:
    //    2 superbanks,
    //    each superbank has 4 sub-banks, 
    //    each sub-bank has 2 ways, 
    //    each way has 64 lines,
	BITSET(R3,16);   // sub-bank increment
	P2=4;   // number of sub-banks
	P1=64;  // number of lines in each way
	R2=32;	// line index increment

	R7=2;    // initial value for DTEST_COMMAND for way 0
            // write to tag for sub-bank 0, way 0, line 0
            // *********************************************************
            // for BF535, bit 24 is reserved, but for BF532 it signifies
            // 0 = access data, 1= access instruction
            // *********************************************************
	R5.H=0x400;	// Initial value for DTEST_COMMAND for way 1
	R5.L=2;     // write to tag for sub-bank 0, way 1, line 0
	R7=R7|R0;   // include superbank specifier
	R5=R5|R0; 
	LSETUP(lbl0c,lbl3c) LC1=P2; // 4 sub-banks
lbl0c:   R0=R7;
         R1=R5;
         LSETUP(lbl1c,lbl2c) LC0=P1; // 32 or 64 lines
lbl1c:      R0 = R0+|+R2 || [I0]=R0;
lbl2c:      R1 = R1+|+R2 || [I0]=R1;
         R7=R7+R3;
lbl3c:   R5=R5+R3;

	/*
	** The emulator can not function correctly if a write command is left
	** in ITEST_COMMAND or DTEST_COMMAND AND access to cache memory is 
	** needed by the emulator. To avoid such problems, write a zero.
	*/
	R0=0;
	[I0]=R0;

	// Restore the original value of DMEM_CONTROL
#if WA_05000125 || defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	CLI R5;              // workaround anomaly ID 05000125 & 05-00-0312
	CSYNC;   // CSYNC require after write to DTEST_COMMAND
	.align 8;
	[P0] = R6;
	.MESSAGE/SUPPRESS 5515 FOR 1 LINES; 
	SSYNC;
	STI R5;
#else
	CSYNC;   // CSYNC require after write to DTEST_COMMAND
	[P0] = R6;
	SSYNC;
#endif
	(R7:5) = [SP++];
	RTS;
._dcache_invalidate.end:

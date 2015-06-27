/* Copyright (C) 2003-2009 Analog Devices, Inc. All Rights Reserved. */
/*********************************************************************/
//
/*********************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libName=libevent;
.file_attr FuncName=_enable_data_cache;
.file_attr libFunc=_enable_data_cache;
.file_attr libFunc=enable_data_cache;
.file_attr FuncName=_disable_data_cache;
.file_attr libFunc=_disable_data_cache;
.file_attr libFunc=disable_data_cache;
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";
#endif

.section program;
.align 2;

#include <sys/anomaly_macros_rtl.h>    // include WA_ macro definitions
#include <sys/platform.h>
#include "cplb.h"

// Public functions to turn the data cache on and off.
// Intended for use with the flushing function.
// The enable function accepts ___cplb_ctrl as a parameter.

// void enable_data_cache(int cplb_ctrl)
_enable_data_cache:
   R1 = CPLB_ENABLE_ANY_CPLBS;
   R1 = R1 & R0;
   CC = R1;
   IF !CC JUMP .f_end;
   P0.L = (DMEM_CONTROL & 0xFFFF);
   P0.H = (DMEM_CONTROL >> 16);

   // Anomaly 05000428 (Speculative read from L2 by Core B may cause a
   // write to L2 to fail) will not cause a failure as the read is not
   // from L2 memory. Suppress this anomaly warning
   .MESSAGE/SUPPRESS 5508,5517 FOR 1 LINES;
   R1 = [P0];

   BITSET(R1,ENDCPLB_P);              // enable CPLBs 
   CC = BITTST(R0, CPLB_ENABLE_DCACHE_P);
   IF !CC JUMP .nocache;
#if defined(__ADSPBF50x__)
   R3 = (1<<DMC_P);                   // Can only enable A
#else
   R2 = ((1<<DMC1_P) | (1<<DMC0_P));  // Enable both
   R3 = (1<<DMC1_P);                  // Enable just A
   CC = BITTST(R0, CPLB_ENABLE_DCACHE2_P);
   IF CC R3 = R2;
#endif
   R1 = R1 | R3;
.nocache:
#if WA_05000125 || defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
   CLI R3;     // workaround anomaly ID 05-00-0125 & 05-00-0312
   .align 8;
   [P0] = R1;
   .MESSAGE/SUPPRESS 5515 FOR 1 LINES;
   SSYNC;
   STI R3;
#else
   [P0] = R1;
   SSYNC;
#endif
.f_end:
   RTS;
._enable_data_cache.end:

.global _enable_data_cache;
.type _enable_data_cache, STT_FUNC;

// void disable_data_cache()
_disable_data_cache:
   P0.L = (DMEM_CONTROL & 0xFFFF);
   P0.H = (DMEM_CONTROL >> 16);
   R0 = [P0];
#if defined(__ADSPBF50x__)
   R2 = ~((1<<ENDCPLB_P) | (1<<DMC_P)) (X);
#else
   R2 = ~((1<<ENDCPLB_P) | (1<<DMC1_P) | (1<<DMC0_P)) (X);
#endif
   R0 = R0 & R2;
#if WA_05000125 || defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
   CLI R2;     // workaround anomaly ID 05-00-0125 & 05-00-0312
   .align 8;
   [P0] = R0;
   .MESSAGE/SUPPRESS 5515 FOR 1 LINES;
   SSYNC;
   STI R2;
#else
   [P0] = R0;
   SSYNC;
#endif
   RTS;
._disable_data_cache.end:

.global _disable_data_cache;
.type _disable_data_cache, STT_FUNC;


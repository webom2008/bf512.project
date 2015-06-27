/* Copyright (C) 2003-2011 Analog Devices, Inc. All Rights Reserved. */
/*
** Define an exception handler to invoke the CPLB manager,
** if the user doesn't have one already.
*/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libName=libevent;
.file_attr FuncName=_cplb_hdr;
.file_attr libFunc=_cplb_hdr;
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";
#endif

#include <cplb.h>
#include <sys/anomaly_macros_rtl.h>

.section cplb_code;

.global _cplb_hdr;
.type _cplb_hdr, STT_FUNC;
.extern ___cplb_ctrl;
.type ___cplb_ctrl, STT_OBJECT;
.extern _cplb_mgr;
.type _cplb_mgr, STT_FUNC;
.extern __unknown_exception_occurred;
.type __unknown_exception_occurred, STT_FUNC;
.extern __cplb_miss_all_locked;
.type __cplb_miss_all_locked, STT_FUNC;
.extern __cplb_miss_without_replacement;
.type __cplb_miss_without_replacement, STT_FUNC;
.extern __cplb_protection_violation;
.type __cplb_protection_violation, STT_FUNC;
.extern __cplb_double_hit;
.type __cplb_double_hit, STT_FUNC;

#if defined(__WORKAROUND_SSYNC) && WA_05000283
#error Conflicting errata workarounds 05-00-054 and 05-00-0283 
#endif

#define UNKNOWN_EXCAUSE -1

.align 2;
_cplb_hdr:
        /* As this is an exception handler, there is no needs to workaround 
        ** 05-00-0312. Suppress the assembler warning.
        */
        .MESSAGE/SUPPRESS 5515;

#if defined(__WORKAROUND_SSYNC)
        SSYNC;
#endif
        [--SP] = (R7:0, P5:0);
        [--SP] = ASTAT;
#if WA_05000283
        P0.H = 0xFFC0;
        P0.L = 0x0014;
        CC = R0 == R0;
        IF CC JUMP .wa05000283;  // This mispredicted jump is always taken so

#if WA_05000428
        // Speculative read from L2 by Core B may cause a write to L2 to fail
        //
        // Suppress the warning associated with the following
        // instruction because it is not loading from L2 memory

.MESSAGE/SUPPRESS 5517 FOR 1 LINES;
        R0 = [P0];               // this MMR read is killed
#else
        R0 = [P0];               // this MMR read is killed
#endif

.wa05000283:
#elif defined(__WORKAROUND_KILLED_MMR_WRITE) || \
      defined(__WORKAROUND_FLAGS_MMR_ANOM_311)
        P0.H = 0xFFC0;
        P0.L = 0x0014;
        R0 = [P0];               // Dummy MMR read
#endif
        [--SP] = I0;
        [--SP] = I1;
        [--SP] = I2;
        [--SP] = I3;
        [--SP] = LT0;
        [--SP] = LB0;
        [--SP] = LC0;
        [--SP] = LT1;
        [--SP] = LB1;
        [--SP] = LC1;
        [--SP] = L0;
        [--SP] = L1;
        [--SP] = L2;
        [--SP] = L3;
        L0 = 0 (X);
        L1 = 0 (X);
        L2 = 0 (X);
        L3 = 0 (X);

        R2 = SEQSTAT;

        R0 = UNKNOWN_EXCAUSE;  // flag for unknown exception

        // Hold possible _cplb_mgr parameter one values in registers 
        // to be conditionally assigned to R0 for appropriate EXCAUSE
        // values.
        R4 = CPLB_EVT_DCPLB_WRITE;  // 2
        R5 = CPLB_EVT_DCPLB_MISS;   // 1
        R6 = CPLB_EVT_ICPLB_MISS;   // 0
        R7 = CPLB_EVT_ICPLB_DOUBLE_HIT; // 3

        R2 <<= 26;         // extract EXCAUSE
        R2 >>= 26;

        R1 = 0x23;         // 0x23 - DCPLB protection violation/first write
        CC = R2 == R1;
        IF CC R0 = R4;

        R1 += 3;           // 0x26 - DCPLB Miss
        CC = R2 == R1;
        IF CC R0 = R5;

        R1 += 6;           // 0x2C - ICPLB Miss
        CC = R2 == R1;
        IF CC R0 = R6;

        R1 += 1;           // 0x2D - ICPLB Double Hit
        CC = R2 == R1;
        IF CC R0 = R7;

        CC = R0 == UNKNOWN_EXCAUSE;
        IF CC JUMP unknown;
        // R0 now holds the first parameter for a call to _cplb_mgr

        LINK 12;
        P0.L = ___cplb_ctrl;
        P0.H = ___cplb_ctrl;
        R1 = [P0];
        CALL.X _cplb_mgr;

#if WA_05000261
        CC = R0 == CPLB_RELOADED; // Return as normal for both CPLB_NO_ACTION and 
        R1 = CPLB_NO_ACTION;      // CPLB_NO_ACTION and CPLB_RELOADED
        R1 = R0 - R1;
        CC |= AZ;
#else
        CC = R0 == CPLB_RELOADED;
#endif

        IF !CC JUMP not_replaced;

#if WA_05000428
        // Speculative read from L2 by Core B may cause a write to L2 to fail

        NOP;
        NOP;
        NOP;
#endif

restore_and_return:
        UNLINK;
        L3 = [SP++];
        L2 = [SP++];
        L1 = [SP++];
        L0 = [SP++];
        LC1 = [SP++];
        LB1 = [SP++];
        LT1 = [SP++];
        LC0 = [SP++];
        LB0 = [SP++];
        LT0 = [SP++];
        I3 = [SP++];
        I2 = [SP++];
        I1 = [SP++];
        I0 = [SP++];
        ASTAT = [SP++];
        (R7:0, P5:0) = [SP++];
        RTX;

unknown:
        CALL.X __unknown_exception_occurred;
        JUMP unknown;

not_replaced:
        CC = R0 == CPLB_NO_UNLOCKED;
        IF !CC JUMP next_check;
        CALL.X __cplb_miss_all_locked;
        JUMP restore_and_return;
next_check:
        CC = R0 == CPLB_NO_ADDR_MATCH;
        IF !CC JUMP next_check2;
        CALL.X __cplb_miss_without_replacement;
        JUMP restore_and_return;
next_check2:
        CC = R0 == CPLB_PROT_VIOL;
        IF !CC JUMP next_check3;
        CALL.X __cplb_protection_violation;
        JUMP restore_and_return;
next_check3:
        R1 = CPLB_DOUBLE_HIT (Z);
        CC = R0 == R1;
        IF !CC JUMP strange_return_from_cplb_mgr;
        CALL.X __cplb_double_hit;
        JUMP restore_and_return;

strange_return_from_cplb_mgr:
        IDLE;
        CSYNC;
        JUMP strange_return_from_cplb_mgr;
._cplb_hdr.end:


/* Copyright (C) 2003-2010 Analog Devices, Inc. All Rights Reserved. */

//***************************************************************************/
// void cplb_init(int enable_cplbs_caches)
// enable_cplbs_caches is a mask requesting one or more of
//  CPLB_ENABLE_CPLBS
//  CPLB_ENABLE_ICACHE
//  CPLB_ENABLE_DCACHE
//  CPLB_ENABLE_DCACHE2
//***************************************************************************/
// This library function is used optionally to set up CPLBs from a table
// for access control, or to configure one or more L1 SRAM banks as cache.
//
// In the latter case CPLBs are mandatory. If only one data bank is to
// be configured as cache, it must be bank A.
//
// In the former case, there may not be any L2 or L3 memory, (as there
// would have to be in the cacheing case); ie both this code and its data
// may all be in L1. Bearing in mind that L1 instruction memory cannot
// be accessed by the DAGs, and data memory cannot be accessed by the
// fetch unit, the code and data must be separable via the LDF, so have
// separate section names.
//
// This can be used for BF532/533/535.

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libName=libevent;
.file_attr FuncName=_cplb_init;
.file_attr libFunc=_cplb_init;
.file_attr libFunc=cplb_init;
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";
#endif

#include <cplb.h>
#include <sys/anomaly_macros_rtl.h>
#include <sys/platform.h>
#include <sys/fatal_error_code.h>

.section cplb_code;
// If this a call involving cacheing, this section must not be
// the L1 SRAM block to be reconfigured.

.global _cplb_init;
.type _cplb_init, STT_FUNC;
.extern _cache_invalidate;
.extern _icplbs_table;
.type _icplbs_table, STT_OBJECT;
.extern _dcplbs_table;
.type _dcplbs_table, STT_OBJECT;
.extern ___l1_code_cache;
.type ___l1_code_cache, STT_OBJECT;
.extern ___l1_data_cache_a;
.type ___l1_data_cache_a, STT_OBJECT;
.extern ___l1_data_cache_b;
.type ___l1_data_cache_b, STT_OBJECT;

.extern cplb_address_is_misaligned_for_cplb_size;
.type cplb_address_is_misaligned_for_cplb_size, STT_FUNC;
.extern l1_code_cache_enabled_when_l1_used_for_code;
.type l1_code_cache_enabled_when_l1_used_for_code, STT_FUNC;
.extern l1_data_a_cache_enabled_when_used_for_data;
.type l1_data_a_cache_enabled_when_used_for_data, STT_FUNC;
.extern l1_data_b_cache_enabled_when_used_for_data;
.type l1_data_b_cache_enabled_when_used_for_data, STT_FUNC;
.extern too_many_locked_data_cplbs;
.type too_many_locked_data_cplbs, STT_FUNC;
.extern too_many_locked_instruction_cplbs;
.type too_many_locked_instruction_cplbs, STT_FUNC;

.align 2;
_cplb_init:
        /* As this function is called from the CRT before interrupts are enabled
        ** there is no needs to workaround 05-00-0312. 
        ** Suppress the assembler warning.
        */
        .MESSAGE/SUPPRESS 5515;

        // Begin with sanity checks. If the L1 caches are requested,
        // check whether the LDFs have mapped any code/data into the
        // respective SRAM sections first.

        CC = BITTST(R0, CPLB_ENABLE_ICACHE_P);
        IF !CC JUMP skip_icache_checks;
        R2 = ___l1_code_cache (Z);      // Set to 0 or 1, by LDF
        CC = R2 == 0;                   // Cache enabled, but SRAM used
        IF CC JUMP .jump_l1_code_used;


skip_icache_checks:
        // Repeat for Dcache bits
        CC = BITTST(R0, CPLB_ENABLE_DCACHE_P);
        IF !CC JUMP skip_dcache_checks;
        R2 = ___l1_data_cache_a (Z);    // Set to 0 or 1, by LDF
        CC = R2 == 0;                   // Cache enabled, but SRAM used
        IF CC JUMP .jump_l1_data_a_used;

        CC = BITTST(R0, CPLB_ENABLE_DCACHE2_P);
        R2 = ___l1_data_cache_b (Z);    // Set to 0 or 1, by LDF
        R2 = ROT R2 BY 1;               // Now b00, b10, b01 or b11
        CC = R2 == 1;                   // Cache enabled, but SRAM used
        IF CC JUMP .jump_l1_data_b_used;

skip_dcache_checks:
        // We only install any CPLBs if ICPLBS or DCPLBs are enabled,
        // but the (deprecated) CPLB_ENABLE_CPLBS flag indicates that
        // both are to be enabled. So, if that flag is set, force the
        // flags for both ICPLBs and DCPLBs to be set, too.

        R1 = CPLB_ENABLE_ICPLBS | CPLB_ENABLE_DCPLBS;
        R2 = 0;
        CC = BITTST(R0, CPLB_ENABLE_CPLBS_P);
        IF CC R2 = R1;
        R0 = R0 | R2;

        // And now, check whether either of ICPLBs or DCPLBs are
        // to be enabled.
        R1 = R0 & R1;
        CC = R1;
        IF !CC JUMP not_enabled;
        [--SP] = (R7:4,P5:3);
        R7 = R0;
        P4 = R0;      // save input bitmask  

#if !defined(__ADSPLPBLACKFIN__)
        // NB There is no reason not to do this for 532/533, although
        // it is not necessary; only those SRAM blocks requested as
        // cache by the user are invalidated, and we can assume that
        // external memory is present, and that the SRAM about to be
        // reconfigured contains nothing that matters.
        //
        // BF535 cache valid bits come up in a random state,
        // so if caching has been requested, mark all of
        // them as invalid.
        R6 = (CPLB_ENABLE_DCACHE|CPLB_ENABLE_ICACHE);
        // ie DcacheA or Icache. We do not need to check for DcacheB
        // because if only one data bank is to be cache it must be bank A;
        // ie DcacheB cannot appear on its own.
        R6 = R7 & R6;
        CC = R6;
        IF !CC JUMP no_cache_inv;
        LINK 12;
        CALL.X _cache_invalidate;
        UNLINK;
no_cache_inv:
#endif

        // Set up initial CPLBs
        // Assign the first 16 to each.

        CC = BITTST(R7, CPLB_ENABLE_DCPLBS_P);   // Skip DCPLBs if not enabled.
        IF !CC JUMP .ddone;

        // Check that addresses in dcplbs_table are aligned correctly
        I0.L = _dcplbs_table;
        I0.H = _dcplbs_table;

        // Load an extraction pattern, to get the page-size bits from
        // the CPLB data entry.
        // Bits 16-17, so two bits at posn 16.
        R3 = ((16 << 8) | 2);
        LINK 0;
        CALL check_addresses_in_tables;
        UNLINK;

        //  DATA CPLBs
        I0.L = DCPLB_ADDR0 & 0xFFFF;
        I0.H = DCPLB_ADDR0 >> 16;

        I1.L = DCPLB_DATA0 & 0xFFFF;
        I1.H = DCPLB_DATA0 >> 16;

        // Begin by marking all DCPLBs as invalid.
        I3 = I1;
        P0 = __CPLB_TABLE_SIZE;   // intialise P0 used for looping over CPLB MMRs
        R1 = 0;
        LSETUP (.dinv, .dinv) LC0 = P0;
.dinv: [I3++] = R1;


        // The CPLB tables may have caching bits set, so that
        // caching functions if enabled. However, if the user
        // has not selected caching in __cplb_ctrl, then we have
        // to mask off the cache-in-L1 bit. Moreover, if caching
        // is disabled we want to ensure that the CPLB is set to
        // WT rather than WB, because the BF535, at
        // least, invoke first-write exceptions on WB pages, even
        // when cache-in-L1 is not set and cache is disabled.

        CC = BITTST(R7, CPLB_ENABLE_DCACHE_P);
        R5 = CPLB_L1_CHBL (Z);
        R6 = CPLB_WT (Z);
        R0 = 0;
        IF CC R5 = R0;            // if not enabling Dcache, clear flags
        IF CC R6 = R0;
        R5 = ~R5;                 // create mask of allowed bits
#if WA_05000204
#  if defined(WA_05000204_CHECK_AVOID_FOR_REV)
#    ifndef REVID
#      define REVID 0xffc00014
#    endif
        P5.L = REVID & 0xFFFF;
        P5.H = REVID >> 16;
        R0 = [P5];
        R0 >>= 28;
        CC = R0 WA_05000204_CHECK_AVOID_FOR_REV;
        R0 = CC;
        P2 = R0;
#  endif
#endif

        // read entries from table, searching
        // for the locked entries on the first pass
        // and then unlocked entries on the 2nd pass
        // until we either run out of cplbs or slots.
        P3.L = LO(___cplb_info);    // record of what we installed
        P3.H = HI(___cplb_info);
        R3 = CPLB_LOCK;             // lock mask
        R1 = R3;
        R4 = __CPLB_TABLE_SIZE;     // available slots
        P0 = 2;                     // number of outer loops
        P5 = -1;                    // number of inner loops
        LSETUP (.rd1, .rd1_end) LC0 = P0;
.rd1:   I2.L = _dcplbs_table;
        I2.H = _dcplbs_table;
        R0 = [I2++];                // read initial address from table
        LSETUP (.rd2, .rd2_end) LC1 = P5;
.rd2:      CC = R0 == -1;           // end point comparison 
        IF CC JUMP .rdlast;         // if end seen jump out 

#if WA_05000428
        // Speculative read from L2 by Core B may
        // cause a write to L2 to fail

        NOP;
        NOP;
        NOP;
#endif
        R2 = [I2++];            // read configuration bits 
        R7 = R2 & R3;           // check if cplb is locked state we require
        CC = R7 == R1;
        IF !CC JUMP .rd2_end;   // look for next if no match
        CC = R4 == 0;           // exit if we are out of slots
        IF CC JUMP .chk_d_ovflw;
        R4 += -1;               // decrement available slots
        R2 = R2 & R5;           // mask off cache-in-L1 and include 
        R2 = R2 | R6;           // WT if caching is disabled 
        [I0++] = R0;            // write address to DCPLB_ADDRx 
#if WA_05000158
        // workaround anomaly 05-00-0158 - set bit 9 in DCPLB_DATA
        // for all revisions of BF531/532/533
        CC = BITTST(R2, CPLB_L1_CHBL_P);
        R0 = R2;
        BITSET(R0,9);
        IF CC R2 = R0;
#endif
#if WA_05000204
        // workaround anomaly 05-00-0204 - by setting CPLB_L1_AOW=1
        // (allocate cache lines on read and writes).
#if defined(WA_05000204_CHECK_AVOID_FOR_REV)
        CC = P2 == 0;
        if CC JUMP .wa_204_done;
#endif
        CC = BITTST(R2, CPLB_VALID_P);
        IF !CC JUMP .wa_204_done;
        CC = BITTST(R2, CPLB_L1_CHBL_P);
        IF !CC JUMP .wa_204_done;
        CC = BITTST(R2, CPLB_WT_P);
        IF !CC JUMP .wa_204_done;
        // write through cache enabled so set CPLB_L1_AOW
        BITSET(R2,CPLB_L1_AOW_P); 
#if WA_05000428
        // Speculative read from L2 by Core B may
        // cause a write to L2 to fail
        NOP;
#endif
.wa_204_done:
#endif 
        [I1++] = R2;            // write address to DCPLB_DATAx
.rd2_end: R0 = [I2++];          // read next address from table
.rdlast:CC = R1;                // Just finished the locked pass?
        IF !CC JUMP .skip_update;
        R2 = I0;                // Yes, so note the first unlocked DCPLB
        [P3+0] = R2;            // and store it for the victimization search
        [P3+4] = R2;            // as "first unlocked" and "next to replace"
.skip_update:
        NOP;                    // Don't make hwloop end the target of a jump
.rd1_end:  R1 = 0 (Z);          // change flag to look for unlocked cplbs
        JUMP .ddone;            // skip overflow check if completed normally

.chk_d_ovflw:
        I1 -= 0x4;              // if we have filled table with locked
        R0 = [I1];              //  cplbs and have more cplbs available
        R0 = R0 & R3;           //  then report issue to user.
        CC = R0 == R3;
        IF CC JUMP .jump_too_many_data_cplb;

.ddone:
        R7 = P4;

        //  INSTRUCTION CPLBs

        CC = BITTST(R7, CPLB_ENABLE_ICPLBS_P);  // skip if not enabled
        IF !CC JUMP .dmem_control;

        // Check that addresses in icplbs_table are aligned correctly
        I0.L = _icplbs_table;
        I0.H = _icplbs_table;
        // Load an extraction pattern, to get the page-size bits from
        // the CPLB data entry.
        // Bits 16-17, so two bits at posn 16.
        R3 = ((16 << 8) | 2);
        LINK 0;
        CALL check_addresses_in_tables;
        UNLINK;

        I0.L = ICPLB_ADDR0 & 0xFFFF;
        I0.H = ICPLB_ADDR0 >> 16;

        I1.L = ICPLB_DATA0 & 0xFFFF;
        I1.H = ICPLB_DATA0 >> 16;

        // Begin by making all ICPLBs invalid.
        I3 = I1;
        P0 = __CPLB_TABLE_SIZE;
        R1 = 0;
        LSETUP (.iinv, .iinv) LC0 = P0;
.iinv:  [I3++] = R1;


        CC = BITTST(R7, CPLB_ENABLE_ICACHE_P);
        R4 = 0;
        R6 = CPLB_L1_CHBL (Z);
        IF CC R6 = R4;              // if not enabling Icache, clear flag 
        R6 = ~R6;                   // create mask of allowed bits

        // read entries from table, searching
        // for the locked entries on the first pass
        // and then unlocked entries on the 2nd pass
        // until we either run out of cplbs or slots.
        P3.L = LO(___cplb_info);    // record of what we installed
        P3.H = HI(___cplb_info);
        R3 = CPLB_LOCK;             // lock mask
        R1 = R3;                    // look-for-lock flag 
        R4 = __CPLB_TABLE_SIZE;     // available slots
        P0 = 2;                     // outer loop counter
        P5 = -1;                    // inner loop counter (infinite)
        LSETUP (.ri1, .ri1_end) LC0 = P0;
.ri1:   I2.L = _icplbs_table;
        I2.H = _icplbs_table;
        R0 = [I2++];                // read initial address from table
        LSETUP (.ri2, .ri2_end) LC1 = P5;
.ri2:   CC = R0 == -1;              // end point comparison 
        IF CC JUMP .rilast;         // if end seen start next iteration

#if WA_05000428
        // Speculative read from L2 by Core B may
        // cause a write to L2 to fail

        NOP;
        NOP;
        NOP;
#endif

        R2 = [I2++];// read configuration bits
        R7 = R2 & R3;           // check if cplb is locked
.ilocked:  CC = R7 == R1;
        IF !CC JUMP .ri2_end;   // look for next if no match

        CC = R4 == 0;           // exit if we are out of slots
        IF CC JUMP .chk_i_ovflw;
        R4 += -1;               // decrement available slots
        R2 = R2 & R6;           // mask off cache bits if disabled
        [I0++] = R0;            // write address to ICPLB_ADDRx
#if WA_05000258
        R0 = R2;                // Workaround anomaly 05-00-0258
        BITSET(R0, 9);          // by setting bit 9 of ICPLB_DATAx
        CC = BITTST(R2, CPLB_L1_CHBL_P);
        IF CC R2 = R0;
#endif
        [I1++] = R2;            // write data to ICPLB_DATAx
.ri2_end:  R0 = [I2++];         // read next address from table
.rilast:   CC = R1;             // Have we just finished locked pass?
        IF !CC JUMP .skip_iupdate;
        R2 = I0;                // Note the first unlocked ICPLB
        [P3+12] = R2;           // and save it for the victimization stage
        [P3+16] = R2;           // as "first unlocked" and "next to replace"
.skip_iupdate:
        NOP;                    // Don't make jump target end of hwloop.
.ri1_end:  R1 = 0;                 // change flag to look for unlocked cplbs
        JUMP .idone;            // skip overflow check if completed normally 

.chk_i_ovflw:
        I1 -= 0x4;              // if we have filled table with locked
        R7 = [I1];              //  cplbs and have more cplbs available
        R7 = R7 & R3;           //  then report issue to user.
        CC = R7 == R3;
        IF CC JUMP .jump_too_many_instr_cplb;
.idone:

        // Enable Cache and CPLBs as requested

        P0.L = IMEM_CONTROL & 0xFFFF;
        P0.H = IMEM_CONTROL >> 16;

#if defined(__ADSPLPBLACKFIN__)
        R0 = (ENICPLB);                    // without icache
        R1 = (IMC | ENICPLB);              // with icache
#else
        R0 = (ENICPLB | ENIM);             // without icache
        R1 = (IMC | ENICPLB | ENIM);       // with icache
#endif
        R7 = P4;
        CC = BITTST(R7,CPLB_ENABLE_ICACHE_P);
        IF CC R0 = R1;

#if WA_05000125
        CLI R1;                            // workaround anomaly ID 05000125
        .align 8;
        [P0] = R0;
        SSYNC;
        STI R1;
#else
        [P0] = R0;
        SSYNC;
#endif

.dmem_control:
        CC = BITTST(R7, CPLB_ENABLE_DCPLBS_P);
        IF !CC JUMP .finished;

        P0.L = DMEM_CONTROL & 0xFFFF;
        P0.H = DMEM_CONTROL >> 16;

        R0 = (ENDCPLB | ENDM);
        CC = BITTST(R7, CPLB_SET_DCBS_P);
        R1 = (ENDCPLB | ENDM | DCBS);
        IF CC R0 = R1;
        R1 = (ASRAM_BSRAM);      // without dcache
        CC = BITTST(R7, CPLB_ENABLE_DCACHE_P);
        R2 = (ACACHE_BSRAM);     // with dcache A
        IF CC R1 = R2;
        CC = BITTST(R7, CPLB_ENABLE_DCACHE2_P);
        R2 = (ACACHE_BCACHE);    // with dcache A and B
        IF CC R1 = R2;
        R0 = R0 | R1;

#if defined(__ADSPLPBLACKFIN__)
        // Set the port preferences of DAG0 and DAG1 to be
        // different; this gives better performance when
        // performing dual-dag operations on SDRAM. These are
        // set on reset so ensure that we don't clobber them.
        BITSET(R0, 12);
        BITCLR(R0, 13);
#endif

#if WA_05000125
        CLI R1;                   // workaround anomaly ID 05000125
        .align 8;
        [P0] = R0;
        SSYNC;
        STI R1;
#else
        [P0] = R0;
        SSYNC;
#endif

.finished:
        (R7:4,P5:3) = [SP++];
not_enabled:
        RTS;


check_addresses_in_tables:
        // check table in I0 for misaligned addresses 
        // use the extraction pattern in R3 to extract page sizes.

        P0.L = ___page_size_mask_table;
        P0.H = ___page_size_mask_table;

        P5 = -1;                   // should be enough
        LSETUP (.checks, .checke) LC0 = P5;
.checks: R1 = [I0++];              // read address
        R2 = [I0++];               // read configuration bits 
        CC = R1 == -1;             // end point comparison 
        IF CC JUMP .cdone;         // if end seen jump out
        R2 = EXTRACT(R2,R3.L) (Z); // Get page size
        P5 = R2;
        P5 = P0 + (P5<<2);   // index into page-prefix table
        R2 = [P5];           // Get the address mask
        R1 = R1 & R2;        // and address to check alignment
        CC = R1 == 0;
#if defined(__ADSPLPBLACKFIN__)
.checke: IF !CC JUMP .jump_cplb_misaligned;
#else
        // jump not allowed as last instr of a HW loop
        IF !CC JUMP .jump_cplb_misaligned;
.checke: NOP;
#endif

.cdone:
        RTS;
.check_addresses_in_tables.end:

.jump_cplb_misaligned:
       JUMP.X cplb_address_is_misaligned_for_cplb_size;

.jump_l1_code_used:
       JUMP.X l1_code_cache_enabled_when_l1_used_for_code;

.jump_l1_data_a_used:
       JUMP.X l1_data_a_cache_enabled_when_used_for_data;

.jump_l1_data_b_used:
       JUMP.X l1_data_b_cache_enabled_when_used_for_data;

.jump_too_many_data_cplb:
       JUMP.X too_many_locked_data_cplbs;

.jump_too_many_instr_cplb:
       JUMP.X too_many_locked_instruction_cplbs;

._cplb_init.end:

.section cplb_data;

.align 4;
___page_size_mask_table:
.byte4 =
        0x000003ff,   /* 1K */
        0x00000fff,   /* 4K */
        0x000fffff,   /* 1M */
        0x003fffff    /* 4M */
        ;
.___page_size_mask_table.end:

.align 4;
___cplb_info:
.byte4 =
        0xffffffff,     /* First non-locked DCPLB */
        0xffffffff,     /* Next DCPLB to evict (-1 means all locked) */
        DCPLB_ADDR15+4, /* When to wrap back to first */
        0xffffffff,     /* First non-locked ICPLB */
        0xffffffff,     /* Next ICPLB to evict (-1 means all locked) */
        ICPLB_ADDR15+4  /* When to wrap back to first */
        ;
.___cplb_info.end:
.global ___cplb_info;
.type ___cplb_info, STT_OBJECT;

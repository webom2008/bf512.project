/* Copyright (C) 2003-2010 Analog Devices, Inc. All Rights Reserved. */

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libName=libevent;
.file_attr FuncName=_cplb_mgr;
.file_attr libFunc=_cplb_mgr;
.file_attr libFunc=cplb_mgr;
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";
#endif

#include <cplb.h>
#include <sys/anomaly_macros_rtl.h>  // include WA_ macro definitions
#if !defined(ENICPLB_P)
   // ENICPLB_P not defined for BF535
#  define ENICPLB_P 1
#endif

#if defined(__ADSPBF535__) 
#  define LAST_LOOP_INSTR  NOP;
#else
#  define LAST_LOOP_INSTR
#endif

#if WA_05000125
#  define WRITE_MEM_CONTROL(_ptr,_val,_tmp) \
        CLI _tmp;                           \
        .align 8;                           \
        [_ptr] = _val;                      \
        SSYNC;                              \
        STI _tmp
#else
#  define WRITE_MEM_CONTROL(_ptr,_val,_tmp) \
        [_ptr] = _val;                      \
        SSYNC
#endif


.section cplb_code; // L2_Bank0_Page0;
.global _cplb_mgr;
.type _cplb_mgr, STT_FUNC;
.extern _icplbs_table;
.type _icplbs_table, STT_OBJECT;
.extern _dcplbs_table;
.type _dcplbs_table, STT_OBJECT;
.extern ___cplb_info;
.type ___cplb_info, STT_OBJECT;

// Usage: int cplb_mgr(int do_what, int enable_cache)
// do_what==CPLB_EVT_DCPLB_WRITE => write to clean data page (mark as dirty)
// do_what==CPLB_EVT_DCPLB_MISS => replace a data cplb
// do_what==CPLB_EVT_ICPLB_MISS => replace an instr cplb
// do_what==CPLB_EVT_ICPLB_DOUBLE_HIT => replace an instr cplb for the next page
// enable_cache is the same mask passed to cplb_init()
//
// Returns:
// CPLB_RELOADED => everything okay
// CPLB_NO_UNLOCKED => No non-locked entry found
// CPLB_NO_ADDR_MATCH => No CPLB in the table to match the address missed.
// CPLB_PROT_VIOL => Attempt to write a read-only resource, or access
//                   a supervisor resource in user mode.
// CPLB_NO_ACTION => Working around 05-00-0261: ignore first attempt to raise
//                   exception, to let MMRs settle. Process on second attempt.
// CPLB_DOUBLE_HIT => CPLB tables include two pages that overlap, and one has
//                    just been accessed.

.align 2;
_cplb_mgr:
        /* As this is intended to be called from an exception handler, there 
        ** is no needs to workaround 05-00-0312. Suppress the assembler warning.
        */
        .MESSAGE/SUPPRESS 5515;

        [--SP]=(R7:4,P5:3);  // save call preserved D and P regs

        CC = R0 == CPLB_EVT_DCPLB_WRITE;
        IF CC JUMP dcplb_write;

        // Disable DCPLBs here before accessing any data defined in this
        // module (section cplb_data). Should there not be CPLB covering this
        // data then a fatal double exception would occur.
        CC = BITTST(R1,CPLB_ENABLE_DCPLBS_P);
        IF !CC JUMP nextcheck;

        P4.L = LO(DMEM_CONTROL);
        P4.H = HI(DMEM_CONTROL);

        // A speculative load from DMEM_CONTROL will not cause problems
        // so there is no need to worry about anomaly 05000245. Suppress the 
        // assembler warning for this anomaly here.
        //
        // Anomaly 05000428 (Speculative read from L2 by Core B may cause a
        // write to L2 to fail) will not cause a failure as the read is not
        // from L2 memory. Also suppress this anomaly warning
        .MESSAGE/SUPPRESS 5508,5517 FOR 1 LINES;
        R5 = [P4];
        BITCLR(R5,ENDCPLB_P);

        WRITE_MEM_CONTROL(P4, R5, R2);

nextcheck:
        CC = R0 == CPLB_EVT_DCPLB_MISS;
        IF CC JUMP dcplb_miss_compare;


        CC = R0 == CPLB_EVT_ICPLB_DOUBLE_HIT;
        IF CC JUMP .icplb_double_hit;

////////////////////////////////////////////////////////////////////////
        // ICPLB Miss Exception.
        // We need to choose one of the currently-installed CPLBs,
        // and replace it with one from the configuration table.

        P4.L = LO(ICPLB_FAULT_ADDR);
        P4.H = HI(ICPLB_FAULT_ADDR);

        // A speculative load from DMEM_CONTROL will not cause problems
        // so there is no need to wory about anomaly 05000245. Suppress the 
        // assembler warning for this anomaly here.
        //
        // Anomaly 05000428 (Speculative read from L2 by Core B may cause a
        // write to L2 to fail) will not cause a failure as the read is not
        // from L2 memory. Also suppress this anomaly warning
        .MESSAGE/SUPPRESS 5508,5517 FOR 1 LINES;
        R4 = [P4];

.back_from_icplb_double_hit:
        R7 = R1; // "bool enable_cache" - save for later

        I0 = R4; // Fault address we'll search for
                 // Note: this may be different from the
                 // address retrieved from ICPLB_FAULT_ADDR,
                 // because we may have advanced to the next
                 // page.

        /* set up pointers */
        P0.L = LO(ICPLB_DATA0);
        P0.H = HI(ICPLB_DATA0);

        CC = BITTST(R7, CPLB_ENABLE_ICACHE_P);

        // The replacement procedure for ICPLBs

        P4.L = LO(IMEM_CONTROL);
        P4.H = HI(IMEM_CONTROL);

        R6 = CPLB_CACHE_ENABLED;
        R2 = 0;
        IF CC R6 = R2;
        R6 = ~R6;                       // cache bits to mask off 

        // Turn off CPLBs while we work, necessary according to HRM before 
        // modifying CPLB descriptors

        R5 = [P4];                      // Control Register 
        BITCLR(R5,ENICPLB_P);

        WRITE_MEM_CONTROL(P4,R5,R1);

        P2.L = LO(_icplbs_table);       // for later use, here to avoid stalls
        P2.H = HI(_icplbs_table);
        P1.L = LO(___page_size_table);
        P1.H = HI(___page_size_table);

        // Determine which CPLB we need to install, from the configuration
        // table. This is a matter of getting the start-of-page
        // addresses and page-lengths from the config table, and
        // determining whether the fault address falls within that
        // range.

        // Retrieve our fault address.
        R0 = I0;

        // Load an extraction pattern, to get the page-size bits from
        // the CPLB data entry. Bits 16-17, so two bits at posn 16.
        R3 = ((16 << 8) | 2);

.inext:
        R4 = [P2++];                    // address from config table
        R2 = [P2++];                    // data from config table 

        CC = R4 == -1;// End of config table
        IF CC JUMP no_page_in_table;

        /* See if failed address > start address */
        CC = R4 <= R0 (IU);
        IF !CC JUMP .inext;

        /* extract page size encoding (17:16)*/
        R1 = EXTRACT(R2, R3.L) (Z);

        /* map page size encoding into actual size via ___page_size_table[enc] */

        P5 = R1;
        P5 = P1 + (P5 << 2);            // scaled, for int access
        R1 = [P5];
        R1 = R1 + R4;

        /* See if failed address < (start address + page size) */
        CC = R0 < R1 (IU);
        IF !CC JUMP .inext;

        // We've found a CPLB in the config table that covers
        // the faulting address: R4==addr, R2==data.
        // Install this CPLB into the
        // next-to-be-replaced slot in the table.

        P1.L = LO(___cplb_info);
        P1.H = HI(___cplb_info);
#if WA_05000428
        NOP;                   // Don't issue a read in shadow of branch
#endif
        R1 = [P1+16];          // Next ICPLB to replace
        R3 = [P1+20];          // Past end of ICPLB table
        CC = R1 == -1;
        P5 = [P1+12];          // First ICPLB to replace
        IF CC JUMP all_locked;
        CC = R1 == R3;
        R3 = P5;               // First ICPLB to replace
        IF CC R1 = R3;
        P5 = R1;

        // R6 contains a bit mask, to remove any caching bits, if
        // cache is not enabled.
        R2 = R2 & R6;

#if WA_05000258
        R1 = R2;                // Workaround anomaly 05-00-0258
         BITSET(R1, 9);         // by setting bit 9 of ICPLB_DATAx
        CC = BITTST(R2, 12);    // to the state of CPLB_L1_CHBL.
        IF CC R2 = R1; 
#endif
        [P5+0x100] = R2;        // write new ICPLB_DATA, evicting old.
        [P5++] = R4;            // Write new ICPLB_ADDR, evicting old.
        [P1+16] = P5;           // Record the next one we'll evict.

        // P4 points to IMEM_CONTROL, and R5 contains its old
        // value, after we disabled ICPLBS. Re-enable them.

        BITSET(R5,ENICPLB_P);
        WRITE_MEM_CONTROL(P4,R5,R2);

        // We disabled DCPLBs at the beginning of the function, so that
        // we could read the data tables without a DCPLB-miss.
        // If they were enabled to start with, re-enable them here.
        // R7 contains the value of __cplb_ctrl passed in to the function
        CC = BITTST(R7, CPLB_ENABLE_DCPLBS_P);
        IF !CC JUMP ret;

        P4.L = LO(DMEM_CONTROL);
        P4.H = HI(DMEM_CONTROL);

        // A speculative load from DMEM_CONTROL will not cause problems
        // so there is no need to wory about anomaly 05000245. Suppress the 
        // assembler warning for this anomaly here.
        //
        // Anomaly 05000428 (Speculative read from L2 by Core B may cause a
        // write to L2 to fail) will not cause a failure as the read is not
        // from L2 memory. Also suppress this anomaly warning
        .MESSAGE/SUPPRESS 5508,5517 FOR 1 LINES;
        R5 = [P4];
        BITSET(R5,ENDCPLB_P);
        WRITE_MEM_CONTROL(P4,R5,R2);

ret:
        (R7:4,P5:3) = [SP++];   // restore call preserved registers
        R0 = CPLB_RELOADED;
        RTS;

////////////////////////////////////////////////////////////////////////
dcplb_miss_compare:

        // Data CPLB Miss event. We need to choose a CPLB to
        // evict, and then locate a new CPLB to install from the
        // config table, that covers the faulting address.

#if WA_05000261
        // We work around 05-00-0261 "DCPLB_FAULT_ADDR MMR may be
        // corrupted" by initially ignoring a DCPLB miss exception on
        // the assumption that the faulting address might be invalid.
        // We return without servicing. The exception will be raised
        // again when the faulting instruction is re-executed. The fault
        // address is correct this time round so the miss exception can
        // be serviced as normal. The only complication is we have to
        // ensure that we are about to service the same miss rather than
        // a miss raised within a higher-priority interrupt handler, where
        // the fault address could again be invalid. We therefore record
        // the last seen RETX and only service an exception when RETX and
        // the last seen RETX are equal.

        P1.L = LO(.last_RETX);
        P1.H = HI(.last_RETX);
        R4 = [P1];
        R7 = RETX;
        [P1] = R7;
        CC = R4 == R7;
        IF CC JUMP .service;
        R0 = CPLB_NO_ACTION;    // here to avoid a speculative SSYNC below
                                // and thus working around anomaly 05000244

        // We disabled DCPLBs at the beginning of the function.
        // Re-enable them here. P4 contains address of DMEM_CONTROL
        // and R5 is the value we last wrote to it.
        BITSET(R5,ENDCPLB_P);
        WRITE_MEM_CONTROL(P4,R5,R2);

        (R7:4,P5:3) = [SP++];   // restore call preserved registers
                                // R0 is live here, assigned above.
        RTS;
  
.service:
#endif
        P4.L = LO(DCPLB_FAULT_ADDR);
        P4.H = HI(DCPLB_FAULT_ADDR);
        R4 = [P4];
        I0 = R4;

        // The replacement procedure for DCPLBs

        R6 = R1;                // Save "enable_cache" parameter for later

        // It is necessary to turn off CPLBs according to HRM before 
        // modifying CPLB descriptors. We did this at the beginning of 
        // the function.



#if defined(__ADSPBF535__) 
       .extern __dcplb_flush;
        // Examine the entry we'll evict. If it's a valid dirty write-back
        // CPLB, we need to flush the pending writes first. That's because
        // FLUSH instructions don't have any effect on BF535 if there's no
        // corresponding DCPLB, meaning that it wouldn't be possible to
        // manually flush the data once we've evicted the DCPLB.

        P4.L = LO(___cplb_info);
        P4.H = HI(___cplb_info);
        R2 = [P4+4];                    // Next to evict
        CC = R2 == -1;
        IF CC JUMP all_locked;
        R1 = [P4+8];                    // past end of table
        CC = R2 == R1;                  // Wrap if we need to
        R1 = [P4+0];                    // back to the address of the first non-locked
        IF CC R2 = R1;
        P0 = R2;                        // Address of DCPLB_ADDR we'll bounce
        R1 = [P0+0x100];                // Fetch corresponding DCPLB_DATA
        
        CC = BITTST(R1, CPLB_VALID_P);  // Is it valid?
        IF !CC JUMP .doverwrite;        // nope. So we can clobber it.
        CC = BITTST(R1, CPLB_DIRTY_P);  // Is it dirty?
        IF !CC JUMP .doverwrite (BP);   // Nope. So we can evict it without flushing.
        CC = BITTST(R1, CPLB_WT_P);     // Is it Write-Through?
        IF CC JUMP .doverwrite;         // Yep, so there'll be nothing to flush, so we can evict.

        // This is a dirty page, so we need to flush all writes
        // that are pending on the page.

        // Extract page size from the CPLB (bits 17:16), and scale
        // to give an offset into the page_prefix tables.
        P4.L = LO(___page_prefix_table);
        P4.H = HI(___page_prefix_table);
        R2 = R1 << 14;                  // Get rid of everything apart from
        R2 >>= 30;                      // bits 17:16, and use as index into
        R2 <<= 2;                       // ___page_prefix_table[].
        P2 = R2;
        P1 = P2 + P4;
        R2 = [P1];                      // This is the address prefix mask

        // Retrieve the page start address
        R0 = [P0];
        LINK 12;
        // __dcplb_flush does not clobber any registers,
        // apart from RETS, ASTAT and (depending on linker) P1.
        // R0==CPLB addr, R1==CPLB data, R2==page prefix
        CALL.X __dcplb_flush;
        UNLINK;
#endif

.doverwrite:

        // The next stage is to locate a CPLB in the
        // config table that covers the faulting address.

        R0 = I0;                        // Our faulting address

        P2.L = LO(_dcplbs_table);
        P2.H = HI(_dcplbs_table);

        P1.L = LO(___page_size_table);
        P1.H = HI(___page_size_table);

        // Load an extraction pattern, to get the page-size bits from
        // the CPLB data entry.
        // Bits 16-17, so two bits at posn 16.
        R3 = ((16 << 8) | 2);

.dnext:
        R4 = [P2++];                    // load address from config table
        R2 = [P2++];                    // load data from config table

        CC = R4 == -1;
        IF CC JUMP no_page_in_table;

        // See if failed address > start address
        CC = R4 <= R0 (IU);
        IF !CC JUMP .dnext;

        // extract page size encoding (17:16)
        R1 = EXTRACT(R2, R3.L) (Z);

        // map page size encoding into page size via __page_size_table[enc],
        // to addr to get range

        P5 = R1;
        P5 = P1 + (P5 << 2);
        R1 = [P5];
        R1 = R1 + R4;

        /* See if failed address < (start address + page size) */
        CC = R0 < R1 (IU);
        IF !CC JUMP .dnext;

        // We've found the CPLB that should be installed (R4, R2), so
        // write it into the DCPLB we're replacing, masking off any caching bits
        // if necessary.

        P1.L = LO(___cplb_info);
        P1.H = HI(___cplb_info);
#if WA_05000428
        NOP;                            // Ensure read isn't in shadow of branch
#endif
        R1 = [P1+4];                    // Next to evict
        CC = R1 == -1;
        R0 = [P1+8];                    // past end of table
        P1 = [P1+0];                    // address of the first non-locked
        IF CC JUMP all_locked;
        CC = R1 == R0;                  // If next-to-evict is past-end-of-table, wrap
        R0 = P1;                        // back to the address of the first non-locked
        IF CC R1 = R0;
        P1 = R1;

        // At this point:
        // P1 is the DCPLB_ADDR we want to evict.
        // R4 is the DCPLB_ADDR content we'll replace it with
        // R2 is the corresponding DCPLB_DATA value.

        // If the DCPLB has cache bits set, but caching hasn't
        // been enabled, then we want to mask off the cache-in-L1
        // bit before installing. Moreover, if caching is off, we
        // also want to ensure that the DCPLB has WT mode set, rather
        // than WB, since WB pages still trigger first-write exceptions
        // even when not caching is off, and the page isn't marked as
        // cachable. Finally, we could mark the page as clean, not dirty,
        // but we choose to leave that decision to the user; if the user
        // chooses to have a CPLB pre-defined as dirty, then they always
        // pay the cost of flushing during eviction, but don't pay the
        // cost of first-write exceptions to mark the page as dirty.

        CC = BITTST(R6, CPLB_ENABLE_DCACHE_P);
        R1 = CPLB_L1_CHBL (Z);
        R1 = ~R1;
        R6 = R2 & R1;                    // Mask off cache-in-L1
        BITSET(R6, 14);                  // Set WT
        IF !CC R2 = R6;

#if WA_05000158
        // workaround anomaly 05-00-0158 - set bit 9 in DCPLB_DATA
        // for all revisions of BF531/532/533
        BITSET(R2, 9);
#endif 


#if WA_05000204
        // workaround anomaly 05-00-0204 - by setting CPLB_L1_AOW=1
        // (allocate cache lines on read and writes).
#if defined(WA_05000204_CHECK_AVOID_FOR_REV)
# ifndef REVID
#  define REVID 0xffc00014
# endif
        P0.L = LO(REVID);
        P0.H = HI(REVID);
        R0 = [P0];
        R0 >>= 28;
        CC = R0 WA_05000204_CHECK_AVOID_FOR_REV;
        if !CC JUMP .wa_204_done;
#endif
        R3 = (CPLB_VALID|CPLB_L1_CHBL|CPLB_WT);
        R0 = R2 & R3;
        R0 = R0 - R3;
        CC = R0 == 0;
        R0 = R2;
        BITSET(R0,15);          // set CPLB_L1_AOW
        IF CC R2 = R0;          // only if write through cache enabled
.wa_204_done:
#endif

        P0.L = LO(___cplb_info);
        P0.H = HI(___cplb_info);
        [P1+0x100] = R2;        // Store new DCPLB Data
        [P1++] = R4;            // Store new DCPLB ADDR
        [P0+4] = P1;            // Update which DCPLB we'll evict next

        // We've installed the CPLB, so re-enable DCPLBs.
        // R5 is the value we last wrote to DMEM_CONTROL when we
        // were disabling CPLBs.

        P4.L = LO(DMEM_CONTROL);
        P4.H = HI(DMEM_CONTROL);
        BITSET(R5, ENDCPLB_P);
        WRITE_MEM_CONTROL(P4,R5,R2);

        (R7:4,P5:3) = [SP++];   // restore call preserved registers
        R0 = CPLB_RELOADED;
        RTS;

////////////////////////////////////////////////////////////////////////
dcplb_write:
        // If a DCPLB is marked as write-back (CPLB_WT==0), and
        // it is clean (CPLB_DIRTY==0), then a write to the
        // CPLB's page triggers a protection violation. We have to
        // mark the CPLB as dirty, to indicate that there are
        // pending writes associated with the CPLB.

        P4.L = LO(DCPLB_STATUS);
        P4.H = HI(DCPLB_STATUS);
        P3.L = LO(DCPLB_DATA0);
        P3.H = HI(DCPLB_DATA0);
        R5 = [P4];

        // A protection violation can be caused by more than just writes
        // to a clean WB page, so we have to ensure that:
        // - It's a write
        // - to a clean WB page
        // - and is allowed in the mode the access occurred.

        CC = BITTST(R5, 16);            // ensure it was a write
        IF !CC JUMP prot_violation;

        // to check the rest, we have to retrieve the DCPLB.

                                        // The low half of DCPLB_STATUS is a bit mask
        R2 = R5.L (Z);                  // indicating which CPLB triggered the event.

       // If no CPLB is indicated, then access was to an illegal address in L1.
       CC = R2 == 0;
       IF CC JUMP prot_violation;

        R3 = 30;                        // so we can use this to determine the offset
        R2.L = SIGNBITS R2;
        R2 = R2.L (Z);                  // into the DCPLB table.
        R3 = R3 - R2;
        P4 = R3;
        P3 = P3 + (P4<<2);
        R3 = [P3];                      // Retrieve the CPLB

        // Now we can check whether it's a clean WB page,

        CC = BITTST(R3, 14);            // 0==WB, 1==WT
        IF CC JUMP prot_violation;
        CC = BITTST(R3, 7);             // 0 == clean, 1 == dirty
        IF CC JUMP prot_violation;

        // Check whether the write is allowed in the mode that was active.

        R2 = 1<<3;                      // checking write in user mode
        CC = BITTST(R5, 17);            // 0==was user, 1==was super
        R5 = CC;
        R2 <<= R5;                      // if was super, check write in super mode
        R2 = R3 & R2;
        CC = R2 == 0;
        IF CC JUMP prot_violation;

        // It's a genuine write-to-clean-page.

        BITSET(R3, 7);                  // mark as dirty
        R0 = CPLB_RELOADED;

        [P3] = R3;                      // and write back.
        CSYNC;
        (R7:4,P5:3) = [SP++];           // restore call preserved registers

        RTS;

////////////////////////////////////////////////////////////////////////
.icplb_double_hit:
        // A multi-word instruction can cross a page boundary.
        // This means the first part of the instruction can be
        // in a valid page, but the second part is not, and hence
        // generates the exception. However, the fault address
        // is for the start of the instruction, not the part that's
        // in the bad page. That means we'll do a page replacement
        // on the first part of the instruction (which already has
        // an ICPLB) and install it a second time. This'll lead to
        // a double-hit. Look for an ICPLB in the unlocked area that
        // matches another ICPLB (which may be locked).

        // At this point:
        // R1 - whether we have cache enabled.
        // On exit: set R4 to an address in the page we want to load.

        // Look at ICPLB_DATA15. If it's not valid, then we haven't yet filled
        // all ICPLBs with entries, and therefore, this double-hit can't be
        // the result of pulling the same entry in twice because of an ICPLB
        // miss when an instr spans a page boundary. Instead, this must really
        // be because we've installed the entire icplb_table[] in one go, and
        // two of those entries really do overlap.

        P1.L = LO(ICPLB_DATA15);
        P1.H = HI(ICPLB_DATA15);
        R3 = [P1];
        CC = BITTST(R3, CPLB_VALID_P);
        IF !CC JUMP real_double_hit;

        P1.L = LO(___cplb_info);
        P1.H = HI(___cplb_info);
#if WA_05000428
        NOP;
#endif
        R4 = [P1+20];           // just past end of ICPLB_ADDR15

        R3 = [P1+12];           // first non-locked ICPLB (should not be -1; that
                                // would imply all-locked, so why would we have
                                // installed an ICPLB a second time?)
        R3 = R4 - R3;           // Installed ICPLBS to check
        R3 >>= 2;               // (downscaled to a count)
        R4 += -4;               // AT ICPLB_ADDR15
        P0 = R4;
        P2 = R3;
        P3 = __CPLB_TABLE_SIZE - 1; // Number of ICPLBs to check against in inner loop
        LOOP .di_outer LC0 = P2;
        LOOP_BEGIN .di_outer;
        R3 = [P0--];            // Look at next ICPLB in non-locked region
        P1 = P0;                // Compare against all ICPLBs below
        LOOP .di_inner LC1 = P3;
        LOOP_BEGIN .di_inner;
        R2 = [P1--];            // Look at next ICPLB (including locked ones)
        CC = R2 == R3;          // Do they contain the same page start?
        IF CC JUMP .di_match;
        LAST_LOOP_INSTR
        LOOP_END .di_inner;
        P3 += -1;               // One ICPLB less to compare against next time round
        LOOP_END .di_outer;

        // We haven't found two CPLBs that matched. Actual double-hit.
        JUMP real_double_hit;

.di_match:
        // R3 contains the start address of the page that contains the fault.
        // [P0+4] is the double-entered ICPLB in the non-locked region.
        P0 += 4;
        P1.L = LO(___cplb_info);
        P1.H = HI(___cplb_info);
        [P1+16] = P0;           // replace this one next time
        R4 = RETX;              // Where we'll continue, after this fault.
        R4 += 32;               // Set our supposed fault address after that multi-issued instruction
        R4 += 32;               // (which will take the address into the next page)
                                // Then continue with an ICPLB-Miss action for that address.
        JUMP .back_from_icplb_double_hit;

////////////////////////////////////////////////////////////////////////
/* FAILED CASES*/
no_page_in_table:
        (R7:4,P5:3) = [SP++];   // restore call preserved registers
        R0 = CPLB_NO_ADDR_MATCH;
        RTS;

all_locked:
        (R7:4,P5:3) = [SP++];   // restore call preserved registers
        R0 = CPLB_NO_UNLOCKED;
        RTS;

prot_violation:
        (R7:4,P5:3) = [SP++];   // restore call preserved registers
        R0 = CPLB_PROT_VIOL;
        RTS;

real_double_hit:
        (R7:4,P5:3) = [SP++];   // restore call preserved registers
        R0 = CPLB_DOUBLE_HIT;
        RTS;

._cplb_mgr.end:


////////////////////////////////////////////////////////////////////////
.section cplb_data;
.align 4;
 
#if WA_05000261
.last_RETX:
.byte4 = 0xFFFFFFFF;           // Initialize to a value not expected to be a code address
#endif

___page_size_table:
.byte4=
        0x00000400,            /* 1K */
        0x00001000,            /* 4K */
        0x00100000,            /* 1M */
        0x00400000;            /* 4M */
.___page_size_table.end:

___page_prefix_table:
.byte4 = 
        0xFFFFFC00,            /* 1K */
        0xFFFFF000,            /* 4K */
        0xFFF00000,            /* 1M */
        0xFFC00000;            /* 4M */
.___page_prefix_table.end:

___dcplb_preference:
.byte4=(CPLB_VALID),
       (CPLB_DIRTY|CPLB_LOCK),
       (CPLB_LOCK);
.___dcplb_preference.end:

.global ___page_size_table;
.global ___page_prefix_table;
.global ___dcplb_preference;

.type ___page_size_table, STT_OBJECT;
.type ___page_prefix_table, STT_OBJECT;
.type ___dcplb_preference, STT_OBJECT;

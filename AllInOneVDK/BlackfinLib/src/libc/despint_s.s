/*
** Copyright (C) 2001-2011 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
** Blackfin despatcher for signal() and interrupt().
**/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libName=libsmall;
.file_attr libGroup=signal.h;
.file_attr FuncName=_despint;
.file_attr libFunc=_despint;
.file_attr libFunc=despint;
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";
#endif

.SECTION/DOUBLEANY program;

#if defined(__ADSPLPBLACKFIN__)
#include <def_LPBlackfin.h>
#else
#include <defblackfin.h>
#endif

#include <macros.h>
#include <sys/anomaly_macros_rtl.h>

#if defined(__WORKAROUND_SSYNC) && WA_05000283
#error Conflicting errata workarounds 05-00-054 and 05-00-0283
#endif

.align 2;
_despint:
#if defined(__WORKAROUND_SSYNC)
      SSYNC;                  // Not required for parts that suffer for 05000312
#endif
#if WA_05000283
      [--SP] = ASTAT;
      [--SP] = P0; 
      CC = R0 == R0;
      P0.H = 0xFFC0;
      P0.L = 0x0014;
      IF CC JUMP .wa05000283; // This mispredicted jump is always taken so
#if WA_05000428
      // Speculative read from L2 by Core B may cause a write to L2 to fail
      //
      // Suppress the warning associated with the following
      // instruction because it is not loading from L2 memory

.MESSAGE/SUPPRESS 5517,5508 FOR 1 LINES;
      R0 = [P0];              // this MMR read is killed
      NOP;
      NOP;
#else
      R0 = [P0];              // this MMR read is killed
#endif

.wa05000283:
      P0 = [SP++];
      ASTAT = [SP++];

#elif defined(__WORKAROUND_KILLED_MMR_WRITE) || \
      defined(__WORKAROUND_FLAGS_MMR_ANOM_311)
      [--SP] = P0; 
      P0.H = 0xFFC0;
      P0.L = 0x0014;
      [--SP] = R0; 
      R0 = [P0];              // Dummy MMR read
      R0 = [SP++];
      P0 = [SP++];
#endif

      save_user_regs;
      save_super_regs;

      L0 = 0;                 // zero L-regs for C ABI
      L1 = 0;
      L2 = 0;
      L3 = 0;

      // Find out which interrupt we are
      P0.H = (IPEND >> 16);
      P0.L = (IPEND & 0xFFFF);
#if defined(__ADSPBLACKFIN__) && !defined(__ADSPLPBLACKFIN__)
/* __ADSPBF535__ core only */
      R0 = W[P0] (Z);         // IPEND is a 16-bit MMR.
#else
      R0 = [P0];              // IPEND is a 32-bit MMR.
#endif
      BITCLR(R0,1);           // ignore the reset bit
      BITCLR(R0,4);           // and global interrupt enable bit
      R4 = 0;
.bitl:
      CC = BITTST(R0, 0);
      R0 >>= 1;
      IF CC JUMP .got_int;    // R4'th bit is set
      CC = R0 == 0;
      R4 += 1;
      IF !CC JUMP .bitl;      // more bits to test?
      // If we got here, no bits were set. Should never happen.

#if WA_05000428
      // Speculative read from L2 by Core B may cause a write to L2 to fail

      nop;
      nop;
      nop;
#endif
      restore_super_regs;
      restore_user_regs;
      RTS;

.got_int:
      // R4 holds the number of the event that's in progress.
      // Retrieve the corresponding signal handling function.
      P0.L = __vector_table;
      P0.H = __vector_table;
      P1 = R4;
      P0 = P0 + (P1<<2);
      R0 = [P0 + 0];
      // Meaning:
      // zero - interrupt is SIG_DFL - just return.
      // LSB clr - signal func - reset, then call.
      // LSB set - intr func - just call.
      CC = R0 == 0;
      IF CC JUMP .ret_seq;    // no handler
      CC = BITTST(R0,0);
      BITCLR(R0,0);           // for the call 
      P1 = R0;
      IF CC JUMP .call_hdr;
      // Have to reset the handler.
      R1 = 0;
      [P0 + 0] = R1;

.call_hdr:
      SP += -12;
      R0 = R4;
      CALL (P1);
      SP += 12;

.ret_seq:
      // Deal with the various return sequences
      // Because there are four different return instructions,
      // and once we've restored regs we can't make any decisions,
      // we have to have four copies of the epilogue.
      CC = R4 <= 3;           // Event, NMI, EMU.
      IF CC JUMP .not_int;

#if WA_05000428
      // Speculative read from L2 by Core B may cause a write to L2 to fail

      nop;
      nop;
      nop;
#endif
      restore_super_regs;
      restore_user_regs;
      RTI;

.not_int:
      CC = R4 == 3;
      IF !CC JUMP .not_event;

#if WA_05000428
      // Speculative read from L2 by Core B may cause a write to L2 to fail

      nop;
      nop;
      nop;
#endif
      restore_super_regs;
      restore_user_regs;
      RTX;

.not_event:
      CC = R4 == 2;
      IF !CC JUMP .not_nmi;

#if WA_05000428
      // Speculative read from L2 by Core B may cause a write to L2 to fail

      nop;
      nop;
      nop;
#endif
      restore_super_regs;
      restore_user_regs;
      RTN;

.not_nmi:
      restore_super_regs;
      restore_user_regs;
      RTE;
#if defined(__WORKAROUND_SPEC_MULTIPLY_F3_F_008013)
      NOP;
      NOP;
      NOP;
#endif

._despint.end:

.global _despint;
.type _despint, STT_FUNC;
.extern __vector_table;

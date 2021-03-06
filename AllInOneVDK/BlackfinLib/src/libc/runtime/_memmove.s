/* Copyright (C) 2000-2008 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
// _memmove: internal version of memmove(), issued by the compiler
// to copy blocks of data around.

/* Define macro VDSP45_COMP_SUPPORT to get a version suitable for use in VDSP++
** 4.5 and later. The name of this function is modified to ___memmove2 for
** later VDSP++ 4.5 because that compiler assumes that Loop-0 (L0) regs is 
** clobbered and has the ability to use L1 regs over calls to ___memmove2. 
** The legacy behaviour saved and restored L0 regs which is less efficient.
*/

#ifdef VDSP45_COMP_SUPPORT
#if defined(__SYSCALL_INFO__)
%regs_clobbered "R0-R3,P0-P2,LC0,LT0,LB0,I1,CCset"
%notes "Compiler version of memmove."
%syscall ___memmove2(pV=pVpcVX)
#endif
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libName=librt;
.file_attr libName=librt_fileio;
#ifdef VDSP45_COMP_SUPPORT
.file_attr libFunc=___memmove2;
.file_attr FuncName=___memmove2;
#else
.file_attr libFunc=___memmove;
.file_attr FuncName=___memmove;
#endif
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";
#endif

#include <sys/anomaly_macros_rtl.h>

.section program;
.epctext:

.align 2;

#ifdef VDSP45_COMP_SUPPORT
___memmove2:
#else
___memmove:
#endif
		CC = R2 <=  0;	// length not positive?
		IF CC JUMP .early_exit;	// Nothing to do

		P0 = R0 ;	// dst
		P1 = R1 ;	// src
		P2 = R2 ;	// length

#ifndef VDSP45_COMP_SUPPORT
      // save loop registers to allow hardware loops
      // over calls to memcpy support
      [--SP] = LC0;
      [--SP] = LT0;
      [--SP] = LB0;
#endif

		// check for overlapping data
		CC = R1 < R0;	// src < dst
		IF !CC JUMP .no_overlap;
		R3 = R1 + R2;
		CC = R0 < R3;	// and dst < src+len
		IF CC JUMP .has_overlap;

.no_overlap:
		// Check for aligned data.

		R3 = R1 | R0;
		R0 = 0x3;
		R3 = R3 & R0;
		CC = R3;	// low bits set on either address?
		IF CC JUMP .not_aligned;

		// Both addresses are word-aligned, so we can copy
		// at least part of the data using word copies.
		P2 = P2 >> 2;
		CC = P2 <= 2;
		IF !CC JUMP .more_than_seven;
		// less than eight bytes...
		P2 = R2;
		LSETUP(.three_start, .three_end) LC0=P2;
		R0 = R1;	// setup src address for return
.three_start:	R3 = B[P1++] (X);
.three_end:	B[P0++] = R3;
#ifndef VDSP45_COMP_SUPPORT
      // restore loop registers
      LB0 = [SP++]; 
      LT0 = [SP++];
      LC0 = [SP++];
#endif 
		RTS;

.more_than_seven:
		// There's at least eight bytes to copy.
		P2 += -1;	// because we unroll one iteration
		LSETUP(.word_loop_s, .word_loop_e) LC0=P2;
		R0 = R1;
		I1 = P1;
		R3 = [I1++];
#if !defined(__WORKAROUND_AVOID_DAG1)
.word_loop_s:
.word_loop_e:	MNOP || [P0++] = R3 || R3 = [I1++];
#else
.word_loop_s:	[P0++] = R3;
.word_loop_e:	R3 = [I1++];
#endif
		[P0++] = R3;
		// Any remaining bytes to copy?
		R3 = 0x3;
		R3 = R2 & R3;
		CC = R3 == 0;
		P1 = I1;	// in case there's something left,
		IF !CC JUMP .bytes_left;

#if WA_05000428
      // Speculative read from L2 by Core B may cause a write to L2 to fail

      NOP;
      NOP;
      NOP;
#endif

#ifndef VDSP45_COMP_SUPPORT
      // restore loop registers
      LB0 = [SP++]; 
      LT0 = [SP++];
      LC0 = [SP++];
#endif 
		RTS;

.bytes_left:	P2 = R3;
.not_aligned:
		// From here, we're copying byte-by-byte.
		LSETUP (.byte_start , .byte_end) LC0=P2;
		R0 = R1;	// Save src address for return
.byte_start:	R1 = B[P1++] (X);
.byte_end:	B[P0++] = R1;
#ifndef VDSP45_COMP_SUPPORT
      // restore loop registers
      LB0 = [SP++]; 
      LT0 = [SP++];
      LC0 = [SP++];
#endif 
		RTS;

.has_overlap:
		// Need to reverse the copying, because the
		// dst would clobber the src.
		// Don't bother to work out alignment for
		// the reverse case.
		R0 = R1;	// save src for later.
		P0 = P0 + P2;
		P0 += -1;
		P1 = P1 + P2;
		P1 += -1;
		LSETUP(.over_start, .over_end) LC0=P2;
.over_start:	R1 = B[P1--] (X);
.over_end:	B[P0--] = R1;
#ifndef VDSP45_COMP_SUPPORT
      // restore loop registers
      LB0 = [SP++]; 
      LT0 = [SP++];
      LC0 = [SP++];
#endif 
.early_exit:
		RTS;

#ifdef VDSP45_COMP_SUPPORT
.___memmove2.end:
	.global ___memmove2;
	.type ___memmove2,STT_FUNC;
#else
.___memmove.end:
	.global ___memmove;
	.type ___memmove,STT_FUNC;
#endif

.epctext.end:
	.section data1;


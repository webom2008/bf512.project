/* Copyright (C) 2003-2008 Analog Devices Inc., All Rights Reserved,
*/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libName=libc;
.file_attr libGroup=string.h;
.file_attr libFunc=_memcpy;
.file_attr FuncName=_memcpy;
.file_attr libFunc=memcpy;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
#endif

#include <sys/anomaly_macros_rtl.h>

.section program;

.align 2;
   
//===============================================
// C Library function MEMCPY
// R0 = To Address ( leave unchanged to form result)
// R1 = From Address
// R2 = count
//
// Favours word alignment
	  
_memcpy:
	[--SP] = P3;
	P0 = R0;              // P0 = To address
	P3 = R1;              // P3 = From Address  
	P2 = R2 ;             // P2 = count
	CC = R2 <= 7(IU);       
	IF CC JUMP  too_small;  
	I0 = R1;
	R3 = R1 | R0;         // OR addresses together
	R3 <<= 30;            // check bottom two bits
	CC =  AZ;             // AZ set if zero.
	IF !CC JUMP  bytes ;  // Jump if addrs not aligned.
	P1 = P2 >> 2;         // count = n/4
	P1 += -1;
	R3 =  3;
	R2 = R2 & R3;         // remainder
	P2 = R2;              // set remainder
	R1 = [I0++];
#if !defined(__WORKAROUND_AVOID_DAG1)
	LSETUP (quad_loop , quad_loop) LC0=P1;
quad_loop:		MNOP || [P0++] = R1 || R1 = [I0++];
#else
	LSETUP (quad_loop_s , quad_loop_e) LC0=P1;
quad_loop_s:	[P0++] = R1;
quad_loop_e:	R1 = [I0++];
#endif
	[P0++] = R1; 
        
	CC = P2 == 0;         // any remaining bytes?
	P3 = I0;              // Amend P3 for remaining copy
	IF !CC JUMP bytes;

#if WA_05000428
	// Speculative read from L2 by Core B may cause a write to L2 to fail

	nop;
	nop;
	nop;
#endif
	P3 = [SP++];
	RTS;

too_small:
	CC = P2 == 0;         // Check zero count
	IF CC JUMP finished;  // very unlikely

#if WA_05000428
	// Speculative read from L2 by Core B may cause a write to L2 to fail

	nop;
	nop;
#endif

bytes:
	LSETUP (byte_loop_s , byte_loop_e) LC0=P2;
byte_loop_s:	R1 = B[P3++](Z);
byte_loop_e:	B[P0++] = R1;

finished:
	P3 = [SP++];
	RTS;


._memcpy.end:
	    .global _memcpy;
	   .type _memcpy,STT_FUNC;

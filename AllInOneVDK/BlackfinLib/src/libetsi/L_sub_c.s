// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2009 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=L_sub_c;
.file_attr FuncName=_L_sub_c;
.file_attr libFunc=_L_sub_c;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#include <sys/anomaly_macros_rtl.h>

        .section program;
.epctext:
        .align 2;
_L_sub_c:
#if __SET_ETSI_FLAGS
	I0.L = _Carry;
	I0.H = _Carry;
	R2 = [I0];
	CC = R2 == 1;
	IF !CC JUMP carry_notset (BP);
	R3 = 0;
	[I0] = R3;	// Clear the carry flag variable
	BITSET(R3,31); // Set the sign bit in R3
	CC = R1 == R3; // If L_var2 is equal to 0x80000000, i.e. fract -1.0
	IF !CC JUMP do_sub;
	R1 = -R1; // Then add 1.0 to L_var1
	CALL.X _L_add_c;
	RTS;
do_sub:
	R0 = R0 - R1; // do the subtraction L_var1 - L_var2 
	CC = R0 <= 0; // If the result is negative
	IF CC JUMP to_return; // Then return
	I1.L = _Overflow;
	I1.H = _Overflow;
	R2 = 0; 			// Otherwise, clear the overflow
	[I1] = R2;
	RTS;

carry_notset:
	I1.L = _Overflow;
	I1.H = _Overflow;
	R2 = R0 - R1 (NS); // Calculate L_test
	CC = BITTST(R2,31);
	IF !CC JUMP pos_result;
	CC = BITTST(R0,31);
	IF CC JUMP zero_carry;
	CC = BITTST(R1,31);
	IF !CC JUMP zero_carry;
	R0 = 1;
	[I1] = R0;
	R1 = 0;
	JUMP checks_done;

pos_result:
	CC = BITTST(R0,31);
	IF !CC JUMP check_signchange;
	CC = BITTST(R1,31);
	IF CC JUMP check_signchange;
	R1 = 1;
	[I1] = R1;
	JUMP checks_done;

check_signchange:
	R3 = R0 ^ R1;
	CC = R3 <= 0;
	IF CC JUMP zero_carry;
	R0 = 0;
	[I1] = R0;
	R1 = 1;
	JUMP checks_done;

zero_carry:
	R1 = 0;

checks_done:
	R3 = 0;
	BITSET(R3,31);
	CC = R2 == R3;				// MIN_32
	IF !CC JUMP set_carry_var;
	R0 = 1;
	[I1] = R0;

set_carry_var:
	[I0] = R1;

sub_one:
	R3 = 1;
	R0 = R2 - R3 (NS);

to_return:
	RTS;

#else
	R0 = R0 - R1 (NS);
	R1 = 1;
	R0 = R0 - R1 (NS);
#if WA_05000371
	NOP;
#endif
	RTS;
#endif

._L_sub_c.end:
        .global _L_sub_c;
        .type _L_sub_c,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _L_add_c;
	.type _L_add_c,STT_FUNC;
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
	.extern _Carry;
	.type _Carry,STT_OBJECT;
#endif
.epctext.end:
        .section data1;

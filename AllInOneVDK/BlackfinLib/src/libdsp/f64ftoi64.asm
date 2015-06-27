/*
** Copyright (C) 2003-2004 Analog Devices, Inc. All Rights Reserved.
**
** Convert 64-bit non-IEEE floating-point numbers to signed 64-bit integer.
**
** This function converts 64-bit non-IEEE floating point numbers
** into 64-bit signed integers. It receives its input in R1:0,
** and returns its result in R1:0.
*/

.file_attr libGroup      = integer_support;
.file_attr libGroup      = floating_point_support;
.file_attr libName = libf64fast;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = ___float64_to_int64;
.file_attr FuncName      = ___float64_to_unsigned_int64_round_to_zero;
.file_attr FuncName      = ___float64_to_unsigned_int64;
.file_attr FuncName      = ___float64_to_int64_round_to_zero;
.file_attr libFunc = ___float64_to_int64;
.file_attr libFunc = ___float64_to_unsigned_int64_round_to_zero;
.file_attr libFunc = ___float64_to_unsigned_int64;
.file_attr libFunc = ___float64_to_int64_round_to_zero;


.section program;
.align 2;

___float64_to_int64:
___float64_to_unsigned_int64_round_to_zero:
___float64_to_unsigned_int64:
___float64_to_int64_round_to_zero:
	// Check for zero
	R2 = R0 | R1;
	CC = R2 == 0;
	IF CC JUMP .ret_zero;

	// Adjust the exponent range.

	R0 += -31;

	CC = R0 < 0;
	IF CC JUMP .shift_right;

	R2 = 32;
	CC = R2 <= R0;
	IF CC JUMP .shift_left_into_high;

	// We're shifting some of the bits from
	// the low half into the high half.

	R2 = R2 - R0;
	R3 = R0;
	R0 = R1;
	R1 >>= R2;
	R0 <<= R3;
.ret_zero:
	RTS;

.shift_left_into_high:

	// All the bits are in the high half, or off the
	// top of the high half.
	R0 = R0 - R2;
	R1 <<= R0;
	R0 = 0;
	RTS;

.shift_right:

	// We're shifting right, and losing bits off the low half.

#ifdef __WORKAROUND_SHIFT
	R2 = -32;
	R0 = MAX(R2,R0);
	R0 = ASHIFT R1 BY R0.L;
#else
	R2 = -R0;
	R0 = R1;
	R0 >>>= R2;
#endif
	R1 = R0 >>> 31;
	RTS;
.___float64_to_int64.end:
.___float64_to_unsigned_int64_round_to_zero.end:
.___float64_to_unsigned_int64.end:
.___float64_to_int64_round_to_zero.end:

.global ___float64_to_int64;
.global ___float64_to_int64_round_to_zero;
.global ___float64_to_unsigned_int64;
.global ___float64_to_unsigned_int64_round_to_zero;
.type ___float64_to_int64, STT_FUNC;
.type ___float64_to_unsigned_int64, STT_FUNC;
.type ___float64_to_int64_round_to_zero, STT_FUNC;
.type ___float64_to_unsigned_int64_round_to_zero, STT_FUNC;

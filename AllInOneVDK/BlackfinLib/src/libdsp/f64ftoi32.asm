/*
** Copyright (C) 2003-2004 Analog Devices, Inc. All Rights Reserved.
**
** Convert 64-bit non-IEEE floating-point numbers to signed 32-bit integer.
**
** This function converts 64-bit non-IEEE floating point numbers
** into 32-bit signed integers. It receives its input in R1:0,
** and returns its result in R0.
*/

.file_attr libGroup      = integer_support;
.file_attr libGroup      = floating_point_support;
.file_attr libName = libf64fast;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = ___float64_to_unsigned_int32;
.file_attr FuncName      = ___float64_to_int32;
.file_attr FuncName      = ___float64_to_unsigned_int32_round_to_zero;
.file_attr FuncName      = ___float64_to_int32_round_to_zero;
.file_attr libFunc = ___float64_to_int32;
.file_attr libFunc = ___float64_to_unsigned_int32;
.file_attr libFunc = ___float64_to_unsigned_int32_round_to_zero;
.file_attr libFunc = ___float64_to_int32_round_to_zero;


.section program;
.align 2;

___float64_to_int32:
___float64_to_unsigned_int32:
___float64_to_unsigned_int32_round_to_zero:
___float64_to_int32_round_to_zero:
	// Check for zero
	R2 = R0 | R1;
	CC = R2 == 0;
	IF CC JUMP .ret_zero;

	// Adjust the exponent range.

	R0 += -31;

	// If the exponent is now negative, we're shifting
	// right. Otherwise, we're shifting left.

	R2 = R1;
	R1 <<= R0;
	CC = R0 < 0;
#ifdef __WORKAROUND_SHIFT
	R3 = -32;
	R3 = MAX(R3,R0);
	R2 = ASHIFT R2 BY R3.L;
#else
	R0 = -R0;
	R2 >>>= R0;
#endif
	IF CC R0 = R2;
	IF !CC R0 = R1;
.ret_zero:
	RTS;
.___float64_to_int32.end:
.___float64_to_unsigned_int32.end:
.___float64_to_unsigned_int32_round_to_zero.end:
.___float64_to_int32_round_to_zero.end:

.global ___float64_to_int32;
.global ___float64_to_int32_round_to_zero;
.global ___float64_to_unsigned_int32;
.global ___float64_to_unsigned_int32_round_to_zero;
.type ___float64_to_int32, STT_FUNC;
.type ___float64_to_unsigned_int32, STT_FUNC;
.type ___float64_to_int32_round_to_zero, STT_FUNC;
.type ___float64_to_unsigned_int32_round_to_zero, STT_FUNC;

/*
** Convert 32-bit unsigned integer to non-IEEE 32-bit floating point.
** Copyright (C) 2004 Analog Devices, Inc. All Rights Reserved.
*/

.file_attr libGroup      = integer_support;
.file_attr libGroup      = floating_point_support;
.file_attr libName = libf64fast;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___unsigned_int32_to_float32;
.file_attr FuncName      = ___unsigned_int32_to_float32;

.section program;
.align 2;

___unsigned_int32_to_float32:

	CC = R0 == 0;
	IF CC JUMP .ret_zero;

	R1 = 31 (Z);

	// If the high bit's
	// in use, we have to shift right one more space (to
	// ensure a zero sign bit), which can cause us to lose
	// what's in the low bit.

	CC = R0 < 0;
	R2 = CC;
	R0 >>= R2;
	R1 = R1 + R2;

	// Normalise.
	R2.L = SIGNBITS R0;
	R2 = R2.L (X);
	R0 <<= R2;
	R0.L = R1.L - R2.L (S);

.ret_zero:
	RTS;
.___unsigned_int32_to_float32.end:
.global ___unsigned_int32_to_float32;
.type ___unsigned_int32_to_float32, STT_FUNC;


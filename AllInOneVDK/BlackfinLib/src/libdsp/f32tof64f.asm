/*
** Copyright (C) 2003-2006 Analog Devices, Inc. All Rights Reserved.
**
** Convert a 32-bit IEEE floating point number to a 64-bit
** non-IEEE floating point number.
**
** This function promotes a 32-bit floating point number in
** IEEE format into a 64-bit floating point number, not in
** IEEE format. The operand is received in R0, and the
** result is in R1:0.
*/

.file_attr libGroup      = floating_point_support;
.file_attr libName       = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = ___float32_to_float64;

.section program;
.align 2;

___float32_to_float64:
	// Check for special cases - zero, Inf, Nan.
	R1 = R0 << 1;
	CC = R1 == 0;
	IF CC R0 = R1;
	IF CC JUMP .ret_zero;
	R2 = 0xFF (Z);
	R2 <<= 24;		// R2 now 0xFF000000
	CC = R2 <= R1 (IU);
	IF CC JUMP .inf_or_nan;

	// Input is a normal number.
	// Get the exponent, and unbias it, taking
	// into account the shift-right-by-one effect
	// of changing to a fraction.

	R1 >>= 24;
	R2 = 126 (Z);	// E - 127 + 1 (because we're
	R1 = R1 - R2;	// 0.x, not 1.x)

	// Note the sign, for later

	R2 = R0;

	// Remove the exponent, add the hidden bit, and
	// position just to right of decimal point.

	R0 <<= 9;
	R0 >>= 2;
	BITSET(R0, 30);		// hidden bit now explicit

	// negate if sign bit set, and move things around a bit.

	CC = R2 < 0;
	R2 = -R0;
	IF !CC R2 = R0;

	R0 = R1;
	R1 = R2;
.ret_zero:
	RTS;

.inf_or_nan:
	// Check whether it's an Inf or a NaN.

	CC = R1 == R2;
	IF !CC JUMP .is_nan;

	// It's an Inf. Return appropriately-signed Inf.
	CC = R0 < 0;
	R1 = 0;
	R0 = R1;
	BITSET(R0, 31);		// now 0x80000000
	R2 = -R0;
	IF !CC R0 = R2;		// could now be 0x7FFFFFFF
	RTS;

.is_nan:
	R0 = 0;
	BITSET(R0, 31);
	R1 = R0;
	RTS;
.___float32_to_float64.end:
.global ___float32_to_float64;
.type ___float32_to_float64, STT_FUNC;

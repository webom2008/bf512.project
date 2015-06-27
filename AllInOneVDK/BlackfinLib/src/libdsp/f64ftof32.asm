/*
** Copyright (C) 2003-2006 Analog Devices, Inc. All Rights Reserved.
**
** Convert non-IEEE 64-bit floating point number to IEEE-format
** 32-bit floating point number.
**
** This function truncates a 64-bit floating point number to
** a 32-bit floating point number, also converting from
** non-IEEE to IEEE format. The operand is received in R1:0,
** and the result is in R0.
*/

.file_attr libGroup      = floating_point_support;
.file_attr libName       = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = ___float64_to_float32;

.section program;
.align 2;

___float64_to_float32:

	// Check for zero/Nans first.
	// Infs take care of themselves later.

	R2 = R0 | R1;
	CC = R2 == 0;
	IF CC JUMP .ret_zero;

	R2 = 0;
	BITSET(R2, 31);		// now 0x80000000
	CC = R0 == R2;
	R3 = R0 - R2;
	CC &= AZ;
	IF CC JUMP .is_nan;

	// A normal number.

	// Note sign. Negate if negative, since IEEE
	// mantissas are unsigned.

	P0 = R1;
	CC = R1 < 0;
	R2 = -R1;
	IF CC R1 = R2;

	// This gives us 0.1xxx normalised, but IEEE numbers are
	// 1.xxx normalised, so the IEEE exponent is one less.

	R0 += -1;

	// Normalise. This leaves a sign-bit in place.
	// (This shouldn't actually have an effect, since
	// the number should be normalised anyway, but
	// just in case...)
	R2.L = SIGNBITS R1;
	R2 = R2.L (X);
	R1 <<= R2;

	// Adjust the exponent by how much we shifted, too.

	R0 = R0 - R2 (S);


	// might be out of the
	// range of an IEEE float.

	R2 = 127 (Z);
	CC = R2 < R0;
	IF CC JUMP .too_big;
	R3 = -R2;
	CC = R0 < R3;
	IF CC JUMP .too_small;

	// Bias the exponent

	R0 = R0 + R2;

	// Position mantissa at LSB, and remove hidden bit.

	BITCLR(R1, 30);
	R1 >>= 7;

	// Position Exponent at MSB, and pull in sign.

	R0 <<= 24;
	CC = P0 < 0;
	R0 = ROT R0 BY -1;

	// Include mantissa

	R0 = R0 | R1;
.ret_zero:
	RTS;

.is_nan:
	R0 = 0xFF1 (Z);		// all-ones exponent, and a
	R0 <<= 23;		// non-zero mantissa.
	CC = R1 < 0;		// check sign
	R0 = ROT R0 BY -1;	// and move into position
	RTS;

.too_big:
	R0 = 0xFF (Z);		// all-ones exponent, and
	R0 <<= 24;		// a zero mantissa.
	CC = R1 < 0;		// check sign
	R0 = ROT R0 BY -1;	// and move into position
	RTS;

.too_small:
	R0 = 0;			// tend to zero
	CC = R1 < 0;		// of the appropriate sign
	R0 = ROT R0 BY -1;
	RTS;
.___float64_to_float32.end:

.global ___float64_to_float32;
.type ___float64_to_float32, STT_FUNC;

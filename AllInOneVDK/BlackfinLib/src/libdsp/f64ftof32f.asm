/*
** Copyright (C) 2003-2004 Analog Devices, Inc. All Rights Reserved.
**
** Convert non-IEEE 64-bit floating point to non-IEEE 32-bit
** floating point.
*/

.file_attr libGroup      = floating_point_support;
.file_attr libName       = libf64fast;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc       = ___float64_to_float32;
.file_attr FuncName      = ___float64_to_float32;

.section program;
.align 2;

___float64_to_float32:
	/* If the exponent is outside the range of the
	   16-bit space in the result, then we have an
	   infinity. In that case, the sign-extended form
	   of the exponent is sufficient, and we zero the
	   mantissa. Otherwise, we just truncate the mantissa. */

	R2 = R1.L (X);
	R3 = 0;
	CC = R2 == R1;
	IF !CC R0 = R3;		// Clear mantissa if out of range
	R0.L = R2.L << 0;
	RTS;
.___float64_to_float32.end:
.global ___float64_to_float32;
.type ___float64_to_float32, STT_FUNC;

/*
** Copyright (C) 2003-2004 Analog Devices, Inc. All Rights Reserved.
**
** Convert 32-bit signed integer to 64-bit non-IEEE floating point.
**
** This function implements conversions from 32-bit signed
** integers, into the special non-IEEE 64-bit floating point
** format. It receives its input in R0, and returns its
** result in R1:0.
*/

.file_attr libGroup      = floating_point_support;
.file_attr libGroup      = integer_support;
.file_attr libName = libf64fast;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___int32_to_float64;
.file_attr FuncName      = ___int32_to_float64;

.section program;
.align 2;

___int32_to_float64:

	// A signed 32-bit int has 31 bits of significand,
	// and a sign bit. Whereas the high-half of our
	// non-IEEE format has 31 bits of fractional
	// significand, and a sign bit. So we're just moving
	// the decimal point 31 spaces to the left, and thus
	// setting the exponent to be 31. Zero is the exception.

	R1 = R0;
	R0 = 31 (Z);

	// Normalise.

	R2.L = SIGNBITS R1;
	R2 = R2.L (X);
	R1 <<= R2;
	R0 = R0 - R2 (S);

	// Check for zero
	CC = R1 == 0;
	IF CC R0 = R1;
	RTS;
.___int32_to_float64.end:
.global ___int32_to_float64;
.type ___int32_to_float64, STT_FUNC;


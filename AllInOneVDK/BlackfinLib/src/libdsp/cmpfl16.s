/*
** Float16 compare.  Compares 2 non-IEEE-754 floating point numbers (16.16)
**
** Copyright (C) 2005 Analog Devices, Inc. All Rights Reserved.
*/

.file_attr libGroup      = floating_point_support;
.file_attr libName       = libf64fast;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc       = ___float32_cmp;
.file_attr FuncName      = ___float32_cmp;

.section program;
.align 2;

___float32_cmp:
	CC = R0 < 0;
	R2 = CC;
	CC = R1 < 0;
	R3 = CC;
	CC = R2 == R3;
	IF !CC JUMP diff_signs;

	// signs are the same. Examine exponents.
	R0 = R0 -|- R1;
	R1 = R0.L (X);	// difference between exponents
	R0 >>>= 16;	// difference between mantissas
	CC = R1 == 0;	// If exponents are the different, they decide.
	IF !CC R0 = R1;	// Otherwise, mantissas decide.
	R1 = -R0;	// If x was negative (and therefore y also negative)
	CC = R2 == 0;	// then we have to negate our result
	IF !CC R0 = R1;	//
	RTS;

diff_signs:
	// Signs are different, so we return a number that
	// derives its sign from whether X was negative.
	R0 = R2 << 31;
	R0 += 1;
	RTS;
.___float32_cmp.end:
.global ___float32_cmp;
.type ___float32_cmp, STT_FUNC;

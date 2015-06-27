/*
** Copyright (C) 2003-2004 Analog Devices, Inc. All Rights Reserved.
**
** 64-bit non-IEEE floating point multiplication.
**
** This function does floating point multiplication for
** 64-bit non-IEEE numbers. X is received in R1:0. Y is
** received in R2 and on the stack. The result is returned
** in R1:0.
*/

.file_attr libGroup      = floating_point_support;
.file_attr libName = libf64fast;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___float64_mul;
.file_attr FuncName      = ___float64_mul;

.section program;
.align 2;

___float64_mul:
	R3 = [SP+12];		// Get high half of Y.

	// Check for zero
	CC = R1 == 0;
	IF CC JUMP .ret_zero;
	CC = R3 == 0;
	IF CC JUMP .ret_zero;

	R0 = R0 + R2 (S);	// Add exponents.

	// 32 x 32 bit fractional multiply. Check the
	// pathological case, where saturation may occur.
	R2 = PACK(R1.L, R3.L);
	CC = R2 == 0;
	CC = !CC;

	A1 = R1.L * R3.L (FU);
	A1 = A1 >> 16;
	A0 = R1.H * R3.H, A1 += R1.H * R3.L (M);
	CC &= AV0;
	A1 += R3.H * R1.L (M);
	A1 = A1 >>> 15;
	R2 = CC;
	R1 = (A0 += A1);
	R1 = R1 + R2;

	// Normalise.
#if defined(__WORKAROUND_SIGNBITS) || defined(__WORKAROUND_DREG_COMP_LATENCY)
	NOP;	// wonder if there is anything better to do here?
#endif
	R2.L = SIGNBITS R1;
	R2 = R2.L (X);
	R1 <<= R2;
	R0 = R0 - R2 (S);

	// Are we returning zero?

	CC = R1 == 0;
	IF CC R0 = R1;

	RTS;

.ret_zero:
	R0 = 0;
	R1 = R0;
	RTS;
.___float64_mul.end:

.global ___float64_mul;
.type ___float64_mul, STT_FUNC;

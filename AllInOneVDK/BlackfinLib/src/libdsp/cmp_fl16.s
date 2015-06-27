/*
** Float16 compare.  Compares 2 non-IEEE-754 floating point numbers (16.16)
**
** Copyright (C) 2003 Analog Devices, Inc. All Rights Reserved.
*/

.file_attr libGroup      = float16.h;
.file_attr libFunc       = _cmp_fl16;
.file_attr libFunc       = cmp_fl16;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = _cmp_fl16;

.section program;
.align 2;

_cmp_fl16:
	[--SP] = R7;

	[SP + 8] = R0;
	[SP + 4] = R1;

	R3 = W[SP + 10] (X);
	CC = R3 == 0;

	R0 = W[SP + 8] (X);
	R7 = W[SP + 6] (X);

	R2 = -32768;
	IF CC R0 = R2;
	CC = R7 == 0;

	R1 = W[SP + 4] (X);
	IF CC R1 = R2;

	R2 = MAX(R0, R1);
	R2 = R2.L (X);

	R0 = R2 - R0;
	R1 = R2 - R1;
	R3 >>>= R0;
	R7 >>>= R1;
	R0 = R3 - R7 (NS) || R7 = [SP++];

	RTS;

._cmp_fl16.end:
.global _cmp_fl16;
.type _cmp_fl16, STT_FUNC;

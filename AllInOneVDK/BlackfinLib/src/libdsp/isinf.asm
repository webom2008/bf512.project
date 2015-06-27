/* Copyright (C) 2002 Analog Devices, Inc. All Rights Reserved. */
/*
** Check whether the supplied value is Infinity.
** Returns non-zero if so, zero if not.
*/

.file_attr libGroup      = math.h;
.file_attr libGroup      = math_bf.h;
.file_attr libFunc       = isinff;
.file_attr libFunc       = _isinf;
.file_attr libFunc       = isinf;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = _isinf;

.section program;
.align 2;
_isinf:
	BITCLR(R0, 31);		// Remove sign bit
	R1 = 0xFF;
	R1 <<= 23;		// R1 now +Inf.
	CC = R0 == R1;
	R0 = CC;
	RTS;
._isinf.end:
.global _isinf;
.type _isinf, STT_FUNC;


/* Copyright (C) 2002 Analog Devices, Inc. All Rights Reserved. */
/*
** Check whether the supplied value is Not A Number.
** Returns non-zero if so, zero if not.
*/

.file_attr libGroup      = math.h;
.file_attr libGroup      = math_bf.h;
.file_attr libFunc       = isnan;
.file_attr libFunc       = _isnan;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = _isnan;

.section program;
.align 2;
_isnan:
	BITCLR(R0, 31);		// Remove sign bit
	R1 = 0xFF;
	R1 <<= 23;		// R1 now +Inf.
	CC = R1 < R0;
	R0 = CC;
	RTS;
._isnan.end:
.global _isnan;
.type _isnan, STT_FUNC;


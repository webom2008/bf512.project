/*
** Normalise a float16.
** Copyright (C) 2003 Analog Devices, Inc. All Rights Reserved.
*/

.file_attr libGroup      = floating_point_support;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = _norm_fl16;
.file_attr FuncName      = _norm_fl16;

.section program;
.align 2;

_norm_fl16:
	R1 = R0;
	R1 >>>= 16;
	R3.L = SIGNBITS R0.H;
	R0.H = ASHIFT R0.H BY R3.L;
	R0.L = R0.L - R3.L (NS);
	CC = R1 == 0;
	IF CC R0 = R1;
	RTS;
._norm_fl16.end:
.global _norm_fl16;
.type _norm_fl16, STT_FUNC;


/*
** Convert fract 16 to float16
** Copyright (C) 2003 Analog Devices, Inc. All Rights Reserved.
*/

.file_attr libGroup      = float16.h;
.file_attr libFunc       = fr16_to_fl16;
.file_attr libFunc       = _fr16_to_fl16;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = _fr16_to_fl16;

.section program;
.align 2;

_fr16_to_fl16:
	CC = R0 == 0;
	IF CC JUMP finished;
	R1.L = SIGNBITS R0.L;
	R0.H = ASHIFT R0.L BY R1.L;
	R1 = -R1 (V);
	R0.L = R1.L >> 0;
finished:
	RTS;
._fr16_to_fl16.end:
.global _fr16_to_fl16;
.type _fr16_to_fl16, STT_FUNC;

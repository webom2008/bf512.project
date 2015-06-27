/*
** Whether a float will fit into a float16 without losing precision.
** Copyright (C) 2003 Analog Devices, Inc. All Rights Reserved.
*/

.file_attr libGroup      = float16.h;
.file_attr libFunc       = _fits_in_fl16;
.file_attr libFunc       = fits_in_fl16;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = _fits_in_fl16;

.section program;
.align 2;
_fits_in_fl16:
	R1 = R0.B (X);
	R0 = 0;
	CC = R1 == 0;
	R0 = ROT R0 BY 1;
	RTS;
._fits_in_fl16.end:
.global _fits_in_fl16;
.type _fits_in_fl16, STT_FUNC;

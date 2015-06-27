/*
** Convert non-ieee float16 to fract16. (high half R0 is a 16-bit fract; low half is the exponent)
** Copyright (C) 2003 Analog Devices, Inc. All Rights Reserved.
*/

.file_attr libGroup      = float16.h;
.file_attr libFunc       = fl16_to_fr16;
.file_attr libFunc       = _fl16_to_fr16;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = _fl16_to_fr16;

.section program;
.align 2;

_fl16_to_fr16:
	R1 = R0.L (X);			// put the exponent into R1
	R0 >>= 16;			// move the fract value into the result half of the register
	R2 = -16;			// clip to range of lshift
	R1 = MAX(R1,R2);
	R2 = 16;			// ashift may see 16 as -16; hence zero or max int is returned (float is too large for the fract - undefined behaviour)
	R1 = MIN(R1,R2);
	R0 = ASHIFT R0 BY R1.L (V);	// shift the fract as appropriate
	RTS;
._fl16_to_fr16.end:
.global _fl16_to_fr16;
.type _fl16_to_fr16, STT_FUNC;

/*
** Convert a 32-bit signed integer into a non-IEEE 32-bit floating point.
** Copyright (C) 2004 Analog Devices, Inc. All Rights Reserved.
*/

.file_attr libGroup      = floating_point_support;
.file_attr libGroup      = integer_support;
.file_attr libName = libf64fast;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___int32_to_float32;
.file_attr FuncName      = ___int32_to_float32;

.section program;
.align 2;

___int32_to_float32:
	R2 = 31;
	R1.L = SIGNBITS R0;
	R1 = R1.L (X);
	R0 <<= R1;
	R0.L = R2.L - R1.L (S);
	RTS;
.___int32_to_float32.end:
.global ___int32_to_float32;
.type ___int32_to_float32, STT_FUNC;

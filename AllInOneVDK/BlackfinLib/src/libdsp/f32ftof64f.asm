/*
** Copyright (C) 2004-2009 Analog Devices, Inc. All Rights Reserved.
**
** Convert non-IEEE 32-bit floating-point to non-IEEE 64-bit
** floating-point.
*/

.file_attr libGroup      = floating_point_support;
.file_attr libName = libf64fast;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___float32_to_float64;
.file_attr FuncName      = ___float32_to_float64;

#include <sys/anomaly_macros_rtl.h>

.section program;
.align 2;

___float32_to_float64:
	/* Exponent is a signed integer, so extend it to the high half. */
	R1 = R0.L (X);
	/* Mantissa is a signed fraction, so extend it to the low half. */
	R0.L = 0;
#if WA_05000371
	NOP;
	NOP;
#endif
	RTS;
.___float32_to_float64.end:
.global ___float32_to_float64;
.type ___float32_to_float64, STT_FUNC;


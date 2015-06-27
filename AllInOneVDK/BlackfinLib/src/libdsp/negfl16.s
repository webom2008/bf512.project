/*
** Float16 negate
** Copyright (C) 2003-2009 Analog Devices, Inc. All Rights Reserved.
*/

.file_attr libGroup      = floating_point_support;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = _negate_fl16;
.file_attr FuncName      = _negate_fl16;

#include <sys/anomaly_macros_rtl.h>

.section program;
.align 2;
_negate_fl16:
	R1 = 0;
	R0.H = R1.H - R0.H (S);
#if WA_05000371
	NOP;
	NOP;
#endif
	RTS;
._negate_fl16.end:
.global _negate_fl16;
.type _negate_fl16, STT_FUNC;

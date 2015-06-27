// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2009 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=norm_l;
.file_attr FuncName=_norm_l;
.file_attr libFunc=_norm_l;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#include <sys/anomaly_macros_rtl.h>

	.section program;

.epctext:

	.align 2;
_norm_l:
		CC =  R0 ==  0;
		IF !CC JUMP non_zero;
#if WA_05000371
		NOP;
		NOP;
#endif
		RTS;
non_zero:
		R1.L = SIGNBITS R0;
		R0 = R1.L (X);
		RTS;
._norm_l.end:
	.global _norm_l;
	.type _norm_l,STT_FUNC;

.epctext.end:

	.section data1;


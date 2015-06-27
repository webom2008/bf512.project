// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2009 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=L_msu;
.file_attr FuncName=_L_msu;
.file_attr libFunc=_L_msu;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#include <sys/anomaly_macros_rtl.h>

	.section program;
.epctext:
	.align 2;
_L_msu:
	A0 = R0 ;
	A0-=R2.L*R1.L (W32);
	R0 = A0;
#if __SET_ETSI_FLAGS
	CC = AV0;
	IF !CC jump no_overflow (bp);
	I0.L = _Overflow;
	I0.H = _Overflow;
	R1 = 1;
	[I0] = R1;
no_overflow:
#else
#if WA_05000371
	NOP;
#endif
#endif /* __SET_ETSI_FLAGS */
	RTS;
._L_msu.end:
	.global _L_msu;
	.type _L_msu,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
#endif
.epctext.end:
	.section data1;


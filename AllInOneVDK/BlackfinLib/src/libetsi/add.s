// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2009 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=add;
.file_attr FuncName=_add;
.file_attr libFunc=_add;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#include <sys/anomaly_macros_rtl.h>

        .section program;
.epctext:
        .align 2;
_add:
	R0.L = R0.L + R1.L (S);
#if __SET_ETSI_FLAGS
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC=V;
#endif
#endif
	R0 = R0.L (X);
#if __SET_ETSI_FLAGS
	IF !CC JUMP no_overflow (bp);
	I0.L = _Overflow;
	I0.H = _Overflow;
	R1 = 0x1;
	[I0] = R1;
no_overflow:
#else
#if WA_05000371
	NOP;
	NOP;
#endif
#endif /* __SET_ETSI_FLAGS */
	RTS;
._add.end:
        .global _add;
        .type _add,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
#endif
.epctext.end:
        .section data1;

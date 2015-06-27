// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2009 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=L_add;
.file_attr FuncName=_L_add;
.file_attr libFunc=_L_add;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#include <sys/anomaly_macros_rtl.h>

        .section program;
.epctext:
        .align 2;
_L_add:
	R0 = R0 + R1 (S);
#if __SET_ETSI_FLAGS
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC=V;
#endif
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
	NOP;
#endif
#endif /* __SET_ETSI_FLAGS */
	RTS;
._L_add.end:
        .global _L_add;
        .type _L_add,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
#endif
.epctext.end:
        .section data1;

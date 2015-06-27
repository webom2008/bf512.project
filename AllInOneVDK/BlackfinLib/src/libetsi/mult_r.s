// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2004 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=mult_r;
.file_attr FuncName=_mult_r;
.file_attr libFunc=_mult_r;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#if defined(__ADSPLPBLACKFIN__)
#include <def_LPBlackfin.h>
#else
#include <defblackfin.h>
#endif
	.section program;

.epctext:

	.align 2;
_mult_r:
	R2 = ASTAT;
	R3 = R2;
	BITSET(R2, ASTAT_RND_MOD_P);
	ASTAT = R2;
#if !defined(__ADSPLPBLACKFIN__)
	NOP;	// There is a latency in the rounding mode
	NOP;	// taking effect on BF535
#endif
	R0.L = R0.L*R1.L;
#if __SET_ETSI_FLAGS
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC=V;
#endif
	IF !CC JUMP no_overflow (BP);
	I0.L = _Overflow;
	I0.H = _Overflow;
	R1 = 1;
	[I0] = R1;
no_overflow:
#endif
	ASTAT = R3;
	R0 = R0.L (X);
	RTS;

._mult_r.end:
	.global _mult_r;
	.type _mult_r,STT_FUNC;
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
.epctext.end:

	.section data1;


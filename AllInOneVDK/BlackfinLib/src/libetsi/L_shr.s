// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2004 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=L_shr;
.file_attr FuncName=_L_shr;
.file_attr libFunc=_L_shr;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
	.section program;
.epctext:
	.align 2;
_L_shr:
	R1 = -R1;
#if __SET_ETSI_FLAGS
	// Need to force the overflow value to be set
	// in the case of left shifting 0xFFFFFFFF by
	// 32 or more bits.
	CC = R0 == -1;
	R3 = 0;
	IF !CC JUMP normal (bp);
	R2 = 0x20;
	CC = R2 <= R1;
	IF !CC JUMP normal (bp);
	R3 = 1;
normal:
#endif
	R2 = 0x1F;
	R1 = MIN(R1,R2);
	R2 = ~R2;
	R1 = MAX(R1,R2);
	R0 = ASHIFT R0 BY R1.L (S);
#if __SET_ETSI_FLAGS
	CC = R3;
#if !defined(__ADSPLPBLACKFIN__)
	CC |= AV0;
#else
	CC |= V;
#endif
	IF !CC JUMP no_overflow (BP);
	I0.L = _Overflow;
	I0.H = _Overflow;
	R1 = 1;
	[I0] = R1;
no_overflow:
#endif
	RTS;
._L_shr.end:
	.global _L_shr;
	.type _L_shr,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
#endif
.epctext.end:
	.section data1;

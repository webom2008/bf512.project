// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2004 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=round;
.file_attr FuncName=_round;
.file_attr libFunc=_round;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
        .section program;
.epctext:
        .align 2;
_round:
	R1 = 0;
	BITSET(R1,15);
	R0 = R0 + R1 (S);
#if __SET_ETSI_FLAGS
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC = V;
#endif
	IF !CC JUMP no_overflow1 (bp);
	I0.L = _Overflow;
	I0.H = _Overflow;
	R1 = 0x1;
	[I0] = R1;
no_overflow1:
#endif
	R0 = R0 >>> 16;
#if __SET_ETSI_FLAGS
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC = V;
#endif
	IF !CC JUMP no_overflow2 (bp);
	I0.L = _Overflow;
	I0.H = _Overflow;
	R1 = 0x1;
	[I0] = R1;
no_overflow2:
#endif
	R0 = R0.L (X);
	RTS;
._round.end:
        .global _round;
        .type _round,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
#endif
.epctext.end:
        .section data1;

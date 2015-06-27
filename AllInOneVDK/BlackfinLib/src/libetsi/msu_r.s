// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2004 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=msu_r;
.file_attr FuncName=_msu_r;
.file_attr libFunc=_msu_r;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
	.section program;
.epctext:
	.align 2;
_msu_r:
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
#endif
	R1 = 0;
	BITSET(R1,15);
	R0 = R0 + R1 (S);
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
	R0 = R0 >>> 16;
	RTS;
._msu_r.end:
	.global _msu_r;
	.type _msu_r,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
#endif
.epctext.end:
	.section data1;


// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2008 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=L_Extract;
.file_attr FuncName=_L_Extract;
.file_attr libFunc=_L_Extract;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
	.section program;
.epctext:
	.align 2;
_L_Extract:
	P1 = R1 ;
	P0 = R2 ;
	R1 = R0 >>> 1 (S);
	R2.L =  16384;
	A0 = R1 || W[P1] = R0.H || NOP;
	R0 = (A0-=R2.L*R0.H);
#if __SET_ETSI_FLAGS
#if !defined(__ADSPLPBLACKFIN__)
	CC=AV0;
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
	W[P0+ 0] = R0;
	RTS;
._L_Extract.end:
	.global _L_Extract;
	.type _L_Extract,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
#endif
.epctext.end:
	.section data1;

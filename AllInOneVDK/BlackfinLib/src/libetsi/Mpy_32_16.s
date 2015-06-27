// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=Mpy_32_16;
.file_attr FuncName=_Mpy_32_16;
.file_attr libFunc=_Mpy_32_16;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
	.section program;
.epctext:
	.align 2;
_Mpy_32_16:
	R3 = 1;
	R1.L = R1.L*R2.L (T);
#if __SET_ETSI_FLAGS	
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC=V;
#endif
	IF !CC JUMP no_overflow1 (BP);
	I0.L = _Overflow;
	I0.H = _Overflow;
	[I0] = R3;
no_overflow1:
#endif
	A0=R1.L*R3.L (W32);
#if __SET_ETSI_FLAGS	
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC=V;
#endif
	IF !CC JUMP no_overflow2 (BP);
	I0.L = _Overflow;
	I0.H = _Overflow;
	[I0] = R3;
no_overflow2:
#endif
	R0=(A0+=R2.L*R0.L);
#if __SET_ETSI_FLAGS	
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC=V;
#endif
	IF !CC JUMP no_overflow3 (BP);
	I0.L = _Overflow;
	I0.H = _Overflow;
	[I0] = R3;
no_overflow3:
#endif
	RTS;
._Mpy_32_16.end:
	.global _Mpy_32_16;
	.type _Mpy_32_16,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow, STT_OBJECT;
#endif
.epctext.end:
	.section data1;

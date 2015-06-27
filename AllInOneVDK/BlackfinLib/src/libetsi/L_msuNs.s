// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=L_msuNs;
.file_attr FuncName=_L_msuNs;
.file_attr libFunc=_L_msuNs;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
        .section program;
.epctext:
        .align 2;
_L_msuNs:
#if __SET_ETSI_FLAGS
	R1 = R1.L * R2.L;
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC=V;
#endif
	IF !CC JUMP no_overflow (bp);
	R3 = 1;
	I0.L = _Overflow;
	I0.H = _Overflow;
	[I0] = R3;
no_overflow:
	[--SP]=RETS;
	CALL.X _L_sub_c;
	RETS=[SP++];
#else
	R1 = R1.L * R2.L;
	R0 = R0 - R1 (NS);
	R3 = 1;
	R0 = R0 - R3 (NS);
#endif
	RTS;
._L_msuNs.end:
        .global _L_msuNs;
        .type _L_msuNs,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
	.extern _L_sub_c;
	.type _L_sub_c,STT_FUNC;
#endif
.epctext.end:
        .section data1;

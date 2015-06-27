// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=shl;
.file_attr FuncName=_shl;
.file_attr libFunc=_shl;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
	.section program;
.epctext:
	.align 2;
_shl:
	R2 = 0x1F;
	R1 = MIN(R1,R2);
	R2 = ~R2;
	R1 = MAX(R1,R2);
	// Use 32 bit shift to allow shifting by +16
	R0 <<= 16;
	R0 = ASHIFT R0 BY R1.L (S);
#if __SET_ETSI_FLAGS
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC=V;
#endif
#endif
	R0 >>>= 16;
#if __SET_ETSI_FLAGS
	IF !CC JUMP no_overflow (bp);
	I0.L = _Overflow;
	I0.H = _Overflow;
	R1 = 1;
	[I0] = R1;
no_overflow:
#endif
	RTS;

._shl.end:
	.global _shl;
	.type _shl,STT_FUNC;
#if __SET_ETSI_FLAGS
        .extern _Overflow;
        .type _Overflow,STT_OBJECT;
#endif
.epctext.end:

	.section data1;


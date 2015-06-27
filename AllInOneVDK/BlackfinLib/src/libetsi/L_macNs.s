// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2009 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=L_macNs;
.file_attr FuncName=_L_macNs;
.file_attr libFunc=_L_macNs;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#include <sys/anomaly_macros_rtl.h>

#if defined(__ADSPLPBLACKFIN__)
#include <def_LPBlackfin.h>
#else
#include <defblackfin.h>
#endif
        .section program;
.epctext:
        .align 2;
_L_macNs:
#if __SET_ETSI_FLAGS
	R1 = R1.L * R2.L;
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC = V;
#endif
	I0.L = _Carry;
	I0.H = _Carry;
	R0 = R0 + R1 (NS);
	AV0 |= CC;
	R3 = ASTAT;
	R2 = [I0];
	CC = R2 == 0;
	IF !CC JUMP no_carry_input;
	R0 = R0 + R2 (NS);
	R2 = ASTAT; 
	R3 = R3 | R2;
no_carry_input:
	R1 = 1;
#if !defined(__ADSPLPBLACKFIN__)
	CC = BITTST (R3, ASTAT_AV0_P);
#else
	CC = BITTST (R3, ASTAT_V_P);
#endif
	IF !CC JUMP no_overflow;
	I1.L = _Overflow;
	I1.H = _Overflow;
	[I1] = R1;
no_overflow:
#if !defined(__ADSPLPBLACKFIN__)
	CC = BITTST (R3, ASTAT_AC_P);
#else
	CC = BITTST (R3, ASTAT_AC0_P);
#endif
	IF !CC JUMP no_carry;
	[I0] = R1;
#else
#if WA_05000371
	NOP;
	NOP;
#endif
	R1 = R1.L * R2.L;
	R0 = R0 + R1 (NS);
#endif
no_carry:
	RTS;

._L_macNs.end:
        .global _L_macNs;
        .type _L_macNs,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
	.extern _Carry;
	.type _Carry,STT_OBJECT;
#endif
.epctext.end:

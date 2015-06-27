// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2009 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=L_add_c;
.file_attr FuncName=_L_add_c;
.file_attr libFunc=_L_add_c;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#if defined(__ADSPLPBLACKFIN__)
#include <def_LPBlackfin.h>
#else
#include <defblackfin.h>
#endif

#include <sys/anomaly_macros_rtl.h>

        .section program;
.epctext:
        .align 2;
_L_add_c:
#if __SET_ETSI_FLAGS
	I0.L = _Carry;
	I0.H = _Carry;
	R2 = [I0];
	CC = R2 == 0;
	R0 = R0 + R1 (NS);
	R3 = ASTAT;
	IF !CC JUMP no_carry_input;
	R0 = R0 + R2 (NS);
	R2 = ASTAT; 
	R3 = R3 | R2; // Combined flags from both additions.
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
	R0 = R0 + R1 (NS);
#if WA_05000371
	NOP;	
	NOP;	
	NOP;	
#endif
#endif
no_carry:
	RTS;

._L_add_c.end:
        .global _L_add_c;
        .type _L_add_c,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
	.extern _Carry;
	.type _Carry,STT_OBJECT;
#endif
.epctext.end:

// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2004 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=div_s;
.file_attr FuncName=_div_s;
.file_attr libFunc=_div_s;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
        .section program;
.epctext:
        .align 2;
_div_s:
#if __ETSI_DIV_CHECKS
	// These checks can be avoided by not defining the macro.
	// Not checking the parameters put the requirement for correct
	// parameters on the calling function.
	
	// Initial checks:
	// R0 > R1, R0 < 0, R1 < 0
	CC = R0.L > R1.L;
	IF CC JUMP divs_check_failure (NP);
	CC = R0.L < 0;
	IF CC JUMP divs_check_failure (NP);
	CC = R1.L < 0;
	IF !CC JUMP divs_check_passed (BP);
divs_check_failure:
	CALL.X _abort;
divs_check_passed:
	CC = R0.L == 0;
	IF !CC JUMP not_zer_return (BP);
	R0.L = 0;
	RTS;
not_zero_return:
	// If R0 = R1 we return MAX 16 - this is the closes representation to
	// fract 1 that we can achieve.
	CC R0.L == R1.L
	IF !CC JUMP true_division (BP);
	R0.L = 0x7FFF;
	RTS;
true_division:
#endif
	// here we start the real division. If __ETSI_DIV_CHECKS is not
	// defined then this is our entry point, ignoring special cases.
	R2 = R0.L (X);		// L_num   = L_deposit_l
	R3 = R1.L (X);		// L_denom = L_deposit_l
	R0 = 0;
#if __SET_ETSI_FLAGS
	// Load I0 with _Overflow outside the loop
	I0.L = _Overflow;
	I0.H = _Overflow;
#endif
	R1 = 1;
	P0 = 15;
	LSETUP(div_start,div_end) LC0=P0;
div_start:
	R2 <<= 1;	// L_num   <<= 1
	R0 <<= 1;	// var_out <<= 1
	CC = R3 <= R2;	// L_Demon < L_num
	IF !CC JUMP skip_sub_add;	// BP is risky here.
	// if (L_n > l_d )
	R2 = R2 - R3 (S);
#if __SET_ETSI_FLAGS
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC = V;
#endif
	IF !CC JUMP skip_sub_flags (BP);
	[I0] = R1;
skip_sub_flags:
#endif
	R0 += 1;
#if __SET_ETSI_FLAGS
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC = V;
#endif
	IF !CC JUMP skip_add_flags (BP);
	[I0] = R1;
skip_add_flags:
#endif
skip_sub_add:
div_end:
	NOP;
	R0 = R0.L (X) ;
	RTS;
._div_s.end:
        .global _div_s;
        .type _div_s,STT_FUNC;
#if __ETSI_DIV_CHECKS
	.extern _abort;
	.type _abort,ST_FUNC;
#endif
#if __SET_ETSI_FLAGS
        .extern _Overflow;
        .type _Overflow,STT_OBJECT;
#endif
.epctext.end:
        .section data1;



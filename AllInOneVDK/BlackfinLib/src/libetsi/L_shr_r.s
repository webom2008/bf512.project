// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2004 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=L_shr_r;
.file_attr FuncName=_L_shr_r;
.file_attr libFunc=_L_shr_r;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
	.section program;
.epctext:
	.align 2;
_L_shr_r:
	R2 = 31;
	CC = R2 < R1;
	IF !CC JUMP not_toobig (BP);
	R0 = 0;
	RTS;
not_toobig:
	R2 = -R1;
	R3 = 31;
	R2 = MIN(R2,R3);
	R2 = ASHIFT R0 BY R2.L (S);
#if __SET_ETSI_FLAGS
	// Need to force the overflow value to be set
	// in the case of left shifting 0xFFFFFFFF by
	// 32 or more bits.
	CC = R0 == -1;
	R3 = 0;
	IF !CC JUMP normal (bp);
	R3 = -32;
	CC = R1 <= R3;
	R3 = 0;
	IF !CC JUMP ov0 (bp);
	R3 = 1;
ov0:
	CC = R3;
normal:
#if !defined(__ADSPLPBLACKFIN__)
	CC |= AV0;
#else
	CC |= V;
#endif 
	IF !CC JUMP no_overflow (BP);
	I0.L = _Overflow;
	I0.H = _Overflow;
	R3 = 1;
	[I0] = R3;
no_overflow:
#endif
        // Check to see if the last bit shifted off the end was set.
        // If so, we need to round up our result, by incrementing by 1.
	CC = R1 <= 0 ;
	IF !CC JUMP check_carry;
	R0 = R2;
	RTS;
check_carry:
	R3 = 1;
	R1 = R1 - R3;
	R1 = -R1;
	R3 = 0x1F;
	R1 = MIN(R1,R3);
	R0 = ASHIFT R0 BY R1.L (S);
	R3 = 1;
	R0 = R0 & R3;
	R0 = R0 + R2;
	RTS;
._L_shr_r.end:
	.global _L_shr_r;
	.type _L_shr_r,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
#endif
.epctext.end:
	.section data1;

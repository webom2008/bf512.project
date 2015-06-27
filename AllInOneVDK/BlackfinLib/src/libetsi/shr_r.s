// **************************************************************************
//
// Analog Devices Blackfin ETSI Implementation.
//    Copyright (C). 2006 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=shr_r;
.file_attr FuncName=_shr_r;
.file_attr libFunc=_shr_r;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

	.section program;
	.align 2;

_shr_r:
	R2 = 0x10;
	CC = R1 <= 0;

	IF CC JUMP leftshift;

	// Clip shift magnitude to -16..0
	R1 = MIN(R1,R2);
	R1 = -R1;

	// Perform a right shift. If last bit shifted
	// off to right was set, perform rounding.
	// R0 sign extended by caller, so ROT here fills
	// up to 16 LSBs with sign bits.
	R2 = ROT R0 BY R1.L;

	// Calculate rounded value
	R1 = 1;
	R1.L = R1.L + R2.L (S);

	// Return rounded value if last bit rotated out was set
	IF CC R2 = R1;
	R0 = R2.L (X);
	RTS;	

	// Perform saturating left shift. Support shift magnitude
	// of +16 by performing 32-bit shift 
leftshift:
	R1 = -R1;
	R1 = MIN(R1,R2);
	R0 <<= 0x10;
	R0 = ASHIFT R0 BY R1.L (S);
#if __SET_ETSI_FLAGS
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC=V;
#endif
#endif
	R0 >>>= 0x10;
#if __SET_ETSI_FLAGS
   IF CC JUMP overflow; 
	RTS;

overflow:
   I0.L = _Overflow;
   I0.H = _Overflow;
   R3 = 1;
   [I0] = R3;
#endif
	RTS;

._shr_r.end:

	.global _shr_r;
	.type _shr_r,STT_FUNC;

#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
#endif

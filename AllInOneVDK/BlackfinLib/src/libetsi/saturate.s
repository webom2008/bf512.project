// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=saturate;
.file_attr FuncName=_saturate;
.file_attr libFunc=_saturate;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
        .section program;
.epctext:
        .align 2;
_saturate:
		R1 = 0x00007FFF;
		CC = R1 <= R0;		// if L_var1 > 0x7FFFL
		IF CC JUMP maxval;
		R1 = 0xFFFF8000;
		CC = R0 < R1;		// if L_var1 < 0xFFFF8000
		IF CC JUMP minval;
					// Extract the low half and return
		R0 = R0.L (X);
		RTS;
maxval:
		R0 = 0x7FFF;
#if __SET_ETSI_FLAGS
		JUMP setflag;
#else
		RTS;
#endif
minval:
		R0 = 0xFFFF8000;
#if __SET_ETSI_FLAGS
setflag:
		I0.L = _Overflow;
		I0.H = _Overflow;
		R1 = 0x1;
		[I0] = R1;
#endif
		RTS;
._saturate.end:
        .global _saturate;
        .type _saturate,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
#endif
.epctext.end:
        .section data1;

// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2009 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=L_sat;
.file_attr FuncName=_L_sat;
.file_attr libFunc=_L_sat;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#include <sys/anomaly_macros_rtl.h>

        .section program;
.epctext:
        .align 2;
_L_sat:
	R0 = R0;
#if __SET_ETSI_FLAGS
	I0.L = _Overflow;
	I0.H = _Overflow;
	R1 = [I0];
	CC = R1;
	IF !CC JUMP no_overflow (BP);
	R1 = 0;
	[I0] = R1;
	I0.L = _Carry;
	I0.H = _Carry;
	R1 = [I0];
no_overflow:
#else
#if WA_05000371
	NOP;
	NOP;
	NOP;
#endif
#endif /*__SET_ETSI_FLAGS */
		RTS;
._L_sat.end:
        .global _L_sat;
        .type _L_sat,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
	.extern _Carry;
	.type _Carry,STT_OBJECT;
#endif
.epctext.end:
        .section data1;

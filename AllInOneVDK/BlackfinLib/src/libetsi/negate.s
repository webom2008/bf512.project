// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2009 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=negate;
.file_attr FuncName=_negate;
.file_attr libFunc=_negate;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#include <sys/anomaly_macros_rtl.h>

	.section program;
.epctext:
	.align 2;
_negate:
		R0 = -R0 (V);
		R0 = R0.L (X);
#if WA_05000371
		NOP;
		NOP;
#endif
		RTS;
._negate.end:
	.global _negate;
	.type _negate,STT_FUNC;
.epctext.end:
	.section data1;


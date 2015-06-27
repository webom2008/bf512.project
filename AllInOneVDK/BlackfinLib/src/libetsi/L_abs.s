// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2009 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=L_abs;
.file_attr FuncName=_L_abs;
.file_attr libFunc=_L_abs;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#include <sys/anomaly_macros_rtl.h>

	.section program;
.epctext:
	.align 2;
_L_abs:
		R0 = ABS R0;
#if WA_05000371
		NOP;
		NOP;
		NOP;
#endif
		RTS;
._L_abs.end:
	.global _L_abs;
	.type _L_abs,STT_FUNC;

.epctext.end:

	.section data1;


// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2002-2009 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=extract_l;
.file_attr FuncName=_extract_l;
.file_attr libFunc=_extract_l;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#include <sys/anomaly_macros_rtl.h>

	.section program;

.epctext:

	.align 2;
_extract_l:
		R0 = R0.L (X);
#if WA_05000371
		NOP;
		NOP;
		NOP;
#endif
		RTS;

._extract_l.end:
	.global _extract_l;
	.type _extract_l,STT_FUNC;

.epctext.end:

	.section data1;


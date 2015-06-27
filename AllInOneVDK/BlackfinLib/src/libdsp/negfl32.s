/*
** Float32 negate
*/
.file_attr libGroup      = floating_point_support;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = _negate_fl32;
.file_attr FuncName      = _negate_fl32;

#include <sys/anomaly_macros_rtl.h>

.section program;
.align 2;
_negate_fl32:
	R1 = -R1;
#if WA_05000371
	NOP;
	NOP;
	NOP;
#endif
	RTS;
._negate_fl32.end:
.global _negate_fl32;
.type _negate_fl32, STT_FUNC;

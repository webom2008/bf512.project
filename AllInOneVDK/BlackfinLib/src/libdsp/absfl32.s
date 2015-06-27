/*
** Float32 abs
*/

.file_attr libGroup      = floating_point_support;
.file_attr libFunc       = _abs_fl32;
.file_attr libFunc       = abs_fl32;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = _abs_fl32;

#include <sys/anomaly_macros_rtl.h>

.section program;
.align 2;
_abs_fl32:
	R1 = ABS R1;
#if WA_05000371
	NOP;
	NOP;
	NOP;
#endif
	RTS;
._abs_fl32.end:
.global _abs_fl32;
.type _abs_fl32, STT_FUNC;

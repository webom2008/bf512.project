/*
** Convert Float32 to fract32.  This is a 64-bit float made up of a 32-bit exponent and 32-bit fractional mantissa
*/

.file_attr libGroup      = floating_point_support;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = _fl32_to_fr32;
.file_attr FuncName      = _fl32_to_fr32;

.section program;
.align 2;

_fl32_to_fr32:
	R2 = -32;
	R0 = MAX(R0,R2);
	R2 = 32;
	R0 = MIN(R0,R2);
	R0 = ASHIFT R1 BY R0.L;
	RTS;
._fl32_to_fr32.end:
.global _fl32_to_fr32;
.type _fl32_to_fr32, STT_FUNC;

/*
** Copyright (C) 2005 Analog Devices, Inc. All Rights Reserved.
**
** This is the internal function implementing IEEE single-precision
** floating-point equality comparison. This functions is for compiler
** internal use only. It does not follow the normal C ABI.
**
** int __float32_adi_eq(float X, float Y)
**
**   X is received in R0.
**   Y is received in R1.
**   CC contains the result and is 1 if neither input is a NaN and X==Y,
**   or 0 otherwise.
*/

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12fcmp ="
%notes "Single-precision floating-point equality comparison."
%regs_clobbered "R0-R3,P1,CCset"
%const
%return_in_cc
%syscall ___float32_adi_eq(I=FF)
#endif

.file_attr libGroup      = floating_point_support;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___float32_adi_eq;
.file_attr FuncName      = ___float32_adi_eq;

.section program;
.align 2;

___float32_adi_eq:

	// A NaN has an exponent of 255, and a non-zero
	// mantissa. Sign is irrelevant. We check whether
	// either input is a NaN by getting rid of the
	// sign bit, and then comparing against 0xFF000000;
	// if the operand is larger, it's got a 255 exponent
	// and non-zero mantissa, hence it's a NaN.
   R2 = R0 << 1;
   R3 = 0xFF;
   R3 <<= 24;
   CC = R2 <= R3 (IU);
   IF !CC JUMP .ret;			// We have a Nan, return 0

	CC = R0 == R1;				// X is not a NaN, check equality
	
	// If CC==1 we already have the result, but quicker to check
	// (-0==0) regardless rather than branch to a return 
	R2 = R0 | R1;
	R2 <<= 1;
	CC |= AZ;					// If AZ then -0==+0 
.ret:
	RTS;

.___float32_adi_eq.end:
.type ___float32_adi_eq, STT_FUNC;
.global ___float32_adi_eq;

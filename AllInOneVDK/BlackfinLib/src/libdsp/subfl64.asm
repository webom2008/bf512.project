/*
** Copyright (C) 2003-2006 Analog Devices, Inc. All Rights Reserved.
**
** 64-bit IEEE floating-point subtraction.
**
** This function implements subtraction for the long double type.
** long double __float64_sub(long double X, long double Y);
** X is passed in R1:0, Y is passed in R2 and on the stack.
** The result is returned in R1:0.
*/

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12f-"
%notes "Double-precision floating-point subtraction."
/* Changing this register set also affects the #pragma regs_clobbered in */
/* softfloat */
%regs_clobbered "R0-R3,P0-P2,A0,A1,I0-I3,M0-M2,CCset"
%const
%syscall ___float64_sub(lD=lDlD)
#endif

.file_attr libGroup      = floating_point_support;
.file_attr libName = libf64ieee;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___float64_sub;
.file_attr FuncName      = ___float64_sub;

.section program;
.align 2;

___float64_sub:
	R3 = [SP+12];
	BITTGL(R3, 31);
	JUMP.X ___float64_add_inregs;
.___float64_sub.end:

.global ___float64_sub;
.type ___float64_sub, STT_FUNC;
.extern ___float64_add_inregs;
.type ___float64_add_inregs, STT_FUNC;

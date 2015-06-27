/************************************************************************
 *
 * normfl32.s : $Revision: 1.4 $
 *
 * (c) Copyright 2000-2003 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
** Normalise a 32-bit float.
*/

.file_attr libGroup      = floating_point_support;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = _norm_fl32;
.file_attr FuncName      = _norm_fl32;

.section program;

.align 2;
_norm_fl32:
   R2.L = SIGNBITS R1;
   R2 = R2.L (X);
   R1 <<= R2;
   R0 = R0 - R1;
   RTS;
._norm_fl32.end:
.global _norm_fl32;
.type _norm_fl32, STT_FUNC;

// end of file

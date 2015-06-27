/************************************************************************
 *
 * fr322fl32.s : $Revision: 1.4 $
 *
 * (c) Copyright 2000-2003 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
** fract32 to 32-bit float.
*/

.file_attr libGroup      = floating_point_support;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = _fr32_to_fl32;
.file_attr FuncName      = _fr32_to_fl32;

.section program;

.align 2;
_fr32_to_fl32:
   R1 = R0;
   CC = R0 == 0;
   IF CC JUMP finished;
   R0.L = SIGNBITS R1;
   R0 = R0.L (X);
   R1 <<= R0;
   R0 = -R0;
finished:
   RTS;

._fr32_to_fl32.end:
.global _fr32_to_fl32;
.type _fr32_to_fl32, STT_FUNC;


// end of file

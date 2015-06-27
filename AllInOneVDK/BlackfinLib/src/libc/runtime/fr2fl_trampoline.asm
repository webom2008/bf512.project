/*
 * Trampoline for legacy purposes to ___fr16_to_float, which used to be
 * called _fr16_to_float.
 *
 * (c) Copyright 2010-2011 Analog Devices Inc. All rights reserved.
 */

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.file_attr libGroup = floating_point_support;
.file_attr libGroup = integer_support;
.file_attr libName  = librt;
.file_attr libName  = librt_fileio;

.file_attr libFunc  = fr16_to_float;
.file_attr libFunc  = _fr16_to_float;
.file_attr FuncName = _fr16_to_float;

.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";

#endif

.section program;
        .align 2;

        .extern ___fr16_to_float
        .global _fr16_to_float

_fr16_to_float:

        JUMP.X ___fr16_to_float;

._fr16_to_float.end:

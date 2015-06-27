/*
 * Trampoline for legacy purposes to ___float_to_fr32, which used to be
 * called _float_to_fr32.
 *
 * (c) Copyright 2010-2011 Analog Devices Inc. All rights reserved.
 */

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.file_attr libGroup = floating_point_support;
.file_attr libGroup = integer_support;
.file_attr libName  = librt;
.file_attr libName  = librt_fileio;

.file_attr libFunc  = float_to_fr32;
.file_attr libFunc  = _float_to_fr32;
.file_attr FuncName = _float_to_fr32;

.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";

#endif

.section program;
        .align 2;

        .extern ___float_to_fr32
        .global _float_to_fr32

_float_to_fr32:

        JUMP.X ___float_to_fr32;

._float_to_fr32.end:

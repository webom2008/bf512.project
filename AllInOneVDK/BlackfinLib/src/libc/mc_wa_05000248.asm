/*
 * Defines a variables used in the tools workaround for ADSP-BF561 silicon
 * anomaly 05000248.
 *
 * (c) Copyright 2010 Analog Devices Inc. All rights reserved.
 */

#if defined(__ADSPBF561__)
.file_attr libName="libc";
.file_attr prefersMem="any";
.file_attr prefersMemNum="50";
.file_attr Content="ZeroData";

.section/DOUBLEANY L2_sram;
.align 4;
.byte ___var_wa_05000248[4];
.global ___var_wa_05000248;
.type ___var_wa_05000248,STT_OBJECT;
.set ___var_wa_06000047, ___var_wa_05000248;
#endif

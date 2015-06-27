/*
** Copyright (C) 2008 Analog Devices, Inc. All Rights Reserved.
** Logical left shift, on unsigned long long.
**
** unsigned long long __lshftli (unsigned long long ll1, int i1);
**
** !!NOTE- The 64-bit division operations call this function without allocating
**         parameter stack space.
*/

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12sx lshift"
%notes "Logical left shift on 64-bit integer."
%regs_clobbered "R0-R3,P1,CCset"
%const
%syscall ___lshftli(lL=lLI)
#endif

.FILE_ATTR FuncName      = ___lshftli;
.FILE_ATTR libName       = libdsp;
.FILE_ATTR libGroup      = integer_support;
.FILE_ATTR libFunc       = ___lshftli;
.FILE_ATTR prefersMem    = internal;
.FILE_ATTR prefersMemNum = "30";

.GLOBAL ___lshftli;
.TYPE ___lshftli, STT_FUNC;

.SECTION program;
.ALIGN 2;

___lshftli:
      CC = R2 < 0;
      IF CC JUMP .negative_magnitude;

.positive_magnitude:
#ifndef __WORKAROUND_SHIFT
      R3 = 64;
      R3 = MIN (R2, R3);
      R3 += -32;
      R3 = LSHIFT R0 BY R3.L;
      R0 <<= R2;
      R1 <<= R2;
      R1 = R1 | R3;
#else
      R3 = 64;
      R2 = MIN (R2, R3);
      R2 += -32;
      R3 = LSHIFT R0 BY R2.L;
      R2 += 32;
      R0 = LSHIFT R0 BY R2.L;
      R1 = LSHIFT R1 BY R2.L;
      R1 = R1 | R3;
#endif
      RTS;

.negative_magnitude:
#ifndef __WORKAROUND_SHIFT
      R3 = -64;
      R3 = MAX (R2, R3);
      R3 += 32;
      R3 = LSHIFT R1 BY R3.L;
      R2 = -R2;
      R1 >>= R2;
      R0 >>= R2;
      R0 = R0 | R3;
#else
      R3 = -64;
      R2 = MAX (R2, R3);
      R2 += 32;
      R3 = LSHIFT R1 BY R2.L;
      R2 += -32;
      R1 = LSHIFT R1 BY R2.L;
      R0 = LSHIFT R0 BY R2.L;
      R0 = R0 | R3;
#endif
      RTS;

.___lshftli.end:

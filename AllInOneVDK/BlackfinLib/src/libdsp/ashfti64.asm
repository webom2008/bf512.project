/*
** Copyright (C) 2008 Analog Devices, Inc. All Rights Reserved.
** Arithmetic left shift, on unsigned long long.
** 
**  long long __ashftli (long long ll1, int i1);
**
*/

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12sx ashift"
%notes "Arithmetic left shift on 64-bit integer."
%regs_clobbered "R0-R3,P1,CCset"
%const
%syscall ___ashftli(lL=lLI)
#endif

.FILE_ATTR FuncName      = ___ashftli;
.FILE_ATTR libName       = libdsp;
.FILE_ATTR libGroup      = integer_support;
.FILE_ATTR libFunc       = ___ashftli;
.FILE_ATTR prefersMem    = internal;
.FILE_ATTR prefersMemNum = "30";

.GLOBAL ___ashftli;
.TYPE ___ashftli, STT_FUNC;

.SECTION program;
.ALIGN 2;

___ashftli:
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
      R3 = ASHIFT R1 BY R3.L;
      R2 = -R2;
      R1 >>>= R2;
      R0 >>= R2;
      R0 = R0 | R3;
#else
      R3 = -64;
      R2 = MAX (R2, R3);
      R2 += 32;
      R3 = ASHIFT R1 BY R2.L;
      R2 += -32;
      R1 = ASHIFT R1 BY R2.L;
      R0 = LSHIFT R0 BY R2.L;
      R0 = R0 | R3;
#endif
      RTS;

.___ashftli.end:

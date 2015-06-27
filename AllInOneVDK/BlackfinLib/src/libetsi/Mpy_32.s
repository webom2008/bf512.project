// **************************************************************************
//
//   Analog Devices Blackfin ETSI Implementation. 
//    Copyright (C). 2002-2008 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=Mpy_32;
.file_attr FuncName=_Mpy_32;
.file_attr libFunc=_Mpy_32;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#include <sys/anomaly_macros_rtl.h>

#if defined(__ADSPBLACKFIN__) && !defined(__ADSPLPBLACKFIN__)
/* __ADSPBF535__ core only */
# define OVERFLOW AV0
#else
# define OVERFLOW V
#endif

.section program;

.align 2;
_Mpy_32:
   [--SP] = R7;                            // save R7
   R7 = 1 (Z);                             // hold 1 in R7
   R1.L = R1.L*R2.L (T) || R3 = [SP+ 16];
#if __SET_ETSI_FLAGS
   CC = OVERFLOW;                          // check for overflow
#endif
   R3.L = R0.L*R3.L (T);
#if __SET_ETSI_FLAGS
   CC |= OVERFLOW;                         // check for overflow
#endif
   A0 = R3.L*R7.L (W32);
#if __SET_ETSI_FLAGS
   CC |= OVERFLOW;
#endif
   A0 += R2.L*R0.L (W32);
#if __SET_ETSI_FLAGS
   CC |= OVERFLOW;                         // check for overflow
#endif
   R0 = (A0+=R7.L*R1.L);
#if __SET_ETSI_FLAGS
   CC |= OVERFLOW;
   IF !CC JUMP no_overflow (BP);           // if overflowed, write 1 to Overflow
   P1.L = _Overflow;
   P1.H = _Overflow;
   [P1] = R7;
no_overflow:
#if WA_05000428
   NOP;
#endif
#endif
   R7 = [SP++];                            // restore R7
   RTS;
._Mpy_32.end:
   .global _Mpy_32;
   .type _Mpy_32,STT_FUNC;

#if __SET_ETSI_FLAGS
   .extern _Overflow;
   .type _Overflow,STT_OBJECT;
#endif

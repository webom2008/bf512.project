/******************************************************************************
  Copyright(c) 2000-2008 Analog Devices Inc. IPDC BANGALORE, India.
  All rights reserved
******************************************************************************
  File Name      : vvsub.asm
  Module Name    : vector vector subtraction
  Label name     : __vecvsub_fr16
  Description    : This function computes the vector vector subtraction
  Operand        : R0 - address of input vector A,
                   R1 - address of input vector B,
                   R2 - address of output vector
  Registers Used : R0,R1,R2,R3,I0,P0,P1,P2.

  Notes          : Input vectors should be in different banks to achieve
                   the cycle count given below.

  CYCLE COUNT    : 13            N == 0
                 : 10 + 2*N      for other N
  'N' - NUMBER OF ELEMENTS

  CODE SIZE      : 40 BYTES
******************************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libGroup      = vector.h;
.file_attr libGroup      = matrix.h;        // called from matrix.h
.file_attr libFunc       = vecvsub_fr16;
.file_attr libFunc       = __vecvsub_fr16;
.file_attr libFunc       = matmsub;        // called from matrix.h
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = __vecvsub_fr16;
#endif

#include <sys/anomaly_macros_rtl.h>

#if defined(__ADSPLPBLACKFIN__) && defined(__WORKAROUND_SPECULATIVE_LOADS)
#define __WORKAROUND_BF532_ANOMALY_050000245
#endif

.section program;
.global __vecvsub_fr16;
.align 2;

__vecvsub_fr16:
        P1 = [SP+12];              // NO. OF ELEMENTS IN INPUT VECTOR
        P0 = R0;                   // ADDRESS OF INPUT VECTOR1
        I0 = R1;                   // ADDRESS OF INPUT VECTOR2
        CC = P1 <= 0;              // CHECK IF NO. ELEMENTS(N) <= 0
        IF CC JUMP RET_ZERO;

#if WA_05000428                                   || \
    defined(__WORKAROUND_BF532_ANOMALY_050000245)
        NOP;
        NOP;
#endif
        P2 = R2;                   // ADDRESS OF OUTPUT VECTOR
        R1 = W[P0++] (Z);          // GET INPUTS FROM VECTOR1 AND VECTOR2
        R2.L = W[I0++];

        LSETUP(ST_VVSUB,END_VVSUB) LC0 = P1;
ST_VVSUB:    R3.L = R1.L - R2.L(S) || R1 = W[P0++] (Z);
                                   // DO SUBTRACTION, FETCH NEXT INPUT FROM VEC1

END_VVSUB:   W[P2++] = R3 || R2.L = W[I0++];
                                   // STORE RESULT IN OUTPUT VECTOR
                                   // AND FETCH NEXT INPUT FROM VECTOR2

RET_ZERO:
        RTS;

.__vecvsub_fr16.end:

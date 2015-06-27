/******************************************************************************
  Copyright(c) 2000-2006 Analog Devices Inc. IPDC BANGALORE, India. 
  All rights reserved
******************************************************************************
  File Name      : vsadd.asm
  Module Name    : vector scalar addition
  Label name     :  __vecsadd_fr16
  Description    : This function computes the sum of a vector and a scalar
  Operand        : R0 - Address of input vector,
                   R1 - scalar value,
                   R2 - Address of output vector
  Registers Used : R3,I0,P0,P1
  Notes          : Input and output vectors should be in different banks
                   to achieve the cycle count given below. Also the function
                   reads two elements beyond the end of the input vector to
                   achieve a 1-cycle loop

  CYCLE COUNT    : 10          N == 0
                 : 14 + N      for other N
                               'N' - NUMBER OF ELEMENTS

  CODE SIZE      : 46 BYTES
******************************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libGroup      = vector.h;
.file_attr libGroup      = matrix.h;             // called from matrix.h
.file_attr libFunc       = vecsadd_fr16;
.file_attr libFunc       = __vecsadd_fr16;
.file_attr libFunc       = matsadd_fr16;        // called from matrix.h
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = __vecsadd_fr16;
#endif

.section      program;
.global       __vecsadd_fr16;
.align 2;

__vecsadd_fr16:
        R3 = [SP+12];              // NO. OF ELEMENTS IN INPUT VECTOR
        CC = R3 <= 0;              // CHECK IF NO. ELEMENTS(N) <= 0
        IF CC JUMP RET_ZERO;

        P0 = R0;                   // ADDRESS OF INPUT COMPLEX VECTOR
        P1 = R3;                   // SET LOOP COUNTER
        I0 = R2;                   // ADDRESS OF OUTPUT COMPLEX VECTOR
        R0 = W[P0++] (Z);          // FETCH INPUT[0]
        R3.L = R0.L + R1.L(S) || R0 = W[P0++] (Z);  
                                   // CALCULATE OUTPUT[0] AND FETCH INPUT[1]

        LSETUP(ST_VSADD,ST_VSADD) LC0 = P1;
            // DO ADDITION, FETCH NEXT INPUT, STORE PREVIOUS OUTPUT
ST_VSADD:   R3.L = R0.L + R1.L(S) || R0 = W[P0++] (Z) || W[I0++] = R3.L;

#if defined(__WORKAROUND_INFINITE_STALL_202)
/* After 2 possible dual dag load/stores, need 2 prefetch loads to avoid the
** anomaly (since the first prefetch is still part of the anomaly sequence).
*/
        PREFETCH[SP];
        PREFETCH[SP];
#endif

RET_ZERO:
        RTS;

.__vecsadd_fr16.end:

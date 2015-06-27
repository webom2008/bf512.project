/******************************************************************************
  Copyright(c) 2000-2008 Analog Devices Inc. IPDC BANGALORE, India.
  All rights reserved
 ******************************************************************************
  File name   :   acos_fr16.asm

  Module name :   fractional Arc Cosine

  Label name  :   __acos_fr16

  Description :   This program finds Arc cosine of fractional input.

  Domain      :   [0x0000,0x7333]  (i.e 0.0 ...... 0.9)
                  The function returns 0 for any input argument that is
                  outside the defined domain.

  Registers used :

  R0 - fractional input no = x.
  R1,R2,R3,P0,P1,A0

  CYCLE COUNT    : 10            N <  0
                 : 12            N == 0
                 : 37            N <= 0X6666
                 : 34            N <= 0X6CCD
                 : 31            N <= 0X7333
                 : 21            other N
  'N' - INPUT VALUE IN FRACTIONAL FORMAT

  CODE SIZE      : 182 BYTES

**************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.file_attr libGroup      = math.h;
.file_attr libGroup      = math_bf.h;
.file_attr libFunc       = __acos_fr16;
.file_attr libFunc       = acos_fr16;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = __acos_fr16;

#endif

#include <sys/anomaly_macros_rtl.h>

.section data1;
 .align 2;
 .acoscoef0:
 .byte2 = 0x513F,0x0413,0xF39C,0x4D9B,0x9672,0x4139;
 .acoscoef1:
 .byte2 = 0x2BB6,0xAD97,0x5DC0;
 .acoscoef2:
 .byte2 = 0x2DA3,0x9F04,0x4F95;


.section  program;
.global __acos_fr16;
.align 2;

__acos_fr16:

      CC = R0 < 0;                       // CHECK R0 < 0
      IF CC JUMP RET_ERROR;              // RETURN ERROR
      CC = R0 == 0;                      // CHECK R0 == 0
      IF CC JUMP RET_MAX;                // RETURN 0X7FFF
      R3 = 0X6666;                       // INITIALISE R3 = 0X6666
      CC = R0 <= R3;                     // CHECK R0 <= 0X6666
      IF CC JUMP ASIN_RTN0;              // IF TRUE BRANCH TO ASIN_RTN0
      R3 = 0X6CCD;                       // INITIALISE R3 = 0X6CCD
      CC = R0 <= R3;                     // CHECK R0 <= 0X6CCD
      IF CC JUMP ASIN_RTN1;              // IF TRUE BRANCH TO ASIN_RTN1
      R3 = 0X7333;                       // INITIALISE R3 = 0X7333
      CC = R0 <= R3;                     // CHECK R0 <= 0X7333
      IF !CC JUMP RET_ERROR;             // IF FALSE THEN JUMP TO RETURN 0X7FFF

ASIN_RTN2:
      P0.L = .acoscoef2;                 // POINTER TO ARRAY OF COEFFICIENT 2
      P0.H = .acoscoef2;

#if WA_05000428
      // Speculative read from L2 by Core B may cause a write to L2 to fail

      NOP;
#endif

      R3 = W[P0++] (Z);                  // GET FIRST COEFFICIENT
      R3 <<= 16;                         // ARRANGE IN 32 BIT FORMAT
      A0 = R3 || R3 = W[P0++] (Z);       // INITIALISE A0 WITH FIRST COEFFICIENT
                                         // AND GET NEXT COEFFICIENT
      R2.L = R0.L * R0.L;                // R2.L = R0.L^2
      A0 += R0.L * R3.L || R3 = W[P0++] (Z); // ACCUMULATES THE PRODUCT OF DATA AND
                                         // COEFFICIENT AND FETCHS NEXT COEFF
      R2 = (A0 += R2.L * R3.L);          // ACCUMULATES THE PRODUCT OF DATA AND
                                         // COEFFICIENT
      R2 >>>= 14;                        // SAVE IN FRACT16
      JUMP RET_VALUE;                    // BRANCH TO RET_VALUE

ASIN_RTN1:
      P0.L = .acoscoef1;                 // POINTER TO ARRAY OF COEFFICIENT 1
      P0.H = .acoscoef1;
      R3 = W[P0++] (Z);                  // GET FIRST COEFFICIENT
      R3 <<= 16;                         // ARRANGE IN 32 BIT FORMAT
      A0 = R3 || R3 = W[P0++] (Z);       // INITIALISE A0 WITH FIRST COEFFICIENT
                                         // AND GET NEXT COEFFICIENT
      R2.L = R0.L * R0.L;                // R2.L = R0.L^2
      A0 += R0.L * R3.L || R3 = W[P0++] (Z); // ACCUMULATES THE PRODUCT OF DATA AND
                                         // COEFFICIENT AND FETCHS NEXT COEFF
      R2 = (A0 += R2.L * R3.L);          // ACCUMULATES THE PRODUCT OF DATA AND
                                         // COEFFICIENT
      R2 >>>= 15;                        // SAVE IN FRACT16
      JUMP RET_VALUE;                    // BRANCH TO RET_VALUE

ASIN_RTN0:
      P0.L = .acoscoef0;                 // POINTER TO ARRAY OF COEFFICIENT 0
      P0.H = .acoscoef0;
      P1 = 3;                            // P1 = 3 = NO OF COEFFICIENTS
      R1 = R0;                           // LOAD R0(y = x)
      A0 = 0 || R3 = W[P0++] (Z);        // CLEAR A0 AND GET FIRST COEFFICIENT
      LSETUP(ASINST,ASINEND) LC0 = P1;   // LOOP SETUP FOR COUNTER P1

ASINST:   R1.H = R1.L * R0.L;
          A0 += R0.L * R3.L || R3 = W[P0++] (Z);
          R0.L = R1.L * R1.H;
ASINEND:  R2 = (A0 += R1.H * R3.L) || R3 = W[P0++] (Z);

      R2.L = R2 (RND);                   // SAVE IN FRACT16

RET_VALUE:
      R2 = R2.L(X);                      // SIGN EXTEND R2
      R0 = -32768;                       // INITIALISE R0 = -1
      R0 = R0 - R2;                      // R0 = -1 - ASIN(X)
      R0 = R0.L(X);                      // SIGN EXTEND THE RESULT
      RTS;

RET_ERROR:
      R0 = 0;                            // RETURN 0 FOR A DOMAIN ERROR
      RTS;

RET_MAX:
      R0 = 0X7FFF;                       // RETURN RESULT
      RTS;

.__acos_fr16.end:

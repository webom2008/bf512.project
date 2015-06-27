/******************************************************************************
  Copyright(c) 2000-2008 Analog Devices Inc. IPDC BANGALORE, India.
  All rights reserved
 ******************************************************************************
  File name   :   asin_fr16.asm

  Module name :   fractional Arc sine

  Label name  :   __asin_fr16

  Description :   This program finds Arc sine of fractional input.

  Domain      :   [0x8ccd,0x7333]  (i.e -0.9 ...... 0.9)
                  The function returns 0 for any input argument that is
                  outside the defined domain.

  Registers used :

  R0 - fractional input no.
  R1,R2,R3,R7,P0,P1,A0

  CYCLE COUNT    : 10            N == -1
                 : 39           |N| <= 0X6666
                 : 35           |N| <= 0X6CCD
                 : 32           |N| <= 0X7333
                 : 22            other N
  'N' - INPUT VALUE IN FRACTIONAL FORMAT

  CODE SIZE      : 184 BYTES

**************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.file_attr libGroup      = math.h;
.file_attr libGroup      = math_bf.h;
.file_attr libFunc       = __asin_fr16;
.file_attr libFunc       = asin_fr16;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = __asin_fr16;

#endif

#include <sys/anomaly_macros_rtl.h>

.section data1;
 .align 2;
 .asincoef0:
 .byte2 = 0x513F,0x0413,0xF39C,0x4D9B,0x9672,0x4139;
 .asincoef1:
 .byte2 = 0x2BB6,0xAD97,0x5DC0;
 .asincoef2:
 .byte2 = 0x2DA3,0x9F04,0x4F95;


.section  program;
.global __asin_fr16;
.align 2;

__asin_fr16:

      R3 = -32768;                       // INITIALISE R3 = -1
      CC = R0 == R3;                     // CHECK R0 == -1
      IF CC JUMP RET_ERROR;              // RETURN 0 (OUT OF DOMAIN)
      R1 = ABS R0;                       // R1 = ABS R0
      R3 = 0X6666;                       // INITIALISE R3 = 0X6666
      CC = R1 <= R3;                     // CHECK R1 <= 0X6666
      IF CC JUMP ASIN_RTN0;              // IF TRUE BRANCH TO ASIN_RTN0
      R3 = 0X6CCD;                       // INITIALISE R3 = 0X6CCD
      CC = R1 <= R3;                     // CHECK R1 <= 0X6CCD
      IF CC JUMP ASIN_RTN1;              // IF TRUE BRANCH TO ASIN_RTN1
      R3 = 0X7333;                       // INITIALISE R3 = 0X7333
      CC = R1 <= R3;                     // CHECK R1 <= 0X7333
      IF !CC JUMP RET_ERROR;             // IF FALSE THEN BRANCH TO RETURN 0

ASIN_RTN2:
      P0.L = .asincoef2;                 // POINTER TO ARRAY OF COEFFICIENT 2
      P0.H = .asincoef2;

#if WA_05000428
      // Speculative read from L2 by Core B may cause a write to L2 to fail

      NOP;
#endif

      R3 = W[P0++] (Z);                  // GET FIRST COEFFICIENT
      R3 <<= 16;                         // ARRANGE IN 32 BIT FORMAT
      A0 = R3 || R3 = W[P0++] (Z);       // INITIALISE A0 WITH FIRST COEFFICIENT
                                         // AND GET NEXT COEFFICIENT
      R2.L = R1.L * R1.L;                // R2.L = R1.L^2
      A0 += R1.L * R3.L || R3 = W[P0++] (Z); // ACCUMULATES THE PRODUCT OF DATA AND
                                         // COEFFICIENT AND FETCHS NEXT COEFF
      R2 = (A0 += R2.L * R3.L);          // ACCUMULATES THE PRODUCT OF DATA AND
                                         // COEFFICIENT
      R2 >>>= 14;                        // SAVE IN FRACT16
      JUMP CHECK_FOR_SIGN;               // BRANCH TO CHECK_FOR_SIGN

ASIN_RTN1:
      P0.L = .asincoef1;                 // POINTER TO ARRAY OF COEFFICIENT 1
      P0.H = .asincoef1;
      R3 = W[P0++] (Z);                  // GET FIRST COEFFICIENT
      R3 <<= 16;                         // ARRANGE IN 32 BIT FORMAT
      A0 = R3 || R3 = W[P0++] (Z);       // INITIALISE A0 WITH FIRST COEFFICIENT
                                         // AND GET NEXT COEFFICIENT
      R2.L = R1.L * R1.L;                // R2.L = R1.L^2
      A0 += R1.L * R3.L || R3 = W[P0++] (Z); // ACCUMULATES THE PRODUCT OF DATA AND
                                         // COEFFICIENT AND FETCHS NEXT COEFF
      R2 = (A0 += R2.L * R3.L);          // ACCUMULATES THE PRODUCT OF DATA AND
                                         // COEFFICIENT
      R2 >>>= 15;                        // SAVE IN FRACT16
      JUMP CHECK_FOR_SIGN;               // BRANCH TO CHECK_FOR_SIGN

ASIN_RTN0:
      [--SP] = R7;                       // PUSH R7 REG TO STACK
      P0.L = .asincoef0;                 // POINTER TO ARRAY OF COEFFICIENT 0
      P0.H = .asincoef0;
      P1 = 3;                            // P1 = 3 = NO OF COEFFICIENTS
      R7 = R1;                           // LOAD R1(y = x)
      A0 = 0 || R3 = W[P0++] (Z);        // CLEAR A0 AND GET FIRST COEFFICIENT
      LSETUP(ASINST,ASINEND) LC0 = P1;   // LOOP SETUP FOR COUNTER P1

ASINST:   R7.H = R7.L * R1.L;
          A0 += R1.L * R3.L || R3 = W[P0++] (Z);
          R1.L = R7.L * R7.H;
ASINEND:  R2 = (A0 += R7.H * R3.L) || R3 = W[P0++] (Z);

      R2.L = R2 (RND);                   // SAVE IN FRACT16
      (R7:7) = [SP++];                   // POP R7

CHECK_FOR_SIGN:
      R2 = R2.L(X);                      // SIGN EXTEND R2
      CC = R0 < 0;                       // CHECK FOR SIGN
      R0 = -R2;                          // OUTPUT IS NEGATED
      IF !CC R0 = R2;                    // IF NO SIGN OUTPUT IS NON-NEGATED
      RTS;

RET_ERROR:
      R0 = 0;                            // RETURN 0 FOR A DOMAIN ERROR
      RTS;

.__asin_fr16.end:

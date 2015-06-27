/******************************************************************************
  Copyright(c) 2000-2004 Analog Devices Inc. IPDC BANGALORE, India.
  All rights reserved
 ******************************************************************************
  File name   :  sin_fr16.asm

  Module name :  Fractional sine

  label name  :  __sin_fr16

  Description :  This program finds the sine of a given fractional input value.

  Sine Approximation: y = sin (x * PI/2)

  Registers used :

  R0 -> INPUT value,
  R1,R2,R3,P0,P1,P2,A0

  CYCLE COUNT    : 25

  CODE SIZE      : 78 BYTES

  DATE           : 26-02-01

**************************************************************/
.file_attr libGroup      = math_bf.h;
.file_attr libGroup      = math.h;
.file_attr libFunc       = __sin_fr16;
.file_attr libFunc       = sin_fr16;
.file_attr FuncName      = __sin_fr16;

/* Called by polar_fr16 */
.file_attr libGroup      = complex.h;
.file_attr libFunc       = polar_fr16;
.file_attr libFunc       = __polar_fr16;

/* Called by twidfft_fr16 */
.file_attr libGroup = filter.h;
.file_attr libFunc  = twidfft_fr16;
.file_attr libFunc  = __twidfft_fr16;

/* Called by twidfft2d_fr16 */
.file_attr libFunc  = twidfft2d_fr16;
.file_attr libFunc  = __twidfft2d_fr16;

/* Called by twidfftf_fr16 */
.file_attr libFunc  = twidfftf_fr16;
.file_attr libFunc  = __twidfftf_fr16;

/* Called by twidfftrad2_fr16 */
.file_attr libFunc  = twidfftrad2_fr16;
.file_attr libFunc  = __twidfftrad2_fr16;

/* Called by twidfftrad4_fr16 */
.file_attr libFunc  = twidfftrad4_fr16;
.file_attr libFunc  = __twidfftrad4_fr16;

.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";




.section data1;
 .align 2;
 .sincoef:
 .byte2 = 0x6480,0x0059,0xD54D,0x0252,0x0388;

.section  program;
.global __sin_fr16;
.align 2;

__sin_fr16:

      P0 = 2;                     // STORE LOOP COUNTER VALUE
      P1.L = .sincoef;            // POINTER TO SIN COEFFICIENT
      P1.H = .sincoef;
      P2 = R0;
      R1 = ABS R0;                // GET THE ABSOLUTE VALUE OF INPUT
      R3 = R1;                    // COPY R1 TO R3 REG (Y = X)

      A0 = 0 || R0 = W[P1++] (Z);     // CLEAR ACCUMULATOR AND GET FIRST COEFFICIENT
      LSETUP(SINSTRT,SINEND) LC0 = P0;
                                  // SET A LOOP FOR LOOP COUNTER VALUE
SINSTRT:  R1.H = R1.L * R3.L;              // EVEN POWERS OF X
          A0 += R3.L * R0.L || R0 = W[P1++] (Z);
          R3.L = R1.L * R1.H;              // ODD POWERS OF X)
SINEND:   A0 += R1.H * R0.L || R0 = W[P1++] (Z);

      R1 = 0x7fff;                // INITIALISE R1 TO 0X7FFF
      R2 = (A0 += R3.L * R0.L);
      R2 = R2 >> 15;              // SAVE IN FRACT16
      CC = R1 < R2;               // IF R2 > 0X7FFF
      IF CC R2 = R1;              // IF TRUE THEN INITIALISE R2 = 0X7FFF
      CC = P2 < 0;                // CHECK WHETHER INPUT IS LESS THAN ZERO
      R0 = -R2;                   // OUTPUT IS NEGATED
      IF !CC R0 = R2;             // USE NON_NEGATED OUTPUT IF INPUT >= ZERO

      RTS;
.__sin_fr16.end:

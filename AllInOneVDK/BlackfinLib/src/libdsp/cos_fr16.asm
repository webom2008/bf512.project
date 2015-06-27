/******************************************************************************
  Copyright(c) 2000-2007 Analog Devices Inc. IPDC BANGALORE, India.
  All rights reserved
 ******************************************************************************
  File name   :   cos_fr16.asm

  Module name :   Fractional cosine

  Label Name  :   __cos_fr16

  Description :   This program finds the cosine of a given fractional
                  input value.

  cosine Approximation: y = cos (x * PI/2)

  Registers used :

  R0 -> INPUT value,
  R1,R2,R3,P0,P1,P2,A0

**************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.file_attr libGroup = math.h;
.file_attr libGroup = math_bf.h;
.file_attr libName  = libdsp;
.file_attr FuncName = __cos_fr16;
.file_attr libFunc  = __cos_fr16;
.file_attr libFunc  = cos_fr16;

    /* Called by twidfftrad4_fr16 */
.file_attr libGroup = filter.h;
.file_attr libFunc  = twidfftrad4_fr16;
.file_attr libFunc  = __twidfftrad4_fr16;

    /* Called by twidfftrad2_fr16 */
.file_attr libFunc  = twidfftrad2_fr16;
.file_attr libFunc  = __twidfftrad2_fr16;

    /* Called by twidfftf_fr16 */
.file_attr libFunc  = twidfftf_fr16;
.file_attr libFunc  = __twidfftf_fr16;

    /* Called by twidfft2d_fr16 */
.file_attr libFunc  = twidfft2d_fr16;
.file_attr libFunc  = __twidfft2d_fr16;

    /* Called by twidfft_fr16 */
.file_attr libFunc  = twidfft_fr16;
.file_attr libFunc  = __twidfft_fr16;

    /* Called by polar_fr16 */
.file_attr libGroup = complex.h;
.file_attr libFunc  = polar_fr16;
.file_attr libFunc  = __polar_fr16;

    /* Called by __cos16_2PIx */
    /* __cos16_2PIx: called by gen_blackman_fr16 */
.file_attr libGroup = window.h;
.file_attr libFunc  = gen_blackman_fr16;
.file_attr libFunc  = __gen_blackman_fr16;

    /* __cos16_2PIx: called by gen_hamming_fr16 */
.file_attr libFunc  = gen_hamming_fr16;
.file_attr libFunc  = __gen_hamming_fr16;

    /* __cos16_2PIx: called by gen_hanning_fr16 */
.file_attr libFunc  = gen_hanning_fr16;
.file_attr libFunc  = __gen_hanning_fr16;

    /* __cos16_2PIx: called by gen_harris_fr16 */
.file_attr libFunc  = gen_harris_fr16;
.file_attr libFunc  = __gen_harris_fr16;

.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
#endif

.section data1;
 .align 2;
 .coscoef:
 .byte2 = 0x6480,0x0059,0xD54D,0x0252,0x0388;

.section  program;
.global __cos_fr16;
.align 2;

__cos_fr16:

      R0 = ABS R0;              // GET THE ABSOLUTE VALUE OF INPUT
      P0.L = .coscoef;          // POINTER TO COSINE COEFFICIENT
      P0.H = .coscoef;
      P1 = 2;                   // SET LOOP COUNTER VALUE = 2
      R3 = -32768;              // INITIALISE R3 = -1.0
      R0 = R3-R0;               // R0 = -1 - R0
      R1 = R0;                  // COPY RO TO R1 REG (Y = X)
      A0 = 0 || R3 = W[P0++] (Z);   // SET ACCUMULATOR = 0 AND GET FIRST COEFFICIENT
      LSETUP(COSSTRT,COSEND) LC0 = P1;
                                // SET A LOOP FOR LOOP COUNTER VALUE = 2
COSSTRT:  R0.H = R0.L * R1.L;                // EVEN POWERS OF X
          A0 += R1.L * R3.L || R3 = W[P0++] (Z);
          R1.L = R0.L * R0.H;                // ODD POWERS OF X
COSEND:   A0 += R0.H * R3.L || R3 = W[P0++] (Z);

      R0 = 0x7fff;              // INITIALISE R0 TO 0X7FFF
      R2 = (A0 += R1.L * R3.L);
      R2 = R2 >> 15;            // SAVE IN FRACT16
      CC = R0 < R2;             // IF R2 > 0x7FFF
      IF CC R2 = R0;            // IF TRUE THEN INITIALISE R2 = 0X7FFF
      R0 = R2.L(X);             // COPY OUTPUT VALUE TO R0
      RTS;

.__cos_fr16.end:

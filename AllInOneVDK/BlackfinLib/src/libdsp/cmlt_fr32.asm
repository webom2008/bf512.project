/************************************************************************
 *
 * cmlt_fr32.asm : $Revision: 1.7 $
 *
 * (c) Copyright 2007 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: cmlt_fr32 - complex multiply

    Synopsis:

        #include <complex.h>
        complex_fract32 cmlt_fr32 (complex_fract32 a,
                                   complex_fract32 b);

    Description:

        The cmlt_fr32 function computes the complex multiplication
        of two complex inputs, a and b, and returns the result.

        An alternative implementation is available as built-in
        (__builtin_cmplx_mul32). It can be used to improve performance
        at the expense of increased code size (if placed multiple times).

    Error Conditions:

        The function does not return any error conditions.

    Algorithm:

        Re(c) = (Re(a) * Re(b)) - (Im(a) * Im(b))
        Im(c) = (Re(a) * Im(b)) + (Im(a) * Re(b))

    Implementation:

        If maximum precision is to be preserved then the implementation
        must be based on the accumulators A0 and A1. If the accumulators
        are not used then there will be issues associated with saturated
        operands returning unexpected results.

        The real component of the result (A * B) is decomposed into 16-bit
        components as follows:

          = ( (ArHi * BrHi) + (ArHi * BrLo) + (ArLo * BrHi) + (ArLo * BrLo) )
          -   (AiHi * BiHi) - (AiHi * BiLo) - (AiLo * BiHi) - (AiLo * BiLo)

            where: ArHi is the Hi component of A.re
                   ArLo is the Lo component of A.re

                   AiHi is the Hi component of A.im
                   AiLo is the Lo component of A.im

        The actual implementation uses the modified formula:

          = ( (ArHi * BrHi) +  (ArHi * BrLo) + (ArLo * BrHi) +  (ArLo * BrLo) )
          -   (AiHi * BiHi) + (-AiHi * BiLo) - (AiLo * BiHi) + (-AiLo * BiLo)

        - this is because the sub-expressions (ArLo * BrLo) and (AiLo * BiLo)
        are calculated using unsigned arithmetic. If these two products are
        then combined as a result of a multiply-decrement operation and
        the resultant difference is negative, then the ACC will saturate
        to zero. To avoid this happening, the function uses the two
        sub-expressions in a sum as follows:

            (ArLo * BrLo) + (-AiLo * BiLo)

        The imaginary component of (A * B) is decomposed into:

          = ( (ArHi * BiHi) + (ArHi * BiLo) + (ArLo * BiHi) + (ArLo * BiLo) )
          + ( (AiHi * BrHi) + (AiHi * BrLo) + (AiLo * BrHi) + (AiLo * BrLo) )

        Note that the above code does not demonstrate how intermediate
        results are scaled and does not include rounding. For reference,
        a fract32 multiply may be calculated as follows:

            A1  = R0.L * R1.L (FU);
            R2  = 128;
            A1 += R2.L * R2.L;
            A1  = A1 >> 16;
            A0  = R0.H * R1.H, A1 += R0.H * R1.H (M);
            A1 += R1.H * R0.L (M);
            A1 += R2.L * R2.L (FU);
            A1  = A1 >>> 15;
            R0  = (A0 += A1);

    Example:

        #include <complex.h>

        complex_fract32 x;
        complex_fract32 y;
        complex_fract32 z;

        z = cmlt_fr32 (x,y);

    Cycle Count:

        49 cycles per call

        (Measured for an ADSP-BF532 using version 5.0.0.35 of the ADSP-BF5xx
         Family Simulator and includes the overheads involved in calling the
         library procedure as well as the costs associated with argument
         passing).

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR  libGroup      = complex.h;
.FILE_ATTR  libFunc       = __cmlt_fr32;
.FILE_ATTR  libFunc       = cmlt_fr32;
.FILE_ATTR  libName       = libdsp;
.FILE_ATTR  prefersMem    = internal;
.FILE_ATTR  prefersMemNum = "30";
.FILE_ATTR  FuncName      = __cmlt_fr32;
#endif

.GLOBAL __cmlt_fr32, _cmul_fr32;
.TYPE   __cmlt_fr32, STT_FUNC;
.TYPE   _cmul_fr32, STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

_cmul_fr32:
__cmlt_fr32:

   /* Preserve registers */

      M1 = R7;
      M0 = R6;

   /* Initialize */

      R6 = PACK (R0.H, R0.L)                // copy A.re to R6
      || R3 = [SP+12];                      // and load B.im into R3

#if defined(__ADSPBLACKFIN__) && !defined(__ADSPLPBLACKFIN__)
/* __ADSPBF535__ core only */
      R0 = 0;
      R0 = R0 - R1 (S);                     // Negate A.im
#else
      R0 = -R1 (S);                         // Negate A.im
#endif

      R7 = 128;                             // constant for rounding

   /* Compute real component (see Implementation above)
   **
   ** Registers:
   **     R6 = A.re;
   **     R0 = -A.im;
   **     R1 = A.im;
   **     R2 = B.re;
   **     R3 = B.im;
   */

      // ACC1 = ACC1 + (-AiHi * BiLo)
      // ACC0 = ACC0 + (-AiLo * BiLo)
      A1 = R0.H * R3.L (M), A0 = R0.L * R3.L (FU);

      // ACC0 += (0x0080 * 0x0080)     = 0x8000 = rounding constant
      // ACC1 += (0x0080 * 0x0080) (M) = 0x4000 = rounding constant
      A1 += R7.L * R7.L (M), A0 += R7.L * R7.L (FU);

      // ACC1 = ACC1 + (BiHi * AiLo)
      A1 -= R3.H * R1.L (M);

      // ACC1 = ACC1 + (ArHi * BrLo)
      // ACC0 = ACC0 + (ArLo * BrLo)
      A1 += R6.H * R2.L (M), A0 += R6.L * R2.L (FU);

      // ACC1 = ACC1 + (BrHi * ArLo)
      A1 += R2.H * R6.L (M);

      A0  = A0 >> 16;
      A0 += A1;
      A0  = A0 >>> 15;

      // ACC0 = ACC0 + (ArHi * BrHi)
      A0 += R6.H * R2.H;

      // ACC0 = ACC0 - (AiHi * BiHi)
      R0 = (A0 -= R1.H * R3.H);          // R0 = real component of the result

   /* Compute imaginary component (see Implementation above) */

      // ACC0 = rounding_constant * rounding_constant
      // ACC1 = rounding_constant * rounding_constant
      A1 = R7.L * R7.L (M), A0 = R7.L * R7.L;

      // ACC1 = ACC1 + (ArHi * BiLo)
      // ACC0 = ACC0 + (ArLo * BiLo)
      A1 += R6.H * R3.L (M), A0 += R6.L * R3.L (FU);

      // ACC1 = ACC1 + (BiHi * ArLo)
      A1 += R3.H * R6.L (M);

      // ACC1 = ACC1 + (AiHi * BrLo)
      // ACC0 = ACC0 + (AiLo * BrLo)
      A1 += R1.H * R2.L (M), A0 += R1.L * R2.L (FU);

      // ACC1 = ACC1 + (BrHi * AiLo)
      A1+= R2.H * R1.L (M);

      A0  = A0 >> 16;
      A0 += A1;
      A0  = A0 >>> 15;

      // ACC0 = ACC0 + (ArHi * BiHi)
      A0 += R6.H * R3.H;

      // R2 = (ACC0 = ACC0 + (AiHi * BrHi)
      R2 = (A0 += R1.H * R2.H);

      R1 = R2;                           // copy imaginary component to result

   /* Restore registers and Return */

      R6 = M0;
      R7 = M1;

      RTS;

.__cmlt_fr32.end:
._cmul_fr32.end:


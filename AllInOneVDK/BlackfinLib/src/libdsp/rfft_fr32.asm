/*****************************************************************************
 *
 * rfft_fr32.asm : $Revision: 1.5.14.2 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: rfft_fr32 - Real radix-2 Fast Fourier Transform

    Synopsis:

        #include <filter.h>
        void rfft_fr32 (const fract32         input[],
                        complex_fract32       output[],
                        const complex_fract32 twiddle_table[],
                        int                   twiddle_stride,
                        int                   fft_size,
                        int                   *block_exponent,
                        int                   scale_method);

    Description:

        This function transforms the time domain real input signal sequence to
        the frequency domain by using the radix-2 Fast Fourier Transform (FFT).

        The size of the input array input and the output array output is
        fft_size, where fft_size represents the number of points in the FFT.
        By allocating these arrays in different memory banks, any potential
        data bank collisions are avoided, thus improving run-time performance.
        If the input data can be overwritten, the optimum memory usage can be
        achieved by also specifying the input array as the output array,
        provided that the size of the input array is at least 2*fft_size.

        The twiddle table is passed in the argument twiddle_table, which must
        contain at least fft_size/2 twiddle factors. The table is composed of
        +cosine and -sine coefficients and may be initialized by using the
        function twidfftrad2_fr32. For optimal performance, the twiddle table
        should be allocated in a different memory section than the output
        array.

        The argument twiddle_stride should be set to 1 if the twiddle table
        was originally created for an FFT of size fft_size. If the twiddle
        table was created for a larger FFT of size N*fft_size (where N is a
        power of 2), then twiddle_stride should be set to N. This argument
        therefore provides a way of using a single twiddle table to calculate
        FFTs of different sizes.

        The argument scale_method controls how the function will apply scaling
        while computing a Fourier Transform. The available options are static
        scaling (dividing the input at every stage by 2), dynamic scaling
        (dividing the input by 2 at any stage at which the largest absolute
        input value is greater than or equal to 0.25), or no scaling.

        If static scaling is selected, the function will always scale
        intermediate results, thus preventing overflow. The loss of precision
        increases in line with fft_size and is more pronounced for input
        signals with a small magnitude (since the output is scaled by
        1/fft_size). To select static scaling, set the argument scale_method
        to 1. The block exponent returned will be log2(fft_size).

        If dynamic scaling is selected, the function will inspect intermediate
        results and only apply scaling where required to prevent overflow. The
        loss of precision increases in line with the size of the FFT and is
        more pronounced for input signals with a large magnitude (since these
        factors increase the need for scaling). To select dynamic scaling, set
        the argument scale_method to a value of 2. The block exponent returned
        will be between 0 and log2(fft_size) depending upon the number of
        times the function scales each set of intermediate results.

        If no scaling is selected, the function will never scale intermediate
        results. There will be no loss of precision unless overflow occurs
        and in this case the function will generate saturated results. The
        likelihood of saturation increases in line with the fft_size and is
        more pronounced for input signals with a large magnitude. To select
        no scaling, set the argument scale_method to 3. The block exponent
        returned will be 0.

        Note: Any values for the argument scale_method other than
              2 or 3 will result in the function performing static
              scaling.

    Error Conditions:

        The rfft_fr32 function aborts if the FFT size is less than 8
        or if the twiddle stride is less than 1.

    Algorithm:

        The function uses a radix-2 algorithm.

    Implementation:

        The function consists of four parts. In the first part, bit-reversing
        is performed. At the same time, the input data are scanned to
        determine whether scaling will be required at the next stage.

        The access to the input array in the first stage is computed using
        'base address + bit-reversed offset'. Doing so removes the requirement
        for the input buffer to be aligned on a memory boundary that is a
        multiple of the FFT size n.

        The bit-reversal is computed for a FFT of size n/4, with the remaining
        accesses done through fixed offsets:

            Normal     Normal                 Bit-reversed
            order      order                     order
           (n = 16)    (n/4)                   (n = 16)
              0          0   -> bit reversed =    0
              1                                   8   ( 0 + N/2 )
              2                                   4   ( 0 + N/2 - N/4 )
              3                                  12   ( 0 + N/2 - N/4 + N/2 )
              4          1   -> bit reversed =    2
              5                                  10   ( 2 + N/2 )
              6                                   6   ( 2 + N/2 - N/4 )
              7                                  14   ( 2 + N/2 - N/4 + N/2 )
              8          2   -> bit reversed =    1
              9                                   9   ( 1 + N/2 )
             10                                   5   ( 1 + N/2 - N/4 )
             11                                  13   ( 1 + N/2 - N/4 + N/2 )
             12          3   -> bit reversed =    3
             13                                  11   ( 3 + N/2 )
             14                                   7   ( 3 + N/2 - N/4 )
             15                                  15   ( 3 + N/2 - N/4 + N/2 )

        The input data are copied into the imaginary slots of the output array.
        This was done to facilitate efficient in-place bit-reversing, which
        follows the same principle as above, however performed in reverse order.
        Moving the imaginary data to the end of the output buffer first, frees
        up sufficient memory to move the real data thereafter.

        The second and third part compute the stages where no twiddle factors
        are being used. Again each stage is used to scan the data to determine
        whether scaling is required at the next stage. During the second stage,
        the data in the output buffer are re-ordered. The real data are copied
        into the appropriate slot in the output array, while the imaginary slots
        located at even indices are zero filled. During the third stage, the
        remaining imaginary values are inserted.

        The final part is used to compute the remaining stages. The fractional
        multiplies employed differ depending on whether scaling is required or
        not. In case of scaling, the complex product is computed using a
        1.30 by 1.30 fractional multiplication without rounding. Scaling is
        handled implicitly by using the multiplier option IS and a down-shift
        of 16 instead of 15:

          A1  = x.re.hi * twiddle.re.lo (M), A0  = x.re.hi * twiddle.re.hi (IS);
          A1 += x.re.lo * twiddle.re.hi (M);
          A1 -= x.im.hi * twiddle.im.lo (M);
          A1 -= x.im.lo * twiddle.im.hi (M), A0 -= x.im.hi * twiddle.im.hi (IS);
          A1 = A1 >>> 16;
          R2 = (A0 += A1);

        The imaginary part of the product is computed along the same lines as
        the real part above. Accuracy issues are negated by scaling the product
        (shifting off any rounding errors).

        Without scaling, the product must preserve more accuracy. For this
        reason, a 1.31 by 1.31 fractional multiplication with rounding is used.

          A1  = x.re.hi * twiddle.re.lo (M),
          A0  = x.re.lo * twiddle.re.lo (FU);

          A1 += x.re.lo * twiddle.re.hi (M);

          A1 += (-x.im.hi) * twiddle.im.lo (M),
          A0 += (-x.im.lo) * twiddle.im.lo (FU);

          A1 -= x.im.lo * twiddle.im.hi (M);

          A1 += 0x4000,
          A0 += 0x8000;

          A0 = A0 >> 16;
          A0 += A1;
          A0 = A0 >>> 15;
          A0 += x.re.hi * twiddle.re.hi;
          R2 = (A0 -= x.im.hi * twiddle.im.hi);

        The imaginary part of the product is computed along the same lines as
        the real part above.

        In order to improve the performance of the remaining stages, the
        magnitude of the data is only scanned if dynamic scaling is selected
        (except for the last stage where such a check is not necessary).

    Example:

        #include <filter.h>
        #define FFT_SIZE1    32
        #define FFT_SIZE2   256
        #define TWID_SIZE  (FFT_SIZE2/2)

        fract32          in1[FFT_SIZE1], in2[FFT_SIZE2];
        complex_fract32  out1[FFT_SIZE1], out2[FFT_SIZE2];
        complex_fract32  twiddle[TWID_SIZE];

        int  block_exponent1, block_exponent2;

        twidfftrad2_fr32 (twiddle, FFT_SIZE2);

        rfft_fr32 (in1, out1, twiddle,
                   (FFT_SIZE2 / FFT_SIZE1), FFT_SIZE1,
                   &block_exponent1, 1 /*static scaling*/ );

        rfft_fr32 (in2, out2, twiddle,
                   1, FFT_SIZE2,
                   &block_exponent2, 2 /*dynamic scaling*/ );

    Cycle Counts:

                    -- Preserving the input data --

             FFT       Static        No            Dynamic
             Size      scaling       scaling       scaling

               8         280           290         278 -    295
              16         541           616         586 -    668
              32        1155          1419        1311 -   1582
              64        2609          3366        3036 -   3800
             128        5999          7969        7081 -   9058
             256       13773         18620       16406 -  21260
             512       31339         42839       37571 -  49078
            1024       70535         97136       84974 - 111582

                    -- Overwriting the input data --

             FFT       Static        No            Dynamic
             Size      scaling       scaling       scaling

               8         280           297         285 -    302
              16         543           625         595 -    677
              32        1162          1433        1325 -   1596
              64        2627          3392        3062 -   3826
             128        6042          8019        7131 -   9108
             256       13864         18718       16504 -  21358
             512       31526         43033       37765 -  49272
            1024       70789         97397       85235 - 111843

        Note that the cycle count for dynamic scaling depends upon the
        number of stages at which intermediate results are scaled to
        avoid overflow. The cycle count will be in the range:

            cycle count for static scaling + ((log2(FFT size)-3)*2*FFT size)
            cycle count for     no scaling + ((log2(FFT size)-3)*2*FFT size)

        (Measured for an ADSP-BF532 using version 5.0.0.35 of the ADSP-BF5xx
        Family Simulator and includes the overheads involved in calling
        the library procedure as well as the costs associated with argument
        passing; the cycle counts do not include the costs associated with
        working around any known silicon anomalies).

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libGroup      = filter.h;
.FILE_ATTR libName       = libdsp;
.FILE_ATTR prefersMem    = internal;
.FILE_ATTR prefersMemNum = "30";
.FILE_ATTR libFunc       = __rfft_fr32;
.FILE_ATTR libFunc       = rfft_fr32;
.FILE_ATTR FuncName      = __rfft_fr32;
#endif

#include <sys/anomaly_macros_rtl.h>

#if !defined(__ADSPLPBLACKFIN__)
/* Suppress Assembler Warning ea5504 "Memory store instruction use may trigger
** hardware anomaly 05-00-0165". The run-time libraries are not required to
** workaround this anomaly.
*/
.MESSAGE/SUPPRESS 5504;
#endif

#if defined(__ADSPLPBLACKFIN__)
/* Suppress Assembler Warning ea5508 "Memory load instruction use may trigger
** hardware anomaly 05-00-0245". All memory accesses in the shadow of a
** conditional branch are reads from the output buffer which has been written
** to previously (and thus can safely be assumed to be neither reserved nor
** illegal memory).
*/
.MESSAGE/SUPPRESS 5508;
#endif


/* Location of input arguments on the stack */

#define  OFFSET_IN_DATA         40
#define  OFFSET_IN_ALTERNATE   (OFFSET_IN_DATA - 28)

#define  IN_STRIDE        ( OFFSET_IN_DATA )
#define  IN_FFTSIZE       ( OFFSET_IN_DATA +  4 )
#define  IN_BLOCKEXPO     ( OFFSET_IN_DATA +  8 )
#define  IN_SCALING       ( OFFSET_IN_DATA + 12 )

/* Placement of temporary data on the stack */

#define  CONST_VAL            0
#define  CURRENT_BLOCKEXPO    4
#define  SIZE_TEMP_DATA      (CURRENT_BLOCKEXPO + 4)


.GLOBAL __rfft_fr32;
.TYPE   __rfft_fr32,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

__rfft_fr32:

   /* Function Prologue */

      [--SP] = (R7:4, P5:3);         // Save reserved registers

      P1 = R0;                       // Address input array

      B0 = R1;                       // Address output buffer
      B1 = R1;
      B2 = R1;
      I0 = R1;                       // Pointer to output data
      P0 = R1;

      B3 = R2;                       // Address twiddle table

      R4 = [SP+IN_STRIDE];           // Twiddle stride
      R5 = [SP+IN_FFTSIZE];          // FFT size
      R6 = [SP+IN_SCALING];          // Scaling method

      SP += -SIZE_TEMP_DATA;         // Allocate space on the stack for
                                     // local data

      R7 = R5 << 3;                  // Size output buffer

      CC = R4 <= 0;
      IF CC JUMP .done;              // Exit if twiddle stride <= 1

      R3 = 8;
      CC = R5 < R3;
      IF CC JUMP .done;              // Exit if FFT size < 8

      L0 = R7;                       // Configure as circular buffer
      L1 = R7;                       // Configure as circular buffer
      L2 = R7;                       // Configure as circular buffer

      R7.L = 2;
      R7.H = 0x80;
      [SP+CONST_VAL] = R7;

      // Convert scaling method
      // 2 (dynamic)        -> 2
      // 3 (no scaling)     -> 0
      //   otherwise (static) -> 1

      CC = R6 == 2;
      IF CC JUMP .dynamic;

      CC = R6 == 3;
      CC = !CC;
      R6 = CC;

.dynamic:
      [SP+(IN_SCALING+SIZE_TEMP_DATA)] = R6;
                                     // Store converted scaling method


   /* Bit-reversal */

      // Divide the output array into four equally sized bins.
      // Compute bit-reversed offset for FFT size / 4 and use
      // offsets p2 and p3 to load four complex input data in
      // bit-reversed order.

      // Configuration

      R2 = R5 >> 2;                  // N/4
      R2 += -1;                      // Loop counter Stage 1 = (N/4)-1
      P4 = R2;

      R2 = R5 >> 1;
      P5 = R2;                       // ((N/2)*4)/4

      CC = P0 == P1;
      IF CC JUMP .bitrev_one_buffer;


.bitrev_two_buffers:

      // Copy data from the input buffer (real valued) into the
      // output buffer (place data into the location of the imaginary
      // data to match the behaviour of the one buffer bit reversal
      // algorithm) in bit-reversed order.
      // At the same time, scan the input data to detect whether
      // dynamic scaling is required for the first stage.
      //
      // R7.L = Reference exponent
      //        No dynamic scaling is required if
      //              0xE0000000 < input data < 0x20000000
      //        => exponent(0x1FFFFFFF) - 1 = 2

      R2 = R5 << 1;
      P2 = R2;                       // Offset N/2

      R2 = -R5;
      P3 = R2;                       // Offset -N/4

      M0 = 8;                        // Stride real[i] -> real[i+1]
      P0 = P1;

#if WA_05000209
      I0 += 4;
      R7.H = R7.H - R7.H (NS)  || R0 = [P1++P2];
      R5   = PACK (R7.H, R7.L) || R1 = [P1++P3] || [I0++M0] = R0;
      R7.L = EXPADJ (R0, R7.L) || R2 = [P1++P2] || [I0++M0] = R1;
      R5.L = EXPADJ (R1, R5.L) || R3 = [P1]     || [I0++M0] = R2;
      R7.L = EXPADJ (R2, R7.L);
#else
      R7.H = R7.H - R7.H (NS)  || I0 += 4;
      R5   = PACK (R7.H, R7.L) || R0 = [P1++P2];
      R7.L = EXPADJ (R0, R7.L) || R1 = [P1++P3] || [I0++M0] = R0;
      R5.L = EXPADJ (R1, R5.L) || R2 = [P1++P2] || [I0++M0] = R1;
      R7.L = EXPADJ (R2, R7.L) || R3 = [P1]     || [I0++M0] = R2;
#endif

      // Loop for (N/4)-1
      LSETUP(.bitrev2_start, .bitrev2_end) LC0 = P4;

         P4 = 0;                     // Bit-reversal offset
         P4 += P5 (BREV);

.bitrev2_start:
         P1 = P0 + P4;

#if WA_05000209
                                      R0 = [P1++P2] || [I0++M0] = R3;
         R7.L = EXPADJ (R3, R7.L ) || R1 = [P1++P3] || [I0++M0] = R0;
         R5.L = EXPADJ (R0, R5.L ) || R2 = [P1++P2] || [I0++M0] = R1;
         R7.L = EXPADJ (R1, R7.L ) || R3 = [P1]     || [I0++M0] = R2;
         R5.L = EXPADJ (R2, R5.L );
#else
         R7.L = EXPADJ (R3, R7.L ) || R0 = [P1++P2] || [I0++M0] = R3;
         R5.L = EXPADJ (R0, R5.L ) || R1 = [P1++P3] || [I0++M0] = R0;
         R7.L = EXPADJ (R1, R7.L ) || R2 = [P1++P2] || [I0++M0] = R1;
         R5.L = EXPADJ (R2, R5.L ) || R3 = [P1]     || [I0++M0] = R2;
#endif

.bitrev2_end:
         P4 += P5 (BREV);


      R7.L = EXPADJ (R3, R7.L ) || [I0++M0] = R3;
      JUMP .bitrev_common;


.bitrev_one_buffer:

      // Copy data from the input buffer (real valued) into the output buffer
      // in bit-reversed order. By processing the input data back to front
      // the data with odd indices can be copied safely to the second half of
      // the buffer since at this time it does not contain any valid data. In
      // addition, moving all data located at the odd indices frees up memory
      // in the first half of the buffer to move the data with even indices.
      // Thus the one buffer bit-reversal can be performed without the need
      // for any conditional copy operations.
      // At the same time, scan the input data to detect whether
      // dynamic scaling is required for the first stage.
      //
      // R7.L = Reference exponent
      //        No dynamic scaling is required if
      //              0xC0000000 <= input data <= 0x3FFFFFFF
      //        => exponent(0x3FFFFFFF) - 1 = 1

      P3 = R5;                       // Offset N/4

      R2 = R5 << 1;
      R2 = -R2;
      P2 = R2;                       // Offset -N/2

      R7.H = R7.H - R7.H (NS)
      || I0 -= 4;                    // I0 points to the last imaginary element
                                     // in the complex valued input buffer

      P1 = P1 + (P3 << 2);
      P1 += -4;                      // P1 points to the last element
                                     // in the real valued input buffer

      M0 = -8;                       // Stride real[i] -> real[i+1]

      P0 = P1;

#if WA_05000209
                                  R0 = [P1++P2];
      R5   = PACK (R7.H, R7.L) || R1 = [P1++P3] || [I0++M0] = R0;
      R7.L = EXPADJ (R0, R7.L) || R2 = [P1++P2] || [I0++M0] = R1;
      R5.L = EXPADJ (R1, R5.L) || R3 = [P1]     || [I0++M0] = R2;
      R7.L = EXPADJ (R2, R7.L);
#else
      R5   = PACK (R7.H, R7.L) || R0 = [P1++P2];
      R7.L = EXPADJ (R0, R7.L) || R1 = [P1++P3] || [I0++M0] = R0;
      R5.L = EXPADJ (R1, R5.L) || R2 = [P1++P2] || [I0++M0] = R1;
      R7.L = EXPADJ (R2, R7.L) || R3 = [P1]     || [I0++M0] = R2;
#endif

      // Loop for (N/4)-1
      LSETUP(.bitrev1_start, .bitrev1_end) LC0 = P4;

         P1 = P0;
         P4 = 0;                     // Bit-reversal offset
         P4 += P5 (BREV);

.bitrev1_start:
         P1 -= P4;

#if WA_05000209
                                      R0 = [P1++P2] || [I0++M0] = R3;
         R7.L = EXPADJ (R3, R7.L ) || R1 = [P1++P3] || [I0++M0] = R0;
         R5.L = EXPADJ (R0, R5.L ) || R2 = [P1++P2] || [I0++M0] = R1;
         R7.L = EXPADJ (R1, R7.L ) || R3 = [P1]     || [I0++M0] = R2;
         R5.L = EXPADJ (R2, R5.L );
#else
         R7.L = EXPADJ (R3, R7.L ) || R0 = [P1++P2] || [I0++M0] = R3;
         R5.L = EXPADJ (R0, R5.L ) || R1 = [P1++P3] || [I0++M0] = R0;
         R7.L = EXPADJ (R1, R7.L ) || R2 = [P1++P2] || [I0++M0] = R1;
         R5.L = EXPADJ (R2, R5.L ) || R3 = [P1]     || [I0++M0] = R2;
#endif

         P1 = P0;

.bitrev1_end:
         P4 += P5 (BREV);

      R7.L = EXPADJ (R3, R7.L ) || [I0] = R3;
      M0 = 8;                        // Stride real[i] -> real[i+1]


.bitrev_common:

      R7.L = R5.L + R7.L (NS)        // If either R5 or R7 == 0 => R7 = 0
      || R6 = [SP+(IN_SCALING+SIZE_TEMP_DATA)];

      R7 = R7 >> 2
      || P0 = [SP+(IN_FFTSIZE+SIZE_TEMP_DATA)];

      BITTGL( R7, 0 );               // IF EXPADJ == 0 => scale (R7 = 1)
                                     //              otherwise don't (R7 = 0)

      CC = R6 == 2;                  // Test for dynamic scaling
      IF CC R6 = R7;                 // R6 = 1 if scaling, 0 otherwise


   /* First stage */

      // At this stage, only need to compute the butterfly for the
      // real data. At the same time, the data need to be re-arranged
      // in memory - the real data are copied into their proper place
      // while the imaginary values for the top of the butterfly
      // operation are zero-filled.
      //
      // M0 expected to be preset to 8

                                     // I0 = Read pointer &out[0].im
      I1 = B0;                       // I1 = Write pointer &out[0].re

      R4 = PACK( R6.H, R6.H );

      R5 = R4 - R6 (NS)
      || R0 = [I0++M0];

      R0 = ASHIFT R0 BY R5.L
      || R1 = [I0++M0];

      R1 = ASHIFT R1 BY R5.L
      || R7 = W[SP+CONST_VAL] (Z);

      // Loop for N/2, computing one butterfly per iteration
      LSETUP(.stage1_start, .stage1_end) LC0 = P0 >> 1;
.stage1_start:
         R2 = R0 + R1, R3 = R0 - R1 (S) || R0 = [I0++M0];
         R0 = ASHIFT R0 BY R5.L         || R1 = [I0++M0] || [I1++] = R2;
         R7.L = EXPADJ (R2, R7.L )                       || [I1++] = R4;
         R1 = ASHIFT R1 BY R5.L                          || [I1++M0] = R3;
.stage1_end:
         R7.L = EXPADJ (R3, R7.L );


      R7 = R7 >> 1
      || [SP+CURRENT_BLOCKEXPO] = R6; // Store block exponent

      BITTGL( R7, 0 );               // IF EXPADJ == 0 => scale (R7 = 1)
                                     //              otherwise don't (R7 = 0)

      R6 = [SP+(IN_SCALING+SIZE_TEMP_DATA)];
                                     // Retrieve scaling method

      CC = R6 == 2;                  // Test for dynamic scaling
      IF CC R6 = R7;                 // R6 = 1 if scaling, 0 otherwise


  /* Second Stage */

      // Compute pairs of butterflies
      // First pair  - real butterfly only, imaginary part already set to 0.
      // Second pair - upper {x2re, 0} +/- (twiddle {0,-1} * lower {x4re, 0})
      //               = upper {x2re, 0} +/- lower' {0, -x4re}
      //               = {x2re +/- 0, 0 +/- -x4re}
      //               => upper'' = {x2re, -x4re}, lower'' = {x2re, x4re}
      //
      // P0 is expected to be set to N.
      // R4 is expected to contain 0x0.
      // M0 is expected to contain 0x8 = Stride real[i] -> real[i+1]

      I0 = B0;                       // Read pointer &out[0].re
                                     // I1 = Write pointer &out[0].re

      P0 = P0 >> 2;                  // Set loop counter (=N/4)

      R7 = W[SP+CONST_VAL] (Z);

      R6 = R4 - R6 (NS)
      || R0 = [I0++M0];

      R0 = ASHIFT R0 BY R6.L
      || R1 = [I0++M0];

      R1 = ASHIFT R1 BY R6.L
      || R2 = [I0++M0];

      // Loop for N/4, compute two butterflies in parallel (real inputs only)
      LSETUP(.stage2_start, .stage2_end) LC0 = P0;
.stage2_start:
         R2 = ASHIFT R2 BY R6.L         || R3 = [I0++M0];
         R0 = R0 + R2, R2 = R0 - R2 (S);
         R3 = ASHIFT R3 BY R6.L                          || [I1++M0] = R0;
         R5 = R4 + R3, R3 = R4 - R3 (S)                  || [I1++] = R1;
         R7.L = EXPADJ (R0, R7.L)       || R0 = [I0++M0] || [I1++] = R3;
         R0 = ASHIFT R0 BY R6.L                          || [I1++M0] = R2;
         R7.L = EXPADJ (R2, R7.L)       || R1 = [I0++M0] || [I1++] = R1;
.stage2_end:
         R1 = ASHIFT R1 BY R6.L         || R2 = [I0++M0] || [I1++] = R5;


      R7 = R7 >> 1
      || R5 = [SP+CURRENT_BLOCKEXPO];// Retrieve block exponent

      R5 = R5 - R6 (NS)
      || R6 = [SP+(IN_SCALING+SIZE_TEMP_DATA)];
                                     // Increment block exponent
                                     // Retrieve scaling method

      BITTGL( R7, 0 );               // IF EXPADJ == 0 => scale (R7 = 1)
                                     //              otherwise don't (R7 = 0)

      CC = R6 == 2;                  // Test for dynamic scaling
      IF CC R6 = R7;                 // R6 = 1 if scaling, 0 otherwise

      [SP+CURRENT_BLOCKEXPO] = R5;   // Store updated block exponent


   /* Remaining Stages */

      // M0 is expected to contain 0x8 = Stride real[i] -> real[i+1]

      P1 = 4;                        // Set loop counter butterflies

      R3 = [SP+(IN_FFTSIZE+SIZE_TEMP_DATA)];

      R4 = [SP+(IN_STRIDE+SIZE_TEMP_DATA)];
      R3 *= R4;                      // Twiddle stride = (((N/2)/4)*8)*stride
      P5 = R3;

      R4 = R3 << 2
      || R7 = [SP+CONST_VAL];

      I3 = B3;
      L3 = R4;                       // Twiddle table as circular buffer

      I0 = B0;                       // Read/Write pointer upper half butterfly
                                     // I1 = Read pointer lower half butterfly
      I2 = B0;                       // Write pointer lower half butterfly

      P2 =  1;                       // Const value
      P4 = 32;                       // Offset real[i] -> real[next butter]
      P3 = P0 << 2;                  // Loop counter scanning (==N)

.ALIGN 4
.iter_stages:

         // Loop for log2(fft_size) - 2

         M2 = P5;                    // Update twiddle stride

         P0 = P0 >> 1;               // Set loop counter stages

         M1 = P4;                    // Update offset

         P1 += -1;                   // First butterfly unrolled

         CC = R6 == 1;
         IF CC JUMP .groups_with_scaling;


#if WA_05000428
         // Speculative read from L2 by Core B may cause a write to L2 to fail
         NOP;
         NOP;
#endif

         // Loop for ngroups
         //   where ngroups = (fft_size)/8   (first iteration .iter_stages)
         //                 = (fft_size)/16  (second iteration .iter_stages)
         //                 = (fft_size)/32  (third iteration .iter_stages)
         //                 ...
         //                   = 1              (last iteration .iter_stages)

         LSETUP(.groups_ns_start, .groups_ns_end) LC0 = P0;
.groups_ns_start:

            // First butterfly unrolled

            R1 = [I0]     || I1 += M1;
            R0 = [I1++M0] || I2 += M1;
            R1 = R1 + R0, R0 = R1 - R0 (S) || R2 = [I1++]   || I3 += M2;

            R1 = R1 -|- R1 || R3 = [I1--]  || [I0++] = R1;
            R0 = R0 -|- R0 || R4 = [I3++]  || [I2++] = R0;

            // Loop for nbutters =  3  (first iteration .iter_stages)
            //                   =  7  (second iteration .iter_stages)
            //                   = 15  (third iteration .iter_stages)
            //                   ...
            //                   = (fft_size / 2) - 1}
            //                         (last iteration .iter_stages)

            LSETUP(.butterflies_ns_start, .butterflies_ns_end) LC1 = P1;
.butterflies_ns_start:

               // Real part of butterfly

#if defined(__ADSPLPBLACKFIN__)
               R1  = -R3 (S)               || R5 = [I3--]   || [I0++] = R1;
#else
               R1 = R1 -|- R1 (S)          || R5 = [I3--]   || [I0++] = R1;
               R1 = R1 - R3 (S);
#endif

               A1  = R4.H * R2.L (M);
               A1 += R1.H * R5.L (M), A0  = R1.L * R5.L (FU);
               A1 += R7.H * R7.H (M), A0 += R7.H * R7.H (FU);
               A1 -= R5.H * R3.L (M);
               A1 += R2.H * R4.L (M), A0 += R2.L * R4.L (FU);

               A0 = A0 >> 16;
               A0 += A1;
               A0 = A0 >>> 15              || R1 = [I0]     || [I2++] = R0;
               A0 += R2.H * R4.H;
               R0 = (A0 -= R3.H * R5.H);
               R1 = R1 + R0, R0 = R1 - R0 (S);

               // Imaginary part of butterfly
               A1  = R5.H * R2.L (M);
               A1 += R7.H * R7.H (M), A0  = R7.H * R7.H;
               A1 += R2.H * R5.L (M), A0 += R2.L * R5.L (FU);
               A1 += R3.H * R4.L (M), A0 += R3.L * R4.L (FU);
               A1 += R4.H * R3.L (M);
               A0 = A0 >> 16;
               A0 += A1                    || [I0++] = R1   || I1 += M0;
               A0 = A0 >>> 15              || R1 = [I0]     || I3 += M2;
               A0 += R2.H * R5.H           || R2 = [I1++]   || [I2++] = R0;
               R0 = (A0 += R3.H * R4.H)    || R3 = [I1--]   || R4 = [I3++];

.butterflies_ns_end:
               R1 = R1 + R0, R0 = R1 - R0 (S);

            [I0++M1] = R1 || I3 -= 4;
.groups_ns_end:
            [I2++] = R0   || I0 += 4;

         JUMP .groups_finish;


.ALIGN 4
.groups_with_scaling:

         // Loop for ngroups
         //   where ngroups = (fft_size)/8   (first iteration .iter_stages)
         //                 = (fft_size)/16  (second iteration .iter_stages)
         //                 = (fft_size)/32  (third iteration .iter_stages)
         //                 ...
         //                 = 1              (last iteration .iter_stages)

         LSETUP(.groups_start, .groups_end) LC0 = P0;
.groups_start:

            // First butterfly unrolled

            R1 = [I0] || I1 += M1;
            R1 = R1 >>> 1                  || R0 = [I1++M0] || I2 += M1;
            R0 = R0 >>> 1                  || R2 = [I1++]   || I3 += M2;
            R1 = R1 + R0, R0 = R1 - R0 (S) || R3 = [I1--]   || R4 = [I3++];
            R1 = R1 -|- R1 || [I0++] = R1;
            R0 = R0 -|- R0 || [I2++] = R0;

            // Loop for nbutters =  3  (first iteration .iter_stages)
            //                   =  7  (second iteration .iter_stages)
            //                   = 15  (third iteration .iter_stages)
            //                   ...
            //                   = (fft_size / 2) - 1}
            //                         (last iteration .iter_stages)

            LSETUP(.butterflies_start, .butterflies_end) LC1 = P1;
.butterflies_start:

               // Real part of butterfly
               A1  = R2.H * R4.L (M), A0  = R2.H * R4.H (IS);
               A1 += R4.H * R2.L (M)       || R5 = [I3--]   || [I0++] = R1;
               A1 -= R3.H * R5.L (M)       || R1 = [I0]     || [I2++] = R0;
               A1 -= R5.H * R3.L (M), A0 -= R5.H * R3.H (IS);
               A1 = A1 >>> 16;
               R0 = (A0 += A1);
               R1 >>>= 1;
               R1 = R1 + R0, R0 = R1 - R0 (S);

               // Imaginary part of butterfly
               A1  = R5.H * R2.L (M), A0  = R5.H * R2.H (IS);
               A1 += R2.H * R5.L (M)       || [I0++] = R1   || I1 += M0;
               A1 += R3.H * R4.L (M), A0 += R3.H * R4.H (IS);
               A1 += R4.H * R3.L (M)       || R1 = [I0]     || [I2++] = R0;
               A1 = A1 >>> 16              || R2 = [I1++]   || I3 += M2;
               R0 = (A0 += A1)             || R3 = [I1--]   || R4 = [I3++];
               R1 >>>= 1;

.butterflies_end:
               R1 = R1 + R0, R0 = R1 - R0 (S);

            [I0++M1] = R1 || I3 -= 4;

.groups_end:
            [I2++] = R0   || I0 += 4;


.ALIGN 4
.groups_finish:

         R5 = [SP+CURRENT_BLOCKEXPO];// Retrieve block exponent

         R5.L = R5.L + R6.L (S)      // Increment block exponent
         || R6 = [SP+(IN_SCALING+SIZE_TEMP_DATA)];
                                     // and retrieve scaling method

         P5 = P5 >> 1;               // Twiddle stride / 2

         [SP+CURRENT_BLOCKEXPO] = R5;// Store updated block exponent

         CC = P0 == 1;
         IF CC JUMP .done;


         P1 = (P1 + P2) << 1;        // (Counter butterflies + 1) * 2

         P4 = P4 << 1;               // Offset read/write pointer * 2

         CC = R6 == 2;               // Test for dynamic scaling

         R7 = [SP+CONST_VAL];

         IF !CC JUMP .iter_stages (BP);


         // For dynamic scaling only, scan input data

#if WA_05000428
         // Speculative read from L2 by Core B may cause a write to L2 to fail
         NOP;
         NOP;
         NOP;
#endif

         R5 = PACK( R7.H, R7.L )
         || R0 = [I0++];

         R1 = [I0++];

         // Loop N times
         LSETUP(.scan_start, .scan_end) LC0 = P3;
.scan_start:
            R7.L = EXPADJ (R0, R7.L) || R0 = [I0++];
.scan_end:
            R5.L = EXPADJ (R1, R5.L) || R1 = [I0++];

         R6.L = R5.L + R7.L (NS)
         || I0 -= 4;

         R6 = R6 >> 2
         || I0 -= 4;

         BITTGL( R6, 0 );            // IF EXPADJ == 0 => scale (R6 = 1)
                                     //           otherwise don't (R6 = 0)

         JUMP .iter_stages;


   /* Function Epilogue */

.done:

      R2 = [SP+CURRENT_BLOCKEXPO];   // Extract block exponent

      P2 = [SP+(IN_BLOCKEXPO+SIZE_TEMP_DATA)];
                                     // Load &block exponent

      L0 = 0;                        // Disable circular buffers
      L1 = 0;
      L2 = 0;
      L3 = 0;

      SP += SIZE_TEMP_DATA;          // Release temporary data on the stack

      [P2] = R2;                     // Write count block exponent

      (R7:4, P5:3) = [SP++];         // Pop the registers before returning
      RTS;

.__rfft_fr32.end:

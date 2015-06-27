/*****************************************************************************
 *
 * ifftf_fr32.asm : $Revision: 1.4 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: ifftf_fr32 - inverse fast N-point FFT

    Synopsis:

        #include <filter.h>
        void ifftf_fr32 (const complex_fract32 input[], 
                         complex_fract32       output[],
                         const complex_fract32 twiddle_table[], 
                         int                   twiddle_stride, 
                         int                   fft_size); 

    Description:

        This function transforms the frequency domain complex input signal
        to the time domain by using the accelerated version of the Discrete
        Fourier Transform known as a Fast Fourier Transform or FFT. The 
        ifftf_fr32 function uses a mixed-radix algorithm.

        The size of the input array input and the output array output is 
        fft_size, where fft_size represents the number of points in the FFT. 
        The number of points in the FFT must be a power of 2 and must be at 
        least 8.

        The twiddle table is passed in the argument twiddle_table, which
        must contain at least 3*fft_size/4 complex twiddle factors. The
        table should be initialized with complex twiddle factors in which
        the real coefficients are positive cosine values and the imaginary
        coefficients are negative sine values. The function twidfftf_fr32
        may be used to initialize the array.

        If the twiddle table has been generated for an fft_size FFT, then
        the twiddle_stride argument should be set 1. On the other hand, if
        the twiddle table has been generated for an FFT of size x, where
        x > fft_size, then the twiddle_stride argument should be set to
        x / fft_size. The twiddle_stride argument therefore allows the
        same twiddle table to be used for different sizes of FFT. (The
        twiddle_stride argument cannot be either zero or negative).

        It is recommended that the output array not be allocated in the same 
        4K memory sub-bank as the input array or the twiddle table, as the 
        performance of the function may otherwise degrade due to data bank 
        collisions. 

        The function uses static scaling of intermediate results to prevent 
        overflow, and the final output therefore is scaled by 1/fft_size. 

    Error Conditions:

        The ifftf_fr32 function returns if the FFT size is less than 8
        or if the twiddle stride is less than one.

    Algorithm:

        The function uses a mixed-radix algorithm (radix-4 and radix-2). 

    Implementation:

        The function consists of three parts. In the first part, data are read
        from the input buffer in bit-reversed order and the first two stages
        of the FFT are computed using a radix-4 algorithm. The result is stored
        in the output buffer.

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

        The second part is a radix-4 implementation, used to handle all 
        remaining stages for FFT sizes that are a power of 4.

        For example part of a stage 3 & 4 butterfly of a 16 point FFT:

                x( 1)  \/       x'( 1)  \  /  x''( 1)
           w2 * x( 5)  /\       x'( 5)   \/            \  / x''( 5)  
                                         /\             \/
                x( 9)  \/  w1 * x'( 9)  /  \  x''( 9)   /\
           w2 * x(13)  /\  w5 * x'(13)                 /  \ x''(13)

           x''( 1) = x(1) + w2 * x(5) + w1 * (x(9) + w2 * x(13)) 
                   = x(1) + w2 * x(5) + w1 * x9 + w3 x(13)

           x''( 5) = x(1) - w2 * x(5) + w5 * (x(9) - w2 * x(13))
                   = x(1) - w2 * x(5) + w5 * x9 - w7 x(13)
                   = x(1) - w2 * x(5) + w4 * (w1 * x9) - w4 * (w3 * x(13))
                   = x(1) - w2 * x(5) + {-Im,Re}(w1 * x9) - {-Im,Re}(w3 * x(13))

           x''( 9) = x(1) + w2 * x(5) - w1 * (x(9) + w2 * x(13))
                   = x(1) + w2 * x(5) - w1 * x9 - w3 x(13)

           x''(13) = x(1) - w2 * x(5) - w5 * (x(9) - w2 * x(13))
                   = x(1) - w2 * x(5) - w5 * x9 + w7 x(13)
                   = x(1) - w2 * x(5) - w4 * (w1 * x9) + w4 * (w3 * x(13))
                   = x(1) - w2 * x(5) - {-Im,Re}(w1 * x9) + {-Im,Re}(w3 * x(13))
 
        Based on:
           { w2 } = { cos( 2*pi*2 / N ), -sin( 2*pi*2 / N ) } 
           { w5 } = { cos( 2*pi*5 / N ), -sin( 2*pi*5 / N ) }
           w2 * w5 = {  cos( 2*pi*2 / N ) *  cos( 2*pi*5 / N ) - 
                       -sin( 2*pi*2 / N ) * -sin( 2*pi*5 / N ),
                        cos( 2*pi*2 / N ) * -sin( 2*pi*5 / N ) +
                       -sin( 2*pi*2 / N ) *  cos( 2*pi*5 / N ) }
                   = {  cos( a ) *  cos( b ) -  sin( a ) * sin( b ),
                        cos( a ) * -sin( b ) + -sin( a ) * cos( b ) } 
                   = {  cos( a + b ), -sin( a + b ) }
                   = {  cos( 2*pi*2/N + 2*pi*5/N ), 
                       -sin( 2*pi*2/N + 2*pi*5/N )  }
                   = {  cos( 2*pi*7/N ), -sin( 2*pi*7/N ) }
                   = w7  

        The last part is a radix-2 implementation, used to handle the last
        stage for FFT sizes that are a power of 2 but not a power of 4. 
 
    Example:

        #include <filter.h>
        #define FFT_SIZE1   32
        #define FFT_SIZE2  256
        #define TWID_SIZE  ((3 * FFT_SIZE2) / 4)

        complex_fract32  in1[FFT_SIZE1], in2[FFT_SIZE2];
        complex_fract32  out1[FFT_SIZE1], out2[FFT_SIZE2];
        complex_fract32  twiddle[TWID_SIZE];

        twidfftf_fr32 (twiddle, FFT_SIZE2);
  
        ifftf_fr32 (in1, out1, twiddle, 
                    FFT_SIZE2/FFT_SIZE1, FFT_SIZE1); 

        ifftf_fr32 (in2, out2, twiddle,
                    1, FFT_SIZE2);

    Cycle Counts:

        Size FFT  Cycles

             8      217
            16      447
            32     1011 
            64     2203
           128     5315
           256    11747 
           512    27643
          1024    60334

        (Measured for an ADSP-BF532 using version 5.0.0.35 of the ADSP-BF5xx
        Family Simulator and includes the overheads involved in calling
        the library procedure as well as the costs associated with argument
        passing).

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libGroup      = filter.h;
.FILE_ATTR libName       = libdsp;
.FILE_ATTR prefersMem    = internal;
.FILE_ATTR prefersMemNum = "30";
.FILE_ATTR libFunc       = __ifftf_fr32;
.FILE_ATTR libFunc       = ifftf_fr32;
.FILE_ATTR FuncName      = __ifftf_fr32;

/* Called by ifft2d_fr32 */
.FILE_ATTR libFunc  = ifft2d_fr32;
.FILE_ATTR libFunc  = __ifft2d_fr32;
.FILE_ATTR FuncName = __ifft2d_fr32;
#endif


#if !defined(__ADSPLPBLACKFIN__)
/* Suppress Assembler Warning ea5504 "Memory store instruction use may trigger
** hardware anomaly 05-00-0165". The run-time libraries are not required to
** workaround this anomaly.
*/

.MESSAGE/SUPPRESS 5504;
#endif

/* Location of input arguments on the stack */

#define  OFFSET_IN_DATA     40
#define  IN_STRIDE        (OFFSET_IN_DATA)
#define  IN_FFTSIZE       (OFFSET_IN_DATA +  4)

/* Placement of temporary data on the stack  */

//    BASE_T         - base address twiddle table
//    XRE_T2         - real valued input to the second butterfly 
//                     of a radix-4 algorithm
//    BFU_13, BFL_13 - temporary storage of intermediate results
//                     of a radix-4 algorithm
//
#define  BASE_T       0
#define  XRE_T2       4
#define  BFU_13       8
#define  BFL_13      12

#define  SIZE_TEMP_DATA   (BFL_13 + 4)

.GLOBAL __ifftf_fr32;
.TYPE   __ifftf_fr32,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

__ifftf_fr32:

   /* Function Prologue */

      [--SP] = (R7:4, P5:3);         // Save reserved registers

      P0 = R0;                       // Address input array
      P1 = R0;

      B1 = R1;                       // Address output buffer
      B0 = R1;                       // Address output buffer
      I0 = R1;                       // Pointer to output data

      B3 = R2;                       // Address twiddle table

      R4 = [SP+IN_STRIDE];           // Twiddle stride
      R5 = [SP+IN_FFTSIZE];          // FFT size

      R7 = R5 << 3;                  // Size input / output buffer

      CC = R4 <= 0;
      IF CC JUMP .done;              // Exit if twiddle stride <= 1

      R3 = 8;
      CC = R5 < R3;
      IF CC JUMP .done;              // Exit if FFT size < 8

      L0 = R7;                       // Configure as circular buffer
      L1 = R7;                       // Configure as circular buffer

   /* Bit-reversal, First & Second Stage */

      // Divide the output array into four equally sized bins.
      // Compute bit-reversed offset for FFT size / 4 and use
      // offsets p2 and p3 to load four complex input data in 
      // bit-reversed order.

      // Configuration

      R2 = R5 >> 2;                  // N/4
      R2 += -1;                      // Loop counter Stage 1 = (N/4)-1
      P4 = R2;

      R2 = R5 << 2;
      R2 += -4;
      P2 = R2;                       // Offset N/2 - 1
 
      R2 = R5 << 1;
      R2 = -R2;
      R2 += -4; 
      P3 = R2;                       // Offset -N/4 - 1

      P5 = R5;                       // (N*4)/4

      R0 = [P1++];
      R0 = R0 >>> 1 || R1 = [P1++P2];
      R1 = R1 >>> 1 || R2 = [P1++];
      R2 = R2 >>> 1 || R3 = [P1++P3];
      R3 >>>= 1;
      R0 = R0 + R2, R2 = R0 - R2 (S) || R4 = [P1++];  
      R1 = R1 + R3, R3 = R1 - R3 (S) || R5 = [P1++P2];
      R0 = R0 >>> 1 || R6 = [P1++];
      R1 = R1 >>> 1 || R7 = [P1];
      R2 >>>= 1;
      R3 >>>= 1;

      P1 = P0 + P5;

      // Loop for (N/4)-1
      LSETUP(.bitrev_start,.bitrev_end) LC0 = P4;

         P4 = 0;                     // Bit-reversal offset
         P4 += P5 (BREV);

.bitrev_start:
         R4 >>>= 1;
         R5 >>>= 1;
         R6 >>>= 1;
         R7 >>>= 1;
         R4 = R4 + R6, R6 = R4 - R6 (S);
         R5 = R5 + R7, R7 = R5 - R7 (S); 
         R4 >>>= 1;
         R5 >>>= 1;
         R6 >>>= 1;
         R7 >>>= 1;
         R0 = R0 + R4, R4 = R0 - R4 (S);
         R1 = R1 + R5, R5 = R1 - R5 (S)                  || [I0++] = R0; 
         R7 = R2 + R7, R2 = R2 - R7 (S) || R0 = [P1++]   || [I0++] = R1; 
         R3 = R3 + R6, R6 = R3 - R6 (S) || R1 = [P1++P2] || [I0++] = R2;
         R0 = R0 >>> 1                  || R2 = [P1++]   || [I0++] = R3;
         R1 = R1 >>> 1                  || R3 = [P1++P3] || [I0++] = R4;
         R2 = R2 >>> 1                  || R4 = [P1++]   || [I0++] = R5;
         R3 = R3 >>> 1                  || R5 = [P1++P2] || [I0++] = R7; 
         R0 = R0 + R2, R2 = R0 - R2 (S)                  || [I0++] = R6; 
         R1 = R1 + R3, R3 = R1 - R3 (S) || R6 = [P1++];
         R0 = R0 >>> 1                  || R7 = [P1];
         R1 >>>= 1;
         R2 >>>= 1;
         R3 >>>= 1;

         P4 += P5 (BREV);

.bitrev_end:
         P1 = P0 + P4;

      R4 >>>= 1;
      R5 >>>= 1;
      R6 >>>= 1;
      R7 >>>= 1;
      R4 = R4 + R6, R6 = R4 - R6 (S);
      R5 = R5 + R7, R7 = R5 - R7 (S);
      R4 >>>= 1;
      R5 >>>= 1;
      R6 >>>= 1;
      R7 >>>= 1;
      R0 = R0 + R4, R4 = R0 - R4 (S);
      R1 = R1 + R5, R5 = R1 - R5 (S) || [I0++] = R0; 
      R7 = R2 + R7, R2 = R2 - R7 (S) || [I0++] = R1; 
      R3 = R3 + R6, R6 = R3 - R6 (S) || [I0++] = R2; 

      R0 = [SP+IN_FFTSIZE]           || [I0++] = R3;
      R3 = [SP+IN_STRIDE]            || [I0++] = R4;
      R1 = R0 << 1                   || [I0++] = R5;
      R2 = R0 << 2                   || [I0++] = R7; 
      R4 = R0 >> 4                   || [I0++] = R6;

      R5 = R1 + R2;

      I3 = B3;                       // Configure twiddle table
                                     // as circular buffer

      R5 *= R3;                      // Size twiddle table 
                                     // (=3/4 * N * 8 * stride)

      L3 = R5;

      CC = R4 == 0;                            
      IF CC JUMP .last_stage_rad2;   // Skip middle stages
                                     // for FFT size 8

   /* Radix-4 Stages */

      // P0 = Loop counter stages
      // P5 = Loop counter butterflies / stage
      //
      // I0 = read / write pointer real data
      // I1 = read / write pointer imaginary data     
      // M0 = read / write offset (preset to  4 * 8)
      // M1 = read / write offset (preset to -4 * 8)
      //
      // P1 = twiddle table (offset == 1 * (FFT size N/16 * 8 * stride)
      // P2 = twiddle table (offset == 2 * (FFT size N/16 * 8 * stride)
      // P3 = twiddle table (preset to 1 * offset) 
      // I3 = twiddle table (offset == 3 * (FFT size N/16 * 8 * stride)
      // M2 = twiddle table (preset to 3 * offset)
      // P4 = temp

      R0 >>= 1;

      SP += -SIZE_TEMP_DATA;         // Allocate space on the stack for
                                     // local data

      R3 *= R0;
      P3 = R3;                       // offset twiddle table

      P0 = R4;                       // P0 = Counter remaining stages
      P5 =  3;                       // P5 = Counter butterflies / stage

      I1 =  B1;
      M0 =  32;
      M1 = -32;

      P4 = P3 + (P3 << 1);

      R3 = B3;
      P1 = B3;
      P2 = B3;

      I1 += M1;
      I1 += M1;

      M2 = P4;
      [SP+BASE_T] = R3;

      // For a FFT size of 16, the algorithm will work as follows:      
      //
      // Compute the first butterfly (no twiddles involved), real values
      // first, followed by the imaginary ones:
      //     x0  = x0 + x4 + x8 + x12
      //     x4  = x0 - x4 + w4x8 - w4x12 (where w4x8 = {-ImX8,ReX8})
      //     x8  = x0 + x4 - x8 - x12
      //     x12 = x0 - x4 - w4x8 + w4x12
      //
      // Compute the remaining butterflies in the following order
      //     real(w3*x13)  
      //     real(w1*x9)  
      //     real(w1*x9)+real(w3*x13), real(w1*x9)-real(w3*x13)
      //
      //     real(w2*x5) 
      //     real(   x1) 
      //     real(x1)+real(w2*x5), real(x1)-real(w2*x5)
      //
      //     real(x1 + w2x5 + w1x9 + w3x13), real(x1 + w2x5 - w1x9 - w3x13)
      //
      //     write input[x9] to the stack[XRE_T2]
      //     write real(w1*x9 - w3*x13) to the stack[BFL_13]
      //     write real(x1 + w2x5 - w1x9 - w3x13) to output[x9]
      //     write real(x1 - w2*x5) to the stack[BFU_13]     
      //     write real(x1 + w2x5 + w1x9 + w3x13) to output[x1]
      //
      //     imag(w3*x13)
      //     imag(w1*x9)
      //     imag(w1*x9)+imag(w3*x13), imag(w1*x9)-imag(w3*x13)
      //
      //     imag(w2*x5)
      //     imag(   x1)
      //     imag(x1)+imag(w2*x5), imag(x1)-imag(w2*x5)
      //
      //     x1 - w2x5 + w4*w1x9 - w4*w3x13, x1 - w2x5 - w4*w1x9 + w4*w3x13
      //     write real(x1 - w2x5 - w4*w1x9 + w4*w3x13) to output[x13]  
      //     write real(x1 - w2x5 + w4*w1x9 - w4*w3x13) to output[x5] 
      //
      //     imag(x1 + w2x5 + w1x9 + w3x13), imag(x1 + w2x5 - w1x9 - w3x13) 
      //     write imag(x1 - w2x5 - w4*w1x9 + w4*w3x13) to output[x13]
      //     write imag(x1 + w2x5 - w1x9 - w3x13) to output[x9] 
      //     write imag(x1 - w2x5 + w4*w1x9 - w4*w3x13) to output[x5]
      //     write imag(x1 + w2x5 + w1x9 + w3x13) to output[x1]
      //

      // Loop for (stages - 2) / 2, where stages = log2(fft_size) 
.ALIGN 4;
.stages_rad4:

         // Loop for groups
         //   where groups = (fft_size)/16  (first iteration .stages_rad4)
         //                = (fft_size)/64  (second iteration .stages_rad4)
         //                = (fft_size)/256 (third iteration .stages_rad4)
         //                ...
         //                = 1 or 2         (last iteration .stages_rad4)
         //                  1 if fft_size power of 4, 2 otherwise

         LSETUP(.loop_groups_start, .loop_groups_end) LC0 = P0;
.loop_groups_start:
           
            // Compute first butterfly top-down, 
            // position twiddle pointers 

            R0 = [I0++M0] || I1 += 4;
            R0 = R0 >>> 1                  || R2 = [I0++M0] || I1 += M0;
            R2 = R2 >>> 1                  || R4 = [I0++M0] || I1 += M0;
            R4 = R4 >>> 1                  || R6 = [I0++M1];
            R6 >>>= 1;
            R0 = R0 + R2, R2 = R0 - R2 (S) || R1 = [P2++P3]; 
            R4 = R4 + R6, R6 = R4 - R6 (S) || R1 = [P2++P3];
            R0 >>>= 1;
            R2 >>>= 1; 
            R4 = R4 >>> 1                  || R1 = [P1++P3];
            R6 = R6 >>> 1                  || R1 = [I1++M0] || I3 += M2;
            R1 = R1 >>> 1                  || R3 = [I1++M0] || I0 += M1;
            R3 = R3 >>> 1                  || R5 = [I1++M0] || I0 += M1;
            R5 = R5 >>> 1                  || R7 = [I1++]   || I3 +=  4; 
            R7 >>>= 1;
            R1 = R1 + R3, R3 = R1 - R3 (S);
            R5 = R5 + R7, R7 = R5 - R7 (S);
            R7 >>>= 1;
            R0 = R0 + R4, R4 = R0 - R4 (S);
            R7 = R2 + R7, R2 = R2 - R7 (S) || [I0++M0] = R0 || I1 += 4;
            R2 = R1 >>> 1                  || [I0++M0] = R2 || R1 = [I1--];  
            R3 = R3 >>> 1                  || [I0++M0] = R4 || I1 -= 4;  
            R5 = R5 >>> 1                  || [I0++] = R7;
 
            R3 = R3 + R6, R4 = R3 - R6 (S) || R7 = [I3--]   || I0 += 4;
            R2 = R2 + R5, R5 = R2 - R5 (S) || R0 = [I0++M1] || R6 = [I3];
                                                                

            // Compute remaining butterflies bottom-up
            // computing the real values first

            A1  = R0.H * R6.L (M), A0  = R0.H * R6.H (IS)   || [I1++M1] = R4;
            A1 += R6.H * R0.L (M)                           || [I1++M1] = R5
                                                            || R4 = [P1++];

            // Loop for nbutters =  3  (first iteration .loop_butter)
            //                   = 15  (second iteration .loop_butter)
            //                   = 63  (third iteration .loop_butter)
            //                   ...
            //                   = (fft_size / 4) - 1
            //                         (last iteration .loop_butter)
            
            LSETUP(.loop_butter_start, .loop_butter_end) LC1 = P5;
.loop_butter_start:

               A1 += R1.H * R7.L (M)                         || [I1++M1] = R3 
                                                             || R5 = [P1--];
               A1 += R7.H * R1.L (M), A0 += R7.H * R1.H (IS) || [I1++M0] = R2;
               A1 = A1 >>> 16                                || I1 += M0;
               R6 = (A0 += A1)                               || R0 = [I0++M1]
                                                             || I1 += 4;
 
               A1  = R0.H * R4.L (M), A0  = R0.H * R4.H (IS) || I1 += 4;
               A1 += R4.H * R0.L (M)                         || R1 = [I1++M1];
               A1 += R1.H * R5.L (M)                         || [SP+XRE_T2] =R0;
               A1 += R5.H * R1.L (M), A0 += R5.H * R1.H (IS);
               A1 = A1 >>> 16                                || R0 = [I0++M1];
               R4 = (A0 += A1)                               || R2 = [P2++];
               R4 = R4 + R6, R6 = R4 - R6 (S);            
               R4 >>>= 1;
 
               A1  = R0.H * R2.L (M), A0  = R0.H * R2.H (IS) || R1 = [I1++M0]
                                                             || [SP+BFL_13] =R6;
               A1 += R2.H * R0.L (M)                         || R3 = [P2--];
               A1 += R1.H * R3.L (M)                         || I1 += M0; 
               A1 += R3.H * R1.L (M), A0 += R3.H * R1.H (IS) || R0 = [I0++M0];
               A1 = A1 >>> 16                                || I0 += M0;
               R2 = (A0 += A1)                               || R6 = [I3++]
                                                             || I0 += M0;
               R0 >>>= 1;               
               R3 = R0 + R2, R2 = R0 - R2 (S)                || R0 = [I0++M1];
               R3 >>>= 1;
               R3 = R3 + R4, R4 = R3 - R4 (S)                || R1 = [I1++M1]
                                                             || [SP+BFU_13] =R2;


               // Compute imaginary valued butterflies
 
               A1  = R6.H * R1.L (M), A0  = R6.H * R1.H (IS) || [I0++M1] = R4
                                                             || R7 = [I3--];
               A1 += R1.H * R6.L (M)                         || I0 += M1;  
               A1 -= R0.H * R7.L (M), A0 -= R0.H * R7.H (IS) || [I0++M0] = R3; 
               A1 -= R7.H * R0.L (M);
               A1 = A1 >>> 16                                || R4 = [P1++];
               R7 = (A0 += A1)                               || R1 = [I1++M1];
 
               A1  = R4.H * R1.L (M), A0  = R4.H * R1.H (IS) || R0 =[SP+XRE_T2];
               A1 += R1.H * R4.L (M)                         || R5 = [P1--];
               A1 -= R0.H * R5.L (M), A0 -= R0.H * R5.H (IS) || R4 = [P1++P3];
               A1 -= R5.H * R0.L (M)                         || R0 = [I0];
               A1 = A1 >>> 16                                || R1 = [I1++M1]
                                                             || I3 += M2;
               R5 = (A0 += A1)                               || R2 = [P2++];

               R5 = R5 + R7, R7 = R5 - R7 (S)                || R3 = [P2--];
               R5 = R5 >>> 1                                 || R4 = [P2++P3]; 
               R7 = R7 >>> 1                                 || R4 = [P2++P3];
 
               A1  = R2.H * R1.L (M), A0  = R2.H * R1.H (IS);
               A1 += R1.H * R2.L (M)                         || R1 = [I1++M0];
               A1 -= R0.H * R3.L (M), A0 -= R0.H * R3.H (IS) || I1 += M0; 
               A1 -= R3.H * R0.L (M)                         || I1 += M0;
               A1 = A1 >>> 16;
               R3 = (A0 += A1)                               || I1 += 4; 
 
               R1 = R1 >>> 1                                 || I1 += 4;
               R2 = R1 + R3, R3 = R1 - R3 (S)                || R4 =[SP+BFU_13];
 
               R4 >>>= 1;
               R7 = R4 + R7, R4 = R4 - R7 (S)                || R1 = [I1--];
               R2 = R2 >>> 1                                 || [I0++M0] = R4
                                                             || I1 -= 4;
               R2 = R2 + R5, R5 = R2 - R5 (S)                || I0 += M0;
                                                             
               R3 = R3 >>> 1                                 || [I0++] = R7 
                                                             || R6 =[SP+BFL_13];
               R4 = R6 >>> 1                                 || R6 = [I3++]
                                                             || I0 += 4;
               R3 = R3 + R4, R4 = R3 - R4 (S)                || R0 = [I0++M1]; 
 
               A1  = R0.H * R6.L (M), A0  = R0.H * R6.H (IS) || [I1++M1] = R4
                                                             || R4 = [P1++];
.loop_butter_end:
               A1 += R6.H * R0.L (M)                         || R7 = [I3--]
                                                             || [I1++M1] = R5;

            [I1++M1] = R3
            || P1 = [SP+BASE_T];     // reset twiddle pointer 

            [I1++M0] = R2
            || P2 = [SP+BASE_T];     // reset twiddle pointer

.loop_groups_end:
            R0 = [I0 ++ M0]
            || I1 += 4;


         P4 = 1;
         R3 = P0;                    // Preserve counter
         P0 = P0 >> 2;               // Counter stages / 4

         R4 = M0;
         R5 = M1;
         R6 = P3;
         R7 = M2;

         P5 = (P5 + P4) << 2;        // (butterflies / stage + 1) * 4 
 
         R5 = R5 << 2                // offset read/write pointer * 4
         || I1 += M0;                // position I1
 
         M1 = R5;
 
         R4 = R4 << 2                // offset read/write pointer * 4
         || I1 += M0;                // position I1
 
         M0 = R4;

         P5 += -1;
 
         CC = P4 <= P0;
 
         R6 = R6 >> 2                // offset twiddle table / 4
         || I1 += M1;                // position I1

         R7 = R7 >> 2                // offset twiddle table / 4
         || I1 += M1;                // position I1
 
         P3 = R6;
         M2 = R7;
 
         IF CC JUMP .stages_rad4 (BP);  // Iterate radix-4 stages while P0 > 0
 
 
      SP += SIZE_TEMP_DATA;          // Release temporary data on the stack
 
      CC = R3 == 1;                
 
      R0 = [SP+IN_FFTSIZE];          // (Avoid 05-00-0245 warning by loading
                                     // the FFT size *before* the jump below)
 
      IF CC JUMP .done;              // Skip last stage if FFT size power of 4
 
 
      R2 = R0 << 2;


   /* Last Stage Radix-2 */

.last_stage_rad2:

      // R0 preset to FFT size N
      // R2 preset to FFT size N/2 * 8 
      // I0 preset to B0  ( Read/Write pointer upper half butterfly )

      R0 += -2;                      
      P1 = R0;                       // Set loop counter ((N/2)-1)

      R3 = B0;

      R2 = R3 + R2 (NS)
      || R6 = [SP+IN_STRIDE];                

      I1 = R2;                       // Read pointer lower half butterfly
      I2 = R2;                       // Write pointer lower half butterfly

      R6 = R6 << 3;
      M2 = R6;                       // Set twiddle offset = stride * 8


      // First butterfly unrolled 
      R2 = [I0++]; 
      R2 = R2 >>> 1                  || R3 = [I0--];
      R3 = R3 >>> 1                  || R0 = [I1++];
      R0 = R0 >>> 1                  || R1 = [I1++];
      R2 = R2 + R0, R0 = R2 - R0 (S) || I3 += M2;
      R1 = R1 >>> 1                  || [I0++] = R2 || R4 = [I3++];
      R3 = R3 + R1, R2 = R3 - R1 (S) || [I2++] = R0 || R5 = [I3--];
      R0 = [I1++];

      // Loop for (N/2)-1
      LSETUP(.last_radix2_start, .last_radix2_end) LC1 = P1 >> 1;
.last_radix2_start:

         // Real part of butterfly 
         A1  = R0.H * R4.L (M), A0  = R0.H * R4.H (IS) || [I2++] = R2  
                                                       || R1 = [I1++];
         A1 += R4.H * R0.L (M);
         A1 += R1.H * R5.L (M);
         A1 += R5.H * R1.L (M), A0 += R5.H * R1.H (IS) || [I0++] = R3
                                                       || I3 += M2;
         A1 = A1 >>> 16                                || R3 = [I0];
         R2 = (A0 += A1);
         R3 >>>= 1;
         R3 = R3 + R2, R2 = R3 - R2 (S);

         // Imaginary part of butterfly 
         A1  = R1.H * R4.L (M), A0  = R1.H * R4.H (IS) || [I0++] = R3; 
         A1 += R4.H * R1.L (M)                         || [I2++] = R2
                                                       || R3 = [I0];
         A1 -= R5.H * R0.L (M), A0 -= R5.H * R0.H (IS);
         A1 -= R0.H * R5.L (M);
         A1 = A1 >>> 16                                || R4 = [I3++];
         R2 = (A0 += A1)                               || R5 = [I3--];
         R3 >>>= 1;

.last_radix2_end:
         R3 = R3 + R2, R2 = R3 - R2 (S)                || R0 = [I1++];

      [I0] = R3;
      [I2] = R2;


   /* Function Epilogue */

.done:

      L0 = 0;                        // Disable circular buffers
      L1 = 0;
      L3 = 0;

      (R7:4, P5:3) = [SP++];         // Pop the registers before returning
      RTS;                           // Return

.__ifftf_fr32.end:

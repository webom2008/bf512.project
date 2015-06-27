/*****************************************************************************
 *
 * rfftf_fr32.asm : $Revision: 1.5 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: rfftf_fr32 - fast N-point real input FFT

    Synopsis:

        #include <filter.h>
        void rfftf_fr32 (const fract32         input[], 
                         complex_fract32       output[],
                         const complex_fract32 twiddle_table[], 
                         int                   twiddle_stride, 
                         int                   fft_size); 

    Description:

        This function transforms the time domain real input signal to the
        frequency domain by using the accelerated version of the Discrete
        Fourier Transform known as a Fast Fourier Transform or FFT. The
        rfftf_fr32 function decimates in frequency using a mixed-radix
        algorithm. 

        The size of the input array input is fft_size, where fft_size
        represents the number of points in the FFT. The number of points
        in the FFT must be a power of 2 and must be at least 16.

        As the complex spectrum of a real FFT is symmetrical about the
        midpoint, the rfftf_fr32 function only generates the first
        (fft_size/2)+1 points of the FFT, and so the size of the output
        array output must be at least of length (fft_size/2) + 1. After
        returning from the rfftf_fr32 function, the output array will
        contain the following values: 

            - DC component of the signal in output[0].re (output[0].im = 0) 
            - First half of the complex spectrum in output[1] ...
              ... output[(fft_size/2)-1] 
            - Nyquist frequency in output[fft_size/2].re
              (and output[fft_size/2].im = 0) 

        Refer to the Example section below to see how an application would
        construct the full complex spectrum using the symmetry of a real FFT. 

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

        The rfftf_fr32 function returns if the FFT size is less than 16 
        or if the twiddle stride is less than one.

    Algorithm:

        The function uses a mixed-radix algorithm (radix-4 and radix-2). 

    Implementation:

        The function calls the cfftf_fr32 function, with the fft_size set to
        half the initial value. The twiddle stride is also adjusted to twice
        the initial value.

        Trigonometric recombination is then used to compute the final Fourier
        Transform:

            fx = cfftf_fr32(N/2);

            output[0] = fx[0].real + fx[0].imag;      // DC component

            output[N/2] = fx[0].real - fx[0].imag;    // Nyquist frequency

            for k = 1 : (N/4 - 1)

               sum.real  = (fx[k].real + fx[N/2-k].real ) / 2
               sum.imag  = (fx[k].imag + fx[N/2-k].imag ) / 2
               diff.real = (fx[k].real - fx[N/2-k].real ) / 2
               diff.imag = (fx[k].imag - fx[N/2-k].imag ) / 2

               output[k].real = sum.real + twiddle[k].real * sum.imag
                                         + twiddle[k].imag * diff.real
               output[k].imag = diff.imag - twiddle[k].real * diff.real
                                          + twiddle[k].imag * sum.imag

               /* Computing above for k = N/2-k => 
               **   sum remains unchanged, 
               **   diff = -diff
               **   twiddle[N/2-k].real = - twiddle[k].real
               **   twiddle[N/2-k].imag remains unchanged
               */
               output[N/2-k].real = sum.real - twiddle[k].real * sum.imag
                                             - twiddle[k].imag * diff.real
               output[N/2-k].imag = -diff.imag - twiddle[k].real * diff.real
                                               + twiddle[k].imag * sum.imag

            /* k = N/4 => fx[k] == fx[N/2-k], twiddle[k] = {0, -1} */
            output[N/4].real =  sum.real =  fx[N/4].real
            output[N/4].imag = -sum.imag = -fx[N/4].imag  

    Example:

        #include <filter.h>
        #include <complex.h> 

        #define FFTSIZE   32 
        #define TWIDSIZE  ((3 * FFTSIZE) / 4)

        fract32 sigdata[FFTSIZE];            /* input signal        */ 
        complex_fract32 r_output[FFTSIZE];   /* FFT of input signal */ 
        complex_fract32 twiddles[TWID_SIZE];

        int i; 

        /* Initialize the twiddle table */ 

        twidfftf_fr32 (twiddles,FFTSIZE);

        /* Calculate the FFT of a real signal */ 

        rfftf_fr32 (sigdata,r_output,twiddles,1,FFTSIZE); 

            /* (rfftf_fr32 sets r_output[FFTSIZE/2] to the Nyquist) */ 

        /* Add the 2nd half of the spectrum */ 

        for (i = 1; i < (FFTSIZE/2); i++) {
            r_output[FFTSIZE - i] = conj_fr32 (r_output[i]); 
        } 

    Cycle Counts:

        Size FFT   Cycles

            16      386
            32      764 
            64     1624
           128     3408
           256     7642 
           512    16442
          1024    37082

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
.FILE_ATTR libFunc       = __rfftf_fr32;
.FILE_ATTR libFunc       = rfftf_fr32;
.FILE_ATTR FuncName      = __rfftf_fr32;

/* Called by rfft2d_fr32 */
.FILE_ATTR libFunc  = rfft2d_fr32;
.FILE_ATTR libFunc  = __rfft2d_fr32;
.FILE_ATTR FuncName = __rfft2d_fr32;
#endif

#if !defined(__ADSPLPBLACKFIN__)
/* Suppress Assembler Warning ea5504 "Memory store instruction use may trigger
** hardware anomaly 05-00-0165". The run-time libraries are not required to
** workaround this anomaly.
*/

.MESSAGE/SUPPRESS 5504;
#endif

.EXTERN __cfftf_fr32;
.TYPE   __cfftf_fr32,STT_FUNC;

.GLOBAL __rfftf_fr32;
.TYPE   __rfftf_fr32,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

__rfftf_fr32:

   /* Function Prologue */

      [--SP] = (R7:4);               // Save reserved registers
      [--SP] = RETS;                 // Save return address
      SP += -20;

      R6 = PACK (R1.H, R1.L)         // Copy &Output
      || R4 = [SP+56];               // and load FFT size

      R4 = R4 >>> 1                  // FFT size / 2
      || R5 = [SP+52];               // and load twiddle stride
 
      R3 = R5 << 1                   // Twiddle stride * 2
      || [SP+16] = R4;               // and store updated FFT size

      R7 = PACK (R2.H, R2.L)         // Copy &Twiddle
      || [SP+12] = R3;               // and store updated twiddle stride

      R3 = 8;
      CC = R4 < R3;                  // Exit if FFT size < 16 
      IF CC JUMP .done;

      CC = R5 < 1;                   // Exit if twiddle stride < 1
      IF CC JUMP .done;

      CALL.X (__cfftf_fr32);

      R0 = R4 << 3;        
      R0 = R0 + R6;                  
      I0 = R6;                       // &Output[0] (Read/Write pointer)
      I1 = R0;                       // &Output[N/2] (Read/Write pointer)
      M1 = -12;                      // Offset Read/Write pointer

      R5 <<= 3;
      I3 = R7;                       // &Twiddle
      M2 = R5;                       // Offset twiddle table

      R4 += -1;                      
      P2 = R4;                       // Set loop counter (FFT size / 2) - 1


   /* Trigonometric Recombination */

      R7 = R7 -|- R7 || R0 = [I0++];
      R0 = R0 >>> 1  || R1 = [I0--];        
      R1 >>>= 1;
      R2 = R0 + R1, R3 = R0 - R1;
      [I0++] = R2;                   // &Output[0] = DC value
      [I0++] = R7;         
      [I1++] = R3;                   // &Output[N/2] = Nyquist
      [I1++M1] = R7;

      R5 = -1;
      R5 = R5 << 15 || R0 = [I0++]; 
      R0 = R0 >>> 1 || R1 = [I0--] || I3 += M2;
      R1 = R1 >>> 1 || R2 = [I1++];
      R2 = R2 >>> 1 || R3 = [I1--];
      R3 >>>= 1; 

      // Loop for (FFT Size/4)-2
      LSETUP(.trigrecomb_start,.trigrecomb_end) LC0 = P2 >> 1;

.trigrecomb_start:
         R0 = R0 + R2, R2 = R0 - R2 (S) || R6 = [I3++];
         R1 = R1 + R3, R3 = R1 - R3 (S) || R7 = [I3++M2]; 

         A1  = R6.H * R1.L (M), A0  = R6.H * R1.H (IS);
         A1 += R7.H * R2.L (M), A0 += R7.H * R2.H (IS);
         A1 -= R5.L * R0.L (M), A0 -= R5.L * R0.H (IS);
         A1 += R1.H * R6.L (M);
         A1 += R2.H * R7.L (M);
         A1 = A1 >>> 16;
         R4 = (A0 += A1); 

         A1  = R7.H * R1.L (M), A0  = R7.H * R1.H (IS);
         A1 -= R6.H * R2.L (M), A0 -= R6.H * R2.H (IS) || [I0++] = R4;
         A1 -= R5.L * R3.L (M), A0 -= R5.L * R3.H (IS);
         A1 += R1.H * R7.L (M);
         A1 -= R2.H * R6.L (M);
         A1 = A1 >>> 16;
         R4 = (A0 += A1);

         A1 = A0 = 0;   
         A1 -= R6.H * R1.L (M), A0 -= R6.H * R1.H (IS);
         A1 -= R7.H * R2.L (M), A0 -= R7.H * R2.H (IS) || [I0++] = R4;
         A1 -= R5.L * R0.L (M), A0 -= R5.L * R0.H (IS);
         A1 -= R1.H * R6.L (M);
         A1 -= R2.H * R7.L (M);
         A1 = A1 >>> 16;
         R4 = (A0 += A1);

         A1  = R7.H * R1.L (M), A0  = R7.H * R1.H (IS);
         A1 -= R6.H * R2.L (M), A0 -= R6.H * R2.H (IS) || [I1++] = R4;
         A1 += R5.L * R3.L (M), A0 += R5.L * R3.H (IS);
         A1 += R1.H * R7.L (M);
         A1 -= R2.H * R6.L (M);
         A1 = A1 >>> 16;
         R4 = (A0 += A1) || R0 = [I0++];

         R0 = R0 >>> 1   || R1 = [I0--] || [I1++M1] = R4;
         R1 = R1 >>> 1   || R2 = [I1++] || I3 -= 4;  
         R2 = R2 >>> 1   || R3 = [I1--];

.trigrecomb_end:
         R3 >>>= 1;


#if defined(__ADSPLPBLACKFIN__)
      R1 = -R1 (S) || [I0++] = R0;
#else
      R5 = R5 -|- R5 (S) || [I0++] = R0;
      R1 = R5 - R1 (S);
#endif
      [I0] = R1; 


   /* Function Epilogue */

.done:
      SP += 20;
      RETS = [SP++];                 // Restore return address
      (R7:4) = [SP++];               // Pop the registers before returning

      RTS;                           // Return

.__rfftf_fr32.end:

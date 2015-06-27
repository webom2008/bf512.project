/*****************************************************************************
  Copyright(c) 2000-2006 Analog Devices Inc.
  All rights reserved
******************************************************************************
  File Name      : rfft_fr16.asm

  Include File   : filter.h
  Label name     : __rfftN_fr16

  Description    : This file contains the code for a radix-2 implementation 
                   of real FFT. The algorithm used is Decimation in Time. 
                   The output is provided in normal order, with the output
                   values being scaled by a factor of 2 using static scaling.
      
                   The C callable prototype of the function is:

                       void rfft_fr16(
                                  const fract16  *in,       // Input array
                                  complex_fract16 *t,       // Temporary buffer
                                  complex_fract16 *out,     // Output array
                                  const complex_fract16 *w, // Twiddle table 
                                  int wst,                  // Twiddle stride
                                  int n,                    // Size FFT
                                  int block_exponent,       // Not used
                                  int scale_method);        // Not used   

                   The input, output and temporary array are expected to be 
                   of size n. For optimum performance, the input, output,
                   temporary and twiddle array should be located in different
                   memory banks.

                   If the input data can be overwritten, the optimum memory
                   usage can be achieved by also specifying the input array
                   as the output array, provided that the memory size of the
                   input array is at least 2*n.
                   
                   The twiddle table must contain at least n/2 elements. A
                   table generated for a larger sized radix-2 FFT can be 
                   used. In this case, the twiddle stride should be set to
                     wst = (FFT size table generated for) / (Size FFT invoked)
                   Otherwise the twiddle stride (wst) should be set to 1.

                   The function:
        
                       #include <filter.h>
                       void twidfftrad2_fr16 (complex_fract16 w[], int n);

                   can be used to generate the desired twiddle table. The
                   twiddle factors will be composed of +cosine and -sine
                   coefficients.

                   The arguments block_exponent and scale_method have been 
                   added for future expansion. At present, these arguments 
                   are ignored by the function. 

                   By default, assembling the source file will generate an
                   object with the entry point name __rfftN_fr16. By
                   defining the macro __RADIX_4_FFT on the command line, the
                   source can be configured as direct replacement for the
                   radix-4 FFT (entry point name _rfftrad4_fr16), which
                   was included up until VisualDSP++4.0. The radix-4 FFT
                   differs from the radix-2 implementation in that the
                   twiddle table is composed of +sine coefficients instead
                   of -sine coefficients.

  Restrictions   : The size of the FFT should be 8 or greater and a power of 2.

  Registers Used : R0-7, A0-1, P1-5, I0-3, B0-3, M0-2, L0-3 

  Cycle count    : N = 32     =   485 cycles
                   N = 256    =  4468 cycles
                   (BF532, Cycle Accurate Simulator)

  Code size      : 414 Bytes
******************************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libGroup      = filter.h;
#ifdef __RADIX_4_FFT
.file_attr libFunc       = __rfftrad4_fr16;
.file_attr libFunc       = rfftrad4_fr16;
.file_attr FuncName      = __rfftrad4_fr16;
#else  /* Radix-2 FFT */
.file_attr libFunc       = __rfftN_fr16;
.file_attr libFunc       = rfft_fr16;
.file_attr FuncName      = __rfftN_fr16;
#endif
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
#endif

.section    program;
#ifdef __RADIX_4_FFT
.global     __rfftrad4_fr16;
#else  /* Radix-2 FFT */
.global     __rfftN_fr16;
#endif

.align     2;
#ifdef __RADIX_4_FFT
__rfftrad4_fr16:
#else  /* Radix-2 FFT */
__rfftN_fr16:
#endif

/** Function Prologue **/

        [--SP] = (R7:4, P5:3);         // Save reserved registers
        B0 = R0;                       // Address Input array
        B1 = R1;                       // Address Temporary buffer
        B2 = R2;                       // Address Output array
        R0 = [SP+40];                  // Address Twiddle table
        R3 = [SP+44];                  // Twiddle stride
        R2 = [SP+48];                  // Size FFT

        R4 = 8;
        CC = R2 < R4;     
        If CC Jump Terminate;          // Exit if FFT size < 8 

        P1 = R3;                       // Preserve stride (wst)

        B3 = R0;                       // Address of Twiddle table
        R3<<= 1;                       // Length of Twiddle table = wst * 2
        R3*= R2;                       // Length of Twiddle table = wst * 2 * N
        L3 = R3;
            // This function will speculatively load values out of the
            // twiddle table that are beyond the end of the array - this
            // will cause an exception if the memory referenced does not
            // exist.
            //
            // The twiddle table is therefore accessed as a circular buffer.

/** Butterfly - First Stage **/

        I2 = B1;                      // Address Temporary array
        P4 = R2;                      // Size FFT
        M0 = R2;                      // Offset required for bit reversing
        
        R3.H = 0;                     // Clear high bit 
        R5.H = 0;                     // Clear high bit
 
        P5 = P4 >> 2;                 // Set loop counter to Size FFT/4
        R2.H = 0;                     // Clear high bit
        R4.H = 0;                     // Clear high bit
        
        M1 = B0;                      // Address Input array
        I0 = 0;                      
        I1 = M1;

        /* 
         * Read data from input array in bit-reversed order,
         * divide the data by 2 to avoid overflow
         * and perform first butterfly computation. 
         * During each iteration, 4 real values are loaded for processing. 
         * The complex results are written to the temporary buffer.
         *
         * At this stage, the twiddle coefficients can be ignored
         * since their value is always 1 for the real part and 
         * 0 for the imaginary part.
         */
        lsetup(Stage12_strt, Stage12_end) LC0 = P5; 
Stage12_strt:
           I0 += M0 (BREV) || R2.L = W[I1];    
           I1 = I0;
           I1 += M1;
           R2 = R2 >>> 1 (V) || I0 += M0 (BREV) || R3.L = W[I1];
           I1 = I0;
           R3 = R3 >>> 1 (V) || I1 += M1 || NOP;
           R0 = R2 +|+ R3, R1 = R2 -|- R3 (ASR) || I0 += M0 (BREV) || 
                                                         R4.L = W[I1];
           I1 = I0;
           R4 = R4 >>> 1 (V) || I1 += M1 || NOP;
           I0 += M0 (BREV) || R5.L = W[I1];
           R5 = R5 >>> 1 (V);
           R6 = R5 +|+ R4, R7 = R5 -|- R4 (ASR, CO);
           R0 = R0 +|+ R6, R6 = R0 -|- R6 (ASR);
           R1 = R1 +|+ R7, R7 = R1 -|- R7 (ASR) || [I2++] = R0 || NOP;
           I1 = I0;
           I1 += M1 || [I2++] = R1;
           [I2++] = R6;
Stage12_end:
           [I2++] = R7; 

        R1 = P1;                       // Twiddle stride wst
        R1 = R1 << 2;                  // R1 = wst * 4
        P3 = 4;                        // Loop counter number of lines in each 
                                       // butterfly at stage 3.
        R7 = P5;  
        R7 *= R1;                      // R7 = (4 * wst) * (Size FFT / 4) 
                                       //    = wst * Size FFT

        R2 = P4;                       // Size FFT   
        R3 = 8;                
        M1 = 16;                       // Offset for next butterfly
        P2 = P3 << 2;                  // P2 required to be 16 when branching
                                       // of to Esc_mid
        CC = R2 == R3;          
        If CC Jump Esc_mid;            // Skip middle stages for Size FFT == 8

        /* 
         * Find the number of iterations (=m) required 
         * to perform the middle stages of the butterfly 
         */
        R0 = 0;                        // Reset counter
Find_m:            
        R2 >>= 1;                      // S = Size FFT/2
        R0 += 1;                       // Increment counter m
        CC = R2 == R3;
        If !CC Jump Find_m (BP);       // Exit loop when S == 8


/** Butterfly - Middle Stages **/

        /* 
         * Loop for number of stages - 3 
         * Data are read from and written to the temporary buffer.
         */
Loopfor_m:
        I2 = B1;                       // Address Temporary buffer
        I1 = B1;             

        P0 = P3 << 2;                  // Number of lines per butterfly * 4
        M2 = P0;                       // Offset counterpart line
        P2 = P0 << 1;                  // Number of lines per butterfly * 8
        P1 = P2 + P0;
        M0 = P1;                       // Offset used for third butterfly
        P5 = P5 >> 1;                  // Set loop counter to Size FFT/(i * 4),
                                       // where i = 2, .., (m+1)

        R7 = R7 >>> 1 || I1 += M0;
        M1 = R7;                       // Twiddle factor offset
                                       // (wst * Size FFT)/(j * 2), j= 1, .., m

        /* 
         * Iter for (Size FFT/(i * 4)) / 2, where i = 2, .., (m+1)
         * Perform butterfly on two sets of data per iteration
         */
        lsetup(Loop1_strt, Loop1_end) LC0 = P5 >> 1;  
Loop1_strt:
           I3 = B3;                    // Address Twiddle table
           R2 = [I2++M2];         
           R4 = [I2];
           I2 -= M2;
           R3 = [I3++M1];

           /*
            * Iter for number of lines per butterfly
            */
           lsetup(Loop2_strt, Loop2_end) LC1 = P3;  
Loop2_strt:                                 
              R2 = R2 +|+ R4, R4 = R2 -|- R4 (ASR) || I1 -= M2 || R6 = [I1];
              A1 = R3.L * R6.H, A0 = R3.L * R6.L || [I2++M2] = R2 || R5 = [I1];
#ifdef __RADIX_4_FFT
              R6.H = (A1 -= R3.H * R6.L), R6.L = (A0 += R3.H * R6.H) ||
                                                  R3 = [I3++M1] || [I2++] = R4;
#else  /* Radix-2 FFT */
              R6.H = (A1 += R3.H * R6.L), R6.L = (A0 -= R3.H * R6.H) ||
                                                  R3 = [I3++M1] || [I2++] = R4;
#endif
              R5 = R5 +|+ R6, R6 = R5 -|- R6 (ASR) || I2 -= M2 || R4 = [I2];
              A1 = R3.L * R4.H, A0 = R3.L * R4.L || [I1++M2] = R5 || R2 = [I2]; 
Loop2_end:
#ifdef __RADIX_4_FFT
              R4.H = (A1 -= R3.H * R4.L), R4.L = ( A0 += R3.H * R4.H) ||
                                                                   [I1++] = R6;
#else  /* Radix-2 FFT */
              R4.H = (A1 += R3.H * R4.L), R4.L = ( A0 -= R3.H * R4.H) ||
                                                                   [I1++] = R6;
#endif

Loop1_end:
           I1 += M0 || R2 = [I2++M0];

        P3 = P3 << 1;                  // Loop counter number of lines in each
                                       // butterfly at stage 3:  2^(2+i)
                                       // where i = 1, .., m
        R0 += -1;                      // Decrement counter m
        CC = R0 == 0;
        If !CC Jump Loopfor_m;         // Loop m times


/** Butterfly - Final Stage **/

        M1 = P2;                       // Offset next butterfly

Esc_mid:
        I0 = B1;                       // Address temporary buffer (CB)
        P0 = B1;                       // Address temporary buffer
        B0 = B1;

        P5 = [SP+48];                  // Size FFT

        I2 = B2;                       // Address output array
        P1 = B2;                       // Address output array

        I3 = B3;                       // Address twiddle table

        P5 = P5 << 2;

        R7 = R7 >>> 1 || R0 = [P0++P2] || R2 = [I0++];
                                       // P0 = &temporary[FFT Size / 2]
        M2 = R7;                       // Twiddle offset

        L0 = P5;                       // Activate circular buffer
        L1 = P5;                       // Activate circular buffer

        R4 = [P0++] || R0 = [I2++M1];  // R4 = temp[FFT Size / 2]
                                       // I2 = &output[FFT Size / 2]

        I1 = P0;                       // Address temporary buffer (CB)
        R3 = [I3++M2];                 // Skip first set of twiddle coeffs
        R3 = [I3++M2];

        /*
         * Iter for FFT Size / 4
         * Perform last butterfly, reading 4 values from temporary buffer
         * and storing the result in the output array in normal order.
         */
        lsetup(Last_strt, Last_end) LC1 = P3 >> 1;
Last_strt:   
           R2 = R2 +|+ R4, R4 = R2 -|- R4 || R6 = [I1++];
           A1 = R3.L * R6.H, A0 = R3.L * R6.L || [I2++] = R4 || R5 = [I0++];
#ifdef __RADIX_4_FFT
           R6.H = (A1 -= R3.H * R6.L), R6.L = (A0 += R3.H * R6.H)  ||
                                                              R3 = [I3++M2];
#else  /* Radix-2 FFT */
           R6.H = (A1 += R3.H * R6.L), R6.L = (A0 -= R3.H * R6.H)  ||
                                                              R3 = [I3++M2];
#endif
           R5 = R5 +|+ R6, R6 = R5 -|- R6 || R4 = [I1++] || [P1++] = R2;
           A1 = R3.L * R4.H, A0 = R3.L * R4.L || [P1++] = R5 || R2 = [I0++]; 
Last_end: 
#ifdef __RADIX_4_FFT
           R4.H = (A1 -= R3.H * R4.L), R4.L = ( A0 += R3.H * R4.H) ||
                                               R3 = [I3++M2] || [I2++] = R6;
#else  /* Radix-2 FFT */
           R4.H = (A1 += R3.H * R4.L), R4.L = ( A0 -= R3.H * R4.H) ||
                                               R3 = [I3++M2] || [I2++] = R6;
#endif

#if defined(__WORKAROUND_INFINITE_STALL_202)
/* After 2 dual dag load/stores, need 2 prefetch loads to avoid the anomaly
** (since the first prefetch is still part of the anomaly sequence).
*/
        PREFETCH[SP];
        PREFETCH[SP];
#endif


/** Function Exit **/

Terminate:
        L0 = 0;                        // Disable circular buffers
        L1 = 0;
        L3 = 0;
        (R7:4, P5:3) = [SP++];         // Restore preserved registers
        RTS;                           // Return


#ifdef __RADIX_4_FFT
.__rfftrad4_fr16.end:
#else  /* Radix-2 FFT */
.__rfftN_fr16.end:
#endif

/*****************************************************************************
  Copyright(c) 2000-2006 Analog Devices Inc.
  All rights reserved
******************************************************************************
  File Name      : ifft_fr16.asm

  Include File   : filter.h
  Label name     : __ifftN_fr16

  Description    : This file contains the code for a radix-2 implementation
                   of an inverse FFT. The function transforms the frequency
                   domain complex input signal sequence to the time domain.
                   The output is provided in normal order, with the output
                   values being scaled by a factor of 2 using static scaling.

                   The C callable prototype of the function is:

                       void ifft_fr16(
                               const complex_fract16 *in,   // Input array
                               complex_fract16       *t,    // Temporary buffer
                               complex_fract16       *out,  // Output array
                               const complex_fract16 *w,    // Twiddle table
                               int wst,                     // Twiddle stride
                               int n,                       // Size FFT
                               int block_exponent,          // Not used
                               int scale_method);           // Not used

                   The input, output and temporary array are expected to be
                   of size n. For optimum performance, the input, output,
                   temporary and twiddle array should be located in different
                   memory banks.

                   If the input data can be overwritten, the optimum memory
                   usage can be achieved by also specifying the input array
                   as the output array.

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
                   object with the entry point name __ifftN_fr16. By
                   defining the macro __RADIX_4_FFT on the command line, the 
                   source can be configured as a direct replacement for the 
                   radix-4 FFT (entry point name _ifftrad4_fr16), which  
                   was included up until VisualDSP++4.0. The radix-4 FFT
                   differs from the radix-2 implementation in that the 
                   twiddle table is composed of +sine coefficients instead
                   of -sine coefficients.

  Restrictions   : The size of the FFT should be 8 or greater and a power of 2.

  Registers Used : R0-7, A0-1, P1-5, I0-3, B0-3, M0-2, L0-1, L3

  Cycle count    : N = 64  =  1102 cycles
                   (BF532, Cycle Accurate Simulator)

  Code size      : 478 Bytes
******************************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libGroup = filter.h;
.file_attr libName = libdsp;
.file_attr prefersMem = internal;
.file_attr prefersMemNum = "30";
#ifdef __RADIX_4_FFT
.file_attr libFunc  = __ifftrad4_fr16;
.file_attr libFunc  = ifftrad4_fr16;
.file_attr FuncName = __ifftrad4_fr16;
#else /* Radix-2 FFT */
.file_attr libFunc  = __ifftN_fr16;
.file_attr libFunc  = ifft_fr16;
.file_attr FuncName = __ifftN_fr16;
#endif
#endif

.section program;
#ifdef __RADIX_4_FFT
.global  __ifftrad4_fr16;
#else  /* Radix-2 FFT */
.global  __ifftN_fr16;
#endif

.align 2;
#ifdef __RADIX_4_FFT
__ifftrad4_fr16:
#else  /* Radix-2 FFT */
__ifftN_fr16:
#endif

/** Function Prologue **/

        [--SP] = (R7:4, P5:3);         // Save reserved registers
        B0 = R0;                       // Address Input array
        B1 = R1;                       // Address of temporary buffer
        B2 = R2;                       // Address of output buffer
        R0 = [SP+40];                  // The address of Twiddle factor
        R3 = [SP+44];                  // The value of the twiddle stride
        R2 = [SP+48];                  // The value of length of input array

        R4 = 8;
        CC = R2 < R4;                  // Exit if the number of input samples 
                                       // is < 8
        If CC Jump Terminate;

        P4 = R2;                       // The length of Input array
        P1 = R3;                       // Preserve wst
        B3 = R0;                       // Address of twiddle table
        R3<<= 1;                       // Length of twiddle table = wst * 2
        R3*= R2;                       // Length of twiddle table = wst * 2 * N
        L3 = R3;

        P3 = P4 << 2;                  // Compute length complex data arrays
                                       // (input array and temporary buffer)

// This function will speculatively load values out of the
// twiddle table that are beyond the end of the array - this
// will cause an exception if the memory referenced does not
// exist.
//
// The twiddle table is therefore accessed as a circular buffer.
// The same idea has to be applied to the input array and the
// temporary buffer.

/* Stages 1 and 2 butterfly */

 /* First of all the input array is copied in temporary buffer in bit-reversed 
  * order. Then the stage 1 and 2 of the butterfly structure are implemented.
  * After the computation the result is stored to output array. The main reason 
  * for separating it out from the general computation, is that the 
  * multiplications of both the stages can be avoided. 
  *
  * In the first stage of signal flow, there are n/2 number of butterflies. 
  * Each butterfly works on two inputs. These input are multiplied by W0 and
  * added and subtracted. Multiplying a data with W0 which is equal to 1 +0j 
  * will result the same data.
  * In the second stage the number of butterflies are n/4. The data are added 
  * and subtracted after multiplication with W0 and Wn/4. The multiplication 
  * with W0 doesn't have any impact. The multiplication of data x+jy with Wn/4 
  * will give the value y-jx. 
  *
  * Therefore, the multiplications involved in both the stages 1 and 2, can be 
  * reduced to additions only. The output is stored after dividing by 2 for 
  * scaling purpose. In one loop data corresponding to two butterflies are 
  * processed.
  */

        I0 = B0;                       // Address of Input array (CB)
        R3 = R2 << 1;    
        L0 = P3;                       // Activate circular buffer
        M0 = R3;                       // M0 stores the offset required 
                                       // for bit-reversing
        M1 = B1;                       // M1 stores the address for 
                                       // temporary buffer
        I1 = 0;                        // I1 is initialized to 0 
                                       // for bit-reversing
        R0 = [I0++];

        //Loop for the size of input length
        lsetup (copy_strt, copy_end) LC0 = P4;  
copy_strt:
           I2 = I1;
           R1 = R0 >>> 1 (V) || I2 += M1 || R0 = [I0++];
           R1 = PACK (R1.L, R1.H);
copy_end:
           I1 += M0 (BREV) || [I2] = R1;

        L0 = 0;                        // Disable circular buffer

        I1 = B1;                       // Address of temporary buffer (CB)
        I2 = B2;

        P5 = P4 >> 2;                  // P5 is equal to number of
                                       // butterflies at Stage 2

        L1 = P3;                       // Activate circular buffer

        R2 = [I1++];                   // R2 reads the data
        R3 = [I1++];
        R4 = [I1++];
        R2 = R2 +|+ R3, R3 = R2 -|- R3 (ASR) || R5 = [I1++];

        // Loop for half of the number of butterflies at Stage 2
        lsetup(Stage12_strt, Stage12_end) LC0 = P5 >> 1;
Stage12_strt:
           R0 = [I1++];
           R1 = [I1++];
           R4 = R4 +|+ R5, R5 = R4 -|- R5 (ASR, CO) || R6 = [I1++];
           R2 = R2 +|+ R4, R4 = R2 -|- R4 (ASR) || R7 = [I1++] ;
           R5 = R3 +|- R5, R3 = R3 -|+ R5 (ASR) || [I2++] = R2;
           R0 = R0 +|+ R1, R1 = R0 -|- R1 (ASR) || [I2++] = R3;

           R6 = R6 +|+ R7, R7 = R6 -|- R7 (ASR, CO) || [I2++] = R4;
           R0 = R0 +|+ R6, R6 = R0 -|- R6 (ASR) || [I2++] = R5;
           R7 = R1 +|- R7, R1 = R1 -|+ R7 (ASR) || [I2++] = R0 || R2 = [I1++];
           [I2++] = R1 || R3 = [I1++];
           [I2++] = R6 || R4 = [I1++];
Stage12_end: 
           R2 = R2 +|+ R3, R3 = R2 -|- R3 (ASR) || [I2++] = R7 || R5 = [I1++];

#if defined(__WORKAROUND_INFINITE_STALL_202)
/* After 2 dual dag load/stores, need 2 prefetch loads to avoid the
** anomaly (since the first prefetch is still part of the anomaly sequence).
*/
        PREFETCH[SP];
        PREFETCH[SP];
#endif

        L1 = 0;                        // Disable circular buffer
        R1 = P1;                       // R1 = wst
        R1 = R1 << 2;                  // R1 = wst * 4
        P3 = 4;                        // P3 holds the number of lines 
                                       // in each butterfly at stage 3
        R7 = P5;  
        R7 *= R1;                      // R7 = wst * 4  * twiddle offset
        R2 = P4;  
        R3 = 8;
        M1 = 16;

        CC = R2 == R3;                 // If input array size is equal to 8, 
                                       // skip middle stages
        If CC Jump Esc_mid;


        R0 = 0;                        // Counter for number of stages (m)
Find_m:            
        R2 >>= 1;
        R0 += 1;
        CC = R2 == R3;
        If !CC Jump Find_m (BP);       // R0 holds the value of m-3


/* Middle stages of butterfly */

/* First of all, a loop for the number of stages - 3 is set. It is a general 
 * implementation of butterfly computation. The first nested loop is set for 
 * half of the number of butter flies at each stage. The second nested loop 
 * is set for the number of lines in each butterfly.
 * The computation is done on the output array. The output is stored after 
 * dividing by 2 for scaling purpose. In one loop two butterfly data are read 
 * and processed.
 */

Loopfor_m:
        I2 = B2;                       // Address of output array
        I1 = B2;                       // Address of output array
        P0 = P3 << 2; 
        M2 = P0;                       // M2 holds the offset of 
                                       // counterpart line
        P2 = P0 << 1;
        P1 = P2 + P0;
        M0 = P1;                       // The offset used for the 
                                       // third butterfly
        P5 = P5 >> 1;
        R7 = R7 >>> 1 || I1 += M0;
        M1 = R7;                       // Twiddle factor offset

        // Loop for half of the butterfly
        lsetup(Loop1_strt, Loop1_end) LC0 = P5 >> 1;  
Loop1_strt:
           I3 = B3;                    // Address of twiddle factor
           R2 = [I2++M2];
           I2 -= M2 || R4 = [I2];
           R3 = [I3++M1];

           // Loop for the number of per butterfly
           lsetup(Loop2_strt, Loop2_end) LC1 = P3;   
Loop2_strt:     
              R2 = R2 +|+ R4, R4 = R2 -|- R4 (ASR) || I1 -= M2 || R6 = [I1];
              A1 = R3.L * R6.H, A0 = R3.L * R6.L || [I2++M2] = R2 || 
                                                                  R5 = [I1];
#ifdef __RADIX_4_FFT
              R6.H = (A1 -= R3.H * R6.L), R6.L = (A0 += R3.H * R6.H) ||
                                               R3 = [I3++M1] || [I2++] = R4;
#else  /* Radix-2 FFT */
              R6.H = (A1 += R3.H * R6.L), R6.L = (A0 -= R3.H * R6.H) || 
                                               R3 = [I3++M1] || [I2++] = R4;
#endif
              R5 = R5 +|+ R6, R6 = R5 -|- R6 (ASR) || I2 -= M2 || R4 = [I2];
              A1 = R3.L * R4.H, A0 = R3.L * R4.L || [I1++M2] = R5;

Loop2_end:
#ifdef __RADIX_4_FFT
              R4.H = (A1 -= R3.H * R4.L), R4.L = ( A0 += R3.H * R4.H) ||
                                                   [I1++] = R6 || R2 = [I2];
#else  /* Radix-2 FFT */
              R4.H = (A1 += R3.H * R4.L), R4.L = ( A0 -= R3.H * R4.H) || 
                                                   [I1++] = R6 || R2 = [I2];
#endif

           I1 += M0;  
Loop1_end: 
           I2 += M0; 

        P3 = P3 << 1;
        R0 += -1;
        
        CC = R0 == 0;
        If !CC Jump Loopfor_m (BP);    // Loop for m
        M1 = P2;


/* Last stage butterfly */

/* This part implements the last stage of the butterfly. The label Esc_mid is 
 * used when the size of input data is 8. In this case the computation of 
 * middle stages have to be escaped. The increment in the twiddle factor 
 * offset is just 1. In the last stage there is only one butterfly. The loop 
 * is set for n/4. 4 data are read and processed at the same time.
 */

Esc_mid:
        B1 = B2;
        B0 = B2;
        I0 = B2;                       // Address output array (CB)

        P5 = [SP+48];                  // Size FFT

        I2 = B2;                       // Address output array
        I1 = B2;                       // Address output array (CB)

        I3 = B3;                       // I3 holds the twiddle factor address
        P5 = P5 << 2;

        R7 = R7 >>> 1 || I1 += M1;
        M2 = R7;                       // M2 holds twiddle factor offset

        L0 = P5;                       // Activate circular buffers
        L1 = P5;
        M0 = 8;

        I0 += M1 || R2 = [I2];
        I1 += 4 || R3 = [I3++M2];
        R4 = [I0];

        // Loop for the number of lines per butterfly
        lsetup(Last_strt, Last_end) LC1 = P3 >> 1;   
Last_strt: 
           R3 = [I3++M2];
           R2 = R2 +|+ R4, R4 = R2 -|- R4 (CO) || I1 -= M1 || R6 = [I1];
           A1 = R3.L * R6.H, A0 = R3.L * R6.L || [I0++M0] = R4;
           R2 = PACK(R2.L, R2.H) || R5 = [I1];
#ifdef __RADIX_4_FFT
           R6.H = (A1 -= R3.H * R6.L), R6.L = (A0 += R3.H * R6.H) ||
                                                          R3 = [I3++M2];
#else  /* Radix-2 FFT */
           R6.H = (A1 += R3.H * R6.L), R6.L = (A0 -= R3.H * R6.H) || 
                                                          R3 = [I3++M2];
#endif
           R5 = R5 +|+ R6, R6 = R5 -|- R6 (CO) || R4 = [I0];
           R5 = PACK(R5.L, R5.H) || [I2++M0] = R2;
           A1 = R3.L * R4.H, A0 = R3.L * R4.L || [I1++M1] = R5;
Last_end: 
#ifdef __RADIX_4_FFT
           R4.H = (A1 -= R3.H * R4.L), R4.L = ( A0 += R3.H * R4.H) ||
                                             [I1++M0] = R6 || R2 = [I2];
#else  /* Radix-2 FFT */
           R4.H = (A1 += R3.H * R4.L), R4.L = ( A0 -= R3.H * R4.H) || 
                                             [I1++M0] = R6 || R2 = [I2];
#endif

Terminate:
        L0 = 0;                        // Disable circular buffers
        L1 = 0;
        L3 = 0;

        (R7:4, P5:3) = [SP++];         // Pop the registers before returning
        RTS;                           // Return

#ifdef __RADIX_4_FFT
.__ifftrad4_fr16.end:
#else  /* Radix-2 FFT */
.__ifftN_fr16.end:
#endif

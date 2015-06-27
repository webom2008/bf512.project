/******************************************************************************
  Copyright(c) 2006-2009 Analog Devices Inc.
  All rights reserved
 ******************************************************************************

  Implementation : radix-2 complex Fast Fourier Transform

  Prototype      : #include <filter.h>
                   void  cfft_fr16( const complex_fract16  in[],
                                          complex_fract16  out[],
                                    const complex_fract16  twiddle[],
                                    int  stride,
                                    int  fft_size,
                                    int  *block_exponent,
                                    int  scaling );

  Description    : Implementation of a radix-2 complex FFT that supports
                   three different scaling modes, dynamic, static and
                   and no scaling.

                   The input and output buffer are assumed to be of length
                   fft_size. The argument fft_size must be a power of two
                   and at least 8. 
                   If the input and output buffer do not overlap, the 
                   contents of the input buffer will be preserved. However
                   when using the function in-place (where the input buffer
                   is used as output buffer also) the original contents of 
                   the input buffer will be clobbered. There is also a 
                   performance penalty for in-place operation.

                   The minimum size of the twiddle table is fft_size / 2.
                   The stride argument enables the function to read larger
                   twiddle tables correctly. The stride is defined as:

                                 FFT size that the table was computed for 
                      stride =  ------------------------------------------ 
                                               fft_size

                   The function

                      #include <filter.h>
                      void twidfftrad2_fr16 (complex_fract16 w[], int n);

                   can be used to generate the desired twiddle table.
                   The coefficients are generated using +cosine and -sine.
                   For optimum performance, the twiddle table should be placed
                   in a different memory section than the output array.

                   The argument scale_method controls how the function will 
                   apply scaling while computing a Fourier Transform. The 
                   available options are static scaling (dividing the input 
                   at any stage by 2), dynamic scaling (dividing the input 
                   at any stage by 2 if the largest absolute input value is 
                   greater or equal than 0.25 (=0x2000)), or no scaling.

                   If static scaling is selected, the function will always 
                   scale intermediate results, thus preventing overflow. The 
                   loss of precision increases in line with the fft_size and 
                   is more pronounced for input signals with a small magnitude 
                   (since the output is scaled by 1/fft_size). To select static 
                   scaling, set the argument scale_method to 1. The block 
                   exponent returned will be log2(fft_size).

                   If dynamic scaling is selected, the function will inspect 
                   intermediate results and only apply scaling where required 
                   to prevent overflow. The loss of precision increases in line 
                   with the fft_size and is more pronounced for input signals 
                   with a large magnitude (since these factors increase the 
                   need for scaling). The need to inspect intermediate results 
                   will have an impact on performance. To select dynamic 
                   scaling, set the argument scale_method to 2. The block 
                   exponent returned will be between 0 and log2(fft_size).

                   If no scaling is selected, the function will never scale 
                   intermediate results. In case of overflow, the function will 
                   saturate, otherwise no loss of precision will be incurred. 
                   The likelihood of saturation increases in line with the 
                   fft_size and is more pronounced for input signals with a 
                   large magnitude. To select no scaling, set the argument 
                   scale_method to 3. The block exponent returned will be 0.

                   For backwards compatibility, values for the argument 
                   scale_method other than 1, 2 or 3 will result in the 
                   function performing static scaling (the default behaviour 
                   of the previous version of the function).


  Algorithm      : The computation of the FFT is split into three parts.
                   
                   The first part handles bit-reversing and copies the input
                   data into the output buffer (or re-arranges the data in
                   the input buffer in bit-reversed order if computing an
                   in-place FFT). 
                   Independent of the scaling mode, the magnitude of the data 
                   is tested (since it can be done at no extra cost) for use
                   in the second part.

                   The first stage of the FFT is computed separately in the
                   second part. With the twiddle data having a constant value
                   of 1 for the real part and 0 for the imaginary part, the
                   computation of the butterflies at this stages can be
                   greatly simplified.
                   Depending on the scaling mode and the magnitude of the 
                   input data, alternative code is computed that either
                   performs a shift operation or not. 
                    
                   The last part computes the remaining stages of the FFT. 
                   Again, depending on the scaling mode and the magnitude of
                   the input data, alternative code is computed that either
                   performs a shift operation or not.
                   Unlike in the previous parts however, testing the output 
                   at each stage for dynamic scaling is done separately before
                   computing the butterflies.


                   Register usage:

                     Loop Counter
                        P2,P3,P5 = Number of butterflies
                        P4       = Number of stages

                     Input to each butterfly:  I2, I3 (as circular buffer)
                     Output of each butterfly: P0, I0 (as circular buffer)
                     Twiddle table:            I1     (as circular buffer)

                     P1, M2  stride next input / output
                     M1      twiddle stride


  Cycle count    : Using the cycle accurate ADSP-BF532 simulator without any
                   workarounds for silicon anomalies enabled. The cycle count
                   includes the cost of calling the function and argument 
                   passing.

                   Size FFT     Cycle Count 
                                no scaling - static scaling - dynamic scaling*

                          8         278            268            291
                         16         408            395            452
                        128        2198           2176           2956
                        256        4446           4422           6242
                       1024       19698          19667          28912

                   * cycle counts for dynamic scaling are dependent on
                     the input data
                     [for a 1024 point FFT:  28897 (always scale)
                                             28928 (never scale)]

******************************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libGroup      = filter.h;
.file_attr libName       = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc       = __cfftN_scaling_fr16;
.file_attr libFunc       = cfftN_scaling_fr16;
.file_attr FuncName      = __cfftN_scaling_fr16;
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
** hardware anomaly 05-00-0245". The anomaly is only an issue if there is a
** load which may access reserved or illegal memory. This can not be the case
** since the memory in question has already been accessed before the point of
** the warning.
*/

.MESSAGE/SUPPRESS 5508;
#endif

/** Location of input arguments on the stack **/

#define  _OFFSET_IN_DATA       40
#define  _OFFSET_IN_ALTERNATE   (_OFFSET_IN_DATA - 28)

#define  _IN_STRIDE        ( _OFFSET_IN_DATA ) 
#define  _IN_FFTSIZE       ( _OFFSET_IN_DATA +  4 )
#define  _IN_BLOCKEXPO     ( _OFFSET_IN_DATA +  8 )
#define  _IN_SCALING       ( _OFFSET_IN_DATA + 12 ) 

/** Placement of temporary data on the stack **/

#define  _TWIDDLE_STRIDE      0           
#define  _INPUT_STRIDE        4
#define  _CONST_FOUR          8

#define  _SIZE_TEMP_DATA   (_CONST_FOUR + 4)


.section program;
.global  __cfftN_scaling_fr16;

.align 2;
__cfftN_scaling_fr16:

/** Function Prologue **/


        [--SP] = (R7:4, P5:3);         // Save reserved registers


        B2 = R0;                       // Address input array
        I2 = R0;

        B0 = R1;                       // Address output buffer
        B3 = R1;
        P0 = R1;

        B1 = R2;                       // Address twiddle table
        I1 = R2;


        R0 = [SP+_IN_STRIDE];          // Twiddle stride
        R6 = [SP+_IN_FFTSIZE];         // FFT size
        R2 = [SP+_IN_SCALING];         // Scaling method

        SP += -_SIZE_TEMP_DATA;        // Allocate space on the stack for 
                                       // local data

        CC = R0 <= 0;
        IF CC JUMP .done;              // Exit if twiddle stride <= 1
 
        R3 = 8;
        CC = R6 < R3; 
        IF CC JUMP .done;              // Exit if FFT size < 8

        /* Convert scaling method
           2 (dynamic)        -> 2
           3 (no scaling)     -> 0
           otherwise (static) -> 1
        */
        CC = R2 == 2;
        IF CC JUMP .dynamic;
        CC = R2 == 3;
        CC = !CC;
        R2 = CC;

.dynamic:
        R4 = 2;                        // Set constant required for bit-reversal

        P1 = R3;                       // Offset to data next butterfly (=8)

        P5 = R6;                       // Loop counter bit-reversal (fft_size)

        R5 = R6 << 1
        || [SP+_INPUT_STRIDE] = R3;    // Push 8 into top half of the register

        [SP+(_IN_SCALING+_SIZE_TEMP_DATA)] = R2;
                                       // Store converted scaling method

        M0 = R5;                       // Size FFT * 2 (for bit-reversal)

        R5 = R5 << 1;                   

        R0 *= R6;                      

        L2 = R5;                       // Enable circular buffer input, output
                                       // (Size FFT * 4) 

        R7 = 2 (Z);
         
        R0 = R0 << 1 
        || [SP+_TWIDDLE_STRIDE] = R0;  // Size FFT/2 * 1/2 * 4 * stride 
                                       // = Size FFT * stride

        R5 = R6 >> 2;
        L1 = R0;                       // Enable circular buffer twiddle table
        P4 = R5;                       // Loop counter first stage(fft_size/4) 


/** Bit reversal **/

        R5 = I2;                       // Address input buffer
        R2 = B0;                       // Address output buffer

        CC = R5 == R2;                 // Test for in-place FFT 
                                       // &input == &output

        M2 = B0;
        I3 = 0;

        IF CC JUMP .bitrev_in_place;


        R0 = 0;

        /* Loop for FFT size 
         *   Copy data from the input buffer into the output buffer 
         *   in bit-reversed order. 
         *   At the same time, scan the input data to detect whether 
         *   dynamic scaling is required for the first stage.
         *
         *   R7.L = Reference exponent
         *          No dynamic scaling is required if 
         *             0xC000 <= input data <= 0x3FFF 
         *          => exponent(0x3FFF) - 1 = 1  
         */
        LSETUP ( .bitrev_start, .bitrev_end ) LC0 = P5 >> 1;
.bitrev_start:
           I0 = I3;
           R7.L = EXPADJ (R0, R7.L ) (V) || I0 += M2 || R0 = [I2++]; 
           I3 += M0 (BREV) || [I0] = R0;
           I0 = I3;
           R7.L = EXPADJ (R0, R7.L ) (V) || I0 += M2 || R0 = [I2++];
.bitrev_end:
           I3 += M0 (BREV) || [I0] = R0;

        R7.L = EXPADJ (R0, R7.L ) (V);
        JUMP .end_bitrev;

.bitrev_in_place:

        I0 = 0; 
        R5 += -4;
        R4 <<= 1;

        /* Loop for FFT size
         *   Re-arrange data in the input buffer in bit-reversed order.
         *   At the same time, scan the input data to detect whether
         *   dynamic scaling is required for the first stage.
         *
         *   R7.L = Reference exponent
         *          No dynamic scaling is required if
         *             0xC000 <= input data <= 0x3FFF
         *          => exponent(0x3FFF) - 1 = 1
         */
        LSETUP ( .bitrev_ip_start, .bitrev_ip_end ) LC0 = P5;
.bitrev_ip_start:
           /* Increment address (= current input),
              generate address bitrev and load input
           */
           R5 = R5 + R4 (NS) || I0 += M2 || R0 = [I2];
           R6 = I0;

           /* Only re-order if address bitrev < address current input */
           CC = R5 <= R6;

#if WA_05000428
           // Speculative read from L2 by Core B may cause a write to L2 to fail

           R2 = [I0];
           IF CC JUMP .skip_swap_op;

           [I0] = R0;
           [I2] = R2;
           NOP;
#else
           IF CC JUMP .skip_swap_op;

           R2 = [I0] || [I0] = R0;
           [I2] = R2;
#endif

.skip_swap_op:
           /* Analyse data for dynamic scaling,
              compute bit-reversal offset, 
              increment read pointer with dummy read
           */
           R7.L = EXPADJ (R0, R7.L ) (V) || I3 += M0 (BREV) || R0 = [I2++];

.bitrev_ip_end:
           I0 = I3;

       
.end_bitrev:

        M2 = P1;                       // Offset to data next butterfly

        I3 = B3;                       // Configure read pointers
        I2 = B3;                        
        B2 = B3;

        L3 = L2;                       // Enable circular buffer input

        L0 = L2;                       // Enable circular buffer output
        I0 = B0;                       // Configure write pointer


/** First stage **/

        R7 >>= 1;
        BITTGL( R7, 0 );               // IF EXPADJ == 0 => scale (R7 = 1)
                                       //              otherwise don't (R7 = 0)  

        R6 = [SP+(_IN_SCALING+_SIZE_TEMP_DATA)];
                                       // Load scaling method 

        CC = R6 == 2;                  // Test for dynamic scaling
        IF CC R6 = R7;                 // R6 = 1 if scaling, 0 otherwise

        R1 = R1 -|- R1
        || R2 = [I2++M2]               // Load data[0]
        || I3 += 4;                    // and advance read pointer to data[1]

        R1.L = R1.L + R6.L (S)         // Increment count block exponent
        || R3 = [I3++M2]               // and load data[1]
        || I0 += 4;                    // and advance write pointer to data[1]

        CC = R6 == 0;                  // Select appropriate first stage
        IF CC JUMP .first_stage_no_scale;


#if WA_05000428
        // Speculative read from L2 by Core B may cause a write to L2 to fail

        NOP;
        NOP;
#endif

        /* Loop N/2 times 
             Compute two butterflies per iteration
             with the twiddle value constant at { 1,0 }.
             Scale input values 
         */
        LSETUP ( .first_s_start, .first_s_end ) LC0 = P4; 
.first_s_start:
#if defined(__ADSPLPBLACKFIN__)
           // +|+ (ASR) implemented as:  (R2 >>> 1 (V)) +|+ (R3 >>> 1 (V))
           R6 = R2 +|+ R3, R7 = R2 -|- R3 (ASR) 
           || R4 = [I2++M2];

           [P0++P1] = R6 || R5 = [I3++M2];

           R6 = R4 +|+ R5, R7 = R4 -|- R5 (ASR) 
           || [I0++M2] = R7 || R2 = [I2++M2];

           [P0++P1] = R6 || R3 = [I3++M2];

.first_s_end:
           [I0++M2] = R7;
#else
           // +|+ (ASR) implemented as: (R2 +|+ R3) >>> 1 (V)
           R2 = R2 >>> 1 (V) || R4 = [I2++M2];
           R3 = R3 >>> 1 (V) || R5 = [I3++M2];
           R6 = R2 +|+ R3, R7 = R2 -|- R3;
           R4 = R4 >>> 1 (V) || [P0++P1] = R6 || R2 = [I2++M2];
           R5 = R5 >>> 1 (V) || [I0++M2] = R7 || R3 = [I3++M2];
           R6 = R4 +|+ R5, R7 = R4 -|- R5;
           [P0++P1] = R6;
.first_s_end:
           [I0++M2] = R7;
#endif

        JUMP .remaining_stages; 


.first_stage_no_scale:

        /* Loop N/2 times
             Compute two butterflies per iteration
             with the twiddle value constant at { 1,0 }.
             No scaling of the input values
         */
        LSETUP ( .first_ns_start, .first_ns_end ) LC0 = P4;
.first_ns_start:
           R6 = R2 +|+ R3, R7 = R2 -|- R3 (S) 
           || R4 = [I2++M2];

           [P0++P1] = R6 || R5 = [I3++M2];

           R6 = R4 +|+ R5, R7 = R4 -|- R5 (S) 
           || [I0++M2] = R7 || R2 = [I2++M2];

           [P0++P1] = R6 || R3 = [I3++M2];

.first_ns_end:
           [I0++M2] = R7;



/** Remaining stages **/

.remaining_stages:


        /* Position pointers, init regs */

        /* M2 set to 8 */

        P1 = 4;
        P3 = 2;                        // Number of butterflies per group 
        P5 = -1;

        R4 = [SP+_TWIDDLE_STRIDE];

        [SP+_CONST_FOUR] = P1
        || R0 = [I3--];                // Position read pointer (dummy read)

        I2 -= M2                       // Position read pointer
        || R0 = [I0++];                // Position write pointer

        R6 = [SP+_INPUT_STRIDE]        // Load offset input
        || R2 = [I2++];                // Load first input value


        /* Loop Counter = P4, preset to N / 4
             For Size FFT = 8:  P4 initial = 8/4 = 2
                                P4 after first iteration = 2 / 2 = 1
                                P4 after next iteration  = 1 / 2 = 0
                                      => exit loop after 2 iterations
        */                              

#if WA_05000428
        // Speculative read from L2 by Core B may cause a write to L2 to fail
        //
        // load scaling method _before_ the label (note that towards the
        // end of the function the code that jumps to the label will first
        // load the scaling method - this is a device to avoid a speculative
        // load in the shadow of a branch-predicted jump)

        R5 = [SP+(_IN_SCALING+_SIZE_TEMP_DATA)];   // Load scaling method

.align 8;
.start_iter_stages:
#else
.align 8;
.start_iter_stages:

        R5 = [SP+(_IN_SCALING+_SIZE_TEMP_DATA)];   // Load scaling method
#endif

        CC = R5 == 2;                  // Test for dynamic scaling
        IF !CC JUMP .end_traverse;

        /* For dynamic scaling 
           traverse data to see if scaling is required 
           for given stage
        */

#if WA_05000428
        // Speculative read from L2 by Core B may cause a write to L2 to fail

        NOP;
        NOP;
        NOP;
#endif
        P2 = [SP+(_IN_FFTSIZE+_SIZE_TEMP_DATA)]; // Load fft_size (loop counter)
        
        R0 = [I2];
        R3 = PACK( R5.H, R5.L) || R7 = [I2++];
                                       // Read data in such a way that
                                       // optimizes pointer positioning
                                       // and minimal use of expadj()
 
        /* Loop for size FFT */
        LSETUP ( .start_scan_data, .end_scan_data ) LC1 = P2 >> 1;
.start_scan_data:
           R3.L = EXPADJ (R0, R3.L ) (V) || R0 = [I2++];
.end_scan_data:
           R5.L = EXPADJ (R7, R5.L ) (V) || R7 = [I2++];

        R3.L = EXPADJ (R0, R3.L ) (V) || R7 = [I2--];

        R5 = R5 + R3;                  // Sum of block exponents
        R5 >>= 2; 
        BITTGL( R5, 0 );           


.end_traverse:

        P1 = P1 + (P3 << 2);           // Offset output (=4*Butterflies+4)
        P2 = P3 + P5;                  // Number of butterflies - 1
        P0 = B0;                       // Reset write pointer to data[0]

        R1.L = R1.L + R5.L (S)         // Increment count block exponent
        || [SP+_INPUT_STRIDE] = R6;    // Store offset input

        M1 = R4;                       // Offset twiddle table;
        M2 = R6;                       // Offset input (next stage)
        M0 = P1;                       // Offset output (=4*Butterflies+4)

        CC = R5 == 0;                  // Select appropriate stage
        IF CC JUMP .other_stages_no_scale;


#if WA_05000428
        // Speculative read from L2 by Core B may cause a write to L2 to fail

        NOP;
        NOP;
#endif
        
        // Loop Counter = P4, preset to N / 4 
        LSETUP ( .start_s_group, .end_s_group ) LC0 = P4;
.start_s_group:

           R3 = [I3++] || I1+=M1; 

           LSETUP ( .start_s_butterfly, .end_s_butterfly ) LC1 = P2;
.start_s_butterfly:
#if defined(__ADSPLPBLACKFIN__)
              // +|+ (ASR) implemented as:  (R2 >>> 1 (V)) +|+ (R3 >>> 1 (V))
#if defined(__WORKAROUND_AVOID_DAG1)
              R6 = R2 +|+ R3, R7 = R2 -|- R3 (ASR)
              || R3 = [I3++];

              R0 = [I1++M1];
#else
              R6 = R2 +|+ R3, R7 = R2 -|- R3 (ASR) 
              || R3 = [I3++] || R0 = [I1++M1];
#endif /* __WORKAROUND_AVOID_DAG1 */

              A1 = R3.L * R0.H, A0 = R3.L * R0.L
              || [P0++] = R6 || R2 = [I2++];

.end_s_butterfly:
              R3.H = (A1 += R3.H * R0.L), R3.L = (A0 -= R3.H * R0.H)
              || [I0++] = R7;

#else
              // +|+ (ASR) implemented as: (R2 +|+ R3) >>> 1 (V)
              R2 = R2 >>> 1 (V);
              R3 = R3 >>> 1 (V);

              R6 = R2 +|+ R3, R7 = R2 -|- R3
              || R3 = [I3++] || R0 = [I1++M1];

              A1 = R3.L * R0.H, A0 = R3.L * R0.L
              || [P0++] = R6 || R2 = [I2++];

.end_s_butterfly:
              R3.H = (A1 += R3.H * R0.L), R3.L = (A0 -= R3.H * R0.H)
              || [I0++] = R7;

#endif /* __ADSPLPBLACKFIN__ */


#if defined(__ADSPLPBLACKFIN__)
           R6 = R2 +|+ R3, R7 = R2 -|- R3 (ASR)
           || I2 += M2;                // Advance read pointer
#else
           R2 = R2 >>> 1 (V);
           R3 = R3 >>> 1 (V);

           R6 = R2 +|+ R3, R7 = R2 -|- R3 
           || I2 += M2;                // Advance read pointer
#endif

           [P0++P1] = R6               // Save result and advance write pointer
           || R2 = [I2++];

.end_s_group:
           [I0++M0] = R7               // Save result and advance write pointer  
           || I3 += M2;                // Advance read pointer


        JUMP .end_other_ns_stages;


.align 4;
.other_stages_no_scale:

        // Loop Counter = P4, preset to N / 4
        LSETUP ( .start_ns_group, .end_ns_group ) LC0 = P4;
.start_ns_group:

           R3 = [I3++] || I1+=M1;

           LSETUP ( .start_ns_butterfly, .end_ns_butterfly ) LC1 = P2;
.start_ns_butterfly:
#if defined(__ADSPLPBLACKFIN__) && defined(__WORKAROUND_AVOID_DAG1)
              R6 = R2 +|+ R3, R7 = R2 -|- R3 (S)
              || R3 = [I3++];
 
              R0 = [I1++M1];
#else
              R6 = R2 +|+ R3, R7 = R2 -|- R3 (S)
              || R3 = [I3++] || R0 = [I1++M1];  
#endif /* __WORKAROUND_AVOID_DAG1 */
              A1 = R3.L * R0.H, A0 = R3.L * R0.L
              || [P0++] = R6 || R2 = [I2++];

.end_ns_butterfly:
              R3.H = (A1 += R3.H * R0.L), R3.L = (A0 -= R3.H * R0.H)
              || [I0++] = R7;

           R6 = R2 +|+ R3, R7 = R2 -|- R3 (S)
           || I2 += M2;                // Advance read pointer

           [P0++P1] = R6               // Save result and advance write pointer
           || R2 = [I2++];

.end_ns_group:
           [I0++M0] = R7               // Save result and advance write pointer
           || I3 += M2;                // Advance read pointer


.end_other_ns_stages:
 
        /* Configure next stage */
 
        P4 = P4 >> 1;                  // Half the number of groups
                                       // Decrement outer loop count

        P3 = P3 << 1;                  // Double number of butterflies

        R4 = R4 >> 1                   // Decrement twiddle stride
        || R6 = [SP+_INPUT_STRIDE];    // Load offset input

        R6 = R6 << 1                   // Increment offset input
        || P1 = [SP+_CONST_FOUR];

        R5 = [I3++M2]                  // Position read, write pointer 
        || I0 += M2;                   // using dummy read
                                  

#if WA_05000428
        // Speculative read from L2 by Core B may cause a write to L2 to fail
        //
        // Pre-load the scaling method in case the JUMP instruction
        // below jumps to the specified label.

        R5 = [SP+(_IN_SCALING+_SIZE_TEMP_DATA)]; // Load scaling method
#endif

        CC = P4 == 0;                  
        IF !CC JUMP .start_iter_stages (BP);  
     


/** Function Epilogue **/

.done:

        R2 = R1.L (Z);                 // Extract block exponent

        P2 = [SP+(_IN_BLOCKEXPO+_SIZE_TEMP_DATA)];
                                       // Load &block exponent

        L0 = 0;                        // Disable circular buffers
        L1 = 0;
        L2 = 0;
        L3 = 0;

        SP += _SIZE_TEMP_DATA;         // Release temporary data on the stack
        
        [P2] = R2;                     // Write count block exponent
        
        (R7:4, P5:3) = [SP++];         // Pop the registers before returning
        RTS;                           // Return

.__cfftN_scaling_fr16.end:

/****************************************************************************
 *
 * iir_fr32.asm : $Revision: 1.5 $
 *
 * (c) Copyright 2008-2009 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: iir_fr32 - infinite impulse response filter

    Synopsis:

        #include <filter.h>
        void iir_fr32 (const fract32    input[],
                       fract32          output[],
                       int              length,
                       iir_state_fr32  *filter_state);

        The IIR filter function uses the following structure to maintain
        the state of the filter:

        typedef struct
        {
            fract32 *c;    /* coefficients             */
            fract32 *d;    /* start of delay line      */
            int k;         /* number of biquad stages  */
        } iir_state_fr32;

    Description:

        The iir function implements a biquad direct form II infinite impulse
        response (IIR) filter. It generates the filtered response of the
        input data input and stores the result in the output vector output.
        The number of input samples and the length of the output vector are
        specified by the argument length.

        The function maintains the filter state in the structured variable
        filter_state, which must be declared and initialized before calling
        the function. The macro iir_init, defined in the filter.h header file,
        is available to initialize the structure and is defined as:

            #define iir_init(state, coeffs, delay, stages)  \
                            (state).c = (coeffs);           \
                            (state).d = (delay);            \
                            (state).k = (stages)

        The characteristics of the filter are dependent upon filter coefficients
        and the number of stages. Each stage has five coefficients which must be
        stored in the order A2, A1, B2, B1, and B0. The value of A0 is implied
        to be 1.0 and A1 and A2 should be scaled accordingly. This requires that
        the value of the A0 coefficient be greater than both A1 and A2 for all
        the stages. The function iirdf1_fr32 implements a direct form I filter,
        and does not impose this requirement; however, it does assume that the
        A0 coefficients are 1.0.

        A pointer to the coefficients should be stored in filter_state->c, and
        filter_state->k should be set to the number of stages.

        Each filter should have its own delay line which is a vector of type
        fract32 and whose length is equal to twice the number of stages. The
        vector should be initially cleared to zero and should not otherwise
        be modified by the user program. The structure member filter_state->d
        should be set to the start of the delay line.

    Error Conditions:

        The iir function checks that the number of input samples and the number
        of stages are greater than zero - if not, the function just returns.

    Algorithm:

        Dm =      Xm - A1 * Dm-1 - A2 * Dm-2
        Ym = B0 * Dm + B1 * Dm-1 + B2 * Dm-2

        where m = 0, 1, .., length-1

    Implementation:

        The algorithm used for the implementation is:

            Dm = (-1 * -Xm) + (-A1 * Dm-1) + (-A2 * Dm-2)
            Ym = (B0 *  Dm) + ( B1 * Dm-1) + ( B2 * Dm-2)

        First Dm is computed in the following manner:

            LoLo = ((-A1Lo * Dm-1Lo) + (-A2Lo * Dm-2Lo) + 0x8000) >> 16;

                   the product (-XmLo * Lo(0x80000000) = 0
                   0x8000 is the rounding factor applied
 
            HiLo_LoHi = (LoLo + (-XmLo * Hi(0x80000000) +  
                         (-A1Lo * Dm-1Hi) + (-A2Lo * Dm-2Hi) + 
                         (-A1Hi * Dm-1Lo) + (-A2Hi * Dm-2Lo) + 0x4000) >> 15;

                   the product (-XmHi * Lo(0x80000000) = 0
                   0x4000 is the rounding factor applied

            HiHi = (HiLo_LoHi + (-XmHi * Hi(0x80000000) +
                   (-A1Hi * Dm-1Hi) + (-A2Hi * Dm-2Hi))

        Negating the A coefficients is neccessary because the intermediate 
        result LoLo is calculated using unsigned arithmetic. If these products
        are then combined as a result of a multiply-decrement operation and
        the resultant difference is negative, then the ACC will saturate
        to zero. 

        Next Ym is computed:

            LoLo = ((B0Lo * DmLo) + (B1Lo * Dm-1Lo) + 
                    (B2Lo * Dm-2Lo) + 0x8000) >> 16;

            HiLo_LoHi = ((B0Lo * DmHi) + (B1Lo * Dm-1Hi) + (B2Lo * Dm-2Hi) +
                         (B0Hi * DmLo) + (B1Hi * Dm-1Lo) + (B2Hi * Dm-2Lo) +
                         LoLo + 0x4000) >> 15;

            HiHi = (B0Hi * DmHi) + (B1Hi * Dm-1Hi) + 
                   (B2Hi * Dm-2Hi) + HiLo_LoHi;

        The array of coefficients and the delay line make use of 
        circular buffering.

    Example:

        #include <filter.h>
        #define NUM_SAMPLES   256
        #define NUM_STAGES      3

        fract32 input[NUM_SAMPLES];
        fract32 output[NUM_SAMPLES];
        segment("L1_data_a") fract32 coeffs[NUM_STAGES*5];
        segment("L1_data_b") fract32 delay[NUM_STAGES*2];

        iir_state_fr32 state;
        int i;

        for (i = 0; i < (NUM_STAGES*2); i++) /* clear the delay line */
        {
            delay[i] = 0;
        }

        iir_init(state, coeffs, delay, NUM_STAGES);
        iir_fr32(input, output, NUM_SAMPLES, &state);

    Cycle Counts:

        44 + (Ni * (2 + (29 * Ns)))

        where Ni is the number of samples and
              Ns is the number of biquad stages

        (Measured for an ADSP-BF532 using version 5.0.0.35 of the ADSP-BF5xx
        Family Simulator and includes the overheads involved in calling
        the library procedure as well as the costs associated with argument
        passing).
 
        For best performance, the delay line and the filter coefficients
        should be allocated in different memory blocks.

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR  libGroup      = filter.h;
.FILE_ATTR  libFunc       = __iir_fr32;
.FILE_ATTR  libFunc       = iir_fr32;
.FILE_ATTR  libName       = libdsp;
.FILE_ATTR  prefersMem    = internal;
.FILE_ATTR  prefersMemNum = "30";
.FILE_ATTR  FuncName      = __iir_fr32;
#endif

#if !defined(__ADSPLPBLACKFIN__)
/* Suppress Assembler Warning ea5504 "Memory store instruction use may trigger
** hardware anomaly 05-00-0165". The run-time libraries are not required to
** workaround this anomaly.
*/
.MESSAGE/SUPPRESS 5504;
#endif

.GLOBAL __iir_fr32;
.TYPE   __iir_fr32,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

__iir_fr32:

   /* Initialize and read filter_state */

      P0 = [SP+12];                  // address of filter structure

      [--SP] = (R7:4);               // preserve reserved registers
                   
      I1 = R0;                       // address input array
      I3 = R1;                       // address output array
      CC = R2 <= 0;                  // check if number of input elements <= 0

      R7 = [P0++];                   // pointer to filter coefficients
      R6 = [P0++];                   // pointer to delay line
      R1 = [P0];                     // number of biquad stages

   /* Check for Error Conditions */

      IF CC JUMP .ret_end;           // return if number of samples <= 0

      CC = R1 <= 0;               
      IF CC JUMP .ret_end;           // return if number of stages <= 0


   /* Configure buffers and loop counters */

      R3 = R1 << 3;                  // size delay line 
                                     //   = num stages * 2 * size fract32
      R0 = 20;

      I0 = R6;                       // delay line as circular buffer
      B0 = R6;      
      L0 = R3;

      R0 *= R1;                      // size array of coefficients
                                     //   = num. stages * 5 * size fract32

      P1 = R2;                       // set loop counter to num samples
      P2 = R1;                       // set loop counter to num stages

      I2 = R7;                       // array of coefficients as circular buffer
      B2 = R7;                      
      L2 = R0;

      R1 = 0x80 (Z);                 // rounding factor

      R0 = R1 << 24                  // set constant 0x8000 0000
      || R7 = [I1++];                // load first input value

      // loop for number of samples
      LSETUP(.samples_start, .samples_end) LC0 = P1;

.samples_start:

         // loop for number of stages
         LSETUP(.stages_start, .stages_end) LC1 = P2;
.stages_start:

         /* Calculate Dm = X/Ym - (Dm-2 * A2) - (Dm-1 * A1) */

#if defined(__ADSPBLACKFIN__) && !defined(__ADSPLPBLACKFIN__)
            // __ADSPBF535__ core only
            R4 = 0;
            R7 = R4 - R7 (S)         // negate input value / Ym
#else
            R7 = -R7 (S)             // negate input value / Ym 
#endif
            || R2 = [I0++]           // load Dm-2
            || R5 = [I2++];          // load A2

#if defined(__ADSPBLACKFIN__) && !defined(__ADSPLPBLACKFIN__)
            // __ADSPBF535__ core only
            R5 = R4 - R5 (S)         // negate A2
#else
            R5 = -R5 (S)             // negate A2
#endif
            || R3 = [I0--]           // load Dm-1
            || R6 = [I2++];          // load A1           

#if defined(__ADSPBLACKFIN__) && !defined(__ADSPLPBLACKFIN__)
            // __ADSPBF535__ core only
            R6 = R4 - R6 (S);        // negate A1
#else
            R6 = -R6 (S);            // negate A1            
#endif

            A1 = R1.L * R1.L (M),    // fill A1 with rounding constant 0x4000
            A0 = R1.L * R1.L;        // fill A0 with rounding constant 0x8000

         /* Calculate Acc1 = (XHi * YLo), Acc0 + = (XLo * YLo) */
                                                               
            A1 += R2.H * R5.L (M), A0 += R2.L * R5.L (FU);  // Dm += Dm-2 * -A2
            A1 += R3.H * R6.L (M), A0 += R3.L * R6.L (FU);  // Dm += Dm-1 * -A1

            A0 = A0 >> 16;           // shift correct LoLo
            A0 += A1;                // add LoLo to Sum HiLo
            A1 = A0;                 // move sum into the correct accumulator
            A0 = 0;                  // reset accumulator

         /* Calculate Acc1 + = (YHi * XLo), Acc0 + = (YHi * XHi) */

            A1 += R0.H * R7.L (M), A0 += R0.H * R7.H;       // Dm += -1 * -X/Ym

            A1 += R5.H * R2.L (M), A0 += R5.H * R2.H        // Dm += Dm-2 * -A2
            || R5 = [I2++];                                 // load B2

            A1 += R6.H * R3.L (M), A0 += R6.H * R3.H        // Dm += Dm-1 * -A1 
            || R6 = [I2++];                                 // load B1

            A1 = A1 >>> 15
            || R7 = [I2++];                                 // load B0

            R4 = (A0 += A1);                                // Dm


         /* Calculate Ym = (Dm-2 * B2) + (Dm-1 * B1) + (Dm * B0) */

            A1 = R1.L * R1.L (M),    // fill A1 with rounding constant 0x4000
            A0 = R1.L * R1.L;        // fill A0 with rounding constant 0x8000

         /* Calculate Acc1 = (XHi * YLo), Acc0 + = (XLo * YLo) */

            A1 += R2.H * R5.L (M), A0 += R2.L * R5.L (FU);  // Ym += Dm-2 * B2
            A1 += R3.H * R6.L (M), A0 += R3.L * R6.L (FU);  // Ym += Dm-1 * B1
            A1 += R4.H * R7.L (M), A0 += R4.L * R7.L (FU);  // Ym += Dm   * B0 

            A0 = A0 >> 16;           // shift correct LoLo
            A0 += A1;                // add LoLo to Sum HiLo
            A1 = A0;                 // move sum into the correct accumulator
            A0 = 0;                  // reset accumulator

         /* Calculate Acc1 + = (YHi * XLo), Acc0 + = (YHi * XHi) */

            A1 += R5.H * R2.L (M), A0 += R5.H * R2.H;       // Ym += Dm-2 * B2

            A1 += R6.H * R3.L (M), A0 += R6.H * R3.H        // Ym += Dm-1 * B1
            || [I0++] = R3;                                 // store Dm-1

            A1 += R7.H * R4.L (M), A0 += R7.H * R4.H        // Ym += Dm   * B0
            || [I0++] = R4;                                 // store Dm

            A1 = A1 >>> 15;

.stages_end:
            R7 = (A0 += A1);                                // Ym


.samples_end:
         R7 = [I1++]                 // load next input value
         || [I3++] = R7;             // store filtered response


.ret_end:

      (R7:4) = [SP++];               // restore preserved registers 

      L0 = 0;                        // reset circular buffers
      L2 = 0;

      RTS;

.__iir_fr32.end:


/****************************************************************************
 *
 * fir_decima_fr32.asm : $Revision: 1.6 $
 *
 * (c) Copyright 2008-2009 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: fir_decima_fr32 - FIR decimation filter

    Synopsis:

        #include <filter.h>
        void fir_decima_fr32 (const fract32    input[],
                              fract32          output[],
                              int              nsamples,
                              fir_state_fr32  *filter_state);

        The filter function uses the following structure to maintain the
        state of the filter:

        typedef struct
        {
            fract32 *h;    /* filter coefficients            */
            fract32 *d;    /* start of delay line            */
            fract32 *p;    /* read/write pointer             */
            int k;         /* number of coefficients         */
            int l;         /* interpolation/decimation index */
        } fir_state_fr32;

    Description:

        The fir_decima function performs an FIR-based decimation filter.
        It generates the filtered decimated response of the input data input
        and stores the result in the output vector output. The number of input
        samples is specified by the argument nsamples, and the size of the 
        output vector should be nsamples / l where l is the decimation index.

        The function maintains the filter state in the structured variable
        filter_state, which must be declared and initialized before calling the
        function. The macro fir_init, defined in the filter.h header file, is
        available to initialize the structure.

        It is defined as:

        #define fir_init(state, coeffs, delay, ncoeffs, index)  \

            (state).h = (coeffs);   \
            (state).d = (delay);    \
            (state).p = (delay);    \
            (state).k = (ncoeffs);  \
            (state).l = (index)

        The characteristics of the filter are dependent upon the number of
        filter coefficients and their values, and on the decimation index
        supplied by the calling program. A pointer to the coefficients should
        be stored in filter_state->h, and filter_state->k should be set to the
        number of coefficients. The function assumes that the coefficients are
        stored in the normal order, thus filter_state->h[0] contains the first
        filter coefficient and filter_state->h[k-1] contains the last
        coefficient. The decimation index is supplied to the function in
        filter_state->l.

        Each filter should have its own delay line which is a vector of type
        fract32 and whose length is equal to the number of coefficients. The
        vector should be initially cleared to zero and should not otherwise
        be modified by the user program. The structure member filter_state->d
        should be set to the start of the delay line, and the function uses
        filter_state->p to keep track of its current position within the vector.

    Error Conditions:

        The fir_decima function checks that the number of input samples,
        the number of coefficients and the decimation index are greater than
        zero - if not, the function just returns.

    Algorithm:

        output[i] = sigma(coeffs[j] * input[i*d-j])

        where d = decimation index
              i = 0, 1, .., ((nsamples / d) - 1)
              j = 0, 1, .., (ncoeffs - 1)

    Implementation:

        The implementation is based on:

              write first input to the delay line

              loop for number of output samples - 1
                  loop for number of coefficients
                      apply coefficients to the values in
                      the delay line to compute filter response

                  write result to the output array

                  loop for decimation index
                      copy decimation index input samples to
                      the delay line

              <final output sample>
                  loop for number of coefficients
                      apply coefficients to the values in the
                      delay line to compute filter response

                  write result to the output array

                  loop for decimation index - 1
                      copy decimation index - 1 input samples to
                      the delay line

        The FIR filter is implementated as:

            A1 = 0x4000;
            A0 = 0x8000 || X = [Vector_X++] || Y = [Vector_Y++];

            Lsetup( . . . ) Loop_Counter = N;
                A1 + = XHi * YLo (M), A0 + = XLo * YLo (FU)
                || X = [Vector_X++] || Y = [Vector_Y++];

            A0 = A0 >> 16;
            A1 + = A0;
            A0 = 0;

            Lsetup( . . . ) Loop_Counter = N;
                A1 + = XLo * YHi (M), A0 + = XHi * YHi
                || X = [Vector_X++] || Y = [Vector_Y++];

            A1 = A1 >>> 15;
            R0 = (A0 + = A1);

        The array of coefficients and the delay line make use of 
        circular buffering.

    Example:

        #include <filter.h>
        #define NUM_INSAMPLES   256
        #define NUM_COEFFS       89
        #define NUM_DECIMATION   16
        #define NUM_OUTSAMPLES  (NUM_INSAMPLES / NUM_DECIMATION)

        fract32 input[NUM_INSAMPLES];
        fract32 output[NUM_OUTSAMPLES];
        segment("L1_data_a") fract32 coeffs[NUM_COEFFS];
        segment("L1_data_b") fract32 delay[NUM_COEFFS];

        fir_state_fr32 state;
        int i;

        for (i = 0; i < NUM_COEFFS; i++) /* clear the delay line */
        {
            delay[i] = 0;
        }

        fir_init(state, coeffs, delay, NUM_COEFFS, NUM_DECIMATION);
        fir_decima_fr32(input, output, NUM_INSAMPLES, &state);

    Cycle Counts:

        69 + (Ni-1) * (10 + (2 * Nc) + decimation index)
           +          (13 + (2 * Nc) + (decimation index - 1))

        where Ni is the number of input samples / decimation index and
              Nc is the number of coefficients.
              The decimation index is assumed to be greater than 1.

        (Measured for an ADSP-BF532 using version 5.0.0.35 of the ADSP-BF5xx
        Family Simulator and includes the overheads involved in calling
        the library procedure as well as the costs associated with argument
        passing).

        For best performance, the delay line and the filter coefficients
        should be allocated to separate memory blocks.

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR  libGroup      = filter.h;
.FILE_ATTR  libFunc       = __fir_decima_fr32;
.FILE_ATTR  libFunc       = fir_decima_fr32;
.FILE_ATTR  libName       = libdsp;
.FILE_ATTR  prefersMem    = internal;
.FILE_ATTR  prefersMemNum = "30";
.FILE_ATTR  FuncName      = __fir_decima_fr32;
#endif

#include <sys/anomaly_macros_rtl.h>

#if !defined(__ADSPLPBLACKFIN__)
/* Suppress Assembler Warning ea5504 "Memory store instruction use may trigger
** hardware anomaly 05-00-0165". The run-time libraries are not required to
** workaround this anomaly.
*/
.MESSAGE/SUPPRESS 5504;
#endif

.GLOBAL __fir_decima_fr32;
.TYPE   __fir_decima_fr32,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

__fir_decima_fr32:

   /* Initialize and read filter_state */

      M0 = P3;                       // preserve reserved register

      P0 = [SP+12];                  // address of filter structure

      I1 = R0;                       // address of input array
      I3 = R1;                       // address of output array
      CC = R2 <= 0;                  // check if number of input elements <= 0

      P3 = 16;                       // set loop counter division

      P1 = [P0++];                   // pointer to filter coefficients
      P2 = [P0++];                   // pointer to delay line
      R0 = [P0];                     // the read/write pointer
      R1 = [P0 + 4];                 // number of coefficients
      R3 = [P0 + 8];                 // decimation index


   /* Check for Error Conditions */

      IF CC JUMP .ret_end;

      CC = R1 <= 0;                  // check if number of coeff. <= 0
      IF CC JUMP .ret_end;

      CC = R3 <= 0;                  // check if decimation index <= 0
      IF CC JUMP .ret_end;


   /* Compute the size of the output vector */

      DIVS( R2, R3 );                // number of input samples / decimation idx
      LSETUP( .div_op, .div_op ) LC0 = P3;
.div_op:
         DIVQ( R2, R3 );         

      R2 = R2.L (Z);           
      R2 += -1;                      // number of output samples - 1
      CC = AZ;


   /* Configure buffers and loop counters */

      P3 = R3;                       // set inner loop counter to 
                                     // decimation index 

      R3 = R1 << 2;                  // compute size delay line, array of coeffs

      I2 = P1;                       // initialize I2 to array of filter coeffs
      B2 = P1;                       // make filter coeffs a circular buffer
      L2 = R3;

      P1 = R2;                       // set outer loop counter to 
                                     // number of output samples - 1

      I0 = R0;                       // initialize I0 to read/write pointer
      B0 = P2;                       // initialize I0 as a circular buffer
      L0 = R3;

      P2 = R1;                       // set inner loop counter to Nc

      R1 = 0x80 (Z);                 // set R1 to a constant which will
                                     // be used below as a rounding factor

      R3 = [I1++]                    // initialize R3 to Input_Vector[0]
      || R2 = [I2--];                // dummy read to position coeff pointer

      A1 = R1.L * R1.L (M),          // fill A1 with rounding constant 0x4000
      A0 = R1.L * R1.L               // fill A0 with rounding constant 0x8000
      || R2 = [I2--]                 // read first coefficient (=last element)
      || [I0++] = R3;                // write input value to delay line

      R3 = [I0++];                   // read from delay line

      IF CC JUMP .last_sample;
                                     // skip main body for num output = 1

#if defined(__WORKAROUND_SPECULATIVE_LOADS) || \
    WA_05000428
      NOP;
#endif

      // loop over the number of output samples - 1
      LSETUP( .fir_loop_start, .fir_loop_end ) LC0 = P1;

.fir_loop_start:

      /* Calculate Acc1 = (XHi * YLo), Acc0 + = (XLo * YLo) */

         // loop over the number of coefficients
         LSETUP( .loop_lolo_hilo, .loop_lolo_hilo ) LC1 = P2;
.loop_lolo_hilo:
            A1 += R2.H * R3.L (M), A0 += R2.L * R3.L (FU)
            || R2 = [I2--] || R3 = [I0++];


         A0 = A0 >> 16;              // shift correct LoLo
         A0 += A1;                   // add LoLo to Sum HiLo
         A1 = A0;                    // move sum into the correct accumulator
         A0 = 0;                     // reset accumulator


      /* Calculate Acc1 + = (YHi * XLo), Acc0 + = (YHi * XHi) */

         // loop over the number of coefficients
         LSETUP( .loop_hihi_lohi, .loop_hihi_lohi ) LC1 = P2;
.loop_hihi_lohi:
            A1 += R3.H * R2.L (M), A0 += R3.H * R2.H
            || R2 = [I2--] || R3 = [I0++];


         A1 = A1 >>> 15;
         R0 = (A0 += A1)             // compute filtered response 
         || I0 -= 4;                 // rewind pointer delay line

         A1 = R1.L * R1.L (M),       // fill A1 with rounding constant 0x4000
         A0 = R1.L * R1.L            // fill A0 with rounding constant 0x8000
         || R3 = [I1++]              // load next input value
         || [I3++] = R0;             // store filtered response

         // loop for decimation index  
         LSETUP( .loop_fill_delay, .loop_fill_delay ) LC1 = P3;
.loop_fill_delay:
            R3 = [I1++] || [I0++] = R3;

.fir_loop_end:
         R0 = [I1--]                 // dummy load to rewind input pointer
         || R3 = [I0++];             // load from delay line   


.last_sample:

   /* Compute final output sample */


   /* Calculate Acc1 = (XHi * YLo), Acc0 + = (XLo * YLo) */

      // loop over the number of coefficients
      LSETUP( .rem_loop_lolo_hilo, .rem_loop_lolo_hilo ) LC1 = P2;
.rem_loop_lolo_hilo:
         A1 += R2.H * R3.L (M), A0 += R2.L * R3.L (FU)
         || R2 = [I2--] || R3 = [I0++];


      A0 = A0 >> 16;                 // shift correct LoLo
      A0 += A1;                      // add LoLo to Sum HiLo
      A1 = A0;                       // move sum into the correct accumulator
      A0 = 0;                        // reset accumulator

      P3 += -1;                      // decrement loop counter

   /* Calculate Acc1 + = (YHi * XLo), Acc0 + = (YHi * XHi) */

      // loop over the number of coefficients
      LSETUP( .rem_loop_hihi_lohi, .rem_loop_hihi_lohi ) LC1 = P2;
.rem_loop_hihi_lohi:
         A1 += R3.H * R2.L (M), A0 += R3.H * R2.H
         || R2 = [I2--] || R3 = [I0++];


      A1 = A1 >>> 15;
      R0 = (A0 += A1)                // compute filtered response
      || I0 -= 4;                    // rewind pointer delay line

      R3 = [I1++]                    // load next input value
      || [I3++] = R0;                // store filtered response


      CC = P3 == 0;                  // don't fill delay line for 
                                     // a decimation index of 1
      IF CC JUMP .skip_fill;

#if defined(__WORKAROUND_SPECULATIVE_LOADS) || \
    WA_05000428
      NOP;
      NOP;
#endif

      // loop for decimation index - 1
      LSETUP( .rem_loop_fill_delay, .rem_loop_fill_delay ) LC1 = P3;
.rem_loop_fill_delay:
         R3 = [I1++] || [I0++] = R3;


.skip_fill:

      R0 = I0;
      [P0] = R0;                     // preserve read/write pointer position


.ret_end:
      P3 = M0;                       // restore reserved register
      L0 = 0;                        // reset circular buffers
      L2 = 0;

      RTS;

.__fir_decima_fr32.end:


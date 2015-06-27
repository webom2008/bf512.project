/****************************************************************************
 *
 * fir_interp_fr32.asm : $Revision: 1.3 $
 *
 * (c) Copyright 2008-2009 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: fir_interp_fr32 - FIR interpolation filter 

    Synopsis:

        #include <filter.h>
        void fir_interp_fr32 (const fract32    input[],
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

        The fir_interp function performs an FIR-based interpolation filter.
        It generates the interpolated filtered response of the input data input
        and stores the result in the output vector output. The number of input
        samples is specified by the argument nsamples, and the size of the
        output vector should be nsamples * l where l is the interpolation index.

        The filter characteristics are dependent upon the number of polyphase
        filter coefficients and their values, and on the interpolation factor
        supplied by the calling program.

        The fir_interp function assumes that the coefficients are stored
        in the following order:

            coeffs[(np * ncoeffs) + nc]

            where:    np = {0, 1, ..., nphases-1}
                      nc = {0, 1, ..., ncoeffs-1}

        In the above syntax, nphases is the number of polyphases and ncoeffs is
        the number of coefficients per polyphase. A pointer to the coefficients
        is passed into the fir_interp function via the argument filter_state,
        which is a structured variable that represents the filter state. This
        structured variable must be declared and initialized before calling the
        function. The filter.h header file contains the macro fir_init that
        can be used to initialize the structure and is defined as:

            #define fir_init(state, coeffs, delay, ncoeffs, index) \
                            (state).h = (coeffs);                  \
                            (state).d = (delay);                   \
                            (state).p = (delay);                   \
                            (state).k = (ncoeffs);                 \
                            (state).l = (index)

        The interpolation factor is supplied to the function in filter_state->l.
        A pointer to the coefficients should be stored in filter_state->h, and
        filter_state->k should be set to the number of coefficients per
        polyphase filter.

        Each filter should have its own delay line which is a vector of type
        fract32 and whose length is equal to the number of coefficients in each
        polyphase. The vector should be cleared to zero before calling the
        function for the first time and should not otherwise be modified by the
        user program. The structure member filter_state->d should be set to the
        start of the delay line, and the function uses filter_state->p to keep
        track of its current position within the vector.

    Error Conditions:

        The fir_interp function checks that the number of input samples,
        the number of coefficients and the interpolation index are greater
        than zero - if not, the function just returns.

    Algorithm:

        output[i*l+m] = sigma(coeffs[m*k+j] * input[i-j])

            where l = interpolation index
                  k = ncoeffs per polyphase
                  i = 0, 1, .., (nsamples - 1)
                  j = 0, 1, .., (ncoeffs per polyphase - 1)
                  m = 0, 1, .., (interpolation index - 1)

    Implementation:

        The interpolation filter is implemented as:

            f = 0;
            g = fir_state.p - fir_state.d; /* R/W Pointer Delay - Base delay */

            /* ITERATE NUMBER OF INPUT SAMPLES */
            for (i = 0; i < n; i++)

                read( in[i] );
                delay[g] = in[i];

                /* ITERATE NUMBER OF POYPHASES */
                for (j = 0; j < L; j++)

                    sum = 0;

                    /* ITERATE (NUMBER OF COEFFS/ POLYPHASES) */
                    for (m = 0; m < k; m++)
                        sum += delay[g--] * h[(j * k) + m];
                    end

                    output[f++] = sum;

                end
            end

        The array of coefficients and the delay line make use of 
        circular buffering.

    Example:

        #include <filter.h>
        #define NUM_INSAMPLES         257
        #define NUM_COEFFS            128
        #define NUM_INTERPOLATION      16
        #define NUM_OUTSAMPLES       (NUM_INSAMPLES * NUM_INTERPOLATION)
        #define NUM_COEFFS_PER_POLY  (NUM_COEFFS / NUM_INTERPOLATION)

        fract32 input [NUM_INSAMPLES];
        fract32 output[NUM_OUTSAMPLES];
        segment("L1_data_a") fract32 coeffs[NUM_COEFFS];
        segment("L1_data_b") fract32 delay [NUM_COEFFS_PER_POLY];

        fir_state_fr32 state;
        int i;

        for (i = 0; i < NUM_COEFFS_PER_POLY; i++) /* clear the delay line */
        {
            delay[i] = 0;
        }

        fir_init(state, coeffs, delay, NUM_COEFFS_PER_POLY, NUM_INTERPOLATION);
        fir_interp_fr32(input, output, NUM_INSAMPLES, &state);

    Cycle Counts:

        58 + Ni * (Np * (((Nc * 2) + 9) + 10))

        where Ni is the number of input samples
              Nc is the number of coefficients
              Np is the number of polyphases

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
.FILE_ATTR  libFunc       = __fir_interp_fr32;
.FILE_ATTR  libFunc       = fir_interp_fr32;
.FILE_ATTR  libName       = libdsp;
.FILE_ATTR  prefersMem    = internal;
.FILE_ATTR  prefersMemNum = "30";
.FILE_ATTR  FuncName      = __fir_interp_fr32;
#endif

#if !defined(__ADSPLPBLACKFIN__)
/* Suppress Assembler Warning ea5504 "Memory store instruction use may trigger
** hardware anomaly 05-00-0165". The run-time libraries are not required to
** workaround this anomaly.
*/
.MESSAGE/SUPPRESS 5504;
#endif

.GLOBAL __fir_interp_fr32;
.TYPE   __fir_interp_fr32,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

__fir_interp_fr32:

   /* Initialize and read filter_state */

      M0 = R4;                       // preserve reserved register
      P0 = [SP+12];                  // address of filter structure

      I1 = R0;                       // address of input array
      I3 = R1;                       // address of output array
      CC = R2 <= 0;                  // check if number of input elements <= 0

      P1 = [P0++];                   // pointer to filter coefficients
      P2 = [P0++];                   // pointer to delay line
      R0 = [P0];                     // the read/write pointer
      R1 = [P0 + 4];                 // number of coefficients
      R4 = [P0 + 8];                 // interpolation index

   /* Check for Error Conditions */

      IF CC JUMP .ret_end;

      CC = R1 <= 0;                  // check if number of coeffs <= 0
      IF CC JUMP .ret_end;

      CC = R4 <= 0;                  // check if interpolation index <= 0
      IF CC JUMP .ret_end;

   /* Configure buffers and loop counters */

      R3 = R1 << 2;                  // size delay line (= 4 * Nc)    

      I0 = R0;                       // delay line as circular buffer
      B0 = P2;                       
      L0 = R3;

      R0 = -R3;
      R0 += -4;

      P2 = R1;                       // set loop counter to Nc

      R3 *= R4;                      // size coeffs (= 4 * Nc * interp. idx.)

      I2 = P1;                       // array of coefficients as circular buffer
      B2 = P1;                       
      L2 = R3;
      M2 = R0;                       // offset to rewind coeff pointer to the
                                     // beginning of the current polyphase

      P1 = R4;                       // set loop counter to interpolation index

      R1.L = 0x80;                   // rounding factor 
      R1.H = 0x1;                    // constant

      A1 = R1.L * R1.L (M),          // fill A1 with rounding constant 0x4000
      A0 = R1.L * R1.L               // fill A0 with rounding constant 0x8000
      || R3 = [I1++]                 // load input[0]
      || R4 = [I2++];                // and load coeff[0][0]


      // loop for number of samples
.loop_samples_start:

         [I0--] = R3;                // write input value to delay line

         // loop for number of polyphases (= interpolation index)
         LSETUP(.poly_start, .poly_end) LC0 = P1;
.poly_start:

         /* Calculate Acc1 = (XHi * YLo), Acc0 + = (XLo * YLo) */

            // loop over the number of coefficients
            LSETUP(.loop_lolo_hilo,.loop_lolo_hilo) LC1 = P2;
.loop_lolo_hilo:
               A1 += R4.H * R3.L (M), A0 += R4.L * R3.L (FU)
               || R4 = [I2++] || R3 = [I0--];

            A0 = A0 >> 16            // shift correct LoLo
            || I2 += M2;             // rewind pointer coefficients
          
            A0 += A1                 // add LoLo to Sum HiLo
            || R4 = [I2++];          // load first coefficient

            A1 = A0;                 // move sum into the correct accumulator
            A0 = 0;                  // reset accumulator


         /* Calculate Acc1 + = (YHi * XLo), Acc0 + = (YHi * XHi) */

            // loop over the number of coefficients
            LSETUP(.loop_hihi_lohi,.loop_hihi_lohi) LC1 = P2;
.loop_hihi_lohi:
               A1 += R3.H * R4.L (M), A0 += R3.H * R4.H
               || R4 = [I2++] || R3 = [I0--];

            A1 = A1 >>> 15;
            R0 = (A0 += A1);

.poly_end:
            A1 = R1.L * R1.L (M),    // fill A1 with rounding constant 0x4000
            A0 = R1.L * R1.L         // fill A0 with rounding constant 0x8000
            || [I3++] = R0;          // store filtered response 

         R2.L = R2.L - R1.H (NS)     // decrement loop counter samples           
         || I0 += 4;

         R3 = [I1++]                 // load next sample
         || I0 += 4;            

         CC = R2 == 0;               // iterate until all inputs processed 
         IF !CC JUMP .loop_samples_start (BP);


      R0 = I0;
      [P0] = R0;                     // preserve read/write pointer position


.ret_end:
      R4 = M0;                       // restore reserved register
      L0 = 0;                        // reset circular buffers
      L2 = 0;

      RTS;

.__fir_interp_fr32.end:


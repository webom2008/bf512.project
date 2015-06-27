/****************************************************************************
 *
 * iirdf1_fr32.asm : $Revision: 1.4 $
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: iirdf1_fr32 - direct form I infinite impulse response filter

    Synopsis:

        #include <filter.h>
        void iirdf1_fr32 (const fract32     input[],
                          fract32           output[],
                          int               length,
                          iirdf1_state_fr32 *filter_state)

        The function uses the following structure to maintain the state of the
        filter.

        typedef struct
        {
            fract32 *c;     /* coefficients           */
            fract32 *d;     /* start of delay line    */
            fract32 *p;     /* read/write pointer     */
            int k;          /* 2*number of stages + 1 */
        } iirdf1_fr32_state;

    Description:

        The iirdf1_fr32 function implements a direct form I, infinite
        impulse filtered response of the input data, input and stores the
        result in the output vector, output. The number of input samples
        and the length of the output vector is specified by the argument
        length.

        The function maintains the filter state in the structured variable
        filter_state, which must be declared and initialized before calling
        the function. The macro iirdf1_init, defined in the filter.h header
        file, is available to initialize the structure.

        The macro is defined as:

            #define iirdf1_init(state, coeffs, delay, stages)  \

                (state).c = (coeffs);  \
                (state).d = (delay);   \
                (state).p = (delay);   \
                (state).k = (2*(stages)+1)

        The characteristics of the filter are dependent upon the filter
        coefficients and the number of stages. The A-coefficients and
        the B-coefficients for each stage are stored in a vector that is
        addressed by the pointer filter_state->c. This vector should be
        generated by the coeff_iirdf1_fr32 function. The variable
        filter_state->k should be set to the expression (2*stages) + 1.

        Note: Both the iirdf1_fr32 and iir_fr32 functions assume that the
              value of the A0 coefficients is 1.0, and that all other
              A-coefficients have been scaled according. For the iir_fr32
              function, this also implies that the value of the A0 coefficient
              is greater than both the A1 and A2 for all stages. This
              restriction does not apply to the iirdf1_fr32 function because
              the coefficients are specified as floating-point values to the
              coeff_iirdf1_fr32 function.

        Each filter should have its own delay line which is a vector of type
        fract32 and whose length is equal to (4 * stages) + 2. The vector
        should be initially cleared to zero and should not otherwise be
        modified by the user program. The structure member filter_state->d
        should be set to the start of the delay line, and the function uses
        filter_state->p to keep track of its current position within the
        vector. For optimum performance, the coefficient and state arrays
        should be allocated in separate memory blocks.

        The iirdf1_fr32 function will adjust the output by the scaling factor
        that was applied to the A-coefficients and the B-coefficients by the
        coeff_iirfd1_fr32 function.

        Note: It is possible the filters gain will cause the filtered response
              to be saturated. To avoid the saturation, the B coefficients can
              be scaled before calling the coeff_iirdf1_fr16 function. For
              more information, refer to the example below.

    Error Conditions:

        If the length of input vector is negative or zero, or the number of
        stages is less than one, then the function will return without
        modifying the output vector.

    Algorithm:

        V = B0 * input(i) + B1 * input(i-1) + B2 * input(i-2);
        output(i) = V + A1 * output(i-1) + A2 * output(i-2)

        where i = 0 to length - 1
        where B0,A1,B1,A2,B2,.. are the filter coefficients obtained from
              the coeff_iirdf1_fr32 function

    Implementation:

        The function requires a number of shift operations:

        a) To compute a fract32 product:
              output[k] += coeff[j] * delay[j] 
                         = Hi_coeff * Hi_delay + 
                           ((Hi_coeff * Lo_delay) + (Lo_coeff * Hi_delay) + 
                           ((Lo_coeff * Lo_delay) >> 16)) >>  15
         
        b) To apply the scaling factor:
              output[k] = output[k] << scaling factor
 
        To minimize the loss of significant digits, the product above is
        implemented as:
              output[k] += coeff[j] * delay[j]
                         = ((Hi_coeff * Hi_delay) << scaling factor) +
                           ((Hi_coeff * Lo_delay) + (Lo_coeff * Hi_delay) +
                           ((Lo_coeff * Lo_delay) >> 16)) >> sfactor2

              where sfactor2 = (15 - scaling factor)

        The implementation is as follows:

            A1 = 0x4000 >> scaling factor;     // Rounding factors  
            A0 = 0x8000;

            Load X = input[k], Load C = coeff[0];
            Store X in delay[j];

            Lsetup( . . . ) Loop_Counter = (4 * Nstages + 1);
                A1 + = XHi * CLo (M), A0 + = XLo * CLo (FU)
                || X = delay++ || C = coeff++;

            A0 = A0 >> 16;
            A1 += A0;
            A0 = 0;

            Lsetup( . . . ) Loop_Counter = (4 * Nstages + 1);
                A1 + = XLo * CHi (M), A0 + = XHi * CHi
                || X = delay++ || C = coeff++;

            A1 = A1 >>> (15 - scaling factor);
            A0 = A0 <<< scaling factor;
            R0 = (A0 += A1);

        The delay line is organized in the following manner
        (number of stages = 1):

            Xi | Yi-1 | Xi-1 | Yi-2 | Xi-2 | Yi

        The array of coefficients and the delay line make use of 
        circular buffering.

    Example:

        #include <filter.h>
        #include <vector.h>

        #define NSAMPLES 50
        #define NSTAGES  2

        /* Coefficients for the coeff_iirdf1_fr32 function */

        const long double a_coeffs[(2 * NSTAGES)] = { . . . };
        const long double b_coeffs[(2 * NSTAGES) + 1] = { . . . };

        /* Coefficients for the iirdf1_fr32 function */

        fract32 df1_coeffs[(4 * NSTAGES) + 2];

        /* Input, Output, Delay Line, and Filter State */

        fract32 input[NSAMPLES], output[NSAMPLES];
        fract32 delay[(4 * NSTAGES) + 2];
        iirdf1_state_fr32 state;
        long double gain;
        int i;

        /* Initialize filter description */

        iirdf1_init (state,df1_coeffs,delay,NSTAGES);

        /* Initialize the delay line */

        for (i = 0; i < ((4 * NSTAGES) + 2); i++)
            delay[i] = 0;

        /* Convert coefficients */

        if (gain >= 1.0L)
            vecsmltd (b_coeffs,(1.0L/gain),b_coeffs,((2*NSTAGES)+1));

        coeff_iirdf1_fr32 (a_coeffs,b_coeffs,df1_coeffs,NSTAGES);

        /* Call the function */

        iirdf1_fr32 (input,output,NSAMPLES,&state);

    Cycle Counts:

        46 + (Ni * (13 + (((4 * Ns) + 1) * 2)))

        where Ni is the number of samples and
              Ns is the number of stages.

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
.FILE_ATTR  libFunc       = __iirdf1_fr32;
.FILE_ATTR  libFunc       = iirdf1_fr32;
.FILE_ATTR  libName       = libdsp;
.FILE_ATTR  prefersMem    = internal;
.FILE_ATTR  prefersMemNum = "30";
.FILE_ATTR  FuncName      = __iirdf1_fr32;
#endif

#if !defined(__ADSPLPBLACKFIN__)
/* Suppress Assembler Warning ea5504 "Memory store instruction use may trigger
** hardware anomaly 05-00-0165". The run-time libraries are not required to
** workaround this anomaly.
*/

.MESSAGE/SUPPRESS 5504;
#endif

.GLOBAL __iirdf1_fr32;
.TYPE   __iirdf1_fr32,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

__iirdf1_fr32:


   /* Initialize and read filter_state */

      [--SP] = (R7:6);               // preserve registers
      P0 = [SP+20];                  // address of filter structure

      I1 = R0;                       // address of input array
      I3 = R1;                       // address of output array
      CC = R2 <= 0;                  // check if number of input elements <= 0

      P1 = [P0++];                   // pointer to filter coefficients
      P2 = [P0++];                   // pointer to delay line
      R0 = [P0++];                   // the read/write pointer
      R1 = [P0--];                   // (2 * number of stages + 1)

   /* Check for Error Conditions */

      IF CC JUMP .ret_end;

      CC = R1 < 3;                   // check if (2 * number of stages + 1) < 3
      IF CC JUMP .ret_end;


   /* Configure buffers and loop counters */

      R3 = R1 << 3;

      I2 = P1;                       // initialize I2 to array of filter coeffs
      B2 = P1;                       // make filter coeffs a circular buffer
      L2 = R3;

      R1 = R1 << 1;
      P1 = R2;                       // set outer loop counter to 
                                     // number of samples
      R1 += -1;

      I0 = R0;                       // initialize I0 to read/write pointer
      B0 = P2;                       // initialize I0 as a circular buffer
      L0 = R3;

      I2 -= 4;                       // &scaling factor 

      P2 = R1;                       // set inner loop counter to 
                                     // (4 * number of stages + 1)

      R1 = 0x80 (Z);                 // set R1 to a constant which will
                                     // be used below as a rounding factor
      R6 = 15 (Z);                   

      A1 = R1.L * R1.L (M),          // fill A1 with rounding constant 0x4000
      A0 = R1.L * R1.L               // fill A0 with rounding constant 0x8000
      || R7 = [I2++];

      R6 = R7 - R6 (NS)              // scaling LoHi+HiLo  
      || R3 = [I1++]                 // and initialize R3 to Input_Vector[0]
      || R2 = [I2++];                // and initialize R2 to Coeff_Vector[0]

      R7 = -R7;

      // loop over the number of samples
      LSETUP(.iir_loop_start,.iir_loop_end) LC0 = P1;

.iir_loop_start:

         A1 = ASHIFT A1 BY R7.L      // shift correct rounding     
         || [I0++] = R3;             // write input value to delay line

      /* Calculate Acc1 = (XHi * YLo), Acc0 + = (XLo * YLo) */

         // loop over the number of coefficients
         LSETUP(.loop_lolo_hilo,.loop_lolo_hilo) LC1 = P2;
.loop_lolo_hilo:
            A1 += R2.H * R3.L (M), A0 += R2.L * R3.L (FU)
            || R2 = [I2++] || R3 = [I0++];

         A0 = A0 >> 16               // shift correct LoLo
         || R2 = [I2++] 
         || R3 = [I0++];   

         A0 += A1;                   // add LoLo to Sum HiLo
         A1 = A0;                    // move sum into the correct accumulator
         A0 = 0;                     // reset accumulator


      /* Calculate Acc1 + = (YHi * XLo), Acc0 + = (YHi * XHi) */

         // loop over the number of coefficients
         LSETUP(.loop_hihi_lohi,.loop_hihi_lohi) LC1 = P2;
.loop_hihi_lohi:
            A1 += R3.H * R2.L (M), A0 += R3.H * R2.H
            || R2 = [I2++] || R3 = [I0++];

      /* Compute (A0 + (A1 >>> 15)) <<< s, where s = Scaling Factor 
      ** as:     (A0 <<< s) + (A1 >>> (15-s)) 
      ** to avoid the loss of the s least significant digits 
      ** which would be set to zero by the final shift operation.
      ** Rewind delay line pointer, load next input, load first coefficient
      */
         A1 = ASHIFT A1 BY R6.L || I0 -= 4;
         A0 = ASHIFT A0 BY R2.L || R3 = [I1++];
         R0 = (A0 += A1)        || R2 = [I2++];  
         [I0--] = R0; 

.iir_loop_end:
         A1 = R1.L * R1.L (M),       // fill A1 with rounding constant 0x4000
         A0 = R1.L * R1.L            // fill A0 with rounding constant 0x8000
         || [I3++] = R0;             // store filtered response 

      R0 = I0;

.ret_end:
      L0 = 0;                        // reset circular buffers
      L2 = 0;
      [P0] = R0;                     // preserve read/write pointer position

      (R7:6) = [SP++];               // restore preserved registers
      RTS;

.__iirdf1_fr32.end:

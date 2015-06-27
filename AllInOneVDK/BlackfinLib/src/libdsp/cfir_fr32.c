/****************************************************************************
 *
 * cfir_fr32.c : $Revision: 1.5 $
 *
 * (c) Copyright 2007-2010 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: cfir_fr32 - Complex Finite Impulse Response Filter

    Synopsis:

        #include <filter.h>
        void cfir_fr32 (const complex_fract32  input[],
                        complex_fract32        output[],
                        int                    length,
                        cfir_state_fr32        *filter_state);

        The filter function uses the following structure to maintain the
        state of the filter:

        typedef struct
        {
            int k;                 /*  number of coefficients  */
            complex_fract32 *h;    /*  filter coefficients     */
            complex_fract32 *d;    /*  start of delay line     */
            complex_fract32 *p;    /*  read/write pointer      */
        } cfir_state_fr32;

    Description:

        The cfir_fr32 function implements a complex finite impulse response
        (CFIR) filter. It generates the filtered response of the complex
        input data, input[] and stores the result in the complex output
        vector, output[].

        The function maintains the filter state in the structured variable
        filter_state, which must be declared and initialized before calling
        the function. The macro cfir_init, in the filter.h header file, is
        available to initialize the structure.

        It is defined as:

        #define cfir_init(state, coeffs, delay, ncoeffs) \
           (state).h = (coeffs);  \
           (state).d = (delay);   \
           (state).p = (delay);   \
           (state).k = (ncoeffs);

        The characteristics of the filter (passband, stopband, and so on)
        are dependent on the number of complex filter coefficients and their
        values. A pointer to the coefficients should be stored in
        filter_state->h and filter_state->k should be set to the number of
        coefficients.

        Each filter should have its own delay line which is a vector of type
        complex_fract32 and whose length is equal to the number of
        coefficients. The vector should be cleared to zero before calling the
        function for the first time and should not otherwise be modified by
        the user program. The structure member filter_state->d should be set
        to the start of the delay line, and the function uses filter_state->p
        to keep track of its current position within the vector.

    Error Conditions:

        The cfir_fr32 function checks that the number of samples and the number
        of coefficients are positive - if not, the function just returns.

    Algorithm:

        y[i] = sigma (h[j] * x[i-j]) where i = 0, 1, ..., length-1
                                     and   j = 0, 1, ..., k-1

    Implementation:

        Protects against overflow by using 64-bit fixed-point arithmetic.

        Makes use of the following optimizing pragmas:

        #pragma extra_loop_loads
        #pragma different_banks
        #pragma vector_for

        Makes use of circular buffering for the delay line.

    Example:

        #include <filter.h>
        #define LENGTH 85
        #define COEFFS_N 32

        complex_fract32 input[LENGTH];
        complex_fract32 output[LENGTH];
        complex_fract32 coeffs[COEFFS_N];
        complex_fract32 delay[COEFFS_N];

        cfir_state_fr32 state;
        int i;

        for (i = 0; i < COEFFS_N; i++) /* clear the delay line */
        {
            delay[i].re = 0;
            delay[i].im = 0;
        }
        cfir_init(state, coeffs, delay, COEFFS_N);
        cfir_fr32(input, output, LENGTH, state);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup =filter.h")
#pragma file_attr("libFunc  =cfir_fr32")
#pragma file_attr("libFunc  =__cfir_fr32")
#pragma file_attr("libName  =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")
#endif

#ifdef _MISRA_RULES
#pragma diag(suppress:misra_rule_6_3)
/* Suppress Rule 6.3 (advisory) whereby typedefs that indicate
** size and signedness should be used in place of the basic types.
**
** Rationale: The C/C++ manual documents the prototypes of the library
** functions using the intrinsic C data types and these prototypes must
** correspond to the actual prototypes that are defined in the header
** files. In addition, Rule 8.3 requires that the type of the result
** and the type of the parameters that are specified in a function
** definition and a function declaration are identical.
*/

#pragma diag(suppress:misra_rule_14_7)
/* Suppress Rule 14.7 (required) which requires that a function shall
** have a single point of exit at the end of the function.
**
** Rationale: The requirement is only suppressed provided that the only
** violation occurs at the beginning of the function when the function
** verifies its parameters are "correct".
*/

#pragma diag(suppress:misra_rule_17_4)
/* Suppress Rule 17.4 (required) which requires that array indexing shall be
** the only allowed form of pointer arithmetic.
**
** Rationale: The offending pointers are local references to external arrays
** referenced by the function's input arguments and it is therefore assumed
** that they will reference a valid range of memory.
*/
#endif /* _MISRA_RULES */

#include <filter.h>
#include <ccblkfn.h>
#include "xutil_fr.h"

extern void
cfir_fr32(const complex_fract32  _input[],
          complex_fract32        _output[],
          int                    _length,
          cfir_state_fr32       *_filter_state)

{
    const complex_fract32 *const input     = _input;
    complex_fract32 *__restrict    output    = _output;
    const int                    n_samples = _length;
    cfir_state_fr32 *__restrict    filter_state = _filter_state;

    const complex_fract32 *const coeffs = filter_state->h;
    complex_fract32 *__restrict    delay;
    complex_fract32              *rw_delay;

    complex_fract32 sample, product;
    int i, j, n_coeffs, index_delay;
    long long sum_re, sum_im;

    /* Read the filter state */
    n_coeffs = filter_state->k;
    rw_delay = filter_state->p;
    delay    = filter_state->d;

    /* Check the number of samples, number of coefficients */
    if ((n_samples <= 0) || (n_coeffs <= 0))
    {
        return;
    }

    /* Calculate Delay line pointer index */
    index_delay = rw_delay - delay;

#pragma extra_loop_loads
#pragma different_banks
#pragma vector_for

    /* Loop until all the input data have been processed */
    for (i = 0; i < n_samples; i++)
    {

        /* Read input data */
        sample = input[i];

        /* Store the input data to the delay line */
        delay[index_delay] = sample;

        /* Initialize the multiply and accumulation variables */
        sum_re = 0;
        sum_im = 0;

#pragma extra_loop_loads
#pragma different_banks
#pragma vector_for

        /* CFIR */
        for (j = 0; j < n_coeffs; j++)
        {

            /* Read the input data from the delay line */
            sample = delay[index_delay];

            /* Multiply and accumulation operation for filtering */
            product = cmlt_fr32 (sample, coeffs[j]);
            sum_re += (long long) product.re;
            sum_im += (long long) product.im;

            index_delay = circindex(index_delay, -1, (unsigned long) n_coeffs);
        }

        /* Store the filtered result to the output buffer */
        output[i].re = (fract32) __builtin_sat_fr1x64(sum_re);
        output[i].im = (fract32) __builtin_sat_fr1x64(sum_im);

        index_delay = circindex(index_delay, 1, (unsigned long) n_coeffs);
    }

    /* Update the read/write pointer */
    filter_state->p = &delay[index_delay];

}

/* End of File */


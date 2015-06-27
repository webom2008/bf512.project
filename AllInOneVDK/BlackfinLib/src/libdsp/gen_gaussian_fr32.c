/************************************************************************
 *
 * gen_gaussian_fr32 : $Revision: 1.6.8.1 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: gen_gaussian_fr32 - Gaussian Window Generator

    Synopsis:

        #include <window.h>
        void gen_gaussian_fr32 (fract32      gaussian_window[],
                                long double  alpha,
                                int          window_stride,
                                int          window_size);

    Description:
 
        The gen_gaussian function generates a vector containing a Gaussian
        window. The length of the window required is specified by the parameter
        window_size, and the parameter window_stride is used to space the
        window values within the output vector gaussian_window. The length of
        the output vector should therefore be window_size*window_stride.

        The parameter alpha is used to control the shape of the window. In
        general, the peak of the Gaussian window will become narrower and the
        leading and trailing edges will tend towards zero the larger alpha
        becomes. Conversely, the peak will get wider the more alpha tends
        towards zero.

    Error Conditions:

        The function gen_gaussian_fr32 exits without modifying the output
        vector if either the window_stride or the window_size is less than 1.

    Algorithm:

        The following equation is the basis of the algorithm.

            w[j] = exp(-0.5 * (alpha * ((2i - window_size + 1)/window_size))^2)

            where   i = 0, 1, .., window_size-1,
                    j = 0, window_stride, .., window_stride*(window_size-1)

        For the implementation, it is necessary to transform the algorithm:

            w[j] = exp(-0.5 * (alpha * ((2i - n + 1) / n))^2 )
                 = exp(-0.5 * ((alpha / n)^2 * (2i - n + 1)^2) )
                 = exp(-1 * (((sqrt(0.5) * alpha) / n)^2 * (2i - n + 1)^2) )
                 = exp(-1 * (alpha_over_n)^2 * (moving_index)^2) )
                 = exp(-1 * (alpha_over_n * moving_index)^2
            w[j] = exp(-tmp * tmp ),

            where  alpha_over_n = (sqrt(0.5) * alpha) / n
                   moving_index = 2i - n + 1
                   tmp = alpha_over_n * moving_index
                   j   = 0, window_stride, .., window_length * window_stride
                   n   = window_size

        Implementing the algorithm using long double arithmetic will ensure
        that all intermediate results will preserve more significant digits
        than can be stored in a variable of type fract32 in order to maximize
        accuracy.

    Example:

        #include <window.h>
        #include <filter.h>
        #include <vector.h>

        #define N_FFT 256

        fract32           signal[N_FFT];

        fract32           windowed_signal[N_FFT];
        complex_fract32   fft_output[N_FFT];
        complex_fract32   twiddle_table[N_FFT/2];

        fract32           gaussian_window[N_FFT];

        int block_exponent;


        gen_gaussian_fr32 (gaussian_window,
                           0.7071067811865L, 1, N_FFT);

        vecvmlt_fr32 (signal, gaussian_window,
                      windowed_signal, N_FFT);

        twidfftrad2_fr32 (twiddle_table, N_FFT);

        rfft_fr32 (windowed_signal, fft_output, twiddle_table,
                   1, N_FFT, &block_exponent, 2);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =window.h")
#pragma file_attr("libFunc  =gen_gaussian_fr32")
#pragma file_attr("libFunc  =__gen_gaussian_fr32")
#pragma file_attr("libName  =libdsp")
#pragma file_attr("prefersMem =external")
#pragma file_attr("prefersMemNum =70")
        /* (Use prefersMem=external because the function
        **  is usually called no more than once)
        */
#endif

#if !defined(TRACE)
#pragma optimize_for_space
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

#pragma diag(suppress:misra_rule_17_4)
/* Suppress Rule 17.4 (required) which requires that array indexing shall be
** the only allowed form of pointer arithmetic.
**
** Rationale: The offending pointers are local references to external arrays
** referenced by the function's input arguments and it is therefore assumed
** that they will reference a valid range of memory.
*/
#endif /* _MISRA_RULES */

#include <window.h>
#include <ccblkfn.h>
#include <math.h>
#include <fract2float_conv.h>

extern void
gen_gaussian_fr32 (fract32      _gaussian_window[],
                   long double  _alpha,
                   int          _window_stride,
                   int          _window_size)
{
    /* local copies of the arguments */

    fract32 *__restrict  gaussian_window = _gaussian_window;

    const int  window_size   = _window_size;
    const int  window_stride = _window_stride;

    long double  alpha = _alpha;

    long double  alpha_over_n, tmp;

    fract32  win_value;

    int  i, n_half, offset_lowhalf, offset_highhalf, moving_index;

    if (expected_true ((window_size > 0) && (window_stride > 0)))
    {
        n_half = shr_fr1x32 (window_size, 1);

        /* set mid-point for odd window length,
        ** dummy write if even window length
        */
        gaussian_window[n_half*window_stride] = 0x7FFFFFFF;

        offset_lowhalf = -window_stride;
        offset_highhalf = window_size * window_stride;

        alpha_over_n = (0.70710678118654752440L * alpha) / 
                       (long double) window_size; 

#pragma vector_for
        for (i = 0; i < n_half; i++)
        {
            moving_index = (2 * i) - (window_size - 1);
            tmp = alpha_over_n * (long double) moving_index;

            win_value = long_double_to_fr32 (expd (-tmp * tmp));

            offset_lowhalf += window_stride;
            offset_highhalf -= window_stride;

            gaussian_window[offset_lowhalf] = win_value;
            gaussian_window[offset_highhalf] = win_value;
        }
    }
}

/* End of File */

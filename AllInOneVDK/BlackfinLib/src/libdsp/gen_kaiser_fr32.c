/************************************************************************
 *
 * gen_kaiser_fr32.c : $Revision: 1.6.8.1 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: gen_kaiser_fr32 - Kaiser Window Generator

    Synopsis:

        #include <window.h>
        void gen_kaiser_fr32 (fract32      kaiser_window[],
                              long double  beta,
                              int          window_stride,
                              int          window_size);

    Description:

        The gen_kaiser function generates a vector containing a Kaiser window.
        The length of the window required is specified by the parameter
        window_size, and the parameter window_stride is used to space the
        window values within the output vector kaiser_window. The length of
        the output vector should therefore be window_size*window_stride.

        The parameter beta is used to specify the shape of the Kaiser window.
        Setting beta to zero will cause the function to generate a Rectangular
        window.

    Error Conditions:

        The function gen_kaiser_fr32 exits without modifying the output
        vector if either the window_stride or the window_size is less than 1.

    Algorithm:

        The following equation is the basis of the algorithm:

            w[j] = fI(beta * (1 - ((i - alpha)/alpha)^2)^0.5) / fI(beta)

            where  i = 0, 1, .., n-1,
                   j = 0, a, .., a*(n-1),
                   fI()  = zero-th order modified Bessel function
                           of the first kind,
                   alpha = (n-1) / 2
                   n = window_size
                   a = window_stride  

    Implementation:

        In order to reduce the number of floating point operations required
        to implement the Kaiser window function, it is necessary to transform
        the algorithm:

            w[j] = fI(beta * (1 - ((i - alpha) / alpha)^2)^0.5) / fI(beta)
                 = fI(beta * (1 - (a^2))^0.5) / fI(beta)
                 = fI(beta * ((1 - a) * (1 + a))^0.5) / fI(beta)
                 = fI(beta * ((1 - ((i - alpha) / alpha)) *
                              (1 + ((i - alpha) / alpha)))^0.5) / fI(beta)
                 = fI(beta * (((alpha - i + alpha) / alpha) *
                              ((alpha - i - alpha) / alpha))^0.5) / fI(beta)
                 = fI(beta * (((1 / alpha)^2) * (((2 * alpha) - i) * i))^0.5)
                                                                  / fI(beta)
                 = fI(beta * (1/|alpha|) * (((2 * ((n-1) / 2)) - i) * i)^0.5)
                                                                  / fI(beta)
                 = fI(beta * (1/|((n - 1) / 2)|) * ((n - 1 - i) * i)^0.5)
                                                                  / fI(beta)
                 = fI(beta * (2 / (n - 1)) * ((n - 1 - i) * i)^0.5)
                                                                  / fI(beta)


            where  i = 0, 1, .., n-1,
                   j = 0, a, .., a*(n-1),
                   fI()  = zero-th order modified Bessel function
                           of the first kind,
                   alpha = (n-1) / 2

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

        fract32           kaiser_window[N_FFT];

        int block_exponent;


        gen_kaiser_fr32 (kaiser_window, 3.0L, 1, N_FFT);

        vecvmlt_fr32 (signal, kaiser_window,
                      windowed_signal, N_FFT);

        twidfftrad2_fr32 (twiddle_table, N_FFT);

        rfft_fr32 (windowed_signal, fft_output, twiddle_table,
                   1, N_FFT, &block_exponent, 2);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =window.h")
#pragma file_attr("libFunc  =gen_kaiser_fr32")
#pragma file_attr("libFunc  =__gen_kaiser_fr32")
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
#include <builtins.h>
#include <math.h>
#include <fract2float_conv.h>

#include "xutil_fr.h"

extern void
gen_kaiser_fr32 (fract32      _kaiser_window[],
                 long double  _beta,
                 int          _window_stride,
                 int          _window_size)
{
    /* local copies of the arguments */

    fract32 *__restrict  kaiser_window = _kaiser_window;

    const int  window_size   = _window_size;
    const int  window_stride = _window_stride;

    long double  beta = _beta;

    long double  beta_over_n, inv_fI_beta, tmp;

    fract32  win_value;

    int  i, n_half, offset_lowhalf, offset_highhalf, moving_index, denum;

    if expected_true ((window_size > 1) && (window_stride > 0))
    {
        n_half = shr_fr1x32 (window_size, 1);

        /* set mid-point for odd window length,
        ** dummy write if even window length
        */
        kaiser_window[n_half*window_stride] = 0x7FFFFFFF;

        offset_lowhalf = -window_stride;
        offset_highhalf = window_size * window_stride;

        denum = window_size - 1; 
        beta_over_n = (2.0L * beta) / (long double) denum;
        inv_fI_beta = 1.0L / bessel0 (beta);

#pragma vector_for
#pragma loop_count(1,,)
        for (i = 0; i < n_half; i++)
        {
            moving_index = ((window_size - 1) - i) * i;
            tmp       = sqrtd ((long double) (moving_index));
            tmp      *= beta_over_n;
            win_value = long_double_to_fr32 (bessel0 (tmp) * inv_fI_beta);

            offset_lowhalf += window_stride;
            offset_highhalf -= window_stride;

            kaiser_window[offset_lowhalf] = win_value;
            kaiser_window[offset_highhalf] = win_value;
        }
    }
    else if expected_false ((window_size == 1) && (window_stride > 0))
    {
        kaiser_window[0] = 0x0;
    }
    else
    {
        ; /* NOP - Required for MISRA compliance */
    }
}

/* End of File */

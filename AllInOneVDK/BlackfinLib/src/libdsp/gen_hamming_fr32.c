/************************************************************************
 *
 * gen_hamming_fr32.c : $Revision: 1.5 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: gen_hamming_fr32 - Hamming Window Generator

    Synopsis:

        #include <window.h>
        void gen_hamming_fr32 (fract32  hamming_window[],
                               int      window_stride,
                               int      window_size);

    Description:

        The gen_hamming function generates a vector containing the Hamming
        window. The length of the window required is specified by the parameter
        window_size, and the parameter window_stride is used to space the
        window values within the output vector hamming_window. The length
        of the output vector should therefore be window_size * window_stride.

    Error Conditions:

        The function gen_hamming_fr32 exits without modifying the output
        vector if either the window_stride or the window_size is less than 1.

    Algorithm:

        The following equation applies:

            w[j] = 0.54 - (0.46 * cos ((2*pi*i)/(n-1)))

                   where  j = 0, a, .., a*(n-1)
                          a = window_stride
                          n = window_size

    Implementation:

        To improve performance, take advantage of the symmetrical nature
        of the algorithm:

            0.54 - (0.46 * cos ((2*pi*i)/(n-1))) =
                                 0.54 - (0.46 * cos ((2*pi*(n-1-i))/(n-1)))

            cos ((2*pi*i)/(n-1)) = cos ((2*pi*(n-1-i))/(n-1))
                                 = cos ((2*pi*((n-1)-(i)))/(n-1))
                                 = cos (((2*pi*(n-1))-(2*pi*i))/(n-1))
                                 = cos (((2*pi*(n-1))/(n-1))-((2*pi*i)/(n-1)))
                                 = cos (2*pi - ((2*pi*i)/(n-1)))
            cos (phi)            = cos (2*pi - phi)   

        Further gains can be had by converting the algorithm to fixed point
        arithmetic:

            0x451eb852 - (0x3ae147ae * __cos32_2PIx (i * (1/(n-1))))

        The function fract32 __cos32_2PIx (long long x) is a support function
        that makes use of the cos_fr32 function to compute the cosine values.

        The value of the first / last element is fixed at:

            0.54 - (0.46 * cos ((2*pi*0)/(n-1))) = 0.54 - 0.46 * cos(0)
                                                 = 0.08 ( = 0x0a3d70a4)

        For odd sample length, the mid point is fixed at:

            0.54 - (0.46 * cos ((2*pi*((n-1)/2))/(n-1))) =
            0.54 - (0.46 * cos ((((2*pi*n)-(2*pi))/2)/(n-1))) =
            0.54 - (0.46 * cos (((pi*n)-pi)/(n-1))) =
            0.54 - (0.46 * cos (((pi*(n-1))/(n-1))) =
            0.54 - (0.46 * cos (pi)) = 0.54 - (0.46 * -1) = 1

        For best accuracy, the function will use long long arithmetic to
        compute the inverse and to compute the input to the __cos32_2PIx
        support function. The remaining arithmetic will be done using
        fract32 builtin-functions.

    Example:

        #include <window.h>
        #include <filter.h>
        #include <vector.h>

        #define N_FFT 256

        fract32           signal[N_FFT];

        fract32           windowed_signal[N_FFT];
        complex_fract32   fft_output[N_FFT];
        complex_fract32   twiddle_table[N_FFT/2];

        fract32           hamming_window[N_FFT];

        int block_exponent;


        gen_hamming_fr32 (hamming_window, 1, N_FFT);

        vecvmlt_fr32 (signal, hamming_window,
                      windowed_signal, N_FFT);

        twidfftrad2_fr32 (twiddle_table, N_FFT);

        rfft_fr32 (windowed_signal, fft_output, twiddle_table,
                   1, N_FFT, &block_exponent, 2);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =window.h")
#pragma file_attr("libFunc  =gen_hamming_fr32")
#pragma file_attr("libFunc  =__gen_hamming_fr32")
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

#pragma diag(suppress:misra_rule_12_7)
/* Suppress Rule 12.7 (required) where bitwise operators shall not be
** applied to operands whose underlying type is signed.
**
** Rationale: To achieve acceptable performance, the function performs
** a multiply-by-reciprocal instead of a fixed-point division operation.
** This requires operands to be scaled, and hence *signed* intermediate
** results have to be 'un-scaled' to achieve correct results. The function
** therefore requires this rule to be suppressed.
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
#include <builtins.h>

#include "xutil_fr.h"

extern void
gen_hamming_fr32 (fract32  _hamming_window[],
                  int      _window_stride,
                  int      _window_size)
{
    /* local copies of the arguments */

    fract32 *__restrict  hamming_window = _hamming_window;

    const int  window_size   = _window_size;
    const int  window_stride = _window_stride;

    fract32  tmp, win_value;

    one_over_long_t  inv_n;
    long long        fx, alpha;

    int  i, n_half, offset_lowhalf, offset_highhalf;
    int  scale_mag;

    if expected_true ((window_size > 1) && (window_stride > 0))
    {
        n_half = shr_fr1x32 (window_size, 1);

        /* set mid-point for odd window length,
        ** dummy write if even window length
        */
        hamming_window[n_half*window_stride] = 0x7FFFFFFF;

        /* set first and last value in table to default */
        offset_lowhalf = 0;
        offset_highhalf = (window_size - 1) * window_stride;
        hamming_window[offset_lowhalf] = 0x0a3d70a4;
        hamming_window[offset_highhalf] = 0x0a3d70a4;

        fx = 0LL;
        inv_n = inverse_ul (window_size - 1);
        scale_mag = 33 - inv_n.sign_bits;

#pragma vector_for
        for (i = 1; i < n_half; i++)
        {
            /* i * (unsigned long)(1/(n-1)) */
            fx  += (long long) inv_n.scaled_reciprocal;

            /* scale (i/(n-1)) to fract 33.31 */
            alpha = fx >> scale_mag;

            /* 0.54 - (0.46 * cos ((2*pi*i)/(n-1))) */
            tmp = multr_fr1x32x32 (0x3ae147ae, __cos32_2PIx (alpha));
            win_value = sub_fr1x32 (0x451eb852, tmp);            

            offset_lowhalf += window_stride;
            offset_highhalf -= window_stride;

            hamming_window[offset_lowhalf] = win_value;
            hamming_window[offset_highhalf] = win_value;
        }
    }
    else if expected_false ((window_size == 1) && (window_stride > 0))
    {
        hamming_window[0] = 0;
    }
    else
    {
        ; /* NOP - Required for MISRA compliance */
    }
}

/* End of File */

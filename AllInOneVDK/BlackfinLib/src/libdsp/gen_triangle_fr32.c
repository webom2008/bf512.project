/************************************************************************
 *
 * gen_triangle_fr32.c : $Revision: 1.6 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: gen_triangle_fr32 - Triangle Window Generator

    Synopsis:

        #include <window.h>
        void gen_triangle_fr32 (fract32  triangle_window[],
                                int      window_stride,
                                int      window_size);

    Description:

        This function generates a vector containing a triangle window. The
        length of the window required is specified by the parameter window_size,
        and the parameter window_stride is used to space the window values
        within the output vector triangle_window. The length of the output
        vector should therefore be window_size*window_stride.

        Refer to the Bartlett window regarding the relationship between it
        and the triangle window.

    Error Conditions:

        The function gen_triangle_fr32 exits without modifying the output
        vector if either the window_stride or the window_size is less than 1.

    Algorithm:

        For even n, the following equation applies:

            w[j] = (2i + 1) / n,       0 < i < n/2
            w[j] = (2n - 2i - 1) / n,  n/2 <= i < n

                   where  j = 0, a, .., a*(n-1)
                          a = window_stride
                          n = window_size

        For odd n, the following equation applies:

            w[j] = (2i + 2) / (n + 1),    0 < i < n/2
            w[j] = (2n - 2i) / (n + 1),   n/2 <= i < n

                   where  j = 0, a, .., a*(n-1)
                          a = window_stride
                          n = window_size

    Implementation:

        Since the triangle window is symmetrical around the midpoint, there is 
        no need to distinguish between the case (0< i < n/2) and (n/2 <= i < n):

            even:
            (2i + 1) / n = (2n - 2(n - 1 - i) - 1) / n
                         = (2n - 2n + 2 + 2i -1) / n
                         = (2i + 1) / n          q.e.d.

            odd:
            (2i + 2) / (n + 1) = (2n - 2(n - 1 - i)) / (n + 1)
                               = (2n - 2n + 2 + 2i) / (n + 1)
                               = (2i + 2) / (n + 1)      q.e.d.

        In order to reduce code size, the same algorithm can be used for even
        and odd length window tables:

            Set even_odd_offset to 2 for odd length, 1 if even
            Set denominator to n+1 for odd length, n if even

        Thus the above algorithm reduces to:

            w[j] = (2i + even_odd_offset) / denominator
                 = (2i + even_odd_offset) * (1 / denominator) 

        The inverse of the denominator is computed using the inline support
        function inverse_ul(). The product of the numerator with the inverse
        is computed using the inline support function multiply_inverse_l().
        Both functions use long long arithmetic internally for best performance
        and accuracy.

    Example:

        #include <window.h>
        #include <filter.h>
        #include <vector.h>

        #define N_FFT 256

        fract32           signal[N_FFT];

        fract32           windowed_signal[N_FFT];
        complex_fract32   fft_output[N_FFT];
        complex_fract32   twiddle_table[N_FFT/2];

        fract32           triangle_window[N_FFT];

        int block_exponent;


        gen_triangle_fr32 (triangle_window, 1, N_FFT);

        vecvmlt_fr32 (signal, triangle_window,
                      windowed_signal, N_FFT);

        twidfftrad2_fr32 (twiddle_table, N_FFT);

        rfft_fr32 (windowed_signal, fft_output, twiddle_table,
                   1, N_FFT, &block_exponent, 2);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =window.h")
#pragma file_attr("libFunc  =gen_triangle_fr32")
#pragma file_attr("libFunc  =__gen_triangle_fr32")
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
#include <builtins.h>
#include "xutil_fr.h"

extern void
gen_triangle_fr32 (fract32  _triangle_window[],
                   int      _window_stride,
                   int      _window_size)
{
    /* local copies of the arguments */

    fract32 *__restrict  triangle_window = _triangle_window;

    const int  window_size   = _window_size;
    const int  window_stride = _window_stride;

    int  even_odd_offset = (((unsigned int) window_size & 1UL) == 0UL) ? 1L 
                                                           : 2L;
    int  denominator = (((unsigned int) window_size & 1UL) == 0UL) ? window_size
                                                           : (window_size + 1L);  
    fract32  win_value;

    int  n_half, i, offset_lowhalf, offset_highhalf, fx;

    one_over_long_t  inv_n;


    if expected_true ((window_size > 1) && (window_stride > 0))
    {
        n_half = shr_fr1x32 (window_size, 1);

        /* set mid-point for odd window length,
        ** dummy write if even window length
        */
        triangle_window[n_half*window_stride] = 0x7FFFFFFF;

        inv_n = inverse_ul (denominator);

        offset_lowhalf = -window_stride; 
        offset_highhalf = window_size * window_stride;

#pragma vector_for
#pragma loop_count(1,,)
        for (i = 0; i < n_half; i++)
        {
            fx = (2L * i) + even_odd_offset; 

            win_value = multiply_inverse_l (fx, inv_n);

            offset_lowhalf += window_stride;
            offset_highhalf -= window_stride;

            triangle_window[offset_lowhalf] = win_value;
            triangle_window[offset_highhalf] = win_value;
        }
    }
    else if expected_false ((window_size == 1) && (window_stride > 0))
    {
        triangle_window[0] = 0x7FFFFFFF;
    }
    else
    {
        ; /* NOP - Required for MISRA compliance */
    }
}

/* End of File */

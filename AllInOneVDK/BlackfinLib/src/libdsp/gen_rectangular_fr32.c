/************************************************************************
 *
 * gen_rectangular_fr32.c : $Revision: 1.5 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: gen_rectangular_fr32 - Rectangle Window Generator

    Synopsis:

        #include <window.h>
        void gen_rectangular_fr32 (fract32  rectangular_window[],
                                   int      window_stride,
                                   int      window_size);

    Description:

        The gen_rectangular function generates a vector containing a rectangular
        window. The length of the window required is specified by the parameter
        window_size, and the parameter window_stride is used to space the window
        values within the output vector rectangular_window. The length of the
        output vector should therefore be window_size * window_stride.

    Error Conditions:

        The function gen_rectangular_fr32 exits without modifying the output
        vector if either the window_stride or the window_size is less than 1.

    Algorithm:

        The following equation applies:

            w[j] = 0x7FFFFFFF

                   where  j = 0, a, .., a*(n-1)
                          a = window_stride
                          n = window_size

    Example:

        #include <window.h>
        #include <filter.h>
        #include <vector.h>

        #define N_FFT 256

        fract32           signal[N_FFT];

        fract32           windowed_signal[N_FFT];
        complex_fract32   fft_output[N_FFT];
        complex_fract32   twiddle_table[N_FFT/2];

        fract32           rectangular_window[N_FFT];

        int block_exponent;


        gen_rectangular_fr32 (rectangular_window, 1, N_FFT);

        vecvmlt_fr32 (signal, rectangular_window,
                      windowed_signal, N_FFT);

        twidfftrad2_fr32 (twiddle_table, N_FFT);

        rfft_fr32 (windowed_signal, fft_output, twiddle_table,
                   1, N_FFT, &block_exponent, 2);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =window.h")
#pragma file_attr("libFunc  =gen_rectangular_fr32")
#pragma file_attr("libFunc  =__gen_rectangular_fr32")
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

extern void
gen_rectangular_fr32 (fract32  _rectangular_window[],
                      int      _window_stride,
                      int      _window_size)
{
    /* local copies of the arguments */

    fract32 *__restrict  rectangular_window = _rectangular_window;

    const int  window_size   = _window_size;
    const int  window_stride = _window_stride;

    int  i;

    if ((window_size > 0) && (window_stride > 0))
    {

#pragma vector_for
#pragma loop_count(1,,)
        for (i = 0; i < window_size; i++)
        {          
            rectangular_window[i*window_stride] = 0x7FFFFFFF;
        }
    }
}

/* End of File */

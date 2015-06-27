/************************************************************************
 *
 * gen_bartlett_fr32.c : $Revision: 1.3 $
 *
 * (c) Copyright 2007-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: gen_bartlett_fr32 - Bartlett Window Generator

    Synopsis:

        #include <window.h>
        void gen_bartlett_fr32 (fract32 bartlett_window[],
                                int     window_stride,
                                int     window_size);

    Description:

        The gen_bartlett_fr32 function generates a vector containing the
        Bartlett window. The length is specified by parameter window_size,
        and the stride parameter window_stride is used to space the window
        values within the output vector bartlett_window. The length of the
        output vector should therefore be (window_stride*window_size).

        The Bartlett window is similar to the Triangle window but has the
        following different properties:

        - The Bartlett window always returns a window with a zero at either
          end of the sequence. Therefore, for odd n, the center section of
          a N+2 Bartlett window equals a N Triangle window.

        - For even n, the Bartlett window is the convolution of two
          rectangular sequences. There is no standard definition for the
          Triangle window for even n; the slopes of the Triangle window
          are slightly steeper than those of the Bartlett window.

    Error Conditions:

        The function gen_bartlett_fr32 is an effective no-op for the below
        condition:

            <window_size> <= 0 || <window_stride> <= 0

    Algorithm:

        <bartlett_window>[k] = 1 - abs[ (i - f) / f ]

        where f = (N-1)/2

        i = 0, 1, ... N-1
        k = 0, n, 2n, ...., n * (N-1)
        N = <window_size>
        n = <window_stride>

    Implementation:

        i)   Formula is simplified to bring the division operation outside
             the loop.

                                      |     (2 * i)         |
             bartlett_window[i] = 1 - | --------------- - 1 |
                                      | (window_size-1)     |

        ii)  If one uses 64-bit fixed-point arithmetic, then each floating
             point value can be converted to fixed point by using a scaling
             factor of 0x0000080000000000LL. The basic algorithm therefore
             becomes:
                                              |  SCALE*(2 * i)              |
                                  (1*SCALE) - | --------------- - (1*SCALE) |
                                              | (window_size-1)             |
             bartlett_window[i] = -------------------------------------------
                                                 SCALE


        iii) This is simplified to:

                                  (1*SCALE) - | (KONST * i) - (1*SCALE) |
             bartlett_window[i] = ---------------------------------------
                                                SCALE

                                   2*SCALE
                 where KONST = ---------------
                               (window_size-1)

        iv)  Performance is further enhanced by looping(calculating) for
             only half the bartlett window - the remaining half being the
             mirror image of the first half.

        v)   Makes use of the following optimizing pragmas:

             #pragma extra_loop_loads
             #pragma vector_for

    Example:

        #include <window.h>
        #include <filter.h>
        #include <vector.h>

        #define N_FFT 256

        fract32           signal[N_FFT];

        fract32           windowed_signal[N_FFT];
        complex_fract32   fft_output[N_FFT];
        complex_fract32   twiddle_table[N_FFT/2];

        fract32           bartlett_window[N_FFT];

        int block_exponent;


        gen_bartlett_fr32 (bartlett_window, 1, N_FFT);

        vecvmlt_fr32 (signal, bartlett_window,
                      windowed_signal, N_FFT);

        twidfftrad2_fr32 (twiddle_table, N_FFT);

        rfft_fr32 (windowed_signal, fft_output, twiddle_table,
                   1, N_FFT, &block_exponent, 2);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr ("libGroup =window.h")
#pragma file_attr ("libName =libdsp")
#pragma file_attr ("libFunc =gen_bartlett_fr32")
#pragma file_attr ("libFunc =__gen_bartlett_fr32")
#pragma file_attr ("prefersMem =external")
#pragma file_attr ("prefersMemNum =70")
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
#endif /* _MISRA_RULES */

#include <window.h>

#define SCALE_FACTOR    (0x0000080000000000LL)
#define ROUNDING        (0x0000000000000800LL)

extern void
gen_bartlett_fr32(fract32 _bartlett_window[],
                  int     _window_stride,
                  int     _window_size)
{

    /* local copies of the arguments */

    fract32 *const bartlett_window = _bartlett_window;
    const int      window_stride   = _window_stride;
    const int      window_size     = _window_size;

    int index1;              /* array index into the 1st half of the window */
    int index2;              /* array index into the 2nd half of the window */
    int i;

    long long konst;            /* set to (2*scale_factor)/(window_size-1)  */
    long long work;             /* intermediate result that is 4096 times
                                   larger than the fract32 equivalent value */
    unsigned long long uwork;
    unsigned int  loopcount;

    if ((window_size > 0) && (window_stride > 0))
    {
        /*
                                     |     (2 * i)         |
            bartlett_window[i] = 1 - | --------------- - 1 |
                                     | (window_size-1)     |
        */

        loopcount = (unsigned) window_size >> 1;
        if (loopcount > 0)
        {
            index1 = 0;
            index2 = window_stride * (window_size - 1);
            konst  = (2 * SCALE_FACTOR) / (window_size - 1);

#pragma extra_loop_loads
#pragma vector_for

            for (i = 0; i < loopcount; i++)
            {
                work = (konst * i);
                work = work - (1 * SCALE_FACTOR);

                if (work < 0)
                {
                    work = -work;
                }

                work = (1 * SCALE_FACTOR) - work;
                work = work + ROUNDING;
                uwork = (unsigned long long) work >> 12; /* (divide by 4096) */

                bartlett_window[index1] = (fract32) uwork;
                bartlett_window[index2] = (fract32) uwork;

                index1 += window_stride;
                index2 -= window_stride;
            }
        }

        if (((unsigned)window_size & 1UL) != 0)
        {
            bartlett_window[(int) loopcount * window_stride] = 
                                         (loopcount == 0x0) ? 0x0 : 0x7FFFFFFF;
        }
    }
    return;
}

/* End of File */

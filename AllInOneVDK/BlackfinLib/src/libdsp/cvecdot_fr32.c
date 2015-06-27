/************************************************************************
 *
 * cvecdot_fr32.c : $Revision: 1.4 $
 *
 * (c) Copyright 2007-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: cvecdot_fr32 - Complex vector dot product

    Synopsis:

        #include <vector.h>
        complex_fract32 cvecdot_fr32 (const complex_fract32 vector_a[],
                                      const complex_fract32 vector_b[],
                                      int                   length);

    Description:

        The cvecdot_fr32 function computes the complex dot product of the
        complex vectors vector_a[] and vector_b[], the complex vectors being
        length in size. The complex scalar result is returned by the function.

    Error Conditions:

        The cvecdot_fr32 function is an effective no-op if the parameter
        length is less than or equal to zero.

    Algorithm:

        product.re = Sigma(i = 0 to length-1)
                     ( (vector_a[i].re * vector_b[i].re) -
                       (vector_a[i].im * vector_b[i].im) )

        product.im = Sigma(i = 0 to length-1)
                     ( (vector_a[i].re * vector_b[i].im) +
                       (vector_a[i].im * vector_b[i].re) )

    Implementation:

        Uses 64-bit fixed-point arithmetic to avoid overflow, and makes
        use of the following optimizing pragmas:

          #pragma extra_loop_loads
          #pragma different_banks
          #pragma vector_for

    Example:

        #include <vector.h>

        #define SAMPLES_N 15

        complex_fract32 input_a[SAMPLES_N], input_b[SAMPLES_N];
        complex_fract32 result;

        result = cvecdot_fr32 (input_a, input_b, SAMPLES_N);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =cvecdot_fr32")
#pragma file_attr("libFunc  =__cvecdot_fr32")
#pragma file_attr("libName  =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#endif

#if !defined(TRACE)
#pragma optimize_for_speed
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

#include <vector.h>
#include "xutil_fr.h"

extern complex_fract32
cvecdot_fr32 (const complex_fract32 _vector_a[],
              const complex_fract32 _vector_b[],
              int                   _length)
{
    /* local copies of the arguments */
    const complex_fract32 *const vector_a = _vector_a;
    const complex_fract32 *const vector_b = _vector_b;
    const int                      length = _length;

    int i;
    long long sum_re = 0, sum_im = 0;
    complex_fract32 dot_product, product;

#pragma extra_loop_loads
#pragma different_banks
#pragma vector_for

    for (i = 0; i < length; i++)
    {
        product = cmlt_fr32 (vector_a[i], vector_b[i]);
        sum_re += (long long) product.re;
        sum_im += (long long) product.im;
    }

    dot_product.re = (fract32) __builtin_sat_fr1x64(sum_re);
    dot_product.im = (fract32) __builtin_sat_fr1x64(sum_im);

    return dot_product;
}

/* End of File */

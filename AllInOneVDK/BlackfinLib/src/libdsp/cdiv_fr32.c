/************************************************************************
 *
 * cdiv_fr32.c : $Revision: 1.8.6.1 $
 *
 * (c) Copyright 2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: cdiv_fr32 - Complex division

    Synopsis:

        #include <complex.h>
        complex_fract32 cdiv_fr32 (complex_fract32 a, complex_fract32 b);

    Description:

        The cdiv_fr32 function computes the complex division of the complex
        input a by the complex input b, and returns the result.

    Error Conditions:
  
        The cdiv_fr32 function returns zero if both the real and imaginary
        value of b is zero.

    Formula:

                 (a.real * b.real) + (a.imag * b.imag)
        c.real = ------------------------------------- 
                 (b.real * b.real) + (b.imag * b.imag)

                 (a.imag * b.real) - (a.real * b.imag)
        c.imag = ------------------------------------- 
                 (b.real * b.real) + (b.imag * b.imag)


    Implementation:

        Transform for |b.real| > |b.imag|

                     a.real + a.imag * (b.imag/b.real)
            c.real = ---------------------------------
                     b.real + b.imag * (b.imag/b.real)

                     a.imag - a.real * (b.imag/b.real)
            c.imag = ---------------------------------
                     b.real + b.imag * (b.imag/b.real)

        Transform for |b.real| < |b.imag|

                     a.real * (b.real/b.imag) + a.imag
            c.real = ---------------------------------
                     b.real * (b.real/b.imag) + b.imag

                     a.imag * (b.real/b.imag) - a.real
            c.imag = ---------------------------------
                     b.real * (b.real/b.imag) + b.imag


        Transform for b.real = b.imag

                     a.real + a.imag
            c.real = ---------------
                       b.real << 1

                     a.imag - a.real
            c.imag = ---------------
                       b.real << 1

        Transform for b.real = -(b.imag)

                     a.real - a.imag
            c.real = ---------------
                       b.real << 1

                     a.imag + a.real
            c.imag = ---------------
                       b.real << 1

    Example:

        #include <complex.h>

        complex_fract32 a = { 0x26666666, 0x0ccccccc };
        complex_fract32 b = { 0x33333333, 0x00000000 };
        complex_fract32 c;

        c = cdiv_fr32(a, b);   /* c = (0x60000000,0x20000000) */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =cdiv_fr32")
#pragma file_attr("libFunc  =__cdiv_fr32")
#pragma file_attr("libName  =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")
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
** Rationale: Shift operations are required to align the decimal points
** of fixed point numbers that can assume any sign. 
*/
#endif /* _MISRA_RULES */

#if !defined(ETSI_SOURCE)
#define ETSI_SOURCE
#endif /* ETSI_SOURCE */

#include <complex.h>
#include <fract_math.h>
#include <libetsi.h>
#include <ccblkfn.h>
#include <stdlib.h>
#include "xutil_fr.h"

extern complex_fract32
cdiv_fr32 (complex_fract32 _a,
           complex_fract32 _b)
{
    /* local copies of the arguments */

    complex_fract32 a = _a;
    complex_fract32 b = _b;

    complex_fract32 result;

    fract32  b_div;

    const long long  one = 0x0000000080000000LL;

    long long abs_b_re = llabs(b.re);
    long long abs_b_im = llabs(b.im);

    long long  numerator_re, numerator_im, denominator, cdiv_re, cdiv_im;
    int  mag_denom, mag_num_re, mag_num_im;
    

    if (expected_false(abs_b_re == abs_b_im))
    {

        if (expected_false(b.re == 0x0))
        {
            result.re = (fract32) 0x0;
            result.im = (fract32) 0x0;
            return result;
        }

        if (b.re == b.im)
        {
            /* b.real == b.imag 
            **
            **           a.real + a.imag
            **  c.real = ---------------
            **           b.real + b.imag
            **
            **           a.imag - a.real
            **  c.imag = ---------------
            **           b.real + b.imag
            */

            numerator_re = (long long) a.re + a.im;
            numerator_im = (long long) a.im - a.re;
        }
        else
        {
            /* |b.real| == |b.imag| 
            **
            **           a.real - a.imag
            **  c.real = ---------------
            **           b.real - b.imag
            **
            **           a.imag + a.real
            **  c.imag = ---------------
            **           b.real - b.imag
            */

            numerator_re = (long long) a.re - a.im;
            numerator_im = (long long) a.im + a.re;
        }
        denominator = ((long long) b.re) << 1;
    }
    else
    {
        if (expected_true (abs_b_re > abs_b_im))
        {
            /* |b.real| > |b.imag| 
            **
            **           a.real + a.imag * (b.imag/b.real)
            **  c.real = ---------------------------------
            **           b.real + b.imag * (b.imag/b.real)
            **
            **           a.imag - a.real * (b.imag/b.real)
            **  c.imag = ---------------------------------
            **           b.real + b.imag * (b.imag/b.real)
            */

            b_div = __builtin_sat_fr1x64 ((((long long) b.im) << 31) /
                                           ((long long) b.re));

            denominator  = (long long) multr_fr1x32x32 (b.im, b_div);
            denominator += (long long) b.re;

            numerator_re  = (long long) multr_fr1x32x32 (a.im, b_div);
            numerator_re += (long long) a.re;

            numerator_im  = (long long) a.im;
            numerator_im -= (long long) multr_fr1x32x32 (a.re, b_div);
        }
        else
        { 
            /* If |b.real| < |b.imag|
            **
            **          a.real * (b.real/b.imag) + a.imag
            ** c.real = ---------------------------------
            **          b.real * (b.real/b.imag) + b.imag
            **
            **          a.imag * (b.real/b.imag) - a.real
            ** c.imag = ---------------------------------
            **          b.real * (b.real/b.imag) + b.imag
            */

            b_div = __builtin_sat_fr1x64 ((((long long) b.re) << 31) /
                                           ((long long) b.im));

            denominator = (long long) multr_fr1x32x32 (b.re, b_div);
            denominator += (long long) b.im;

            numerator_re = (long long) multr_fr1x32x32 (a.re, b_div);
            numerator_re += (long long) a.im;

            numerator_im = (long long) multr_fr1x32x32 (a.im, b_div);
            numerator_im -= (long long) a.re;

        }
    }

    /* Compute the inverse of the denominator and 
    ** get magnitude of the top half of the result
    */
    denominator = (one << 31) / denominator;
    mag_denom  = (int) 32 - norm_fr1x32 ((fract32) (denominator >> 32));

    /* Get magnitude of the top half of the Numerators */  
    mag_num_re = (int) 32 - norm_fr1x32 ((fract32) (numerator_re >> 32));
    mag_num_im = (int) 32 - norm_fr1x32 ((fract32) (numerator_im >> 32));

    /* Protect against overflow in 64-bit result when computing 
    **   scaled inverse denominator * numerator
    ** (scale inverse denominator such that only the final 32-bit 
    **  product will overflow but not the intermediate 64-bit result).
    */
    cdiv_re = ((mag_denom >= 31) && (mag_num_re > 1))
     ? ((denominator >> 40) * numerator_re)
     : (((denominator >> mag_denom) * numerator_re) >> (31 - mag_denom));

    cdiv_im = ((mag_denom >= 31) && (mag_num_im > 1))
     ? ((denominator >> 40) * numerator_im)
     : (((denominator >> mag_denom) * numerator_im) >> (31 - mag_denom));

    result.re = __builtin_sat_fr1x64 (cdiv_re);
    result.im = __builtin_sat_fr1x64 (cdiv_im);

    return  result;
}

/* End of File */

/**************************************************************************
 *
 * lkdivi.c : $Revision: 1.6.4.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: lkdivi - divide a long int by a long int to give a long
                       accum result

    Synopsis:

        long accum lkdivi (long int dividend, long int divisor);

    Description:
 
        This function divides a long integer argument by a
        long integer argument to produce a long accum result. The result
        is rounded to nearest. The rounding to nearest may be either
        biased or unbiased.

    Error Conditions:

        None, all inputs valid.
        If the result overflows the long accum size, the value is
        saturated to long accum.

    Algorithm:

        Do division by repeated subtraction.

    Implementation:

        Do division of total bits of dividend and fract bits of divisor.
        Dividend implicitly shifted up by number of fract bits in
        divisor. Accumulate result bitwise in result type, overflowed
        bits are lost on shift up, so remember if we shifted off a set
        bit so we can saturate later.
        Finally, use the remainder we computed to decide which way to round.

    Example:
    
        #include <stdfix.h>
        long int dividend;
        long int divisor;
        long accum result;
  
        result = lkdivi (dividend,divisor);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=lkdivi")
#pragma file_attr("libFunc=_lkdivi")
#pragma file_attr("libName=libc")
#pragma file_attr("prefersMem=internal")
#pragma file_attr("prefersMemNum=30")

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

#pragma diag(suppress:misra_rule_19_7)
/* Suppress Rule 19.7 (advisory) whereby functions should be used in
** preference to function-like macros.
**
** Our offending macro is BIASED_ROUNDING. But it is more efficient to
** use a macro for this than to call a function, and it is machine dependent
** so shouldn't be inlined here.
*/
#endif

#define __DISABLE_LKDIVI 1

#include <stdint.h>
#include <stdbool.h>
#include <stdfix.h>
#include <limits.h>

#include "rounding.h"

extern long accum
lkdivi(long int _dividend,
       long int _divisor) 
{
    /* generate the result here */
    long int dividend = _dividend;
    long int divisor = _divisor;

    /* the absolute value of the divisor. */
    unsigned long int abs_divisor = (unsigned long int)divisor;

    /* the absolute value of the dividend */
    unsigned long int abs_dividend = (unsigned long int)dividend;

    /* the actual result, after rounding and saturation */
    long accum result = 0.0lk;

    /* an iterator */
    unsigned int i;

    /* did we overflow? */
    bool overflow = false;

    /* do we need to negate the result of the division of absolute values? */
    bool negate = false;

    if (divisor < 0)
    {
        abs_divisor = 0U - abs_divisor;
        negate = (!negate);
    }

    if (dividend < 0)
    {
        abs_dividend = 0U - abs_dividend;
        negate = (!negate);
    }

    if ((dividend != 0) && (divisor != 0))
    {
        /* the result of the division of absolute values as an integer 
        ** bit-pattern
        */
        uint_ulk_t i_result = 0U;

        /* the number of bits in the dividend */
        unsigned int dividend_bits = (  sizeof(long int)
                                   * (unsigned int)CHAR_BIT);

        /* the number of fractional bits in the divisor */
        unsigned int fract_bits = (unsigned int)LACCUM_FBIT;

        /* number of bits in the dividend minus 1 */
        unsigned long int dividend_bits_minus_1
          = (  (unsigned long int)sizeof(long int)
             * (unsigned long int)CHAR_BIT) - 1U;

        /* number of bits in the result minus 1 */
        uint_ulk_t result_bits_minus_1 = (uint_ulk_t)LACCUM_FBIT
                                       + (uint_ulk_t)LACCUM_IBIT - 1U;

        /* the accumulated remainder */
        unsigned long int remainder = 0U;

        /* saved remainder before we overwrite it with a possibly
        ** overflowing value
        */
        unsigned long int old_remainder;

        /* constant which is the smallest number that will overflow the
        ** divisor size if we shift it left by one.
        */
        unsigned long int top_bit_set
          = (unsigned long int)1U << dividend_bits_minus_1;

        /* the threshold at which saturation occurs */
        uint_ulk_t saturation_threshold
          = (uint_ulk_t)1U << result_bits_minus_1;

        /* a value to add to the result to round it */
        long accum rnd_inc = 0.0lk;

        /* a divisor used for comparison with the remainder. It is adjusted by
        ** 1 depending on whether we wish to round up in the half-way case or
        ** not.
        */
        unsigned long int divisor_for_comparison = abs_divisor;

        /* iterate over the bits in the dividend */
        for (i = 0U; i < dividend_bits; i++) 
        {
						/* the bit in the dividend that we are processing */
            unsigned long int bit = abs_dividend >> dividend_bits_minus_1;

            /* saved remainder before we overwrite it with a possibly
            ** overflowing value
            */
            old_remainder = remainder;

            /* put this bit in the remainder */
            remainder <<= 1U;
            remainder += bit;

            /* set the bit in the result if the remainder was greater than
            ** the divisor.
            */
            i_result <<= 1U;
            if (i_result >= saturation_threshold)
            {
                /* the top bit is set. The shift has overflowed. */
                overflow = true;
            }
            if (   (remainder >= abs_divisor)
                || (old_remainder >= top_bit_set)) 
            {
                i_result += 1U;
                remainder -= abs_divisor;
            }

            /* go to the next bit in the dividend */
            abs_dividend <<= 1U;
        }

        /* add zeros for all the fract bits that aren't in the dividend */
        for (i = 0U; i < fract_bits; i++)
        {
            /* saved remainder before we overwrite it with a possibly
            ** overflowing value
            */
            old_remainder = remainder;

            /* shift up the remainder */
            remainder <<= 1U;

            /* set the bit in the result if the remainder was greater than
            ** the divisor.
            */
            i_result <<= 1U;
            if (i_result >= saturation_threshold)
            {
                /* the top bit is set. The shift has overflowed. */
                overflow = true;
            }
            if (   (remainder >= abs_divisor)
                || (old_remainder >= top_bit_set))
            {
                i_result += 1U;
                remainder -= abs_divisor;
            }
        }

        /* we have the result and the remainder. Now work out how to
        ** round the result.
        */    

        /* we wish to round up in the half-way case when we're doing biased
        ** rounding, or when the current quotient is odd.
        */
        if (BIASED_ROUNDING)
        {
            if (!negate)
            {
                divisor_for_comparison--;
            }
        }
        else
        {
            if ((i_result & 1U) != 0U)
            {
                divisor_for_comparison--;
            }
        }

        /* round up if twice the remainder is greater than
        ** divisor_for_comparison.
        */
        if ((2U * remainder) > divisor_for_comparison)
        {
            if (negate)
            {
                rnd_inc = -LACCUM_EPSILON;
            }
            else
            {
                rnd_inc = LACCUM_EPSILON;
            }
        }

        /* we must now saturate down to 16 bits. */
        if (overflow)
        {
            if (negate)
            {
                result = LACCUM_MIN;
            }
            else
            {
                result = LACCUM_MAX;
            }
        }
        else
        {
            /* and convert to long accum. */
            result = lkbits((int_lk_t)i_result);
            if (negate)
            {
                result = -result;
            }
            result += rnd_inc;
        }
    }
    return result;
}

/* End of file */

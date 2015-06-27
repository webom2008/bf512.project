/**************************************************************************
 *
 * ukdivi.c : $Revision: 1.6.4.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: ukdivi - divide an unsigned int by an unsigned int to give an 
                       unsigned accum result

    Synopsis:

        unsigned accum ukdivi (unsigned int dividend, unsigned int divisor);

    Description:
 
        This function divides an unsigned integer argument by an unsigned
        integer argument to produce an unsigned accum result. The result
        is rounded to nearest. The rounding to nearest may be either
        biased or unbiased.

    Error Conditions:

        None, all inputs valid.
        If the result overflows the unsigned accum size, the value is
        saturated to unsigned accum.

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
        unsigned int dividend;
        unsigned int divisor;
        unsigned accum result;
  
        result = ukdivi (dividend,divisor);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=ukdivi")
#pragma file_attr("libFunc=_ukdivi")
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

#define __DISABLE_UKDIVI 1

#include <stdint.h>
#include <stdbool.h>
#include <stdfix.h>
#include <limits.h>

#include "rounding.h"

extern unsigned accum
ukdivi(unsigned int _dividend,
       unsigned int _divisor) 
{
    /* generate the result here */
    unsigned int dividend = _dividend;
    unsigned int divisor = _divisor;

    /* the actual result, after rounding and saturation */
    unsigned accum result = 0.0uk;

    /* an iterator */
    unsigned int i;

    /* did we overflow? */
    bool overflow = false;

    if ((dividend != 0U) && (divisor != 0U))
    {
        /* the result as an integer bit-pattern */
        uint_uk_t i_result = 0U;

        /* the number of bits in the dividend */
        unsigned int dividend_bits = (  sizeof(unsigned int)
                                      * (unsigned int)CHAR_BIT);

        /* the number of fractional bits in the divisor */
        unsigned int fract_bits = (unsigned int)UACCUM_FBIT;

        /* number of bits in the dividend minus 1 */
        unsigned int dividend_bits_minus_1
          = (  (unsigned int)sizeof(unsigned int)
             * (unsigned int)CHAR_BIT) - 1U;

        /* number of bits in the result minus 1 */
        uint_uk_t result_bits_minus_1 = (uint_uk_t)UACCUM_FBIT
                                      + (uint_uk_t)UACCUM_IBIT - 1U;

        /* the accumulated remainder */
        unsigned int remainder = 0U;

        /* saved remainder before we overwrite it with a possibly
        ** overflowing value
        */
        unsigned int old_remainder;

        /* constant which is the smallest number that will overflow the
        ** divisor size if we shift it left by one.
        */
        unsigned int top_bit_set = (unsigned int)1U << dividend_bits_minus_1;

        /* shift left by one of this value gives saturation. */
        uint_uk_t saturation_threshold = (uint_uk_t)1U << result_bits_minus_1;

        /* a value to add to the result to round it */
        unsigned accum rnd_inc = 0.0uk;

        /* a divisor used for comparison with the remainder. It is adjusted by
        ** 1 depending on whether we wish to round up in the half-way case or
        ** not.
        */
        unsigned int divisor_for_comparison = divisor;

        /* iterate over the bits in the dividend */
        for (i = 0U; i < dividend_bits; i++) 
        {
						/* the bit in the dividend that we are processing */
            unsigned int bit = dividend >> dividend_bits_minus_1;

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
            if (i_result >= saturation_threshold)
            {
                /* the top bit is set. The shift will overflow. */
                overflow = true;
            }
            i_result <<= 1U;
            if (   (remainder >= divisor)
                || (old_remainder >= top_bit_set)) 
            {
                i_result += 1U;
                remainder -= divisor;
            }

            /* go to the next bit in the dividend */
            dividend <<= 1U;
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
            if (i_result >= saturation_threshold)
            {
                /* the top bit is set. The shift will overflow. */
                overflow = true;
            }
            i_result <<= 1U;
            if (   (remainder >= divisor)
                || (old_remainder >= top_bit_set))
            {
                i_result += 1U;
                remainder -= divisor;
            }
        }

        /* we have the result and the remainder. Now work out how to
        ** round the result.
        */    

        /* we wish to round up in the half-way case when we're doing biased 
        ** rounding, or when the current quotient is odd.
        */
        if ((i_result & 1U) != 0U)
        {
            divisor_for_comparison--;
        }
        else if (BIASED_ROUNDING)
        {
            divisor_for_comparison--;
        }
        else
        {
            /* do nothing, keep misra happy */
        }

        /* round up if twice the remainder is greater than
        ** divisor_for_comparison.
        */
        if (   ((int)remainder < 0)
            || ((remainder << 1U) > divisor_for_comparison))
        {
            rnd_inc = UACCUM_EPSILON;
        }

        /* we must now saturate down to 16 bits. */
        if (overflow)
        {
            result = UACCUM_MAX;
        }
        else
        {
            /* and convert to unsigned accum. */
            result = ukbits(i_result);
        }
        result += rnd_inc;
    }
    return result;
}

/* End of file */

/**************************************************************************
 *
 * divurur_rnd.c : $Revision: 1.6.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: divurur_rnd - divide an unsigned fract by an unsigned
                            fract to give an unsigned fract result

    Synopsis:

        unsigned fract divurur_rnd (unsigned fract dividend,
                                    unsigned fract divisor);

    Description:
 
        This function divides an unsigned fract argument by an unsigned
        fract argument to produce unsigned fract result. The result
        is rounded to nearest. The rounding to nearest may be either
        biased or unbiased.

    Error Conditions:

        None, all inputs valid.
        If the result overflows the unsigned fract size, the value is
        saturated to unsigned fract.

    Algorithm:

        The algorithm is based on the assumptions that the twice the
        number of bits in an unsigned fract is at most 32.
        We first convert the dividend and divisor to 32-bit unsigned ints,
        and shift the dividend up by the number of fractional bits in an
        unsigned fract.
        We can then divide the two values to get the quotient.
        We then adjust the quotient to round up if the remainder is greater
        than half the divisor. We do this by comparison of twice the
        remainder with the divisor, with extra checks for the mid-point
        to perform unbiased rounding correctly.
        Finally we saturate the 32-bit value to the number of bits in
        the unsigned fract.

    Implementation:

    Example:
    
        None.

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=libc")
#pragma file_attr("libFunc=divurur_rnd")
#pragma file_attr("libFunc=__divurur_rnd")
#pragma file_attr("libName=libc")
#pragma file_attr("prefersMem=internal")
#pragma file_attr("prefersMemNum=30")

#endif

#ifdef _MISRA_RULES
#pragma diag(suppress:misra_rule_19_7)
/* Suppress Rule 19.7 (advisory) whereby functions should be used in
** preference to function-like macros.
**
** Our offending macro is BIASED_ROUNDING. But it is more efficient to
** use a macro for this than to call a function, and it is machine dependent
** so shouldn't be inlined here.
*/
#endif

#include <limits.h>

#include "embc_rt.h"
#include "rounding_blackfin.h"

#if UFRACT_FBIT > 16
#error This routine works only for fractional sizes of 16 bits or less.
#endif

extern unsigned fract
divurur_rnd(unsigned fract _dividend,
            unsigned fract _divisor) 
{
    /* generate the result here */
    unsigned fract result;

    /* the dividend zero-extended to an unsigned 32-bit integer */
    uint32_t u32_dividend = (uint32_t)bitsur(_dividend);

    /* the divisor zero-extended to an unsigned 32-bit integer */
    uint32_t u32_divisor = (uint32_t)bitsur(_divisor);

    /* the quotient found from dividing the two 32-bit values */
    uint32_t u32_quotient;

    /* the remainder found from dividing the two 32-bit values */
    uint32_t u32_remainder;

    /* a divisor used for comparison with the remainder. It is adjusted by
    ** 1 depending on whether we wish to round up in the half-way case or
    ** not.
    */
    uint32_t divisor_for_comparison;

    u32_dividend <<= (uint32_t)UFRACT_FBIT;
    divisor_for_comparison = u32_divisor;

    u32_quotient = u32_dividend / u32_divisor;
    u32_remainder = u32_dividend % u32_divisor;

    /* we wish to round up in the half-way case when we're doing biased 
    ** rounding, or when the current quotient is odd.
    */
    if ((u32_quotient & 1U) != 0U)
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

    /* round up if twice the remainder is greater than divisor_for_comparison.
    */
    if ((2U * u32_remainder) > divisor_for_comparison)
    {
        u32_quotient++; /* round up */
    }

    /* we must now saturate down to 16 bits. */
    if (u32_quotient > (uint32_t)bitsur(UFRACT_MAX))
    {
        result = UFRACT_MAX;
    }
    else
    {
        /* and convert to unsigned fract. */
        result = urbits((uint_ur_t)u32_quotient);
    }
    return result;
}

/* End of file */

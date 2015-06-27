/**************************************************************************
 *
 * divrr_rnd.c : $Revision: 1.6.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: divrr_rnd - divide a fract by a fract to give a fract result

    Synopsis:

        fract divrr_rnd (fract dividend, fract divisor);

    Description:
 
        This function divides a fract argument by a fract argument
        to produce a fract result. The result is rounded to nearest.
        The rounding to nearest may be either biased or unbiased.

    Error Conditions:

        None, all inputs valid.
        If the result overflows the fract size, the value is
        saturated to fract.

    Algorithm:

        The algorithm is based on the assumptions that the twice the
        number of bits in a fract is at most 32.
        We first convert the dividend and divisor to 32-bit ints,
        and shift the dividend up by the number of fractional bits in a
        fract.
        We can then divide the two values to get the quotient.
        We then adjust the quotient to round up if the absolute value of
        the remainder is greater than half the divisor. We do this by
        comparison of twice the remainder with the divisor, with extra checks
        for the mid-point to perform unbiased rounding correctly.
        Finally we saturate the 32-bit value to the number of bits in
        the fract.

    Implementation:

    Example:
    
        None.

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=libc")
#pragma file_attr("libFunc=divrr_rnd")
#pragma file_attr("libFunc=__divrr_rnd")
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

#pragma diag(suppress:misra_rule_19_15)
#endif

#include <limits.h>

#include "embc_rt.h"
#include "rounding_blackfin.h"

#if UFRACT_FBIT > 16
#error This routine works only for fractional sizes of 16 bits or less.
#endif

extern fract
divrr_rnd(fract _dividend,
          fract _divisor) 
{
    const int_r_t dividend = bitsr(_dividend);
    const int_r_t divisor = bitsr(_divisor);

    /* generate the result here */
    fract result;

    /* the dividend sign-extended to a signed 32-bit integer */
    int32_t i32_dividend = (int32_t)dividend;

    /* the dividend interpreted as unsigned */
    uint32_t u32_dividend = (uint32_t)i32_dividend;

    /* the divisor sign-extended to an unsigned 32-bit integer */
    int32_t i32_divisor = (int32_t)divisor;

    /* the absolute value of the divisor */
    int_r_t abs_divisor;

    /* the quotient found from dividing the two 32-bit values */
    int32_t i32_quotient;

    /* the remainder found from dividing the two 32-bit values */
    int32_t i32_remainder;

    /* the remainder found from dividing the two 32-bit values */
    int_r_t remainder;

    /* the absolute value of the remainder */
    int_r_t abs_remainder;

    /* a divisor used for comparison with the remainder. It is adjusted by
    ** 1 depending on whether we wish to round up in the half-way case or
    ** not.
    */
    uint_ur_t divisor_for_comparison;

    if (divisor < 0)
    {
        abs_divisor = -divisor;
    }
    else
    {
        abs_divisor = divisor;
    }

    u32_dividend <<= (uint32_t)FRACT_FBIT;
    i32_dividend = (int32_t)u32_dividend;
    divisor_for_comparison = (uint_ur_t)abs_divisor;

    i32_quotient = i32_dividend / i32_divisor;
    i32_remainder = i32_dividend % i32_divisor;
    remainder = (int_r_t)i32_remainder;

    if (remainder < 0)
    {
        abs_remainder = -remainder;
    }
    else
    {
        abs_remainder = remainder;
    }

    /* we wish to round up in the half-way case when we're doing biased 
    ** rounding, or when the current quotient is odd.
    */
    if (BIASED_ROUNDING)
    {
        if (i32_quotient > 0)
        {
            divisor_for_comparison--;
        }
    }
    else
    {
        if (((uint32_t)i32_quotient & 1U) != 0U)
        {
            divisor_for_comparison--;
        }
    }

    /* round up if twice the remainder is greater than divisor_for_comparison.
    */
    if ((2U * (uint_ur_t)abs_remainder) > divisor_for_comparison)
    {
        if (   (i32_quotient > 0)
            || ((i32_quotient == 0) && (remainder > 0)))
        {
            i32_quotient += 1; /* round up */
        }
        else
        {
            i32_quotient -= 1; /* round down */
        }
    }

    /* we must now saturate down to 16 bits. */
    if (i32_quotient > (int32_t)bitsr(FRACT_MAX))
    {
        result = FRACT_MAX;
    }
    else if (i32_quotient < (int32_t)bitsr(FRACT_MIN))
    {
        result = FRACT_MIN;
    }
    else
    {
        /* and convert to fract. */
        result = rbits((int_r_t)i32_quotient);
    }
    return result;
}

/* End of file */

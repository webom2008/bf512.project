/**************************************************************************
 *
 * rdivi_bf.c : $Revision: 1.6.4.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: rdivi - divide an int by an int to give
                      a fract result. The result is rounded to nearest.

    Synopsis:

        fract rdivi (int dividend, int divisor);

    Description:
 
        This function divides an int argument by an int argument
        to produce a fract result. The result is rounded to nearest.
        The rounding to nearest may be either biased or unbiased.

    Error Conditions:

        None, all inputs valid.
        If the result overflows the fract size, the value is
        saturated to fract.

    Algorithm:

        The algorithm is based on the assumptions that the sum of the
        number of bits in an int and a fract is at most 64.
        We first convert the dividend and divisor to 64-bit ints,
        and shift the dividend up by the number of fractional bits in a
        long fract.
        We can then divide the two values to get the quotient.
        We then adjust the quotient to round up if the absolute value of
        the remainder is greater than half the divisor. We do this by
        comparison of twice the remainder with the divisor, with extra checks
        for the mid-point to perform unbiased rounding correctly.
        Finally we saturate the 64-bit value to the number of bits in
        the long fract.

    Implementation:

    Example:

        int dividend, divisor;
        fract result;
   
        result = rdivi(dividend, divisor);
    
***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=libc")
#pragma file_attr("libFunc=rdivi")
#pragma file_attr("libFunc=__rdivi")
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

#define __DISABLE_RDIVI 1

#include <stdbool.h>
#include <stdint.h>
#include <stdfix.h>
#include <limits.h>

#include "rounding_blackfin.h"

#if UFRACT_FBIT > 32
#error This routine works only for fractional sizes of 32 bits or less.
#endif

extern fract
rdivi(int _dividend,
      int _divisor) 
{
    const int dividend = _dividend;
    const int divisor = _divisor;

    /* generate the result here */
    fract result;

    /* the dividend sign-extended to a signed 64-bit integer */
    int64_t ll_dividend = (int64_t)dividend;

    /* the dividend interpreted as unsigned */
    uint64_t ull_dividend = (uint64_t)ll_dividend;

    /* the divisor sign-extended to an unsigned 64-bit integer */
    int64_t ll_divisor = (int64_t)divisor;

    /* the absolute value of the divisor */
    int abs_divisor;

    /* the quotient found from dividing the two 64-bit values */
    int64_t ll_quotient;

    /* the remainder found from dividing the two 64-bit values */
    int64_t ll_remainder;

    /* the remainder found from dividing the two 64-bit values */
    int remainder;

    /* the absolute value of the remainder */
    int abs_remainder;

    /* a divisor used for comparison with the remainder. It is adjusted by
    ** 1 depending on whether we wish to round up in the half-way case or
    ** not.
    */
    unsigned int divisor_for_comparison;

    if (divisor < 0)
    {
        abs_divisor = -divisor;
    }
    else
    {
        abs_divisor = divisor;
    }

    ull_dividend <<= (uint64_t)FRACT_FBIT;
    ll_dividend = (int64_t)ull_dividend;
    divisor_for_comparison = (unsigned int)abs_divisor;

    ll_quotient = ll_dividend / ll_divisor;
    ll_remainder = ll_dividend % ll_divisor;
    remainder = (int)ll_remainder;

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
        if (ll_quotient > 0LL)
        {
            divisor_for_comparison--;
        }
    }
    else
    {
        if (((uint64_t)ll_quotient & 1ULL) != 0ULL)
        {
            divisor_for_comparison--;
        }
    }

    /* round up if twice the remainder is greater than divisor_for_comparison.
    */
    if ((2U * (unsigned int)abs_remainder) > divisor_for_comparison)
    {
        if (   (ll_quotient > 0)
            || ((ll_quotient == 0) && (remainder > 0)))
        {
            ll_quotient += 1; /* round up */
        }
        else
        {
            ll_quotient -= 1; /* round down */
        }
    }

    /* we must now saturate down to 16 bits. */
    if (ll_quotient > (int64_t)bitsr(FRACT_MAX))
    {
        result = FRACT_MAX;
    }
    else if (ll_quotient < (int64_t)bitsr(FRACT_MIN))
    {
        result = FRACT_MIN;
    }
    else
    {
        /* and convert to fract. */
        result = rbits((int_r_t)ll_quotient);
    }
    return result;
}

/* End of file */

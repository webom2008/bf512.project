/**************************************************************************
 *
 * ulrdivi_bf.c : $Revision: 1.6.4.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: ulrdivi - divide an unsigned long int by an unsigned
                        long int to give an unsigned long fract result

    Synopsis:

        unsigned long fract ulrdivi (unsigned long int dividend,
                                     unsigned long int divisor);

    Description:
 
        This function divides an unsigned long int argument by an unsigned
        long int argument to produce unsigned long fract result. The result
        is rounded to nearest. The rounding to nearest may be either
        biased or unbiased.

    Error Conditions:

        None, all inputs valid.
        If the result overflows the unsigned long fract size, the value is
        saturated to unsigned long fract.

    Algorithm:

        The algorithm is based on the assumptions that the sum of the
        number of bits in an unsigned long int and an unsigned long fract
        is at most 64.
        We first convert the dividend and divisor to 64-bit unsigned ints,
        and shift the dividend up by the number of fractional bits in an
        unsigned long fract.
        We can then divide the two values to get the quotient.
        We then adjust the quotient to round up if the remainder is greater
        than half the divisor. We do this by comparison of twice the
        remainder with the divisor, with extra checks for the mid-point
        to perform unbiased rounding correctly.
        Finally we saturate the 64-bit value to the number of bits in
        the unsigned long fract.

    Implementation:

    Example:
    
        #include <stdfix.h>
        unsigned long int dividend;
        unsigned long int divisor;
        unsigned long fract result;

        result = ulrdivi(dividend, divisor);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=ulrdivi")
#pragma file_attr("libFunc=__ulrdivi")
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

#define __DISABLE_ULRDIVI 1

#include <stdint.h>
#include <stdfix.h>
#include <limits.h>

#include "rounding_blackfin.h"

#if ULFRACT_FBIT > 32
#error This routine works only for fractional sizes of 32 bits or less.
#endif

extern unsigned long fract
ulrdivi(unsigned long int _dividend,
        unsigned long int _divisor) 
{
    /* generate the result here */
    unsigned long fract result;

    /* the dividend zero-extended to an unsigned 64-bit integer */
    uint64_t ull_dividend = (uint64_t)_dividend;

    /* the divisor zero-extended to an unsigned 64-bit integer */
    uint64_t ull_divisor = (uint64_t)_divisor;

    /* the quotient found from dividing the two 64-bit values */
    uint64_t ull_quotient;

    /* the remainder found from dividing the two 64-bit values */
    uint64_t ull_remainder;

    /* a divisor used for comparison with the remainder. It is adjusted by
    ** 1 depending on whether we wish to round up in the half-way case or
    ** not.
    */
    uint64_t divisor_for_comparison;

    ull_dividend <<= (uint64_t)ULFRACT_FBIT;
    divisor_for_comparison = ull_divisor;

    ull_quotient = ull_dividend / ull_divisor;
    ull_remainder = ull_dividend % ull_divisor;

    /* we wish to round up in the half-way case when we're doing biased 
    ** rounding, or when the current quotient is odd.
    */
    if ((ull_quotient & 1ULL) != 0ULL)
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
    if ((2ULL * ull_remainder) > divisor_for_comparison)
    {
        ull_quotient++; /* round up */
    }

    /* we must now saturate down to 16 bits. */
    if (ull_quotient > (uint64_t)bitsulr(ULFRACT_MAX))
    {
        result = ULFRACT_MAX;
    }
    else
    {
        /* and convert to unsigned fract. */
        result = ulrbits((uint_ulr_t)ull_quotient);
    }
    return result;
}

/* End of file */

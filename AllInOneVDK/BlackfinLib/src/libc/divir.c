/**************************************************************************
 *
 * divir.c : $Revision: 1.5.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: divir - divide an int value by a fract to give an int result

    Synopsis:

        int divir (int dividend, fract divisor);

    Description:
 
        This function divides an integer argument by an
        fract argument to produce an integer
        result. The result is rounded towards zero.

    Error Conditions:

        None, all inputs valid.
        If the result overflows the integer size, the value is truncated
        to int by discarding the higher order bits.

    Algorithm:

        Do division by repeated subtraction.

    Implementation:

        First convert the dividend and divisor to their absolute values,
        and work out the sign of the result.
        Do division of total bits of dividend and fract bits of divisor.
        Dividend implicitly shifted up by number of fract bits in
        divisor. Accumulate result bitwise in result type, overflowed
        bits are lost on shift up.
        Negate if necessary to get the final result.

    Example:
    
        #include <stdfix.h>
        int dividend;
        fract divisor;
        int result;

        result = divir (dividend,divisor);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=divir")
#pragma file_attr("libFunc=_divir")
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

#endif

#define __DISABLE_DIVIR 1

#include <stdbool.h>
#include <stdint.h>
#include <stdfix.h>
#include <limits.h>

extern int 
divir(int _dividend,
      fract _divisor) 
{
    int dividend = _dividend;
    fract divisor = _divisor;

    /* the divisor interpreted as an integer */
    int_r_t i_divisor = bitsr(divisor);

    /* the absolute value of the divisor. */
    uint_ur_t abs_divisor = (uint_ur_t)i_divisor;

    /* the absolute value of the dividend */
    unsigned int abs_dividend = (unsigned int)dividend; 

    /* the result of the division of the absolute values */
    unsigned int u_result = 0U;

    /* the final result */
    int result;

    /* an iterator */
    unsigned int i;

    /* do we need to negate the result of the division of absolute values? */
    bool negate = false;

    if (divisor < 0.0r)
    {
        abs_divisor = 0U - abs_divisor;
        negate = (!negate);
    }

    if (dividend < 0)
    {
        abs_dividend = 0U - abs_dividend;
        negate = (!negate);
    }

    if ((dividend != 0) && (divisor != 0.0r))
    {
        /* the number of bits in the dividend */
        unsigned int dividend_bits = (  sizeof(int)
                                      * (unsigned int)CHAR_BIT);

        /* the number of fractional bits in the divisor */
        unsigned int fract_bits = (unsigned int)FRACT_FBIT;

        /* number of bits in the dividend minus 1 */
        unsigned int dividend_shift_amount
          = (  (unsigned int)sizeof(int)
             * (unsigned int)CHAR_BIT) - 1U;

        /* number of bits in the divisor minus 1 */
        uint_ur_t divisor_shift_amount = (  (uint_ur_t)sizeof(int_r_t)
                                        * (uint_ur_t)CHAR_BIT) - 1U;

        /* the accumulated remainder */
        uint_ur_t remainder = 0U;

        /* saved remainder before we overwrite it with a possibly
        ** overflowing value
        */
        uint_ur_t old_remainder;

        /* constant which is the smallest number that will overflow the
        ** divisor size if we shift it left by one.
        */
        uint_ur_t top_bit_set = (uint_ur_t)1U << divisor_shift_amount;

        /* iterate over the bits in the dividend */
        for (i = 0U; i < dividend_bits; i++) 
        {
						/* the bit in the dividend that we are processing */
            unsigned int bit = abs_dividend >> dividend_shift_amount;

            /* saved remainder before we overwrite it with a possibly
            ** overflowing value
            */
            old_remainder = remainder;

            /* put this bit in the remainder */
            remainder <<= 1U;
            remainder += (uint_ur_t)bit;

            /* set the bit in the result if the remainder was greater than
            ** the divisor.
            */
            u_result <<= 1U;
            if (   (remainder >= abs_divisor)
                || (old_remainder >= top_bit_set)) 
            {
                u_result += 1U;
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
            u_result <<= 1U;
            if (   (remainder >= abs_divisor)
                || (old_remainder >= top_bit_set))
            {
                u_result += 1U;
                remainder -= abs_divisor;
            }
        }
    }

    if (negate)
    {
        result = 0 - (int)u_result;
    }
    else
    {
        result = (int)u_result;
    }
    return result;
}

/* End of file */

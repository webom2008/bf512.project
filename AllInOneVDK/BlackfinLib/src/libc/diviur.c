/**************************************************************************
 *
 * diviur.c : $Revision: 1.5.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: diviur - divide an unsigned int value by an
                       unsigned fract to give an unsigned int result

    Synopsis:

        unsigned int diviur (unsigned int dividend, unsigned fract divisor);

    Description:
 
        This function divides an unsigned integer argument by an
        unsigned fract argument to produce an unsigned integer
        result. The result is rounded towards zero.

    Error Conditions:

        None, all inputs valid.
        If the result overflows the integer size, the value is truncated
        to unsigned int by discarding the higher order bits.

    Algorithm:

        Do division by repeated subtraction.

    Implementation:

        Do division of total bits of dividend and fract bits of divisor.
        Dividend implicitly shifted up by number of fract bits in
        divisor. Accumulate result bitwise in result type, overflowed
        bits are lost on shift up.

    Example:
    
        #include <stdfix.h>
        unsigned int dividend;
        unsigned fract divisor;
        unsigned int result;

        result = diviur (dividend,divisor);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=diviur")
#pragma file_attr("libFunc=_diviur")
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

#define __DISABLE_DIVIUR 1

#include <stdint.h>
#include <stdfix.h>
#include <limits.h>

extern unsigned int 
diviur(unsigned int _dividend,
       unsigned fract _divisor) 
{
    unsigned int dividend = _dividend;
    unsigned fract divisor = _divisor;

    /* the divisor interpreted as an integer */
    uint_ur_t i_divisor = bitsur(divisor);

    /* the final result */
    unsigned int result = 0U;

    /* an iterator */
    unsigned int i;

    if ((dividend != 0U) && (divisor != 0.0ur))
    {
        /* the number of bits in the dividend */
        unsigned int dividend_bits = (  sizeof(unsigned int)
                                      * (unsigned int)CHAR_BIT);

        /* the number of fractional bits in the divisor */
        unsigned int fract_bits = (unsigned int)UFRACT_FBIT;

        /* number of bits in the dividend minus 1 */
        unsigned int dividend_shift_amount
          = (  (unsigned int)sizeof(unsigned int)
             * (unsigned int)CHAR_BIT) - 1U;

        /* number of bits in the divisor minus 1 */
        uint_ur_t divisor_shift_amount = (  (uint_ur_t)sizeof(uint_ur_t)
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
            unsigned int bit = dividend >> dividend_shift_amount;

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
            result <<= 1U;
            if (   (remainder >= i_divisor)
                || (old_remainder >= top_bit_set)) 
            {
                result += 1U;
                remainder -= i_divisor;
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
            result <<= 1U;
            if (   (remainder >= i_divisor)
                || (old_remainder >= top_bit_set))
            {
                result += 1U;
                remainder -= i_divisor;
            }
        }
    }
    return result;
}

/* End of file */

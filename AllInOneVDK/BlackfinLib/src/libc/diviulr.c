/**************************************************************************
 *
 * diviulr.c : $Revision: 1.5.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: diviulr - divide an unsigned long int value by an unsigned
                        long fract to give an unsigned long int result

    Synopsis:

        unsigned long int diviulr (unsigned long int dividend,
                                   unsigned long fract divisor);

    Description:
 
        This function divides an unsigned long integer argument by an
        unsigned long fract argument to produce an unsigned long integer
        result. The result is rounded towards zero.

    Error Conditions:

        None, all inputs valid.
        If the result overflows the integer size, the value is truncated
        to unsigned long int by discarding the higher order bits.

    Algorithm:

        Do division by repeated subtraction.

    Implementation:

        Do division of total bits of dividend and fract bits of divisor.
        Dividend implicitly shifted up by number of fract bits in
        divisor. Accumulate result bitwise in result type, overflowed
        bits are lost on shift up.

    Example:
    
        #include <stdfix.h>
        unsigned long int dividend;
        unsigned long fract divisor;
        unsigned long int result;

        result = diviulr (dividend,divisor);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=diviulr")
#pragma file_attr("libFunc=_diviulr")
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

#define __DISABLE_DIVIULR 1

#include <stdint.h>
#include <stdfix.h>
#include <limits.h>

extern unsigned long int 
diviulr(unsigned long int _dividend,
        unsigned long fract _divisor) 
{
    unsigned long int dividend = _dividend;
    unsigned long fract divisor = _divisor;

    /* the divisor interpreted as an integer */
    uint_ulr_t i_divisor = bitsulr(divisor);

    /* the final result */
    unsigned long int result = 0U;

    /* an iterator */
    unsigned long int i;

    if ((dividend != 0U) && (divisor != 0.0ulr))
    {
        /* the number of bits in the dividend */
        unsigned int dividend_bits = (  sizeof(unsigned long int)
                                      * (unsigned int)CHAR_BIT);

        /* the number of fractional bits in the divisor */
        unsigned int fract_bits = (unsigned int)ULFRACT_FBIT;

        /* number of bits in the dividend minus 1 */
        unsigned long int dividend_shift_amount
          = (  (unsigned long int)sizeof(unsigned long int)
             * (unsigned long int)CHAR_BIT) - 1U;

        /* number of bits in the divisor minus 1 */
        uint_ulr_t divisor_shift_amount = (  (uint_ulr_t)sizeof(uint_ulr_t)
                                          * (uint_ulr_t)CHAR_BIT) - 1U;

        /* the accumulated remainder */
        uint_ulr_t remainder = 0U;

        /* saved remainder before we overwrite it with a possibly
        ** overflowing value
        */
        uint_ulr_t old_remainder;

        /* constant which is the smallest number that will overflow the
        ** divisor size if we shift it left by one.
        */
        uint_ulr_t top_bit_set = (uint_ulr_t)1U << divisor_shift_amount;

        /* iterate over the bits in the dividend */
        for (i = 0U; i < dividend_bits; i++) 
        {
						/* the bit in the dividend that we are processing */
            unsigned long int bit = dividend >> dividend_shift_amount;

            /* saved remainder before we overwrite it with a possibly
            ** overflowing value
            */
            old_remainder = remainder;

            /* put this bit in the remainder */
            remainder <<= 1U;
            remainder += (uint_ulr_t)bit;

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

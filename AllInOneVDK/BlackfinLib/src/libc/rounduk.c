/**************************************************************************
 *
 * rounduk.c : $Revision: 1.6.4.1 $
 * (c) Copyright 2008-2010 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: rounduk - round an unsigned accum value to a given precision

    Synopsis:

        unsigned accum rounduk (unsigned accum parm, int num_fract_bits);

    Description:
 
        This function rounds a given unsigned accum parameter
        to the number of fractional bits given by the second parameter. 
        The rounding is performed to-nearest. In the case where the value
        is exactly half-way between two values of the destination precision,
        the value is either rounded up (biased rounding), or rounded to
        nearest even (unbiased rounding), depending on the current setting
        of the RND_MOD bit. The rounding may saturate, so that 0x7fffffffff
        is a valid output.

    Error Conditions:

        If the input precision is greater than or equal to UACCUM_FBIT,
        no rounding occurs. If the input precision is less than or equal to 0,
        rounding is to the nearest whole number.

    Algorithm:

        Add constant of 1 in the place after the intended LSB of the result.
        If the addition saturates, the saturated value is returned.
        Truncate off the bits after the intended LSB of the result.

    Implementation:

    Example:
    
        #include <stdfix.h>
        unsigned accum a;
        unsigned accum rnda;

        rnda = rounduk(a, 18);
        

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=rounduk")
#pragma file_attr("libFunc=_rounduk")
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

#ifdef __FX_NO_ACCUM
#error This file requires _Accum types
#endif

#include "rounding.h"

#define __DISABLE_ROUNDUK 1
#include <stdbool.h>
#include <stdint.h>
#include <stdfix.h>
#include <builtins.h>

extern unsigned accum 
rounduk(unsigned accum _a,
        int _num_fract_bits) 
{
    const unsigned accum a = _a;
    const int num_fract_bits = _num_fract_bits;

    /* the result after rounding */
    unsigned accum result;

    if (expected_false((num_fract_bits >= UACCUM_FBIT) ? (int)1 : (int)0))
    {

        /* no bits to round off.
        */
        result = a;

    }
    else
    {

        /* the accum bit-pattern interpreted as an unsigned integer */
        uint_uk_t a_ui = bitsuk(a);

        /* num_fract_bits, but negative values clipped to zero.
        ** If we asked for a precision less than zero, assume a precision of
        ** zero is fine, because we have no integer bits.
        */
        int clipped_num_fract_bits
            = (num_fract_bits < 0) ? 0 : num_fract_bits;

        /* the position of the bit after the last one we want to keep,
        ** relative to the LSB of the operand.
        */
        uint32_t most_significant_rounded_bit
            = ((uint32_t)UACCUM_FBIT - 1U) - (uint32_t)clipped_num_fract_bits;

        /* the amount we have to add before truncation in order to get
        ** rounding.
        */
#if UACCUM_FBIT == ULFRACT_FBIT
        /* in this case we can do the rounding more efficiently */
        uint_ulr_t increment_i
             = (uint_ulr_t)1U << (uint_ulr_t)most_significant_rounded_bit;
#else
        uint_uk_t increment_i
             = (uint_uk_t)1U << (uint_uk_t)most_significant_rounded_bit;
#endif

        /* is biased rounding enabled? */
        bool biased_rounding = false;

        if (BIASED_ROUNDING)
        {
            biased_rounding = true;
        }

        /* do the addition. This is a non-saturating addition, so if the
        ** result is smaller than the original then we overflowed, in
        ** which case we must saturate.
        */
        a_ui += (uint_uk_t)increment_i;

        if (a_ui < bitsuk(a))
        {
            /* If the addition above saturated, we don't want to AND off
            ** the bottom bits because the result should saturate, so just
            ** return the max possible value.
            */
            result = UACCUM_MAX;
        }
        else
        {
            /* truncating a_ui gives us the right answer for biased rounding,
            ** so we set up a mask to do that. Initially we compute the
            ** mask with ones in the places that we are throwing away, but we
            ** eventually invert it.
            ** This mask only covers the fractional bits.
            */
#if UACCUM_FBIT == ULFRACT_FBIT
            uint_ulr_t mask = increment_i;
#else
            uint_uk_t mask = increment_i;
#endif

            /* the mask above, but with 1's in the bits representing integer
            ** bits.
            */
            uint_uk_t mask_long;

            mask <<= 1U;
            mask -= 1U;

            /* We may want unbiased rounding. If necessary, adjust the mask.
            */
            if (!biased_rounding)
            {
                /* we're in unbiased rounding mode, so round to nearest even.
                ** We just clear the least significant bit of the result
                ** (as well as the lower bits) if all the lower bits of the
                ** addition are zero. That means that the original fractional
                ** value was exactly half-way between two consecutive rounded
                ** values.
                */

#if UACCUM_FBIT == ULFRACT_FBIT
                /* the low (fractional) part of a_ui */
                uint_ulr_t a_ui_lo = (uint_ulr_t)a_ui;
#else
                /* just a copy of a_ui */
                uint_uk_t a_ui_lo = a_ui;
#endif

                if ((mask & a_ui_lo) == 0U)
                {
                    mask <<= 1U;
                    mask |= 1U;
                }
            }

            /* invert the mask to get the bits to keep */
            mask_long = ~ ((uint_uk_t)mask);
  
            /* and round off the bits we don't want */
            a_ui &= mask_long;
  
            /* convert back from an integer bit pattern to a fract.
            */
            result = ukbits(a_ui);

        }
    }
    return result;
}

/* End of file */

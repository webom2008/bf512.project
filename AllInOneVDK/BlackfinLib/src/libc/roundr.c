/**************************************************************************
 *
 * roundr.c : $Revision: 1.6.4.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: roundr - round a fract value to a given precision

    Synopsis:

        fract roundr (fract parm, int num_fract_bits);

    Description:
 
        This function rounds a given fract parameter
        to the number of fractional bits given by the second parameter. 
        The rounding is performed to-nearest. In the case where the value
        is exactly half-way between two values of the destination precision,
        the value is either rounded up (biased rounding), or rounded to
        nearest even (unbiased rounding), depending on the current setting
        of the RND_MOD bit. The rounding may saturate, so that 0x7fffffff
        is a valid output.

    Error Conditions:

        If the input precision is greater than or equal to FRACT_FBIT, no
        rounding occurs. If the input precision is less than or equal to 0,
        rounding is to the nearest whole number.

    Algorithm:

        Add constant of 1 in the place after the intended LSB of the result.
        If the addition saturates, the saturated value is returned.
        Truncate off the bits after the intended LSB of the result.

    Implementation:

    Example:
    
        #include <stdfix.h>
        fract f;
        fract rndf;

        rndf = roundr(f, 12);
        

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=roundr")
#pragma file_attr("libFunc=_roundr")
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

#include "rounding.h"

#define __DISABLE_ROUNDR 1
#include <stdbool.h>
#include <stdfix.h>
#include <builtins.h>


extern fract 
roundr(fract _f,
       int _num_fract_bits) 
{
    const fract f = _f;
    const int num_fract_bits = _num_fract_bits;

    /* the result after rounding */
    fract result;

    if (expected_false((num_fract_bits >= FRACT_FBIT) ? (int)1 : (int)0))
    {

        /* no bits to round off.
        */
        result = f;

    }
    else
    {

        /* the fract bit-pattern interpreted as an unsigned integer */
        uint_ur_t f_ui = (uint_ur_t)bitsr(f);

        /* num_fract_bits, but negative values clipped to zero.
        ** If we asked for a precision less than zero, assume a precision of
        ** zero is fine, because we have no integer bits.
        */
        int clipped_num_fract_bits
            = (num_fract_bits < 0) ? 0 : num_fract_bits;

        /* the position of the bit after the last one we want to keep,
        ** relative to the LSB of the operand.
        */
        uint_ur_t most_significant_rounded_bit
            = ((uint_ur_t)FRACT_FBIT - 1U)
              - (uint_ur_t)clipped_num_fract_bits;

        /* the amount we have to add before truncation in order to get
        ** rounding.
        */
        uint_ur_t increment_i
            = (uint_ur_t)1U << most_significant_rounded_bit;

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
        f_ui += increment_i;

        if ((int_r_t)f_ui < bitsr(f))
        {
            /* If the addition above saturated, we don't want to AND off
            ** the bottom bits because the result should saturate, so just
            ** return the max possible value. If we're rounding off all
            ** the fractional bits and we're doing unbiased rounding, then
            ** 0.5 will round to 0.0 rather than 1.0.
            */
            result = (   ((int_r_t)f_ui != bitsr(FRACT_MIN))
                      || biased_rounding
                      || (clipped_num_fract_bits != 0))
                   ? FRACT_MAX : 0.0r;
        }
        else
        {
            /* truncating f_ui gives us the right answer for biased rounding,
            ** so we set up a mask to do that. Initially we compute the
            ** mask with ones in the places that we are throwing away, but we
            ** eventually invert it.
            */
            uint_ur_t mask = increment_i;

            mask <<= 1U;
            mask -= 1U;

            /* We may want unbiased rounding. If necessary adjust the mask.
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
                if ((mask & f_ui) == 0U)
                {
                    mask <<= 1U;
                    mask |= 1U;
                }
            }

            /* invert the mask to get the bits to keep */
            mask = ~ mask;
  
            /* and round off the bits we don't want */
            f_ui &= mask;
  
            /* convert back from an integer bit pattern to a fract.
            */
            result = rbits((int_r_t)f_ui);

        }
    }
    return result;
}

/* End of file */

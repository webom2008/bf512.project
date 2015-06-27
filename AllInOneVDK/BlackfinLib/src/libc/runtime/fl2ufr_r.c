/**************************************************************************
 *
 * fl2ufr_r.c : $Revision: 1.7.4.1 $
 * (c) Copyright 2008-2010 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: float_to_unsigned_fr16_rnd - convert 32-bit floating point
                                           value to unsigned fract value with
                                           rounding

    Synopsis:

        unsigned fract float_to_unsigned_fr16_rnd (float parm1);

    Description:
 
        This function convert a 32-bit float value to an unsigned
        fract, truncating any unneeded bits.
        If the value is out of range of the unsigned fract, a saturated
        value is returned.

    Error Conditions:

        If the input argument is a NaN, the output result will be saturated
        according to the sign of the sign bit.
        If the input argument is an infinity, the output result is
        saturated.

    Algorithm:

    Implementation:

        Extract mantissa and shift to produce a 0.16 result.
        If the floating-point number exceeds the range
        of the 0.16 value, then the result is saturated.

    Example:
    
        None.

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=libc")
#pragma file_attr("libFunc=float_to_unsigned_fr16_rnd")
#pragma file_attr("libFunc=__float_to_unsigned_fr16_rnd")
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

#pragma diag(suppress:misra_rule_18_4)
/* Suppress Rule 18.4 which says that unions shall not be used.
**
** Rationale: we need to extract the exponent and mantissa from the
** float - the easiest way to do this is to reinterpret the bit
** pattern as an int.
*/

#pragma diag(suppress:misra_rule_12_12)
#endif

#include "embc_rt.h"
#include "rounding_blackfin.h"

extern unsigned fract 
float_to_unsigned_fr16_rnd(float parm1)
{
    /* the bits of the 0.16 result. */
    uint16_t result;

    /* what we must add to the result to round */
    unsigned fract round_amount = 0.0ur;

    /* the exponent of the floating-point number. */
    uint32_t exponent;

    /* a union to allow conversion of the floating-point value to a
    ** bit-pattern.
    */
    union {
        float fl;
        uint32_t i;
    } xx;

    /* extract the exponent. */
    xx.fl = parm1;
    exponent = (xx.i >> 23U);
    exponent &= 255U;

    if ((int32_t)xx.i < 0)
    {
        result = 0U;
    }
    else if (exponent >= 127U)
    {
        result = bitsur(UFRACT_MAX);
    }
    else if (exponent > 109U)
    {
        /* extract the mantissa. */
        uint64_t full_result;
        uint32_t umantissa = (xx.i << 9U);
        int32_t adj_exponent = ((int32_t)exponent - 127) + 8;
        full_result = (uint64_t)umantissa;
        full_result |= 0x100000000ULL;

        /* shift the mantissa into the two parts of the 24.40 result */
        full_result = __builtin_lshiftll(full_result, (int64_t)adj_exponent);

        result = (uint16_t)__builtin_lshiftll(full_result, -24);

        /* now do the rounding.
        */
        {
            uint32_t threshold = 0x800000U;
            uint32_t lo = (uint32_t)full_result;
            lo &= 0xffffffU;

            if (!BIASED_ROUNDING)
            {
                if (((uint64_t)result & 1ULL) == 0ULL)
                {
                    threshold++;
                }
            }
            if (lo >= threshold)
            {
                round_amount = urbits(1U);
            }
        }
    }
    else
    {
        result = 0U;
    }

    return urbits(result) + round_amount;
}

/* End of file */

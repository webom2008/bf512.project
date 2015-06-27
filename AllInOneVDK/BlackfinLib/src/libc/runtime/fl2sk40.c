/**************************************************************************
 *
 * fl2sk40.c : $Revision: 1.7.4.1 $
 * (c) Copyright 2008-2010 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: float_to_accum - convert 32-bit floating point value to
                               signed accum value with truncation

    Synopsis:

        accum float_to_accum (float parm1);

    Description:
 
        This function convert a 32-bit float value to a signed
        accum, truncating any unneeded bits.
        If the value is out of range of the signed accum, a saturated
        value is returned.

    Error Conditions:

        If the input argument is a NaN, the output result will be saturated
        according to the sign of the sign bit.
        If the input argument is an infinity, the output result is
        saturated.

    Algorithm:

    Implementation:

        Extract mantissa and shift to produce an s8.31 result.
        Negation occurs after the shifting so that truncation is towards
        zero rather than -infinity.
        If the floating-point number exceeds the range
        of the s8.31 value, then the result is saturated.

    Example:
    
        None.

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=libc")
#pragma file_attr("libFunc=float_to_accum")
#pragma file_attr("libFunc=__float_to_accum")
#pragma file_attr("libName=libc")
#pragma file_attr("prefersMem=internal")
#pragma file_attr("prefersMemNum=30")

#endif

#ifdef _MISRA_RULES
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

extern accum 
float_to_accum(float parm1)
{
    /* the bits of the s8.31 result. */
    int64_t result;

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

    if (exponent >= 135U)
    {
        /* note that accum_min comes through this path. 
        */
        if ((int32_t)xx.i < 0)
        {
            result = bitsk(ACCUM_MIN);
        }
        else
        {
            result = bitsk(ACCUM_MAX);
        }
    }
    else if (exponent > 95U)
    {
        /* extract the mantissa. */
        uint64_t uresult;
        uint32_t umantissa = (xx.i << 9U);
        int32_t adj_exponent = (int32_t)exponent - 128;
        uresult = (uint64_t)umantissa;
        uresult |= 0x100000000ULL;

        /* shift the mantissa into the two parts of the s31.32 result */
        uresult = __builtin_lshiftll(uresult, (int64_t)adj_exponent);
        result = (int64_t)uresult;

        /* negate the result if fp value is negative */
        if ((int32_t)xx.i < 0)
        {
            result = -result;
        }
    }
    else
    {
        result = 0LL;
    }

    return kbits(result);
}

/* End of file */

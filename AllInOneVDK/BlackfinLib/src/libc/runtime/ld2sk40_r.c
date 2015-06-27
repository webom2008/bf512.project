/**************************************************************************
 *
 * ld2sk40_r.c : $Revision: 1.7.4.1 $
 * (c) Copyright 2008-2010 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: float64_to_accum_rnd - convert 64-bit floating point value to
                                     signed accum value with rounding 

    Synopsis:

        accum float64_to_accum_rnd (long double parm1);

    Description:
 
        This function convert a 64-bit long double value to a signed
        accum, rounding the value to nearest using
        either biased or unbiased rounding as per the RND_MOD bit.
        If the value is out of range of the signed accum, a saturated
        value is returned.

    Error Conditions:

        If the input argument is a NaN, the output result will be saturated
        according to the sign of the sign bit.
        If the input argument is an infinity, the output result is
        saturated.

    Algorithm:

    Implementation:

        Extract mantissa and shift to produce an s32.95 result. We round
        off the bottom 64 bits to produce an s32.31 result which is then
        saturated to accum. All 52 bits of the mantissa are held in 
        the s32.95 result unless the floating point number underflows
        the range of the accum, so no precision is lost during the
        rounding process. If the floating-point number exceeds the range
        of the s32.95 value, then this value is saturated.

    Example:
    
        None.

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=libc")
#pragma file_attr("libFunc=float64_to_accum_rnd")
#pragma file_attr("libFunc=__float64_to_accum_rnd")
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
** long double - the easiest way to do this is to reinterpret the bit
** pattern as an int.
*/

#pragma diag(suppress:misra_rule_12_12)
#endif

#include "embc_rt.h"
#include "rounding_blackfin.h"

extern accum 
float64_to_accum_rnd(long double parm1)
{
    /* the bottom 64 bits of the s32.95 intermediate result. */
    uint64_t bits_0_63;

    /* the top 64 bits of the s32.95 intermediate result. */
    int64_t bits_64_127;

    /* the exponent of the floating-point number. */
    uint32_t exponent;

    /* a temporary variable used during extraction of the exponent */
    uint64_t exponent_part;

    /* a union to allow conversion of the floating-point value to a
    ** bit-pattern.
    */
    union {
        long double dbl;
        uint64_t ll;
    } xx;

    /* extract the exponent. */
    xx.dbl = parm1;
    exponent_part = (xx.ll >> 52ULL);
    exponent = (uint32_t)exponent_part & 2047U;

    if (exponent >= 1031U)
    {
        /* note that accum_min comes through this path.
        */
        if ((int64_t)xx.ll < 0LL)
        {
            bits_64_127 = bitsk(ACCUM_MIN);
        }
        else
        {
            bits_64_127 = bitsk(ACCUM_MAX);
        }
    }
    else if (exponent > 990U)
    {
        /* extract the mantissa. */
        int64_t mantissa;
        uint64_t umantissa = (xx.ll << 12U);
        int32_t adj_exponent = (int32_t)exponent - 990;
        umantissa >>= 2U;
        umantissa |= 0x4000000000000000ULL;
        mantissa = (int64_t)umantissa;

        /* negate the mantissa if fp value is negative */
        bits_64_127 = 0LL;
        if ((int64_t)xx.ll < 0LL)
        {
            mantissa = -mantissa;
            bits_64_127 = -1LL;
        }

        /* shift the mantissa into the two parts of the s31.95 result */
        bits_0_63 = (uint64_t)__builtin_ashiftll(mantissa,
                                                 (int64_t)adj_exponent);
        if (exponent > 991U)
        {
            adj_exponent = (int32_t)exponent - 1054;
            bits_64_127 = __builtin_ashiftll(mantissa, (int64_t)adj_exponent);
        }

        /* now do the rounding.
        */
        {
            uint64_t threshold = 0x8000000000000000ULL;

            if (!BIASED_ROUNDING)
            {
                uint64_t top = (uint64_t)bits_64_127;
                if ((top & 1ULL) == 0ULL)
                {
                    threshold++;
                }
            }
            if (bits_0_63 >= threshold)
            {
                bits_64_127++;
            }
        } 
    }
    else
    {
        bits_64_127 = 0LL;
    }

    return kbits(bits_64_127);
}

/* End of file */

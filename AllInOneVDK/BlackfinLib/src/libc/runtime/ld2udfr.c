/**************************************************************************
 *
 * ld2udfr.c : $Revision: 1.7.4.1 $
 * (c) Copyright 2008-2010 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: float64_to_unsigned_fr32 - convert 64-bit floating point value
                                         to unsigned long fract value 

    Synopsis:

        unsigned long fract float64_to_unsigned_fr32 (long double parm1);

    Description:
 
        This function convert a 64-bit long double value to an unsigned
        long fract, truncating any unneeded bits.
        If the value is out of range of the unsigned long fract, a saturated
        value is returned.

    Error Conditions:

        If the input argument is a NaN, the output result will be saturated
        according to the sign of the sign bit.
        If the input argument is an infinity, the output result is
        saturated.

    Algorithm:

    Implementation:

        Extract mantissa and shift to produce a 0.32 result.
        If the floating-point number exceeds the range
        of the 0.32 value, then the result is saturated.

    Example:
    
        None.

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=libc")
#pragma file_attr("libFunc=float64_to_unsigned_fr32")
#pragma file_attr("libFunc=__float64_to_unsigned_fr32")
#pragma file_attr("libName=libc")
#pragma file_attr("prefersMem=internal")
#pragma file_attr("prefersMemNum=30")

#endif

#ifdef _MISRA_RULES
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

extern unsigned long fract 
float64_to_unsigned_fr32(long double parm1)
{
    /* the bit pattern of the result */
    uint32_t result;

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

    if ((int64_t)xx.ll < 0LL)
    {
        result = 0U;
    }
    else if (exponent >= 1023U)
    {
        result = bitsulr(ULFRACT_MAX);
    }
    else if (exponent > 990U)
    {
        /* extract the most significant part of the mantissa. */
        uint64_t umantissa = (xx.ll << 12U);
        uint32_t umantissa_significant = (uint32_t)(umantissa >> 32U);
        uint32_t adj_exponent = 1022U - exponent;
        umantissa_significant >>= 1U;
        umantissa_significant |= 0x80000000U;

        umantissa_significant >>= adj_exponent; 
        result = umantissa_significant;
    }
    else
    {
        /* underflow */
        result = 0U;
    }

    return ulrbits(result);
}

/* End of file */

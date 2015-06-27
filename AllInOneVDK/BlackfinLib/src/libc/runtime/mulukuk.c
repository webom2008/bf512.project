/**************************************************************************
 *
 * mulukuk.c : $Revision: 1.6.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: mulukuk - multiply two unsigned accums to give an
                        unsigned accum result

    Synopsis:

        unsigned accum mulukuk (unsigned accum parm1, unsigned accum parm2);

    Description:
 
        This function multiplies two 8.32 unsigned accum arguments together
        to produce a 8.32 unsigned accum result. If the multiplication
        overflows the result is saturated at the 40-bit boundary.

    Error Conditions:

        None, all inputs valid.

    Algorithm:

        Operands converted to int64_t.
        Perform 48 x 48 => 96 bit multiplication. Truncate off the bottom
        32 bits. Unsigned saturate the result to the 40-bit limit.

    Implementation:

        Use function to perform widening 48-bit multiplication, and
        OR appropriate parts of the 96-bit result into the correct
        place in the output. This gives us a result in 32.32 format,
        which is saturated to 40-bits using ukbits.

    Example:
    
        None.

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=libc")
#pragma file_attr("libFunc=mulukuk")
#pragma file_attr("libFunc=__mulukuk")
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
#endif

#include "embc_rt.h"
#include "rounding_blackfin.h"

extern unsigned accum 
mulukuk(unsigned accum a,
        unsigned accum b,
        bool rnd_to_nearest) 
{
    /* the following three variables hold the 96 bits of the
    ** 48 x 48 -> 96 bit multiply.
    ** The multiply is signed, so the MSB part is a signed integer.
    */
    uint32_t bits_0_31;
    uint32_t bits_32_63;
    int32_t bits_64_95;

    /* the following will hold the relevant part of the multiply result,
    ** which we hold in an unsigned 64-bit integer,
    */
    uint64_t result;

    /* the following holds the bit-pattern of A zero-extended to
    ** a 64-bit container.
    */
    uint64_t ll_a = (uint64_t)bitsuk(a);

    /* the following holds the bit-pattern of B zero-extended to
    ** a 64-bit container.
    */
    uint64_t ll_b = (uint64_t)bitsuk(b);

    /* since we zero extended to 48-bits we are permitted to use a signed
    ** widening multiply, as this will give the same answer.
    */
    mul48x48_96((int64_t)ll_a, (int64_t)ll_b, &bits_0_31, &bits_32_63,
                &bits_64_95);

    /* the part of the 96-bit result that we want comes from discarding 32
    ** fractional bits. So that's bits 32 to 95 inclusive.
    */
    result = (uint64_t)bits_32_63;
    result |= (((uint64_t)bits_64_95) << 32);

    if (rnd_to_nearest)
    {
        uint32_t rnd_threshold = 0x80000000U;

        if (BIASED_ROUNDING || ((bits_32_63 & 1U) != 0U))
        {
            rnd_threshold -= 1U;
        }

        if (bits_0_31 > rnd_threshold)
        {
            result += 1U;
        }
    }

    /* saturate the result to 40 bits.
    */
    return ukbits(result);
}

/* End of file */

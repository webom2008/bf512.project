/**************************************************************************
 *
 * mulkk.c : $Revision: 1.6.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: mulkk - multiply two accums to give an accum result

    Synopsis:

        accum mulkk (accum parm1, accum parm2);

    Description:
 
        This function multiplies two 8.31 accum arguments together
        to produce a 8.31 accum result. If the multiplication overflows
        the result is saturated at the 40-bit boundary.

    Error Conditions:

        None, all inputs valid.

    Algorithm:

        Operands converted to long long. 
        Perform 48 x 48 => 96 bit multiplication. Truncate off the bottom
        31 bits. Saturate the result to the 40-bit limit.

    Implementation:

        Use function to perform widening 48-bit multiplication, and
        OR appropriate parts of the 96-bit result into the correct
        place in the output. This gives us a result in 32.31 format,
        which is saturated to 40-bits using kbits.

    Example:
    
        None.

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=libc")
#pragma file_attr("libFunc=mulkk")
#pragma file_attr("libFunc=__mulkk")
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

extern accum 
mulkk(accum a,
      accum b,
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
    ** which we hold in an unsigned 64-bit integer so that we can perform
    ** bitwise operations on it.
    */
    uint64_t uresult;

    /* the following holds the bit-pattern of A sign-extended to
    ** a 64-bit container.
    */
    int64_t ll_a = (int64_t)bitsk(a);

    /* the following holds the bit-pattern of B sign-extended to
    ** a 64-bit container.
    */
    int64_t ll_b = (int64_t)bitsk(b);

    /* do the 48 x 48 -> 96 bit multiply. */
    mul48x48_96(ll_a, ll_b, &bits_0_31, &bits_32_63, &bits_64_95);

    /* the part of the 96-bit result that we want comes from discarding 31
    ** fractional bits. So that's bits 31 to 94 inclusive.
    */
    uresult = ((uint64_t)bits_0_31 >> 31);
    uresult |= ((uint64_t)bits_32_63 << 1);
    uresult |= (((uint64_t)bits_64_95) << 33);

    if (rnd_to_nearest)
    {
        uint32_t rnd_threshold = 0x40000000U;

        if (BIASED_ROUNDING || ((bits_0_31 >> 31) != 0U))
        {
            rnd_threshold -= 1U;
        }

        if ((bits_0_31 & 0x7FFFFFFFU) > rnd_threshold)
        {
            uresult += 1U;
        }
    }

    /* saturate the result to 40 bits.
    */
    return kbits((int64_t)uresult);
}

/* End of file */

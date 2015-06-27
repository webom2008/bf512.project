/**************************************************************************
 *
 * mului48uk.c : $Revision: 1.5.24.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: mului48uk - multiply an unsigned 48-bit value by an unsigned
                          accum to give an unsigned accum result

    Synopsis:

        unsigned accum mului48uk (uint64_t parm1, unsigned accum parm2);

    Description:
 
        This function multiplies a 48-bit unsigned integer argument by an 8.32
        unsigned accum argument to produce an 8.32 unsigned accum result.
        If the multiplication overflows the result is saturated at the
        40-bit boundary.

    Error Conditions:

        None, all inputs valid, although the top 16 bits of the uint64_t
        argument are ignored (they are assumed to be zero-extension bits).

    Algorithm:

        Unsigned accum operands converted to uint64_t. 
        Perform 48 x 48 => 96 bit multiplication. 
        Unsigned saturate the result to the 40-bit limit.

    Implementation:

        Use function to perform widening 48-bit multiplication. The
        bottom 40 bits of the 96-bit result are what we require, 
        but if the top 56-bits of the result are not merely zero-extension
        bits then we saturate. We first saturate to 64 bits, and then use
        ukbits to saturate to 40 bits.

    Example:
    
        None.

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=libc")
#pragma file_attr("libFunc=mului48uk")
#pragma file_attr("libFunc=__mului48uk")
#pragma file_attr("libName=libc")
#pragma file_attr("prefersMem=internal")
#pragma file_attr("prefersMemNum=30")

#endif

#include "embc_rt.h"
#include <limits.h>

extern unsigned accum 
mului48uk(uint64_t a,
          unsigned accum b) 
{
    /* the following three variables hold the 96 bits of the
    ** 48 x 48 -> 96 bit multiply.
    ** The multiply is signed, so the MSB part is a signed integer.
    */
    uint32_t bits_0_31;
    uint32_t bits_32_63;
    int32_t bits_64_95;

    /* the following will hold the relevant part of the multiply result,
    ** which we hold in an unsigned 64-bit integer.
    */
    uint64_t uresult;

    /* the following holds the bit-pattern of B zero-extended to
    ** a 64-bit container.
    */
    uint64_t ll_b = (uint64_t)bitsuk(b);

    /* we can use signed multiplication because top bits are zero-extension
    ** bits.
    */
    mul48x48_96((int64_t)a, (int64_t)ll_b, &bits_0_31, &bits_32_63,
                &bits_64_95);

    if ((uint64_t)bits_64_95 > 0U)
    {
        /* we overflow and are positive, so saturate to the maximum possible
        ** value. There's no need to set bits_0_31 because setting bits_32_63
        ** in this way means we're guaranteed to saturate when we convert
        ** back to the accum.
        */
        bits_32_63 = UINT_MAX;
    }

    /* the part of the 96-bit result that we want comes from the bottom of
    ** the 96-bit value. So that's bits 0 to 63 inclusive.
    */
    uresult = (uint64_t)bits_0_31;
    uresult |= ((uint64_t)bits_32_63 << 32);
    return ukbits(uresult);
}

/* End of file */

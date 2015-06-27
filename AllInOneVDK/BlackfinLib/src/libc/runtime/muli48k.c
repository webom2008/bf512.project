/**************************************************************************
 *
 * muli48k.c : $Revision: 1.5.24.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: muli48k - multiply a 48-bit integer value by an accum to give
                        an accum result

    Synopsis:

        accum muli48k (int64_t parm1, accum parm2);

    Description:
 
        This function multiplies a 48-bit integer argument by an 8.31
        accum argument to produce an 8.31 accum result. If the
        multiplication overflows the result is saturated at the
        40-bit boundary.

    Error Conditions:

        None, all inputs valid, although the top 16 bits of the int64_t
        argument are ignored (they are assumed to be sign-extension bits).

    Algorithm:

        accum operands converted to int64_t. 
        Perform 48 x 48 => 96 bit multiplication. 
        Saturate the result to the 40-bit limit.

    Implementation:

        Use function to perform widening 48-bit multiplication. The
        bottom 40 bits of the 96-bit result are what we require, 
        but if the top 56-bits of the result are not merely sign-extension
        bits then we saturate. We first saturate to 64 bits, and then use
        kbits to saturate to 40 bits.

    Example:
    
        None.

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=libc")
#pragma file_attr("libFunc=muli48k")
#pragma file_attr("libFunc=__muli48k")
#pragma file_attr("libName=libc")
#pragma file_attr("prefersMem=internal")
#pragma file_attr("prefersMemNum=30")

#endif

#include "embc_rt.h"
#include <limits.h>

extern accum 
muli48k(int64_t a,
        accum b) 
{
    /* the following three variables hold the 96 bits of the
    ** 48 x 48 -> 96 bit multiply.
    ** The multiply is signed, so the MSB part is a signed integer.
    */
    uint32_t bits_0_31;
    uint32_t bits_32_63;
    int32_t bits_64_95;

    /* bits 32 to 63 interpreted as a signed integer */
    int32_t bits_32_63_as_signed;

    /* the following will hold the relevant part of the multiply result,
    ** which we hold in an unsigned 64-bit integer so that we can perform
    ** bitwise operations on it.
    */
    uint64_t uresult;

    /* the following holds the bit-pattern of B sign-extended to
    ** a 64-bit container.
    */
    int64_t ll_b = (int64_t)bitsk(b);

    /* do the 48 x 48 -> 96 bit multiply. */
    mul48x48_96(a, ll_b, &bits_0_31, &bits_32_63, &bits_64_95);

    bits_32_63_as_signed = (int32_t)bits_32_63;

    if (   (bits_64_95 < 0)
        && ((bits_64_95 != -1) || (bits_32_63_as_signed >= 0)))
    {
        /* we overflow and are negative, so saturate to the minimum possible
        ** value. There's no need to set bits_0_31 because setting bits_32_63
        ** in this way means we're guaranteed to saturate when we convert
        ** back to the accum.
        */
        bits_32_63_as_signed = INT_MIN;
    }
    else if (   (bits_64_95 >= 0)
             && ((bits_64_95 != 0) || (bits_32_63_as_signed < 0)))
    {
        /* we overflow and are positive, so saturate to the maximum possible
        ** value. There's no need to set bits_0_31 because setting bits_32_63
        ** in this way means we're guaranteed to saturate when we convert
        ** back to the accum.
        */
        bits_32_63_as_signed = INT_MAX;
    }
    else
    {
        /* unsaturated */
    }

    /* the part of the 96-bit result that we want comes from the bottom of
    ** the 96-bit value. So that's bits 0 to 63 inclusive.
    */
    uresult = (uint64_t)bits_0_31;
    uresult |= ((uint64_t)bits_32_63_as_signed << 32);

    return kbits((int64_t)uresult);
}

/* End of file */

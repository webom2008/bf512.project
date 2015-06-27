/**************************************************************************
 *
 * mulik_bf.c : $Revision: 1.6.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: mulik - multiply a 32-bit integer value by an  accum to give
                      an integer result

    Synopsis:

        int mulik (int parm1, accum parm2);

    Description:
 
        This function multiplies a 32-bit integer argument by an 8.31
        accum argument to produce a 32-bit integer result. The result
        is rounded towards zero.

    Error Conditions:

        None, all inputs valid.
        If the result overflows the integer size, the value is truncated
        to int by discarding the higher order bits.

    Algorithm:

        accum operand converted to int64_t. 
        int sign-extend to long long.
        Perform 48 x 48 => 96 bit multiplication (which conserves codesize,
        because we already have this function). 
        Remove 31 bits of the result by rounding towards zero.

    Implementation:

        Use function to perform widening 48-bit multiplication.
        We then truncate 31 bits of the result. If the 96-bit multiply
        result was negative and any of the fractional bits were non-zero,
        then this will round down when we should round towards zero. So
        detect that case after the event and add one to the integer
        result if we rounded the wrong way.

    Example:
    
        #include <stdfix.h>
        int parm1;
        accum parm2;
        int result;

        result = mulik (parm1,parm2);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=mulik")
#pragma file_attr("libFunc=_mulik")
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
#endif

#define __DISABLE_MULIK 1

#include <stdint.h>
#include <stdfix.h>
#include "embc_rt.h"

extern int 
mulik(int _a,
      accum _b) 
{
    const int a = _a;
    const accum b = _b;

    /* the following three variables hold the 96 bits of the
    ** 48 x 48 -> 96 bit multiply.
    ** The multiply is signed, so the MSB part is a signed integer.
    */ 
    uint32_t bits_0_31;
    uint32_t bits_32_63;
    int32_t bits_64_95;

    /* the following holds the bit-pattern of B sign-extended to
    ** a 64-bit container.
    */
    int64_t ll_b = (int64_t)bitsk(b);

    /* the following will hold the relevant part of the multiply result,
    ** which we hold in an unsigned 32-bit integer so that we can perform
    ** bitwise operations on it.
    */
    uint32_t uresult;

    /* do the 48 x 48 -> 96 bit multiply. */
    mul48x48_96((int64_t)a, ll_b, &bits_0_31, &bits_32_63, &bits_64_95);

    /* the part of the 96-bit result that we want comes from discarding 31
    ** fractional bits, along with any bits of overflow. So that's bits
    ** 31 to 62 inclusive.
    */
    uresult = (bits_0_31 >> 31) & 1U;
    uresult |= (bits_32_63 << 1);

    /* perform the rounding. If the 96-bit result is positive, we've truncated
    ** correctly, but if it's negative and any of the fractional bits are
    ** non-zero then we've rounded down when we should have rounded up.
    ** Detect this case and add one to the result.
    */
    if (bits_64_95 < 0)
    {
        uint32_t truncated_bits = bits_0_31 << 1;
        if (truncated_bits != 0U)
        {
            uresult += 1U;
        }
    }

    /* return the result interpreted as a signed integer */
    return (int)uresult;
}

/* End of file */

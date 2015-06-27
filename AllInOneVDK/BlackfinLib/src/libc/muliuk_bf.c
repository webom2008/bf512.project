/**************************************************************************
 *
 * muliuk_bf.c : $Revision: 1.6.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: muliuk - multiply a 32-bit unsigned integer value by an 
                       unsigned accum to give an unsigned integer result

    Synopsis:

        unsigned int muliuk (unsigned int parm1, unsigned accum parm2);

    Description:
 
        This function multiplies a 32-bit unsigned integer argument by an 8.32
        unsigned accum argument to produce a 32-bit unsigned integer result.
        The result is rounded towards zero.

    Error Conditions:

        None, all inputs valid.
        If the result overflows the integer size, the value is truncated
        to unsigned int by discarding the higher order bits.

    Algorithm:

        Unsigned accum operand zero-extended to int64_t. 
        Unisnged int zero-extend to long long.
        Perform 48 x 48 => 96 bit multiplication (which conserves codesize,
        because we already have this function). 
        Discard lowest 32 bits of the result.

    Implementation:

        Use function to perform widening 48-bit multiplication.
        We then discard 32 bits of the result, which has the effect of
        rounding towards zero.

    Example:
    
        #include <stdfix.h>
        unsigned int parm1;
        unsigned accum parm2;
        unsigned int result;

        result = muliuk (parm1,parm2);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=muliuk")
#pragma file_attr("libFunc=_muliuk")
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
#endif /* _MISRA_RULES */

#define __DISABLE_MULIUK 1

#include <stdint.h>
#include <stdfix.h>
#include "embc_rt.h"

extern unsigned int 
muliuk(unsigned int _a,
       unsigned accum _b) 
{
    const unsigned int a = _a;
    const unsigned accum b = _b;

    /* the following three variables hold the 96 bits of the
    ** 48 x 48 -> 96 bit multiply.
    ** The multiply is signed, so the MSB part is a signed integer.
    */
    uint32_t bits_0_31;
    uint32_t bits_32_63;
    int32_t bits_64_95;

    /* the following holds the bit-pattern of B zero-extended to
    ** a 64-bit container, which we then interpret as signed.
    */
    int64_t ll_b = (int64_t)bitsuk(b);

    /* we may use a signed 48 x 48 -> 96 bit multiply since we zero-extended
    ** the operands.
    */
    mul48x48_96((int64_t)a, ll_b, &bits_0_31, &bits_32_63, &bits_64_95);

    /* the part of the 96-bit result that we want comes from discarding 32
    ** fractional bits, along with any bits of overflow. So that's bits
    ** 32 to 63 inclusive.
    */
    return (unsigned int)bits_32_63;
}

/* End of file */

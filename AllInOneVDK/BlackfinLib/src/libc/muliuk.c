/**************************************************************************
 *
 * muliuk.c : $Revision: 1.6.4.1 $
 * (c) Copyright 2008-2010 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: muliuk - multiply an unsigned integer value by an unsigned
                       accum to give an unsigned integer result

    Synopsis:

        unsigned int muliuk (unsigned int parm1, unsigned accum parm2);

    Description:
 
        This function multiplies an unsigned integer argument by an
        unsigned accum argument to produce an unsigned integer result.
        The result is rounded towards zero.

    Error Conditions:

        None, all inputs valid.

    Algorithm:

        Multiply the two bit patterns together to give exact result of 
        greater bitsize. Discard the bits representing fractional
        parts from the result.

    Implementation:

        To cope with different possible sizes of int and fixed-point types,
        perform multiplication to produce an array of values each of the 
        same size as the result type. Arrange so that the final result is
        held in the top-most element and all other parts represent fractional
        bits.

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
#pragma diag(suppress:misra_rule_12_1)
#endif

#ifdef __FX_NO_ACCUM
#error This file requires _Accum types
#endif

#define __DISABLE_MULIUK 1

#include <stdint.h>
#include <stdfix.h>
#include <limits.h>

/* How many chunks of the same size as A are required to hold the
** product of A and B? Take into account the fact that we shift
** B up in order to align the result in the product array.
** The computation is:
**   B's fract bits rounded up to integral number of A's
**     plus 1 for A itself
**     plus B's integral bits rounded up to integral number of A's
*/
#define NUM_CHUNKS \
  ((((unsigned long)UACCUM_FBIT \
    +(sizeof(unsigned int)*(unsigned long)CHAR_BIT)-1U) \
   / (sizeof(unsigned int)*(unsigned long)CHAR_BIT)) \
  + 1U \
  + (((unsigned long)UACCUM_IBIT \
     +(sizeof(unsigned int)*(unsigned long)CHAR_BIT)-1U) \
    / (sizeof(unsigned int)*(unsigned long)CHAR_BIT)))

#if (ULLONG_MAX / UINT_MAX) < UINT_MAX
  #error Assumes long long is twice size of int
#endif

extern unsigned int 
muliuk(unsigned int _a,
       unsigned accum _b) 
{
    const unsigned int a = _a;
    const unsigned accum b = _b;

    /* the accum b re-interpreted as a bit-pattern */
    uint_uk_t bi = bitsuk(b);

    /* the bitsize of a */
    unsigned long bitsize_a = sizeof(a) * (unsigned long)CHAR_BIT;

    /* array to hold all the components of the full multiplication */
    unsigned int prod[NUM_CHUNKS];

    /* iterator */
    unsigned int i;

    /* the number of bits we have to shift the accum up by to align the
    ** product so that the final result lives in the top array element.
    */
    unsigned long shift = (bitsize_a
                           - ((unsigned long)UACCUM_FBIT % bitsize_a))
                        % bitsize_a;

    /* zero the product array */
    for (i = 0U; i < NUM_CHUNKS; i++) {
      prod[i] = 0U;
    }

    /* compute the full product */
    for (i = 0U; i < (NUM_CHUNKS - 1U); i++) {
        unsigned int bi_lo = (unsigned int)bi;
        unsigned long long product;
        bi_lo <<= shift;
        product = (unsigned long long)a * bi_lo;
        prod[i] += (unsigned int)product;
        prod[i+1U] += (unsigned int)(product >> bitsize_a);
        shift = bitsize_a - shift;
        bi >>= (uint_uk_t)shift;
        shift = 0U;
    }
 
    /* the result is contained after the fractional part of the product. */
    return prod[((unsigned long)UACCUM_FBIT + bitsize_a - 1U) / bitsize_a];
}

/* End of file */

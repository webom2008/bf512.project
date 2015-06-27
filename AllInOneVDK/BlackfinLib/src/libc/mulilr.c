/**************************************************************************
 *
 * mulilr.c : $Revision: 1.5.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: mulilr - multiply a long integer value by a
                       long fract to give a long integer result

    Synopsis:

        long int mulilr (long int parm1, long fract parm2);

    Description:
 
        This function multiplies a long integer argument by a
        long fract argument to produce a long integer result.
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
        bits. The product of the absolute values is computed and the 
        fractional bits discarded before possibly negating the result,
        in order that the rounding is towards zero.

    Example:
    
        #include <stdfix.h>
        long int parm1;
        long fract parm2;
        long int result;

        result = mulilr (parm1,parm2);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=mulilr")
#pragma file_attr("libFunc=_mulilr")
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

#define __DISABLE_MULILR 1

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
  ((((unsigned long)LFRACT_FBIT \
    +(sizeof(long int)*(unsigned long)CHAR_BIT)-1U) \
   / (sizeof(long int)*(unsigned long)CHAR_BIT)) \
  + 1U \
  + 1U)
  

#if (ULLONG_MAX / UINT_MAX) < UINT_MAX
  #error Assumes long long is twice size of long int
#endif

extern long int 
mulilr(long int _a,
       long fract _b) 
{
    const long int a = _a;
    const long fract b = _b;

    /* the result */
    long int result;

    /* the absolute value of a */
    long int abs_a = (a < 0) ? -a : a;
    unsigned long int uabs_a = (unsigned long int)abs_a;

    /* the long fract b re-interpreted as a bit-pattern */
    int_lr_t bi = bitslr(b);

    /* the absolute value of the long fract bit-pattern */
    int_lr_t abs_bi = (bi < 0) ? -bi : bi;
    uint_ulr_t uabs_bi = (uint_ulr_t)abs_bi;

    /* the bitsize of a */
    unsigned long bitsize_a = sizeof(a) * (unsigned long)CHAR_BIT;

    /* array to hold all the components of the full multiplication */
    unsigned long int prod[NUM_CHUNKS];

    /* iterator */
    unsigned int i;

    /* the number of bits we have to shift the long fract up by to align the
    ** product so that the final result lives in the top array element.
    */
    unsigned long shift = (bitsize_a
                           - ((unsigned long)LFRACT_FBIT % bitsize_a))
                        % bitsize_a;

    /* zero the product array */
    for (i = 0U; i < NUM_CHUNKS; i++) {
      prod[i] = 0U;
    }

    /* compute the full product of the absolute values */
    for (i = 0U; i < (NUM_CHUNKS - 1U); i++) {
        unsigned long int bi_lo = (unsigned long int)uabs_bi;
        unsigned long long product;
        bi_lo <<= shift;
        product = (unsigned long long)uabs_a * bi_lo;
        prod[i] += (unsigned long int)product;
        prod[i+1U] += (unsigned long int)(product >> bitsize_a);
        uabs_bi >>= (uint_ulr_t)(bitsize_a - shift);
        shift = 0U;
    }
 
    /* the result will be contained in the top part of the product,
    ** but must be negated if the two arguments had opposite signs
    */
    result = (long int)prod[((unsigned long)LFRACT_FBIT+bitsize_a-1U)
           / bitsize_a];
    if (a < 0)
    {
        result = -result;
    }
    if (b < 0)
    {
        result = -result;
    }
    return result;
}

/* End of file */

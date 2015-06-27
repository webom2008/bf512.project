/**************************************************************************
 *
 * mulir.c : $Revision: 1.5.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: mulir - multiply an integer value by a 
                      fract to give an integer result

    Synopsis:

        int mulir (int parm1, fract parm2);

    Description:
 
        This function multiplies an integer argument by a
        fract argument to produce an integer result.
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
        int parm1;
        fract parm2;
        int result;

        result = mulir (parm1,parm2);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=mulir")
#pragma file_attr("libFunc=_mulir")
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

#define __DISABLE_MULIR 1

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
  ((((unsigned long)FRACT_FBIT+(sizeof(int)*(unsigned long)CHAR_BIT)-1U) \
   / (sizeof(int)*(unsigned long)CHAR_BIT)) \
  + 1U \
  + 1U)
  

#if (ULLONG_MAX / UINT_MAX) < UINT_MAX
  #error Assumes long long is twice size of int
#endif

extern int 
mulir(int _a,
      fract _b) 
{
    const int a = _a;
    const fract b = _b;

    /* the result */
    int result;

    /* the absolute value of a */
    int abs_a = (a < 0) ? -a : a;
    unsigned int uabs_a = (unsigned int)abs_a;

    /* the fract b re-interpreted as a bit-pattern */
    int_r_t bi = bitsr(b);

    /* the absolute value of the fract bit-pattern */
    int_r_t abs_bi = (bi < 0) ? -bi : bi;
    uint_ur_t uabs_bi = (uint_ur_t)abs_bi;

    /* the bitsize of a */
    unsigned long bitsize_a = sizeof(a) * (unsigned long)CHAR_BIT;

    /* array to hold all the components of the full multiplication */
    unsigned int prod[NUM_CHUNKS];

    /* iterator */
    unsigned int i;

    /* the number of bits we have to shift the fract up by to align the
    ** product so that the final result lives in the top array element.
    */
    unsigned long shift = (bitsize_a
                           - ((unsigned long)FRACT_FBIT % bitsize_a))
                        % bitsize_a;

    /* zero the product array */
    for (i = 0U; i < NUM_CHUNKS; i++) {
      prod[i] = 0U;
    }

    /* compute the full product of the absolute values */
    for (i = 0U; i < (NUM_CHUNKS - 1U); i++) {
        unsigned int bi_lo = (unsigned int)uabs_bi;
        unsigned long long product;
        bi_lo <<= shift;
        product = (unsigned long long)uabs_a * bi_lo;
        prod[i] += (unsigned int)product;
        prod[i+1U] += (unsigned int)(product >> bitsize_a);
        uabs_bi >>= (uint_ur_t)(bitsize_a - shift);
        shift = 0U;
    }
 
    /* the result will be contained in the top part of the product,
    ** but must be negated if the two arguments had opposite signs
    */
    result = (int)prod[((unsigned long)FRACT_FBIT+bitsize_a-1U) / bitsize_a];
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

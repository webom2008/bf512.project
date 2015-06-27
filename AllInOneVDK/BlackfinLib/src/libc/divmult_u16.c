/**************************************************************************
 *
 * divmult_u16.c : $Revision: 1.1.12.1 $
 * (c) Copyright 2010 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: divmult_u16 - return multiplier and shift amount required
                            to divide an unsigned short integer by a divisor.

    Synopsis:

        int2x16 divmult_u16(unsigned short divisor);

    Description:

        This function computes the multiplier and log of the denominator,
        required to emit code that does division by multiplication of an
        unsigned short integer numerator.
        The code that knows how to use these results is in the compiler.

    Error Conditions:

        None, all inputs valid.

    Algorithm:

        The algorithm to perform division by multiplication is described
        fully in "Division of Invariant Integers by Multiplication",
        Granlund and Montgomery.

    Implementation:

    Example:
  
      None.

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=libc")
#pragma file_attr("libFunc=divmult_u16")
#pragma file_attr("libFunc=___divmult_u16")
#pragma file_attr("libName=libc")
#pragma file_attr("prefersMem=internal")
#pragma file_attr("prefersMemNum=30")

#endif

#include <stdint.h>

typedef struct __divmult_u16_s {
  uint16_t multiplier;
  int16_t log_denom;
} __divmult_u16_t;

#if defined(__ADSP21000__)
#pragma regs_clobbered "ALLScratch,LoopDepth0"
#elif defined(__ADSPBLACKFIN__)
#pragma regs_clobbered "R0-R3,P0-P2,LC0,LT0,LB0,CCset"
#endif
#ifdef __SYSCALL_INFO__
%syscall
%notes "Returns the multiplier and log of denominator required to divide " \
       "an unsigned short integer by multiplication"
#endif
#pragma const
#pragma linkage_name ___divmult_u16
__divmult_u16_t divmult_u16(uint16_t _denom);

__divmult_u16_t
divmult_u16(uint16_t _denom)
{
    const uint16_t denom = _denom;

    /* the result structure */
    __divmult_u16_t result;

    /* data used to compute the log of the denominator */
    /*                        0   1   2   3   4   5   6   7   8 */
    static int16_t lgt[] = { -1,  0,  1,  1,  2,  2,  2,  2,  3 };

    /* the log of the denominator */
    int16_t l = 0;

    /* the multiplier */
    uint32_t m;

    if (denom < 3U) {

        l = lgt[denom];

    } else {

        /* temporary value used in computing the log of the denominator */
        uint16_t tmp = denom - 1U;

        /* compute the ceil of the log of the denominator. Do this by dividing
        ** by 2^3 until we have a value whose log can be read from the table.
        */
        while (tmp >= (sizeof(lgt) / sizeof(*lgt))) {
          l += 3;
          tmp >>= 3U;
        }
        l += lgt[tmp];
        l += 1;
    }

    /* multiplier is 1 + floor((2^N * (2^l - d))/denom) */
    m = (uint32_t)1U << 16;
    m = m * (((uint32_t)1U << l) - (uint32_t)denom);
    m = m / (uint32_t)denom;
    m += 1U;

    /* return the structure containing multiplier and log(denom) */
    result.multiplier = (uint16_t)m;
    result.log_denom = l;
    return result;
}

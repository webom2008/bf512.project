/**************************************************************************
 *
 * divmult_u32.c : $Revision: 1.1.12.1 $
 * (c) Copyright 2010 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: divmult_u32 - return multiplier and shift amount required
                            to divide an unsigned integer by a divisor.

    Synopsis:

        int2x32 divmult_u32(unsigned int divisor);

    Description:

        This function computes the multiplier and log of the denominator,
        required to emit code that does division by multiplication of an
        unsigned integer numerator.
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
#pragma file_attr("libFunc=divmult_u32")
#pragma file_attr("libFunc=___divmult_u32")
#pragma file_attr("libName=libc")
#pragma file_attr("prefersMem=internal")
#pragma file_attr("prefersMemNum=30")

#endif

#include <stdint.h>

typedef struct __divmult_u32_s {
  uint32_t multiplier;
  int32_t log_denom;
} __divmult_u32_t;

#if defined(__ADSP21000__)
#pragma regs_clobbered "ALLScratch,LoopDepth0"
#elif defined(__ADSPBLACKFIN__)
#pragma regs_clobbered "R0-R3,P0-P2,LC0,LT0,LB0,CCset"
#endif
#ifdef __SYSCALL_INFO__
%syscall
%notes "Returns the multiplier and log of denominator required to divide " \
       "an unsigned integer by multiplication"
#endif
#pragma const
#pragma linkage_name ___divmult_u32
__divmult_u32_t divmult_u32(uint32_t _denom);

__divmult_u32_t
divmult_u32(uint32_t _denom)
{
    const uint32_t denom = _denom;

    /* the result structure */
    __divmult_u32_t result;

    /* data used to compute the log of the denominator */
    /*                        0   1   2   3   4   5   6   7   8 */
    static int32_t lgt[] = { -1,  0,  1,  1,  2,  2,  2,  2,  3 };

    /* the log of the denominator */
    int32_t l = 0;

    /* the multiplier */
    uint64_t m;

    if (denom < 3U) {

        l = lgt[denom];

    } else {

        /* temporary value used in computing the log of the denominator */
        uint32_t tmp = denom - 1U;

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
    m = 1ULL << 32;
    m = m * ((1ULL << l) - (uint64_t)denom);
    m = m / (uint64_t)denom;
    m += 1ULL;

    /* return the structure containing multiplier and log(denom) */
    result.multiplier = (uint32_t)m;
    result.log_denom = l;
    return result;
}

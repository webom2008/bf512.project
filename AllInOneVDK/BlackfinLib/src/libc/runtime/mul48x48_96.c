/**************************************************************************
 *
 * mul48x48_96.c : $Revision: 1.5.24.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: mul48x48_96 - multiply two 48-bit values to give a 96-bit result

    Synopsis:

        void mul48x48_96 (int64_t parm1, int64_t parm2,
                          uint32_t *bits_0_31, uint32_t *bits_32_63,
                          int32_t *bits_64_95);

    Description:
 
        This function multiplies two 48-bit arguments together
        to produce a 96-bit result. The two arguments are held in int64_t
        types but the top 16 bits of each is ignored. The 96 result bits are
        stored in the three addresses passed into the function.

    Error Conditions:

        None, all inputs valid, although the top 16 bits of each int64_t
        parameter are assumed to be sign-extension bits.

    Algorithm:

        Perform 48 x 48 => 96 bit multiplication. Return three words of
        result in addresses passed in.

    Implementation:

        The 48 x 48 bit multiplication is performed as 9 16 x 16 => 32 bit
        multiplications. We accumulate the result into a number of uint64_t's
        as we go. A single chain of accumulates is used, which will
        use one of the accumulator registers, and this is successively
        shifted down as we compute higher-order parts of the result.
        Once we've finished accumulating into each of t0_39, t16_55 etc.,
        then the bottom 16 bits hold the final value for that part of
        the result, but the top bits go on to be accumulated into the
        next multiplies. The bottom 16 bits are placed into the output
        locations.

    Example:
    
        None.

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=libc")
#pragma file_attr("libFunc=mul48x48_96")
#pragma file_attr("libFunc=__mul48x48_96")
#pragma file_attr("libName=libc")
#pragma file_attr("prefersMem=internal")
#pragma file_attr("prefersMemNum=30")

#endif

#include "embc_rt.h"

extern void
mul48x48_96(int64_t a,
            int64_t b,
            uint32_t *bits_0_31,
            uint32_t *bits_32_63,
            int32_t *bits_64_95) 
{
    /* this holds the currently value in the accumulator register. */
    int64_t acc;

    /* copy the operands into unsigned types so we can do bitwise operations
    ** on them.
    */
    uint64_t a_unsigned = a;
    uint64_t b_unsigned = b;

    /* temporary results to hold parts of the final result. After computation
    ** the bottom 16 bits of each variable is the actual value of those
    ** bits of the result, while the top 24 bits go on to seed the 
    ** computation of the next 16 bits.
    */
    uint64_t t0_39, t16_55, t32_71, t48_87;
    int64_t t64_95;

    /* the input values, split into 16-bit parts. Part 0 is the least
    ** significant part, and part 2 is the most-significant.
    */
    uint16_t a0, a1, b0, b1;
    int16_t a2, b2;

    /* split up the inputs into 16-bit chunks.
    */
    a0 = (uint16_t)a_unsigned; /* the bottom 16 bits */
    a_unsigned >>= 16U;
    a1 = (uint16_t)a_unsigned; /* bits 16 -> 31 */
    a_unsigned >>= 16U;
    a2 = (int16_t)a_unsigned; /* bits 32 -> 47 */
    b0 = (uint16_t)b_unsigned; /* the bottom 16 bits */
    b_unsigned >>= 16U;
    b1 = (uint16_t)b_unsigned; /* bits 16 -> 31 */
    b_unsigned >>= 16U;
    b2 = (int16_t)b_unsigned; /* bits 32 -> 47 */

    /* compute the bottom 16-bits of the 96-bit result.
    ** Store them in the bottom of bits_0_31.
    */
    acc = __builtin_A_mult_FU((int16_t)a0, (int16_t)b0);
    t0_39 = (uint64_t)acc;
    *bits_0_31 = (uint32_t)t0_39 & 65535U; 

    /* compute bits 16 -> 31 of the 96-bit result.
    ** Store them in the top of bits_0_31.
    */
    acc = __builtin_A_ashift(acc, -16);
    acc = __builtin_A_mac_FU(acc, (int16_t)a1, (int16_t)b0);
    acc = __builtin_A_mac_FU(acc, (int16_t)a0, (int16_t)b1);
    t16_55 = (uint64_t)acc;
    *bits_0_31 |= (((uint32_t)t16_55 & 65535U) << 16); 

    /* compute bits 32 -> 47 of the 96-bit result.
    ** Store them in the bottom of bits_32_63.
    */
    acc = __builtin_A_ashift(acc, -16);
    acc = __builtin_A_mac_FU(acc, (int16_t)a1, (int16_t)b1);
    acc = __builtin_A_mac_MIS(acc, a2, b0);
    acc = __builtin_A_mac_MIS(acc, b2, a0);
    t32_71 = (uint64_t)acc;
    *bits_32_63 = (uint32_t)t32_71 & 65535U;

    /* compute bits 48 -> 63 of the 96-bit result.
    ** Store them in the top of bits_32_63.
    */
    acc = __builtin_A_ashift(acc, -16);
    acc = __builtin_A_mac_MIS(acc, a2, b1);
    acc = __builtin_A_mac_MIS(acc, b2, a1);
    t48_87 = (uint64_t)acc;
    *bits_32_63 |= (((uint32_t)t48_87 & 65535U) << 16); 

    /* compute bits 64 -> 95 of the 96-bit result.
    ** Store them in bits_64_94.
    */
    acc = __builtin_A_ashift(acc, -16);
    acc = __builtin_A_mac_IS(acc, a2, b2);
    t64_95 = acc;
    *bits_64_95 = (int32_t)t64_95;
}

/* End of file */

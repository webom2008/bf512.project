/**************************************************************************
 *
 * diviuk_bf.c : $Revision: 1.5.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: diviuk - divide a 32-bit unsigned int value by an
                       unsigned accum to give an unsigned int result

    Synopsis:

        unsigned int diviuk (unsigned int dividend, unsigned accum divisor);

    Description:
 
        This function divides a 32-bit unsigned integer argument by an 8.32
        unsigned accum argument to produce a 32-bit unsigned integer
        result. The result is rounded towards zero.

    Error Conditions:

        None, all inputs valid.
        If the result overflows the integer size, the value is truncated
        to unsigned int by discarding the higher order bits.

    Algorithm:

        Unsigned accum divisor converted to uint_uk_t and zero-extended
        to 64 bits.
        Unsigned int dividend zero-extended to 64 bits.
        Shift dividend up by 32 bits.
        Perform 64 / 64 => 64 bit unsigned divide.
        Truncate to 32 bits.

    Implementation:

    Example:
    
        #include <stdfix.h>
        unsigned int dividend;
        unsigned accum divisor;
        unsigned int result;

        result = diviuk (dividend,divisor);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=diviuk")
#pragma file_attr("libFunc=_diviuk")
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

#define __DISABLE_DIVIUK 1

#include <stdint.h>
#include <stdfix.h>

#if (UACCUM_FBIT + UACCUM_IBIT) > 64
#error This routine works only for accum types of 64-bit or less.
#endif

extern unsigned int 
diviuk(unsigned int _dividend,
       unsigned accum _divisor) 
{
    /* the dividend zero-extended to a unsigned 64-bit integer */
    uint64_t ull_dividend = _dividend;

    /* the divisor bit-pattern as an unsigned integer */
    uint_uk_t i_divisor = bitsuk(_divisor);

    /* the divisor bit-pattern held in an unsigned 64-bit integer */ 
    uint64_t ull_divisor = i_divisor;

    /* the result of the division, held in an unsigned 64-bit integer */
    uint64_t ull_result;

    /* shift the dividend up by the number of fractional bits in divisor */
    ull_dividend <<= (uint64_t)UACCUM_FBIT;

    /* perform 64-bit unsigned division to generate 64-bit result. */
    ull_result = ull_dividend / ull_divisor;

    /* return result truncated to unsigned int */
    return (unsigned int)ull_result;
}

/* End of file */

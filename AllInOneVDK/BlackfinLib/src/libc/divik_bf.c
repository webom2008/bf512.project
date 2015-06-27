/**************************************************************************
 *
 * divik_bf.c : $Revision: 1.5.14.1 $
 * (c) Copyright 2008 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: divik - divide a 32-bit int value by an accum to give an 
                      int result

    Synopsis:

        int divik (int dividend, accum divisor);

    Description:
 
        This function divides a 32-bit integer argument by a s8.31
        accum argument to produce a 32-bit integer result. The result
        is rounded towards zero.

    Error Conditions:

        None, all inputs valid.
        If the result overflows the integer size, the value is truncated
        to int by discarding the higher order bits.

    Algorithm:

        accum divisor converted to int_k_t and sign-extended
        to 64 bits.
        int dividend sign-extended to 64 bits.
        Shift dividend up by 31 bits.
        Perform 64 / 64 => 64 bit divide.
        Truncate to 32 bits.

    Implementation:

    Example:
    
        #include <stdfix.h>
        int dividend;
        accum divisor;
        int result;
  
        result = divik (dividend,divisor);

***************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup=stdfix.h")
#pragma file_attr("libFunc=divik")
#pragma file_attr("libFunc=_divik")
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

#define __DISABLE_DIVIK 1

#include <stdint.h>
#include <stdfix.h>

#if (ACCUM_FBIT + ACCUM_IBIT + 1) > 64
#error This routine works only for accum types of 64-bit or less.
#endif

extern int 
divik(int _dividend,
      accum _divisor) 
{
    /* the dividend sign-extended to a signed 64-bit integer */
    int64_t ll_dividend = _dividend;

    /* the dividend bit-pattern as an unsigned 64-bit integer */
    uint64_t ull_dividend = (uint64_t)ll_dividend;
  
    /* the divisor bit-pattern as a signed integer */
    int_k_t i_divisor = bitsk(_divisor);

    /* the divisor bit-pattern held in a signed 64-bit integer */
    int64_t ll_divisor = i_divisor;

    /* the result of the division, held in a signed 64-bit integer */
    int64_t ll_result;

    /* shift the dividend up by the number of fractional bits in divisor */
    ull_dividend <<= (uint64_t)ACCUM_FBIT;

    /* reinterpret shifted result as a signed 64-bit integer */
    ll_dividend = (int64_t)ull_dividend;
  
    /* perform 64-bit signed division to generate 64-bit result. */
    ll_result = ll_dividend / ll_divisor;

    /* return result truncated to int */
    return (int)ll_result;
}

/* End of file */

/************************************************************************
 *
 * xutil_fr.h
 *
 * (c) Copyright 2010 Analog Devices, Inc.  All rights reserved.
 * $Revision: 1.3.8.1 $
 ************************************************************************/

/* Internal header file for the DSP run-time library that contains
 * prototypes and definitions for fixed-point support functions
 */

#ifndef  XUTIL_FR_H
#define  XUTIL_FR_H

#include <fract_typedef.h>

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_5_3:"ADI header will re-use identfiers")
#pragma diag(suppress:misra_rule_5_4:"ADI header will re-use identfiers")
#pragma diag(suppress:misra_rule_5_6:"ADI header will re-use identfiers")
#pragma diag(suppress:misra_rule_5_7:"ADI header will re-use identfiers")
#pragma diag(suppress:misra_rule_8_1:"ADI header uses inline functions")
#pragma diag(suppress:misra_rule_8_5:"ADI header uses inline functions")
#pragma diag(suppress:misra_rule_8_11)
#pragma diag(suppress:misra_rule_10_1_a:"ADI header - use of inline function")
#pragma diag(suppress:misra_rule_12_7:  "ADI header - use of inline function")
#pragma diag(suppress:misra_rule_19_1)
#endif /* _MISRA_RULES */

/****************************************
**
**  Conversion  RAW64  <->  FLOAT
**
**  The following support functions convert between a float and a 64-bit
**  fixed-point number that has a variable number of digits after the
**  binary point.
**
****************************************/

 float raw64_to_float (long long    _raw64,
                       unsigned int _binary_decimal_digits);

#include "raw64_to_float.h"


#pragma linkage_name __float_to_raw64
 long long float_to_raw64 (float _x,
                           unsigned int _binary_decimal_digits);


/**************************************** 
**
** Data structure used to store result from
** the division operation inverse_ul()
**
****************************************/

typedef struct one_over_long_t
{
    unsigned long   scaled_reciprocal;
    int             sign_bits;
} one_over_long_t;


/****************************************       
**
**               INVERSE_UL     
**
**    Computes 1 / n, for n = (1..(2^31))
**
**    To convert the return value to a fract32:
**
**        (fract32) (return_val.scaled_reciprocal >> (33-return_val.signbits))
**
**    Support function used by the following window generator functions:
**        gen_blackman_fr32, gen_hamming_fr32, gen_hanning_fr32,
**        gen_harris_fr32 and gen_triangle_fr32.
**
*****************************************/

#ifndef TRACE
#pragma inline
#pragma always_inline
#endif
        static one_over_long_t
        inverse_ul (const int denumerator)
        {
            one_over_long_t     fraction;

            unsigned long long  numerator;
            unsigned long long  reciprocal;

            /* Perform fixed point division:  64.0 / 32.0 */
            numerator = (1ULL << 63);
            reciprocal = numerator / (unsigned long long) denumerator;

            /* Scale result to align with the MSB of the 32.0 return value */
            fraction.sign_bits = (int) 32 -
                                 (int) __builtin_norm_fr1x32 (reciprocal >> 32);

            fraction.scaled_reciprocal =
                                 (unsigned long) (reciprocal >>
                                              (fraction.sign_bits - 1));
            return fraction;
        }


/****************************************
**
**       MULTIPLY_INVERSE_LL
**
**    Computes:
**        LL_value * inverse_ul(n) = LL_value * (1 / n)
**
*****************************************/

#ifndef TRACE
#pragma inline
#pragma always_inline
#endif
        static fract32
        multiply_inverse_ll (const long long  value,
                             const one_over_long_t  fraction)
        {
            long long  product, p1, p2;

            int  sign_bits;

            /* Scale value to align with the MSB of the lower 32 bits */
            sign_bits = 32L - __builtin_norm_fr1x32 (value >> 32);

            p1 = value >> sign_bits;
            p2 = (long long) (fraction.scaled_reciprocal);
            product = p1 * p2;

            /* Scale correct product */
            product = product >> ((32L - sign_bits) - fraction.sign_bits);

            return (fract32)(product >> 32);
        }


/****************************************
**
**       MULTIPLY_INVERSE_L
**
**    Computes:
**        L_value * inverse_ul(n) = L_value * (1 / n)
**
**    Support function used by the following window generator function:
**        gen_triangle_fr32
**
*****************************************/

#ifndef TRACE
#pragma inline
#pragma always_inline
#endif
        static fract32
        multiply_inverse_l (const long  value,
                            const one_over_long_t  fraction)
        {
            long long  product, p1, p2;
            long  scaled_value;
            int  sign_bits;

            /* Scale value to align with the MSB of the lower 32 bits */
            sign_bits = __builtin_norm_fr1x32 (value);

            scaled_value = value << sign_bits;
            p1 = (long long) scaled_value;
            p2 = (long long) (fraction.scaled_reciprocal);
            product = p1 * p2;

            /* Scale correct product by:
            ** (32L - (32L - sign_bits - 1) - fraction.sign_bits)
            */
            product = product >> ((sign_bits + 1L) - fraction.sign_bits);

            return (fract32)(product >> 32);
        }


/****************************************
**
**              BESSEL0
**
**    Computes a modified Bessel function of the first kind (zero-th order)
**
**    Support function used by the following window generator function:
**        gen_kaiser_fr32
**
*****************************************/

#pragma linkage_name __bessel0
        long double bessel0 (const long double _x);


/****************************************
**
**              COS32_2PI
**
**    Computes cos_fr32 (2 * pi * (i/n)),  for i = [0 .. n)
**
**    Support function used by the following window generator functions:
**        gen_blackman_fr32, gen_hamming_fr32,
**        gen_hanning_fr32 and gen_harris_fr32
**
*****************************************/

#pragma linkage_name ___cos32_2PIx
        fract32 __cos32_2PIx(long long x);



/****************************************
**
**  Saturate  64-bit  ->  32-bits  
**
****************************************/
#if __VERSIONNUM__ < 0x08010000
/* Support for VDSP++5.0 and earlier */
static int __builtin_sat_fr1x64 (long long x);

#pragma inline
#pragma always_inline
static int __builtin_sat_fr1x64 (long long x)
{
    __asm volatile ("R2 = %0L >> 31;     \n\t"
                    "R2 = %0H + R2 (S);  \n\t"
                    "CC = R2 == 0;       \n\t"
                    "R2 = R2 << 31 (S);  \n\t"
                    "IF !CC %0L = R2;    \n\t"
                    : "+I" (x)
                    :
                    : "CC", "R2" );
    return (int) x;
}
#endif

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

#endif /* XUTIL_FR_H */

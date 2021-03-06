/*****************************************************************************
 *
 * atanf.c : $Revision: 1.4 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: atanf - arc tangent

    Synopsis:

        #include <math.h>
        float atanf (float x);

    Description:

        The atanf function returns the arc tangent of x. Both the argument x
        and the function's result are in radians.

        The atanf function returns a result that is in the range [-pi/2, pi/2].

    Error Conditions:

        The atanf function does not return an error condition.

    Algorithm:

        The algorithm is based on:

        "Software Manual for the Elementary Functions", Cody and Waite
        p194 ff (using the algorithm for non-decimal fixed-point machines)

    Example:

        #include <math.h>

        float  y = atanf (1.0F);    /* y = pi/4 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup=math.h")
#pragma file_attr("libFunc=__atanf")
#pragma file_attr("libFunc=atanf")

#pragma file_attr("libName=libdsp")
#pragma file_attr("prefersMem=internal")
#pragma file_attr("prefersMemNum=30")
#endif


#if !defined(TRACE)
#pragma optimize_for_speed
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
#pragma diag(suppress:misra_rule_12_7)
/* Suppress Rule 12.7 (required) where bitwise operators shall not be
** applied to operands whose underlying type is signed.
**
** Rationale: The function uses a signed integral division to compute a 
** signed fractional division. For the operation to work correctly, it is 
** neccessary to place the numerator data into the high half of the long long
** argument. A second shift operation is required to place the decimal point
** of the result correctly.
*/
#pragma diag(suppress:misra_rule_12_12)
/* Suppress Rule 12.12 (required) whereby the underlying bit representation
** of floating-point values shall not be used. 
**
** Rationale: Optimal performance on a fixed-point architecture sometimes
** depends on the ability to interrogate the bit patterns of certain
** floating-point values. Knowledge of the underlying representation is
** permitted provided that its use is restricted to values that are known
** to be part of a (heavily) restricted subset of the domain of floating-point
** numbers.
*/
#pragma diag(suppress:misra_rule_18_4)
/* Suppress Rule 18.4 (required) that states that unions shall not be used.
**
** Rationale: The use of unions is permitted so long as use of the feature
** is associated with accessing the underlying bit patterns of certain
** floating-point values to provide optimal floating-point performance.
*/
#endif /* _MISRA_RULES */


#if !defined(ETSI_SOURCE)
#define ETSI_SOURCE
#endif /* ETSI_SOURCE */


#include <math.h>
#include <string.h>
#include <fract2float_conv.h>
#include <libetsi.h>
#include <ccblkfn.h>


extern float
atanf (float _x)
{
    const float  x = _x;

    union {float fl; long fx;} f;

    float  rf, result;

    /* p1, p0, q1, q0 */
    const fract32 coeffs_pq[4] = {
        (fract32) 0xf97bcb78, 
        (fract32) 0xc3bbc29b,
        (fract32) 0x40000000, 
        (fract32) 0x5a666981 
    };

    /* a1 = pi/6, a2 = pi/2, a3 = pi/3 */
    const float coeffs_pi[3] = {
        0.52359877559829887308F, 
        1.57079632679489661923F,   
        1.04719755119659774615F 
    }; 

    const long  one = 0x3f800000L;  /* 1.0 in 32-bit hexadecimal notation */

    const fract32  min_scale = (fract32) 0x224c28bd;  /* 2 - sqrt(3) */
    const fract32  A_scale   = (fract32) 0x6ed9eba1;  /* sqrt(3) / 2 */
    const fract32  B_scale   = (fract32) 0x376cf5d1;  /* sqrt(3) / 4 */
    const fract32  eps       = (fract32) 0x0000b505;  /* 2^(-31/2)   */

    fract32  y, g, p, q, num, denum;
    fract32  x_fract;
    int n;

    long long  r;


    f.fl = fabsf (x);

    /* execution path marker */
    n = 0;

    if (f.fx > one)
    {
        /* scale x for x > 1.0 */
        f.fl = 1.0F / f.fl;

        /* increment marker */
        n += 2; 
    }
   
    /* convert x to fixed-point */
    y = float_to_fr32 (f.fl);

    if (expected_true (y > min_scale))
    {
        /* scale for y > (2 - sqrt(3)), where y = 2 * f */ 
        y = shr_fr1x32 (y, 1);

        num = multr_fr1x32x32 (y, A_scale);
        num = L_sub (num, (fract32) 0x20000000);

        denum = L_add (shr_fr1x32 (y, 1), B_scale); 

        /* perform fract32 division: y = num / denum */
        r = ((long long) num) << 31;
        y = (fract32) (r / (long long) denum);

        f.fl = fr32_to_float (y);

        /* increment marker */
        n += 1;
    }

    /* set default return value */
    result = f.fl;

    if (expected_true (L_abs (y) >= eps))
    {
        /* polynomial approximation for |y| > (2^(-31/2)) */
        g = multr_fr1x32x32 (y, y);        

        /* p = ((p1 * g) + p0) * g */
        p = multr_fr1x32x32 (coeffs_pq[0], g);
        p = L_add (p, coeffs_pq[1]);
        p = multr_fr1x32x32 (p, g);

        /* q = (q1 * g) + q0 */
        q = multr_fr1x32x32 (coeffs_pq[2], g);
        q = L_add (q, coeffs_pq[3]);

        /* perform fract32 division: r = 0.5 * (p / q) */
        r = ((long long) p) << 30;
        r = r / (long long) q;

        rf = fr32_to_float ((fract32) r);

        /* operation requires extra dynamic range */
        result += (rf * f.fl);
    }

    /* correct result depending on path selected */
    if (expected_true (n > 0))
    {
        if (n > 1)
        {
           result = -result; 

        }

        result = result + coeffs_pi[n-1];

    }

    /* copy bit-pattern in x to x_fract for subsequent compare op */
    memcpy (&x_fract, &x,sizeof(x));

    if (x_fract < 0) 
    {
        /* copy sign x */
        result = -result;

    } 

    return  result;

} 
/* End of File */

/*****************************************************************************
 *
 * atan_fr32.c : $Revision: 1.4 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: atan_fr32 - arc tangent

    Synopsis:

        #include <math.h>
        fract32 atan_fr32 (fract32 x);

    Description:

        The atan_fr32 function returns the arc tangent of x. Both the argument
        x and the function's result are in radians.

        The function is defined for fractional input values in the range
        [-1.0, 1.0), and it will return a result that is in the range
        [-pi/4, pi/4].

    Error Conditions:

        The atan_fr32 function does not return an error condition.

    Algorithm:

        The algorithm is based on:

        "Software Manual for the Elementary Functions", Cody and Waite
        p194 ff (using the algorithm for non-decimal fixed-point machines)

    Example:

        #include <math.h>

        fract32  y = atan_fr32 (0x80000000);    /* y = -pi/4 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup=math_bf.h")
#pragma file_attr("libFunc=__atan_fr32")
#pragma file_attr("libFunc=atan_fr32")

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
#endif /* _MISRA_RULES */


#if !defined(ETSI_SOURCE)
#define ETSI_SOURCE
#endif /* ETSI_SOURCE */


#include <math.h>
#include <libetsi.h>
#include <ccblkfn.h>


extern fract32
atan_fr32 (fract32 _x)
{
    const fract32 x = _x;

    /* p1, p0, q2, q1, q0 */
    const fract32 coeffs_pq[5] = {
        (fract32) 0xe8f58a41,
        (fract32) 0xd1ead61e,
        (fract32) 0x10000000,
        (fract32) 0x4c091df8,
        (fract32) 0x451fbedf
    };

    const fract32  min_scale   = (fract32) 0x2aaaaaab;  /* 1/3         */
    const fract32  A_scale     = (fract32) 0x6ed9eba1;  /* sqrt(3) / 2 */
    const fract32  B_scale     = (fract32) 0x376cf5d1;  /* sqrt(3) / 4 */
    const fract32  eps         = (fract32) 0x0000b505;  /* 2^(-31/2)   */
    const fract32  pi_over_six = (fract32) 0x430548e1;  /* pi / 6      */

    fract32  g, p, q, tmp, num, denum, result;
    fract32  y = L_abs (x);
    fract32  coeffs_pi = 0;

    long long  r;

    if (expected_true (y > min_scale))
    {
        /* scale for y > 1/3 */ 
        y = shr_fr1x32 (y, 1);

        num = multr_fr1x32x32 (y, A_scale);
        num = L_sub (num, (fract32) 0x20000000);

        denum = L_add (shr_fr1x32 (y, 1), B_scale); 

        /* perform fract32 division: y = num / denum */
        r = ((long long) num) << 31;
        y = (fract32) (r / (long long) denum);

        /* correction coefficient */
        coeffs_pi = pi_over_six;
    }

    /* set default return value */
    result = y;

    if (expected_true (L_abs (y) >= eps))
    {
        /* polynomial approximation for |y| > (2^(-31/2)) */
        g = multr_fr1x32x32 (y, y);        

        /* p = ((p1 * g) + p0) * g */
        p = multr_fr1x32x32 (coeffs_pq[0], g);
        p = L_add (p, coeffs_pq[1]);
        p = multr_fr1x32x32 (p, g);

        /* q = (((q2 * g) + q1) * g) + q0 */
        q = multr_fr1x32x32 (coeffs_pq[2], g);
        q = L_add (q, coeffs_pq[3]);
        q = multr_fr1x32x32 (q, g);
        q = L_add (q, coeffs_pq[4]);

        /* perform fract32 division: r = 0.5 * (p / q) */
        r   = ((long long) p) << 30;
        r   = r / (long long) q;
        tmp = (fract32) r;

        result = L_add (multr_fr1x32x32 (tmp, y), result);
    }

    /* correct result depending on path selected */
    result = L_add (result, coeffs_pi);

    if (x < (fract32) 0x0) 
    {
        /* copy sign x */
        result = L_negate (result);

    }
    return  result;

} 
/* End of File */

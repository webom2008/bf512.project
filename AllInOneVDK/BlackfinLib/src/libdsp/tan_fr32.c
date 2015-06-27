/*****************************************************************************
 *
 * tan_fr32.c : $Revision: 1.5 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: tan_fr32 - tangent

    Synopsis:

        #include <math.h>
        fract32 tan_fr32 (fract32 x);

    Description:

        The tan_fr32 function returns the tangent of x. Both the argument x
        and the function results are in radians. The function is defined for
        fractional input values between [- pi/4, pi/4]. The output from the
        function is in the range [-1.0, 1.0).

    Error Conditions:

        The tangent function returns a zero if the input is not within the
        defined domain.

    Algorithm:

        The algorithm is based on:

        "Software Manual for the Elementary Functions", Cody and Waite
        p150 ff (using the algorithm for non-decimal fixed-point machines)

    Example:

        #include <math.h>

        fract32  y = tan_fr32 (0x9b7812af);  /* tan(-pi/4) = -1 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup=math_bf.h")
#pragma file_attr("libFunc=__tan_fr32")
#pragma file_attr("libFunc=tan_fr32")

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
#endif /* _MISRA_RULES */

#if !defined(ETSI_SOURCE)
#define ETSI_SOURCE
#endif /* ETSI_SOURCE */

#include <math.h>
#include <libetsi.h>
#include <ccblkfn.h>

extern fract32
tan_fr32 (fract32 _x)
{
    const fract32  x = _x;

    const fract32  eps = (fract32) 0x000b504f;  /* 2^(-23/2) = 3.45266983e-4 */
    const fract32  pi_quart = (fract32) 0x6487ed51;  /* pi/4 */

    /* set error return */
    fract32  result = (fract32) 0x0;

    /* p1, p0, q2, q1, q0 */
    const fract32 coeffs_pq[5] = {
        (fract32) 0xff0be843,   /* -0.0074491186974 */
        (fract32) 0x15555554,   /*  0.1666666658406 */
        (fract32) 0x0105b543,   /*  0.0079866960665 */
        (fract32) 0xe38a1ef0,   /* -0.2223473860140 */
        (fract32) 0x40000000    /*  0.5000000000000 */
    };

    fract32  y, h, g, num, denum;

    unsigned long long  r;


    y = L_abs (x);

    if (expected_true (y <= pi_quart)) 
    {
        /* if |x| <= (pi/4), otherwise return 0 */  

        /* return x for x <= f_eps */
        result = y;
        
        if (expected_true (y > eps))
        {
            /* if |x| > eps (=3.452669830012e-4) */

            /* g = h * h = y * y */ 
            h = y;
            g = multr_fr1x32x32 (h, h);

            /* num = ((p1 * g) + p0) * g * h */
            num = multr_fr1x32x32 (g, coeffs_pq[0]);
            num = L_add (num, coeffs_pq[1]);
            num = multr_fr1x32x32 (num, g);
            num = multr_fr1x32x32 (num, h);

            /* denum = ((q2 * g) + q1) * g + q0 */
            denum = multr_fr1x32x32 (g, coeffs_pq[2]);
            denum = L_add (denum, coeffs_pq[3]);
            denum = multr_fr1x32x32 (denum, g);
            denum = L_add (denum, coeffs_pq[4]);

            /* perform fract32 division: r = num / denum */
            r = ((unsigned long long) num) << 31;
            r = r / (unsigned long long) denum;

            /* result = y + r */
            result = L_add (result, (fract32) r);
        }

        if (x < 0)
        {
            result = L_negate (result);
        } 
    }
 
    return  result;

}
/* End of File */

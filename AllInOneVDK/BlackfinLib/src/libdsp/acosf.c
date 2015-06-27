/*****************************************************************************
 *
 * acosf.c : $Revision: 1.4 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: acosf - arc cosine

    Synopsis:

        #include <math.h>
        float acosf (float x);

    Description:

        The acosf function returns the arc cosine of x. Both the argument x
        and the function's result are in radians.

        The input for the function must be in the range [-1, 1]. The function
        returns a result that will be in the range [0, pi].

    Error Conditions:

        The acosf function returns 0.0 if the input is not within the
        defined range.

    Algorithm:

        The algorithm is based on:

        "Software Manual for the Elementary Functions", Cody and Waite
        p174 ff (using the algorithm for non-decimal fixed-point machines)

    Example:

        #include <math.h>

        float  y = acosf (0.0F);    /* y = pi/2 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup=math.h")
#pragma file_attr("libFunc=__acosf")
#pragma file_attr("libFunc=acosf")

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
#include <ccblkfn.h>
#include <fract2float_conv.h>


extern float
acosf (float _x)
{
    const float x = _x;

    union {float fl; long fx;} yf;

    /* p2, p1, q2, q1, q0 */
    const fract32 coeffs_pq[5] = {
        (fract32) 0xfa3d395c,
        (fract32) 0x0aaaa8cb,
        (fract32) 0x0b6bd1aa,
        (fract32) 0xc0a1493c,
        (fract32) 0x40000000
    };

    /* a0 = 0, a1 = pi/4, b0 = pi/2, b1 = pi/4 */
    const float coeffs_pi[4] = {
        0.0F,
        0.785398163397448F,
        1.570796326794897F,
        0.785398163397448F
    };

    const long  one = 0x3f800000L;  /* 1.0 in 32-bit hexadecimal notation */
    const long  eps = 0x39b504f3L;  /* 3.4e-4 in 32-bit hexadecimal notation */

    float    result = 0.0F;
    float    rf;

    fract32  f, g, p, q, y, tmp;
    fract32  x_fract;
    int      idx = 1;

    unsigned long long  r;


    yf.fl = fabsf (x);

    if (expected_true (yf.fx <= one))
    {
        /* acos(x) only defined for 0 <= |x| <= 1 */

        if (expected_true (yf.fx >= eps))
        {
            /* use polynomial approximation if |x| > eps (=3.45266983001e-4) */

            y = float_to_fr32 (yf.fl);

            if (expected_true (y > 0x40000000L))
            {
                idx = 0;              /* set flag idx to path x > 0.5 */

                /* f = 1 - Y = (0.5 - y) + 0.5
                ** g = f / 2
                */
                f = L_add (L_sub (0x40000000L, y), 0x40000000L);
                g = shr_fr1x32 (f, 1);

                /* Y = -2 * sqrt (g) */
                tmp = L_negate (sqrt_fr32 (g));
                y   = shl_fr1x32 (tmp, 1);

                yf.fl = fr32_to_float (y);

                /* Variables required subsequently: idx, g, yf = float(y) */

            }
            else
            {
                /* g = Y * Y */
                g = multr_fr1x32x32 (y, y);

                /* Variables required subsequently: idx, g, yf = |x| */

            }

            /* p = (p2*g + p1) * g */
            tmp = multr_fr1x32x32 (coeffs_pq[0], g);
            tmp = L_add (coeffs_pq[1], tmp);
            p   = multr_fr1x32x32 (tmp, g);

            /* q = (((q2 * g) + q1) * g) + q0 */
            tmp = multr_fr1x32x32 (coeffs_pq[2], g);
            tmp = L_add (coeffs_pq[3], tmp);
            tmp = multr_fr1x32x32 (tmp, g);
            q   = L_add (coeffs_pq[4], tmp);

            /* perform fract32 division: r = p / q */
            r = ((unsigned long long) p) << 31;
            r = r / (unsigned long long) q;

            rf = fr32_to_float ((fract32) r);

            /* Need to revert back to floating point */
            yf.fl = yf.fl + (yf.fl * rf);
        }

        /* copy bit-pattern in x to x_fract for subsequent compare op */
        memcpy (&x_fract,&x,sizeof(x));

        if (x_fract < 0)
        {
            /* x < 0, return ((b[idx] + result) + b[idx]) */
            result = (coeffs_pi[idx+2] + yf.fl) + coeffs_pi[idx+2];
        }
        else
        {
           /* x >= 0, return ((a[idx] - result) + a[idx]) */
           result = (coeffs_pi[idx] - yf.fl) + coeffs_pi[idx];
        }
    }

    return result;

}

/* End of File */

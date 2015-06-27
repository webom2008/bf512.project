/*****************************************************************************
 *
 * cotf.c : $Revision: 1.4.6.1 $
 *
 * (c) Copyright 2010 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: cotf - cotangent

    Synopsis:

        #include <math.h>
        float cotf (float x);

    Description:

        The cotf function returns the cotangent of x. Both the argument x and
        the function result are in radians. The function is defined for input
        values in the range [-51471, 51471].

    Error Conditions:

        The cotf function returns a zero if the input is not within the
        defined range.

    Algorithm:

        The algorithm is based on:

        "Software Manual for the Elementary Functions", Cody and Waite
        p150 ff (using the algorithm for non-decimal fixed-point machines)

    Example:

        #include <math.h>
        #define PI 3.1415927F

        float r1 = cotf (PI/4);  /* r1 = cot(pi/4) = 1       */
        float r2 = cotf (PI/6);  /* r2 = cot(pi/6) = 1.73205 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup=math.h")
#pragma file_attr("libFunc=__cotf")
#pragma file_attr("libFunc=cotf")

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
** necessary to place the numerator data into the high half of the long long
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
#include <libetsi.h>
#include <ccblkfn.h>
#include "xutil_fr.h"

extern float
cotf (float _x)
{
    const float  x = _x;

    typedef long long fract_17_47_t;

    union {float fl; long fx;} yabs;

    const long  ymax  = 0x47490fdb;        /* 2^15 * PI/2 = 5.147185546875e+4*/
    const long  f_eps = 0x39b504f3;        /* 2^(-23/2)   = 3.452669830012e-4*/

    const fract32  eps = (fract32) 0x000b504f;   /* 2^(-23/2) = 3.45266983e-4*/

    const float  two_over_pi = 0.63661977236758134308F;   /* 2/pi */

    const fract_17_47_t pi_by_2r = 0x0000c90fdaa22169LL;

    /* set error return */
    float  result = 0.0F;

    /* p1, p0, q1, q0 */
    const fract32 coeffs_pq[4] = {
        (fract32) 0xffe4a4be,   /* -0.0008348534 */
        (fract32) 0x15555c71,   /*  0.1666675142 */
        (fract32) 0xe614f318,   /* -0.2024856694 */
        (fract32) 0x40000000    /*  0.5000000000 */
    };

    fract32  h, g, num, denum, fr;
    long  n;

    fract_17_47_t r;
    float  tmp;

    yabs.fl = fabsf (x);

    if (expected_true (yabs.fx <= ymax))
    {
        /* if |x| > ymax (=5.147185546875e4), return 0 */

        /* return 1/x for x <= f_eps (or 0 for x = 0) */
        result = x;
        n = (yabs.fx == (long) 0x0) ? (long) 0x1 : (long) 0x0;

        if (expected_true (yabs.fx > f_eps))
        {
            /* if |x| > f_eps (=3.452669830012e-4) */

            /* n = (|x| * 2/pi) + 0.5) */
            tmp = ((yabs.fl * two_over_pi) + 0.5F);
            n = (long) float_to_raw64 (tmp, 0UL);

            /* perform scaling input x
            **    fr = x - (n * pi/2)  (using 17.47 fixed point notation)
            */

            /* convert yabs to 17.47 fixed point */
            r = float_to_raw64 (yabs.fl, 47UL);
            r -= ((long long) n) * pi_by_2r;
            fr = (fract32) ((r + (long long) 0x8000) >> 16);

            /* convert 17.47 fixed point to 32bit float */
            result = raw64_to_float (r, 47UL);

            if (expected_true (L_abs (fr) > eps))
            {
                /* |f| >= eps (=3.452669830012e-4) */

                /* g = h * h = fr * fr */
                h = fr;
                g = multr_fr1x32x32 (h, h);

                /* num = ((p1 * g) + p0) * g * h */
                num = multr_fr1x32x32 (g, coeffs_pq[0]);
                num = L_add (num, coeffs_pq[1]);
                num = multr_fr1x32x32 (num, g);
                num = multr_fr1x32x32 (num, h);

                /* denum = (q1 * g) + q0 */
                denum = multr_fr1x32x32 (g, coeffs_pq[2]);
                denum = L_add (denum, coeffs_pq[3]);

                result += (raw64_to_float ((long long) num, 31UL) /
                           raw64_to_float ((long long) denum, 31UL));
            }

            if (x < 0.0F)
            {
                result = -result;
            }

        }

        if ((n & (long) 0x1) == 0)
        {
            /* N even */
            result = 1.0F / result;
        }
        else
        {
            result = -result;
        }
    }

    return  result;

}
/* End of File */

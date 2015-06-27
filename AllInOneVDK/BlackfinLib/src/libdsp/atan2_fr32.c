/*****************************************************************************
 *
 * atan2_fr32.c : $Revision: 1.4 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: atan2_fr32 - arc tangent of quotient

    Synopsis:

        #include <math.h>
        fract32 atan2_fr32 (fract32 y, fract32 x);

    Description:

        The atan2_fr32 function computes the arc tangent of the input value y
        divided by the input value x. Both the arguments x and y, and the
        function's result are in radians.

        The atan2_fr32 function is defined for fractional input values in the
        range [-1.0, 1.0). The output from this function is scaled by pi and
        is in the range [-1.0, 1.0).

    Error Conditions:

        The atan2_fr32 function returns zero if x = 0 and y = 0.

    Algorithm:

        The algorithm is based on:

        "Software Manual for the Elementary Functions", Cody and Waite
        p194 ff (using the algorithm for non-decimal fixed-point machines)

    Example:

        #include <math.h>

        fract32  y;
        y = atan2_fr32 (0x7fffffff, 0x80000000); /* y = 3/4 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup=math_bf.h")
#pragma file_attr("libFunc=__atan2_fr32")
#pragma file_attr("libFunc=atan2_fr32")

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
atan2_fr32 (fract32 _y, fract32 _x)
{
    const fract32  x = _x;
    const fract32  y = _y;
    const fract32  xabs = L_abs (_x);
    const fract32  yabs = L_abs (_y); 
 
    fract32  fy, f, g, p, q, num, denum, tmp, result; 
    int  n;

    unsigned long long  ru;
    long long  rs;

    /* p1, p0, q2, q1, q0 */
    const fract32 coeffs_pq[5] = { 
        (fract32) 0xe8f58a41, 
        (fract32) 0xd1ead61e,
        (fract32) 0x10000000, 
        (fract32) 0x4c091df8,
        (fract32) 0x451fbedf 
    };

    /* a1 = 1/6, a2 = 1/2, a3 = 1/3 */
    const fract32 coeffs_pi[3] = { 
        (fract32) 0x15555555,  
        (fract32) 0x40000000,
        (fract32) 0x2aaaaaab
    }; 

    const fract32  min_scale = (fract32) 0x2aaaaaab;  /* 1/3         */
    const fract32  A_scale   = (fract32) 0x6ed9eba1;  /* sqrt(3) / 2 */
    const fract32  B_scale   = (fract32) 0x376cf5d1;  /* sqrt(3) / 4 */
    const fract32  eps       = (fract32) 0x0000b505;  /* 2^(-31/2)   */
    const fract32  inv_pi    = (fract32) 0x28be60dc;  /* 1 / pi      */


    if (expected_false (yabs == (fract32) 0x0))
    {
        /* return 1 if x < 0, 0 otherwise */ 
        result = (fract32) 0x0;

    }
    else if (expected_false (xabs == (fract32) 0x0))
    {
        /* return -1/2 if y < 0, 1/2 otherwise */ 
        result = (fract32) 0x40000000;

    }
    else if (expected_false (xabs == yabs))
    {
        /* return (-3/4 * pi / pi ) = -3/4 if x < 0, y < 0
                  (-1/4 * pi / pi ) = -1/4 if x > 0, y < 0
                  ( 3/4 * pi / pi ) =  3/4 if x < 0, y > 0
                  ( 1/4 * pi / pi ) =  1/4 if x > 0, y > 0
        */
        result = (fract32) 0x20000000;

    }
    else
    {
        /* execution path marker */
        n = 0;

        if (yabs > xabs)
        {
            /* perform fract32 division: f = xabs / yabs */
            ru = ((unsigned long long) xabs) << 31;
            ru = ru / (unsigned long long) yabs;
            f  = (fract32) ru;

            /* increment marker */
            n += 2;
        } 
        else 
        {
            /* perform fract32 division: f = yabs / xabs */
            ru = ((unsigned long long) yabs) << 31;
            ru = ru / (unsigned long long) xabs;
            f  = (fract32) ru;

        }
   
        if (expected_true (f > min_scale))
        {
            /* scale for f > 1/3 */ 
            fy = shr_fr1x32 (f, 1);

            num = multr_fr1x32x32 (fy, A_scale);
            num = L_sub (num, (fract32) 0x20000000);

            denum = L_add (shr_fr1x32 (fy, 1), B_scale); 

            /* perform fract32 division: f = num / denum */
            rs = ((long long) num) << 31;
            rs = rs / (long long) denum;
            f  = (fract32) rs;

            /* increment marker */
            n += 1;
        }

        /* set default return value */
        result = f;

        if (expected_true (L_abs (f) >= eps))
        {
            /* polynomial approximation for |f| > (2^(-31/2)) */
            g = multr_fr1x32x32 (f, f);        

            /* p = ((p1 * g) + p0) * g */
            p = multr_fr1x32x32 (coeffs_pq[0], g);
            p = L_add (p, coeffs_pq[1]);
            p = multr_fr1x32x32 (p, g);

            /* q = (((q2 * g) + q1) * g) + q0 */
            q = multr_fr1x32x32 (coeffs_pq[2], g);
            q = L_add (q, coeffs_pq[3]);
            q = multr_fr1x32x32 (q, g);
            q = L_add (q, coeffs_pq[4]);

            /* perform fract32 division:  r = 0.5 * (p / q) */
            rs  = ((long long) p) << 31;
            rs  = (rs / (long long) q) >> 1;
            tmp = (fract32) rs;

            result = L_add (multr_fr1x32x32 (tmp, f), result);
        }

        /* scale result by pi */
        result = multr_fr1x32x32 (result, inv_pi); 

        /* correct result depending on path selected */
        if (expected_true (n > 0))
        {
            if (n > 1)
            {
                result = L_negate (result); 
            }
            result = L_add (result, coeffs_pi[n-1]);
        }
    } 

    if (x < (fract32) 0x0)
    {
        /* result = (pi - result) / pi 
                  = 1 - (result / pi) 
                  = - (-1 + (result / pi))
        */ 
        result = L_negate( L_add (result, (fract32) 0x80000000)); 
    } 

    if (y < (fract32) 0x0)
    {
        result = L_negate (result);
    }

    return  result;

} 
/* End of File */

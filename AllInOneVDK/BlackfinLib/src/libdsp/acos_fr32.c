/*****************************************************************************
 *
 * acos_fr32.c : $Revision: 1.6 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: acos_fr32 - arc cosine

    Synopsis:

        #include <math.h>
        fract32 acos_fr32( fract32 x );

    Description:

        The acos_fr32 function returns the arc cosine of x. Both the argument
        x and the function's result are in radians.

        The function is defined for fractional input values between 0 and 0.9.
        The output from the function is in the range
        [acos (0) * 2/pi, acos (0.9) * 2/pi].

    Error Conditions:

        The acos_fr32 function returns a zero if the input is not within the
        defined range.

    Algorithm:

        The algorithm is based on:

        "Software Manual for the Elementary Functions", Cody and Waite
        p174 ff (using the algorithm for non-decimal fixed-point machines)

    Example:

        #include <math.h>

        fract32  y = acos_fr32 (0x40000000);    /* acos_fr32(0.5) = 2/3 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup=math_bf.h")
#pragma file_attr("libGroup=math.h")
#pragma file_attr("libFunc=__acos_fr32")
#pragma file_attr("libFunc=acos_fr32")

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
#include <ccblkfn.h>
#include <fract2float_conv.h>

extern fract32 
acos_fr32 (fract32 _x)
{
    const fract32  x = _x; 

    /* p3, p2, p1, q3, q2, q1, q0 */
    const fract32 coeffs_pq[7] = { 
        (fract32) 0x024df77d, 
        (fract32) 0xf4bc49fb, 
        (fract32) 0x0aaaaaaa,
        (fract32) 0xfc1fa0b2, 
        (fract32) 0x280f0b0f, 
        (fract32) 0x9f9ceeab, 
        (fract32) 0x40000000
    };

    const fract32 max_x       = (fract32) 0x73333333;
    const fract32 eps         = (fract32) 0x0000b505;
    const fract32 large_y     = (fract32) 0x48000000;
    const fract32 minus_one   = (fract32) 0x80000000;
    const fract32 two_over_pi = (fract32) 0x517cc1b7;

    fract32  f, g, p, q, tmp, result, y;
    int      idx;

    unsigned long long  r;


    /* return 0x0 for an out-of-range error */
    result = (fract32) 0x0;

    idx = 1;

    if (expected_true ((x >= 0x0) && (x <= max_x)))
    {
        /* 0 <= x <= 0.9 */  
        y = x;

        if (expected_true (y >= eps))
        { 
            /* x >= eps (=2.157918643757774e-005) */ 

            if (expected_true (y >= large_y))
            {
                /* y >= 0.56250 */
 
                idx = 0;

                /* f = 1 - Y = (0.5 - y) + 0.5 or - ( -1 + y)
                ** g = f / 2 (with rounding)
                */ 
                f = L_negate (L_add (minus_one, y));

                g = shr_fr1x32 (L_add (f, (fract32) 0x1), 1);  

                /* Y = -2 * sqrt (g) */
                tmp = L_negate (sqrt_fr32 (g)); 

                y = shl_fr1x32 (tmp, 1);

                /* Variables required subsequently: idx, g, y */ 
            }
            else
            {
                /* y < 0.56250 */

                /* g = Y * Y */ 
                g = multr_fr1x32x32 (y, y);  

                /* Variables required subsequently: idx, g, y */
            } 
  
            /* p = (((p3*g + p2) * g) + p1) * g */
            tmp = multr_fr1x32x32 (coeffs_pq[0], g);
            tmp = L_add (coeffs_pq[1], tmp);
            tmp = multr_fr1x32x32 (tmp, g);
            tmp = L_add (coeffs_pq[2], tmp);
            p   = multr_fr1x32x32 (tmp, g);
 
            /* q = (((((q3 * g) + q2) * g) + q1) * g) + q0 */
            tmp = multr_fr1x32x32 (coeffs_pq[3], g);
            tmp = L_add (coeffs_pq[4], tmp);
            tmp = multr_fr1x32x32 (tmp, g);
            tmp = L_add (coeffs_pq[5], tmp);
            tmp = multr_fr1x32x32 (tmp, g);
            q   = L_add (coeffs_pq[6], tmp);

            /* perform fract32 division: r = p / q */
            r = ((unsigned long long) p) << 31;
            r = r / (unsigned long long) q;

            tmp = multr_fr1x32x32 ((long) r, y);
            y   = L_add (tmp, y);

        }         

        /* If idx = 0 => result = (0 - y + 0) * (2/pi) 
                                = - (y * 2/pi) 

           If idx = 1 => result = ((pi/4 - y) + (pi/4)) * (2/pi)        
                                =  (pi/2 - y) * (2/pi)
                                =  (pi/2 * 2/pi) - (y * 2/pi)
                                = 1 - (y * 2/pi) 
                                = - (-1 + (y * 2/pi))
        */

        y = multr_fr1x32x32 (two_over_pi, y);

        if (idx == 1)
        {
            y = L_add (minus_one, y);       

        }
        result = L_negate (y);

    }
    return result;

}

/* End of File */

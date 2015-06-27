/*****************************************************************************
 *
 * sinf.c : $Revision: 1.3.6.1 $
 *
 * (c) Copyright 2010 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: sinf - sine

    Synopsis:

        #include <math.h>
        float sinf (float x);

    Description:

        The sinf function returns the sine of x. Both the argument x
        and the function result are in radians, and x must be in the
        range [-102940.0, 102940.0].

    Error Conditions:

        The sinf function returns 0.0 if the input is outside the
        defined domain.

    Algorithm:

        The algorithm is based on:

        "Software Manual for the Elementary Functions", Cody and Waite
        p125 ff (using the algorithm for non-decimal fixed-point machines)

    Example:

        #include <math.h>

        float  y = sinf (0.52359878F);  /* y = sin(pi/6) = 0.5 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =sinf")
#pragma file_attr("libFunc  =__sinf")
#pragma file_attr("libFunc  =sin")

    /* Called by polarf */
#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =polarf")
#pragma file_attr("libFunc  =__polarf")
#pragma file_attr("libFunc  =polar")

    /* Called by cexpf */
#pragma file_attr("libFunc  =cexpf")
#pragma file_attr("libFunc  =__cexpf")
#pragma file_attr("libFunc  =cexp")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")
#endif

#if !defined(TRACE)
#pragma optimize_for_space
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
#include <libetsi.h>
#include <ccblkfn.h>
#include <fract2float_conv.h>
#include "xutil_fr.h"

extern float
sinf (float _x)
{
    const float  x = _x;

    typedef long long fract_15_49_t;

    union {float fl; long fx;} yabs;

    const long  ymax        = 0x47c90e00;  /* pi*2^(30/2)      = 102940 */
    const long  tresh_scale = 0x46c90fdb;  /* pi*2^(((30/2)-2) =  25736 */

    const long  f_eps = 0x39b504f3;  /* 2^(-23/2)   = 3.452669830012e-4*/

    const fract32  eps = (fract32) 0x000b504f;   /* 2^(-23/2) = 3.45266983e-4*/

    const float  one_over_pi = 0.31830988618379067154F;   /* 1/pi */

    const fract_15_49_t pi_r[2] = { 
        0x0003243f6a8885a3LL, /* pi (15.49) */
        0x0000c90fdaa22169LL  /* pi (17.47) */
    };  

    /* set error return */
    float  result = 0.0F;

    /* r5, r4, r3, r2, r1 */
    const fract32 coeffs_r[5] = {
        (fract32) 0xffff32f9,   /* -0.0000244411867 */
        (fract32) 0x001716ba,   /*  0.0007046136593 */
        (fract32) 0xfe5fe857,   /* -0.0126981330068 */
        (fract32) 0x111110b7,   /*  0.1333332915289 */
        (fract32) 0xaaaaaab0    /* -0.6666666643530 */
    };

    fract32  f, g, rg;
    long  n, k;
    unsigned long  scale_adj;

    fract_15_49_t  ff;

    float  temp;


    yabs.fl = fabsf (x);

    if (expected_true (yabs.fx <= ymax)) 
    {
        /* if |x| > ymax (=102940), return 0 */  

        /* return x for x <= f_eps */
        result = x;
        
        if (expected_true (yabs.fx > f_eps))
        {
            /* if |x| > f_eps (=3.452669830012e-4) */

            /* n = (y * 1/pi) + 0.5) */
            temp = ((yabs.fl * one_over_pi) + 0.5F);
            n    = (long) temp;

            /* for |x| > pi*2^(((30/2)-2) = 25736 
            **   use 17.47 notation,
            **   otherwise use 15_49 for extra precision
            */
            scale_adj = (yabs.fx >= tresh_scale) ? 2UL : 0UL;

            if (n > 0)
            {
                /* ff = |x| - n * pi */
                ff  = (fract_15_49_t) float_to_raw64 (yabs.fl, 
                                                      48UL - scale_adj);
                ff -= (((fract_15_49_t) n) * pi_r[scale_adj >> 1]);
                result = raw64_to_float (ff, 48UL - scale_adj);
                f = (fract32) (ff >> (18UL - scale_adj));
            }
            else
            {
                /* for n = 0 => ff = |x| - 0 * pi = |x| */
                result = yabs.fl;
                f = (fract32) float_to_raw64 (yabs.fl, 30UL);
            }

            if (expected_true (L_abs (f) > eps))
            { 
                /* |f| >= eps (=3.452669830012e-4) */

                /* g = f * f */ 
                g = multr_fr1x32x32 (f, f);

                /* rg = ((((r5 * g + r4) * g + r3) * g + r2) * g + r1) * g */
                
                rg = multr_fr1x32x32 (g, coeffs_r[0]);
                for (k = 1; k < 5; k++)
                { 
                    rg = L_add (rg, coeffs_r[k]);
                    rg = multr_fr1x32x32 (rg, g);
                }

                result += (result * raw64_to_float ((long long) rg, 31UL));
            }

            if ((x < 0.0F) ^ (n & (long) 0x1))
            {
                result = -result; 
            }  
        }
    }
 
    return  result;

}
/* End of File */

/************************************************************************
 *
 * sin_fr32.c : $Revision: 1.5 $
 *
 * (c) Copyright 2007-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: sin_fr32 - sine function

    Synopsis:

        #include <math.h>
        fract32 sin_fr32 (fract32 x);

    Description:

        The sin_fr32 function returns the sine of the argument. The sin_fr32
        function inputs a fractional value in the range [-1.0, 1.0)
        corresponding to [-p/2, p/2]. The domain represents half a cycle which
        can be used to derive a full cycle if required. The result, in radians,
        is in the range [-1.0, 1.0).

    Error Conditions:

        The sin_fr32 function does not return an error condition.

    Algorithm:

        The general form of a n-th degree Polynomial Algorithm is as follows:

           Sin(x) = ( ( ( (p0 * x + p1) * x + p2) * x + p3) * x + ...) * x + pn

        When approximating the sine function, p1, p3 etc. are 0. Thus:

           Sin(x) = ( ( (p0 * x^2 + p2) * x^2 + ...) * x^2 + pm) * x

        With pm close to pi/2, which exceeds fractional domain, this becomes:

           Sin(x) = ( (p0 * x^2 + p2) * x^2 + ...) * x^3 + pm/2 * x + pm/2 * x

        For the above formula, the coefficients p0 .. pm are:

           p0  = 0x00000075, p2  = 0xffffe1d7, p4  = 0x000541db,
           p6  = 0xff66969c, p8  = 0x0a335e33, p10 = 0xad510c67,
           p12 = 0x6487ed51

           Based on:
           t=[-1:2^-20:(1-(2^-31))];
           [pcoeff,perr]=polyfit(t,sin((pi/2)*t),13);

        To improve the accuracy of the function, the first 5 coefficients
        (p0 to p8) are scaled by 4:

           Sin(x) = ( ( ( ( (p0 * 4 * x^2 + p2 * 4) * x^2 + ...) * x^2) / 4
                    + p10) * x^3 + pm/2 * x + pm/2 * x

        The new set of coefficients is:

           p0  = 0x000001d4, p2  = 0xffff875d, p4  = 0x0015076d,
           p6  = 0xfd9a5a6f, p8  = 0x28cd78cd, p10 = 0xad510c67,
           p12 = 0x6487ed51

    Implementation:

    Example:

        #include <math.h>

        fract32 x;
        fract32 y;

        y = sin_fr32 (x);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("liGroup  =math_bf.h")
#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =sin_fr32")
#pragma file_attr("libFunc  =__sin_fr32")

/* Called by polar_fr32 */
#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =polar_fr32")
#pragma file_attr("libFunc  =__polar_fr32")

/* Called by twidfft_fr32 */
#pragma file_attr("libGroup =filter.h")
#pragma file_attr("libFunc  =twidfft_fr32")
#pragma file_attr("libFunc  =__twidfft_fr32")

/* Called by twidfft2d_fr32 */
#pragma file_attr("libFunc  =twidfft2d_fr32")
#pragma file_attr("libFunc  =__twidfft2d_fr32")

/* Called by twidfftf_fr32 */
#pragma file_attr("libFunc  =twidfftf_fr32")
#pragma file_attr("libFunc  =__twidfftf_fr32")

/* Called by twidfftrad2_fr32 */
#pragma file_attr("libFunc  =twidfftrad2_fr32")
#pragma file_attr("libFunc  =__twidfftrad2_fr32")

/* Called by twidfftrad4_fr32 */
#pragma file_attr("libFunc  =twidfftrad4_fr32")
#pragma file_attr("libFunc  =__twidfftrad4_fr32")

#pragma file_attr("libName  =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

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
#endif /* MISRA RULES */

#if !defined(ETSI_SOURCE)
#define ETSI_SOURCE
#endif /* ETSI_SOURCE */

#include <math.h>
#include <fract.h>
#include <builtins.h>
#include <libetsi.h>

#define SIN_ENTRY 7

extern fract32
sin_fr32 ( fract32 _x )
{
    static const fract32 sin_coeff[SIN_ENTRY] =
    {
        (fract32) 0x000001d4, (fract32) 0xffff875d, (fract32) 0x0015076d,
        (fract32) 0xfd9a5a6f, (fract32) 0x28cd78cd, (fract32) 0xad510c67,
        (fract32) 0x6487ed51
    };

    fract32 x = _x;
    fract32 x_square, poly_prod, poly_sum, temp_prod, sin_val;

    int i;

    /* computing x square */
    x_square = multr_fr1x32x32 (x, x); /* computing x square */

    /* p0 * x^2 + p2 */
    poly_prod = multr_fr1x32x32 (sin_coeff[0], x_square);
    poly_sum  = L_add (poly_prod, sin_coeff[1]);

    /* (((p0 * x^2 + p2) * x^2 + p4) * x^2 + p6) * x^2 + p8 */
    for (i = 2; i < 5 ; i++)
    {
        poly_prod = multr_fr1x32x32 (poly_sum, x_square);
        poly_sum  = L_add (poly_prod, sin_coeff[i]);
    }

    /* apply rounding */
    poly_sum  = L_add (poly_sum, (fract32) 0x00000002);
    poly_sum  = L_shr (poly_sum, 2);

    /* ((((p0 * x^2 + p2) * x^2 + p4) * x^2 + p6) * x^2 + p8) * x^2 + p10 */
    poly_prod = multr_fr1x32x32 (x_square, poly_sum);
    poly_sum  = L_add (poly_prod, sin_coeff[5]);

    /*
       (((((p0*x^2 + p2)*x^2 + p4)*x^2 + p6)*x^2 + p8)*x^2 + p10)*x^3 +
                                                      p12/2*x + p12/2*x
    */
    poly_prod = multr_fr1x32x32 (poly_sum, x_square);
    poly_prod = multr_fr1x32x32 (poly_prod, x);

    temp_prod = multr_fr1x32x32 (sin_coeff[6], x);

    poly_sum  = L_add (poly_prod, temp_prod);
    sin_val   = L_add (poly_sum, temp_prod);

    return sin_val;

}

/* End of File */

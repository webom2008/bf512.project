/************************************************************************
 *
 * cos_fr32.c : $Revision: 1.7 $
 *
 * (c) Copyright 2007-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: cos_fr32 - cosine function

    Synopsis:

        #include <math.h>
        fract32 cos_fr32 (fract32 x);

    Description:

        The cos_fr32 function returns the cosine of the argument. The
        cos_fr32 function inputs a fractional value in the range [-1.0, 1.0)
        corresponding to [-pi/2, pi/2]. The domain represents half a cycle
        which can be used to derive a full cycle if required. The result, in
        radians, is in the range [-1.0, 1.0).

    Error Conditions:

        The cos_fr32 function does not return an error condition.

    Algorithm:

        cos(x) = -[ sin(-pi/2 + x) ]

        where the sine function is defined by an n-th degree Polynomial
        Algorithm is as follows:

           sin(x) = ( ( ( (p0 * x + p1) * x + p2) * x + p3) * x + ...) * x + pn

        When approximating the sine function, p1, p3 etc. are 0. Thus:

           sin(x) = ( ( (p0 * x^2 + p2) * x^2 + ...) * x^2 + pm) * x

        With pm close to pi/2, which exceeds fractional domain, this becomes:

           sin(x) = ( (p0 * x^2 + p2) * x^2 + ...) * x^3 + pm/2 * x + pm/2 * x

        For the above formula, the coefficients p0 .. pm are:

           p0  = 0x00000075, p2  = 0xffffe1d7, p4  = 0x000541db,
           p6  = 0xff66969c, p8  = 0x0a335e33, p10 = 0xad510c67,
           p12 = 0x6487ed51

           Based on:
           t=[-1:2^-20:(1-(2^-31))];
           [pcoeff, perr]=polyfit(t, sin((pi/2)*t), 13);

        To improve the accuracy of the function, the first 5 coefficients
        (p0 to p8) are scaled by 4:

           sin(x) = ( ( ( ( (p0 * 4 * x^2 + p2 * 4) * x^2 + ...) * x^2) / 4
                      + p10) * x^3 + pm/2 * x + pm/2 * x

        The new set of coefficients is:

           p0  = 0x000001d4, p2  = 0xffff875d, p4  = 0x0015076d,
           p6  = 0xfd9a5a6f, p8  = 0x28cd78cd, p10 = 0xad510c67,
           p12 = 0x6487ed51

        For any input value that is (more or less) less than 2^16,
        cosine should generate 0x7fffffff.

    Implementation:

        if (x == -1.0)
        {
            cos(x) = 0;
        }
        else if ( x <= 0x0000c000 )
        {
            cos(x) = 0x7fffffff
        }
        else
        {
            cos(x) = -[ sin(-pi/2 + x) ]
        }

    Example:

        #include <math.h>

        fract32 x;
        fract32 y;
        y = cos_fr32 (x);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("liGroup  =math_bf.h")
#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =cos_fr32")
#pragma file_attr("libFunc  =__cos_fr32")

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

/* Called by acos_fr32 */
#pragma file_attr("libFunc  =acos_fr16")
#pragma file_attr("libFunc  =__acos_fr16")

/* Called by gen_blackman_fr32 */
#pragma file_attr("libGroup =window.h")
#pragma file_attr("libFunc  =gen_blackman_fr32")
#pragma file_attr("libFunc  =__gen_blackman_fr32")

/* Called by gen_hamming_fr32 */
#pragma file_attr("libFunc  =gen_hamming_fr32")
#pragma file_attr("libFunc  =__gen_hamming_fr32")

/* Called by gen_hanning_fr32 */
#pragma file_attr("libFunc  =gen_hanning_fr32")
#pragma file_attr("libFunc  =__gen_hanning_fr32")

/* Called by gen_harris_fr32 */
#pragma file_attr("libFunc  =gen_harris_fr32")
#pragma file_attr("libFunc  =__gen_harris_fr32")

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
#endif /* _MISRA_RULES */

#if !defined(ETSI_SOURCE)
#define ETSI_SOURCE
#endif /* ETSI_SOURCE */

#include <math.h>
#include <builtins.h>
#include <libetsi.h>

#define N_COS_COEFFS 7

extern fract32
cos_fr32 (fract32 _x)
{
    static const fract32 cos_coeff[N_COS_COEFFS] =
    {
        (fract32) 0x000001d4, (fract32) 0xffff875d, (fract32) 0x0015076d,
        (fract32) 0xfd9a5a6f, (fract32) 0x28cd78cd, (fract32) 0xad510c67,
        (fract32) 0x6487ed51
    };

    fract32 x = ((_x < 0x0) ? L_negate(_x) : _x);
    fract32 x_signed = _x;
    fract32 x_square, poly_prod, poly_sum, temp_prod, cos_val;
    int i;

    if (x_signed == (fract32)0x80000000)
    {
        cos_val = 0x0;
    }
    else if (x <= 0x0000c000)
    {
        cos_val = 0x7fffffff;
    }
    else
    {
        /* Computing (-p/2 + x) or (-p/2 - x) */
        x = L_add((fract32)0x80000000, x);

        /* Computing x square */
        x_square = multr_fr1x32x32( x, x );

        /* p0 * x^2 + p2 */
        poly_prod = multr_fr1x32x32( cos_coeff[0], x_square );
        poly_sum  = L_add( poly_prod, cos_coeff[1] );

        /* (((p0 * x^2 + p2) * x^2 + p4) * x^2 + p6) * x^2 + p8 */
        for (i = 2; i < 5; i++)
        {
            poly_prod = multr_fr1x32x32( poly_sum, x_square );
            poly_sum  = L_add( poly_prod, cos_coeff[i] );
        }

        /* Apply rounding */
        poly_sum  = L_add( poly_sum, (fract32) 0x00000002 );
        poly_sum  = L_shr( poly_sum, 2 );

        /*
           ((((p0 * x^2 + p2) * x^2 + p4) * x^2 + p6) * x^2 + p8) * x^2 + p10
        */
        poly_prod = multr_fr1x32x32( x_square, poly_sum );
        poly_sum  = L_add( poly_prod, cos_coeff[5] );

        /*
           (((((p0*x^2 + p2)*x^2 + p4)*x^2 + p6)*x^2 + p8)*x^2 + p10)*x^3 +
                                                          p12/2*x + p12/2*x
        */
        poly_prod = multr_fr1x32x32( poly_sum, x_square);
        poly_prod = multr_fr1x32x32( poly_prod, x );

        temp_prod = multr_fr1x32x32( cos_coeff[6], x );

        poly_sum  = L_add( poly_prod, temp_prod );
        cos_val   = L_add( poly_sum, temp_prod );

        /* Negating the final result to obtain cos(x) */
        cos_val = L_negate(cos_val);
    }

    return cos_val;
}

/* End of File */

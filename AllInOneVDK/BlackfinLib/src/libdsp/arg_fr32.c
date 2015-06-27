/****************************************************************************
 *
 * arg_fr32.c : $Revision: 1.4.6.1 $
 *
 * (c) Copyright 2010 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: arg_fr32 - Get phase of a complex number

    Synopsis:

        #include <complex.h>
        fract32 arg_fr32 (complex_fract32 a);

    Description:

        The arg_fr32 function computes the phase associated with a Cartesian
        number, represented by the complex argument a, and returns the result
        (in radians, scaled by 2 * pi).

    Error Conditions:

        The arg_fr32 function does not return an error condition.

    Algorithm:

        phase = atan( a.im / a.re )
              = atan2( a.im, a.re )

    Implementation:

        Makes use of the following optimizing pragma:

          #pragma optimize_for_speed

    Example:

        #include <complex.h>

        complex_fract32  a;
        fract32          phase;

        a.re = 0x80000000;
        a.im = 0x7fffffff;

        phase = arg_fr32 (a);   /* phase = 0x30000000 = 0.375 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =arg_fr32")
#pragma file_attr("libFunc  =_arg_fr32")
#pragma file_attr("libName  =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#endif

#if !defined(TRACE)
#pragma optimize_for_speed
#endif

#if !defined(ETSI_SOURCE)
#define ETSI_SOURCE
#endif /* ETSI_SOURCE */

#include <complex.h>
#include <math.h>
#include <libetsi.h>

extern fract32 arg_fr32(complex_fract32 _a)
{
    complex_fract32  a = _a;
    fract32          phase;

    phase = atan2_fr32(a.im, a.re);

    /* atan2_fr32( [0..0.5) ) = [0x0..0x7fffffff]
    ** Expected phase :         [0..0.5)
    ** => Need to divide output from atan2_fr32 by 2 to scale
    */

    phase = shr_fr1x32(phase, 1);

    if (phase < 0)
    {
        /* atan2_fr32( [0.5..1.0) ) = [0x80000000..0x0)
        ** Expected phase :           [0.5..1.0)
        ** => Need to scale by:       (2.0 + output atan2_fr32) / 2
        **                          =  1.0 + output/2
        **                          =  0.5 + 0.5 + output/2
        */

        phase = L_add( phase, (fract32) 0x40000000 );
        phase = L_add( phase, (fract32) 0x40000000 );

    }

    return( phase );
}

/* End of file */

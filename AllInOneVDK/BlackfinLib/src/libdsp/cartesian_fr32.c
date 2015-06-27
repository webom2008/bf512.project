/**************************************************************************** 
 * 
 * cartesian_fr32.c : $Revision: 1.2 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: cartesian_fr32 - Convert Cartesian to polar notation

    Synopsis:

       #include <complex.h>
       fract32 cartesian_fr32 (complex_fract32 a, fract32* phase);

    Description:

        The cartesian_fr32 function transforms a complex number from Cartesian
        notation to polar notation. The Cartesian number is represented by the
        argument a that the function converts into a corresponding magnitude,
        which it returns as the functions result, and a phase that is returned
        via the second argument phase.

    Error Conditions:

        The cartesian_fr32 function does not return an error condition.

    Implementation:

        magnitude = sqrt( a.re^2 + a.im^2 )  /* length of a vector */ 
                  = cabs( a )

        phase     = tan^-1( a.im / a.re )    /* angle of a vector */ 
                  = atan( a.im / a.re ) 
                  = arg( a )

    Example:

        #include <complex.h>

        complex_fract32  cart;
        fract32          magnitude, phase;

        cart.re = 0x10000000;   /* 0.125             */
        cart.im = 0x1bb67ae8;   /* 0.216506350785494 */

        magnitude = cartesian_fr32 ( cart, &phase);

        /* magnitude = 0x02000000;  (0.25) */
        /* phase     = 0x15555555;  (pi/3 = 60 deg. (scaled)) */


*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =cartesian_fr32")
#pragma file_attr("libFunc  =_cartesian_fr32")
#pragma file_attr("libName  =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#endif

#if !defined(TRACE)
#pragma optimize_for_space
#endif

#include <complex.h>


extern
fract32 cartesian_fr32 (complex_fract32 _a, fract32* _phase)
{
     fract32 magnitude;

     magnitude = cabs_fr32(_a);
     *_phase = arg_fr32(_a);
 
     return magnitude;
}

/* End of File */

/**************************************************************************** 
 * 
 * polar_fr32.c : $Revision: 1.4 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: polar_fr32 - Convert polar to Cartesian notation

    Synopsis:

       #include <complex.h>
       complex_fract32 polar_fr32 (fract32 magnitude, fract32 phase);

    Description:

       The polar function transforms the polar coordinate, specified by the
       arguments magnitude and phase, into a Cartesian coordinate and returns 
       the result as a complex number in which the x-axis is represented by the 
       real part, and the y-axis by the imaginary part.

       The phase argument is interpreted as radians. It must be scaled by 2PI 
       and must be in the range [0x80000000, 0x7fffffff]. The value of the 
       phase may be either positive or negative.

       Positive values are interpreted as an anti-clockwise motion around a 
       circle with a radius equal to the magnitude, and negative numbers are 
       interpreted as a clockwise motion.

    Error Conditions:

       The polar_fr32 function does not return an error condition.

    Implementation:

       Initially convert the phase to positive if it is negative, with the 
       clockwise positive mapping on to the anti-clockwise negate phase as 
       follows:

           Phase:  -1.0  == 0.0
                   -0.75 == 0.25
                   -0.5  == 0.5
                   -0.25 == 0.75

           if (phase < 0) 
               phase += 1.0

       Check for early exit case of phase being zero:

           if (phase == 0)
               return Re(a)=Mag, Im(a)=0

       The input range for sin_fr32 and cos_fr32 is [0..1.0) from -pi/2..pi/2,
       with the input range for each of the quadrants being 0.25. To obtain the
       full range we can move the phase into [-0.25..0.25) range and then scale
       to use the full range by multiplying by 4:

           phase=[0.00..0.25) => phase = phase * 4
           phase=[0.25..0.50) => phase = (-0.5 + phase) * 4
           phase=[0.50..0.75) => phase = (phase - 0.5) * 4
           phase=[0.75..1.00) => phase = (-1.0+phase) * 4
 
           if (0 < phase < 1/4)
               phase *= 4
           else if (1/4 <= phase < 3/4)
               phase = (phase - 0.5) * 4, Mag = -Mag
           else
               phase = (phase - 1) * 4

           Re(a) = Mag * cos_fr32(phase)
           Im(a) = Mag * sin_fr32(phase)

    Example:

        #include <complex.h>

        complex_fract32  cart;
        fract32          magnitude = 0x02000000;  /* 0.25 */
        fract32          phase     = 0x15555555;  /* pi/3 = 60 deg. (scaled) */

        cart = polar_fr32 ( magnitude, phase);
        /* cart.re = 0x10000000 */
        /* cart.im = 0x1bb67ae8 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =polar_fr32")
#pragma file_attr("libFunc  =_polar_fr32")
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

#include <libetsi.h>
#include <math.h>
#include <complex.h>


extern
complex_fract32 polar_fr32(fract32 _magnitude, fract32 _phase)
{
     fract32         magnitude = _magnitude;
     complex_fract32 result;
     fract32         phase = (_phase < 0) ? L_sub(_phase, (fract32)0x80000000)
                                          : _phase;

     /* For a phase or zero we can avoid any further calculations. Note
     ** that we have already ensured that phase is positive, so an input
     ** phase of -1.0 will also map to zero.
     */

     result.re = magnitude; 
     result.im = 0; 

     if (expected_true(phase != 0)) {
         /* convert phase into [-0.25..0.25) range. */
         if (phase < 0x20000000) {
             /* Phase is 0..pi/2; just multiply by scale. */

             phase *= 4;

         } else if (phase < 0x60000000) {
             /* Phase is pi..3/2pi; add 0.5 and multiply by scale. */

             phase = (phase + (fract32)0xc0000000) * 4; 
             magnitude = L_negate(magnitude);

         } else {
             /* Phase is 3/2pi..pi; add -1.0 and multiply by scale. */

              phase = (phase + (fract32)0x80000000) * 4;
         }

         result.re = multr_fr1x32x32(magnitude, cos_fr32(phase));
         result.im = multr_fr1x32x32(magnitude, sin_fr32(phase));
     }

     return result;
}

/* End of File */

/*************************************************************************
 *
 * Lib_cos32_2PIx.c : $Revision: 1.7 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: cos32_2PIx - Fract32 Cosine

    Synopsis:

        fract32 __cos32_2PIx(long long x);

    Description:

        The function has been designed specifically for the case:

           float   pi = 3.14...;
           fract32 x32;
           ...
           for( i=0; i<n; i++)
             x32 = float_to_fr32(cosf(2*pi*(i/n)));
           ...

        The expression can now be rewriten as:

           x32 = __cos32_2PIx( i/n )

    Algorithm:

        Convert to be in range [0..1), one single period 

        The fractional cosine function is limited in their input and output
        range:

        cos_fr32():
              0x80000000 (=-pi/2) = 0x00000000,
              0x00000000 (=0)     = 0x7fffffff,
              0x7fffffff (=pi/2)  = 0x00000000

        To compute the cosine across the entire range of the phase, it is
        necessary to modify the phase. This can be done by taking advantage
        of the symmetrical nature of the cosine function:

          Q1: [  0   .. 1/2pi) :  cos_fr32(x), x=[0x0..0x7fffffff)
          Q2: [1/2pi ..    pi) : -cos_fr32(x), x=[0x80000000..0x0)
          Q3: [   pi .. 3/2pi) : -cos_fr32(x), x=[0x0..0x7fffffff)
          Q4: [3/2pi ..   2pi) :  cos_fr32(x), x=[0x80000000..0x0)

        To match the phase [0..1.0) to x requires at most two transformations.
        Firstly, the range for the phase in each quarter Q1 to Q4 is 0.25
        while the range for x is 1.0. Therefore the phase has to be multiplied
        by four. For example:

          Q1: x=[0x0..0x7fffffff), phase=[0..0.25)    => phase_m = phase * 4

        Secondly for Q2, Q3 and Q4 it is also necessary to modify the phase
        in such a way that it falls into the desired input range for x:

         Q2: x=[0x80000000..0x0), 
                          phase=[0.25..0.50) => phase_m = (-0.5 + phase) * 4
         Q3: x=[0x0..0x7fffffff), 
                          phase=[0.50..0.75) => phase_m = (phase - 0.5) * 4
         Q4: x=[0x80000000..0x0), 
                          phase=[0.75..1.00) => phase_m = (-1.0 + phase) * 4


*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libFunc  =__cos32_2PIx")
#pragma file_attr("libFunc  =___cos32_2PIx")
/* called by gen_blackman_fr32 */
#pragma file_attr("libGroup =window.h")
#pragma file_attr("libFunc  =gen_blackman_fr32")
#pragma file_attr("libFunc  =__gen_blackman_fr32")
/* called by gen_hamming_fr32 */
#pragma file_attr("libFunc  =gen_hamming_fr32")
#pragma file_attr("libFunc  =__gen_hamming_fr32")
/* called by gen_hanning_fr32 */
#pragma file_attr("libFunc  =gen_hanning_fr32")
#pragma file_attr("libFunc  =__gen_hanning_fr32")
/* called by gen_harris_fr32 */
#pragma file_attr("libFunc  =gen_harris_fr32")
#pragma file_attr("libFunc  =__gen_harris_fr32")
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =any")
#pragma file_attr("prefersMemNum =50")

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
#endif

#if !defined(ETSI_SOURCE)
#define ETSI_SOURCE
#endif /* ETSI_SOURCE */

#include <math.h>
#include <ccblkfn.h>
#include <fract_typedef.h>
#include <libetsi.h>
#include "xutil_fr.h"


fract32 __cos32_2PIx(long long _x)
{
     fract32  res32;
     unsigned long long abs_val;
     
     /* get absolute value */
     if (_x < 0) {
         _x = -_x;
     }

     /* reduce the input to fit into one period and convert to fract32 */
     abs_val = (unsigned long long)_x;
     abs_val = (abs_val&0x7fffffffULL);
     res32 = (fract32)abs_val;

     if (expected_false(res32 < 0x20000000))
     {
         /* input is 0..pi/2; just multiply by scale. */

         res32 *= 4;
         res32 = cos_fr32(res32);
     }
     else if (expected_true(res32 < 0x60000000))
     {
         /* input is pi..3/2pi; add -0.5 and multiply by scale. */

         res32 = (res32 + (fract32)0xc0000000) * 4;
         res32 = -cos_fr32(res32);
     }
     else
     {
         /* input is 3/2pi..pi; add -1.0 and multiply by scale. */

         res32 = (res32 + (fract32)0x80000000) * 4;
         res32 = cos_fr32(res32);
     }

     return res32;
}


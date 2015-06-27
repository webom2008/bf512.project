/****************************************************************************
 *
 * csub_fr32.c : $Revision: 1.2 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: csub_fr32 - complex subtraction

    Synopsis:

        #include <complex.h>
        complex_fract32 csub_fr32 (complex_fract32 a, complex_fract32 b);

    Description:

        The csub_fr32 function computes the difference between two complex
        fract32 inputs, a and b.

    Error Conditions:

        The csub_fr32 function returns no error conditions.

    Algorithm:

        Re(c) = Re(a) - Re(b)
        Im(c) = Im(a) - Im(b)

    Implementation:

        Makes use of the following optimizing pragma:

          #pragma optimize_for_space

    Example:

        #include <complex.h>

        complex_fract32 a = {0x20000000, 0x60000000}; /* 0.25, 0.75 */
        complex_fract32 b = {0x40000000, 0x40000000}; /* 0.5 , 0.5  */

        complex_fract32 res = csub_fr32(a, b);

        /* res = 0xE0000000, 0x20000000 = -0.25, 0.25*/

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =csub_fr32")
#pragma file_attr("libFunc  =_csub_fr32")
#pragma file_attr("libName  =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#endif

#if !defined(TRACE)
#pragma optimize_for_space
#endif

#if !defined(__NO_BUILTIN)
# define __NO_BUILTIN
#endif

#include <complex.h>


extern complex_fract32 
csub_fr32(complex_fract32 _a, complex_fract32 _b)
{
   _composite_complex_fract32 c, d;

   _CCFR32_CPLX(c) = _a;
   _CCFR32_CPLX(d) = _b;
   _CCFR32_RAW(c) = __builtin_csub_fr32(_CCFR32_RAW(c), _CCFR32_RAW(d));
   return _CCFR32_CPLX(c);
}

/* End of file */

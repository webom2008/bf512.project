/****************************************************************************
 *
 * copysign_fr32.c : $Revision: 1.3 $
 *
 * (c) Copyright 2010 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: copysign_fr32 - Copysign

    Synopsis:

        #include <math.h>
        fract32 copysign_fr32 (fract32 x, fract32 y);

    Description:

        The copysign_fr32 function copies the sign of the second argument
        to the first argument.

    Error Conditions:

        The copysign_fr32 function does not return an error condition.

    Algorithm:

        return (|parm1| * copysignof(parm2))

    Example:

        #include <math.h>

        /* a = copysign_fr32(0.5, -0.75) */
        fract32 a = copysign_fr32(0x40000000, 0xA0000000);

        /* a = 0xC0000000 = -0.5 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =copysign_fr32")
#pragma file_attr("libFunc  =_copysign_fr32")
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

#include <math.h>
#include <libetsi.h>


extern fract32 copysign_fr32(fract32 _x, fract32 _y)
{
   fract32 result = abs_fr32(_x);
   if (_y < 0) {
      if (_x > 0) {
           result = L_negate(_x);
      } else {
           result = _x;
      }
   }
   return result;
}

/* End of file */

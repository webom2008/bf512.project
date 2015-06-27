/****************************************************************************
 *
 * min_fr32.c : $Revision: 1.3 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: min_fr32 - minimum

    Synopsis:

        #include <math.h>
        fract32 min_fr32 (fract32 _x, fract32 _y);

    Description:

        The min_fr32 function returns the smaller of the input arguments _x
        and _y.

    Error Conditions:

        The min_fr32 function does not return an error condition.

    Algorithm:

        if (_x > _y)
           return _y
        else
           return _x

    Implementation:

        Makes use of the following optimizing pragma:

          #pragma optimize_for_speed

    Example:

        #include <math.h>

        fract32 min = min_fr32(0xA0000000, 0x60000000);  /* (-0.75, 0.75) */

        /* min = 0xA0000000 = -0.75 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =min_fr32")
#pragma file_attr("libFunc  =_min_fr32")
#pragma file_attr("libName  =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#endif

#if !defined(TRACE)
#pragma optimize_for_speed
#endif

#if !defined(__NO_BUILTIN)
#define __NO_BUILTIN
#endif

#include <math.h>


extern fract32 
min_fr32(fract32 _x, fract32 _y)
{
   return  __builtin_min_fr1x32(_x, _y);
}

/* End of file */

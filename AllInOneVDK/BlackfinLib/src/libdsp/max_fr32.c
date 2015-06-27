/****************************************************************************
 *
 * max_fr32.c : $Revision: 1.3 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: max_fr32 - maximum

    Synopsis:

        #include <math.h>
        fract32 max_fr32 (fract32 _x, fract32 _y);

    Description:

        The max_fr32 function returns the larger of the input arguments _x
        and _y.

    Error Conditions:

        The max_fr32 function does not return an error condition.

    Algorithm:

        if (_x >= _y)
           return _x
        else
           return _y

    Implementation:

        Makes use of the following optimizing pragma:

          #pragma optimize_for_speed

    Example:

        #include <math.h>

        fract32 max = max_fr32(0xA0000000, 0x60000000);  /* (-0.75, 0.75) */

        /* max = 0x60000000 = 0.75 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =max_fr32")
#pragma file_attr("libFunc  =_max_fr32")
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
max_fr32(fract32 _x, fract32 _y)
{
   return  __builtin_max_fr1x32(_x, _y);
}

/* End of file */

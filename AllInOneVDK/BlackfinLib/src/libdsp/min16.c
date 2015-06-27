/****************************************************************************
 *
 * min16.c : $Revision: 3543 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: min_fr16 - minimum

    Synopsis:

        #include <math.h>
        fract16 min_fr16 (fract16 _x, fract16 _y);

    Description:

        The min_fr16 function returns the smaller of the input arguments _x
        and _y.

    Error Conditions:

        The min_fr16 function does not return an error condition.

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

        fract16 min = min_fr16(0xA000, 0x6000);  /* (-0.75, 0.75) */

        /* min = 0xA000 = -0.75 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =fract.h")
#pragma file_attr("libGroup =fract_math.h")
#pragma file_attr("libGroup =math_bf.h")
#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =__fmin_fr16")
#pragma file_attr("libFunc  =min_fr16")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")
#endif

#if !defined(TRACE)
#pragma optimize_for_speed
#endif

#if !defined(__NO_BUILTIN)
# define __NO_BUILTIN
#endif

#include <math.h>


extern fract16 min_fr16(fract16 _x, fract16 _y)
{
   return  (fract16)__builtin_min_fr2x16((int)_x, (int)_y);
}

/* End of file */

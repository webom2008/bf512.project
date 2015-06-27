/****************************************************************************
 *
 * abs_fr32.c : $Revision: 1.3 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: abs_fr32 - absolute value

    Synopsis:

        #include <math.h>
        fract32 abs_fr32 (fract32 _x);

    Description:

        The abs_fr32 function computes the absolute value of its input _x.

    Error Conditions:

        The abs_fr32 function does not return an error condition.

    Algorithm:

        if (_x >= 0)
             abs_val = _x
        else if (_x == 0x80000000)
                 abs_val = 0x7fffffff
             else
                 abs_val = -_x

    Implementation:

        Makes use of the following optimizing pragma:

          #pragma optimize_for_speed

    Example:

        #include <math.h>

        fract32 a = abs_fr32(0xA0000000);   /* a = abs_fr32(-0.75) */

        /* a = 0x60000000 = 0.75 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =abs_fr32")
#pragma file_attr("libFunc  =_abs_fr32")
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
abs_fr32(fract32 _x)
{
   return  __builtin_abs_fr1x32(_x);
}

/* End of file */

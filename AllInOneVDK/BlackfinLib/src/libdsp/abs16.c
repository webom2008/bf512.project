/****************************************************************************
 *
 * abs16.c : $Revision: 3543 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: abs_fr16 - absolute value

    Synopsis:

        #include <math.h>
        fract16 abs_fr16 (fract16 _x);

    Description:

        The abs_fr16 function computes the absolute value of its input _x.

    Error Conditions:

        The abs_fr16 function does not return an error condition.

    Algorithm:

        if (_x >= 0)
             abs_val = _x
        else if (_x == 0x8000)
                 abs_val = 0x7fff
             else
                 abs_val = -_x

    Implementation:

        Makes use of the following optimizing pragma:

          #pragma optimize_for_speed

    Example:

        #include <math.h>

        fract16 a = abs_fr16(0xA000);   /* a = abs_fr16(-0.75) */

        /* a = 0x6000 = 0.75 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =fract.h")
#pragma file_attr("libGroup =fract_math.h")
#pragma file_attr("libGroup =math_bf.h")
#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =__abs_fr16")
#pragma file_attr("libFunc  =abs_fr16")

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


extern fract16 abs_fr16(fract16 _x)
{
   return  (fract16)__builtin_abs_fr2x16((int)_x);
}

/* End of file */

/****************************************************************************
 *
 * clip.c : $Revision: 1.2 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: clip - Clip

    Synopsis:

        #include <math.h>

        int clip (int value, int limit);
        long int lclip (long int value, long int limit);

        fract32 clip_fr32 (fract32 value, fract32 limit);

    Description:

        The clip functions return the first argument value if the absolute 
        value of value is less than the absolute value of the second
        argument limit; otherwise, they return the absolute value of limit if
        value is positive, or minus the absolute value of limit if value is
        negative.

    Error Conditions:

        The clip functions do not return an error condition.

    Algorithm:

        if (|value| < |limit|)
           clip = value
        else
           if (value < 0)
              clip = -(|limit|)
           else
              clip = |limit|

    Implementation:

        The same implementation is used by each of the clip, lclip and 
        clip_fr32 functions. The math.h include file utilizes the 
        linkage_name pragma to instruct the clip and clip_fr32 prototypes 
        to call the clip function implementation.

        Makes use of the following optimizing pragma:

          #pragma optimize_for_speed

    Example:

        #include <math.h>

        int     a;
        fract32 f;

        a = clip(256, 512);

        /* a = 256 */

        f = clip_fr32(0xA0000000, 0x20000000);   /* (-0.75, 0.25) */

        /* f = 0xE0000000 = -0.25 */

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =clip")
#pragma file_attr("libFunc  =_clip")
#pragma file_attr("libFunc  =lclip")
#pragma file_attr("libFunc  =_lclip")
#pragma file_attr("libFunc  =clip_fr32")
#pragma file_attr("libFunc  =_clip_fr32")
#pragma file_attr("libName  =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

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
#endif /* _MISRA_RULES */

#include <math.h>
#include <stdlib.h>

extern int
clip (int _value, int _limit)
{
     int value = _value;
     int limit = _limit;

     int abs_limit = abs (limit);
     int ret_value = value;

     /* If limit = -1 => return value always */
     if (expected_true(limit != (int) 0x80000000))
     {
          /* Lower boundary
          ** If (value < -|limit|) need to return -|limit|,
          **                       otherwise continue 
          */      
          value = max (value, -abs_limit);
 
          /* Upper boundary
          ** If (value > |limit|)  if value == -|limit| => return -|limit|
          **                       else if -|limit|<value<|limit| => return 
          **                       value else return |limit|
          */ 
          ret_value = min (value, abs_limit);
     }

     return ret_value;
}

/* End of File */

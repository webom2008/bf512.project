/************************************************************************
 *
 * vecmaxloc_fr32.c : $Revision: 1.2 $
 *
 * (c) Copyright 2007 Analog Devices, Inc.  All rights reserved.
 *
 ***********************************************************************/

#if defined(__DOCUMENTATION__)

    Function: vecmaxloc_fr32 - vector maximum location

    Synopsis:

        #include <vector.h>
        int vecmaxloc_fr32 (const fract32 vector[],
                            int           length);

    Description:

        The vecmaxloc_fr32 function returns the location of the largest
        value in the given vector, vector[]. The parameter length defines
        the size of the input vector.

    Error Conditions:

        The vecmaxloc_fr32 function is an effective no-op if the parameter
        length is less than or equal to zero.

    Algorithm:

        max_val = vector[0];
        for(i = 1; i < length; i++)
        {
            if (vector[i] > max_val)
            {
                max_val = vector[i];
                max_loc = i;
            }
        }

    Implementation:

        Makes use of the following optimizing pragmas:

          #pragma extra_loop_loads
          #pragma vector_for

    Example:

        #include <vector.h>

        #define SAMPLES_N 50

        fract32 input[SAMPLES_N];
        int max_loc;

        max_loc = vecmaxloc_fr32 (input,SAMPLES_N);

*************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =vecmaxloc_fr32")
#pragma file_attr("libFunc  =__vecmaxloc_fr32")
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

#include <vector.h>

extern int
vecmaxloc_fr32 (const fract32 _vector[],
                int           _length)
{
    const fract32 *const vector = _vector;
    const int            length = _length;

    fract32 max_val;
    int max_loc = 0;
    int i;

    max_val = vector[0];

#pragma extra_loop_loads
#pragma vector_for

    for(i = 1; i < length; i++)
    {
        if (vector[i] > max_val)
        {
            max_val = vector[i];
            max_loc = i;
        }
    }

    return max_loc;
}

/* End of File */

/************************************************************************
 *
 * vecmin_fr32.c : $Revision: 1.4 $
 *
 * (c) Copyright 2007 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: vecmin_fr32 - vector minimum

    Synopsis:

        #include <vector.h>
        fract32 vecmin_fr32 (const fract32 vector[],
                             int           length);

    Description:

        The vecmin_fr32 function returns the smallest value in the
        given vector, vector[]. The parameter length defines the size
        of the input vector.

    Error Conditions:

        The vecmin_fr32 function is an effective no-op if the parameter
        length is less than or equal to zero.

    Algorithm:

        vecmin_fr32(vector,length) = minimum (vector[0],..,vector[length-1])

    Implementation:

        Makes use of the following optimizing pragmas:

          #pragma extra_loop_loads
          #pragma vector_for

    Example:

        #include <vector.h>

        #define SAMPLES_N 50

        fract32 input[SAMPLES_N], min_val;

        min_val = vecmin_fr32 (input,SAMPLES_N);

*************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =vecmin_fr32")
#pragma file_attr("libFunc  =__vecmin_fr32")
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

#pragma diag(suppress:misra_rule_14_7)
/* Suppress Rule 14.7 (required) which requires that a function shall
** have a single point of exit at the end of the function.
**
** Rationale: The requirement is only suppressed provided that the only
** violation occurs at the beginning of the function when the function
** verifies its parameters are "correct".
*/
#endif /* _MISRA_RULES */

#if !defined(ETSI_SOURCE)
#define ETSI_SOURCE
#endif /* ETSI_SOURCE */

#include <libetsi.h>
#include <vector.h >

extern fract32
vecmin_fr32(const fract32 _vector[],
            int           _length)
{
    const fract32 *const vector = _vector;
    const int            length = _length;
    fract32 min_val;

    int i;

    if (length <= 0)
    {
        return 0;
    }

    min_val = vector[0];

#pragma extra_loop_loads
#pragma vector_for

    for (i = 1; i < length; i++)
    {
        min_val = min_fr1x32(vector[i],min_val);
    }

    return min_val;

}

/* End of File */

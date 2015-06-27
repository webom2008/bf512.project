/************************************************************************
 *
 * zerocross_fr32.c : $Revision: 1.2 $
 *
 * (c) Copyright 2007 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: zero_cross_fr32 - count zero crossings

    Synopsis:

        #include <stats.h>
        int zero_cross_fr32 (const fract32 samples[],
                             int           sample_length);

    Description:

        The zero_cross_fr32 function returns the number of times that a signal
        represented in the input array samples[] crosses over the zero line.
        If all the input values are either positive or zero, or they are all
        either negative or zero, then the function returns a zero.

    Error Conditions:

        The function returns zero if the parameter sample_length is less than
        or equal to one.

    Algorithm:

        count = 0U;
        sign_reference = (unsigned int) samples[k] >> 31;

        k += 1;

        for ( ; k < sample_length; k++)
        {
            if (samples[k] != 0)
            {
                sign_current = (unsigned int) samples[k] >> 31;
                count += (sign_current ^ sign_reference);
                sign_reference = sign_current;
            }
        }

    Implementation:

        To improve preformance use Boolean algebra to minimize the number
        of conditional statements within a loop. Also make use of the
        following optimizing pragmas

        #pragma extra_loop_loads
        #pragma vector_for

    Example:

        #include <stats.h>

        #define SAMPLE_LENGTH 100

        fract32 samples[SAMPLE_LENGTH];
        int count;

        count = zero_cross_fr32 (samples,SAMPLE_LENGTH);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =stats.h")
#pragma file_attr("libFunc  =zero_cross_fr32")
#pragma file_attr("libFunc  =__zero_cross_fr32")
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

#include <stats.h>

extern int
zero_cross_fr32(const fract32 _samples[],
                int           _sample_length)
{

    const fract32 *samples  = _samples;
    const int sample_length = _sample_length;

    unsigned int sign_reference;     /* sign bit of reference sample */
    unsigned int sign_current;       /* sign bit of current sample   */
    unsigned int count = 0U;         /* the result                   */

    int k;

    if (sample_length <= 1)
    {
        return 0;
    }

    k = 0;

   /* Prune leading zeros */

#pragma extra_loop_loads
#pragma vector_for

    while ((samples[k] == 0) && (k < sample_length))
    {
        k++;
    }

    sign_reference = (unsigned int) samples[k] >> 31;

    k += 1;

#pragma extra_loop_loads
#pragma vector_for

    for ( ; k < sample_length; k++)
    {
        if (samples[k] != 0)
        {
            sign_current = (unsigned int) samples[k] >> 31;
            count += (sign_current ^ sign_reference);
            sign_reference = sign_current;
        }
    }

    return (int) count;

}

/* End of File */

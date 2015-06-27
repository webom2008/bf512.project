/****************************************************************************
 *
 * var_fr32.c : $Revision: 1.5 $
 *
 * (c) Copyright 2007-2010 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: var_fr32 - variance

    Synopsis:

        #include <stats.h>
        fract32 var_fr32(const fract32 samples[],
                         int     sample_length);

    Description:

        The var_fr32 function returns the variance of the elements within
        the input vector samples[]. The number of elements in the vector is
        sample_length.

    Error Conditions:

        The var_fr32 function shall be used to compute the variance of up to
        4294967295 input data with a value of 0x80000000 before the sum a[i]
        saturates.

        The function returns zero if the sample_length is less than or equal
        to one.

    Algorithm:

        Ss == Sigma(samples[i] * samples[i])   -- Sum of squares
        Se == Sigma(samples[i])                -- Sum of elements
        n  == sample_length

        Variance =  ((n*Ss) - (Se*Se))
                    ---------------
                     (n * (n-1))

    Implementation:

        Protects against oveflow by using 64-bit fixed-point arithmetic.

        Makes use of the following optimizing pragmas:

        #pragma loop_count(2, ,)
        #pragma extra_loop_loads

        Formula on which variance implementation is based -

        variance   ==     Ss        -     ((Se)   (Se)) * n
                                           (-) *  (-)
                                           (n)   (n)
                        -------          --------------
                         (n-1)                (n-1)

                   ==     Ss        -    (mean(samples) * mean(samples) * n)
                        -------         -----------------------------------
                         (n-1)                (n-1)

                   ==     Ss - (mean(samples) * mean(samples) * n)
                         ----------------------------------------
                                      (n-1)

    Example:

        #include <stats.h>
        #define SAMPLES_LENGTH 15

        fract32 samples[SAMPLES_LENGTH];
        fract32 variance;

        variance = var_fr32 (samples,SAMPLES_LENGTH);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =stats.h")
#pragma file_attr("libFunc  =var_fr32")
#pragma file_attr("libFunc  =__var_fr32")
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

#pragma diag(suppress:misra_rule_17_4)
/* Suppress Rule 17.4 (required) which requires that array indexing shall be
** the only allowed form of pointer arithmetic.
**
** Rationale: The offending pointers are local references to external arrays
** referenced by the function's input arguments and it is therefore assumed
** that they will reference a valid range of memory.
*/
#endif /* _MISRA_RULES */

#include <stats.h>
#include <builtins.h>
#include "xutil_fr.h"

extern fract32
var_fr32(const fract32 _samples[],
         int           _sample_length)
{
    /* local copies of the arguments */

    const fract32 *const samples = _samples;
    const int      sample_length = _sample_length;

    fract32    mean_samples, mean_sqr;
    long long  ss = 0;                  /* sum of squares */
    long long  sum = 0;                 /* sum of elements */
    long long  mean2_n;                 /* mean^2 * n */
    long long  diff, var_samples;
    fract32    result;
    int i;

    if (sample_length <= 1)
    {
        return 0;
    }

#pragma loop_count(2, ,)
#pragma extra_loop_loads

    for (i = 0; i < sample_length; i++)
    {
        ss += (long long) multr_fr1x32x32(samples[i], samples[i]);
        sum += (long long) samples[i];
    }

    mean_samples = (fract32)(sum/sample_length);
    mean_sqr = (long long) multr_fr1x32x32(mean_samples, mean_samples);

    mean2_n = (long long)(mean_sqr) * sample_length;    /* sum = mean^2 * n */

    diff = ss - mean2_n;          /*  sum of squares - mean^2 * n */

    var_samples = (diff / (sample_length - 1));

    result = (fract32) __builtin_sat_fr1x64(var_samples);

    return result;

}

/* End of File */

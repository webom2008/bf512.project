/******************************************************************************
 *
 * mean_fr32.c : $Revision: 1.4 $
 *
 * (c) Copyright 2007-2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: mean_fr32 - mean

    Synopsis:

        #include <stats.h>
        fract32 mean_fr32(const fract32 samples[], int sample_length)
        
    Description:

        This function return the mean of the input array samples[ ]. 
        The number of elements in the array is sample_length.

    Error Conditions:

        The mean_fr32 function can be used to compute the mean of up to 
        4294967295 input data with a value of 0x80000000 before the sum ai 
        saturates.

        The function returns zero if the sample_length is less than or equal
        to zero. 
    
    Algorithm:

        mean = (1/N) * (x1 + x2 + ... + xn)

    Implementation:

        Protects against oveflow by using 64-bit fixed-point arithmetic.
      
        Makes use of the following optimizing pragmas:

        #pragma extra_loop_loads
        #pragma vector_for
       
    Example:
        
        #include <stats.h>
        #define SAMPLES_N 100

        fract32 input1[SAMPLES_N];
        fract32 average;     

        average = mean_fr32 (input1,SAMPLES_N);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =stats.h")
#pragma file_attr("libFunc  =mean_fr32")
#pragma file_attr("libFunc  =__mean_fr32")
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

#endif

#include <stats.h>

extern fract32
mean_fr32 (const fract32 _samples[], int _sample_length)
{

    /* local copies of the arguments */

    const fract32 *const samples = _samples;
    const int      sample_length = _sample_length;

    long long sum = 0;
    fract32 average;
    int i;

    if (sample_length <= 0) {
        return 0;
    }

#pragma extra_loop_loads
#pragma vector_for

    for ( i = 0; i < sample_length; i++)
    {
        sum += samples[i];     /* calculates the summation for mean */
    }

    average = (fract32) (sum / sample_length);

    return average; 
}

/* End of File */

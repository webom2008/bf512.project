/******************************************************************************
 *
 * rms_fr32.c : $Revision: 1.3 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: rms_fr32 - Root Mean Square

    Synopsis:

        #include <stats.h>
        fract32 rms_fr32 (const fract32 samples[], int sample_length)
        
    Description:

        The rms_fr32 function returns the root mean square of the elements 
        within the input vector samples[]. The number of elements in the 
        vector is sample_length. 

    Error Conditions:

        The rms_fr32 function will return zero if the sample_length, length 
        of the input vector, is negative or zero.
    
    Algorithm:

        sqrt ( (Sigma(i = 0 to n-1) a[i]^2)/n )

    Implementation:

        Protects against oveflow by using 64-bit fixed-point arithmetic.
      
        Makes use of the following optimizing pragmas:

        #pragma optimize_for_speed
        #pragma extra_loop_loads
        #pragma vector_for
       
    Example:
        
        #include <stats.h>

        #define SAMPLES_N 100
        fract32 input1[SAMPLES_N];
        fract32 rms;     
        rms = rms_fr32 (input1,SAMPLES_N);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =stats.h")
#pragma file_attr("libFunc  =rms_fr32")
#pragma file_attr("libFunc  =__rms_fr32")
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

#pragma diag(suppress:misra_rule_17_4)
/* Suppress Rule 17.4 (required) which requires that array indexing shall be 
** the only allowed form of pointer arithmetic.
**
** Rationale: The offending pointers are local references to external arrays
** referenced by the function's input arguments and it is therefore assumed
** that they will reference a valid range of memory.
*/
#endif

#include <stats.h>
#include <math.h>

extern fract32
rms_fr32 (const fract32 _samples[], int _sample_length)
{

     /* local copies of the arguments */

     const fract32      *const samples = _samples;
     const int           sample_length = _sample_length;

     unsigned long long  sum = 0UL;
     fract32             rms32 = 0;
     int                 i;

     if (sample_length > 0) 
     {

#pragma extra_loop_loads
#pragma vector_for

         for ( i = 0; i < sample_length; i++)
         {
             sum += (unsigned long long)multr_fr1x32x32(samples[i], samples[i]);
         }

         sum  = sum /(unsigned long long) sample_length;
         rms32 = sqrt_fr32((fract32) sum);
     }

     return rms32; 
}

/* End of File */

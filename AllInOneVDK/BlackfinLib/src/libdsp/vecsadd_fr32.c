/************************************************************************
 *
 * vecsadd_fr32.c : $Revision: 1.6 $
 *
 * (c) Copyright 2007 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: vecsadd_fr32 - vector scalar addition

    Synopsis:

        #include <vector.h>
        void vecsadd_fr32 (const fract32 vector[],
                           fract32       scalar,
                           fract32       sum[],
                           int           length);

    Description:

        The vecsadd_fr32 function computes the sum of each element of the
        vector vector[] with the scalar and stores the result in the output
        vector sum[], the input and output vectors being length in size.

    Error Conditions:

        The vecsadd_fr32 function is an effective no-op if the parameter
        length is less than or equal to zero.

    Algorithm:

        sum[i] = vector[i] + scalar
                 where i = 0 to length-1

    Implementation:

        Makes use of the following optimizing pragmas:

          #pragma extra_loop_loads
          #pragma vector_for

    Example:

        #include <vector.h>

        #define SAMPLES_N 43

        fract32 input[SAMPLES_N], sum[SAMPLES_N];
        fract32 scalar_input;

        vecsadd_fr32 (input, scalar_input, sum, SAMPLES_N);

*************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =vecsadd_fr32")
#pragma file_attr("libFunc  =__vecsadd_fr32")
#pragma file_attr("libGroup =matrix.h")      /* called from matrix.h */
#pragma file_attr("libFunc  =matsadd_fr32")
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

#if !defined(ETSI_SOURCE)
#define ETSI_SOURCE
#endif /* ETSI_SOURCE */

#include <libetsi.h>
#include <vector.h>

extern void
vecsadd_fr32(const fract32 _vector[],
             fract32       _scalar,
             fract32       _sum[],
             int           _length)
{

    const fract32 *const vector = _vector;
    const fract32        scalar = _scalar;
    fract32 *__restrict    sum    = _sum;
    const int            length = _length;

    int i;

#pragma extra_loop_loads
#pragma vector_for

    for (i = 0; i < length; i++)
    {
        sum[i] = L_add(vector[i],scalar);
    }

}

/* End of File */

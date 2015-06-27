/************************************************************************
 *
 * cvecsadd_fr32.c : $Revision: 1.6 $
 *
 * (c) Copyright 2007 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: cvecsadd_fr32 - Complex vector scalar addition

    Synopsis:

        #include <vector.h>
        void cvecsadd_fr32 (const complex_fract32 vector[],
                            complex_fract32       scalar,
                            complex_fract32       sum[],
                            int                   length);

    Description:

        The cvecsadd_fr32 function adds input complex scalar, scalar to each
        element of input complex vector, vector[] and stores the results in
        the output complex vector, sum[]. The input and output complex vectors
        being length in size.

    Error Conditions:

        The cvecsadd_fr32 function is an effective no-op if the parameter
        length is less than or equal to zero.

    Algorithm:

        sum[i].re = vector[i].re + scalar.re
        sum[i].im = vector[i].im + scalar.im
                    where i = 0 to length-1

    Implementation:

        Makes use of the following optimizing pragmas:

          #pragma extra_loop_loads
          #pragma vector_for

    Example:

        #include <vector.h>

        #define SAMPLES_N 15
        complex_fract32 input[SAMPLES_N], result[SAMPLES_N];
        complex_fract32 scalar_input; 

        cvecsadd_fr32 (input, scalar_input, result, SAMPLES_N);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =cvecsadd_fr32")
#pragma file_attr("libFunc  =__cvecsadd_fr32")
#pragma file_attr("libGroup =matrix.h")          /* called from matrix.h */
#pragma file_attr("libFunc  =cmatsadd_fr32")
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
cvecsadd_fr32(const complex_fract32 _vector[],
              complex_fract32       _scalar,
              complex_fract32       _sum[],
              int                   _length)
{

    const complex_fract32 *const vector = _vector;
    const complex_fract32        scalar = _scalar;
    complex_fract32 *__restrict       sum = _sum;
    const int                    length = _length;

    int i;

#pragma extra_loop_loads
#pragma vector_for

    for (i = 0; i < length; i++)
    {
        sum[i].re = L_add (vector[i].re, scalar.re);
        sum[i].im = L_add (vector[i].im, scalar.im);
    }

}

/* End of File */

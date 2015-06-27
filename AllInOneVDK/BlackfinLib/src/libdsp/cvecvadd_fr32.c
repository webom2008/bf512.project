/***************************************************************************
 * 
 * cvecvadd_fr32.c : $Revision: 1.6 $
 * 
 * (c) Copyright 2007 Analog Devices, Inc.  All rights reserved.
 * 
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: cvecvadd_fr32 - complex vector vector addition

    Synopsis:

        #include <vector.h>
        void cvecvadd_fr32 (const complex_fract32 vector_a[],
                            const complex_fract32 vector_b[],
                            complex_fract32       sum[],
                            int                   length);

    Description:

        The cvecvadd_fr32 function computes the sum of each of the elements
        of the vectors vector_a[] and vector_b[], and stores the result
        in the output vector sum[], the input and output vectors being
        length in size.

    Error Conditions:

        The cvecvadd_fr32 function is an effective no-op if the parameter
        length is less than or equal to zero.

    Algorithm:

        sum[i].re = vector_a[i].re + vector_b[i].re
        sum[i].im = vector_a[i].im + vector_b[i].im
                    where i = 0 to length-1

    Implementation:

        Makes use of the following optimizing pragmas:

          #pragma extra_loop_loads
          #pragma different_banks
          #pragma vector_for

    Example:

        #include <vector.h>

        #define SAMPLES_N 15

        complex_fract32 vector_x[SAMPLES_N], result[SAMPLES_N];
        complex_fract32 vector_y[SAMPLES_N];

        cvecvadd_fr32 (vector_x, vector_y, result, SAMPLES_N);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =cvecvadd_fr32")
#pragma file_attr("libFunc  =__cvecvadd_fr32")
#pragma file_attr("libGroup =matrix.h")          /* called from matrix.h */
#pragma file_attr("libFunc  =cmatmadd_fr32")
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
cvecvadd_fr32(const complex_fract32 _vector_a[],
              const complex_fract32 _vector_b[],
              complex_fract32       _sum[],
              int                   _length)
{

    const complex_fract32 *const vector_a = _vector_a;
    const complex_fract32 *const vector_b = _vector_b;
    complex_fract32 *__restrict         sum = _sum;
    const int                      length = _length;

    int  i;

#pragma extra_loop_loads
#pragma different_banks
#pragma vector_for

    for (i = 0; i < length; i++)
    {
        sum[i].re = L_add(vector_a[i].re, vector_b[i].re);
        sum[i].im = L_add(vector_a[i].im, vector_b[i].im);
    }

}

/* End of File */

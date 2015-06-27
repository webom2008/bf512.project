/***************************************************************************
 * 
 * vecvadd_fr32.c : $Revision: 1.6 $
 * 
 * (c) Copyright 2006-2007 Analog Devices, Inc.  All rights reserved.
 * 
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: vecvadd_fr32 - Vector vector addition

    Synopsis:

        #include <vector.h>
        void vecvadd_fr32 (const fract32 vector_x[],
                           const fract32 vector_y[],
                           fract32       sum[],
                           int           length);

    Description:

        The vecvadd_fr32 function computes the sum of each of the elements
        of the vectors vector_x[] and vector_y[], and stores the result in
        the output vector sum[], the input and output vectors being length
        in size.

    Error Conditions:

        The vecvadd_fr32 function is an effective no-op if the parameter
        length is less than or equal to zero.

    Algorithm:

        sum[i] =  vector_x[i] + vector_y[i]
                  where i = 0 to length-1

    Implementation:

        Makes use of the following optimizing pragmas:

          #pragma extra_loop_loads
          #pragma different_banks
          #pragma vector_for

    Example:

        #include <vector.h>
        #define LENGTH 15

        fract32 vector_x[LENGTH],sum[LENGTH];
        fract32 vector_y[LENGTH];

        vecvadd_fr32 (vector_x,vector_y,sum,LENGTH);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =vecvadd_fr32")
#pragma file_attr("libFunc  =__vecvadd_fr32")
#pragma file_attr("libGroup =matrix.h")      /* called from matrix.h */
#pragma file_attr("libFunc  =matmadd_fr32")
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
vecvadd_fr32(const fract32 _vector_x[],
             const fract32 _vector_y[],
             fract32       _sum[],
             int           _length)
{

    const fract32 *const vector_x = _vector_x;
    const fract32 *const vector_y = _vector_y;
    fract32 *__restrict         sum = _sum;
    const int              length = _length;

    int  i;

#pragma extra_loop_loads
#pragma different_banks
#pragma vector_for

    for (i = 0; i < length; i++)
    {
        sum[i] = L_add(vector_x[i],vector_y[i]);
    }

}

/* End of File */

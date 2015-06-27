/************************************************************************
 *
 * vecssub_fr32.c : $Revision: 1.6 $
 *
 * (c) Copyright 2007 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: vecssub_fr32 - vector scalar subtraction

    Synopsis:

        #include <vector.h>
        void vecssub_fr32 (const fract32 vector[],
                           fract32       scalar,
                           fract32       difference[],
                           int           length);

    Description:

        The vecssub_fr32 function computes the difference of each element
        of the vector vector[] with the scalar and stores the result in
        the output vector difference[], the input and output vector being
        length in size.

    Error Conditions:

        The vecssub_fr32 function is an effective no-op if the parameter
        length is less than or equal to zero.

    Algorithm:

        difference[i] = vector[i] - scalar 
                        where i = 0 to length-1

    Implementation:

        Makes use of the following optimizing pragmas:

          #pragma extra_loop_loads
          #pragma vector_for

    Example:

        #include <vector.h>

        #define SAMPLES_N 43

        fract32 input[SAMPLES_N],difference[SAMPLES_N];
        fract32 scalar_input;

        vecssub_fr32 (input,scalar_input,difference,SAMPLES_N);

**************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =vecssub_fr32")
#pragma file_attr("libFunc  =__vecssub_fr32")
#pragma file_attr("libGroup =matrix.h")      /* called from matrix.h */
#pragma file_attr("libFunc  =matssub_fr32")
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
vecssub_fr32(const fract32 _vector[],
             fract32       _scalar,
             fract32       _difference[],
             int           _length)
{

    const fract32 *const vector  = _vector;
    const fract32        scalar  = _scalar;
    fract32 *__restrict difference = _difference;
    const int             length = _length;

    int i;

#pragma extra_loop_loads
#pragma vector_for

    for (i = 0; i < length; i++)
    {
        difference[i] = L_sub(vector[i],scalar);
    }

}

/* End of File */

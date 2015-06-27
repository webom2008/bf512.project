/************************************************************************
 *
 * vecsmlt_fr32.c : $Revision: 1.5 $
 *
 * (c) Copyright 2007-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: vecsmlt_fr32 - vector scalar multiplication

    Synopsis:

        #include <vector.h>
        void vecsmlt_fr32 (const fract32 vector[],
                           fract32       scalar,
                           fract32       product[],
                           int           length);

    Description:

        The vecsmlt_fr32 function computes the product of each element of
        the vector vector[] with the scalar and the result is stored in
        the output vector product[], the input and output vectors being
        length in size.

    Error Conditions:

        The vecsmlt_fr32 function is an effective no-op if the parameter
        length is less than or equal to zero.

    Algorithm:

        product[i] = vector[i] * scalar
                     where i = 0 to length-1

    Implementation:

        Makes use of the following optimizing pragmas:

          #pragma extra_loop_loads
          #pragma vector_for

    Example:

        #include <vector.h>

        #define SAMPLES_N 60

        fract32 input[SAMPLES_N],product[SAMPLES_N];
        fract32 scalar_input;

        vecsmlt_fr32 (input,scalar_input,product,SAMPLES_N);

**************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =vecsmlt_fr32")
#pragma file_attr("libFunc  =__vecsmlt_fr32")
#pragma file_attr("libGroup =matrix.h")      /* called from matrix.h */
#pragma file_attr("libFunc  =matsmlt_fr32")
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
#endif /* _MISRA_RULES */

#include <vector.h>
#include <builtins.h>

extern void
vecsmlt_fr32(const fract32 _vector[],
             fract32       _scalar,
             fract32       _product[],
             int           _length)
{

    const fract32 *const vector = _vector;
    const fract32        scalar = _scalar;
    fract32 *__restrict   product = _product;
    const int           length  = _length;

    int i;

#pragma extra_loop_loads
#pragma vector_for

    for (i = 0; i < length; i++)
    {
        product[i] = multr_fr1x32x32(vector[i],scalar);
    }

}

/* End of File */

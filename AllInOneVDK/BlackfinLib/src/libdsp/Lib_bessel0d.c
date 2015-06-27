/************************************************************************
 *
 * Lib_bessel0d.c : $Revision: 1.1 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: modified Bessel function of the first kind (zero-th order) 

    Synopsis:

        #include "xwingen.h"
        long double bessel0 (const long double _x)

    Description:

        The bessel0 function computes the modified Bessel function of the 
        first kind (zero-th order) for the input argument x. 

        The function is a support function, used by the gen_kaiser_fr32
        window generator.

    Error Conditions:

        N/A

    Algorithm:

        The function is based on the following algorithm:

            long double x2 = x * x;
            long double s = 1.0L;
            long double ds = 1.0L;
            int  d = 0;

            do
            {
                d  += 2;
                ds *= (x2 / (long double) (d*d));
                s  += ds;
            } while (ds > (1.0e-9L * s));

            return s;

    Implementation:

        In order to accelerate the computation for the most common cases
        (x <= 15), a lookup table will be used for the first 25 values
        of the expression 1.0L / (long double) (d*d). 

        A separate loop will handle input values that require additional
        iterations to approxiamte the Bessel function.          

    Example:

        Refer to the implementation of the windows generator gen_kaiser_fr32.

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =window.h")
#pragma file_attr("libFunc  =gen_kaiser_fr32")
#pragma file_attr("libFunc  =__gen_kaiser_fr32")
#pragma file_attr("libName  =libdsp")
#pragma file_attr("prefersMem =external")
#pragma file_attr("prefersMemNum =70")
        /* (Use prefersMem=external because the function
        **  is usually called no more than once)
        */
#endif

#if !defined(TRACE)
#pragma optimize_for_space
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
** Rationale: Optimize performance for the most common use case.
*/
#endif /* _MISRA_RULES */

#include <window.h>
#include <limits.h>
#include <builtins.h>
#include <math.h>
#include "xwingen.h"

#define  MAX_BESSEL_TABLE  25

extern long double
bessel0 (const long double _x)
{
#if defined(__DOCUMENTATION__)
----------------------------------------------
    Const table declaration
    Populated using the following formula:
    
        denom=[2:2:(24*2)];
        bessel_table = (1./(denom.*denom))';
----------------------------------------------
#endif
    static const long double bessel_table[MAX_BESSEL_TABLE] =
    {
        0.250000000000000L,
        0.062500000000000L,
        0.027777777777778L,
        0.015625000000000L,
        0.010000000000000L,
        0.006944444444444L,
        0.005102040816327L,
        0.003906250000000L,
        0.003086419753086L,
        0.002500000000000L,
        0.002066115702479L,
        0.001736111111111L,
        0.001479289940828L,
        0.001275510204082L,
        0.001111111111111L,
        0.000976562500000L,
        0.000865051903114L,
        0.000771604938272L,
        0.000692520775623L,
        0.000625000000000L,
        0.000566893424036L,
        0.000516528925620L,
        0.000472589792060L,
        0.000434027777778L,
        0.000400000000000L
    };

    /* local copies of the arguments */

    long double x = _x;
    long double x2 = x * x;
    long double s = 1.0L;
    long double ds = 1.0L;

    int  d, i, denom;

    /* Loop utilizing the lookup table */
#pragma vector_for
#pragma loop_count(1,25,)
    for (i = 0;
         i < MAX_BESSEL_TABLE;
         i++)
    {
        ds *= (x2 * bessel_table[i]);
        s  += ds;

        if ((1.0e-9L * s) > ds) 
        { 
            return s; 
        }
    }

    /* Loop to handle cases where the number of iterations
    ** required will exceed the size of the lookup table
    */
#pragma vector_for
#pragma loop_count(1,,)
    for (d = 2 * MAX_BESSEL_TABLE; ; d += 2)
    {
        denom = d * d;
        ds *= (x2 / (long double) denom);
        s  += ds;

        if ((1.0e-9L * s) > ds)
        {
            break;
        }
    }

    return s;
}

/* End of File */

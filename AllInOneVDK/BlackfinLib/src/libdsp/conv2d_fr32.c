/*****************************************************************************
 *
 * conv2d_fr32.c : $Revision: 1.5 $
 *
 * (c) Copyright 2008-2010 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: 2-D convolution

    Synopsis:

        #include <filter.h>
        void conv2d_fr32 (const fract32  input_x[],
                          int            rows_x,
                          int            columns_x,
                          const fract32  input_y[],
                          int            rows_y,
                          int            columns_y,
                          fract32        output[]);

    Description:

        The conv2d function computes the two-dimensional convolution of
        input matrix input_x with dimensions [rows_x][columns_x] and input
        matrix input_y with dimensions [rows_y][columns_y] and stores the
        result in matrix output with dimensions [rows_x + rows_y - 1]
        and [columns_x + columns_y - 1].

    Error Conditions:

        The conv2d function returns if the sizes of any of the dimensions
        (rows_x, columns_x, rows_y, columns_y) are less than or equal to zero.

    Algorithm:

        The two-dimensional convolution of x[rows_x][cols_x] and
        y[rows_y][cols_y] is defined as:

           output[r][c] = Sum (j) Sum (k) x[j][k] * y[r-j][c-k]

                where:    r = [ 0 .. (rows_x + rows_y - 1) ]
                          c = [ 0 .. (cols_x + cols_y - 1) ]
                          j = [ max(0, r - rows_y) .. min(r, rows_x - 1) ]
                          k = [ max(0, c - cols_y) .. min(c, cols_x - 1) ]

    Implementation:

        The function makes use of the following loop optimization pragmas:

           #pragma no_alias
           #pragma extra_loop_loads
           #pragma different_banks
           #pragma loop_count (1,,)

        Intermediate sums are stored using 64-bit integers.

    Example:

        #include <filter.h> 

        #define ROWS_1 4 
        #define ROWS_2 4 
        #define COLS_1 8 
        #define COLS_2 2 

        fract32 input_1[ROWS_1][COLS_1], *a_p = (fract32 *) (&input_1); 
        fract32 input_2[ROWS_2][COLS_2], *b_p = (fract32 *) (&input_2); 
        fract32 result [ROWS_1+ROWS_2-1][COLS_1+COLS_2-1];

        fract32 *res_p = (fract32 *) (&result); 

        conv2d_fr32 (a_p, ROWS_1, COLS_1, b_p, ROWS_2, COLS_2, res_p); 

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup=filter.h")
#pragma file_attr("libFunc=__conv2d_fr32")
#pragma file_attr("libFunc=conv2d_fr32")

#pragma file_attr("libName=libdsp")
#pragma file_attr("prefersMem=internal")
#pragma file_attr("prefersMemNum=30")
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

#if !defined(ETSI_SOURCE)
#define ETSI_SOURCE
#endif /* ETSI_SOURCE */

#include <libetsi.h>
#include <filter.h>
#include <math.h>
#include "xutil_fr.h"

extern void
conv2d_fr32 (const fract32  _input_x[],
             int            _rows_x,
             int            _columns_x,
             const fract32  _input_y[],
             int            _rows_y,
             int            _columns_y,
             fract32        _output[])
{
    /* local copies of the arguments */

    const int rows_x    = _rows_x;
    const int columns_x = _columns_x;
    const int rows_y    = _rows_y;
    const int columns_y = _columns_y;

    /* compute size output array */

    const int rows_out    = (rows_x + rows_y) - 1;
    const int columns_out = (columns_x + columns_y) - 1;

    /* local ptrs to the matrices */

    const fract32 (*const x)[columns_x]  = (const fract32 (*)[]) _input_x;
    const fract32 (*const y)[columns_y]  = (const fract32 (*)[]) _input_y;
    fract32 (*const output)[columns_out] = (fract32 (*)[]) _output;

    int        r, c, j, min_j, max_j, k, min_k, max_k;
    long long  sum;

    if ( (rows_x <= 0) || (columns_x <= 0) ||
         (rows_y <= 0) || (columns_y <= 0) )
    {
        return;
    }

    /* preset values */
    min_j = 0;
    max_j = 0;

#pragma loop_count (1,,)
    for (r = 0; r < rows_out; r++)
    {

        min_k = 0;
        max_k = 0;

#pragma no_alias
#pragma extra_loop_loads
#pragma different_banks
#pragma loop_count (1,,)

        /* loop for every column in output (remainder) */
        for (c = 0; c < columns_out; c++)
        {
            sum = 0;

#pragma no_alias
#pragma extra_loop_loads
#pragma different_banks
#pragma loop_count (1,,)

            for (j = min_j; j <= max_j; j++)
            {

#pragma no_alias
#pragma extra_loop_loads
#pragma different_banks
#pragma loop_count (1,,)

                for (k = min_k; k <= max_k; k++)
                {
                    sum += (long long) multr_fr1x32x32 (x[j][k],y[r-j][c-k]);
                }
            }

            output[r][c] = (fract32) __builtin_sat_fr1x64(sum);

            /* ensure that y[][c-k] valid index */
            min_k = max( 0, ((c - columns_y) + 2) );

            /* ensure that x[][k] valid index */
            max_k = min( c + 1, columns_x - 1 );
        }

        /* ensure that y[r-j][] valid index */
        min_j = max( 0, ((r - rows_y) + 2) );

        /* ensure that x[r][] valid index */
        max_j = min( r + 1, rows_x - 1 );
    }
}

/* End of File */

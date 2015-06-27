/*****************************************************************************
 *
 * conv2d3x3_fr32.c : $Revision: 1.5 $
 *
 * (c) Copyright 2008-2010 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: 2-D circular convolution with a 3 x 3 matrix

    Synopsis:

        #include <filter.h>
        void conv2d3x3_fr32 (const fract32 input_x[],
                             int           rows_x,
                             int           columns_x,
                             const fract32 input_y[],
                             fract32       output[]);

    Description:

        The conv2d3x3 function computes the two-dimensional circular
        convolution of matrix input_x with dimensions [rows_x][columns_x]
        and matrix input_y with dimensions [3][3], and stores the
        result in matrix output with dimensions [rows_x][columns_x].

    Error Conditions:

        The conv2d3x3 function returns if any of the dimensions
        rows_x or columns_x are less than or equal to zero.

    Algorithm:

        The two-dimensional circular convolution of input_x[rows_x][cols_x]
        and input_y[3][3] is defined as:

        output[r][c] = Sum (j = 0, 1, 2)
                           Sum (k = 0, 1, 2)
                               x[(r-j) % rows_x][(c-k) % cols_x] * y[j][k]

            where r = [0..(rows_x - 1)]
                  c = [0..(cols_x - 1)]

    Implementation:

        The implementation is based on the following algorithm. In order to
        improve performance, the modulo operation is performed using circular
        buffers.

        for (i = 0; i < rows_a; i++)
        {
            for (j = 0; j< cols_a; j++)
            {
                c1 = 0;
                for (k = 0; k < 3; k++)
                {
                    for (l = 0; l < 3; l++)
                    {
                        c1 = c1 +
                             mult_r( b[3*k+l],
                                     a[((r-k)%rows_a)*cols_a + ((c-l)%cols_a)]);
                    }
                }
                if (c1 >= INT_MAX)
                    c[cols_a*i+j] = INT_MAX;
                else if(c1 <= INT_MIN)
                    c[cols_a*i+j] = INT_MIN;
                else
                    c[cols_a*i+j] = (fract32)c1;
            }
        }

        Intermediate sums are stored using 64-bit integers.

    Example:

        #include <filter.h>

        #define ROWS 9
        #define COLS 9

        fract32 input_1[ROWS][COLS], *a_p = (fract32 *) (&input_1);
        fract32 input_2[3][3],       *b_p = (fract32 *) (&input_2);
        fract32 result [ROWS][COLS];

        fract32 *res_p = (fract32 *) (&result);

        conv2d3x3_fr32 (a_p, ROWS, COLS, b_p, res_p);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup=filter.h")
#pragma file_attr("libFunc=__conv2d3x3_fr32")
#pragma file_attr("libFunc=conv2d3x3_fr32")

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
#include <ccblkfn.h>
#include <filter.h>
#include "xutil_fr.h"

extern void
conv2d3x3_fr32 (const fract32  _input_x[],
                int            _rows_x,
                int            _columns_x,
                const fract32  _input_y[],
                fract32        _output[])
{
    /* local copies of the arguments */

    const int rows_x    = _rows_x;
    const int columns_x = _columns_x;

    /* local ptrs to the matrices */

    const fract32 (*const x)[columns_x] = (const fract32 (*)[]) _input_x;
    const fract32 (*const y)[3]         = (const fract32 (*)[]) _input_y;
    fract32  (*const output)[columns_x] = (fract32 (*)[]) _output;

    int  row_out, col_out;
    int  base_row_x, read_row_x;
    int  base_col_x, read_col_x;
    int  i, j;

    long long  sum;

    if ((rows_x <= 0) || (columns_x <= 0))
    {
        return;
    }

    base_row_x = 0;
    base_col_x = 0;

    /* Loop for every row in output */
    for (row_out = 0; row_out < rows_x; row_out++)
    {

        /* Loop for every column in output */
        for (col_out = 0; col_out < columns_x; col_out++)
        {
            sum = 0;

            read_row_x = base_row_x;
            read_col_x = base_col_x;

#pragma no_alias
#pragma extra_loop_loads
#pragma different_banks
#pragma vector_for

            /* Loop for every row in y */
            for (i = 0; i < 3; i++)
            {

                /* Loop for every row in y */
                for (j = 0; j < 3; j++)
                {
                    sum += (long long) multr_fr1x32x32 (y[i][j],
                                                  x[read_row_x][read_col_x]);

                    read_col_x = circindex(read_col_x,
                                           -1,
                                           (unsigned long) columns_x);
                }

                read_col_x = base_col_x;
                read_row_x = circindex(read_row_x, -1, (unsigned long) rows_x);
            }

            /* Clip convolution sum to fit fractional range */
            output[row_out][col_out] = (fract32) __builtin_sat_fr1x64(sum);

            base_col_x = circindex(base_col_x, 1, (unsigned long) columns_x);
        }

        base_row_x = circindex(base_row_x, 1, (unsigned long) rows_x);
    }
}

/* End of File */

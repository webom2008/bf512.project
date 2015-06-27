/************************************************************************
 *
 * cmatmmlt_fr32.c : $Revision: 1.4 $
 *
 * (c) Copyright 2007-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

     Function: cmatmmlt_fr32 - complex matrix-matrix multiplication

     Synopsis:

        #include <matrix.h>
        void cmatmmlt_fr32 (const complex_fract32 matrix_a[],
                            int                   rows_a,
                            int                   columns_a,
                            const complex_fract32 matrix_b[],
                            int                   columns_b,
                            complex_fract32       product[])
    Description:

        The cmatmmlt_fr32 function computes the product of two input complex
        matrices, matrix_a and matrix_b, and stores the result in the output
        matrix, product. The dimensions of matrix_a are rows_a and columns_a,
        and the dimensions of matrix_b are columns_a and columns_b. The
        dimensions of the resulting output matrix are rows_a and columns_b.

    Error Conditions:

        The cmatmmlt_fr32 function will return if the sizes of any of
        the dimensions are either zero or negative.

    Algorithm:

        for (i = 0; i < rows_a; i++) {

            for (j = 0; j < columns_b; j++) {

                product[i][j].re = 0;
                product[i][j].im = 0;

                for (k = 0; k < columns_a; k++) {
                    product[i][j].re += (matrix_a[i][k].re * matrix_b[k][j].re)
                                      - (matrix_a[i][k].im * matrix_b[k][j].im);
                    product[i][j].im += (matrix_a[i][k].re * matrix_b[k][j].im)
                                      + (matrix_a[i][k].im * matrix_b[k][j].re);
                }
            }
        }

    Implementation:

        Uses 64-bit fixed-point arithmetic to avoid overflow, and makes
        use of the following optimizing pragmas:

        #pragma extra_loop_loads
        #pragma different_banks
        #pragma vector_for

    Example:

        #include <matrix.h>
        #define ROWS_X  8
        #define COLS_X 16
        #define COLS_Y 32

        fract32 matrix_x[ROWS_X][COLS_X], *px = (fract32 *)(&matrix_x);
        fract32 matrix_y[COLS_X][COLS_Y], *py = (fract32 *)(&matrix_y);
        fract32 product [ROWS_X][COLS_Y], *pr = (fract32 *)(&product);

        cmatmmlt_fr32 (px,ROWS_X,COLS_X,py,COLS_Y,pr);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =matrix.h")
#pragma file_attr("libFunc  =cmatmmlt_fr32")
#pragma file_attr("libFunc  =__cmatmmlt_fr32")
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

#include <matrix.h>
#include <complex.h>
#include "xutil_fr.h"

extern void
cmatmmlt_fr32 (const complex_fract32 _matrix_a[],
               int                   _rows_a,
               int                   _columns_a,
               const complex_fract32 _matrix_b[],
               int                   _columns_b,
               complex_fract32       _product[])

{
    /* local copies of the arguments */

    const int rows_a    = _rows_a;
    const int columns_a = _columns_a;
    const int columns_b = _columns_b;

    /* local ptrs to the matrices */

    const complex_fract32 (*const x)[columns_a] =
                          (const complex_fract32 (*)[]) _matrix_a;
    const complex_fract32 (*const y)[columns_b] =
                          (const complex_fract32 (*)[]) _matrix_b;
    complex_fract32 (*const product)[columns_b] =
                                 (complex_fract32 (*)[]) _product;

    complex_fract32 acc;
    long long sum_re, sum_im;
    int i, j, k;

    if ( (rows_a <= 0) || (columns_a <= 0) || (columns_b <= 0) )
    {
        return;
    }

    for (i = 0; i < rows_a; i++)
    {

#pragma extra_loop_loads
#pragma different_banks
#pragma vector_for
        for (j = 0; j < columns_b; j++)
        {
            sum_re = 0;
            sum_im = 0;

#pragma extra_loop_loads
#pragma different_banks
#pragma vector_for
            for (k = 0; k < columns_a; k++)
            {
                acc = cmlt_fr32 (x[i][k], y[k][j]);
                sum_re += (long long) acc.re;
                sum_im += (long long) acc.im;
            }

            product[i][j].re= (fract32) __builtin_sat_fr1x64(sum_re);
            product[i][j].im= (fract32) __builtin_sat_fr1x64(sum_im);
        }
    }
}

/* End of File */

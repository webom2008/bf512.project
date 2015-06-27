/*****************************************************************************
 *
 * conv2d3x3_fr16.c : $Revision: 3543 $
 *
 * (c) Copyright 2007 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: 2-D convolution with a 3 x 3 matrix 

    Synopsis:

        #include <filter.h>
        void conv2d3x3_fr16 (const fract16 input_x[],
                             int rows_x, 
                             int columns_x,
                             const fract16 input_y[],
                             fract16 output[]);

    Description:

        The conv2d3x3 function computes the two-dimensional circular 
        convolution of matrix input_x (size [rows_x][columns_x]) with 
        matrix input_y (size [3][3]). 

    Error conditions:

        The conv2d3x3 function aborts for rows_x or columns_x <= 0.

    Algorithm:

        The two-dimensional circular convolution of input_x[rows_x][cols_x] 
        and input_y[3][3] is defined as:
 
        output[r][c] = Sum (j = 0, 1, 2) 
                          Sum (k = 0, 1, 2) x[(r-j) % rows_x][(c-k) % cols_x]
                                            * y[j][k]
 
           where r = [0..(rows_x - 1)]
                 c = [0..(cols_x - 1)]

 
        The implementation is based on the following algorithm. In order to
        improve performance, the modulo operation is performed using circular 
        buffers.   
      
        for (i = 0; i < rows_x; i++) 
        {
            for (j = 0; j< cols_x; j++)
            {
                c1 = 0;
                for (k = 0; k < 3; k++)
                {
                    for (l = 0; l < 3; l++)
                    {
                        c1 = c1 + 
                             mult_r( b[3*k+l],
                                     a[((r-k)%rows_x)*cols_x + 
                                       ((c-l)%cols_x)] );
                    }
                }
                if (c1 >= 32767)
                    c[cols_x*i+j] = 32767;
                else if(c1 <= -32768)
                    c[cols_x*i+j] = -32768;
                else
                    c[cols_x*i+j] = (fract16)c1;
            }
        }

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup=filter.h")
#pragma file_attr("libFunc=__conv2d3x3_fr16")
#pragma file_attr("libFunc=conv2d3x3_fr16")

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

#pragma diag(suppress:misra_rule_10_1_a)
/* Suppress Rule 10.1.a (required) whereby the value of an expression of
** integer type shall not be implicitly converted to a different underlying
** type if it is not a conversion to a wider integer type of the same
** signedness. (in this case "short" and "fract16").
**
** Rationale: The algorithm requires to clip the accumulated sum
** to ensure the result falls within the fractional range. The conditional 
** expression used ensures that no value can exceed the range of the type
** fract16.
*/

#pragma diag(suppress:misra_rule_14_7)
/* Suppress Rule 14.7 (required) which requires that a function shall
** have a single point of exit at the end of the function.
**
** Rationale: The requirement is only suppressed provided that the only
** violation occurs at the beginning of the function when the function
** verifies its parameters are "correct".
*/
#endif /* _MISRA_RULES */


#if !defined(ETSI_SOURCE)
#define ETSI_SOURCE
#endif /* ETSI_SOURCE */


#include <libetsi.h>
#include <ccblkfn.h>
#include <filter.h>
#include <limits.h>


extern void 
conv2d3x3_fr16( const fract16 _input_x[], int _rows_x, int _columns_x,
                const fract16 _input_y[], 
                fract16 _output[])
{
    /* local copies of the arguments */

    const int rows_x    = _rows_x;
    const int columns_x = _columns_x;

    /* local ptrs to the matrices */

    const fract16 (*const x)[columns_x] = (const fract16 (*)[]) _input_x;
    const fract16 (*const y)[3]         = (const fract16 (*)[]) _input_y;
    fract16 (*const output)[columns_x]  = (fract16 (*)[]) _output;


    int  row_out, col_out, k;
    int  sum32;

    int  base_row_x, read_row_x, base_col_x, read_col_x; 


    /* Error Handling */
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

            sum32 = 0;

            read_row_x = base_row_x;
            read_col_x = base_col_x;

#pragma no_alias
#pragma extra_loop_loads
#pragma different_banks
#pragma vector_for

            /* Loop for every row in y 
            ** Innermost loop for every column in y unrolled 
            */
            for (k = 0; k < 3; k++)
            {
                sum32 += (int) mult_r( y[k][0], x[read_row_x][read_col_x] );

                read_col_x = circindex( read_col_x, 
                                        -1, 
                                        (unsigned long) columns_x  );

                sum32 += (int) mult_r( y[k][1], x[read_row_x][read_col_x] );

                read_col_x = circindex( read_col_x,
                                        -1,
                                        (unsigned long) columns_x  );

                sum32 += (int) mult_r( y[k][2], x[read_row_x][read_col_x] );

                read_col_x = base_col_x;                
                read_row_x = circindex( read_row_x, -1, rows_x );

            }

            /* Clip convolution sum to fit fractional range */
            output[row_out][col_out] = 
                          (fract16) ((sum32 >= SHRT_MAX) ? SHRT_MAX :
                                    ((sum32 <= SHRT_MIN) ? SHRT_MIN : sum32));

            base_col_x = circindex( base_col_x, 1, columns_x );
        }
        
        base_row_x = circindex( base_row_x, 1, rows_x );
    }
}

/* End of File */

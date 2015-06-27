/****************************************************************************
 *
 * rfft2d_fr32.c : $Revision: 1.8.12.1 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: rfft2d_fr32 - N x N point 2-D real Fast Fourier Transform 

    Synopsis:

        #include <filter.h>
        void rfft2d_fr32 (const fract32          *input,
                          complex_fract32        *temp,
                          complex_fract32        *output,
                          const complex_fract32  twiddle_table[],
                          int                    twiddle_stride, 
                          int                    fft_size);

    Description:

        This function computes the two-dimensional Fast Fourier Transform (FFT)
        of the real input matrix input[fft_size][fft_size] and stores the
        result in the complex output matrix output[fft_size][fft_size].

        The size of the input array input, the output array output, and the
        temporary working buffer temp is fft_size * fft_size, where fft_size 
        represents the number of rows and the number of columns in the FFT. 
        The number of points in the FFT must be a power of 2 and must be at
        least 16.

        Memory bank collisions, which have an adverse effect on run-time 
        performance, may be avoided by allocating the twiddle table in a 
        different memory bank than the output matrix and the temporary buffer. 
        If the input data can be overwritten, optimal memory usage can be 
        achieved by also specifying the input matrix as the output buffer. In
        this case, the size of the of the input buffer has to be increased to
        2 * fft_size * fft_size.

        The twiddle table is passed in the argument twiddle_table, which
        must contain at least 3*fft_size/4 complex twiddle factors. The
        table should be initialized with complex twiddle factors in which
        the real coefficients are positive cosine values and the imaginary
        coefficients are negative sine values. The function twidfft2d_fr32
        may be used to initialize the array.

        If the twiddle table has been generated for an fft_size FFT, then
        the twiddle_stride argument should be set 1. On the other hand, if
        the twiddle table has been generated for an FFT of size x, where
        x > fft_size, then the twiddle_stride argument should be set to
        x / fft_size. The twiddle_stride argument therefore allows the
        same twiddle table to be used for different sizes of FFT. (The
        twiddle_stride argument cannot be either zero or negative).

        To avoid overflow, the function scales the output by fft_size*fft_size.

    Error Conditions:

        The rfft2d_fr32 function aborts if fft_size is less than 16 
        or if the twiddle stride is less than 1.

    Implementation:

        The function has been optimized for best performance when using large 
        FFT sizes, with data being allocated in external memory. It makes use
        of the rfftf_fr32, cfftf_fr32, conj_fr32 and ctranspm_fr32 DSP 
        runtime library functions.

    Example:

        #include <filter.h>
        #define FFT_SIZE1         128
        #define FFT_SIZE2          32

        #define TWIDDLE_STRIDE1  (FFT_SIZE1 / FFT_SIZE1)
        #define TWIDDLE_STRIDE2  (FFT_SIZE1 / FFT_SIZE2)

        complex_fract32  out1[FFT_SIZE1][FFT_SIZE1];
        fract32         *in1 = (fract32 *) out1;

        complex_fract32  temp[FFT_SIZE1][FFT_SIZE1];

        fract32          in2[FFT_SIZE2][FFT_SIZE2];
        complex_fract32  out2[FFT_SIZE2][FFT_SIZE2];

        complex_fract32  twiddle[(3*FFT_SIZE1)/4];


        twidfft2d_fr32 (twiddle, FFT_SIZE1);

        /* In-place computation */
        rfft2d_fr32 (in1, temp, out1, twiddle, 
                     TWIDDLE_STRIDE1, FFT_SIZE1);

        rfft2d_fr32 (in2, temp, out2, twiddle,
                    TWIDDLE_STRIDE2, FFT_SIZE2);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =filter.h")
#pragma file_attr("libFunc  =rfft2d_fr32")
#pragma file_attr("libFunc  =__rfft2d_fr32")
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


#include <filter.h>

#include <matrix.h>
#include <complex.h>

extern void  
rfft2d_fr32 (const fract32          *_input,
             complex_fract32        *_temp,
             complex_fract32        *_output,
             const complex_fract32  _twiddle_table[],
             int                    _twiddle_stride, 
             int                    _fft_size)
{
    const fract32 *const          input         = _input;
    complex_fract32 *__restrict     temp          = _temp;
    complex_fract32 *__restrict     output        = _output;
    const complex_fract32 *const  twiddle_table = _twiddle_table;

    const int  twiddle_stride_rows    = _twiddle_stride;
    const int  twiddle_stride_columns = _twiddle_stride;

    const int  rows    = _fft_size;
    const int  columns = _fft_size;  

    complex_fract32  cval;

    int  i, j;


    if ((rows < 16) || (columns < 16) || 
        (twiddle_stride_rows < 1) || (twiddle_stride_columns < 1))
    {
        return;
    } 

#pragma different_banks
#pragma loop_count(16,,)
    /* Compute FFT for each row */
    for (j = 0; j < rows; j++)
    {
        rfftf_fr32 (&input[j*columns], &temp[j*columns], 
                    twiddle_table, twiddle_stride_columns, columns); 

        /* Since the rfftf_fr32 function only returns ((columns/2)+1)
        ** output values, need to fill remaining columns in the array
        ** temp with conjugate values.
        **/
#pragma loop_count(8,,)
#pragma no_alias
        for (i = 1; i < (columns/2); i++)
        {
            cval = temp[(j*columns) + i];
            temp[((j+1)*columns) - i] = conj_fr32 (cval);
        }
    }    

    /* Copy the data stored in the temporary buffer 
    ** to the output buffer in transposed order.
    **/
    ctranspm_fr32 (temp, rows, columns, output);

#pragma different_banks
#pragma loop_count(16,,)
    /* Compute FFT for each column */
    for (j = 0; j < columns; j++)
    {
        cfftf_fr32 (&output[j*rows], &temp[j*rows],
                    twiddle_table, twiddle_stride_rows, rows);
    }

    /* Copy the data stored in the temporary buffer
    ** back to the output buffer in transposed order
    */
    ctranspm_fr32 (temp, columns, rows, output);
}

/* End of File */

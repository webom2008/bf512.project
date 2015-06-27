/******************************************************************************
 *
 * fft_magnitude_fr16.c :  $Revision: 1.3 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: FFT magnitude function

    Synopsis:

        #include <filter.h>
        void fft_magnitude_fr16 (const complex_fract16 input[],
                                 fract16               output[],
                                 int                   fft_size,
                                 int                   block_exponent,
                                 int                   mode);

    Description:

        The fft_magnitude_fr16 function computes a normalized power spectrum
        from the output signal generated by an FFT function. The fft_size
        argument specifies the size of the FFT and must be a power of 2. The
        mode argument is used to specify the type of FFT function used to
        generate the input array.

        If the input array has been generated from a time-domain complex
        input signal, the mode must be set to 0. Otherwise the mode argument
        must to set to 1 to signify that the input array has been generated
        from a time domain real input signal. For example, mode must be set
        to 0 if the input was generated by one of the following library
        functions:

            cfft_fr16, cfftf_fr16, cfftrad4_fr16

        and mode must be set 1 if the input was generated by one of:

            rfft_fr16, rfftrad4_fr16

        The block_exponent argument is used to control the normalization of
        the power spectrum. It will usually be set to the block_exponent that
        is returned by the cfft_fr16 and rfft_fr16 functions. If on the other
        hand the input array was generated by one of the functions cfftf_fr16,
        cfftrad4_fr16 or rfftrad4_fr16 then the block_exponent argument should
        be set to -1, which indicates that the input array was generated using
        static scaling.

        If the input array was generated by some other means, then the
        value specified for the block_exponent argument will depend upon
        how the FFT was calculated. If the function used to calculate the
        FFT did not scale the intermediate results at any of the stages
        of the computation, then set block_exponent to zero; if the FFT
        function scaled the intermediate results at each stage of the
        computation, then set block_exponent to -1; otherwise set
        block_exponent to the number of computation stages that did scale
        the intermediate results (this value will be in the range 0 to
        log2(fft_size)).

        Note: Functions that compute an FFT using fixed-point arithmetic
              will usually scale a set of intermediate results to avoid the
              arithmetic from generating any saturated results. Refer to the
              description of the cfft_fr16 or rfft_fr16 functions for more
              information about different scaling methods.

        The fft_magnitude_fr16 function writes the power spectrum to the
        output array output. If mode is set to 0, then the length of the
        power spectrum will be fft_size. If mode is set to 1, then the
        length of the power spectrum will be ((fft_size/2)+1).

    Error Conditions:

        The function fft_magnitude_fr16 exits without modifying the output
        vector if any of the following conditions are true:

            - fft_size is less than 2,
            - the mode argument is set to a value other than 0 or 1,
            - block_exponent contains a value less than -1,
            - block exponent is greater than 0 and the following condition
              is not true:

                  fft_size >= (1 << block_exponent)

    Algorithm:

        If the input array was generated using a complex input signal (and
        thus the output contains unique negative and positive frequency
        components):

            fft_magnitude[i] = sqrt(input[i].re^2 + input[i].im^2) / fft_size

                where:  i = [0 ... fft_size)

        If the input array was generated using a real input signal (and thus
        the output contains negative and positive frequency components that
        are symmetrical around DC), a different formula applies. Taking
        advantage of the Hermitian symmetry one can discard the negative
        frequency components. Doing so necessitates applying a correction
        factor of 2 to the above formula (see the Note below) :

            fft_magnitude[i] = 2*sqrt(input[i].re^2 + input[i].im^2) / fft_size

                where:  i = [0 .. fft_size/2]

        Note: Strictly speaking, the DC component and the Nyquist frequency
              component should not be scaled. This is however a valid
              simplification of the algorithm since these signals are
              rarely of interest when analysing real world signals.

    Implementation:

        The function makes use of the shr_fr1x16 and norm_fr1x32 functions
        (defined in fract_math.h) as well as the cabs_fr16 function (defined
        in complex.h).

        Instead of a division by fft_size, the output from the cabs_fr16
        function is shifted right by log2(fft_size). Using the shr_fr1x16
        function to perform the shift operation ensures that in cases where
        mode==1 (rfft) and (log2(fft_size) - block_exponent)==0 the resulting
        left shift will saturate.

        The table below illustrates how normalizing the FFT magnitude is
        affected by the various scaling modes available in the cfft_fr16
        and rfft_fr16 functions:

        Static Scaling
        --------------

            CFFT generated input:  Mag(i) =   |input[i]|
            RFFT generated input:  Mag(i) = 2*|input[i]|

            block_exponent = log2 (fft_size), and so the input has
            already been scaled by fft_size and the magnitude will
            therefore not need to be scaled

        No Scaling
        ----------

            CFFT generated input:  Mag(i) =    |input[i]| /fft_size
            RFFT generated input:  Mag(i) = (2*|input[i]|)/fft_size

            block_exponent = 0, and so the magnitude should be scaled by
            fft_size

        Dynamic Scaling
        ---------------

            CFFT generated input:  Mag(i) =    |input[i]| /(2^scale_exponent)
            RFFT generated input:  Mag(i) = (2*|input[i]|)/(2^scale_exponent)
                                          =    |input[i]|/(2^(scale_exponent-1))

                where scale_exponent = log2(fft_size) - block_exponent

            block_exponent will be in the range [0 - log2(fft_size)], and
            so the input may have been (partially) scaled beforehand and
            the magnitude may need to be scaled as follows, assuming an
            FFT size of 16:

                log2(fft_size):    4    4    4    4    4
                block_exponent:    0    1    2    3    4
                scale_exponent:    4    3    2    1    0

        For reference:
            log2(fft_size) = sizeof(int) - leading signbits - 1 (actual bit set)
                           = 32 - (norm_fr1x32(fft_size) + 1) - 1
                           = 32 -  norm_fr1x32(fft_size) - 2

            So log2(fft_size=16) = 32 - 26 - 2
                                 = 4

    Example:

        #include <filter.h>

        #define N_FFT 1024

        #pragma align 4096
        complex_fract16   cplx_signal[N_FFT];

        fract16           real_signal[N_FFT];
        complex_fract16   fft_output[N_FFT];
        complex_fract16   twiddle_table[N_FFT];

        fract16           real_magnitude[(N_FFT/2)+1];
        fract16           cplx_magnitude[N_FFT];

        int block_exponent;


        twidfftrad2_fr16 (twiddle_table, N_FFT);

        rfft_fr16 (real_signal, fft_output, twiddle_table,
                   1, N_FFT, &block_exponent, 2);

        fft_magnitude_fr16 (fft_output, real_magnitude,
                   N_FFT, block_exponent, 1);


        twidfftf_fr16 (twiddle_table, N_FFT);

        cfftf_fr16 (cplx_signal, fft_output, twiddle_table,
                    1, N_FFT);

        fft_magnitude_fr16 (fft_output, cplx_magnitude,
                    N_FFT, -1, 0);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup = filter.h")
#pragma file_attr("libFunc  = fft_magnitude_fr16")
#pragma file_attr("libFunc  = __fft_magnitude_fr16")
#pragma file_attr("libName  = libdsp")
#pragma file_attr("prefersMem = internal")
#pragma file_attr("prefersMemNum = 30")
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
#include <complex.h>
#include <fract_math.h>

extern void
fft_magnitude_fr16 (const complex_fract16  _input[],
                    fract16                _output[],
                    int                    _fft_size,
                    int                    _block_exponent,
                    int                    _mode)
{
    /* local copies of the arguments */
    const complex_fract16 *const  input  = _input;
    fract16 *__restrict             output = _output;

    const int  fft_size = _fft_size;
    const int  mode = _mode;
    int        block_exponent = _block_exponent;

    short scale_exponent;

    int   pow2_block_exponent = shl_fr1x32 (0x1, (short) block_exponent);
    int   pow2_fft_size;

    int   max_i = shr_fr1x32 (fft_size, (short) mode) + mode;
    int   i;

    if ((fft_size < 2) || (block_exponent < -1)
       || (( mode < 0) || (mode > 1))
       || (fft_size < pow2_block_exponent))
    {
       return;
    }

    pow2_fft_size = norm_fr1x32 (fft_size);
    pow2_fft_size = 32 - pow2_fft_size - 2;

    if (block_exponent == -1)
    {
       block_exponent = pow2_fft_size;
    }

    /* Determine the shift magnitude

       For RFFT generated input, decrement the shift magnitude by one
       (equivalent to a multiplication by 2)
    */
    scale_exponent = (short) (pow2_fft_size - block_exponent - mode);

#pragma different_banks
#pragma loop_count(2,,)
    /* Compute FFT magnitude
    ** for mode = 0, iterate fft_size times
    ** for mode = 1, iterate (fft_size/2)+1 times 
    */
    for (i = 0; i < max_i; i++)
    {
       output[i] = shr_fr1x16 (cabs_fr16 (input[i]), scale_exponent);
    }
}

/* End of File */

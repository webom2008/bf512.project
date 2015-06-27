/************************************************************************
 *
 * autocorr_fr32.c : $Revision: 1.3.6.1 $
 *
 * (c) Copyright 2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: autocorr_fr32 - Auto correlation

    Synopsis:

        #include <stats.h>
        void autocorr_fr32 (const fract32 samples[],
                            int           sample_length,
                            int           lags,
                            fract32       correlation[]);

    Description:

        The autocorr_fr32 function performs an autocorrelation of a signal.
        Autocorrelation is the cross-correlation of a signal with a copy of
        itself. It provides information about the time variation of the signal.
        The signal to be autocorrelated is given by the input array samples[].
        The number of samples of the auto-correlation sequence to be produced
        is given by lags. The length of the input sequence is given by
        sample_length.

        Autocorrelation is used in digital signal processing applications such
        as speech analysis.

    Error Conditions:

        The autocorr_fr32 function is an effective no-op if the
        parameter sample_length or lags is less than or equal to zero.

    Algorithm:

        correlation[k] = (1 / n) * Sigma(i = 0 to n-k-1) (a[i] * a[i+k])

        where k = 0 to lags-1
              n = sample_length
              a = samples

    Implementation:

        i)  According to the above algorithm, the calculation of each element
            of the output vector involves a division by the number of samples.
            A division operation can be expensive depending upon the values of
            its operands, and so the division operation is replaced by a
            multiply operation using a reciprocal instead, as follows:

            - the reciprocal of an integer (i.e. 1/N) cannot be expressed as
              an integer because it is always less than 1.0;

            - therefore the function calculates (1 * SCALE)/N; however this
              expression may be less than 1.0 if N is sufficiently large or
              if SCALE is sufficiently small;

            - to ensure that SCALE is not sufficiently small, SCALE is set to
              0x0000800000000000ULL and so depending upon the value of N, the
              value of the reciprocal will commonly range from 2^31 to 2^48
              and hence all intermediate arithmetic is performed using long
              long arithmetic. However to ensure that overflow does not
              occur unnecessarily, the reciprocal is then scaled to
              approximately 2^31.

            - part of the final calculation must be to undo the effects of the
              scaling that has been described above.

        ii) Makes use of the following optimizing pragmas:

            #pragma extra_loop_loads
            #pragma vector_for

    Example:

        #include <stats.h>
        #define SAMPLES 1024
        #define LAGS      16

        fract32 x[SAMPLES];
        fract32 response[LAGS];

        autocorr_fr32 (x, SAMPLES, LAGS, response);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =stats.h")
#pragma file_attr("libFunc  =autocorr_fr32")
#pragma file_attr("libFunc  =__autocorr_fr32")
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

#pragma diag(suppress:misra_rule_12_7)
/* Suppress Rule 12.7 (required) where bitwise operators shall not be
** applied to operands whose underlying type is signed.
**
** Rationale: To achieve acceptable performance, the function performs
** a multiply-by-reciprocal instead of a fixed-point division operation.
** This requires operands to be scaled, and hence *signed* intermediate
** results have to be 'un-scaled' to achieve correct results. The function
** therefore requires this rule to be suppressed.
*/

#pragma diag(suppress:misra_rule_17_4)
/* Suppress Rule 17.4 (required) which requires that array indexing shall be
** the only allowed form of pointer arithmetic.
**
** Rationale: The offending pointers are local references to external arrays
** referenced by the function's input arguments and it is therefore assumed
** that they will reference a valid range of memory.
*/
#endif

#if !defined(ETSI_SOURCE)
#define ETSI_SOURCE
#endif /* ETSI_SOURCE */

#include <stats.h>
#include <builtins.h>
#include <libetsi.h>

#define SCALE_FACTOR    (0x0000800000000000ULL)

extern void
autocorr_fr32(const fract32 _samples[],
              int           _sample_length,
              int           _lags,
              fract32       _correlation[])
{

    /* local copies of the arguments */

    const fract32 *const samples = _samples;

    const int sample_length = _sample_length;
    const int lags          = _lags;

    fract32 *__restrict correlation = _correlation;

    unsigned long long reciprocal;

    long long   scaled_reciprocal;
    long long   rounding;
    long long   sum;

    unsigned int temp; /* temporary that is set to 1st 32 bits of (SCALE/N) */
    int signbits;      /* number of leading sign bits in (SCALE/N)          */
    int signbits2;     /* number of leading sign bits in sum                */
    int shiftcnt;
    int i, j;

    if ( (lags <= 0) || (sample_length <= 0) )
    {
        return;
    }

    reciprocal = SCALE_FACTOR / (unsigned long long) sample_length;
    temp       = (unsigned int) (reciprocal >> 32);
    signbits   = norm_l ((fract32) temp);
    reciprocal = reciprocal >> (32 - signbits);

    scaled_reciprocal = (long long) reciprocal;

#pragma extra_loop_loads
#pragma vector_for

    for (i = 0; i < lags; i++)
    {
        sum = 0;

#pragma extra_loop_loads
#pragma vector_for

        for (j = 0; j < (sample_length - i); j++)
        {
            sum += (long long) multr_fr1x32x32(samples[j], samples[j+i]);
        }

        /* Multiply the sum by the reciprocal
        **
        ** The magnitude of the reciprocal is approximately 2^31, which
        ** means that if the magnitude of the sum is greater than this
        ** then the product will overflow. Therefore, before multiplying
        ** by the reciprocal, the sum is scaled so that its magnitude is
        ** no greater than 2^31.
        */
        temp      = (unsigned int) (sum >> 32);
        signbits2 = norm_l ((fract32) temp);

        if (signbits2 > 0)
        {
            signbits2 = 32 - signbits2;
            sum = sum >> signbits2;
        }

        sum = sum * (long long)reciprocal;

        /* Apply rounding before adjusting the sum to its proper scale */
        shiftcnt = 15+signbits-signbits2;
        rounding = sum & (1LL << (shiftcnt-1));

        sum = (sum + rounding) >> shiftcnt;

        correlation[i] = (fract32) sum;
    }
}

/* End of File */

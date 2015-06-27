
/****************************************************************************
 *
 * histogram_fr32.c : $Revision: 1.4 $
 *
 * (c) Copyright 2007-2010 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: histogram_fr32 - histogram

    Synopsis:

        #include <stats.h>
        void histogram_fr32 (const fract32 samples[],
                             int           histogram[],
                             fract32       max_sample,
                             fract32       min_sample,
                             int           sample_length,
                             int           bin_count);

    Description:

       The histogram_fr32 function computes a histogram of the input vector
       samples[] that contains sample_length samples, and stores the result
       in the output vector histogram.

       The minimum and maximum value of any input sample is specified by
       min_sample and max_sample, respectively. These values are used by
       the function to calculate the size of each bin as:

               (max_sample - min_sample) / bin_count

               where bin_count is the size of the output vector histogram.

       Any input value that is outside the range [ min_sample, max_sample )
       exceeds the boundaries of the output vector and will be discarded.

    Error Conditions:

        The histogram_fr32 function will return if either sample_length
        or bin_count is less than or equal to zero or if max_sample is
        less than or equal to min_sample.

    Algorithm:

        bin_size = (max_sample - min_sample) / bin_count

        for (sample = next input sample) {
            bin_number = (sample - min_sample) / bin_size
            if ((bin_number >= 0) and
                (bin_number <  bin_count)) {
                histogram_x[bin_number] += 1;
            }
        }

    Implementation:

        Evaluating the expression (max_sample - min_sample) may overflow if
        one uses signed fixed-point arithmetic. One can avoid this overflow
        if one chooses to use signed 64-bit (long long) arithmetic but at
        the expense of performance.

        This function predominantly uses 32-bit unsigned arithmetic, and
        rather than using division to calculate the appropriate bin number,
        it cycles through the histogram looking for the appropriate bin to
        increment. This approach is more cycle-efficient for larger input
        vectors and smaller histograms.

    Example:

        #include <stats.h>
        #define SAMPLES_LENGTH 125
        #define BIN_COUNT      16

        fract32 samples[SAMPLES_LENGTH];
        int histogram[BIN_COUNT];
        fract32 max_sample,min_sample;

        histogram_fr32 (samples,histogram,max_sample,min_sample,SAMPLES_LENGTH,
                        BIN_COUNT);

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr("libGroup =stats.h")
#pragma file_attr("libFunc  =histogram_fr32")
#pragma file_attr("libFunc  =__histogram_fr32")
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

#include <stats.h>
#include <builtins.h>

extern void
histogram_fr32 (const fract32 _samples_x[],
                int           _histogram_x[],
                fract32       _max_sample,
                fract32       _min_sample,
                int           _sample_length,
                int           _bin_count)
{
    const fract32 *const samples_x = _samples_x;
    int *__restrict      histogram_x = _histogram_x;
    const fract32       max_sample = _max_sample;
    const fract32       min_sample = _min_sample;
    const int        sample_length = _sample_length;
    const int           bin_count  = _bin_count;

    unsigned int diff;       /* difference between max_sample and min_sample */
    unsigned int bin_size;   /* number of different values held in each bin  */
    unsigned int bin_number; /* a bin identifier                             */
    unsigned int work;       /* difference between sample and min_sample     */

    fract32 x;               /* current input sample */
    int i;

    if((sample_length <= 0) || (bin_count <= 0) || (max_sample <= min_sample))
    {
        return;
    }

    diff = (unsigned int) max_sample - (unsigned int) min_sample;

    bin_size = diff / (unsigned int) bin_count;
    if(bin_size == 0U)
    {
        bin_size = 1U;
    }

#pragma extra_loop_loads
#pragma vector_for

    for(i = 0; i < bin_count; i++)
    {
        histogram_x[i] = 0;
    }

#pragma extra_loop_loads
#pragma different_banks
#pragma vector_for

    for(i = 0; i < sample_length; i++)
    {
        x = min_fr1x32(samples_x[i],max_sample);
        work = (unsigned int)(x) - (unsigned int) min_sample;
        if(work < diff)
        {
            bin_number = 0U;
            while(work >= bin_size)
            {
                work -= bin_size;
                bin_number++;
            }
            histogram_x[bin_number]++;
        }
    }
}

/* End of File */

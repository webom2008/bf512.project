/*****************************************************************************
 *
 * fl2fr.asm : $Revision: 3542 $
 *
 * (c) Copyright 2006-2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: float_to_fr16 - Convert a floating-point value to a fract16

    Synopsis:

        #include <fract2float_conv.h>

        fract16 float_to_fr16 (float x);
        _Fract  float_to_fx16 (float x);

    Description:

        The float_to_fr16 function converts a single precision, 32-bit IEEE
        value into a fract16 number in 1.15 notation. Floating-point values
        are rounded towards zero.

    Error Conditions:

        Floating-point values that are NaN or +Inf or positive values that
        are greater or equal to 1.0 will be converted to FRACT16_MAX (0x7FFF)

        Floating-point values that are -NaN or -Inf or negative values that
        are less than -1.0 will be converted to FRACT16_MIN (0x8000)

        Floating-point values that are smaller than fabs(x) < 3.0517578125e-5
        will be converted to zero.

    Algorithm:

        The traditional algorithm to convert a floating-point value to 1.15
        fractional notation is:

            (fract16) (x * 32768.0)

        However on Blackfin, floating-point multiplication is relatively
        slow as it is emulated in software, and this basic algorithm does
        not handle out of range results.

    Implementation:

        The implementation is based on extracting the exponent, which is
        a power of two that has been bias'ed by 127, and examining it to
        determine whether it is a regular floating-point number. Zero and
        values that are outside the domain of fract16 numbers are treated
        as a special case - all other values are converted by:

            - isolating the mantissa
            - appending the hidden bit to the mantissa
            - shifting the mantissa into the lower 16 bits based on the
              value of the exponent.

        (Note that the IEEE single precision, 32-bit, representation
        contains 24 bits of precision, made up of a hidden bit and 23
        bits of mantissa, and thus some precision may be lost by converting
        a float to a fract16).

        The following use is made of the registers:

            R0 - the input argument and result
            R1 - the exponent of the input argument or a shift amount
            R2 - various constants

    Example:

        #include <fract2float_conv.h>

        float x = 0.5;
        fract16 fr;
        _Fract  fx;

        fr = float_to_fr16 (x);   /* returns 0x4000 */
        fx = float_to_fx16 (x);   /* returns 0.5r   */

    Cycle Counts:

        28 cycles when the result is within range
        29 cycles when the result is out of range
        29 cycles when the input is 0.0

        These cycle counts were measured using version 5.0.0.45 of the
        ADSP-BF532 Family Simulator; they include the overheads involved
        in calling the library procedure as well as the costs associated
        with argument passing. The cycle counts do not include the costs
        associated with working around any known silicon anomalies.

*****************************************************************************
#endif

#if defined(__SYSCALL_INFO__)
/* System call information */
%regs_clobbered "R0-R2,P1,CCset"
%const
%notes "Converts a single-precision floating-point number to a 1.15 " \
       "fractional number."
%syscall ___float_to_fr16(W=F)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.FILE_ATTR libGroup = floating_point_support;
.FILE_ATTR libGroup = integer_support;
.FILE_ATTR libName  = librt;
.FILE_ATTR libName  = librt_fileio;

.FILE_ATTR libFunc  = float_to_fr16;
.FILE_ATTR libFunc  = _float_to_fr16;
.FILE_ATTR libFunc  = ___float_to_fr16;
.FILE_ATTR FuncName = ___float_to_fr16;

.FILE_ATTR prefersMem    = internal;
.FILE_ATTR prefersMemNum = "30";

#endif

/* Macros */

// EXP_BIAS is the constant that is applied to every binary exponent before
// it becomes part of an IEEE representation. After this has been applied, all
// exponents greater than EXP_BIAS will represent powers of 2 that are +ve,
// and all exponents less than EXP_BIAS will represent powers of 2 that are
// -ve.
#define EXP_BIAS 0x7F


.GLOBAL ___float_to_fr16;

.SECTION/DOUBLEANY program;
.ALIGN 2;

___float_to_fr16:

   /* Extract the exponent from X and check it is not greater than 1.0 */

      R1.L = 0x1708;                // load EXTRACT mask

      // Anomaly 05-00-0209 says that neither register that is read by
      // the EXTRACT instruction may be created by the previous instruction
      // - so work around the anomaly by inserting a useful instruction.
      // (which in this case is the binary exponent of 1.0)
      R2 = EXP_BIAS;

      R1 = EXTRACT(R0,R1.L) (Z);
      CC = R2 <= R1;                // compare exponent of 1.0 and of X

      // The following JUMP instruction jumps away to saturate if
      // abs(x) >= 1.0 and also if the argument is a NaN or Inf
      IF CC JUMP .saturate;

   /* Check if X is (effectively) Zero */

      R2 += (-15);                  // manufacture exponent of the smallest
                                    // representable-in-fract16 floating-point
                                    // value (which is EXP_BIAS-15)

      // If the exponent is less than 112 (EXP_BIAS-15), then either
      // the argument is +0.0 or -0.0 or it is a denormalized number
      // or it is smaller than the smallest floating-point value that
      // can be represented in a fract16 - in all four cases the function
      // will return zero
      CC = R1 < R2;
      IF CC JUMP .return_zero;

      CC = R0 < 0;                  // set CC if the argument is negative

   /* X is a regular value - convert it to fract16 */

      BITSET(R0,23);                // add the hidden bit to the mantissa
      R0 <<= 8;                     // shift the hidden bit to MSB and lose
                                    // the sign and rest of the exponent

      R2 = EXP_BIAS + 16;
      R1 = R1 - R2;                 // bias'ed exponent - EXP_BIAS - 16
      R0 = LSHIFT R0 BY R1.L;       // shift the mantissa right by 16 and
                                    // then by the amount represented by the
                                    // unbiased exponent

      R1 = -R0;                     // get a negative version of the result
      IF CC R0 = R1;                // and use this if the argument was -ve

      RTS;

.saturate:
   /* Handle extremes via saturation */

      R0 >>>= 31;                   // propagate sign bit everywhere
      BITTGL(R0,31);                // create either 0x80000000 or 0x7FFFFFFF
      R0 = ~ R0;                    // swap to the opposite one

      R0 >>>= 16;                   // truncate a fract32 to a fract16
      RTS;

.return_zero:
   /* Return a Zero */

      R0 = 0;
      RTS;

.___float_to_fr16.end:

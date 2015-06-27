/*****************************************************************************
 *
 * ld2dfr.asm : $Revision: 1.2 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: long_double_to_fr32 - Convert a long double to a fract32

    Synopsis:

        #include <fract2float_conv.h>
        fract32 long_double_to_fr32 (long double x);

        long _Fract __float64_to_fr32 (long double x);

    Description:

        The long_double_to_fr32 function converts a double precision, 64-bit
        IEEE value into a fract32 number in 1.31 notation.

        The __float64_to_fr32 function is a compiler support routine that
        converts a double precision, 64-bit IEEE value into a long _Fract
        value.

    Error Conditions:

        Floating-point values that cannot be converted to fract32 notation
        are handled as follows:

            return 0x7fffffff if x >= 1.0 or  NaN or +Inf
            return 0x80000000 if x < -1.0 or -NaN or -Inf
            return 0          if fabs(x) < 4.656612873077393e-10

    Implementation Notes:

        Note that the IEEE double precision, 64-bit, representation contains
        53 bits of precision, made up of a hidden bit and 52 bits of mantissa,
        and thus some precision may be lost by converting a long double to a
        fract32.

        Refer to the SYSCALL_INFO section below for details of the Clobber Set

    Example:

        #include <fract2float_conv.h>

        long double point5 = 0.5L;
        fract32     half;

        half = long_double_to_fr32 (point5);

    Cycle Counts:

        30 cycles when the result is within range
        28 cycles when the input is 0.0
        35 cycles when the result is out of range

        (Measured for an ADSP-BF532 using version 5.0.0.35 of the ADSP-BF5xx
        Family Simulator and includes the overheads involved in calling
        the library procedure as well as the costs associated with argument
        passing)

*****************************************************************************
#endif

#if defined(__SYSCALL_INFO__)
/* System call information */
%regs_clobbered "R0-R3,P1,CCset"
%const
%notes "Converts a double-precision floating-point number to a 1.31 " \
       "fractional number." \
%syscall ___float64_to_fr32(lW=lD)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.file_attr libGroup = floating_point_support;
.file_attr libGroup = integer_support;
.file_attr libName  = librt;
.file_attr libName  = librt_fileio;

.file_attr libFunc  = long_double_to_fr32;
.file_attr libFunc  = ___long_double_to_fr32;
.file_attr FuncName = ___long_double_to_fr32;

.file_attr libFunc  = float64_to_fr32;
.file_attr libFunc  = ___float64_to_fr32;
.file_attr FuncName = ___float64_to_fr32;

.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";

#endif

.GLOBAL ___long_double_to_fr32;
.TYPE   ___long_double_to_fr32,STT_FUNC;

.GLOBAL ___float64_to_fr32;
.TYPE   ___float64_to_fr32,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

___long_double_to_fr32:
___float64_to_fr32:

   /* Extract the exponent */

      R3.L = 0x140B;

      // Anomaly 05-00-0209 says that neither register that is read by
      // the EXTRACT instruction may be created by the previous instruction
      // - so work around the anomaly by inserting a useful instruction.
      //
      // In this case the useful instruction takes the least significant
      // word (LSW), which contains the least significant 32 bits of the long
      // double mantissa, and shifts it right so that the most significant
      // 11 bits are right-justified in the register - these 11 bits will
      // be concatenated later with the hidden bit and the leading 20 bits
      // of mantissa that are present in the most significant word (MSW)
      R0 >>= 32 - 11;

      R2   = EXTRACT(R1,R3.L) (Z);

   /* Check whether the result is zero
   **
   ** (this includes the case when the long double is a denormalized
   **  number as well as a signed zero, and when the long double is
   **  less than 2^(-32))
   */

      R3 = 0x3FF - 32;          // (0x3FF is the bias'ed exponent for 2^0)
      CC = R2 <= R3;
      IF CC JUMP .zero_return;

   /* Check whether the magnitude is too large for a fract32
   **
   ** (the magnitude will be too large if the exponent is 0x7FF
   **  representing a NaN or Infinity or if the exponent is greater
   **  or equal to the exponent for 1.0 - note that this catches
   **  the case when the long double is -1.0 - and for all these
   **  cases, a saturated result is required).
   */

      R3 = 0x3FF;              // (this is the bias'ed exponent for 1.0)
      R2 = R3 - R2;
      CC = R2 <= 0;
      IF CC JUMP .saturate_return;

   /* Append the hidden bit to the mantissa in MSW */

      BITSET(R1,20);

      // Move the mantissa (now with the hidden bit) to the MSB. This
      // means that the subsequent scaling of the mantissa will always
      // be towards the right
      R3 = R1 << 11;

   /* Incorporate the 11 most significant bits of the mantissa from LSW
   **
   ** (refer above to "Extract the exponent")
   */

      R0 = R3 + R0;

   /* Scale - set the sign - return */

      R0 >>= R2;               // (this is the scale operation)

      CC = R1 < 0;
      R1 = -R0;
      IF CC R0 = R1;

      RTS;

.zero_return:
   /* Return a zero */

      R0 = 0;
      RTS;

.saturate_return:
   /* Handle extremes via saturation */

      CC = R1 <= 0;            // Test whether the parameter is positive

      R0 = R3 << 31;           // Generate 0x80000000 (from 0x3FF)
      R1 = ABS R0;             // Generate 0x7fffffff from 0x80000000
      IF !CC R0 = R1;          // and return 0x7fffffff if the parameter is +ve

      RTS;

___float64_to_fr32.end:
___long_double_to_fr32.end:

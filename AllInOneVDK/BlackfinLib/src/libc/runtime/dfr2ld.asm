/*****************************************************************************
 *
 * dfr2ld.asm : $Revision: 1.2 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: fr32_to_long_double - Convert a fract32 to a long double

    Synopsis:

        #include <fract2float_conv.h>
        long double fr32_to_long_double (fract32 x);

        long double __fr32_to_float64 (long _Fract x);

    Description:

        The fr32_to_long_double function converts a fixed-point, 32-bit
        fractional number in 1.31 notation into a double precision, 64-bit
        IEEE floating-point value.

        The __fr32_to_float64 function is a compiler support routine that
        converts a long _Fract value into a double precision, 64-bit IEEE
        floating-point value.

    Error Conditions:

        There are no error conditions; all inputs are converted to long
        double.

    Algorithm:

        result = ((long double) x) / 2147483648.0L

    Implementation:

        The implementation is based on the compiler support routine that
        converts a signed integer 32-bit value into an IEEE double-precision
        floating point value (___int32_to_float64). The values of 0 and
        FRACT_MIN (0x80000000) are handled as special cases values.

        The function uses a short-cut to add the appropriate sign to the
        result. A typical way of doing this would be to set the MSB of the
        result but only if the original argument was negative. The short-cut
        utilizes the ROT(ate) instruction - it first ensures that CC is set
        if the input argument is -ve - it then performs a 1-bit rotate to
        the right, which rotates the source register by copying the CC
        register to the MSB of the result and copying the LSB of the source
        register into the CC register. However for this short-cut to work
        the exponent must be placed in bits 31-21 and the mantissa in bits
        20-1 (in a long double, the exponent field is bits 30-20 and the
        mantissa field is bits 19-0).

        Refer to the SYSCALL_INFO section below for details of the Clobber Set

    Example:

        #include <fract2float_conv.h>

        fract32     half = 0x40000000;
        long double point5;

        point5 = fr32_to_long_double (point5);

    Cycle Counts:

        29 cycles when the input is 0.0
        29 cycles when the input is FRACT_MIN (0x80000000)
        30 cycles for all other inputs

        (Measured for an ADSP-BF532 using version 5.0.0.35 of the ADSP-BF5xx
        Family Simulator and includes the overheads involved in calling
        the library procedure as well as the costs associated with argument
        passing)

*****************************************************************************
#endif

#if defined(__SYSCALL_INFO__)
/* System call information */
%regs_clobbered "R0-R2,P1,CCset"
%const
%notes "Converts a 1.31 fractional number to a double-precision " \
       "floating-point number."
%syscall ___fr32_to_float64(lD=lW)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.file_attr libGroup = floating_point_support;
.file_attr libGroup = integer_support;
.file_attr libName  = librt;
.file_attr libName  = librt_fileio;

.file_attr libFunc  = fr32_to_long_double;
.file_attr libFunc  = ___fr32_to_long_double;
.file_attr FuncName = ___fr32_to_long_double;

.file_attr libFunc  = fr32_to_float64;
.file_attr libFunc  = ___fr32_to_float64;
.file_attr FuncName = ___fr32_to_float64;

.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";

#endif

.GLOBAL ___fr32_to_long_double;
.TYPE   ___fr32_to_long_double,STT_FUNC;

.GLOBAL ___fr32_to_float64;
.TYPE   ___fr32_to_float64,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

___fr32_to_long_double:
___fr32_to_float64:

   /* Check for zero or FRACT32_MIN */

      R1 = R0 << 1;
      CC = AZ;
      IF CC JUMP .special_return;

   /* Set CC if parameter is negative */

      CC = R0 < 0;
      R0 = ABS R0;               // make the parameter positive

   /* Calculate the exponent */

      R1 = 0x3FE;                // (this is the bias'ed exponent of 0.5L)

      // The following instruction is sensitive to anomalies 05-00-0209
      // and 05-00-0127 - to avoid the anomaly, the input to the SIGNBITS
      // instruction must not be created in the preceding instruction
      R2.L = SIGNBITS R0;

      R2 = R2.L (X);
      R1 = R1 - R2;

   /* Normalize the mantissa
   **
   ** and during the normalization (which is a left shift) shift the
   ** leading bit off (this becomes the hidden bit and so needs to be
   ** discarded). This means that the first bit of the mantissa will
   ** be at bit position 31
   */

      R0 <<= R2;                 // (this is the normalization)
      R0 <<= 2;                  // discard the leading bit and Bit 31 is now
                                 // the most significiant bit of the mantissa

   /* Manufacture the MSW
   **
   ** as follows: (1) taking the exponent which is right-justified in R1
   **             (2) shifting so that it is left-justified in R1
   **             (3) shifting the normalized mantissa in R0 to bit position 20
   **             (4) adding (3) and (4) together in R1
   **             (5) use the ROT(ate) instruction to move CC into the sign
   **                 bit position and to move the exponent and the mantissa
   **                 one bit to the right
   */

      R1 <<= 21;
      R2   = R0 >> 11;
      R1   = R1 + R2;
      R1   = ROT R1 BY -1;

   /* Create the LSW and return */

      R0 <<= 20;
      RTS;

.special_return:
   /* Return either zero or -1.0L */

      CC = BITTST(R0,31);        // (CC is set if parameter = 0x80000000)
      R0 = 0;                    // (R0 may have had 0x80000000)

      R1.H = 0xBFF0;             // Exponent and sign for -1.0L
      IF !CC R1 = R0;

      RTS;

___fr32_to_float64.end:
___fr32_to_long_double.end:

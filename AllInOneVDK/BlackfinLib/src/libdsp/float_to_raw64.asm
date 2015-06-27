/*****************************************************************************
 *
 * float_to_raw64.asm : $Revision: 1.1 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: float_to_raw64 - Convert float to a variable-precision long long

    Synopsis:

        #include "xutil_fr.h"
        long long float_to_raw64 (float x,
                                  unsigned int binary_decimal_digits);

    Description:

        The float_to_raw64 function is a support routine.

        It converts the parameter x from IEEE single-precision floating-
        point value to a fixed-point 64-bit integer; the number of binary
        digits to the right of the decimal point is specified by the
        parameter binary_decimal_digits. For example if the number of
        places after the decimal point is specified as zero, then this
        function will convert x to a long long; if the number of digits
        after the decimal point is 63, the x will converted into a
        fractional number in 1:63 format.

        Any bits in the floating-point value that are too small to be
        represented in the result will be truncated - rounding is not
        performed by this function.

    Error Conditions:

        If the value is too small to be represented given the specified
        number of binary digits after the decimal (binary) point, then
        the support function will return zero; if the value is too large
        then the supoprt function will return a saturated result (with
        the appropriate sign).

    Algorithm

        #include <math.h>

        float x;
        long long ll = ldexpf (x,-digits_after_binary_point);

    Implementation:

        Each iteration of the loop involves calling the comparison function

        An IEEE single precision, 32-bit, floating-point number has three
        fields:

              1      8                     23
            ----------------------------------------------------
            | S | Exponent |            Mantissa               |
            ----------------------------------------------------

        The support function:

            [1] records the sign bit,
            [2] extracts the exponent,
            [3] shifts the mantissa to the left as follows:

                  1            23                         8
                ----------------------------------------------------
                | H |        Mantissa            | 0 0 0 0 0 0 0 0 |
                ----------------------------------------------------

                where H represents the hidden bit (and is 1 of course)

            [4] uses the exponent and the number of binary digits to the
                right of the decimal point to calculate how much [3] should
                be shifted right (which may be more that 31 bits)

                The formula for the number of right shifts required is:

                    Bias applied to an IEEE exponent
                  + Number of shifts required if [3] represents the value 1.0
                  - Binary exponent of x
                  - Number of binary digits after the decimal point

                =>  127
                  +  63
                  - exponent
                  - binary_decimal_digits

            [5] creates the MSW of the result by shifting [3] by [4]
                (if [4] is greater than 30 then the MSW will be zero
            [6] creates the LSW of the result by shifting [3] to the
                left by 32 - [4] (the LSW will be zero if [4] is
                greater than 8)
            [7] examines [1] to determine the correct sign of the result.

    Example:

        #include "xutil_fr.h"

        #define DIGITS_IN_DECIMAL_PART 60U

        float     pi = 3.1415927F;
        long long pi_ll;

        /* convert pi into fixed-point 4:60 format */

        pi_ll = float_to_raw64 (pi,DIGITS_IN_DECIMAL_PART);

    Cycle Counts:

        33 cycles if float_to_raw64 returns a saturated result
        42 cycles if float_to_raw64 returns a zero
        48 cycles if float_to_raw64 returns a negative result
        35 cycles if float_to_raw64 returns a positive result

	The cycle counts were measured using version 5.0.0.45 of the
        ADSP-BF5xx Blackfin Family Simulators; they include the overheads
        involved in calling the support routine as well as the costs
        associated with argument passing).

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.file_attr FuncName      = __float_to_raw64;
.file_attr libName       = libdsp;

.file_attr libGroup      = integer_support;
.file_attr libGroup      = floating_point_support;
.file_attr libFunc       = _float_to_raw64;
.file_attr libFunc       = __float_to_raw64;

.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";

#endif

/* Macros */

#define EXPONENT_BIAS         127

#if defined(__ADSPBLACKFIN__) && !defined(__ADSPLPBLACKFIN__)
/* __ADSPBF535__ core only */
#define CARRY AC
#else
#define CARRY AC0
#endif

.GLOBAL __float_to_raw64;
.TYPE   __float_to_raw64,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

__float_to_raw64:

   /* Extract the exponent */

      R3 = 0x1708;
      P1 = R0;                    // Preserve parameter (the sign in particular)
      R2 = EXTRACT (R0,R3.L) (Z);

   /* Check if the exponent is too large for specified notation */

      R3 = EXPONENT_BIAS + 63;    // == 190
      R3 = R3 - R1;               // == 190 - binary_decimal_digits

      CC = R3 <= R2;              // Compare against exponent of parameter
      IF CC JUMP .saturate;       // Jump if the parameter is too large
      R3 = R3 - R2;               // Scale factor to be applied to mantissa

   /* Move mantissa to Bit 30 and put the hidden bit in Bit 31 */

      R2 = R0 << 8;
      BITSET (R2,BITPOS(0x80000000));      // see [3] under Implementation

   /* Create the MSW */

#if defined(__ADSPBLACKFIN__) && !defined(__ADSPLPBLACKFIN__)
/* __ADSPBF535__ core only */
      R1 = -R3;
      R1 = LSHIFT R2 BY R1.L;
#else

      // create the MSW by shifting it to the right by the scale factor
      // in R3 - use the instruction form Dreg >>= Dreg for two reasons:
      //
      //    [a] the scale factor does not have to be negated for right shift
      //    [b] all 32-bits of the shift magnitude determine how much to shift
      R1 = R2;
      R1 >>= R3;
#endif

   /* Check if the exponent is too small for the specified notation */

      R0 = 32;
      R3 = R0 - R3;

      R0 = -31;
      CC = R3 < R0;
      IF CC JUMP .zero;

      R0 = LSHIFT R2 BY R3.L;

   /* Negate if Appropriate */

      CC = P1 < 0;
      IF CC JUMP .negate;

      RTS;

.negate:
   /* Return -result */

      R0 = - R0;
      CC = CARRY;
      R1 = ~ R1;
      R2 = CC;
      R1 = R1 + R2;
      RTS;

.zero:
   /* Return 0x0 */

      R0 = 0;
      R1 = 0;
      RTS;

.saturate:
   /* Return one of FRACT64_MAX or FRACT64_MIN */

      R1 = R0 << 9 (S);   // Will saturate to 0x7FFFFFFF if +ve,
                          // or 0x80000000 if -ve
      R0 = R1;
      BITTGL (R0, 31);    // If R1 = 0x7FFFFFFF => R0 = 0xFFFFFFFF,
                          // else R1 = 0x80000000 => R1 = 0x00000000

      RTS;

.__float_to_raw64.end:

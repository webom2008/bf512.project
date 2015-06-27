/****************************************************************************
 *
 * f32toi32z.asm : $Revision: 3543 $
 *
 * (c) Copyright 2003-2008 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: FLOAT_TO_INT - Convert a float to int

    Synopsis:

        int __float32_to_int32_round_to_zero (float x);

    Description:

        This function converts a single precision, 32-bit, IEEE value
        into a 32-bit signed integer.

        The IEEE single precision, 32-bit, representation contains 24 bits
        of precision, made up of a hidden bit and 23 bits of mantissa,
        and therefore this will be the maximum precision in the result
        that the function returns.

    Error Conditions:

        The IEEE standard (ANSI/IEEE Std 754-1985) states that an Invalid
        Operation exception shall be raised if the floating-point value
        cannot be converted to an alternative format and, if no trap occurs,
        the result shall be a quiet NaN provided that the target is
        floating-point (see Section 7.1 Invalid Operation).

        If the floating-point value x cannot be converted to integer then
        this function will not raise an exception - instead it will handle
        the invalid number as follows:

            Return 0x7fffffff if x > INT_MAX or =  NaN or = +Inf
            Return 0x80000000 if x < INT_MIN or = -NaN or = -Inf
            Return 0          if x is a denormalized number

        Note that the IEEE standard ignores the sign of a NaN (see
        Section 6.3 The Sign Bit) and therefore strictly this function
        should not distinguish between +NaN and -NaN. Use the switch
        -ieee-fp if INT_MAX should be returned for both +NaN and -NaN.

    Implementation:

        Converting to an integer involves:

            - Extracting the binary exponent;
            - Examining the exponent to check the magnitude of the float;
            - If the exponent corresponds to a value between 2^0 and 2^31,
              then the float can be represented by a 32-bit integer;
            - Extracting the mantissa from the float;
            - Applying the hidden bit to the mantissa;
            - Shifting the mantissa according to the value of
              the exponent;
            _ Negating the result if appropriate.

        A float cannot be represented as an int if its value is:

            +Infinity    (0x7F800000)
            -Infinity    (0xFF800000)
             NaN         (0x7Fxxxxxx for (x > 0x800000))
                      or (0xFFxxxxxx for (x > 0x800000))
            +denormlised (0x00xxxxxx for (x < 0x800000))
            -denormlised (0x80xxxxxx for (x < 0x800000))
            > INT_MAX    (0x4F000000)
            < INT_MIN    (0xCFyyyyyy for (yyyyyy>0))

	In all of the above cases, the exponent is either zero or is
        greater than or equal to: Exponent Bias + 31
                                  = 127 + 31
                                  = 158
                                  = 0x9E
        Registers Used:

            R0 - the input argument
            R1 - various constants
            R2 - the exponent of the input argument or a shift amount
            R3 - the mantissa of the input argument

    Cycle Counts:

        31 cycles when the result is within range
        29 cycles when the result is out of range or is zero

        (Measured for an ADSP-BF532 using version 5.0.0.35 of the
        ADSP-BF5xx Blackfin Family Simulators and includes the overheads
        involved in calling the function as well as the costs associated
        with argument passing).

*****************************************************************************
#endif

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "1fs"
%notes "Converts a single-precision floating-point number to a 32-bit integer."
/* If you change this clobber set rembember to change SOFTFLOAT */
%regs_clobbered "R0-R3,P1,CCset"
%const
%syscall ___float32_to_int32_round_to_zero(I=F)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.file_attr libGroup = floating_point_support;
.file_attr libGroup = integer_support;
.file_attr libName  = libdsp;

.file_attr libFunc  = ___float32_to_int32_round_to_zero;
.file_attr FuncName = ___float32_to_int32_round_to_zero;

.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";

#endif

/* Macros */

#define SIGNBIT_MASK     0x80000000
#define SIGNBIT_POS      BITPOS(SIGNBIT_MASK)

#define HIDDENBIT_MASK   0x00800000
#define HIDDENBIT_POS    BITPOS(HIDDENBIT_MASK)

// EXP_BIAS
// is the constant that is applied to every binary exponent before it
// becomes part of an IEEE representation. After this has been applied,
// all exponents greater than EXP_BIAS will represent powers of 2 that
// are +ve, and all exponents less than EXP_BIAS will represent powers
// of 2 that are -ve.

#define EXP_BIAS         0x7F

// EXP_CONST
// The exponent represents the scale of the most significant bit of the
// mantissa; the scale of the least significant bit can be determined by
// subtracting this constant from the exponent. It is set to 23 for
// single-precision floating-point numbers because there are 23 bits
// in the mantissa.

#define EXP_CONST        23

// EXP_ADJUST
// When this constant is subtracted from a (raw) exponent, we are
// effectively determining the scale of the least significant bit of
// the result. We then use this value to shift the floating-point
// mantissa into a 32-bit integer value.
//

#define EXP_ADJUST      (EXP_BIAS + EXP_CONST)

.GLOBAL  ___float32_to_int32_round_to_zero;
.TYPE    ___float32_to_int32_round_to_zero,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

___float32_to_int32_round_to_zero:

    /*  Extract the exponent and check the magnitude against INT_MIN */

        R1.L = 0x1708;              // load EXTRACT mask

        // Anomaly 05-00-0209 says that neither register that is read by
        // the EXTRACT instruction may be created by the previous instruction
        // - so work around the anomaly by inserting a useful instruction.
        // (which in this case is the binary exponent of (float) MIN_INT)
        R3 = (EXP_BIAS + 31);

        R2 = EXTRACT(R0,R1.L) (Z);
        CC = R3 <= R2;              // compare exponent of INT_MIN and of X

        IF CC JUMP .saturate;       // jump to saturate if abs(X) >= MIN_INT

        // Note: we will also jump if the argument is a NaN or Inf

    /*  Check if X is (effectively) Zero */

        CC = R2 == 0;               // Test if the exponent is zero
        IF CC JUMP .return_zero;    // If the exponent is zero, then either
                                    // the argument is +0.0 or -0.0 or a
                                    // denormalized number - in all three
                                    // cases we will return zero.

    /*  Determine Shift Amount for the Mantissa */

        R1 = EXP_ADJUST;
        R2 = R2 - R1;    // shift amount = exponent - EXP_ADJUST

        R3.L = 0x0017;   // This is initializing the 2nd operand for the
                         // EXTRACT instruction below - it has been moved
                         // away from the EXTRACT instruction to avoid
                         // Anomaly 05-00-0209 which requires that the
                         // instruction that precedes the EXTRACT instruction
                         // must not create one of its operands

        R1 = -(EXP_CONST+1);
        R2 = MAX(R2,R1); // Clip the minimum shift amount to -24 (if the shift
                         // amount is less than this then the correct return
                         // value will be zero).
                         //
                         // Note that we have already determined that the
                         // magnitude of the argument is less than MAX_INT,
                         // and therefore any positive shift amount will be
                         // a small (<9) value

    /*  Extract the Mantissa from X */

        R3 = EXTRACT(R0,R3.L) (Z);
        BITSET(R3,HIDDENBIT_POS);   // add the hidden bit to the mantissa

    /*  and Apply the Shift Amount */

        CC = R0 < 0;                // Set CC if the argument is negative
        R0 = ASHIFT R3 BY R2.L (S);

        R1 = -R0;
        IF CC R0 = R1;
        RTS;

.saturate:

    /* Handle Extremes via Saturation */

        R0 >>>= 31;             // -ve -> 0xFFFFFFFF / +ve -> 0x00000000
        BITTGL(R0,SIGNBIT_POS); // -ve -> 0x7FFFFFFF / +ve -> 0x80000000
        R0 = ~ R0;              // -ve -> 0x80000000 / +ve -> 0x7FFFFFFF

        RTS;

.return_zero:

    /* Return a Zero */

        R0 = 0;
        RTS;

.___float32_to_int32_round_to_zero.end:

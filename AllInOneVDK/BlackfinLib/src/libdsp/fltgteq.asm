/*
** Copyright (C) 2005-2010 Analog Devices, Inc. All Rights Reserved.
**
** This is the internal function implementing IEEE single-precision
** floating-point greater than or equal comparison. This function is
** for compiler internal use only. It does not follow the normal C ABI.
**
** int __float32_adi_gteq(float X, float Y)
**
**   X is received in R0.
**   Y is received in R1.
**   CC contains the result and is 1 if neither input is a NaN and X>=Y,
**   or 0 otherwise.
**
** Cycle Count:  29 cycles
**
** (Measured for an ADSP-BF532 using version 5.0.0.69 of the ADSP-BF5xx
**  Family Simulator and includes the overheads involved in calling
**  the internal procedure as well as the costs associated with argument
**  passing; the cycle counts do not include the costs associated with
**  working around any known silicon anomalies).
*/

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12fcmp >="
%notes "Single-precision floating-point greater-than-or-equal-to comparison."
%regs_clobbered "R0-R3,P1,CCset"
%const
%return_in_cc
%syscall ___float32_adi_gteq(I=FF)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libGroup      = floating_point_support;
.file_attr libName       = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc       = ___float32_adi_gteq;
.file_attr FuncName      = ___float32_adi_gteq;
#endif

.section program;
.align 2;

___float32_adi_gteq:

    // Can treat the floats as integers (they have a sign bit,
    // tiny exponents are smaller than huge exponents).
    // Increasing positive floats become increasing positive
    // integers, and increasing negative floats become decreasing
    // negative integers.  We can flip the negatives around by
    // removing the sign bit and negating (two-complement), and
    // then we have all numbers increasing on the one scale.
    // Note that -0 and +0 both map to integer zero.

    // Convert X and Y from signed-magnitude to twos-complement form.
    R0 = ROT R0 BY 1;   // Detect sign of X
    R0 = R0 >> 1;       // R0 = X with sign bit removed
    R2 = -R0;
    IF !CC R2 = R0;     // R2 = R0, negated if X is negative

    R1 = ROT R1 BY 1;   // Detect sign of Y
    R1 = R1 >> 1;       // R1 = Y with sign bit removed
    R3 = -R1;
    IF !CC R3 = R1;     // R3 = R1, negated if Y is negative

    // Now we can just use integer compare to see which is greater.
    // If there turn out to be NaNs in the input, we'll throw this away.
    CC = R3 <= R2;

    // Check for NaNs, which must compare as not-equal,
    // no matter to what they are compared.
    //
    // A NaN has an exponent of 255, and a non-zero
    // mantissa. Sign is irrelevant. We check whether
    // either input is a NaN by comparing the absolute
    // value against 0x7F800000.
    // If the operand is larger, it's got a 255 exponent
    // and non-zero mantissa, hence it's a NaN.

    R1 = MAX(R0,R1);    // If X or Y is NaN, this will pick it
    R0 = 0x0001 (X);
    R0.H = 0x7F80;      // Load 0x7F800001
    R0 = R1 - R0;       // If any input is NaN, force CC to zero.
    CC &= AN;

    RTS;

.___float32_adi_gteq.end:
.type ___float32_adi_gteq, STT_FUNC;
.global ___float32_adi_gteq;

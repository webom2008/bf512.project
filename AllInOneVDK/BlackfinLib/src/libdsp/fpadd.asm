/************************************************************************
 *
 * fpadd.asm : $Revision: 3903 $
 *
 * (c) Copyright 2000-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

//
//  Single-precision IEEE floating-point addition and subtraction.
//  Rounding is to nearest even. Denormals, inf, NaN and -zero are supported.
//
//  Registers used:
//         R0 - X operand and return value
//         R1 - Y operand, clobbered
//         R2, R3 - registers clobbered
//         R6, R7 - registers used but preserved
//         ASTAT - clobbered
//
//  A single-precision (32-bit) float has one sign bit,
//  eight bits of exponent, biased by 127 (0-255, not -128..+127),
//  and twenty-three bits of mantissa.
//  31                                    0
//  seee eeee emmm mmmm mmmm mmmm mmmm mmmm
//
//  Cycle Count for z = ___float32_add (x, y):
//
//      AVG   : 73
//      MIN   : 30
//      MAX   : 103
//
//  (Measured for an ADSP-BF532 using version 5.0.0.70 of the ADSP-BF5xx
//   Family Simulator and includes the overheads involved in calling
//   the library procedure as well as the costs associated with argument
//   passing; the cycle counts do not include the costs associated with
//   working around any known silicon anomalies).
//

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12f+"
%notes "Single-precision floating-point addition."
/* Changing this register set also affects the #pragma regs_clobbered in */
/* softfloat*/
%regs_clobbered "R0-R3,P0-P2,CCset"
%const
%syscall ___float32_add(F=FF)

%rtl "12f-"
%notes "Single-precision floating-point subtraction."
/* Changing this register set also affects the #pragma regs_clobbered in */
/* softfloat*/
%regs_clobbered "R0-R3,P0-P2,CCset"
%const
%syscall ___float32_sub(F=FF)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libGroup = floating_point_support;
.file_attr libName = libdsp;
.file_attr prefersMem = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___float32_add;
.file_attr FuncName = ___float32_add;
.file_attr libFunc = ___float32_sub;
.file_attr FuncName = ___float32_sub;
#endif

#include <sys/anomaly_macros_rtl.h>

#if !defined(__ADSPLPBLACKFIN__)
/* Suppress Assembler Warning ea5504 "Memory store instruction use may trigger
** hardware anomaly 05-00-0165". The run-time libraries are not required to
** workaround this anomaly.
*/
.MESSAGE/SUPPRESS 5504;
#endif

#define MAXBIASEXP 254

.section program;

// C Prototypes:
//      float __float32_sub(float A, float B);
//      float __float32_add(float A, float B);
//
// Inputs:
//      R0 = A
//      R1 = B
// Returns:
//      R0 = A + B
// Reg usage:
//      R7:6, R3:0, ASTAT
// Clobbers:
//      R3:0, ASTAT

.align 2;
.global ___float32_sub;
.type ___float32_sub, STT_FUNC;
___float32_sub:
    BITTGL(R1,31);          // Flip sign bit of B, fall through to add

.align 2;
.global ___float32_add;
.type ___float32_add, STT_FUNC;
___float32_add:

    // STEP 1 - Arrange largest input first
    // Input:   R0 = A, R1 = B as IEEE float
    // Output:  R0 = X = whichever of A and B has greatest magnitude
    //          R1 = Y = whichever of A and B has least magnitude

    R3 = R1 << 1;       // Remove sign bit
    CC = R3 == 0;       // Return if Y == 0
    IF CC JUMP .return_x_nopop;

    R2 = R0 << 1;       // Remove sign bit
    CC = R2 == 0;       // Return if X == 0 (CC must be set when jumping)
    IF CC JUMP .return_y_nopop;

    [--SP] = R0;        // This instruction reserves a word on the stack
                        // in which R6 can be preserved later using an
                        // indirect post-decrement store instruction (which
                        // can be multi-issued - pre-decrement store
                        // instructions cannot be multi-issued)

    CC = R2 < R3 (IU);  // If abs(A) < abs(B)...
    R2 = R0;
    IF CC R0 = R1;      // ...then swap R0,R1.
    IF CC R1 = R2;

    // STEP 2 - Unpack inputs and handle special cases
    //          extract mantissas and exponents
    // Input:   R0,R1 from step 1
    // Output:  R2 = X.exp, R3 = Y.exp
    //          R6 = X.mant, R7 = Y.mant

    // Extract and compare the two exponents. Since there are
    // 23 bits of mantissa, if the difference between exponents (D)
    // is greater than 24, the operand with the smaller exponent
    // is too insignificant to affect the other. If the difference
    // is exactly 24, the 24th (hidden) bit will be shifted into the
    // R position for rounding, and so can still affect the result.
    // (R is the most significant bit of the remainder, which is
    // all the bits shifted off when adjusting exponents to match)
    //
    // The mantissas are expanded as:
    //          31                            0
    //          s s s s 1.m m m m ... m z z z z
    // z - The four LSB are zeros.
    // m - The 23 mantissa bits are copied from the input.
    // 1 - The implied one is added.
    // . - The point comes below bit 27.
    // s - If the input is negative, the entire expanded value is negated,
    //     resulting in four sign bits. Two of these reserve space for
    //     sticky bits which are shifted in later, and another allows a
    //     one-bit overflow during addition.

    // Extract the exponents, still with bias. Also use multi-issue
    // instructions to preserve R6 and R7
    R3.L = 0x1708;      // Bit field 8 bits long starting at bit 23.
    R2 = EXTRACT (R0, R3.L) (Z) || [SP--] = R6;
    R3 = EXTRACT (R1, R3.L) (Z) || [SP]   = R7;

    R7 = MAXBIASEXP+1;

    // Check for addition of denormals, which will be in Y
    // because Y is the smaller operand.
    CC = R3;            // Special handling if exponent of Y is zero.
    IF !CC JUMP .handle_denorm_y;

    // Handle identities where an operand is NaN or inf.
    // As X is the larger operand, if there is an inf or NaN, it will be in X.
    CC = R2 == R7;      // If X is NaN or inf, handle it specially.
    IF CC JUMP .handle_nan_inf_x;

    // At this point, we know X and Y are both normal - that is,
    // not zero, denormal, inf or NaN.

    // Extract the mantissas into bits 28..5 (zero extended), and apply
    // the hidden bit.
    R7 = R1 << 9;       // Extract mantissa of Y first
    R7 >>= 4;
    BITSET(R7,28);
.extract_mantissa_x:
    R6 = R0 << 9;       // Now extract mantissa of X
    R6 >>= 4;
    BITSET(R6,28);
.mantissas_done:        // (Execution jumps here after handling denormals.)

    // If the exponents differ by > 24, we could skip the add and save time.
    // But we expect this to be rare in most apps, so we omit the test because
    // it would slow down the more common case where the difference is <= 24.

    // STEP 3 - add or subtract mantissas "C = X +/- Y"
    // Input:   R0,R1 from step 1; R2,R3,R6,R7 from step 2
    // Output:  R2 = C.exp, R6 = C.mant (including sticky bit)

    // Convert the Y mantissa to signed
    // If X and Y have the same sign, we want to compute X + Y.  But if they
    // have opposite sign, we want X + -Y, so we negate Y.
    R1 = R0 ^ R1;       // Compare signs
    CC = R1 < 0;        // If different...
    R1 = -R7;           // (Prepare -Y)
    IF CC R7 = R1;      // ...then negate Y.

    // Now downshift one of the mantissas, so that its point aligns with
    // the other (the value with smaller exponent is shifted, until they
    // have the same exponent).
    // Some bits of Y get shifted beyond the LSB. These are converted
    // into a 'sticky' bit, which contains the logical OR of all bits
    // lost, and is shifted into the least significant bit of the sum.
    // Note that the result is always positive, or zero.
    // Difference in exponents is not constrained - this works for all.

    // Align the mantissas and add
    R3 = R2 - R3;       // Shift Y to align with X.

#ifdef __WORKAROUND_SHIFT
    // The shift magnitude can exceed the range supported.
    R1 = 31;
    R3 = MIN (R1, R3);
#endif

    // Duplicate Y.mant
    R1 = R7;

    R1 >>>= R3;

    R6 = R6 + R1;       // Add.

    // Check for zero.  Zero here means the result after rounding and
    // normalization will still be zero.
    CC = R6;
    IF !CC JUMP .return_zero;

    // Detect loss of precision and set sticky bit.
    R1 <<= R3;
    CC = R1 < R7 (IU);
    R6 = ROT R6 BY 1;

    // STEP 4 - Normalize
    // Input:   R2,R6 from step 3
    // Output:  R2 = C.exp updated
    //          R3 = C.mant normalized in 25 MSB, plus round and sticky bits

    // After rotating in the sticky bit, there are nominally two sign
    // bits and the point is below bit 29.  If there are not two sign bits,
    // we need to adjust the exponent of the result.  We normalize the
    // mantissa as well, but with one less sign bit thus ensuring we only
    // do left shifts.  A right shift would lose the sticky bit.
    //
    // If we need to normalize by only one bit, then we can shift it in
    // from the sum.  If we need more, we know that all the low order bits
    // we shift in (after the first) must be zero, because the only way to
    // get such a large shift is if the the X and Y inputs are almost equal.
    // It follows that when we aligned the points for adding, neither input
    // was shifted by more than one bit, so there are no low order bits to
    // recover now.
    //
    // The sticky bit is constructed to be zero whenever a normalizing shift
    // of more than one bit is required, ensuring zeros are shifted in.
    //
    // Therefore, one of the following always applies:
    // - we need to normalize by more than 1 bit, and the 6 LSB of the sum
    //   correctly represent the bits to shift in. (Bit 5 contains
    //   numerical data; the other bits are zero.) Or,
    // - we need to normalize by zero or one bits, and bits 5 to 1 contain
    //   numerical data, bit 0 is sticky.

    // Normalize result so there is only one sign bit (in the MSB)
    R7 = MAXBIASEXP+1;      // (Insert this instruction to avoid silicon
                            // anomaly 05000209 ("Speed Path in Computational
                            // Unit Affects Certain Instructions") by
                            // separating the following SIGNBITS instruction
                            // from the instruction that created its operand.)
    R3.L = SIGNBITS R6;     // Returns #sign bits - 1
                            // IMPORTANT NOTE: R3.H is assumed to be 0 as R3
                            // contains the exponent of Y

    CC = R2 < R3;           // Jump if the output is too small to normalize
    IF CC JUMP .handle_denorm_result;
    R2 = R2 - R3;           // Decrease exponent by shift amount
    R3 = LSHIFT R6 BY R3.L; // Shift mantissa into place

    // Adjust exponent for changed bit position.
    // The mantissa is nominally in bits 29..7, but we just normalized
    // it into bits 30..8, causing a phantom difference of one.  Remove that.
    R2 += 1;

    // Check for overflow
    CC = R7 <= R2;          // (R7 is MAXBIASEXP+1)
    IF CC JUMP .return_inf_signed;

    // STEP 5 - unbiased round
    // Input:   R3 = C.mant from step 4
    // Output:  R3 updated, and bit 30 indicates overflow

    // Now rounding. We round up or down if there's a clear
    // choice (remainder greater or less than 0.5). If remainder
    // is exactly 0.5, then we round to nearest even value of sum.
    // The MSB of the remainder is called R. Has value 0.5 or 0.
    // The LSB of the sum is called G (guard bit).
    // The rest of the remainder is all ORed together, and is
    // called S ("sticky").

    // At this point, the mantissa includes a sign bit (bit 31, always zero)
    // and a hidden bit (bit 30), and is in the 25 MSB of R3.

    BITCLR (R3, 30);    // Remove the hidden bit, ready for packing.
.back_from_denorm:

    // Round to nearest even
    R7 = 0x40;          // 1/2 LSB of mantissa in R3, used for rounding
    R1 = R3 + R7;       // Precalc mantissa with biased rounding
                        //   If the mantissa overflows, giving 0x40000000, we
                        //   must increment the exponent later.
    R6 = R3 << 24;      // Get the 8 LSB...
    R6 >>= 24;
    CC = R6 == R7;      // Test if mantissa is exactly even plus a half.
    IF !CC R3 = R1;     // Apply biased rounding if not, do nothing if it is.

    // STEP 6 - pack result
    // Input:   R0 from step 1; R2 from step 3; R3 from step 4
    // Output:  R0 = c as IEEE float

    // At this point, the sum is in the 25 MSB of R3, the IEEE hidden bit
    // is removed, and there may be overflow due to rounding.

    // Combine mantissa, exponent and sign - and restore R6 and R7
    // using multi-issue instructions
    R3 >>= 6;           // Shift mantissa to bits 23..1

    R0 = ROT R0 BY  1 || R7 = [SP++]; // Rotate out the sign bit from X operand
    R3 = ROT R3 BY -1 || R6 = [SP++]; // Rotate it into the result
                        // (mantissa is now in 23 LSB)
    R2 <<= 23;
    R0 = R3 + R2;       // Add exponent
                    // If rounding overflowed, it now increments the exponent.
                    // If the exponent overflows, the value becomes +/-inf.
    RTS;


    // Y is zero so return X, but for IEEE there's an exception.
    // If X is -0, return zero with the sign of Y (in other words, return Y).
.return_x_nopop:
    R3 = 1;
    R3 <<= 31;          // Load -0.0 (0x80000000)
    CC = R0 == R3;      // Test X

.return_y_nopop:
    IF CC R0 = R1;      // If X == -0, return Y instead (on fall-through)
                        // Return Y if X == 0 and Y != 0 (otherwise)
    RTS;

.handle_denorm_y:       // (R7 contains MAXBIASEXP+1)
    R3 = R1 << 9;       // Remove the sign from Y (which is a denorm)
    // Handle identities where an operand is NaN or inf.
    // As X is the larger operand, if there is an inf or NaN, it will be in X.
    CC = R2 == R7;      // If X is NaN or inf, handle it specially
    IF CC JUMP .handle_nan_inf_x;

    // Extract denormal Y. The mantissa goes to R7 (bits 28..5) and exponent
    // to R3, same as the main execution path.  Note that denormals always
    // have exponent of one, even though the exponent field stored in the
    // float is zero.
    R7 = R3 >> 4;
    R3 = 1 (X);

    // Check if X is denormal.  It can't be zero, because then Y would also
    // be zero, and we've already handled that.
    CC = R2;
    IF CC JUMP .extract_mantissa_x; // If normal, extract as usual

    // Extract denormal X mantissa into R6 and exponent into R2.
    R6 = R0 << 9;
    R6 >>= 4;
    R2 = 1 (X);
    JUMP .mantissas_done;


.handle_nan_inf_x:
    // Return inf or NaN, because X is inf or NaN.
    // Input:   R0 = X, R1 = Y (from step 1)

    // If X is NaN, we will return NaN, always.
    // If X is +/-inf, we will return NaN only if Y is -X, otherwise return X.
    // To put it another way, if Y == -X, return NaN, and otherwise return X.
    // Note that Y cannot be NaN unless X is also NaN, because X is bigger
    // than Y and NaNs are the 'biggest' values.

    R2 = -1;            // R2 = default NaN
    BITTGL(R1,31);      // Calculate -Y
    CC = R0 == R1;
    IF CC R0 = R2;      // If X == -Y, return NaN

    (R7:6) = [SP++];
    RTS;


.handle_denorm_result:
    // Result has underflowed: make mantissa and exponent form a denormal.
    // Input:   R2 = result exponent, R6 = result mantisssa
    // Output:  R2 = updated exponent, R3 = updated mantissa

    // Note that the shift amount (R2.L) is always less than 30 and
    // non-negative, which ensures the lshift works as intended.
    R3 = LSHIFT R6 BY R2.L; // Shift mantissa into place
    R2 = 0;                 // Set exponent
    JUMP .back_from_denorm;


.return_zero:
    // Return 'positive' zero.  The sign bit (MSB of R0) is ignored.
    R0 = 0 (X);
    (R7:6) = [SP++];
    RTS;


.return_inf_signed:
    // Return +/-inf, using the sign of X and use multi-issue instructions
    // to restore R6 and R7
    //
    // Input:   R0 = X (from step 1)
    //          R7 = MAXBIASEXP+1.
    R1 = R7 << 24;                     // Set the exponent
    R0 = ROT R0 BY  1 || R7 = [SP++];  // Rotate out the sign bit from X
    R0 = ROT R1 BY -1 || R6 = [SP++];  // Rotate it into the result

    RTS;

.___float32_add.end:
.___float32_sub.end:

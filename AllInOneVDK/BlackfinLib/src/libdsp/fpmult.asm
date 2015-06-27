/************************************************************************
 *
 * fpmult.asm : $Revision: 3855 $
 *
 * (c) Copyright 2000-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

//
//  Single-precision IEEE floating-point multiplication.
//  Rounding is to nearest even.  Denormals, inf, NaN and -zero are supported.
//
//  Registers used:
//         R0 - X operand and return value
//         R1 - Y operand, clobbered
//         R2, R3 - registers clobbered
//         R7 - register used but preserved
//         A0, A1, ASTAT - clobbered
//
//  A single-precision (32-bit) float has one sign bit,
//  eight bits of exponent, biased by 127 (0-255, not -128..+127),
//  and twenty-three bits of mantissa.
//  31                                    0
//  seee eeee emmm mmmm mmmm mmmm mmmm mmmm
//
//  The refernce below was not used to develop the code, but does describe a
//  similar algorithm.
//
//  Reference  : Computer Architecture a Quantitative Approach 2nd Ed.
//               by John L Hennessy and David Patterson
//
//  Cycle Count for z = ___float32_mul (x, y):
//     41 .. 146 cycles
//            71 cycles for x and y != {Inf, Nan, Denormalized, 0}
//            41 cycles for x = 0 and y != {Inf, Nan, Denormalized, 0}
//            46 cycles for y = 0 and x != {Inf, Nan, Denormalized, 0}
//
//  (Measured for an ADSP-BF532 using version 5.0.0.69 of the ADSP-BF5xx
//   Family Simulator and includes the overheads involved in calling
//   the library procedure as well as the costs associated with argument
//   passing; the cycle counts do not include the costs associated with
//   working around any known silicon anomalies).
//

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12f*"
%notes "Single-precision floating-point multiplication."
/* Changing this register set also affects the #pragma regs_clobbered in */
/* softfloat*/
%regs_clobbered "R0-R3,P0-P2,A0,A1,CCset"
%const
%syscall ___float32_mul(F=FF)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libGroup = floating_point_support;
.file_attr libName = libdsp;
.file_attr prefersMem = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___float32_mul;
.file_attr FuncName = ___float32_mul;
#endif

#include <sys/anomaly_macros_rtl.h>

#if !defined(__ADSPLPBLACKFIN__)
/* Suppress Assembler Warning ea5504 "Memory store instruction use may trigger
** hardware anomaly 05-00-0165". The run-time libraries are not required to
** workaround this anomaly.
*/

.MESSAGE/SUPPRESS 5504;
#endif

#if defined(__ADSPLPBLACKFIN__) && defined(__WORKAROUND_SPECULATIVE_LOADS)
#define __WORKAROUND_BF532_ANOMALY_050000245
#endif

#define MAXBIASEXP 254

.section program;

// C Prototype:
//      float __float32_mul(float X, float Y);
//
// Inputs:
//      R0 = X
//      R1 = Y
// Returns:
//      R0 = X * Y
// Reg usage:
//      R7, R3:0, A1:0, ASTAT
// Clobbers:
//      R3:0, A1:0, ASTAT

.align 2;
.global ___float32_mul;
.type ___float32_mul, STT_FUNC;
___float32_mul:
    [--SP] = R7;

    // Extract exponents of operands
    R3.L = 0x1708;          // Bit field 8 bits long starting at bit 23
    R2 = extract (R0, R3.L) (Z);    // R2 = exponent of X
    R3 = extract (R1, R3.L) (Z);    // R3 = exponent of Y

    // Test for inf/NaN operands
    R7 = MAXBIASEXP+1;
    CC = R2 == R7;          // If X is NaN or inf, handle it specially
    IF CC JUMP .handle_nan_inf_x;
    CC = R3 == R7;          // If Y is NaN or inf, handle it specially
    IF CC JUMP .handle_nan_inf_y;

    // Calc sign of result
    R7 = R0 ^ R1;

    // Test for zero/denormal (exponent will be 0) and extract mantissas
    CC = R2;                // Special handling if X is zero/denorm
    IF !CC JUMP .handle_small_x;
    R0 = R0 << 8;           // R0 = mantissa of X
    BITSET(R0,31);          // Apply hidden bit
.back_from_small_x:

    CC = R3;                // Special handling if Y is zero/denorm
    IF !CC JUMP .handle_small_y;
    R1 = R1 << 8;           // R1 = mantissa of Y
    BITSET(R1,31);          // Apply hidden bit
.back_from_small_y:

    // Calc result exponent
    R2 = R2 + R3;
    R2 += -64;              // Subtract off the exponent bias (127).
    R2 += -63;              // (The most we can add is -64, so add twice.)

    // Multiply unsigned 1.23 by 1.23, to get 2.46 result (48 bits)
    A0 = R0.L * R1.L, A1 = R0.H * R1.L (FU);
    A0 = A0 >> 16;          // Discard lowest 16 bits which are always zero
    A0 += A1;
    A0 += R0.L * R1.H, A1 = R0.H * R1.H (FU);
    R0 = A0.W;              // R0.L = next 16 bits (not saturated) for rounding
    R0 = R0.L (Z);          // Keep just the low 16 bits
    A0 = A0 >> 16;
    A0 += A1;               // (We can't use R1=(A0+=A1) because it saturates)
    R1 = A0.W;              // Remaining 32 bits are the result mantissa (2.30)

    // Set sticky bit
    // We keep the 32 MSB of the result, but for rounding we need to know if
    // any of the 16 LSB were non-zero.  We do this by ORing them all together,
    // into the LSB of the mantissa (in R1).
    CC = R0;                // Test if they are non-zero
    R0 = CC;
    R1 = R1 | R0;           // OR into mantissa

    // Normalize result so there are no leading zeros ('sign bits')
    // If there are already none then the mantissa represents a value between
    // 2.0 and <4.0, so we increment the exponent (which effectively halves
    // the mantissa).  Otherwise there is one leading zero, and we shift up
    // once to remove it.  After this, the mantissa (R1) is 1.31 format.
    // We must not shift down, as that would lose bits needed for rounding.
    R0.L = SIGNBITS A0;     // Get #sign bits in R1, less one (-1..0)
    R0 = R0.L (X);
    R2 = R2 - R0;           // Increase exponent if necessary (+1..0)
    R0 +=  1;               // Calculate actual number of sign bits (0..1)
    R1 = LSHIFT R1 BY R0.L; // Shift mantissa by (0..1)

    // Check for underflow or overflow
    CC = R2 <= 0;           // Underflow if result exponent <= 0
    IF CC JUMP .handle_small_result;
    R0 = MAXBIASEXP;
    CC = R0 < R2;           // Overflow if result exponent > MAXBIASEXP
    IF CC JUMP .return_inf_signed;

    // Remove the hidden bit
    BITCLR (R1, 31);
.back_from_small_result:

    // Combine exponent and sign
    // This moves the exponent into bits 23..30, and the sign into bit 31.
    CC = R7 < 0;            // Get the sign bit from R7
    R2 <<= 24;
    R2 = ROT R2 BY -1;      // Combine it with the exponent

    // Round mantissa to nearest even
    // R1 has the mantissa in 1.31 so we round off the 8 LSB to get 1.23.
    // Because the hidden bit is already removed, the MSB is always zero.
    R3 = 0x80;              // 1/2 LSB of final mantissa
    R0 = R1 + R3;           // Precalc mantissa with biased rounding
                    // If the mantissa overflows, giving 0x80000000, we
                    // must increment the exponent later.
    R7 = R1 << 23;          // Get the 9 LSB...
    R7 >>= 23;
    CC = R7 == R3;          // Test if mantissa is exactly even plus a half
    IF !CC R1 = R0;         // Apply biased rounding if not, do nothing if it is
                    // This is exactly the same as biased rounding except
                    // that ties go to even, which is what we want.

    // Combine mantissa into final result and return
    R1 >>= 8;               // Shift mantissa to bits 22..0
    R0 = R1 + R2;           // Add mantissa
                    // If rounding overflowed, it now increments the exponent.
                    // If the exponent overflows, the value becomes +/-inf.
    R7 = [SP++];
    RTS;


.handle_nan_inf_y:
    // Just swap X and Y then fall through to handle_nan_inf_x.
    R2 = R1;
    R1 = R0;
    R0 = R2;

.handle_nan_inf_x:
    // If Y == 0 or Y == NaN return NaN; if Y < 0 return -X; else return X
    // Input:   R0 = X, R1 = Y, CC = 1

    // Prepare return value in R0 - it is already set to X.
    R2 = R0;                // Speculative compute -X
    BITTGL (R2, 31);
                    // CC is set, so we'll use that rather than clear it.
    R1 = ROT R1 BY 1;       // Rotate 1 into LSB of Y, and sign bit of Y into CC
    IF CC R0 = R2;          // If Y is negative, set return value to -X

    R2 = -1;                // Speculative load NaN
    CC = R1 == 1;           // If R1 is one now, then Y is zero
    IF CC R0 = R2;          // If Y is zero, set return value to NaN

    R3.L = 0x0001;
    R3.H = 0xFF00;
    CC = R3 < R1 (IU);      // If R1 is more than 0xFF000001 now, then Y is NaN
    IF CC R0 = R2;          // If Y is NaN, set return value to NaN

    R7 = [SP++];
    RTS;


.handle_small_x:
    // If mantissa of X is zero, then X is zero so we return zero.
    R0 = R0 << 9;           // Get mantissa of X
    CC = R0;
    IF CC JUMP .handle_denorm_x;    // Handle denormals separately
    R0 = R7 >> 31;          // Return zero with correct sign
    R0 <<= 31;

#if WA_05000428                                   || \
    defined(__WORKAROUND_BF532_ANOMALY_050000245)
    // Speculative read from L2 by Core B may cause a write to L2 to fail.
    // Insert a delay so the stack pop (below) is never speculative.
   NOP;
#endif

    R7 = [SP++];
    RTS;

.handle_denorm_x:
    // Otherwise, X is denorm so we normalize it and resume the multiply.
    // Note that there's no hidden bit.
    R0 >>= 1;               // Temporarily insert a sign bit
    R2.H = 0;
    R2.L = SIGNBITS R0;     // Get #sign bits - 1
    R0 = LSHIFT R0 BY R2.L; // Normalize
    R0 <<= 1;               // Remove the sign bit
    R2 = -R2;               // Get exponent of X
    JUMP .back_from_small_x;


.handle_small_y:
    // If mantissa of Y is zero, then Y is zero so we return zero.
    R1 = R1 << 9;           // Get mantissa of Y
    CC = R1;
    IF CC JUMP .handle_denorm_y;    // Handle denormals separately
    R0 = R7 >> 31;          // Return zero with correct sign
    R0 <<= 31;

#if WA_05000428                                   || \
    defined(__WORKAROUND_BF532_ANOMALY_050000245)
    // Speculative read from L2 by Core B may cause a write to L2 to fail.
    // Insert a delay so the stack pop (below) is never speculative.
   NOP;
#endif

    R7 = [SP++];
    RTS;

.handle_denorm_y:
    // Otherwise, Y is denorm so we normalize it and resume the multiply.
    // Note that there's no hidden bit.
    R1 >>= 1;               // Temporarily insert a sign bit
    R3.H = 0;
    R3.L = SIGNBITS R1;     // Get #sign bits - 1
    R1 = LSHIFT R1 BY R3.L; // Normalize
    R1 <<= 1;               // Remove the sign bit
    R3 = -R3;               // Get exponent of Y
    JUMP .back_from_small_y;


.handle_small_result:
    // Product is too small for a normal, and may be too small to
    // represent at all.
    // Input:   R2 = result exponent which is zero or less.
    //          R1 = normalized result mantissa.
    // Output:  R2 = 0, R1 updated, forming a denormal or zero.

    // Down shift mantissa to one bit lower than denormal position.
    // (We shift it up one bit when we bring in the sticky bit.)
    R0 = 2 (X);
    R0 = R0 - R2;
    R2 = R1;                // We use >>= because we need very large shifts.
    R2 >>= R0;              // LSHIFT and >> are limited to +/-31 bits.

    // Detect loss of precision and set sticky bit.
    // The sticky bit is the OR of all bits that were lost due to shifting.
    R3 = R2;
    R3 <<= R0;              // Shift mantissa back to its original position
    CC = R3 < R1 (IU);      // If it has changed, bits have been lost
    R1 = ROT R2 BY 1;       // Rotate sticky bit into LSB of mantissa
                    // And now the mantissa is in its correct position.

    // Set exponent and resume processing.
    R2 = 0;
    JUMP .back_from_small_result;


.return_inf_signed:
    // Return inf, signed using the MSB of R7.
    CC = R7 < 0;            // Get sign bit
    R0 = 0 (X);             // R0 = (inf << 1), and we're about to shift down
    R0.H = 0xFF00;
    R0 = ROT R0 by -1;      // Apply the sign bit
    R7 = [SP++];
    RTS;


.___float32_mul.end:

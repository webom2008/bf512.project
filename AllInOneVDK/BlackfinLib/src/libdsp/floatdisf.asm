/*
   Copyright (C) 2003-2010 Analog Devices, Inc. All Rights Reserved.

   Convert a signed long long to a float (i.e. I8 to R4)

   It's similar to the algorithm in floatdiuf.asm.

*/

#if defined(__SYSCALL_INFO__)
%const
%regs_clobbered "R0-R3,P1,CCset"
%rtl "1sf"
%notes "Converts a 64-bit integer to a single-precision floating-point number."
%syscall ___longlong64_to_float32(F=lL)
#endif

.file_attr libGroup      = floating_point_support;
.file_attr libGroup      = integer_support;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___longlong64_to_float32;
.file_attr FuncName      = ___longlong64_to_float32;

.section program;

.align 2;
__longlong64_to_float32: /* for backwards compatibility */
___longlong64_to_float32:
   /*
   ** Begin by checking whether this is an I8 or an I4.
   */
   R2 = R0 >>> 31;
   CC = R2 == R1;
   IF CC JUMP .i4tor4;   // top 32-bits all signs, do in 32-bits

   CC = R1 == 0;
   IF CC JUMP .u4tor4;   // top 32-bits zero's, do un unsigned 32-bits

   /*
   ** Get absolute value of input and record if negative.
   */
   CC = R1 < 0;          // Check whether it's negative
   P1 = 157;             // Assume positive value

   // P1 contains a exponent to which will be added the scale of the
   // input - see below under "Calculate the Exponent" for the derivation
   // of '157'

   IF !CC JUMP .no_neg;

   CC = R0;              // CC is zero if LSW is zero
   R2 = CC;              // R2 is zero if LSW is zero

   R0 = -R0;             // Negate LSW

#if defined(__ADSPBLACKFIN__) && !defined(__ADSPLPBLACKFIN__)
   R1 = -R1;
   CC = AN;              // AN is set if the result is -ve (so R1=0x80000000)
   R3 = CC;              // R3 = 1 if result = MSW = 0x80000000 - zero otherwise
   R1 = R1 - R3;         // Force saturation if MSW = 0x80000000
#else
   R1 = -R1 (S);         // (S) causes 0x80000000 to saturate to 0x7FFFFFFF
#endif

   R1 = R1 - R2;
   P1 = 256 | 157;       // It's negative

   // P1 contains a base exponent that will be adjusted by the scale of
   // the input to calculate the required exponent - P1 also contains
   // the negative sign bit. This is an optimization which means that
   // additional code will not be required to explicitly include code
   // to apply the correct sign to the result

   // This instruction also separates the following SIGNBITS instruction
   // from the instruction that calculates its input (Anomaly 05000127
   // and Anomaly 05000209)

.no_neg:
   /*
   ** How many sign bits are in the positive long long input?
   */
   R3.L = SIGNBITS R1;
   R3 = R3.L (X);

   /*
   ** Normalize the long long - this is a 64-bit left shift operation
   ** with R3 containing the number of bits to shift
   */

   R2 = LSHIFT R0 BY R3.L;  // R2 is the LSW of the normalized number
   R1 = LSHIFT R1 BY R3.L;  // R1 is the MSW of the normalized number
                            //    but it is still missing those bits in
                            //    the LSW (R0) that should be in the MSW

   R3 += -32;               // shifts now are complementary right shifts

   R0 = LSHIFT R0 BY R3.L;  // R0 has those bits in the LSW that should be
                            // in the MSW
   R1 = R1 | R0;

   // At this point:
   //
   //    R1 is the MSW of the normalized long long
   //    R2 is the LSW of the normalized long long
   //    R3 is the number of leading sign bit minus 1 minus 32
   //
   // R1 has the following form:
   //
   //    0mmm mmmm mmmm mmmm mmmm mmmm mddd dddd
   //
   //    where mmmmm represents the 24 most-significant bits that, after
   //                  rounding, will be the mantissa
   //
   //          ddddd represents the 'discarded' bits - they are used
   //                to determine whether the mantissa should be rounded
   //
   // Note that the sign bit of R1 will be clear, and the MSB of the
   // mantissa will be set. Also, all the bits in R2 are 'discarded' bits
   // and will be used to determine whether rounding should be applied.
   //
   //
   // The next step is to perform Round-To-Nearest-Even; this operation is
   // described as follows:
   //
   //     We round up or down if there's a clear choice (the remainder is
   //     either less or more than 0.5). If the remainder is exactly 0.5,
   //     then we round to the nearest even value of sum.
   //
   //     The MSB of the remainder is called R - it has the value 0.5 or 0.
   //     The LSB of the mantissa is called G (for Guard bit).
   //     The rest of the remainder is all OR'ed together, and is called S
   //         (for "Sticky").
   //
   //         if (remainder < 0.5 (R==0)) {
   //             don't round
   //         } else if ( remainder > 0.5 (R==1 and S==1)) {
   //             do round
   //         } else if (remainder==0.5 && guard set (R==1 and G==1)) {
   //             do round
   //         } else {
   //             don't round
   //         }
   //
   //    which is: R & (S | G)
   //
   // The following diagram identifies the Guard, Remainder, and Sticky bits
   // in the MSW (R1):
   //
   //    0mmm mmmm mmmm mmmm mmmm mmmm mddd dddd
   //                                  ^^
   //                                  ||
   //                                  GRSS SSSS
   //
   //   (all the bits in the LSW (R2) are also Sticky bits).

   R0 = R1 << 24;        // Shift out the mantissa, giving us GRSS SSSS
   BITCLR (R0,30);       // Clear out R(emainder),  giving us G0SS SSSS
   R0 = R0 | R2;         // OR together _all_ the Sticky bits and G(uard)
   CC = R0;
   R0 = CC;              // S | G

   R2 = R1 << 25;
   R2 = R2 >> 31;        // R
   R2 = R2 & R0;         // R & (S | G)

   // R2 is now the amount by which we'll round - 0 or 1.
   // When we add it, this might be enough to overflow the
   // mantissa past 23 bits. If that happens, we also need to
   // increment the exponent. Therefore, we OR the exponent
   // into the word first - if the mantissa overflows, it'll
   // cause the exponent to be incremented too. We need to
   // remove the hidden bit first, but since we know it's a
   // 1, we know it'd propagate the overflow, if it were
   // present.

   /*
   ** Calculate the exponent
   */
   R0 = P1;              // P1 contains either 157 or (256 ! 157)
   R3 = R0 - R3;

   // The exponent is calculated as follows:
   //
   //     [1] The (bias'ed) exponent of '1' is 127
   //     [2] The smallest value that we are processing here is '1' in
   //         the MSW - therefore the minimum exponent is 127 + 32
   //     [3] We know that the SIGNBITS of '1' in the MSW is 30 and so
   //         the exponent is 127 + 32 + (30 - SIGNBITS)
   //     [4] We also know that R3 = SIGNBITS - 32
   //     [5] So the actual exponent required is:
   //
   //            127 + 32 + (30 - SIGNBITS)
   //         => 127 + 32 + (30 - (R3 + 32))
   //         => 127 + 32 + (30 -  R3 - 32 )
   //         => 127 + 32 + (   -  R3 -  2 )
   //         => 127 + 30       -  R3
   //         => 157            -  R3

   R3 <<= 23;            // Position the exponent
   R1 >>=  7;            // Position the mantissa
   BITCLR (R1,23);       // Remove the hidden bit
   R0   = R1 | R3;       // Combine the exponent and the mantissa
   R0   = R0 + R2;       // Round

   RTS;

.i4tor4:
   // The input value is a 32-bit value, sign-extended to
   // a 64-bit value. So just convert as a normal int-to-float.
   JUMP.X ___int32_to_float32;

.u4tor4:
   // The input value is a 32-bit unsigned value, zero-extended
   // to a 64-bit value. So convert that to a real.
   JUMP.X ___unsigned_int32_to_float32;

.__longlong64_to_float32.end:
.___longlong64_to_float32.end:

.global __longlong64_to_float32;
.global ___longlong64_to_float32;
.type __longlong64_to_float32, STT_FUNC;
.type ___longlong64_to_float32, STT_FUNC;

.extern ___int32_to_float32;
.extern ___unsigned_int32_to_float32;

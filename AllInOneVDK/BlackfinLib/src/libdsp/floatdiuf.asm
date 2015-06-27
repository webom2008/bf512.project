/*
 Copyright (C) 2003-2010 Analog Devices, Inc. All Rights Reserved.

 Convert an unsigned long long to a float (i.e. I8 to R4)

 How it works:
   This function implements ieee 754 'round to nearest' rounding. If
   two numbers are equally near (i.e. the only bit set in the lost bits
   is the most significant one), we choose the even number to return.
*/

#if defined(__SYSCALL_INFO__)
%const
%regs_clobbered "R0-R3,P1,CCset"
%rtl "1uf"
%notes "Converts a 64-bit unsigned integer to a single-precision " \
       "floating-point number."
%syscall ___unsigned_longlong64_to_float32(F=ulL)
#endif

.file_attr libGroup      = floating_point_support;
.file_attr libGroup      = integer_support;
.file_attr libName       = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc       = ___unsigned_longlong64_to_float32;
.file_attr FuncName      = ___unsigned_longlong64_to_float32;

.section program;

.align 2;
__unsigned_longlong64_to_float32: /* for backwards compatibility */
___unsigned_longlong64_to_float32:
   /*
   ** If our long long is really a 32-bit integer, call the support routine
   ** ___unsigned_int32_to_float32.
   */
   CC = R1 == 0;           // is this an I8 or I4?
   IF CC JUMP .u4tor4;

   /*
   ** Find out how many leading sign bits there are (this is SIGNBITS+1
   ** unless Bit 31 of the MSW is set in which case it is 0 (zero))
   */
   R3.L = SIGNBITS R1;
   R3   = R3.L (Z);
   R3  += 1;
   CC   = R1 < 0;
   R2   = 0;
   IF CC R3 = R2;

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
   //    R1 is the MSW of the normalized unsigned long long
   //    R2 is the LSW of the normalized unsigned long long
   //    R3 is the number of leading sign bits minus 32
   //
   // R1 has the following form:
   //
   //    mmmm mmmm mmmm mmmm mmmm mmmm dddd dddd
   //
   //    where mmmmm represents the 24 most-significant bits that, after
   //                  rounding, will be the mantissa
   //
   //          ddddd represents the 'discarded' bits - they are used
   //                to determine whether the mantissa should be rounded
   //
   // Note that the sign bit of R1 will be set and represents the MSB of
   // the mantissa. Also, all the bits in R2 are 'discarded' bits and
   // will be used to determine whether rounding should be applied.
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
   //    mmmm mmmm mmmm mmmm mmmm mmmm dddd dddd
   //                                ^ ^
   //                                | |
   //                                G RSSS SSSS
   //
   //   (all the bits in the LSW (R2) are also Sticky bits).

   R0 = R1 << 23;        // Shift out the mantissa, giving us GRSS SSSS
   BITCLR (R0,30);       // Clear out R(emainder),  giving us G0SS SSSS
   R0 = R0 | R2;         // OR together _all_ the Sticky bits and G(uard)
   CC = R0;
   R0 = CC;              // S | G

   R2 = R1 << 24;
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
   R0 = 158;
   R3 = R0 - R3;

   // The exponent is calculated as follows:
   //
   //     [1] The (bias'ed) exponent of '1' is 127
   //     [2] The smallest value that we are processing here is '1' in
   //         the MSW - therefore the minimum exponent is 127 + 32
   //     [3] When the MSW is 1, we know that the SIGNBITS instruction
   //         will return 30; but the SIGNBITS instruction assumes that
   //         the value is signed and therefore the exponent is
   //         127 + 32 + (31 - SIGNBITS)
   //     [4] We also know that R3 = SIGNBITS - 32
   //     [5] So the actual exponent required is:
   //
   //            127 + 32 + (31 - SIGNBITS)
   //         => 127 + 32 + (31 - (R3 + 32))
   //         => 127 + 32 + (31 -  R3 - 32 )
   //         => 127 + 32 + (   -  R3 -  1 )
   //         => 127 + 31       -  R3
   //         => 158            -  R3

   R3 <<= 23;            // Position the exponent
   R1 >>=  8;            // Position the mantissa
   BITCLR (R1,23);       // Remove the hidden bit
   R0   = R1 | R3;       // Combine the exponent and the mantissa
   R0   = R0 + R2;       // Round

   RTS;

.u4tor4:
   /* The input value is a 32-bit unsigned value, zero-extended
   ** to a 64-bit value. So convert that to a real.
   */
   JUMP.X ___unsigned_int32_to_float32;

.__unsigned_longlong64_to_float32.end:
.___unsigned_longlong64_to_float32.end:

.global __unsigned_longlong64_to_float32;
.global ___unsigned_longlong64_to_float32;
.type __unsigned_longlong64_to_float32, STT_FUNC;
.type ___unsigned_longlong64_to_float32, STT_FUNC;

.extern ___unsigned_int32_to_float32;


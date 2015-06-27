/*
** Copyright (C) 2003-2010 Analog Devices, Inc. All Rights Reserved.
**
** Convert an unsigned integer 32-bit value into an IEEE
** double-precision floating point value.
** long double __unsigned_int32_to_float64(unsigned int)
*/

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "1uf"
%notes "Converts a 32-bit unsigned integer to a double-precision " \
       "floating-point number."
/* Note you'll need to change SOFTFLOAT source if you change this regset */
/* and also i64tof64.asm and u64tof64.asm */
%regs_clobbered "R0-R3,P1,CCset"
%const
%syscall ___unsigned_int32_to_float64(lD=U)
#endif

.file_attr libGroup      = integer_support;
.file_attr libGroup      = floating_point_support;
.file_attr libName = libf64ieee;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___unsigned_int32_to_float64;
.file_attr FuncName      = ___unsigned_int32_to_float64;

.section program;
.align 2;

___unsigned_int32_to_float64:
        // Check for zero first.

        CC = R0 == 0;
        IF CC R1 = R0;
        IF CC JUMP .ret_zero;

        CC = R0 < 0;            // I.e. the MSB (not a sign bit) is set
        IF CC JUMP .big_val;

        // Determine the exponent.

        R2.L = SIGNBITS R0;
        R2 = R2.L (X);

        R0 <<= R2;              // Adjust for normalisation
        R1 = R0;                // Put adjusted bits in high half.
        R0 <<= 22;              // Adjust for the bits that are
                                // in high half (including hidden)
        R1 >>= 10;              // and then back for exponent space

        R3 = (1023+29);         // Bias the exponent (+29 cos we add
                                // in the hidden bit that makes it +30)
        R2 = R3 - R2;

        R2 <<= 20;              // Position at MSB, leaving space for sign.
        R1 = R1 + R2;           // Combine with mantissa.
.ret_zero:
        RTS;

.big_val:
        // The MSB is set, where we'd expect to find the sign bit,
        // so we know what our exponent will be. Set the values
        // directly.

        R1 = R0 >> 11;          // Align MSB with hidden bit
        R0 <<= 21;              // Adjust remaining bits in low half.
        R2 = (1023+30);         // Bias exponent (+30 cos we dont zero the
                                // hidden bit - instead we add it in)
        R2 <<= 20;              // and position for exponent; sign is positive.
        R1 = R1 + R2;           // and include exponent.
        RTS;
.___unsigned_int32_to_float64.end:

.global ___unsigned_int32_to_float64;
.type ___unsigned_int32_to_float64,STT_FUNC;

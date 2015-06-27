/*
** Copyright (C) 2004 Analog Devices, Inc. All Rights Reserved.
**
** Convert non-IEEE 32-bit floating point to 64-bit integer.
*/

.file_attr libGroup      = floating_point_support;
.file_attr libGroup      = integer_support;
.file_attr libName = libf64fast;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";     
.file_attr FuncName      = __float32_to_int64;
.file_attr FuncName      = __float32_to_unsigned_int64;
.file_attr FuncName      = __float32_to_int64_round_to_zero;
.file_attr FuncName      = __float32_to_unsigned_int64_round_to_zero;
.file_attr libFunc = __float32_to_int64;
.file_attr libFunc = __float32_to_unsigned_int64;
.file_attr libFunc = __float32_to_int64_round_to_zero;
.file_attr libFunc = __float32_to_unsigned_int64_round_to_zero;

.section program;
.align 2;

__float32_to_int64:
__float32_to_unsigned_int64:
__float32_to_int64_round_to_zero:
__float32_to_unsigned_int64_round_to_zero:

	// Quick check for zero

	CC = R0 == 0;
	IF CC JUMP .ret_zero;

	// Unpack into separate mantissa and
	// exponent.

	R1 = R0.L (X);
	R0.L = 0;

	// Check whether the exponent is outside the
	// range of the 64-bit value. If the exponent's
	// negative, then we've just got zero as a
	// result, because we lose all the bits.

	R3 = 64 (Z);
	R2 = 0;
	CC = R1 < 0;
	IF CC JUMP .ret_zero_cpy;

	CC = R3 < R1;		// Too big?
	IF CC JUMP .ret_zero_cpy;

	// Our exponent is now known to be in the
	// range 0..64. Will the bits all be in the
	// high-half (49-64), the low-half (0..32)
	// or split across (33-48)?

	R2 = R1;
	R3 += -16;	// Now 48
	CC = R3 <= R2;
	IF CC JUMP .high_half;
	R3 += -16;	// Now 32
	CC = R3 <= R2;
	IF !CC JUMP .both_halves;

	// In the range 0..32, so all bits will be in
	// the low half. Determine how many bits make it
	// into the low half, and shift.

	R2 = R3 - R2;
#ifdef __WORKAROUND_SHIFT
	R0 = ASHIFT R0 BY R2.L;
#else
	R0 >>>= R2;
#endif
	R1 = R0 >>> 31;		// sign-extend
	RTS;

.high_half:
	R1 = R0;  
	R3 += 16;		// Now 64 again
	R2 = R3 - R2;		// will be 0..15
	R1 >>>= R2;
	R0 = 0;
	RTS;

.both_halves:
	R1 = R0;
	R3 += 16;		// Now 64 again
	R3 = R3 - R2;
	R1 >>>= R3;
	R2 += -32;
	R0 <<= R2;
	RTS;

.ret_zero_cpy:
	R0 = R2;
.ret_zero:
	R1 = R0;
	RTS;
.__float32_to_int64.end:
.__float32_to_unsigned_int64.end:
.__float32_to_int64_round_to_zero.end:
.__float32_to_unsigned_int64_round_to_zero.end:
.global __float32_to_int64;
.global __float32_to_unsigned_int64;
.global __float32_to_int64_round_to_zero;
.global __float32_to_unsigned_int64_round_to_zero;
.type __float32_to_int64, STT_FUNC;
.type __float32_to_unsigned_int64, STT_FUNC;
.type __float32_to_int64_round_to_zero, STT_FUNC;
.type __float32_to_unsigned_int64_round_to_zero, STT_FUNC;

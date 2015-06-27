/*
** Copyright (C) 2003-2008 Analog Devices, Inc. All Rights Reserved.
**
** long double floating-point comparison.
**
** This is the internal function implementing IEEE double-precision
** floating-point comparison. It compares two numbers X and Y.
** If X < Y, it returns <0.
** If X > Y, it returns >0.
** If X == Y, it returns 0.
** NaNs always compare as unequal, and zeroes always compare as equal
** regardless of sign.
**
** X is received in R1:0. Y is received in R2 and on the stack.
** The result is in R0.
** int __float64_cmp(long double, long double)
*/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.file_attr libGroup      = floating_point_support;
.file_attr libName = libf64ieee;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___float64_cmp;
.file_attr FuncName      = ___float64_cmp;

#endif

#include <sys/anomaly_macros_rtl.h>

.section program;
.align 2;

___float64_cmp:
	P0 = R4;		// Grab some workspace.

	// NaNs are represented as all-ones exponent, and
	// a non-zero mantissa. If sign is ignored, then
	// an operand is a NaN if its high half is larger
	// than 0x7FF0, or if its high half is equal to
	// 0x7FF0, and its low half is non-zero.

	// Check X first.

	R4 = R1;		// clear the sign
	BITCLR(R4, 31);
	R3 = 0x7FF;
	R3 <<= 20;		// Set up NaN limit.
	CC = R3 < R4;		// If it's a NaN, it'll do
	IF CC JUMP .ret_y;	// as the return value
	CC = R4 < R3;		// If high-half less than the NaN
	R0 += 0;		// limit, or low-half is zero, it's
	CC |= AZ;		// not a NaN.
	IF !CC JUMP .ret_x;

	// Check Y next

	P1 = R5;		// Need more space
	R5 = R3;		// NaN limit

#if WA_05000428
	// Speculative read from L2 by Core B may cause a write to L2 to fail

	NOP;
#endif
	R3 = [SP+12];		// Get other half of Y
	R4 = R3;
	BITCLR(R4,31);		// and clear its sign
	CC = R5 < R4;		// If it's over the limit
	IF CC JUMP .ret_y;	// it'll do as the return limit
	CC = R4 < R5;		// If its high-half is less than limit
	R2 += 0;		// or low-half is zero, it's not a
	CC |= AZ;		// a NaN.
	IF !CC JUMP .ret_y;

	// Neither is a Nan. Make any zero operands positive.
	R4 = R4 | R2;		// Check Y == 0
	CC = R4 == 0;
	IF CC R3 = R4;		// and clear sign if so.
	R5 = R1 << 1;
	R5 = R5 | R0;		// Check X == 0;
	CC = R5 == 0;
	IF CC R1 = R5;		// and clear sign if so.

	// Are both zero?
	R4 = R4 | R5;		// both operands without signs
	CC = R4 == 0;		// and if everything is zero, then
	IF CC R3 = R4;		// return zero.
	IF CC JUMP .ret_y;

	// Nope. Are they both the same?
	CC = R0 == R2;
	R4 = R1 - R3;
	CC &= AZ;
	IF CC R3 = R4;		// if so, then return zero.
	IF CC JUMP .ret_y;

	// They're both valid numbers, and X != Y, so do
	// a comparison. And we do a signed comparison,
	// because the remaining floating point values
	// are (almost) ordered like a signed integer range.

	CC = R1 < R3;
	R4 = CC;
	CC = R1 == R3;
	R4 = ROT R4 BY 1;
	CC = R0 < R2 (IU);
	R4 = ROT R4 BY 1;
	R5 = R1 & R3;
	R1 = 1;
	R0 = -R1;
	CC = R4 < 3;
	IF CC R0 = R1;
	R1 = -R0;
	CC = R5 < 0;		// if both are negative
	IF CC R0 = R1;		// negate the result again.
	R5 = P1;
	R4 = P0;
	RTS;

.ret_y:
	R5 = P1;
	R1 = R3;
.ret_x:
	R4 = P0;
	R0 = R1;
	RTS;
.___float64_cmp.end:

.global ___float64_cmp;
.type ___float64_cmp, STT_FUNC;

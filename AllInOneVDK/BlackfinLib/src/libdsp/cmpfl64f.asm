/*
** Copyright (C) 2003-2005 Analog Devices, Inc. All Rights Reserved.
**
** 64-bit comparison of floating point numbers in non-IEEE format.
**
** This function implements comparison of two floating point 64-bit
** numbers, in a non-IEEE format. X is received in R1:0. Y is
** received in R2 and on the stack. The result is in R0, and is
**	X <  Y		<0
**	X == Y		 0
**	X >  Y		>0
** Nans compare as unequal.
**
** int __float64_cmp(long double, long double)
*/

.file_attr libGroup      = floating_point_support;
.file_attr libName = libf64fast;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___float64_cmp;
.file_attr FuncName      = ___float64_cmp;

.section program;
.align 2;

___float64_cmp:
	P0 = R7;		// Workspace
	R3 = [SP+12];		// Get high half of Y

	CC = R0 == R2;
	R7 = R1 - R3;
	CC &= AZ;
	IF CC JUMP .same;

	// They're different. Is either a zero or an inf?

	// First, do either have zero mantissas?
	CC = R1 == 0;
	R3 += 0;
	CC |= AZ;
	IF CC JUMP .possible_infs;

.normal:
	// Compare by doing R=X-Y,
	// but must align decimal points first. Make sure that
	// Y is the least significant value.

	CC = R0 < R2;		// Swap mantissas of X and Y
	IF CC R7 = R3;
	IF CC R3 = R1;
	IF CC R1 = R7;

	// Determine the difference between the exponents,
	// which is the amount to shift Y. If we swapped
	// mantissas, negate the difference.

	R2 = R0 - R2 (S);
	R0 = -R2;
	IF CC R2 = R0;

	// Shift Y to align with X, and subtract. This will give
	// us a positive or negative value, depending on the
	// relative values of X and Y.

#ifdef __WORKAROUND_SHIFT
	// R2 could be over the WHOLE range of R2 (i.e. 0 to 0xffffffff)
	// This causes some difficulties.
	R0 = 32;
	CC = R0 < R2 (IU);
	R7 = R2;
	IF CC R7 = R0;
	R7=-R7;
	R3 = ASHIFT R3 by R7.L;
#else
	R3 >>>= R2;
#endif
	R0 = R1 - R3 (S);
	R2 = R1 & R3;

	// If we swapped X and Y, we want to negate the
	// result. But if both X and Y are negative, we
	// want to negate *that* result. So if we swapped
	// or if both are negative, we negate, but not if
	// both or neither conditions are true:

	CC ^= AN;		// AN set by ANDing X and Y

	R1 = -R0;
	IF CC R0 = R1;

	R7 = P0;
	RTS;

.same:
	// X and Y match. But are they NaNs?

	R0 = 0;			// Assume a match (NaNs are unlikely)
	CC = R3 == R2;		// NaNs have mantissa==exponent
	IF CC JUMP .nannish;
	R7 = P0;
	RTS;
.nannish:
	R7 = 0;
	BITSET(R7,31);		// -Nan (0x80000000)
	CC = R2 == R7;
	R1 = R3 - R7;
	CC &= AZ;
	IF CC R0 = R7;
	R7 += -1;		// +Nan
	CC = R2 == R7;
	R1 = R3 - R7;
	CC &= AZ;
	IF CC R0 = R7;
	R7 = P0;		// (restore)
	RTS;			// Else return zero.

.possible_infs:
	// X or Y has a zero mantissa. Could be zero or Inf.

	// Most likely got an infinity.
	// X==+Inf || Y==-Inf => >0
	// X==-Inf || Y==+Inf => <0

	R7.H = 0x7FFF;
	R7.L = 0xFFFF;
	CC = R2 == R7;
	IF CC JUMP .negate;

	CC = R0 == R7;
	IF CC JUMP .ret_r7;

	R7 += 1;
	CC = R2 == R7;
	IF CC JUMP .negate;

	CC = R0 == R7;
	IF CC JUMP .ret_r7;

	// X==0 => Return -Y
	// Y==0 => Return X

	R7 = R0 | R1;
	CC = R7 == 0;
	R7 = -R3;
	IF CC JUMP .ret_r7;

	R7 = R2 | R3;
	CC = R7 == 0;
	IF CC R0 = R1;
	IF CC JUMP .ret;
	// bizarrely large exponents, but not infinity
	JUMP .normal;

.negate:
#if defined(__ADSPBLACKFIN__) && !defined(__ADSPLPBLACKFIN__)
/* __ADSPBF535__ core only */
	R0 = 0;
	R7 = R0 - R7 (S);
#else
	R7 = -R7 (S);
#endif
.ret_r7:
	R0 = R7;
.ret:
	R7 = P0;
	RTS;

.___float64_cmp.end:

.global ___float64_cmp;
.type ___float64_cmp, STT_FUNC;

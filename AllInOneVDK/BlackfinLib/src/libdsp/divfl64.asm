/*
** Copyright (C) 2003-2011 Analog Devices, Inc. All Rights Reserved.
**
** 64-bit floating-point division.
**
** This is the internal function implementing emulation of IEEE
** double-precision floating point division (X/Y). X is received
** in R1:0, Y is received in R2 and on the stack. The result is
** returned in R1:0.
**
** Denormal inputs are treated as 0.0 and results are returned as 
** signed 0.0.
**
** long double __float64_div(long double, long double);
*/

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12f/"
%notes "Double-precision floating-point division."
%regs_clobbered "R0-R3,P0-P2,A0,A1,I0-I3,M0-M2,LC0,LT0,LB0,CCset"
%const
%syscall ___float64_div(lD=lDlD)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.file_attr libGroup      = floating_point_support;
.file_attr libName = libf64ieee;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___float64_div;
.file_attr FuncName      = ___float64_div;

#endif

#define  MAXBIASEXP 0x7FE 

#if defined(__ADSPBLACKFIN__) && !defined(__ADSPLPBLACKFIN__)
/* __ADSPBF535__ core only */
#define CARRY AC
#else
#define CARRY AC0
#endif

.section program;
.align 2;

___float64_div:
	R3 = [SP+12];			// Recover high half of Y.
	[--SP] = (R7:4);		// Claim some workspace.

	// Note the sign sR of the result, which is the exclusive-OR
	// of the signs of X and Y.

	R4 = R1 ^ R3;
	P0 = R4;

	// Check for common mathematical identities. Get rid of
	// the sign bits on the operands, so that it's easier to
	// compare with zero (without worrying about +0 == -0).

	BITCLR(R1, 31);
	BITCLR(R3, 31);

	// First check if either operand is a NaN or inf, and handle certain
	// identities with these values
	R4 = MAXBIASEXP+1;

	// Extract exponents
	R6 = R1 >> 20; 
	R7 = R3 >> 20;

	// Handle cases where at least one operand is a NaN or inf
	CC = R4 == R6;
	IF CC JUMP .HANDLE_NAN_INF_X;

	CC = R4 == R7;
	IF CC JUMP .HANDLE_NAN_INF_Y;

	// Handle identities where neither operand is a NaN or inf
	CC = R7 == 0;
	IF CC JUMP .DIV_BY_ZERO;

	CC = R6 == 0;        // if X is 0.0 or denormal, return 0.0
	IF CC JUMP .RET_ZERO;

	R5 = 0x3FF (Z);			// If X/1.0, return X
	R5 <<= 20;
	CC = R3 == R5;
	R2 += 0;
	CC &= AZ;
	IF CC JUMP .SIGN_AND_RETURN;

	CC = R0 == R2;			// If X/X, return +/- 1
	R5 = R1 - R3;
	CC &= AZ;
	IF CC JUMP .RET_ONE;

	// Compute result exponent eR = eX - eY + 1023 (both eX and eY
	// have the bias of 1023 added in; removing one bias is
	// the same as removing a bias from each of eX and eY,
	// subtracting, and adding a bias back to eR).
	R6 = R6 - R7;
	R7 = 1023 (Z);
	R6 = R6 + R7;

	// We may have overflowed, producing a larger exponent
	// than can be expressed. If so, return the appropriate
	// Infinity.
	CC = R4 < R6;
	IF CC JUMP .RET_INF;

	// Clear the exponents out of X and Y, leaving just
	// mX and mY.

	R1 <<= 12;
	R1 >>= 12;
	R3 <<= 12;
	R3 >>= 12;

	// Make the IEEE hidden bits explicit, at posn 52
	// (posn 20, in the high halves).

	BITSET(R1, 20);
	BITSET(R3, 20);

	// Prepare for the division sequence. The algorithm is:
	// P = X
	// R = 0
	// if (Y < P) then
	//	P = P - Y
	//	n = 63
	// else
	//	n = 64
	// for i = 1 to n
	//	if (Y < P) then
	//		P = P - Y
	//		left-shift 1 into LSB of R
	//	else
	//		left-shift 0 into LSB of R
	//	left-shift 0 into LSB of P
	//
	// So we have partial figure P, result R,
	// we're comparing against Y (+Y) and we're
	// subtracting Y (adding -Y), so that gives us
	// four 64-bit values to maintain in the loop:
	// P (starts out as mX) in R1:0.
	// R (starts out as 0) in I1:0
	// Y (is mY) in R3:2
	// -Y (is -mY) in I3:2

	P1 = R6;	// save eR

   R6 = 0; 
   [ --SP ] = R6; I0 = SP;   // R starts at 0
   [ --SP ] = R6; I1 = SP;

	R6 = -R2;	// negate Y
	CC = CARRY;
	CC = !CC;
	R5 = CC;
	R7 = -R3;
	R7 = R7 - R5;
   [ --SP ] = R7; I3 = SP;   // save -Y
   [ --SP ] = R6; I2 = SP; 

	P2 = 64;	// Assume max iterations

	// Compare Y<P, i.e. R3:2 < R1:0

	CC = R3 < R1 (IU);
	R4 = CC;
	CC = R3 == R1;
	R4 = ROT R4 BY 1;
	CC = R2 < R0 (IU);
	R4 = ROT R4 BY 1;
	CC = R4 < 3;
	IF CC JUMP .max_iterations;

	// Do P = P - Y (P = P + -Y), and knock off an iteration.
	P2 += -1;	// one less iteration
	R0 = R0 + R6;	// Add -Y to P
	CC = CARRY;
	R6 = CC;
	R1 = R1 + R7;
	R1 = R1 + R6;

.max_iterations:
	LSETUP(.lstart, .lend) LC0 = P2;
.lstart:	
	CC = R3 < R1 (IU);
	R4 = CC;
	CC = R3 == R1;
                     // Recover -Y in R6:7
	R4 = ROT R4 BY 1 || R6 = [I2];
	CC = R2 < R0 (IU);
	R4 = ROT R4 BY 1 || R7 = [I3];
	CC = R4 < 3;
	CC = !CC;
	R5 = CC;
	IF !CC R6 = R5;	// but if comparison failed, then just
	IF !CC R7 = R5;	// add zeroes instead
	R0 = R0 + R6;	// Add -Y to P
	CC = CARRY;
	R6 = CC;
	R1 = R1 + R7;
	R1 = R1 + R6;
                                     // Recover R
	R5 = ROT R5 BY -1 || R6 = [I0];  // Move result cmp into CC
	R6 = ROT R6 BY 1  || R7 = [I1];  // Rotate in result of comparison
                                     // and store R for next iteration
	R7 = ROT R7 BY 1  || [I0] = R6;
	R5 = ROT R5 BY -1 || [I1] = R7;  // Set up a 0 for shifting into P
	R0 = ROT R0 BY 1; // and move into P.
.lend:	R1 = ROT R1 BY 1;

   SP += 16;
	// At this point, our result is in I1:0 (and R7:6).
	// The inputs were in 12.52 format, and the output is
	// in 2.62 format. We need to normalise it, so that
	// the decimal point appears between bit 51 and 52.
	// Since it's currently between bits 61 and 62, that's
	// a right-shift of ten places.

	R2 = R6 << 22;		// Keep what we lose off the bottom
	R5 = R7 << 22;
	R0 = R6 >> 10;
	R0 = R5 | R0;
	R1 = R7 >> 10;

	// Our result is now in R1:0. Restore the exponent.

	R4 = P1;

	// If the IEEE hidden bit is set, then there's overflow
	// into the exponent space, so right-shift and increment
	// exponent.

	CC = BITTST(R1, 20);
	IF CC JUMP .clear_hidden;
.hidden_cleared:
	CC = R4 < 0;
	IF CC JUMP .RET_ZERO;

.rounding:
	// if we've cranked the exponent up past the limit, return infinity.
	R7 = 0x7ff (Z);
	CC = R7 <= R4;
	IF CC JUMP .RET_INF;

   // Determine whether we need to round. Normally based on:
   // - the Guard bit G (LSB of mR, i.e. R0.0)
   // - the round bit R (MSB of lost bits, i.e. R2.31)
   // - the stick bit S (rest of lost bits, ORed together)
   // - Round if (R & (G|S)).
   // But not for division. Here, we round based on R only.
   // That way, if the two numbers are similar, but slightly different,
   // we get a result slightly different from 1.0. We only get 1.0 if
   // X==Y.

   R3 = R2 >> 31;			// Just R bit

   // R1:0 constains the mantissa. R3 is the rounding bit.
   // R4 is the exponent. If we rounded, would we have a
   // zero mantissa?
   // Note the exponent can be zero here, which should only produce a non-zero
   // result if we round up.

   R0 = R0 + R3;        // add in rounding bit
   CC = CARRY;          // check that we don't have all lower bits + rounding 
   R3 = CC;
   CC = !CC;  
   R2 = R0 | R1;
   CC &= AZ; 
   IF CC JUMP .RET_ZERO;

	// Combine sign, exponent and mantissa, add in rounding and return

   R4 <<= 20;           // Position exponent and combine
   R1 = R4 | R1;

   R1 = R1 + R3;        // and nudge exponent if rounding cause overflow 
   R4 = R1 >> 20;       // re-extract the exponent
   CC = R4 == 0;        // check it's not zero - which would mean we didn't
                        // round up enough.
   IF CC JUMP .RET_ZERO;
   R1 <<= 1;            // Position ready to grab sign.
   CC = P0 < 0;
   R1 = ROT R1 BY -1;   // and pull sign back in.
   (R7:4) = [SP++];
   RTS;

	// Exit points for special cases, because we're returning known values.
	// The value should be signed appropriately unless it is a NaN.

.HANDLE_NAN_INF_X:
	// Return a NaN for all cases except where X=inf and Y is a valid number
	// including 0, in which case return inf (signed appropriately)
	CC = R0 == 0;		// If X significand is zero then X is inf
	R5 = R1 << 12;
	CC &= AZ;
	R5 = R7 - R4;		// If Y exp - MAXBIASEXP is -ve then Y is a valid number
	CC &= AN;
	IF CC JUMP .RET_INF (BP);
						// (predict branch to avoid 05-00-0428)
.RET_NAN:
	R0 = -1;            // Otherwise return a NaN
	R1 = R0;
	(R7:4)=[SP++];      // Pop R7-R4 from stack
	RTS;

.DIV_BY_ZERO:
	// Return inf unless 0/0 in which case we return NaN;
	CC = R6 == 0;
	IF CC JUMP .RET_NAN;

.RET_INF:
	R1 = 0x7FF (Z);
	R1 <<= 20;
	R0 = 0;
	JUMP .SIGN_AND_RETURN;

.RET_ONE:
	R1 = 0x3FF (Z);
	R1 <<= 20;
	R0 = 0;
	JUMP .SIGN_AND_RETURN;

.HANDLE_NAN_INF_Y:
	// X is known to be a valid (non-NaN, non-Inf, possibly zero) and Y is known
	// to have all exponents bits set, so it's a NaN or +-Infinity.
	// If Y is a NaN return a NaN, otherwise return 0
	CC = R2 == 0;	// if mantissa Y is non-zero then Y is a NaN
	R5 = R3 << 12;
	CC &= AZ;
	IF !CC JUMP .RET_NAN;
.RET_ZERO:
	R0 = 0;
	R1 = R0;
.SIGN_AND_RETURN:
	CC = P0 < 0;
	R2 = R1;
	BITSET(R2,31);
	IF CC R1 = R2;
	(R7:4)=[SP++];       // Pop R7-R4 from stack
	RTS;

.clear_hidden:
	R4 += -1;
	BITCLR(R1, 20);
	JUMP .hidden_cleared;

.___float64_div.end:

.global ___float64_div;
.type ___float64_div, STT_FUNC;

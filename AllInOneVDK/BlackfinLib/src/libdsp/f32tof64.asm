/*
** Copyright (C) 2003-2010 Analog Devices, Inc. All Rights Reserved.
**
** Convert an IEEE single-precision floating point number
** to an IEEE double-precision floating point number.
**
** long double __float32_to_float64(float)
*/

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "1ff"
%notes "Converts a single-precision floating-point number to double-precision."
/* If you change this clobber set rembember to change SOFTFLOAT */
%regs_clobbered "R0-R3,P1,CCset"
%const
%syscall ___float32_to_float64(lD=F)
#endif

.file_attr libGroup      = floating_point_support;
.file_attr libName = libf64ieee;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___float32_to_float64;
.file_attr FuncName      = ___float32_to_float64;

.section program;
.align 2;

___float32_to_float64:

	// Check for some unusual numbers first.

	// If all zeroes, apart from sign, then it's a zero value.
    R1 = R0 << 1;
	CC = R1 == 0;
	IF CC JUMP .ret_zero;

	// If exponent is all ones, then either a NaN or +/- Inf.

	R2 =  R1 >> 24;                     // extract the exponent
	R1 = 255 (Z);
	CC = R1 <= R2 (IU);
	IF CC JUMP .nan_or_inf;

	// Remaining numbers are "ordinary" (although not necessarily
	// "normal")

	// Extract the sign

	R3 = R0 >> 31;
	R3 <<= 31;

	// The sp exponent is biased, with 127.
	// The dp exponent needs to be biased with 1023,
	// so add another 896.

	R1 = 896 (Z);
	R2 = R2 + R1;
    CC = R1 == R2;
    R1 = R0 << 9;                 // chop off sign and exp
    IF CC JUMP .normalise ; // if exponent wasn't 0, jump to finish up
                                  // otherwise we'll need to normalise.
.finish_up:
    // we now have a valid mantissa and exponent in R1 and R2 respectively.

	R2 <<= 20;          // Move into position in high half
	R3 = R3 | R2;       // and combine with sign bit.

	// Of the 23 bits of mantissa, most will be in the high half.
	// The least significant 3 bits will be in the low half.

	R0 = R1 << 20;      // place the least significant 3 bits into the low half.
	R1 >>= 12;          // place the rest into the high half.
	R1 = R1 | R3;       // Combine with exponent and sign.
	RTS;

.normalise:
    R2 += 1;                      // correct denormal exponent.
    CC = !CC;                     // if we're here, CC=1.  make it 0.
.normalise_loop:
      // we can assume there's at least 1 bit in the mantissa as we aren't
      // Inf or Zero.  Therefore we need to slide the mantissa along, altering
      // the exponent as we go until the most significant bit falls off the end.
      R1 = ROT R1 BY 1;
      R2 += -1;
      IF !CC JUMP .normalise_loop (bp) ;
 
    JUMP .finish_up ;
 

.ret_zero:
	R1 = R0;            // Put the sign bit at MSB of top half
	R0 <<= 1;           // and make bottom half zero.
	RTS;

.nan_or_inf:
	// Inf is exponent all ones, mantissa all zeros.
	// NaN is exponent all ones, mantissa non-zero.
	// So for both cases, we need to extend the exponent from being
	// 8 1-bits to being 11 1-bits, and we're extending towards the
	// LSB.
	// At this point, R1 contains R0, left-shifted one space to
	// remove the sign.

	R1 <<= 20;	// shift that FF back 1, and then down 3 more
	R1 = R0 | R1;	// which will give us total of 11 exponent bits.

	// We now shift all of the sign and exponent bits out of R0,
	// leaving it with just the mantissa bits. These will be zero
	// for +/- Inf, and non-zero for NaN. We can't rely on R1's
	// part of the mantissa having any non-zero bits, because they
	// might have been overwritten when we extended the exponent.
	// This means we're still returning NaN, but it'll be a different
	// NaN, and not just zero-filled into the lower-precision bits.

	R0 <<= 9;
	RTS;

.___float32_to_float64.end:

.global ___float32_to_float64;
.type ___float32_to_float64, STT_FUNC;

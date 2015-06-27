/*
** Copyright (C) 2003-2010 Analog Devices, Inc. All Rights Reserved.
**
** Convert a signed integer 32-bit value into an IEEE
** double-precision floating point value.
** long double __int32_to_float64(int)
*/

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "1sf"
%notes "Converts a 32-bit integer to a double-precision floating-point number."
/* Note you'll need to change SOFTFLOAT source if you change this regset */
%regs_clobbered "R0-R3,P1,CCset"
%const
%syscall ___int32_to_float64(lD=I)
#endif

.file_attr libGroup      = floating_point_support;
.file_attr libGroup      = integer_support;
.file_attr libName = libf64ieee;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___int32_to_float64;
.file_attr FuncName      = ___int32_to_float64;

.section program;
.align 2;

___int32_to_float64:

	// Check for inputs of 0x0 or 0x80000000 
	R1 = R0 << 1;
	CC = AZ;
	IF CC JUMP .ret_min_or_zero;

	// The mantissa will be unsigned, so if the input
	// value is negative, negate it, and remember this
	// fact.

	CC = R0 < 0;
	R1 = -R0;
	IF CC R0 = R1;

	// Determine the exponent.

	R3 = 1053;			// Load exponent bias in advance here to prevent
							// signbits operand having been loaded in previous
							// instruction (anomaly 05-00-0209 and 05-00-0127)
	R2.L = SIGNBITS R0;
	R2 = R2.L (X);
	R0 <<= R2;			// Adjust for normalisation
	R1 = R0;				// Put normalized bits into high half.
	R0 <<= 22;			// Adjust for the bits that are
							// in high half (including hidden)
	R1 >>= 10;			// and then back for exponent space
	BITCLR(R1,20);		// Remove the hidden bit.
							// Exponent is biased by 1023, and it also
	R2 = R3 - R2;		// includes the number of bits we'd shift
							// *right*, to make the most significant bit
							// into 1 (because we're 1.x raised to a
							// power). So that's 1023+(30-signbits).

	R2 <<= 21;			// Position at MSB
	R2 = ROT R2 BY -1;// and insert sign bit (in CC), realigning exponent
	R1 = R1 | R2;		// Combine with mantissa.
	RTS;

.ret_min_or_zero:
	CC = BITTST(R0,31);
	R0 = 0;
	R1.H = 0xC1E0;		// Exponent and sign for int min
	IF !CC R1 = R0;	// Return representation of int min
	RTS;	

.___int32_to_float64.end:

.global ___int32_to_float64;
.type ___int32_to_float64,STT_FUNC;

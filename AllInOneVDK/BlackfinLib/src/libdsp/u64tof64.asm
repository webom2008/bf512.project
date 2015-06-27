/*
   Copyright (C) 2003-2010 Analog Devices, Inc. All Rights Reserved.

   Convert unsigned long long to IEEE double-precision 64-bit
   floating point.
 
   long double __unsigned_int64_to_float64(unsigned long long);

   How It Works:
   
   1) Firstly we see if we're really converting a 32-bit value and if so call
      the appropriate 32-bit conversion routine.

   2) We then grab some workspace (R7) and store into R7 the amount we'll have
      to left shift our long long to get a '1' in the most significant bit of
      R1.

   3) We then do the shift.

   4) We then do some of the rounding calculation.  IEEE 754 nearest-even
      rounding is used: 
        round to nearest & if both numbers are as near, round to even.

      So we add 1 to the most significant lost bit (which will ripple in lost
      bits if the most significant lost bit is set)

      We take into account the fact that R1 may overflow in this calculation
      in the next stage.
   
   5) We right shift our number by 11 places, rotating in the carry in case
      of overflow.

   6) If none of the lost bits are set (apart from the most significant one),
      we zero the least significant bit of the result (as we have to round 
      to even).

   7) We calculate the exponent and *add it in* to the mantissa.  We've not
      cleared the hidden bit so the mantissa will either be 1, or, in the
      case that the rounding caused an overflow of R1, 2.
*/

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "1uf"
%notes "Converts a 64-bit unsigned integer to a double-precision " \
       "floating-point number."
/* Note you'll need to change SOFTFLOAT source if you change this regset */
%regs_clobbered "R0-R3,P1,CCset"
%const
%syscall ___unsigned_int64_to_float64(lD=ulL)
#endif

.file_attr libGroup      = integer_support;
.file_attr libGroup      = floating_point_support;
.file_attr libName       = libf64ieee;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc       = ___unsigned_int64_to_float64;
.file_attr FuncName      = ___unsigned_int64_to_float64;

#if defined(__ADSPBLACKFIN__) && !defined(__ADSPLPBLACKFIN__)
/* __ADSPBF535__ core only */
#define CARRY AC
#else
#define CARRY AC0
#endif

.section program;
.align 2;

___unsigned_int64_to_float64:
        // 1)
        CC = R1 == 0;
        IF CC JUMP .is_32bit;

        // 2)
        P1 = R7;                // Workspace

        R7.L = SIGNBITS R1;
        R7 = R7.L;
        R7 += 1;
        CC = R1 < 0;
        R3 = 0;
        IF CC R7 = R3;          // R7 now equals the number of bits to shift
                                // by to get MSB in bit 31

        // 3)
        R2 = 32;
        R2 = R7 - R2;
        R3 = LSHIFT R0 BY R2.L; // bits to be shifted from R0 to R1
        R0 <<= R7;
        R1 <<= R7;
        R1 = R1 | R3;

        // So now in R1, R0 we have shifted the long long so that the top bit
        // of R1 is set.  R7 is the shift amount.
        // 4)
        R3 = R0 << 21;          // extract the bits that will be lost.
        R2 = 0x400;
        R0 = R0 + R2;
        CC = CARRY;
        R2 = CC;
        R1 = R1 + R2;           // carry will be set if we've overflowed the
                                // top half

        // 5)
        R2 = R1 << 21;
        R1 = ROT R1 BY -1;      // rotate in the carry
        R1 >>= 10;
        R0 >>= 11;
        R0 = R0 | R2;

        // 6)
        BITTGL(R3,31);
        CC = R3 == 0;
        R3 = CC;
        R3 = ~ R3;
        R0 = R0 & R3;

        // 7)
        R3 = (1023 + 62) (Z);
        R7 = R3 - R7;
        R7 <<= 20;
        R1 = R1 + R7;
        R7 = P1;
        RTS;

.is_32bit:
        JUMP.X ___unsigned_int32_to_float64;

.___unsigned_int64_to_float64.end:


.global ___unsigned_int64_to_float64;
.type ___unsigned_int64_to_float64,STT_FUNC;

.extern ___unsigned_int32_to_float64;

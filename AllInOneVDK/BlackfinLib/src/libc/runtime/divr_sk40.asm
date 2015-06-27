/****************************************************************************
 *
 * divr_sk40.asm : $Revision: 1.7 $
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 *
 ****************************************************************************/

#if defined(__DOCUMENTATION__)
    
   Function: __divr_sk40 - signed 40-bit accum division with rounding
    
   Synopsis:
    
      sk40 __divr_sk40(s64 num, s40 den)
  
   Description:
      
      The __divr_sk40 compiler support function performs a signed division of
      its integer operands and returns a signed fixed-point result with
      8 integer bits and 31 fractional bits.
      The denominator is limited to 40 bits, i.e. the range from 0x8000000000
      to 0x7FFFFFFFFF. The result is saturated to and rounded to nearest, with
      bias depending on the RND_MOD flag.
      
   Arguments:
   
      A non-standard calling convention is used in that the fourth word is
      expected in R3 instead of [SP + 12].

      R1:0 - numerator
      R3:2 - denominator

   Results:

      R1:0 - quotient

   Clobbered registers:
    
      Refer to syscall information below.
      
   Stack size:
    
      The three unused parameter slots plus three additional slots are used.

   Error Conditions:
    
      If the denominator is zero or out of range, the result of the division
      operation is undefined, although the function is guaranteed to return
      and take no longer than with any valid operands.

   Algorithm & Implementation:
        
      The __divrem_u96_u64 routine is used to perform the actual division.
      First the numerator is shifted left by 32 bits to account for the
      fractional result and the denominator is shifted left by one bit to
      account for the 1 bit of precision lost due to the sign bits.
      
********************************* **** * *  **********************************
#endif

#if defined(__SYSCALL_INFO__)
/* System call information */
%regs_clobbered "R0-R3,P0-P2,LC0,LT0,LB0,CCset"
%notes "Division of 64-bit integer by 40-bit accum with rounding."
%const
%pass_64bit_operands_in_regs
%syscall ___divr_sk40(K=lLK)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libName  = libdsp;
.FILE_ATTR libGroup = integer_support;
.FILE_ATTR libFunc  = ___divr_sk40;
.FILE_ATTR FuncName = ___divr_sk40;
.FILE_ATTR prefersMem = internal;
.FILE_ATTR prefersMemNum = "30";
#endif

.TYPE   ___divr_sk40, STT_FUNC;
.GLOBAL ___divr_sk40;

.EXTERN ___divrem_u96_u64;
.EXTERN ___divr_k40_round;

.SECTION/DOUBLEANY program;

#include <sys/anomaly_macros_rtl.h>

#if !defined(__ADSPLPBLACKFIN__)
  #define AC0 AC
#endif


.ALIGN 2;
___divr_sk40:

#ifdef __WORKAROUND_WB_DCACHE
      SSYNC;
#endif
      [SP + 8] = R7;

      // Save return address.
#ifdef __ADSPLPBLACKFIN__
      P2 = RETS;
#else
      R7 = RETS;
      P2 = R7;
#endif

      // Nothing to be done if numerator is zero.
      // (Saves some grief with LLONG_MIN.)
      R7 = R0 | R1;
      CC = R7 == 0;
      IF CC JUMP .ret;
      
#ifdef __WORKAROUND_WB_DCACHE
      SSYNC;
#endif

      // Shift denominator left by one. (And spill registers).
      R2 = ROT R2 BY 1  ||  [SP] = R4;
      R3 = ROT R3 BY 1  ||  [SP + 4] = R5;
      SP += -8;
      
      // Branch if denominator is negative.
      IF CC JUMP .den_neg;

.den_pos:

      CC = R1 < 0;
      IF CC JUMP .quo_neg;

.quo_pos:

      // Save denominator for rounding.
      R4 = R2;
      R5 = R3;
      
      // Fill in low word of 96-bit numerator.
      R7 = 0;

      // Let others do the real work.
      CALL.X ___divrem_u96_u64;
      CALL.X ___divr_k40_round;
      
      // Saturate to ACCUM_MAX.
      R2 = -1;
      R3 = 0x7F;
      CC = R3 < R1 (IU);
      IF CC R1 = R3;
      IF CC R0 = R2;   

      JUMP (P2);

      
.den_neg:

      // den = -den
      R2 = -R2;
      CC = AZ;
      R7 = CC;
      R3 = ~R3;
      R3 = R3 + R7;
      
      // num = -num
      R0 = -R0;
      CC = AZ;
      R7 = ~R1;
      R1 = CC;
      R1 = R1 + R7;
      
      CC = R7 < 0;
      IF !CC JUMP .quo_pos;
      
.quo_neg:
      
      // num = ~num
      R7 = -1;
      R0 = ~R0;
      R1 = ~R1;

      // Save denominator for rounding.
      R4 = R2;
      R5 = R3;
      
      CALL.X ___divrem_u96_u64;
      
      // Check for overflow.
      R7 = 0x80;
      CC = R7 <= R1 (IU);
      IF CC JUMP .quo_min;
      
      // quo = ~quo
      R0 = ~R0;
      R1 = ~R1;
      
      // rem = den + ~rem
      R2 = ~R2;
      R3 = ~R3;
      R2 = R2 + R4;
      CC = AC0;
      R7 = CC;
      R3 = R3 + R5;
      R3 = R3 + R7;
      
      RETS = P2;
      JUMP.X ___divr_k40_round;


.quo_min:
      
      // Negative overflow.
      R0 = 0;
      R1 = -0x80;
      
      SP += 8;
      R4 = [SP];
      R5 = [SP + 4]; 

.ret:
      R7 = [SP + 8];
      JUMP (P2);


.___divr_sk40.end:

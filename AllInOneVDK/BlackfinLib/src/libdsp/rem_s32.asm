/****************************************************************************
 *
 * rem_s32.asm : $Revision: 3543 $
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 *
 ****************************************************************************/

#if defined(__DOCUMENTATION__)
    
   Function: __rem_s32 - signed 32-bit remainder
    
   Synopsis:
    
      int32_t __rem_s32(int32_t num, int32_t den);

      int32_t __rem32(int32_t num, int32_t den);
  
   Description:
      
      The __rem_s32 compiler support function performs a signed
      division of its operands and returns the remainder.  As required by the
      C standard, the division is performed with rounding towards zero, which
      implies that the remainder takes the sign of the numerator (unless it is
      zero).
      
      __rem32 is an an alias for __rem_s32 that is provided for backwards
      compatibility.
        
   Arguments:

      R0 - numerator
      R1 - denominator

   Results:

      R0 - remainder = num % den

   Error Conditions:
    
      The remainder is undefined when the denominator is zero, although the
      function is guaranteed to return and take no longer than with any valid
      operands.

   Algorithm & Implementation:
      
      This is a cut-down version of divrem_s32.asm, where the code for
      extracting and signing the quotient was omitted.
      
   Register and flag usage:
      
      R2 - numerator, work register
      R3 - denominator
      R0 - num_sign, den_shift
      R1 - num_shift, temp
      P1 - quo_bits
      R0.31 - numerator sign
        
   Clobbered registers:
    
      Refer to syscall information below.
    
   Stack size:
    
      The function makes no use of the stack.
    
   Cycle counts:
        
      The function takes between 25 and 93 cycles, depending on the lengths
      of the operands. On average it takes about 53 cycles.
      (Measured on BF548 revision 0.0. Numbers include call and return).
    
********************************* **** * *  **********************************
#endif

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12%"
%notes "Signed 32-bit integer remainder."
%regs_clobbered "R0-R3,P1,LC0,LT0,LB0,CCset"
%const
%syscall ___rem32(I=II)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR  libGroup = integer_support;
.FILE_ATTR  libName  = libdsp;
.FILE_ATTR  libFunc  = ___rem_s32;
.FILE_ATTR  FuncName = ___rem_s32;
.FILE_ATTR  libFunc  = ___rem32;
.FILE_ATTR  FuncName = ___rem32;
.FILE_ATTR  prefersMem    = internal;
.FILE_ATTR  prefersMemNum = "30";
#endif

.TYPE   ___rem_s32, STT_FUNC;
.GLOBAL ___rem_s32;
.TYPE   ___rem32, STT_FUNC;
.GLOBAL ___rem32;

.SECTION/DOUBLEANY program;


.ALIGN 2;
___rem_s32:
___rem32:
      // Calculate the magnitudes of the operands.
      R2 = -R0;
      R3 = -R1;
      R2 = MAX(R0, R2);
      R3 = MAX(R1, R3);
      
      // Shortcut for quotient zero, in which case num == rem.
      CC = R2 < R3 (IU);
      R1 = R3 >> 1;  // Scheduled here to avoid SIGNBITS stall below.
      IF CC JUMP .ret;
      
      // Count leading zeroes of operands.
      R0.L = SIGNBITS R1;  // den_shift
      R1.L = SIGNBITS R2;  // num_shift

      // Line up operands.
      #ifdef __WORKAROUND_SHIFT
         // The 535 ignores the upper half of shift magnitudes.
         R2 <<= R1;
         R3 <<= R0;
      #else
         R2 = LSHIFT R2 BY R1.L;
         R3 = LSHIFT R3 BY R0.L;
      #endif
      
      // Calculate number of quotient bits and hence the loop count.
      R1 = R0 - R1;
      R1 = R1.L (Z);
      P1 = R1;
      
      // If the left-justified denominator has any bits set outside the top
      // fifteen, the full path has to be taken.
      R1 = R3 << 15;
      CC = R1;
      IF CC JUMP .full;
      
      // INT_MIN / 2**n needs to be treated specially, because it falls
      // just outside the valid range for the main loop.
      CC = R2 == R3;
      IF CC JUMP .zero;


   /* Quick path. */
.quick:      
      // Clear AQ and position denominator for DIVQ.
      AQ = CC;
      R1 = R3 >> 17;
      
      // Loop executed up to 32 times.
      LOOP(.quick_loop) LC0 = P1;
      LOOP_BEGIN .quick_loop;
         DIVQ(R2, R1);
      LOOP_END .quick_loop;
      
      // Correct remainder.
      R3 = R2 + R3;
      CC = AQ;
      IF !CC R3 = R2;

      // Extract remainder.
      #ifdef __WORKAROUND_SHIFT
         // Zero extension not needed on 535.
         R3 >>= R0;
      #else
         R1 = R0.L (Z);
         R3 >>= R1;
      #endif
      
      // Apply sign.
      CC = BITTST(R0, 31);
      R0 = -R3;
      IF !CC R0 = R3;
      
      RTS;


   /* Full 32-bit division. */
.full:
      // Position denominator for main loop, where bit 31 is considered a sign
      // bit. Using an arithmetic shift instead of a logical (i.e. unsigned)
      // shift here is a trick that saves an instruction for negating the MSB
      // of the work register to obtain a quotient bit (the ! infront of rotl1
      // in the C version). The MSB of the left-justified denominator is
      // guaranteed to be 1, so after the arithmetic right shift it will
      // remain 1. This way, the add and subtract instructions in the loop
      // will implicitly perform the required negation.
      R3 >>>= 1;
      
      // Loop executed up to 16 times.
      LOOP(.full_loop) LC0 = P1;
      LOOP_BEGIN .full_loop;
         R2 = R2 + R3, R1 = R2 - R3;
         IF CC R2 = R1;
         R2 = ROT R2 BY 1;
      LOOP_END .full_loop;

      // Correct remainder.
      R3 <<= 1;
      R3 = R2 + R3;
      IF CC R3 = R2;
      
      // Extract remainder.
      #ifdef __WORKAROUND_SHIFT
         // Zero extension not needed on 535.
         R3 >>= R0;
      #else
         R1 = R0.L (Z);
         R3 >>= R1;
      #endif
      
      // Apply sign.
      CC = BITTST(R0, 31);
      R0 = -R3;
      IF !CC R0 = R3;

      RTS;
      
      
.zero:
      R0 = 0;
.ret:      
      RTS;


.___rem_s32.end:
.___rem32.end:

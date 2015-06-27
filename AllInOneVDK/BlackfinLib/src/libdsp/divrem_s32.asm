/****************************************************************************
 *
 * divrem_s32.asm : $Revision: 3543 $
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 *
 ****************************************************************************/

#if defined(__DOCUMENTATION__)
    
   Function: __divrem_s32 - signed 32-bit division and remainder
    
   Synopsis:
    
      typedef struct { int quo; int rem; } __divrem_s32_t;

      __divrem_s32_t __divrem_s32 (int32_t num, int32_t den);
      div_t div(int numer, int denom);
      ldiv_t ldiv(long numer, long denom);
  
   Description:
      
      The __divrem_s32 compiler support function performs a signed
      division of its operands and returns both quotient and remainder.
      As required by the C standard, the quotient is rounded towards zero
      and the operands and results fulfil the following condition:
      
        num == den * quo + rem
        
      The requirement to round towards zero translates into these two
      conditions:
        
        num >= 0 ? rem >= 0 : rem <= 0 && abs(rem) < abs(den)
        
      This routine also implements the C standard library functions
      div() and ldiv().
        
   Arguments:

      R0 - numerator
      R1 - denominator

   Results:

      R0 - quotient  = num / den
      R1 - remainder = num % den

   Error Conditions:
    
      The result of the division operation is undefined when the denominator
      is zero, although the function is guaranteed to return and take no
      longer than with any valid operands.
      
      INT_MIN / -1 is also left undefined by the C standard, because the
      correct result is outwith the range of int. However, the function
      returns INT_MIN in order to stay consistent with the overflow behaviour
      of the negation operator.

   Algorithm:
      
      The same non-restoring add/subtract-shift algorithm as in unsigned
      32-bit division is used here. (See divrem_u32.c for more details.)
      
      Although the algorithm can be used with signed operands, it rounds
      towards negative infinity. Therefore the absolute value of the operands 
      is taken at the start so that it is run with unsigned numbers. The signs
      are saved separately and applied to the results at the end:
      
         num den => quo rem
          +   +      +   +
          +   -      -   +
          -   +      -   -
          -   -      +   -
   
   Implementation:
      
      The quotient and remainder signs are computed at the start and stored
      in the top two bits of register R1.

      A special case for MIN_INT divided by a power of 2 is required because
      that case the operands end up outside the valid range of the main loop
      (i.e. the magnitude of the remainder always has to be less than twice
      the denominator.)        
      
   Register and flag usage:
      
      R2 - numerator, work register
      R3 - denominator
      R0 - num_shift, temp
      R1 - den_shift, signs
      P1 - quo_bits
      R1.30 - quotient sign
      R1.31 - remainder sign
        
   Clobbered registers:
    
      Refer to syscall information below.
    
   Stack size:
    
      The function make no use of the stack.
    
   Cycle counts:
        
      The function takes between 26 and 102 cycles, depending on the lengths
      of the operands. On average it takes about 62 cycles.
      (Measured on BF548 revision 0.0. Numbers include call and return).
    
********************************* **** * *  **********************************
#endif

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12/ 12% kz"
%notes "Combined signed 32-bit integer division and remainder."
%regs_clobbered "R0-R3,P1,LC0,LT0,LB0,CCset"
%const
%syscall ___divrem_s32(xI=II)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR  libGroup      = integer_support;
.FILE_ATTR  libName       = libdsp;
.FILE_ATTR  libFunc       = ___divrem_s32;
.FILE_ATTR  FuncName      = ___divrem_s32;
.FILE_ATTR  libFunc       = _div;
.FILE_ATTR  FuncName      = _div;
.FILE_ATTR  libFunc       = _ldiv;
.FILE_ATTR  FuncName      = _ldiv;
.FILE_ATTR  prefersMem    = internal;
.FILE_ATTR  prefersMemNum = "30";
#endif

.TYPE   ___divrem_s32, STT_FUNC;
.GLOBAL ___divrem_s32;
.TYPE   _div, STT_FUNC;
.WEAK   _div;
.TYPE   _ldiv, STT_FUNC;
.WEAK   _ldiv;

.SECTION/DOUBLEANY program;


.ALIGN 2;
___divrem_s32:
_div:
_ldiv:
      // Calculate the magnitudes of the operands.
      R2 = -R0;
      R3 = -R1;
      R2 = MAX(R0, R2);
      R3 = MAX(R1, R3);
      
      // Shortcut for quotient zero.
      CC = R2 < R3 (IU);
      IF CC JUMP .zero;
      
      // Store remainder and quotient signs.
      R1 = R0 ^ R1;
      CC = BITTST(R0, 31);
      R0 = R3 >> 1;  // scheduled here to avoid SIGNBITS stall below
      R1 = ROT R1 BY -1;
      
      // Count leading zeroes of operands.
      R1.L = SIGNBITS R0;  // den_shift
      R0.L = SIGNBITS R2;  // num_shift
      
      // Line up operands.
      #ifdef __WORKAROUND_SHIFT
         // The 535 ignores the upper half of shift magnitudes.
         R2 <<= R0;
         R3 <<= R1;
      #else
         R2 = LSHIFT R2 BY R0.L;
         R3 = LSHIFT R3 BY R1.L;
      #endif
      
      // Calculate number of quotient bits and hence the loop count.
      R0 = R1 - R0;
      R0 = R0.L (Z);
      P1 = R0;
      
      // If the left-justified denominator has any bits set outside the top
      // fifteen, the full path has to be taken.
      R0 = R3 << 15;
      CC = R0;
      IF CC JUMP .full;
      
      // INT_MIN / 2**n needs to be treated specially, because it falls
      // just outside the valid range for the main loop.
      CC = R2 == R3;
      IF CC JUMP .intmin_pow2;


   /* Quick path. */
.quick:      
      // Clear AQ and position denominator for DIVQ.
      AQ = CC;
      R0 = R3 >> 17;
      
      // Loop executed up to 32 times.
      LOOP(.quick_loop) LC0 = P1;
      LOOP_BEGIN .quick_loop;
         DIVQ(R2, R0);
      LOOP_END .quick_loop;
      
      // Correct remainder.
      R3 = R2 + R3;
      CC = AQ;
      IF !CC R3 = R2;

      // Extract remainder.
      #ifdef __WORKAROUND_SHIFT
         // Zero extension not needed on 535.
         R3 >>= R1;
      #else
         R0 = R1.L (Z);
         R3 >>= R0;
      #endif
      
      // Extract quotient.
      R2 = EXTRACT(R2, R1.L) (Z);
      
      // Apply signs.
      CC = BITTST(R1, 30);
      R0 = -R2;
      IF !CC R0 = R2;
      CC = BITTST(R1, 31);
      R1 = -R3;
      IF !CC R1 = R3;
      
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
         R2 = R2 + R3, R0 = R2 - R3;
         IF CC R2 = R0;
         R2 = ROT R2 BY 1;
      LOOP_END .full_loop;

      // Correct remainder.
      R3 <<= 1;
      R3 = R2 + R3;
      IF CC R3 = R2;
      
      // Extract remainder.
      #ifdef __WORKAROUND_SHIFT
         // Zero extension not needed on 535.
         R3 >>= R1;
      #else
         R0 = R1.L (Z);
         R3 >>= R0;
      #endif
      
      // Pick up last quotient bit and extract quotient.
      R2 = ROT R2 BY 1;
      R0 = P1;
      CC = BITTST(R1, 30);  // scheduled here to avoid EXTRACT stall
      R2 = EXTRACT(R2, R0.L) (Z);
      
      // Apply signs.
      R0 = -R2;
      IF !CC R0 = R2;
      CC = BITTST(R1, 31);
      R1 = -R3;
      IF !CC R1 = R3;
      
      RTS;


   /* Special case for zero quotient. */
.zero:
      R1 = R0;
      R0 = 0;
      RTS;


   /* Special case for MIN_INT divided by power of 2. */
.intmin_pow2:
      R2 = 1;
      #ifdef __WORKAROUND_SHIFT
         R2 <<= R1;
      #else
         R2 = LSHIFT R2 BY R1.L;
      #endif

      CC = BITTST(R1, 30);
      R0 = -R2;
      IF !CC R0 = R2;
            
      R1 = 0;
      
      RTS;


._ldiv.end:
._div.end:
.___divrem_s32.end:

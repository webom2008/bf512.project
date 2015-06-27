/*****************************************************************************
 *
 * divrem_s64.asm : $Revision: 5132 $
 *
 * (c) Copyright 2008-2011 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)
    
   Function: __divrem_s64 - signed 64-bit division and remainder
   
   Synopsis:
   
      typedef struct { int64_t quo; int64_t rem; } __divrem_s64_t;
      
      __divrem_s64_t __divrem_s64(int64_t num, int64_t den);
  
      int64_t __divdi3(int64_t num, int64_t den);

   Description:
      
      The __divrem_s64 compiler support function performs a signed
      division of its operands and returns the quotient.
      As required by the C standard, the quotient is rounded towards zero
      and the operands and results fulfil the following condition:
      
        num == den * quo + rem
        
      The requirement to round towards zero translates into these two
      conditions:
        
        num >= 0 ? rem >= 0 : rem <= 0 && abs(rem) < abs(den)
        
      __divdi3 is an additional entry point that is provided for backwards
      compatibility. It differs from __divrem_s64 only in its calling 
      convention: the upper half of the denominator is loaded from the stack
      into R3, and the remainder in R3:2 is ignored.
      
   Arguments:

      R1:0 - numerator
      R3:2 - denominator
      
   Results:

      R1:0 - quotient  = numerator / denominator
      R3:2 - remainder = numerator % denominator
        
   Error Conditions:
    
      The result of the division operation is undefined when the denominator
      is zero, although the function is guaranteed to return and take no
      longer than with any valid operands.
        
      LLONG_MIN / -1 is also left undefined by the C standard, because the
      correct result is outwith the range of long long. However, the
      function returns LLONG_MIN in order to stay consistent with the
      overflow behaviour of the negation operator.

   Algorithm & Implementation:
        
      This function only handles the signs of the arguments and results, while
      the unsigned __divrem_u64 routine is called to perform the actual
      division operation. 
      
   Register Usage:
   
      Scratch:   Refer to syscall information below.
      Preserved: R7:4, SP
    
   Stack Usage:
    
      The three stack slots reserved for the first three parameters plus two
      additional slot are used for spilling registers.
      
   Dependencies:
   
      divrem_u64.asm
    
   Cycle Counts:
        
      The cycle count depends on the bit lengths and signs of the arguments.
      For positive arguments, __divrem_s64 takes at most 365 cycles and about
      163 cycles on average. If either of the arguments is negative,
      __divrem_64 takes at most 398 cycles and about 195 cycles on average.
      (The numbers were measured on BF548 revision 0.0 and include procedure
      call and return).
      
********************************* **** * *  **********************************
#endif

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12/ 12% kz"
%notes "Combined signed 64-bit integer division and remainder."
%regs_clobbered "R0-R3,P0-P2,LC0,LT0,LB0,CCset"
%const
%pass_64bit_operands_in_regs
%return_register_quad
%syscall ___divrem_s64(xlL=lLlL)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libGroup = integer_support;
.FILE_ATTR libName  = libdsp;
.FILE_ATTR libFunc  = ___divrem_s64;
.FILE_ATTR FuncName = ___divrem_s64;
.FILE_ATTR libFunc  = ___divdi3;
.FILE_ATTR FuncName = ___divdi3;
.FILE_ATTR prefersMem = internal;
.FILE_ATTR prefersMemNum = "30";
#endif

.TYPE   ___divrem_s64, STT_FUNC;
.GLOBAL ___divrem_s64;
.TYPE   ___divdi3, STT_FUNC;
.GLOBAL ___divdi3;

.EXTERN ___divrem_u64;

.SECTION/DOUBLEANY program;


.ALIGN 2;
___divdi3:
      
      // Fetch upper half of denominator from stack.
      R3 = [SP + 12];
      
 ___divrem_s64:

      CC = R3 < 0;
      IF CC JUMP .any_by_neg;
      
.any_by_pos:
      
      CC = R1 < 0;
      IF CC JUMP .neg_by_pos;
      
.pos_by_pos:
      
      JUMP.X ___divrem_u64;
      
      
.neg_by_pos:
      
#ifdef __WORKAROUND_WB_DCACHE
      SSYNC;
#endif
      [SP + 8] = R7;
      
#ifdef __ADSPLPBLACKFIN__
      P2 = RETS;
#else
      R7 = RETS;  // 'Pd = RETS' is invalid on 535.
      P2 = R7;
#endif

      // Negate numerator.
      R0 = -R0;
      CC = AZ;
      R7 = CC;
      R1 = ~R1;
      R1 = R1 + R7;
      
      CALL.X ___divrem_u64;
      
      // Negate quotient.
      R0 = -R0;
      CC = AZ;
      R7 = CC;
      R1 = ~R1;
      R1 = R1 + R7;

      // Negate remainder.
      R2 = -R2;
      CC = AZ;
      R7 = CC;
      R3 = ~R3;
      R3 = R3 + R7 (NS)  ||  R7 = [SP + 8];
      
      JUMP (P2);
      
      
.any_by_neg:

#ifdef __WORKAROUND_WB_DCACHE
      SSYNC;
#endif
      [SP + 8] = R7;
      
#ifdef __ADSPLPBLACKFIN__
      P2 = RETS;
#else
      R7 = RETS;  // 'Pd = RETS' is invalid on 535.
      P2 = R7;
#endif
      
      // Negate denominator.
      R2 = -R2;
      CC = AZ;
      R7 = CC;
      R3 = ~R3;
      R3 = R3 + R7;
      
      CC = R1 < 0;
      IF !CC JUMP .pos_by_neg;

.neg_by_neg:

      // Negate numerator.
      R0 = -R0;
      CC = AZ;
      R7 = CC;
      R1 = ~R1;
      R1 = R1 + R7;
      
      CALL.X ___divrem_u64;
      
      // Negate remainder.
      R2 = -R2;
      CC = AZ;
      R7 = CC;
      R3 = ~R3;
      R3 = R3 + R7 (NS)  ||  R7 = [SP + 8];
      
      JUMP (P2);

.ALIGN 4;
.pos_by_neg:
      
      CALL.X ___divrem_u64;

      // Negate quotient.
      R0 = -R0;
      CC = AZ;
      R7 = CC;
      R1 = ~R1;
      R1 = R1 + R7 (NS)  ||  R7 = [SP + 8];
      
      JUMP (P2);
      
      
.___divrem_s64.end:
.___divdi3.end:

/*****************************************************************************
 *
 * rem_s64.asm : $Revision: 5132 $
 *
 * (c) Copyright 2008-2011 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)
    
   Function: __moddi3 - signed 64-bit remainder
   
   Synopsis:
   
      int64_t __moddi3(int64_t num, int64_t den);
  
   Description:
      
      The __moddi3 compiler support function performs a signed division of 
      its operands and returns the remainder. As required by the
      C standard, the division is performed with rounding towards zero, which
      implies that the remainder takes the sign of the numerator (unless it is
      zero).
      
      This function is provided for backwards compatibility only. New code
      should call __divrem_s64 instead.
        
   Arguments:

      R1:0 - numerator
      R3:2 - denominator (R3 is passed on the stack)
      
   Results:

      R1:0 - remainder = numerator % denominator
        
   Error Conditions:
    
      The result of the division operation is undefined when the denominator
      is zero, although the function is guaranteed to return and take no
      longer than with any valid operands.
      
   Algorithm & Implementation:
        
      This function only handles the signs of the arguments and result, while 
      __divrem_u64 is called to perform the actual division operation. 
      
   Register Usage:
   
      Scratch:   Refer to syscall information below.
      Preserved: R7:4, SP
    
   Stack Usage:
    
      The three stack slots reserved for the first three parameters plus two
      additional slots are used.
      
   Dependencies:
   
      divrem_u64.asm

   Cycle Counts:
        
      The cycle count depends on the bit lengths and signs of the arguments.
      For positive arguments, __rem_s64 takes at most 373 cycles and about
      168 cycles on average. If either of the arguments is negative,
      __rem_s64 takes at most 395 cycles and about 194 cycles on average.
      (The numbers were measured on BF548 revision 0.0 and include procedure
      call and return).
      
********************************* **** * *  **********************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libGroup = integer_support;
.FILE_ATTR libName  = libdsp;
.FILE_ATTR libFunc  = ___moddi3;
.FILE_ATTR FuncName = ___moddi3;
.FILE_ATTR prefersMem = internal;
.FILE_ATTR prefersMemNum = "30";
#endif

.TYPE   ___moddi3, STT_FUNC;
.GLOBAL ___moddi3;

.EXTERN ___divrem_u64;

.SECTION/DOUBLEANY program;


.ALIGN 2;
___moddi3:

#ifdef __ADSPLPBLACKFIN__
      P2 = RETS;
#else
      R3 = RETS;
      P2 = R3;
#endif

      R3 = [SP + 12];

      CC = R3 < 0;
      IF CC JUMP .any_by_neg;
      
.any_by_pos:
      
      CC = R1 < 0;
      IF CC JUMP .neg_by_pos;
      
.pos_by_pos:
      
      CALL.X ___divrem_u64;
      
      // Copy remainder.
      R0 = R2;
      R1 = R3;
      
      JUMP (P2);            
      
.neg_by_pos:
      
#ifdef __WORKAROUND_WB_DCACHE
      SSYNC;
#endif
      [SP + 8] = R7;

      // Negate numerator.
      R0 = -R0;
      CC = AZ;
      R7 = CC;
      R1 = ~R1;
      R1 = R1 + R7;
      
      CALL.X ___divrem_u64;
      
      // Negate remainder and move it to R1:0.
      R0 = -R2;
      CC = AZ;
      R7 = CC;
      R1 = ~R3;
      R1 = R1 + R7 (NS)  ||  R7 = [SP + 8];
      
      JUMP (P2);
      
      
.any_by_neg:

#ifdef __WORKAROUND_WB_DCACHE
      SSYNC;
#endif
      [SP + 8] = R7;

      // Negate denominator.
      R2 = -R2;
      CC = AZ;
      R7 = CC;
      R3 = ~R3;
      R3 = R3 + R7 (NS)  ||  R7 = [SP + 8];
      
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
      
      // Negate remainder and move it to R1:0.
      R0 = -R2;
      CC = AZ;
      R7 = CC;
      R1 = ~R3;
      R1 = R1 + R7 (NS)  ||  R7 = [SP + 8];
      
      JUMP (P2);

.pos_by_neg:
      
      CALL.X ___divrem_u64;
      
      // Copy remainder.
      R0 = R2;
      R1 = R3;
      
      JUMP (P2);
      
      
.___moddi3.end:

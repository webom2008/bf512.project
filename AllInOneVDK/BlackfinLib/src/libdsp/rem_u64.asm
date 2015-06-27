/*****************************************************************************
 *
 * rem_u64.asm : $Revision: 5132 $
 *
 * (c) Copyright 2008-2011 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)
    
   Function: __umoddi3 - unsigned 64-bit remainder
   
   Synopsis:
   
      uint64_t __umoddi3(uint64_t num, uint64_t den);
  
   Description:
      
      The __umoddi3 compiler support function performs an unsigned
      division of its arguments and returns the remainder. The quotient is 
      assumed to be rounded towards zero, so that the remainder
      fulfils the following condition:

         (0 <= rem)  &&  (rem <= den)
      
      This function is provided for backwards compatibility only. New code
      should directly call __divrem_u64 instead.
        
   Arguments:

      R1:0 - numerator
      R3:2 - denominator (R3 is passed on the stack)
      
   Results:

      R1:0 - remainder = numerator % denominator
        
   Error Conditions:
    
      The result is undefined when the denominator is zero, although the
      function is guaranteed to return and take no longer than with any valid
      operands.
        
   Implementation:
        
      ___divrem_u64 is invoked to perform the actual division operation, and
      afterwards the remainder is moved from R3:2 to R1:0.
      
   Register Usage:
   
      Scratch:   Refer to syscall information below.
      Preserved: R7:4, SP
    
   Stack Usage:
    
      The stack slots reserved for the first three argument registers and
      two additional stack slots are used for spilling of registers.
      
   Dependencies:
   
      divrem_u64.asm
   
   Cycle Counts:
        
      The cycle count depends on the bit lengths of the arguments.
      __rem_u64 takes at most 379 cycles and about 172 cycles on average,
      including call and return. (Measured on BF548 revision 0.0).
      
********************************* **** * *  **********************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libGroup = integer_support;
.FILE_ATTR libName  = libdsp;
.FILE_ATTR libFunc  = ___umoddi3;
.FILE_ATTR FuncName = ___umoddi3;
.FILE_ATTR prefersMem = internal;
.FILE_ATTR prefersMemNum = "30";
#endif

.TYPE   ___umoddi3, STT_FUNC;
.GLOBAL ___umoddi3;

.EXTERN ___divrem_u64;

.SECTION/DOUBLEANY program;


.ALIGN 2;
___umoddi3:

#ifdef __ADSPLPBLACKFIN__
      P2 = RETS;
#else
      R3 = RETS;
      P2 = R3;
#endif

      R3 = [SP + 12];
      
      CALL.X ___divrem_u64;
      
      // Copy remainder.
      R0 = R2;
      R1 = R3;
      
      JUMP (P2);

.___umoddi3.end:

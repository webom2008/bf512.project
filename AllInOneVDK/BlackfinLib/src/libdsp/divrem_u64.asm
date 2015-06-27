/*****************************************************************************
 *
 * divrem_u64.asm : $Revision: 5132 $
 *
 * (c) Copyright 2008-2011 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)
    
   Function: __divrem_u64 - unsigned 64-bit division and remainder
   
   Synopsis:
   
      typedef struct { uint64_t quo; uint64_t rem; } __divrem_u64_t;
      
      __divrem_u64_t __divrem_u64(uint64_t num, uint64_t den);
                    
      uint64_t __udivdi3(uint64_t num, uint64_t den);
  
   Description:
      
      The __divrem_u64 compiler support function performs an unsigned
      division of its arguments and returns both quotient and remainder.
      As required by the C standard, the arguments and return values fulfil
      the following conditions:
      
         (num == den * quo + rem)  &&  (0 <= rem)  &&  (rem <= den)
      
      __udivdi3 is an additional entry point that is provided for backwards
      compatibility. It differs from __divrem_u64 only in its calling 
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
        
   Algorithm:
        
      A restoring subtract&shift algorithm is used is used to perform the 
      division operation. (See e.g. "Computer Arithmetic Algorithms" by
      Israel Koren.)
      
      The SIGNBITS instruction is used to determine the lengths of the
      operands and cut out loop iterations that merely line up the operands.
      
   Implementation:
        
      When both numerator and denominator are no longer than 32 bits,
      __divrem_u32 is called to perform the division. Otherwise there
      are two separate paths. The div_64_32 path handles cases where the
      denominator is no longer than 32 bits, whereas .div_64_64 handles
      64-bit denominators. In the 32-bit denominator case the carry bit does
      not need to be considered, whereas in the 64-bit case carry handling
      makes up a large part of the inner loop due to the lack of add/subtract
      with carry instructions.
      
   Register Usage:
   
      Scratch:   Refer to syscall information below.
      Preserved: R7:4, SP
    
   Stack Usage:
    
      The stack slots reserved for the first two arguments plus two
      additional stack slots are used for spilling registers.
      (The third argument slot is reserved for divrem_s64).
   
   Dependendencies:
   
      divrem_u32.asm
      
   Dependents:
   
      rem_u64.asm, divrem_s64.asm, rem_s64.asm
    
   Cycle Counts:
        
      The cycle count depends on the bit lengths of the arguments.
      __divrem_u64 takes at most 366 cycles and about 157 cycles on average,
      including call and return. (Measured on BF548 revision 0.0).
  
********************************* **** * *  **********************************
#endif

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12u/ 12u% kz"
%regs_clobbered "R0-R3,P0-P2,LC0,LT0,LB0,CCset"
%const
%notes "Combined unsigned 64-bit integer division and remainder."
%pass_64bit_operands_in_regs
%return_register_quad
%syscall ___divrem_u64(xulL=ulLulL)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libName  = libdsp;
.FILE_ATTR libGroup = integer_support;
.FILE_ATTR libFunc  = ___divrem_u64;
.FILE_ATTR FuncName = ___divrem_u64;
.FILE_ATTR libFunc  = ___udivdi3;
.FILE_ATTR FuncName = ___udivdi3;

.FILE_ATTR libFunc  = ___divrem_s64;
.FILE_ATTR libFunc  = ___umoddi3;
.FILE_ATTR libFunc  = ___divdi3;
.FILE_ATTR libFunc  = ___moddi3;

.FILE_ATTR libFunc  = ___divrem_u96_u64;
.FILE_ATTR libFunc  = ___divt_uk40;
.FILE_ATTR libFunc  = ___divt_sk40;
.FILE_ATTR libFunc  = ___divr_uk40;
.FILE_ATTR libFunc  = ___divr_sk40;

.FILE_ATTR prefersMem    = internal;
.FILE_ATTR prefersMemNum = "30";
#endif

.TYPE   ___divrem_u64, STT_FUNC;
.GLOBAL ___divrem_u64;
.TYPE   ___udivdi3, STT_FUNC;
.GLOBAL ___udivdi3;

.EXTERN ___divrem_u32;

.SECTION/DOUBLEANY program;

#include <sys/anomaly_macros_rtl.h>

#if !defined(__ADSPLPBLACKFIN__)
  #define AC0 AC
#endif


.ALIGN 2;
___udivdi3:
      
      // Fetch upper half of denominator from stack.
      R3 = [SP + 12];
      
___divrem_u64:

      CC = R3 == 0;
      IF !CC JUMP .div_64_64;

      CC = R1 == 0;
      IF CC JUMP .div_32_32;
      
   /* Path for denominators up to 32 bits. */
.div_64_32:

#ifdef __WORKAROUND_WB_DCACHE
      SSYNC;
#endif
      
      // Check if denominator is a power of 2
      R3.L = ONES R2  ||  [SP] = R4;
      CC = R3 == 1;
      IF CC JUMP .shift_64_32;

      // Count leading zeroes of operands.
      R4 = R2 >> 1;
      R5 = R1 >> 1  ||  [SP + 4] = R5;
      R3.L = SIGNBITS R4;
      R5.L = SIGNBITS R5;
      R5 = R5.L (Z);
      
      // Left-justify numerator.
      R1 <<= R5;
      R4 = LSHIFT R0 BY R5.L;
      R5 += -32;
      R0 = LSHIFT R0 BY R5.L;

      // Calculate number of quotient bits.
      R5 = R3 - R5;
      P1 = R5;

      // Complete left-justified numerator (in R5:4).
      R5 = R1 | R0;
      
      // Left-justify denominator.
      R1 = LSHIFT R2 BY R3.L;
      
      // Calculate top quotient bit (leaving remainder in R2).
      R2 = R5 - R1;
      CC = AC0;
      IF !CC R2 = R5;
      
      // Loop executed up to 63 times.
      LOOP(.div_64_32_loop) LC0 = P1;
      LOOP_BEGIN .div_64_32_loop;
         R4 = ROT R4 BY 1;
         R5 = ROT R2 BY 1;
         R2 = R5 - R1;
         CC |= AC0;
         IF !CC R2 = R5;
      LOOP_END .div_64_32_loop;
      
      // Extract quotient and fetch last quotient bit. 
      R0 = ROT R4 BY 1            ||  R4 = [SP];
      R1 = EXTRACT(R2, R3.L) (Z)  ||  R5 = [SP + 4];
      R1 = ROT R1 BY 1 ;           

      // Extract remainder.
      R2 >>= R3;
      R3 = 0;
      
      RTS;


   /* Denominator is a 32-bit power of 2: a shift will do */
.shift_64_32:
      
      // Compute shift magnitude(s).
      R2 >>= 1;
      R4 = 31;
      R3.L = SIGNBITS R2;
      R4 = R4 - R3;
      R3 += 1;
      
      // Extract remainder.
      R2 = EXTRACT(R0, R4.L) (Z);
      
      // Shift to get quotient.
      R3 = LSHIFT R1 BY R3.L;
      R0 >>= R4;
      R1 >>= R4;
      R0 = R0 + R3 (NS)  ||  R4 = [SP];
      
      // Upper half of remainder is zero.
      R3 = 0;
      
      RTS;


   /* Path for denominators longer than 32 bits. */
#ifdef __WORKAROUND_WB_DCACHE
.div_64_64:
     SSYNC;
#else
.ALIGN 8;
.div_64_64:
#endif
      // Count leading zeroes of operands.
      R4 = R3 >> 1             ||  [SP--] = R4;
      R5 = R1 >> 1             ||  [SP--] = R5;
      R4.L = SIGNBITS R4       ||  [SP] = R6;
      R6 = 0x201;  // EXTRACT pattern for AC0 (here to avoid EXPADJ stall)
      R5.L = EXPADJ(R5, R4.L)  ||  [SP + 12] = R7;

      // Calculate number of quotient bits.
      R5 = R4 - R5;
      R7 = R5.L (Z);
      P1 = R7;
      
      // Align the denominator with the numerator (leaving it in R5:4).
      R5 += -32;
      R5 = LSHIFT R2 BY R5.L;
      R4 = LSHIFT R2 BY R7.L;
      R3 <<= R7;
      R5 = R3 | R5;
      
      // Prepare for subtract-with-carry in inner loop.
      R5 = ~R5;
      
      // Loop executed up to 32 times.
      P1 += 1;
      LOOP(.div_64_64_loop) LC0 = P1;
      LOOP_BEGIN .div_64_64_loop;
         // 64-bit subtract: remainder - denominator
         // EXTRACT is used to get the carry from the lower word, so that CC,
         // which contains the carry from the ROT operations is not clobbered
         R2 = R0 - R4;
         R3 = ASTAT;
#if WA_05000209
         NOP;
#endif
         R3 = EXTRACT(R3, R6.L) (Z);
         R3 = R3 + R5;
         R3 = R3 + R1;
         
         // Keep the difference, if remainder was greater than the denominator.
         CC |= AC0;
         IF !CC R2 = R0;
         IF !CC R3 = R1;
         
         // Store the quotient bit and shift up the remainder.
         R0 = ROT R2 BY 1;
         R1 = ROT R3 BY 1;      
      LOOP_END .div_64_64_loop;      
      
      // Extract quotient and fetch last quotient bit.
      CC = BITTST(R0, 0);
      R0 = EXTRACT(R2, R7.L) (Z)  ||  R6 = [SP++];
      R0 = ROT R0 BY 1            ||  R5 = [SP++];
      
      // Extract remainder (doing a 64-bit shift.)
      R1 = -R7;
      R1 += 32;
      R1 = LSHIFT R3 BY R1.L      ||  R4 = [SP];
      R3 >>= R7;
      R2 >>= R7;
      R2 = R2 + R1 (NS)           ||  R7 = [SP + 4];
      
      // Upper half of quotient is zero.
      R1 = 0;
      
      RTS;


   /* 32/32 divisions are passed on to __divrem_u32. */
.div_32_32:
      
      R1 = R2;
#ifdef __ADSPLPBLACKFIN__
      P0 = RETS;
#else
      R3 = RETS;
      P0 = R3;
#endif
      CALL.X ___divrem_u32;
      R2 = R1;
      R1 = 0;
      R3 = 0;
      JUMP (P0);


.___divrem_u64.end:
.___udivdi3.end:

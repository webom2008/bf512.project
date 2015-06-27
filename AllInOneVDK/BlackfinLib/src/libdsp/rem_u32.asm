/****************************************************************************
 *
 * div_u32.asm : $Revision: 3543 $
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 *
 ****************************************************************************/

#if defined(__DOCUMENTATION__)
    
   Function: __rem_u32 - unsigned 32-bit division
    
   Synopsis:
    
      uint32_t __rem_u32(uint32_t num, uint32_t den);
                    
      uint32_t __urem32(uint32_t num, uint32_t den);
  
   Description:
      
      The __rem_u32 compiler support function performs an unsigned
      division of its operands. As required by the C standard, the quotient is
      rounded towards zero.
      
      __urem32 is an an alias for __rem_u32 that is provided for backwards
      compatibility.
      
   Arguments:

      R0 - numerator
      R1 - denominator

   Results:

      R0 - remainder  = num % den

   Error Conditions:
    
      The result of the division operation is undefined when the denominator
      is zero, although the function is guaranteed to return and take no
      longer than with any valid operands.

   Algorithm & Implementation:
        
      This is a cut-down version of divrem_u32.asm, without the code for
      extracting the quotient. See there for details.
      
   Register usage:
      
      R0 - numerator, work register
      R1 - denominator
      R2 - num_shift, multi-purpose temp
      R3 - den_shift
      P1 - quo_bits
        
   Clobbered registers:
    
      Refer to syscall information below.
      
   Dependencies:
   
      rems_u32.asm depends on the clobber set above.
    
   Stack size:
    
      The function makes no use of the stack.
    
   Cycle counts:
        
      The function takes up to 89 cycles, depending on the lengths
      of the operands. On average it takes about 47 cycles.
      (Measured on BF548 revision 0.0. Numbers include call and return).
    
********************************* **** * *  **********************************
#endif

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12u%"
%notes "Unsigned 32-bit integer remainder."
%regs_clobbered "R0-R3,P1,LC0,LT0,LB0,CCset"
%const
%syscall ___urem32(U=UU)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR  libGroup = integer_support;
.FILE_ATTR  libName  = libdsp;
.FILE_ATTR  libFunc  = ___rem_u32;
.FILE_ATTR  FuncName = ___rem_u32;
.FILE_ATTR  libFunc  = ___urem32;
.FILE_ATTR  FuncName = ___urem32;
.FILE_ATTR  prefersMem    = internal;
.FILE_ATTR  prefersMemNum = "30";
#endif

.TYPE   ___rem_u32, STT_FUNC;
.GLOBAL ___rem_u32;
.TYPE   ___urem32, STT_FUNC;
.GLOBAL ___urem32;

#if !defined(__ADSPLPBLACKFIN__)
  #define AC0 AC
#endif

.SECTION/DOUBLEANY program;


.ALIGN 4;
___rem_u32:
___urem32:
      // If the numerator is less than twice the denominator, the quotient
      // is a single bit that can be determined by simple comparison.
      R2 = R0 >> 1;
      CC = R2 < R1 (IU);
      IF CC JUMP .one_bit;

      // Count leading zeroes of arguments and left-adjust arguments.
      R3 = R1 >> 1;
      R2.L = SIGNBITS R2;
      R3.L = SIGNBITS R3;
      #ifndef __WORKAROUND_SHIFT
         // The 535 ignores the upper half of shift magnitudes anyway.
         R2 = R2.L (Z);
         R3 = R3.L (Z);
      #endif
      R0 <<= R2;
      R1 <<= R3;
      
      // Calculate number of quotient bits and hence the loop count.
      R2 = R3 - R2;
      #ifdef __WORKAROUND_SHIFT
         // Even on the 535 the loop count is a 32-bit number though.
         R2 = R2.L (Z);
      #endif
      P1 = R2;

      // Bring remainder into range for add/subtract algorithm.
      R0 = R0 - R1;

      // If the left-justified denominator has any bits set outside the top
      // fifteen, the full path has to be taken.
      R2 = R1 << 15;
      CC = R2;
      IF CC JUMP .full;


   /* Quick path. */
.quick:
      // Fetch top quotient bit into AQ.
      CC = BITTST(R0, 31);
      AQ = CC;

      // Position denominator for DIVQ.
      R2 = R1 >> 17;
      
      // Loop executed up to 32 times.
      LOOP(.quick_loop) LC0 = P1;
      LOOP_BEGIN .quick_loop;
         DIVQ (R0, R2);
      LOOP_END .quick_loop;
      
      // Correct and extract remainder.
      CC = AQ;
      R1 = R0 + R1;
      IF CC R0 = R1;
      R0 >>= R3;
   
      RTS;
      
      
   /* Full 32-bit division. */
.full:
      // Fetch top quotient bit.
      CC = !BITTST(R0, 31);
      
      // Position denominator for main loop, where bit 31 is considered a sign
      // bit. Using an arithmetic shift instead of a logical (i.e. unsigned)
      // shift here is a trick that saves an instruction for negating the MSB
      // of the work register to obtain a quotient bit (the ! infront of rotl1
      // in the C version). The MSB of the left-justified denominator is
      // guaranteed to be 1, so after the arithmetic right shift it will
      // remain 1. This way, the add and subtract instructions in the loop
      // will implicitly perform the required negation.
      R1 >>>= 1;
      
      // Loop executed up to 16 times.
      LOOP(.full_loop) LC0 = P1;
      LOOP_BEGIN .full_loop;
         R0 = R0 + R1, R2 = R0 - R1;
         IF CC R0 = R2;                // if CC R0 -= R1; else R0 += R1;
         R0 = ROT R0 BY 1;
      LOOP_END .full_loop;
      
      // Correct and extract remainder.
      R1 <<= 1;
      R1 = R0 + R1;
      IF !CC R0 = R1;
      R0 >>= R3;
            
      RTS;
      
      
   /* Single-bit quotient case. */
.one_bit:
      // Compare numerator and denominator to determine one-bit quotient.
      // Adjust remainder accordingly.
      CC = R1 <= R0 (IU);
      R1 = R0 - R1;
      IF CC R0 = R1;

      RTS;


.___rem_u32.end:
.___urem32.end:

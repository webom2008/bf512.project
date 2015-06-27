/****************************************************************************
 *
 * divrem_u32.asm : $Revision: 3543 $
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 *
 ****************************************************************************/

#if defined(__DOCUMENTATION__)
    
   Function: __divrem_u32 - unsigned 32-bit division and remainder
    
   Synopsis:
    
      typedef struct { uint quo; uint rem; } __divrem_u32_t;

      __divrem_u32_t __divrem_u32(uint32_t num, uint32_t den);
                    
   Description:
      
      The __divrem_u32 compiler support function performs an unsigned
      division of its operands and returns both quotient and remainder.
      As required by the C standard, the quotient is rounded towards zero
      and the operands and results fulfil the following conditions:
      
        (num == den * quo + rem)  &&  (0 <= rem)  &&  (rem <= den)
      
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

   Algorithm:
        
      A non-restoring add/subtract-shift algorithm is used to perform the 
      division operation. (See e.g. "Computer Arithmetic Algorithms" by
      Israel Koren.) 
      
      The Blackfin SIGNBITS instruction is used to determine the lengths of
      the operands in order to cut out loop iterations that merely line up
      the operands.
      
      In C the function implementation might look as follows. (The signbits()
      function is assumed to do the same as the SIGNBITS instruction, i.e.
      return the number of sign bits minus one. rotl1() is assumed to do a
      rotate left by 1 bit.)
        
         // Calculate leading number of zeroes of both operands.
         uint num_shift = lzeroes(num);
         uint den_shift = lzeroes(den);
         
         // Calculate the number of resulting quotient bits.
         // (Actually one bit more than this needs to be calculated.)
         int quo_bits = den_shift - num_shift;
         
         // If the numerator is no longer than the denominator,
         // the quotient is a single bit.
         if (quo_bits <= 0) {
           bool quo = num >= den;
           return (__divrem_u32_t){quo, num - den * quo};
         }
         
         // Left-justify the operands.
         num <<= num_shift;
         den <<= den_shift;
         
         // Compute the first quotient bit.
         bool quo_bit = num >= den;
         
         // Adjust operands for main add/subtract-shift loop.
         uint rnq = num - den;
         den >>= 1;
         
         // Main loop.
         int i = quo_bits;
         while (i--) {
           rnq += quo_bit ? -den : den;
           quo_bit = !rotl1(&rnq, quo_bit);
         }
         
         // Compensate for the "non-restoring" algorithm to get the remainder.
         if (!quo_bit) rnq += den << 1;
         uint rem = rnq >> den_shift;
         
         // Extract the quotient and shift in the last quotient bit.
         uint quo = extract(rnq, den_shift);
         rotl1(&quo, quo_bit);
         return (__divrem_u32_t){quo, rem};
       
      'rnq' is the main work register. It holds the current remainder in the
      top bits, followed by the left-over numerator bits, and the computed
      quotient bits. Any remaining bits are filled with zeroes.
      
        Layout of rnq (_r_emainder _n_umerator _q_uotient)
        
        den_shift - number of leading zeroes of denominator operand
        num_shift - number of leading zeroes of numerator operand
        quo_bits  - number of quotient bits minus one
        
        i - number of iterations left
      
        PART      |              SIZE |      POSITION OF LSB
        ----------|-------------------|---------------------
        remainder |    32 - den_shift |            den_shift
        numerator |                 i |        den_shift - i
        zeroes    |         num_shift |                    i
        quotient  |      quo_bits - i |                    0

      In each iteration a numerator bit is consumed while a quotient bit is
      produced, so the size of numerator and quotient at each step depends on
      the loop count LC0. 
      
      'quo_bit' always holds the last quotient bit that was produced. The
      rotate at the end of the loop pushes the previous quotient
      bit into the LSB of 'rnq' while at the same time getting the next
      quotient bit from the MSB.
        
   Implementation:
        
      There are two different implementations of the algorithm that are
      are selected depending on the denominator.
      
      'full' is more or less direct translation of the C code above into
      assembler. It is suitable for any operands and is centered around
      a three-cycle inner loop that uses a parellel add/subtract
      followed by a conditional move and a rotate to implement the
      add/subtract-shift algorithm.
      
      'quick' uses the Blackfin DIVQ instruction to do the same in a single
      cycle. DIVQ cannot be used for all operands due to a couple of
      limitations.
      
      'DIVQ(Rn, Rd)' has the following effects, whereby .number is meant
      to indicate bit access:
        
        Rn.H += AQ ? Rd.L : -Rd.L;
        AQ = Rn.31 ^ Rd.31;
        Rn <<= 1;
        Rn.0 = AQ;

      So DIVQ only uses the bottom half of the denominator register.
      Coupled with the fact that the algorithm requires the top bit of the
      numerator register as a sign bit, this means that DIVQ can only be 
      used for unsigned denominators up to 15 bits long.
      
      Also, unlike the 'full' loop which rotates the 'quo_bit' through CC,
      DIVQ copies the new quotient bit directly into the bottom bit. As a
      consequence, the top quotient bit does not fit into the work register
      when the numerator is 32 bits long, which is why it is kept in CC
      instead.
      
   Register usage:
      
      R0 - numerator, work register
      R1 - denominator
      R2 - num_shift
      R3 - den_shift, temp
      P1 - quo_bits
        
   Clobbered registers:
    
       Refer to syscall information below.
    
   Stack size:
    
      The functions make no use of the stack.
    
   Cycle counts:
        
      The function takes up to 91 cycles, depending on the lengths
      of the operands. On average it takes about 52 cycles.
      (Measured on BF548 revision 0.0. Numbers include call and return).
    
********************************* **** * *  **********************************
#endif

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12u/ 12u% kz"
%notes "Combined unsigned 32-bit integer division and remainder."
%regs_clobbered "R0-R3,P1,LC0,LT0,LB0,CCset"
%const
%syscall ___divrem_u32(xU=UU)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libGroup = integer_support;
.FILE_ATTR libName  = libdsp;
.FILE_ATTR libFunc  = ___divrem_u32;
.FILE_ATTR FuncName = ___divrem_u32;

.FILE_ATTR libFunc  = ___divrem_u64;
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

.TYPE   ___divrem_u32, STT_FUNC;
.GLOBAL ___divrem_u32;

.SECTION/DOUBLEANY program;


.ALIGN 4;
___divrem_u32:
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
      
      // Keep top quotient bit safe.
      CC = !CC;
      R2 = CC;
      
      // Correct and extract remainder.
      CC = AQ;
      R1 = R0 + R1;
      IF !CC R1 = R0;
      R1 >>= R3;
   
      // Extract quotient and add top quotient bit.
      R0 = EXTRACT(R0, R3.L) (Z);
      R3 = P1;
      R2 <<= R3;
      R0 = R0 | R2;
      
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
      IF CC R1 = R0;
      R1 >>= R3;
      
      // Extract quotient and rotate in the last quotient bit.
      R0 = EXTRACT (R0, R3.L) (Z);
      R0 = ROT R0 BY 1;
      
      RTS;
      
      
   /* Single-bit quotient case. */
.one_bit:
      // Compare numerator and denominator to determine one-bit quotient.
      // Adjust remainder accordingly.
      CC = R1 <= R0 (IU);
      R1 = R0 - R1;
      IF !CC R1 = R0;
      R0 = CC;      

      RTS;


.___divrem_u32.end:

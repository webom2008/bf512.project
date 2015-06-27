/****************************************************************************
 *
 * div_s32.asm : $Revision: 3543 $
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 *
 ****************************************************************************/

#if defined(__DOCUMENTATION__)
    
   Function: __div_s32 - signed 32-bit division
    
   Synopsis:
    
      int32_t __div_s32(int32_t num, int32_t den);

      int32_t __div32(int32_t num, int32_t den);
  
   Description:
      
      The __div_s32 compiler support function performs a signed
      division of its operands. As required by the C standard, the quotient is
      rounded towards zero.
      
      __div32 is an an alias for __div_s32 that is provided for backwards
      compatibility.
        
   Arguments:

      R0 - numerator
      R1 - denominator

   Results:

      R0 - quotient  = num / den

   Error Conditions:
    
      The result of the division operation is undefined when the denominator
      is zero, although the function is guaranteed to return and take no
      longer than with any valid operands.
      
      INT_MIN / -1 is also left undefined by the C standard, because the
      correct result is outwith the range of int. However, the function
      returns INT_MIN in order to stay consistent with the overflow behaviour
      of the negation operator.

   Algorithm & Implementation:
      
      This is a cut-down version of divrem_s32.asm, where the code for
      correcting, extracting and signing the remainder was omitted.
      
   Register and flag usage:
      
      R2 - numerator, work register
      R3 - denominator
      R0 - num_shift, quo_bits, quo_sign
      R1 - den_shift, temp
      P1 - quo_bits
      R0.31 - quotient sign
        
   Clobbered registers:
    
      Refer to syscall information below.
    
   Stack size:
    
      The function makes no use of the stack.
    
   Cycle counts:
        
      The function takes between 25 and 91 cycles, depending on the lengths
      of the operands. On average it takes about 52 cycles.
      (Measured on BF548 revision 0.0. Numbers include call and return).
    
********************************* **** * *  **********************************
#endif

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12/"
%notes "Signed 32-bit integer division."
%regs_clobbered "R0-R3,P1,LC0,LT0,LB0,CCset"
%const
%syscall ___div32(I=II)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libGroup = integer_support;
.FILE_ATTR libName  = libdsp;
.FILE_ATTR libFunc  = ___div_s32;
.FILE_ATTR FuncName = ___div_s32;
.FILE_ATTR libFunc  = ___div32;
.FILE_ATTR FuncName = ___div32;

.FILE_ATTR libFunc  = __cdiv_fr16;
.FILE_ATTR libFunc  = __mean_fr16;
.FILE_ATTR libFunc  = __rms_fr16;
.FILE_ATTR libFunc  = __var_fr16;

.FILE_ATTR  prefersMem    = internal;
.FILE_ATTR  prefersMemNum = "30";
#endif

.TYPE   ___div_s32, STT_FUNC;
.GLOBAL ___div_s32;
.TYPE   ___div32, STT_FUNC;
.GLOBAL ___div32;

.SECTION/DOUBLEANY program;


.ALIGN 2;
___div_s32:
___div32:
      // Calculate the magnitudes of the operands.
      R2 = -R0;
      R3 = -R1;
      R2 = MAX(R0, R2);
      R3 = MAX(R1, R3);
      
      // Shortcut for quotient zero.
      CC = R2 < R3 (IU);
      IF CC JUMP .zero;
      
      // Compute quotient sign.
      R0 = R0 ^ R1;
      
      // Count leading zeroes of operands.
      R1 = R3 >> 1;
      R0.L = SIGNBITS R2;  // num_shift
      R1.L = SIGNBITS R1;  // den_shift
      
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
      R0.L = R1.L - R0.L (NS);
      R1 = R0.L (Z);
      P1 = R1;
      
      // If the left-justified denominator has any bits set outside the top
      // fifteen, the full path has to be taken.
      R1 = R3 << 15;
      CC = R1;
      IF CC JUMP .full;
      
      // INT_MIN / 2**n needs to be treated specially, because it falls
      // just outside the valid range for the main loop.
      CC = R2 == R3;
      IF CC JUMP .intmin_pow2;


   /* Quick path. */
.quick:      
      // Clear AQ and position denominator for DIVQ.
      AQ = CC;
      R3 >>= 17;
      
      // Loop executed up to 32 times.
      LOOP(.quick_loop) LC0 = P1;
      LOOP_BEGIN .quick_loop;
         DIVQ(R2, R3);
      LOOP_END .quick_loop;
      
      // Fetch sign (and avoid EXTRACT stall)
      CC = BITTST(R0, 31);  

      // Extract quotient.
      R2 = EXTRACT(R2, R0.L) (Z);
      
      // Apply sign.
      R0 = -R2;
      IF !CC R0 = R2;
      
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

      // Pick up last quotient bit and extract quotient.
      R2 = ROT R2 BY 1;
      CC = BITTST(R0, 31);  // scheduled here to avoid EXTRACT stall
      R2 = EXTRACT(R2, R0.L) (Z);
      
      // Apply signs.
      R0 = -R2;
      IF !CC R0 = R2;
      
      RTS;


   /* Special case for zero quotient. */
.zero:
      R0 = 0;
      RTS;


   /* Special case for MIN_INT divided by power of 2. */
.intmin_pow2:
      R2 = 1;
      #ifdef __WORKAROUND_SHIFT
         R2 <<= R0;
      #else
         R2 = LSHIFT R2 BY R0.L;
      #endif

      CC = BITTST(R0, 31);
      R0 = -R2;
      IF !CC R0 = R2;
      
      RTS;


.___div_s32.end:
.___div32.end:

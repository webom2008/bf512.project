/*****************************************************************************
 *
 * divrem_u96_u64.asm : $Revision: 1.5 $
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)
    
   Function: __divrem_u96_u64
   
   Synopsis:
   
      typedef struct { u64 quo; u64 rem; } __quorem_u64_t;
      
      __quorem_u64_t __divrem_u96_u64(u96 num, u64 den);
   
   Description:
   
      The __divrem_u96_u64 compiler support function performs an unsigned
      division of a 96-bit numerator by a 64-bit denominator and returns both
      quotient and remainder. If the quotient is too big, it is saturated
      to ULLONG_MAX and the remainder is set to zero.
      
      This routine is used by the various fractional division compiler support
      routines (div?_?k40.asm).
   
   Arguments:
      
      A non-standard calling convention is used in that the fourth word is
      expected in R3 instead of [SP + 12] and a fifth in R7 instead of
      [SP + 16].      
      
      R1:0,7: numerator (i.e. MSB in R1 and LSB in R7)
      R3:2: denominator

   Results:

      R1:0 - quotient
      R3:2 - remainder

   Clobbered registers:
    
      Scratch: R3:0, P1:0, ASTAT, LOOP0 regs.
      
   Stack size:
    
      Two of the unused parameter slots plus one additional slot are used.
      The third parameter slot is reserved for the routines calling this one.

   Error Conditions:
    
      If the denominator is zero, the result of the division operation is
      undefined, although the routine is guaranteed to return and take no
      longer than with any valid operands.

   Algorithm & Implementation:
        
      Simple cases are handed off to __divrem_u64. Otherwise the routine
      closely resembles __divrem_u64, except that three registers are needed
      to hold remainder, numerator and quotient in its inner loops.
      There are two separate paths. The .div_96_32 path handles cases where the
      denominator is no longer than 32 bits, whereas .div_96_64 handles
      64-bit denominators. In the 32-bit denominator case the carry bit does
      not need to be considered, whereas in the 64-bit case carry handling
      makes up a large part of the inner loop due to the lack of add/subtract
      with carry instructions.
      
********************************* **** * *  **********************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libName  = libdsp;
.FILE_ATTR libGroup = integer_support;
.FILE_ATTR FuncName = ___divrem_u96_u64;
.FILE_ATTR libFunc  = ___divrem_u96_u64;

.FILE_ATTR libFunc  = ___divt_uk40;
.FILE_ATTR libFunc  = ___divt_sk40;
.FILE_ATTR libFunc  = ___divr_uk40;
.FILE_ATTR libFunc  = ___divr_sk40;

.FILE_ATTR prefersMem = internal;
.FILE_ATTR prefersMemNum = "30";
#endif

.TYPE   ___divrem_u96_u64, STT_FUNC;
.GLOBAL ___divrem_u96_u64;

.EXTERN ___divrem_u64;

.SECTION/DOUBLEANY program;

#include <sys/anomaly_macros_rtl.h>

#if !defined(__ADSPLPBLACKFIN__)
  #define AC0 AC
#endif


___divrem_u96_u64:
      CC = R1 == 0;
      IF CC JUMP .div_64_64;
      
      CC = R3 == 0;
      IF !CC JUMP .div_96_64;
      
   /* Path for denominators up to 32 bits. */
.div_96_32:
      CC = R1 < R2 (IU);
      IF !CC JUMP .overflow;

#ifdef __WORKAROUND_WB_DCACHE
      SSYNC;
#endif
      // Count leading zeroes of operands.
      R3 = R1 >> 1;
      R6 = R2 >> 1  ||  [SP] = R6;
      R3.L = SIGNBITS R3;
      R6.L = SIGNBITS R6;
      
      // Calculate number of loop iterations.
      R3 = R3 - R6;  // num_zeroes - den_zeroes
      R3 = R3.L (Z);
      R6 = 64;
      R6 = R6 - R3;
      P1 = R6;
      
      R6 = R2;
      
      // Line up numerator with denominator.
      R1 <<= R3;
      R3 += -32;
      R2 = LSHIFT R0 BY R3.L;
      R2 = R2 | R1;
      R1 = LSHIFT R7 BY R3.L;
      R3 += 32;
      R0 <<= R3;
      R1 = R1 | R0;
      R0 = LSHIFT R7 BY R3.L;

      // Calculate top quotient bit (leaving remainder in R2).
      R3 = R2 - R6;
      CC = AC0;
      IF CC R2 = R3;
      
      // Loop executed 33 to 64 times.
      LOOP(.div_96_32_loop) LC0 = P1;
      LOOP_BEGIN .div_96_32_loop;
         R0 = ROT R0 BY 1;
         R1 = ROT R1 BY 1;
         R2 = ROT R2 BY 1;
         R3 = R2 - R6;
         CC |= AC0;
         IF CC R2 = R3;
      LOOP_END .div_96_32_loop;
      
      // Fetch last quotient bit. 
      R0 = ROT R0 BY 1  ||  R6 = [SP];
      R1 = ROT R1 BY 1;
      
      // Top half of remainder is zero.
      R3 = 0;
      
      RTS;


   /* Path for denominators longer than 32 bits. */
#ifdef __WORKAROUND_WB_DCACHE
.div_96_64:
     SSYNC;
#else
.ALIGN 8;
.div_96_64:
#endif
      // Count leading zeroes of operands. (And spill registers.)
      R4 = R3 >> 1        ||  [SP--] = R4;
      R5 = R1 >> 1        ||  [SP] = R5;
      R4.L = SIGNBITS R4  ||  [SP + 8] = R6;
      R6 = 0x201;  // EXTRACT pattern for AC0 in ASTAT
      R5.L = EXPADJ(R5, R4.L);

      // Calculate number of quotient bits.
      R5 = R4 - R5;
      R5 = R5.L (Z);
      P1 = R5;
      
      // Align the denominator with the numerator (leaving it in R5:4).
      R3 <<= R5;
      R4 = LSHIFT R2 BY R5.L;
      R5 += -32;
      R5 = LSHIFT R2 BY R5.L;
      R5 = R3 | R5;
      
      // Prepare for subtract-with-carry in inner loop.
      R5 = ~R5;
            
      // Loop executed 33 to 64 times.
      P1 += 33;
      LOOP(.div_96_64) LC0 = P1;
      LOOP_BEGIN .div_96_64;
         // 64-bit subtract: remainder - denominator
         // EXTRACT is used to get the carry from the lower word, so that CC,
         // which contains the carry from the ROT operations is not clobbered.
         R2 = R0 - R4;
         R3 = ASTAT;
#if WA_05000209
         NOP;
#endif
         R3 = EXTRACT(R3, R6.L) (Z);
         R3 = R3 + R5;
         R3 = R3 + R1;
         
         // Keep the difference if remainder was greater than the denominator.
         CC |= AC0;
         IF !CC R2 = R0;
         IF !CC R3 = R1;
         
         // Store the quotient bit and shift up the remainder.
         R7 = ROT R7 BY 1;
         R0 = ROT R2 BY 1;
         R1 = ROT R3 BY 1;
      LOOP_END .div_96_64;      
      
      // Extract quotient
      CC = BITTST(R0, 0);
      R0 = R7;
      P1 += -33;
      R7 = P1;
      R1 = EXTRACT(R2, R7.L) (Z)  ||  R6 = [SP + 8];
      R1 = ROT R1 BY 1            ||  R5 = [SP++];
      
      // Extract remainder (doing a 64-bit shift.)
      R4 = -R7;
      R4 += 32;
      R4 = LSHIFT R3 BY R4.L;
      R2 >>= R7;
      R3 >>= R7;
      R2 = R2 + R4 (NS)           ||  R4 = [SP];
      
      RTS;


   /* 32/64 divisions are passed on to __divrem_u64. */
.ALIGN 4;
.div_64_64:

      R1 = R0;
      R0 = R7;
      
      JUMP.X ___divrem_u64;


   /* Overflow: quotient ULLONG_MAX, remainder zero */
.overflow:

      R0 = -1;
      R1 = -1;
      R2 = 0;
      R3 = 0;
      
      RTS;


.___divrem_u96_u64.end:

/****************************************************************************
 *
 * divr_k40_round.asm : $Revision: 1.5 $
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 *
 ****************************************************************************/

#if defined(__DOCUMENTATION__)
    
   Function: __divr_k40_round - rounding for 40-bit accum division
    
   Synopsis:
    
      s64 __divr_k40_round(s64 quo, u40 rem, u40 den)
  
   Description:
      
      __divr_k40_round is a helper routine intended for use by the
      __divr_uk40 and __divr_sk40 accum division routines. It has a very
      non-standard call protocol and should not be called from elsewhere.
      The quotient is rounded up by one if the remainder is greater than half
      the denominator. If the remainder is exactly half the denominator, 
      the behaviour depends on RND_MOD: with biased rounding (RND_MOD==1), the
      quotient is always rounded up, whereas with unbiased rounding
      (RND_MOD==0) it is only rounded up if the LSB of the quotient is zero.
      
   Arguments:
   
      R1:0 - quotient
      R3:2 - remainder
      R5:4 - denominator

   Results:

      R1:0 - rounded quotient

   Register and stack usage:
    
      Two slots are dropped from the stack and R4, R5 and R7 are restored
      from the stack, where the calling functions are expected to have saved
      them.
      
********************************* **** * *  **********************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libName  = libdsp;
.FILE_ATTR libGroup = integer_support;
.FILE_ATTR libFunc  = ___divr_k40_round;
.FILE_ATTR FuncName = ___divr_k40_round;

.FILE_ATTR libFunc  = ___divr_uk40;
.FILE_ATTR libFunc  = ___divr_sk40;

.FILE_ATTR prefersMem    = internal;
.FILE_ATTR prefersMemNum = "30";
#endif

.TYPE   ___divr_k40_round, STT_FUNC;
.GLOBAL ___divr_k40_round;

.SECTION/DOUBLEANY program;

#if !defined(__ADSPLPBLACKFIN__)
  #define AC0 AC
#endif


.ALIGN 2;
___divr_k40_round:
      
      // CC = RND_MOD | quotient.0
      R7 = ASTAT;
      R7 >>= 8;
      R7 = R7 | R0;
      CC = BITTST(R7,0);
      
      // remainder2 = remainder * 2 + CC
      // (no overflow since remainder < denominator < 0x100.00000000)
      R2 = ROT R2 BY 1;
      R3 = ROT R3 BY 1;

      SP += 8;

      // CC = denominator < remainder2
      CC = R4 < R2 (IU);
      R5 = R5 - R3 (S)   ||  R7 = [SP + 8];
      CC &= AZ;
      CC |= AN;
    
      // quotient += CC;
      R2 = CC;
      R0 = R0 + R2 (NS)  ||  R4 = [SP];
      CC = AC0;
      R3 = CC;
      R1 = R1 + R3 (NS)  ||  R5 = [SP + 4];
      
      RTS;
      
.___divr_k40_round.end:

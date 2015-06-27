/*****************************************************************************
 *
 * lldiv.asm : $Revision: 131 $
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)
    
   Function: __divrem_s64 - signed 64-bit division and remainder
   
   Synopsis:
   
      lldiv_t lldiv(long long num, long long den);

   Description:
      
      This routines implements the C standard library function lldiv.
      
   Arguments:

      R1:0 - numerator
      [SP+12],R2 - denominator
      
   Results:

      [P0 + 4],[P0]      - quotient  = numerator / denominator
      [P0 + 12],[P0 + 8] - remainder = numerator % denominator
        
   Algorithm & Implementation:
        
      This is a wrapper for __divrem_s64, which implements the standard
      calling convention.
      
   Register Usage:
   
      Scratch:   R3:0, P2:0, ASTAT, LOOP0 regs.
      Preserved: R7:4, SP
    
   Stack Usage:
    
      This function uses two of the stack slots reserved for the first three
      parameters are used for spilling registers.
      
   Dependencies:
   
      divrem_s64.asm
    
********************************* **** * *  **********************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libGroup = integer_support;
.FILE_ATTR libName  = libdsp;
.FILE_ATTR libFunc  = _lldiv;
.FILE_ATTR FuncName = _lldiv;
.FILE_ATTR prefersMem = internal;
.FILE_ATTR prefersMemNum = "30";
#endif

.TYPE   _lldiv, STT_FUNC;
.GLOBAL _lldiv;

.EXTERN ___divrem_s64;

.SECTION/DOUBLEANY program;

.ALIGN 2;
_lldiv:
#ifdef __WORKAROUND_WB_DCACHE
      SSYNC;
#endif
      R3 = RETS;
      [SP + 4] = R3;    // Save return address.
      [SP + 8] = P0;    // Save result pointer.
      R3 = [SP + 12];   // Fetch upper part of denominator.
      SP += -8;
      CALL ___divrem_s64;
      SP += 8;
      P1 = [SP + 4];    // Fetch return address.
      P0 = [SP + 8];    // Fetch result pointer.
      [P0] = R0;        // Store result.
      [P0 + 4] = R1;
      [P0 + 8] = R2;
      [P0 + 12] = R3;
      JUMP (P1);
._lldiv.end:

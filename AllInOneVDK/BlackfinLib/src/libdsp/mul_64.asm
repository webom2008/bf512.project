/*****************************************************************************
 *
 * mul_64.asm : $Revision: 3543 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)
    
   Function: __mul_64 - 64-bit multiplication
   
   Synopsis:
   
      int64 __mul_64(int64 a, int64 b);
      int64 __mullu3(int64 a, int64 b);
      int64 __mulli3(int64 a, int64 b);
  
   Description:
      
      The __mul_64 compiler support function performs a 64-bit multiplication
      of its operands and returns the lower 64 bits of the full 128-bit result.
      This implements both unsigned and signed 'long long' multiplication,
      because discarding the upper 64 bits means that signedness
      of the operands makes no difference to the result.
      
      The _mullu3 and _mulli3 entry points are provided for backwards
      compatibility. Unlike _mul_64 they expect the upper half of their second
      operand in [SP+12] instead of R3.
      
   Arguments:

      R1:0 - a
      R3:2 - b
      
   Result:

      R1:0 - a * b
        
   Error Conditions:
      
      None.
      
   Algorithm & Implementation:
      
      A 64-bit multiply can be decomposed into 32*32->64 multiplies:
      
        R1:0 * R3:2
      = ((R1*R3) << 64) + ((R1*R2 + R3*R0) << 32) + (R0*R2)
      
      R1*R3 and the upper halves of R1*R2 and R3*R0 can be discarded as they
      do not contribute to the lower 64 bits of the result. R1*R2 and R3*R0
      can therefore be implemented using 32-bit multiply instructions.
      
      The full 64-bit result of R0*R2 is required though, which is implemented
      using four 16*16->32 MAC operations.
      
        R0 * R2
      = ((R0.H*R2.H) << 32) + ((R0.H*R2.L + R0.L*R2.H) << 16) + (R0.L*R2.L)
      
   Register Usage:
   
      Scratch:   Refer to syscall information below.
      Preserved: R5:4
    
   Stack Usage:
    
      The stack slots reserved for the first two arguments are used for
      spilling registers.
   
   Cycle Counts:
        
      The routine takes 16 cycles excluding call and return.
      (Measured on BF548).
  
********************************* **** * *  **********************************
#endif

#if defined(__SYSCALL_INFO__)
/* System call information */
%rtl "12*"
%notes "64-bit integer multiplication."
%regs_clobbered "R0-R3,P1,A1,CCset"
%const
%syscall ___mulli3(lL=lLlL)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libName  = libdsp;
.FILE_ATTR libGroup = integer_support;
.FILE_ATTR libFunc  = ___mul_64;
.FILE_ATTR FuncName = ___mul_64;
.FILE_ATTR libFunc  = ___mullu3;
.FILE_ATTR FuncName = ___mullu3;
.FILE_ATTR libFunc  = ___mulli3;
.FILE_ATTR FuncName = ___mulli3;

.FILE_ATTR prefersMem    = internal;
.FILE_ATTR prefersMemNum = "30";
#endif

.TYPE   ___mul_64, STT_FUNC;
.GLOBAL ___mul_64;
.TYPE   ___mullu3, STT_FUNC;
.GLOBAL ___mullu3;
.TYPE   ___mulli3, STT_FUNC;
.GLOBAL ___mulli3;

.SECTION/DOUBLEANY program;


___mullu3:
___mulli3:

      R3 = [SP+12];

___mul_64:

#ifdef __WORKAROUND_WB_DCACHE
      SSYNC;
#endif


      R1 *= R2;
      R3 *= R0;

      R1 = R1 + R3; 

      R3 = (A1 = R0.L * R2.L) (FU);
      A1 = A1 >> 16;
      A1 += R0.L * R2.H (FU)  ||  [SP] = R4;
      A1 += R0.H * R2.L (FU)  ||  [SP+4] = R5;
      R4 = A1.W;
      A1 = A1 >> 16;
      R5 = (A1 += R0.H * R2.H) (FU);
      R0 = PACK(R4.L, R3.L)   ||  R4 = [SP];
      R1 = R1 + R5 (NS)       ||  R5 = [SP+4];
      RTS;

.___mul_64.end:
.___mulli3.end:
.___mullu3.end:

/************************************************************************
 *
 * fpsub.asm : $Revision: 1.11 $
 *
 * (c) Copyright 2000-2005 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if 0
   This function performs 32 bit floating point subtraction.
   It calls  floating point addition function.
     
  Registers used:
     Operands in  R0 & R1 
     R0 - X operand, R1 - Y operand,R2   
                  
  Special case: 
     IF Y == 0,RETURN X

  !!NOTE- Uses non-standard clobber set in compiler:
          DefaultClobMinusABIMandLoopRegs

  Also note that if this clobber set changes, you may need to change the
  pragma regs_clobbered in softfloat.

#endif

.file_attr libGroup      = floating_point_support;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = ___float32_sub;
.file_attr FuncName      = ___float32_sub;

.section  program;

.global ___float32_sub;
.type ___float32_sub, STT_FUNC;

.extern ___float32_add;
.type ___float32_add, STT_FUNC;

.align 2;
___float32_sub:
   BITTGL(R1,31);          // Flip sign bit of Y 
   JUMP.X ___float32_add;  // Call addition routine
.___float32_sub.end:

// end of file

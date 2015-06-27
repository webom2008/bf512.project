/******************************************************************************
  Copyright(c) 2000-2004 Analog Devices Inc. 
  All rights reserved
 ******************************************************************************
  File Name      : divfract32.asm
  Module Name    : Library support routine
  Label Name     : __divfract32

  Description    : This function performs signed division 
                   of two 32 bit integer numbers. 
                   The result is in fractional 1.31 format.

                   The Numerator must be less than Denominator, 
                   otherwise the result will overflow.  

  Operands       : R0 - Numerator,
                   R1 - Denominator

  Registers Used : R0-5, P0

  Cycle count    : 248 Cycles (BF532, Cycle Accurate Simulator)
  Code Size      : 50 Bytes
******************************************************************************/

/* Called by autocoh_fr16 */
.file_attr libGroup      = stats.h;
.file_attr libFunc       = __autocoh_fr16;
.file_attr libFunc       = autocoh_fr16;
/* Called by autocorr_fr16 */
.file_attr libFunc       = __autocorr_fr16;
.file_attr libFunc       = autocorr_fr16;
/* Called by crosscoh_fr16 */
.file_attr libFunc       = __crosscoh_fr16;
.file_attr libFunc       = crosscoh_fr16;
/* Called by crosscorr_fr16 */
.file_attr libFunc       = __crosscorr_fr16;
.file_attr libFunc       = crosscorr_fr16;

.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr libFunc = __divfract32;
.file_attr FuncName      = __divfract32;

.section  program;
.global   __divfract32;

.align 2;
__divfract32:
                   [--SP] = R5;            // PUSH REGISTERS ONTO STACK
                   [--SP] = R4;
                   P0 = 0x20;              // SET LOOP COUNTER TO 32 
                                           // TO GET 32 BIT QUOTIENT
                   R3 = R0 ^ R1;           // SET SIGNBIT OF R3 IF SIGNS 
                                           // NUMERATOR AND DENOMINATOR DIFFER 
                   R2 = ABS R0;            // ABS VALUE OF NUMERATOR
                   R1 = ABS R1;            // ABS VALUE OF DENOMINATOR

                   R5 = 0;
                   R0 = 0;                 // CLEAR TO STORE RESULT
                   LSETUP( SUB_START, SUB_END ) LC0 = P0;
SUB_START:           R0 <<= 1;             // SET LEAST SIGNIFICANT BIT TO 0
                     R4 = R2 - R1;         // NUMERATOR - DENOMINATOR
                     CC = R5 <= R4;        
                     IF CC R2 = R4;        // STORE THE RESULT AS NUMERATOR
                                           // IF RESULT POSITIVE
                     R4 = CC;
                     R0 = R0 + R4;         // CHANGE LEAST SIGNIFICANT BIT TO 1
                                           // IF RESULT POSITIVE
SUB_END:             R2 <<= 1;             // NUMERATOR SHIFTED FOR CALCULATING
                                           // NEXT QUOTIENT BIT 

                   R2 = -R0;
                   CC = R3 < 0;            // CHECK SIGN OF RESULT
                   IF CC R0 = R2;          // NEGATE RESULT IF SIGNBIT R3 SET
                   R4 = [SP++];            // POP REGISTERS FROM STACK
                   R5 = [SP++];             
                   RTS;

.__divfract32.end:












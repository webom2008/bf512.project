/******************************************************************************
  Copyright(c) 2000-2009 Analog Devices Inc.  All rights reserved
*******************************************************************************
  File Name      : zerocross.asm
  Include File   : stats.h
  Label name     : __zero_cross_fr16

  Description    : 

                   int zero_cross_fr16 (const fract16 _samples[], 
                                        int _sample_length);

                   Counting the number of times the signal
                   contained in x[] crosses the zero line.

                   If the number of samples is less than two, zero is returned

  Operand        : R0 - Address of input array a[],
                   R1 - Number of samples

  Registers Used : R0-4, P1-3

  Cycle count    : n = 24, first value in array zero: 202 Cycles
                   (BF532, Cycle Accurate Simulator) 

                   26 + 15*nz + 7*nr 

                   where:
                     nz = Number of leading zeros in array
                     nr = Number of samples - nz
   
  Code size      : 76 Bytes 
******************************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libGroup      = stats.h;
.file_attr libFunc       = zero_cross_fr16;
.file_attr libFunc       = __zero_cross_fr16;
.file_attr libName       = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = __zero_cross_fr16;
#endif

#include <sys/anomaly_macros_rtl.h>

.section  program;
.global   __zero_cross_fr16;

.align 2;
__zero_cross_fr16:     

   P1 = R0;                // Pointer to input array
   R0 = 0;                 // Reset result to 0 
   R1 += -1;
   P2 = R1;                // Number of samples to loop over

   CC = R1 < 1;            // Terminate if number of samples < 2
   IF CC JUMP .ret_zero;

#if defined(__WORKAROUND_SPECULATIVE_LOADS) || \
    WA_05000428
   NOP;
   NOP;
   NOP;
#endif

   R3 = W[P1++](Z);        // Read input

   LSETUP (.l1start,.l1end) LC0=P2;
.l1start:  CC = R3 == 0;           // Loop until first non-zero value  
           IF !CC JUMP .skipped_zeros;

#if defined(__WORKAROUND_SPECULATIVE_LOADS) || \
    WA_05000428
           NOP;
           NOP;
           NOP;
#endif

.l1end:    R3 = W[P1++](Z);          // Get next sample

   RTS;                              // All inputs zero, return 0

.skipped_zeros:                      // R3 is now non-zero
   R1 = R3 >> 15 || R3 = W[P1++](Z); // Copy sign bit to R1
                                     // and Read next input 

   LSETUP (.l2start,.l2end) LC0;
.l2start: R2 = R3 >> 15;             // Copy sign bit to R2
          CC = R3 == 0;              // Test next value for zero
          IF CC R2 = R1;             // Force signs to be equal if new value=0
          R3 = W[P1++](Z);           // Read next input

          R1 = R1 ^ R2;              // R3 = 1 if signs differ 
          R0 = R0 + R1;              // Increment counter if signs differ

.l2end:   R1 = R2;                   // Copy new sign to old sign

.ret_zero: 
   RTS;

.__zero_cross_fr16.end:

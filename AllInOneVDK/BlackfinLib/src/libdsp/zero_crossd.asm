/******************************************************************************
  Copyright(c) 2004-2008 Analog Devices Inc.  All rights reserved
*******************************************************************************
  File Name      : zero_crossd.asm
  Include File   : stats.h
  Label name     : __zero_crossd

  Description    : 

                   int zero_crossd (const long double _samples[], 
                                    int _sample_length);

                   Counting the number of times the signal
                   contained in x[] crosses the zero line.

                   If the number of samples is less than two, zero is returned
                   If all input values are +/- zero, zero is returned

                   The function treats +/- Inf and +/-NaN like any other
                   number. Thus if x[i] = +Inf and x[i+1] = negative value, 
                   then a zero_crossing is counted.

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
.file_attr libFunc       = zero_crossd;
.file_attr libFunc       = __zero_crossd;
.file_attr libFunc       = zero_cross;   // from stats.h
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = __zero_crossd;
#endif

#include <sys/anomaly_macros_rtl.h>

.section  program;
.global   __zero_crossd;

.align 2;
__zero_crossd:     
   P1 = R0;                // Pointer to input array
   R0 = 0;                 // Reset result to 0 
   R1 += -1;

   CC = R1 < 2;            // Terminate if number of samples < 1
   IF CC JUMP RET_ZERO;

   P2 = R1;                // Number of samples to loop over
   P0 = 8 (Z);             // Load constant for post-modify
   P1 += 4;                // Move input ptr to MSB

   R3 = [P1++P0];          // Read input (MSB)
   LSETUP (l1start,l1end) LC0=P2;
l1start:  R2 = R3 << 1;           // Remove sign
          CC = R2 == 0;           // Loop until first non-zero value  
          IF !CC JUMP skipped_zeros;

#if WA_05000428
          // Speculative read from L2 by Core B may cause a write to L2 to fail

          NOP;
          NOP;
          NOP;
#endif

l1end:    R3 = [P1++P0];          // Read input (MSB)
   RTS;                    // all inputs zero, return 0

skipped_zeros:
   [--SP] = R4;            // Push R4 onto stack
                           // R3 is now non-zero
   R1 = R3 >> 31 || R4 = [P1++P0];          
                           // Remove mantissa and exponent
                           // and Read next input (MSB)

   LSETUP (ST_LOOP,END_LOOP) LC0;
ST_LOOP:  R3 = R4 << 1;         // Remove sign
          R2 = R4 >> 31 || R4 = [P1++P0];
                                // Remove mantissa and exponent
                                // and read next input (MSB)
          CC = R3 == 0;         // For zero-value data
          IF CC R2 = R1;        // force signs to be equal

          R3 = R1 ^ R2;         // R3 = 1 if signs differ 
          R0 = R0 + R3;         // Increment counter if signs differ

END_LOOP: R1 = R2;              // Copy new sign to old sign
          
   R4 = [SP++];            // Pop R4 from stack
RET_ZERO: 
   RTS;

.__zero_crossd.end:

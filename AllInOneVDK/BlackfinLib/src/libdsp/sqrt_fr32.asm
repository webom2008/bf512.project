/****************************************************************************
 *
 * sqrt_fr32.asm : $Revision: 1.2 $
 *
 * (c) Copyright 2010 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: sqrt_fr32 - Square Root

    Description:
    Synopsis:

        #include <math.h>
        fract32 sqrt_fr32 (fract32 x);

    Description:

        The square root function returns the positive square root of the
        argument x.

    Error Conditions:

        The square root function returns a zero if the input argument is
        negative.

    Algorithm:

        The function uses a Newton Approximation with three iterations.
        The algorithm is implemented using 64-bit unsigned fractional
        arithmetic. 

        For an even exponent:

          sqrt( 2^exponent * mantissa ) 

                  = ( 2^exponent * mantissa ) ^ (1/2)
                  = (( 2^exponent ) ^ (1/2)) * (mantissa ^ (1/2))
                  = 2^(exponent/2) * sqrt(mantissa)

                  => sqrt(mantissa) >> (exponent/2) 

        For an odd exponent:

          sqrt( 2^exponent * mantissa )          

                  = ( 2^exponent * mantissa ) ^ (1/2)
                  = ( 2^exponent * 2 * 1/2 * mantissa ) ^ (1/2)
                  = (( 2 * 2^exponent ) ^ (1/2)) * ((1/2 * mantissa) ^ (1/2))
                  = ( 2^(exponent+1) ^ (1/2)) * ((1/2 * mantissa) ^ (1/2))
                  = ( 2^((exponent+1)/2) * sqrt(mantissa/2)

                  => sqrt(mantissa/2) >> ((exponent+1) / 2)


    Implementation:

        The support function ___SQRT_Mult64 and the seed table 
        ___SQRT_Seed_Table are shared with the sqrtd implementation.

        There are three path through the function, depending on the scaling
        required. For input values >= 0.5, normalising the input value can
        be greatly simplified since the number of signbits will always be
        zero. Thus all that is required is a down shift by 1. Furthermore,
        the exponent will always be odd, thus the final shift magnitude can
        be fixed to a constant value also (an up shift by 2). Since the 
        algorithm to perform the approximation uses 64-bits, no significant 
        bits are lost when performing the down shift - the least significant
        bit simply spills into the lower 32-bits of the 64-bit number. After
        computing the approximation, a 64-bit up shift is performed. Rounding
        is subsequently applied to ensure best accuracy.

        For input values < 0.03125, normalising will require an up shift.
        Care has to be taken that even / odd exponents are handled correctly.
        After the approximation, rounding is applied to the upper 32-bits of
        the result before performing the final down shift. 

        For input values between 0.03125 and 0.5, the initial scaling requires
        the same operations as for input values < 0.03125. After the 
        approximation however, an up shift will be required and thus execution
        will follow the case for input values >= 0.5. 
         
    Example:

        #include <math.h>

        fract32  y;
        y = sqrt_fr32 (0x20000000); /* y = sqrt(1/4) = 1/2 */

    Cycle Counts:

        for sqrt_fr32(x), where x >= 0.5:

           498 cycles

        for sqrt_fr32(x), where 0.5 > x >= 0.03125:

           503 cycles

        for sqrt_fr32(x), where 0.03125 > x:

           506 cycles

        (Measured for an ADSP-BF532 using version 5.0.0.49 of the ADSP-BF5xx
        Family Simulator and includes the overheads involved in calling
        the library procedure as well as the costs associated with argument
        passing).

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.file_attr libGroup = math_bf.h;
.file_attr libGroup = math.h;
.file_attr libName  = libdsp;
.file_attr FuncName = __sqrt_fr32;

      /* Called from acosf */
.file_attr libFunc  = acosf;
.file_attr libFunc  = __acosf;
.file_attr libFunc  = acos;

      /* Called from acos_fr32 */
.file_attr libFunc  = acos_fr32;
.file_attr libFunc  = __acos_fr32;

      /* Called from asinf */
.file_attr libFunc  = asinf;
.file_attr libFunc  = __asinf;
.file_attr libFunc  = asin;

      /* Called from asin_fr32 */
.file_attr libFunc  = asin_fr32;
.file_attr libFunc  = __asin_fr32;

      /* Called from cabs_fr32 */
.file_attr libFunc  = cabs_fr32;
.file_attr libFunc  = __cabs_fr32;

      /* cabs_fr32 is called by cartesian_fr32 */
.file_attr libFunc  = cartesian_fr32;
.file_attr libFunc  = __cartesian_fr32;

      /* Called from rms_fr32 */
.file_attr libFunc  = rms_fr32;
.file_attr libFunc  = __rms_fr32;

      /* Called from sqrt_fr32 */
.file_attr libFunc  = sqrt_fr32;
.file_attr libFunc  = __sqrt_fr32;

.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";

#endif

#include <sys/anomaly_macros_rtl.h>

#if !defined(__ADSPLPBLACKFIN__)
/* Suppress Assembler Warning ea5504 "Memory store instruction use may trigger
** hardware anomaly 05-00-0165". The run-time libraries are not required to
** workaround this anomaly.
*/
.MESSAGE/SUPPRESS 5504;
#endif

/* Data placement on the stack
**
**      Top of stack  -------------
**                         xn
**                    -------------
**                      TempInput
**                    -------------
**                        Temp
**                    -------------
**                       Exponent
**                    -------------
*/

#define  _OFFSET_XN         0            // .byte4 _xn[2];
#define  _OFFSET_TEMPIN     8            // .byte4 _TempInput[2];
#define  _OFFSET_TEMP      16            // .byte4 _Temp[3];
#define  _OFFSET_EXPONENT  28            // .byte4 _Exponent;

.SECTION/DOUBLEANY constdata;

/* Const data */

.align 2;
.byte2 _Zero = 0;

.align 4;
.byte4 _OnePtFive = 0x30000000;

.SECTION/DOUBLEANY program;

.GLOBAL  __sqrt_fr32;
.TYPE    __sqrt_fr32, STT_FUNC;
.EXTERN  ___SQRT_Mult64;
.EXTERN  ___SQRT_Seed_Table;

.ALIGN 2;
__sqrt_fr32:

   /* Check Input, preserve reserved registers */

      CC = R0 <= 0;
      IF CC jump .ErrSqrt32;         // return 0 if input <= 0

      R2 = 0x0 (Z);                  // get exponent 
      R2.L = SIGNBITS R0;       
      CC = R2 == 0;

      [--SP] = (R7:4,P5:4);          // save registers R4-R7 and P4, P5
      [--SP] = RETS;

      IF CC JUMP .large_input;       // deal with inputs >= 0.5 separately

  
   /* Handle inputs < 0.5 */

.default_input:
      R1 = 0x1 (Z);
      R3 = R2 +|+ r1, R1 = R2 -|- R1; // R3 = exponent + 1, R1 = exponent - 1
      CC = BITTST(R1 ,0);            // exponent odd ?
      IF !CC R1 = R2;                // if exponent even, copy exponent
                                     // if exponent odd, decrement exponent

   /* Compute final shift correction taking into account 
   **  - any scaling applied before the Newton Approximation
   **  - adjust exponent depending on whether it is even or odd
   **  - conversion from 1.63U back to fract32 (signed 1.31)
   */
      R7 = CC;                       // R7 set to 1 if exponent odd, 0 if even 
      R4 = R3 +|+ R7, R5 = R3 -|- R7 (ASR); 
                                     // SV = (exponent + 1 + {1|0}) / 2
      R7 += 2;
      R6 = R7 - R4;                  // set shift value SV to
                                     // -SV +  2 + {1|0}

   /* Convert from fract32 to 1.63U and normalize input value */
      R3 = LSHIFT R0 BY R1.L;        // upper 32-bits
      R2 = R2 ^ R2;                  // lower 32-bits 


   /* Compute Newton Approximation (1.63U) */
      CALL.X .___sqrt_fr64;  


   /* For inputs >= 0.03125 follow algorithm for large input values */
      CC = R2 < 0;
      IF !CC JUMP .round_scale_up (bp);

.round_scale_down:

   /* Apply rounding to the upper 32-bits before applying a down shift
   ** (the lower 32-bits will be discarded)
   */                                            
      R7 = 0x1 (Z);
      R6 = -R2;
      R6 = R6 - R7;                  // R6 = -shift value - 1
      R6 = LSHIFT R7 BY R6.L;        // create rounding constant
      R1 = R1 + R6;                  // apply rounding constant
      R0 = LSHIFT R1 BY R2.L;        // apply scaling


   /* Exit */

.EndSqrt32_default:

      RETS = [SP++];
      (R7:4,P5:4) = [SP++];          // restore all registers that were saved

      RTS;


   /* Handle large input values (x >= 0.5) */

.large_input:

   /* Convert from fract32 to 1.63U 
   ** Exponent will always be 0 (and thus odd)
   */

      R3 = R0 >> 1;                  // upper 32-bits
      R2 = R0 << 31;                 // lower 32-bits 

      R6 = 2 (Z);                    // set shift value to 2


   /* Compute Newton Approximation (1.63U) */
      CALL.X .___sqrt_fr64;  


.round_scale_up:

   /* Apply rounding to the upper 32-bits after a 64-bit left shift */  

      R6 = LSHIFT R0 BY R2.L;        // move rounding bit to bit pos. 31

      R4 = LSHIFT R1 BY R2.L;        // shift correct upper 32-bits
      R2 += -32;
      R5 = LSHIFT R0 BY R2.L;        // extract significant bits in 
                                     // the lower 32-bits

      R6 >>= 31;                     // move rounding bit to bit pos. 0

      R4 = R4 + R6 (S);              // apply rounding to the upper 32-bits
                                     // => more efficient to round after data 
                                     //    aligned in 32-bit return register 

      R0 = R4 + R5 (S);              // compute final return value


   /* Exit */

.EndSqrt32_large:

      RETS = [SP++];
      (R7:4,P5:4) = [SP++];          // restore all registers that were saved

      RTS;


   /*
   ** Computing the square root (1.63U)
   **   R2 = input value, lower 32-bits
   **   R3 = input value, upper 32-bits
   **   R6 = final scaling required 
   */

.align 2;
.___sqrt_fr64:

   /* Get seed */

      P2.L = ___SQRT_Seed_Table-0x20; // pointer to const data
      P2.H = ___SQRT_Seed_Table-0x20;

      R0 = R3 >> 24;
      P1 = R0;                       // offset sqrt lookup table

      [--SP] = RETS;

      SP += -32;                     // allocate space on stack for local data

      [SP+_OFFSET_TEMPIN  ] = R2;    // [r3:r2] = y = normalized input
      [SP+_OFFSET_TEMPIN+4] = R3;    // write contents r3:r2 to _TempInput
      [SP+_OFFSET_EXPONENT] = R6;    // write final scale value to _EXPONENT

      I0.L = _OnePtFive;             // pointer to const data
      I0.H = _OnePtFive;
 
      P0.L = _Zero;                  // pointer to const data
      P0.H = _Zero;                  // (required by function ___SQRT_Mult64)

      P2 = P2 + P1;                  // pointer to seed
      R0 = R0 - R0 (NS) || R1 = B[P2] (Z);

      P2 = 3;                        // set number of Newton iterations 

      R1 <<= 24;

      P5 = SP;

      [SP+_OFFSET_XN  ] = R0;
      [SP+_OFFSET_XN+4] = R1;        // write contents r1:r0 to _xn

      P5 += _OFFSET_TEMP;            // set pointer local data to &_Temp
                                     // (required by function ___SQRT_Mult64)


   /* Iterate Newton Approximation */

      LSETUP(.Sqrt64Start, .Sqrt64End) LC0 = P2;

.Sqrt64Start:
         R2 = R0;
         R3 = R1;

         CALL.X ___SQRT_Mult64;      // [r1:r0] = x0^2


         R2 = [SP+_OFFSET_TEMPIN  ];
         R3 = [SP+_OFFSET_TEMPIN+4]; // [r3:r2] = y = input

         CALL.X ___SQRT_Mult64;      // [r1:r0] = y * x0^2/2


         R2 = R2 - R2 (NS)           // [r3:r2] = 1.5
         || R3 = [I0];

         CC = R2 < R0 (IU);
         R7 = CC;                    // R7 = Carry

         R0 = R2 - R0 (NS);

         R5 = R3 - R7 (NS)           // [r3:r2] = x(n)
         || R2 = [SP+_OFFSET_XN];   

         R1 = R5 - R1 (NS)           // [r1:r0] = (3.0-y*x(n)^2)/2
         || R3 = [SP+_OFFSET_XN+4]; 

         CALL.X ___SQRT_Mult64;      // [r5:r4] = x(n+1) = x(n)*(3-y*x(n)^2)/2


         R4 = R0 >> 29;
         R0 = R0 << 3;
         R1 = R1 << 3;
         R1 = R1 + R4 (NS) || [SP+_OFFSET_XN] = R0;

.Sqrt64End:
         [SP+_OFFSET_XN+4] = R1;     // write contents r1:r0 to _xn


      R2 = [SP+_OFFSET_TEMPIN  ];
      R3 = [SP+_OFFSET_TEMPIN+4];    // [r3:r2] = y = input

      CALL.X ___SQRT_Mult64;         // [r1:r0] = y/sqrt(y) = sqrt(y)


      SP += _OFFSET_EXPONENT;        // pop all local data, with the exception
                                     // of _Exponent, from the stack

      R2 = [SP++];                   // get exponent and
                                     // pop remaining local data of the stack

      RETS = [SP++];

      RTS;


   /* Error Return */

.ErrSqrt32:

      R0 = 0;                        // set return value to zero
      RTS;


.__sqrt_fr32.end:

/************************************************************************
 *
 * vecdot_fr32.asm : $Revision: 1.5 $
 *
 * (c) Copyright 2007-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: vecdot_fr32 - vector dot product

    Synopsis:

        #include <vector.h>
        fract32 vecdot_fr32 (const fract32 vector_x[],
                             const fract32 vector_y[],
                             int           length);

    Description:

        The vecdot_fr32 function computes the dot product of the two
        vectors vector_x[] and vector_y[] which are 'length' in size
        and returns the scalar result.

    Error Conditions:

        The function is an effective no-op if the parameter length is
        less than or equal to zero.

    Algorithm:

        result =  x[0]*y[0] + x[1]*y[1] + x[2]*y[2]...x[length-1]*y[length-1]

    Implementation:

        The core of algorithm can be written as:

            accumulator += (X * Y);

        Suppose that X == [XHi,XLo] and Y == [YHi,YLo], then the multiply
        operation can be evaluated as:

            (XHi * YHi) + (XHi * YLo) + (XLo * YHi) + (XLo * YLo)

        These individual terms can be implemented succinctly using the
        Blackfin's 16-bit Multiply-And-Accumulate instructions as outlined
        as follows:

            A1 = A0 = 0 || X = [Vector_X++] || Y = [Vector_Y++];

            Lsetup( . . . ) Loop_Counter = N;
                A1 += XHi * YLo (M), A0 += XLo * YLo (FU)
                || X = [Vector_X++] || Y = [Vector_Y++];

            A0 = A0 >> 16;
            A1 += A0;
            A0 = 0;

            Lsetup( . . . ) Loop_Counter = N;
                A1 += XLo * YHi (M), A0 += XHi * YHi 
                || X = [Vector_X++] || Y = [Vector_Y++];

            A1 = A1 >>> 15;
            R0 = (A0 += A1);

        The above code assumes that the vectors Vector_X and Vector_Y
        are accessed as circular buffers. Note that it calculates the
        term (XLo * YLo) first to preserve precision, and precision is
        retained for as long as possible inside the accumulators (using
        8.32 notation).

        The code is incomplete because it ignores the requirement for
        rounding intermediate results.

    Example:

        #include <vector.h>
        #define SAMPLES_N 100

        fract32 input1[SAMPLES_N];
        fract32 input2[SAMPLES_N];
        fract32 r;

        r = vecdot_fr32 (input1,input2,SAMPLES_N);

    Cycle Counts:

        36 + (2 * N)

        where N is the number of samples

        (Measured for an ADSP-BF532 using version 5.0.0.28 of the ADSP-BF5xx
        Family Simulator and includes the overheads involved in calling
        the library procedure as well as the costs associated with argument
        passing).

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR  libGroup      = vector.h;
.FILE_ATTR  libFunc       = __vecdot_fr32;
.FILE_ATTR  libFunc       = vecdot_fr32;
.FILE_ATTR  libName       = libdsp;
.FILE_ATTR  prefersMem    = internal;
.FILE_ATTR  prefersMemNum = "30";
.FILE_ATTR  FuncName      = __vecdot_fr32;
#endif

#include <sys/anomaly_macros_rtl.h>

.GLOBAL __vecdot_fr32;
.TYPE   __vecdot_fr32,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

__vecdot_fr32:

   /* Initialize and set up circular buffering */

      R3 = R2 << 2;             

      B0 = R0;                       // initialise circular buffer on Vector_X
      I0 = R0;
      L0 = R3;

      B1 = R1;                       // initialise circular buffer on Vector_Y
      I1 = R1;
      L1 = R3;

      P1 = R2;                       // set loop counter
      R1 = 0x80 (Z);                 // set R1 to a constant which will
                                     // be used below as a rounding factor

      // logically the following instructions would be executed first
      // but they have been moved here to soak-up some of the latency
      // that occurs between initializing a DAG register and then
      // reading via it.
      CC = R2 <= 0;                  // check that number of elements is +ve
      IF CC JUMP .ret_zero;

#if defined(__WORKAROUND_SPECULATIVE_LOADS) || \
    WA_05000428
      NOP;
      NOP;
      NOP;
#endif

      A1 = R1.L * R1.L (M),          // fill A1 with rounding constant 0x4000
      A0 = R1.L * R1.L               // fill A0 with rounding constant 0x8000
      || R3 = [I0++]                 // and initialize R3 to Vector_X[0]
      || R2 = [I1++];                // and initialize R2 to Vector_Y[0]

      // loop over the number of samples
      LSETUP(.loop_lolo_hilo, .loop_lolo_hilo) LC0 = P1;
.loop_lolo_hilo:
         /*  Calculate Acc1 = (XHi * YLo), Acc0 += (XLo * YLo) */ 
         A1 += R2.H * R3.L (M), A0 += R2.L * R3.L (FU)
         || R2 = [I1++] || R3 = [I0++];

      A0 = A0 >> 16;                 // shift correct LoLo 
      A0 += A1;                      // add LoLo to Sum HiLo
      A1 = A0;                       // move sum into the correct accumulator  
      A0 = 0;                        // reset accumulator

      // loop over the number of samples
      LSETUP(.loop_hihi_lohi, .loop_hihi_lohi) LC0 = P1;
.loop_hihi_lohi:
         /* Calculate Acc1 += (YHi * XLo), Acc0 += (YHi * XHi) */ 
         A1 += R3.H * R2.L (M), A0 += R3.H * R2.H
         || R3 = [I0++] || R2 = [I1++];

      A1 = A1 >>> 15;                // shift correct sum (HiLo + LoHi)
      R0 = (A0 += A1);               // compute the final result

      L0 = 0;                        // reset circular buffers
      L1 = 0;
      RTS;

.ret_zero:
      L0 = 0;                        // reset circular buffers
      L1 = 0;

      R0 = 0;
      RTS;

.__vecdot_fr32.end:


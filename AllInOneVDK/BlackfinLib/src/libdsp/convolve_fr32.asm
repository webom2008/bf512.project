/****************************************************************************
 *
 * convolve_fr32.asm : $Revision: 1.1 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: convolve_fr32 - Convolution

    Synopsis:

        #include <filter.h>
        void convolve_fr32 (const fract32    input_x[],
                            int              length_x,
                            const fract32    input_y[],
                            int              length_y,
                            fract32          output[] );

    Description:

        This function convolves two sequences pointed to by input_x and
        input_y. If input_x points to the sequence whose length is length_x
        and input_y points to the sequence whose length is length_y, the
        resulting sequence pointed to by output has length length_x+length_y-1.

    Error Conditions:

        If either of the length arguments is negative or zero, the function
        will return without modifying the output array.

    Algorithm:

        output[i] = sum( input_x[j] * input_y[i-j] )

           where i = [0, 1, 2, ... , (length_x + length_y - 1)-1]
                 j = [max( 0, i + 1 - length_y ), .., min( k, length_x - 1 )]

    Implementation:

        The algorithm is implemented as:

           Loop for all output values

              Intialize accumulators with rounding constants 

              Loop for set number of elements

                 Compute the sum of products 
                 (HiLo and LoLo part of fract32 multiply)

              Shift correct LoLo and add to sum HiLo, rewind read pointers.

              Loop for set number of elements

                 Compute the sum of products
                 (LoHi and HiHi part of fract32 multiply)              

              Shift correct sum (HiLo + LoHi) and add to sum HiHi.

              Set minimum and maximum loop count for the next iteration,
              update pointers as required.

        The function uses circular buffering for the input vectors input_x
        and input_y.
  
    Example:

        #include <filter.h>
        #define LENGTH_X 85
        #define LENGTH_Y 25

        fract32 input_x[LENGTH_X];
        fract32 input_y[LENGTH_Y];
        fract32 output[LENGTH_X+LENGTH_Y-1];

        convolve_fr32 (input_x, LENGTH_X,
                       input_y, LENGTH_Y, output);

    Cycle Counts:

        57 + (lenght_out * 25) + (length_x * (2 * length_y))

        where:  lenght_out = length_x + length_y - 1

        Rational: (lenght_out * 25) = cost outer loop
                                      (incurred for all elements). 

                  (length_x * (2 * length_y)) = cost inner loop 
                       
                  The problem is that the number of iterations for the inner
                  loop vary. For the first length_y-1 elements and the last 
                  length_y-1 elements the number of iterations in the inner 
                  loop keep increasing / descreasing. However, assuming that
                  length_x >= length_y, one can obtain a constant count of
                  length_y-1: 

                      iterations for first output + 
                      iterations for lenght_out - (length_y-1) = length_y

                   Ditto:

                      iterations for last output +
                      iterations (length_y-1) = length_y                     

                  Thus:
                      remaining iter = lenght_out - 2 * (length_y - 1)            
                                     = length_x + length_y - 1 
                                       - 2 * (length_y - 1)  
                                     = length_x - length_y + 1 
                       
                      first/last iter = 2 * (length_y - 1) / 2 
                                      = length_y - 1

                      total iter inner = remaining iter + first/last iter
                                       = length_x - length_y + 1 + length_y - 1    
                                       = length_x

        (Measured for an ADSP-BF532 using version 5.0.0.35 of the ADSP-BF5xx
        Family Simulator and includes the overheads involved in calling
        the library procedure as well as the costs associated with argument
        passing).

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR  libGroup      = filter.h;
.FILE_ATTR  libFunc       = __convolve_fr32;
.FILE_ATTR  libFunc       = convolve_fr32;
.FILE_ATTR  libName       = libdsp;
.FILE_ATTR  prefersMem    = internal;
.FILE_ATTR  prefersMemNum = "30";
.FILE_ATTR  FuncName      = __convolve_fr32;
#endif

/* Location of input arguments on the stack */

#define  OFFSET_IN_DATA         28

#define  IN_LENGTH_Y      ( OFFSET_IN_DATA )
#define  IN_ADRESS_OUT    ( OFFSET_IN_DATA +  4 )

/* Placement of temporary data on the stack */

#define  ROUNDING_CONST       0
#define  VAL_ZERO             2
#define  VAL_ONE              4
#define  BASE_INPUT_X         8
#define  BASE_INPUT_Y        12
#define  SIZE_TEMP_DATA      (BASE_INPUT_Y + 4)

.GLOBAL __convolve_fr32;
.TYPE   __convolve_fr32,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

__convolve_fr32:

   /* Initialize and read filter_state */

      [--SP] = (R7:4);               // preserve registers

      R3 = 1;                        // curr_k = 1 (at iteration 1)

      R6 = R1 - R3 (NS)              // length_out = length_x - 1 
      || R4 = [SP+IN_LENGTH_Y];      // load length_y

      R6 = R6 + R4 (NS)              // length_out = length_x + length_y - 1 
      || P0 = [SP+IN_ADRESS_OUT];    // load address output

      SP += -SIZE_TEMP_DATA;         // Allocate space on the stack for
                                     // local data

      CC = R1 <= 0;                    
      IF CC JUMP .done;              // exit if length_x <= 0 

      CC = R4 <= 0;                  // exit if length_y <= 0
      IF CC JUMP .done;


      R7 = R1 << 2                   // I0 = &input_x (as circular buffer)
      || [SP+BASE_INPUT_X] = R0;

      B0 = R0;
      I0 = R0;
      L0 = R7;

      R7 = R4 << 2                   // I1 = &input_y (as circular buffer)
      || [SP+BASE_INPUT_Y] = R2;

      B1 = R2;
      I1 = R2;
      L1 = R7;

      P1 = R6;                       // P1 = Loop counter outer (=length_out)
      P2 = 1;                        // P2 = Loop counter inner

      M0 = 4;                        // offset to rewind read pointers

      R7 = 0x80;                     // R7 = Rounding constant

      R4 = R3 - R4 (NS)              // curr_j = 1 - length_y (at iteration 0)
      || [SP+ROUNDING_CONST] = R7;    
      
      R0 = PACK (R3.H, R3.L)         
      || [SP+VAL_ONE] = R3;


      // loop for length output (= lenth_x + length_y - 1)
      LSETUP(.iter_out_start,.iter_out_end) LC0 = P1;

.iter_out_start:
         A1 = R7.L * R7.L (M),       // fill A1 with rounding constant 0x4000
         A0 = R7.L * R7.L            // fill A0 with rounding constant 0x8000
         || R5 = [I0++]              // load input_x[], input[y]  
         || R6 = [I1--];  

      /* Calculate Acc1 = (XHi * YLo), Acc0 + = (XLo * YLo) */

         // loop for j, where
         //    j = max_j - min_j + 1,
         //       min_j = max (0, k + 1 - length_y);
         //       max_j = min (k, length_x - 1 );
         LSETUP(.loop_lolo_hilo,.loop_lolo_hilo) LC1 = P2;
.loop_lolo_hilo:
            A1 += R5.H * R6.L (M), A0 += R5.L * R6.L (FU)
            || R5 = [I0++] || R6 = [I1--];

      /* Rewind pointers input_x and input_y by (loop count P2 + 1).
      ** Since they are circular buffers, the offset in M0 must not exceed
      ** the size of either buffer. Setting the offset M0 to P2 will ensure
      ** that the hardware restrictions are adhered to. Rewinding the remaining
      ** read is done separately.
      */

         R4 = R4 + R0 (NS)           // increment curr_j = 1 + 1 - length_y
         || R6 = [I1++] || I0 -= 4;  // reset input pointers (dummy load) 

         A0 = A0 >> 16               // shift correct LoLo
         || R6 = [I1++M0] || I0 -= M0 ;  // reset input pointers (dummy load)

         A0 += A1                    // add LoLo to Sum HiLo
         || R5 = [I0++] || R6 = [I1--];

         A1 = A0;                    // move sum into the correct accumulator
         A0 = 0;                     // reset accumulator

      /* Calculate Acc1 + = (YHi * XLo), Acc0 + = (YHi * XHi) */

         // loop over the number of coefficients
         LSETUP(.loop_hihi_lohi,.loop_hihi_lohi) LC1 = P2;
.loop_hihi_lohi:
            A1 += R6.H * R5.L (M), A0 += R6.H * R5.H
            || R5 = [I0++] || R6 = [I1--];

         A1 = A1 >>> 15;

         R6 = (A0 += A1)
         || R5 = W[SP+VAL_ZERO] (Z);

      /* Update inner loop counter and 
      ** compute new starting addresses for input_x and input_y    
      **
      **    min_j = max (0, k + 1 - length_y);
      **    max_j = min (k, length_x - 1 );
      **
      **    for (j = min_j; j <= max_j; j++) {
      **       sum += input_x[j] * input_y[k - j];
      **    }              
      */ 
         R6 = MAX (R5, R4)           // compute min_j = max(0, k + 1 - length_y)
         || [P0++] = R6;             // store output value  

         R5 = R6 << 2                // offset input_x = min_j * sizeof(fract32)
         || R0 = [SP+BASE_INPUT_X];  // load &input_x[0]

         R7 = R3 - R6 (NS)           // offset input_y = k - min_j 
         || R2 = [SP+BASE_INPUT_Y];  // load &input_y[0]

         R7 <<= 2;                   // offset input_y * sizeof(fract32)

         R3 += 1;                    // increment curr_k 

         R0 = R0 + R5;               // &input_x = &input_x[0] + offset input_x
         I0 = R0;

         R2 = R2 + R7;               // &input_y = &input_y[0] + offset input_y
         I1 = R2; 
         
         R5 = MIN (R3, R1)           // compute max_j = min (k + 1, length_x)
         || R0 = [SP+VAL_ONE];       // (since loop iters from min_j to max_j)

         R5 = R5 - R6 (NS)           // compute new inner loop count 
         || R7 = W[SP+ROUNDING_CONST] (Z); 

         P2 = R5; 

         R5 <<= 2;                   // compute offset rewind

.iter_out_end:
         M0 = R5;          


.done:
      SP += SIZE_TEMP_DATA;          // Release temporary data on the stack

      L0 = 0;                        // reset circular buffers
      L1 = 0;

      (R7:4) = [SP++];               // restore preserved registers

      RTS;

.__convolve_fr32.end:


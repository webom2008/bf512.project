/*****************************************************************************
 *
 * rms_fr16.asm : $Revision: 1865 $
 *
 * (c) Copyright 2000-2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: rms_fr16 - root mean square

    Synopsis:

        #include <stats.h>
        fract16 rms_fr16(const fract16 samples[],
                         int           sample_length);

    Description:

        This function returns the root mean square of the elements within
        the input vector samples[]. The number of elements in the vector
        is sample_length.

    Error Conditions:

        If the number of samples is less than 1, then the function will
        return zero.

        If the function detects an overflow, then it will return 0x7FFF.

    Algorithm:

        The root mean square is defined as

            sqrt( (x[0]^2 + x[1]^2 + ........ + x[N-1]^2)/N )

    Implementation:

        The function uses both accumulators to calculate the sum of the
        square of each array element. 
        
        An inner loop accumulates a maximum of 256 values (the maximum 
        possible before overflow has a significant effect). This will also 
        preserve maximum accuracy. For the first time, the inner loop is
        executed sample_length % 256 times (if the remainder is non-zero)
        or 256 times (if sample_length is a multiple of 256). Thereafter
        the loop is executed 256 times. For example for a length of 679:
           679 % 256 = 167 + 256 + 256 = 679
        And for a length of 768:
           768 % 256 = 0 => 256 + 256 + 256 = 768

        An outer loop utilizes the second accumulator to compute the final sum.
        To provide better protection against overflow, as well as to preserve
        as many significant bits as possible, the sum computed in the inner
        loop will be downshifted by 8 before being added to the total.
        The outer loop will be executed ((sample_length / 256) + 1) times
        (if number of elements is not a multiple of 256) or (sample_length/256)
        times (if number of elements is a multiple of 256). Going back to the
        above example:
           sample_length = 679 => (int)(679/256) + 1 = 2 + 1 = 3 iterations.
           sample_length = 768 => (int)(768/256)     = 3 iterations.

        The final sum is then moved to a register prior to performing the 
        division (by N) - this may involve first scaling the calculation to 
        avoid saturation.

        The function sqrt_fr16 is used to calculate the square root, which
        means that the result of the division must be no greater than 0x7FFF.
        This may involve further scaling of the intermediate result. At this
        point the function may detect overflow if the total scaling required
        is more than 2^16.

    Stack Size:

        Two words to preserve registers

    Example:

        #include <stats.h>
        #define SIZE 256

        fract16 input[SIZE];
        fract16 result;

        result = rms_fr16 (input,SIZE);

    Cycle Counts:

        61 + (6 * p) + ((p - 1) * 256) + q + cost of sdiv32 + cost of sqrt_fr16

        where p = (int)(N/256) + 1 (if N%256 != 0), otherwise p = (int)(N/256)
              q = N%256 (if N%256 != 0), otherwise q = 256

        Measured using version 4.5.0.22 of the ADSP-BF5xx Blackfin
        Family Simulator and includes the overheads involved in
        calling the library procedure and passing in the arguments.

 *****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.FILE_ATTR FuncName      = __rms_fr16;
.FILE_ATTR libNamei      = libdsp;

.FILE_ATTR libGroup      = stats.h;
.FILE_ATTR libFunc       = __rms_fr16;
.FILE_ATTR libFunc       = rms_fr16;

.FILE_ATTR prefersMem    = internal;
.FILE_ATTR prefersMemNum = "30";

#endif

/* Macros */

#define BIT0_MASK 0x00000001

/* The macro BIT0_MASK above identifies the least significant bit in a word */


.EXTERN  ___div32;
.TYPE    ___div32,STT_FUNC;

.EXTERN  __sqrt_fr16;
.TYPE    __sqrt_fr16,STT_FUNC;

.GLOBAL  __rms_fr16;
.TYPE    __rms_fr16,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

__rms_fr16:

   /* Check for valid input and initialize */

      P0 = R0;                /* address input array X                       */
      CC = R1 <= 1;           /* test the number of samples (=sample_length) */
      IF CC JUMP .ret_short;

      R3 = 256;
      R0 = R1 >> 8;           /* sample_length / 256                         */ 
      R2 = R0 << 8;           /* (int)(sample_length / 256) * 256            */
      R2 = R1 - R2;           /* sample_length % 256                         */
      CC = R2 == 0;              
      IF CC R2 = R3;          /* if sample_length % 256 == 0, set it to 256  */ 
 
      [--SP] = R4;            /* preserve R4                                 */

      R4 = R0;                /* outer loop counter (if sample_length%256==0)*/ 
      R0 += 1;                /* outer loop counter (if sample_length%256!=0)*/
      IF CC R0 = R4;

      P1 = R0;                /* initialize outer loop counter               */
      P2 = R2;                /* initialize inner loop counter               */

      [--SP] = RETS;          /* push RETS onto the stack                    */
      R4 = 16;

      A1 = A0 = 0
      || R2 = w[P0++](Z);     /* prime loop by loading X[0]  */


      /* loop for ((N/256) + 1) if (sample_length % 256) != 0 or
              for (N/256) if (sample_length % 256) == 0
         to compute total sum
      */
      LSETUP (.outer_start, .outer_end) LC0 = P1;
.outer_start:
 
          /* loop for N-(N/256) in the first iteration, 256 thereafter 
             if (sample_length % 256) != 0, otherwise use 256 throughout
             to compute partial sum (max length == 256)
          */
          LSETUP (.inner, .inner) LC1 = P2;
.inner:
             A1 += R2.L * R2.L  || R2 = w[P0++](Z);

          P2 = R3;            /* set the loop counter to 256   */

          A1 = A1 >>> 8;      /* discard lower 8-bits          */
          R0 = (A0 += A1);    /* update total sum              */ 

.outer_end:
          A1 = 0;             /* prime loop for next iteration */


   /* Convert the intermediate sum to 1.31 notation 

      The intermediate sum must be scaled by 16 before calculating 
      the square root (or by 8 after the square root)
   */

      R3 = 8; 
      R2.L = SIGNBITS A0;
      R2 = R2.L (X);
      CC = R2 < R3;        /* CC = 1 if R0=A0 would saturate         */

      R0 <<= 8;            /* R0 = the intermediate sum (1.31)       */
      A0 = A0 >> 8;
      R2 = A0;             /* R2 = the scaled intermediate sum (8.16)*/

      IF CC R0 = R2;       /* R0 = the scaled sum if R0=A0 saturates */

   /* Record the scaling performed above */

      R2 = 0;
      IF CC R4 = R2;       /* R4 = 0 if A0 has already been scaled   */


   /* Divide the sum of the squares by N */

      CALL.X ___div32;


   /* Convert the quotient from 1:31 to 1:15 notation
   **
   ** (If the quotient is less than 32768 then no special action is
   ** required - otherwise it must be scaled so that it is just less
   ** than 32768.
   **
   ** Note that R4 indicates the amount of scaling that has to be applied
   ** to the final result and also note that:
   **
   **    SQRT (X/(2^n)) == SQRT(X) / (2^m)      where m = n/2
   **
   ** This means that any scaling prior to calculating the sqrt must be
   ** an even power. It also means that R4 must be adjusted to compensate
   ** for the pre-scaling)
   */

      R2.L = SIGNBITS R0;
      R2 = R2.L (X);

      R1 = 16;
      R1 = R1 - R2;              /* R1 = amount of scaling reqd if R1>0   */
      R2 = 0;
      CC = R1 <= 0;              /* CC = 1 if quotient is less than 32768 */
      IF CC R1 = R2;             /* R1 = 0 if CC=1
                                 ** so that next 4 instrs are effective no-ops
                                 */

      R1 += 1;                      /* Round scaling reqd          */
      BITCLR(R1,BITPOS(BIT0_MASK)); /*       to next even multiple */

   /* Check for overflow
   **
   ** (typically the intermediate sum of squares is divided by 2^16
   ** before calculating the square root - but this scaling is sometimes
   ** deferred in the interests of preserving precision. The amount of
   ** scaling still required is recorded in R4 and if this value turns
   ** negative then there is an overflow condition)
   */

      R4 = R4 - R1;
      CC = R4 < 0;
      IF CC JUMP .overflow;

   /* Calculate SQRT ((sum of the squares)/N) */

      R0 >>= R1;                 /* scale the quotient appropriately     */
      CALL.X __sqrt_fr16;

      R4 >>= 1;
      R0 >>= R4;                 /* apply any remaining scaling required */

.exit:
      RETS = [SP++];
      R4   = [SP++];
      RTS;

.ret_short:
      CC = R1 == 1;
      IF CC JUMP .ret_single;
      R0 = 0;                    /* return zero if N <= 0 */
      RTS;

.ret_single:
      R0 = w[P0] (Z);
      R0 = ABS R0 (V);           /* if n=1 then SQRT(x1^2/1) = |x1| */
      RTS;

.overflow:
      R0 = 0x7FFF;
      JUMP .exit;

.__rms_fr16.end:

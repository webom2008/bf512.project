/****************************************************************************
 *
 * matmmlt_fr32.asm : $Revision: 1.1 $
 *
 * (c) Copyright 2008-2009 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: matmmlt_fr32 - real matrix matrix multiplication

    Synopsis:

        #include <matrix.h>
        void matmmlt_fr32 (const fract32   matrix_x[],
                           int             rows_x,
                           int             columns_x,
                           const fract32   matrix_y[],
                           int             columns_y,
                           fract32         product[]);
    Description:

        The matmmlt_fr32 function computes the product of the input matrices
        matrix_x[][] and matrix_y[][], and stores the result to matrix
        product[][]. The dimensions of matrix matrix_x[][] are rows_x and
        columns_x. The dimensions of matrix matrix_y[][] are columns_x and
        columns_y. The resulting matrix product[][] has dimensions rows_x
        and columns_y.

    Error Conditions:

        If either rows_x, columns_x, or columns_y are less or equal to zero,
        the function will return without modifying the matrix product[][].

    Algorithm:

        product[rx][cy] = sum (x[rx][cx] * y[cx]][cy])

            where rx = 0, 1, ..., rows_x-1
                  cx = 0, 1, ..., columns_x-1
                  cy = 0, 1, ..., columns_y-1

                  x = matrix_x
                  y = matrix_y

    Implementation:

        The implementation is based on the following algorithm:

            A1 = 0x4000, A0 = 0x8000
            || X = [Row_Vector_X++] || Y = [Column_Vector_Y++];

            Lsetup( . . . ) Loop_Counter = COLUMNS_X;
                A1 + = XHi * YLo (M), A0 + = XLo * YLo (FU)
                || X = [Row_Vector_X++] || Y = [Column_Vector_Y++];

            A0 = A0 >> 16;
            A1 + = A0;
            A0 = 0;

            Lsetup( . . . ) Loop_Counter = COLUMNS_X;
                A1 + = XLo * YHi (M), A0 + = XHi * YHi
                || X = [Row_Vector_X++] || Y = [Column_Vector_Y++];

            A1 = A1 >>> 15;
            R0 = (A0 + = A1);

        Circular buffering is used for Row_Vector_X and Column_Vector_Y.

    Example:

        #include <matrix.h>
        #define ROWS_X      8
        #define COLUMNS_X  16
        #define COLUMNS_Y  32

        fract32 in_x[ROWS_X * COLUMNS_X];
        fract32 in_y[COLUMNS_X * COLUMNS_Y];
        fract32 out[ROWS_X * COLUMNS_Y];

        matmmlt_fr32 (in_x, ROWS_X, COLUMNS_X,
                      in_y, COLUMNS_Y, out);

    Cycle Counts:

        49 + (Nrx * (14 + (Ncy * (11 + (2 * Ncx)))))

        where Nrx is the number of rows in x,
              Ncx is the number of columns in x,
              Ncy is the number of columns in y.

        (Measured for an ADSP-BF532 using version 5.0.0.35 of the ADSP-BF5xx
        Family Simulator and includes the overheads involved in calling
        the library procedure as well as the costs associated with argument
        passing).

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR  libGroup      = matrix.h;
.FILE_ATTR  libFunc       = __matmmlt_fr32;
.FILE_ATTR  libFunc       = matmmlt_fr32;
.FILE_ATTR  libName       = libdsp;
.FILE_ATTR  prefersMem    = internal;
.FILE_ATTR  prefersMemNum = "30";
.FILE_ATTR  FuncName      = __matmmlt_fr32;
#endif


#if !defined(__ADSPLPBLACKFIN__)
/* Suppress Assembler Warning ea5504 "Memory store instruction use may trigger
** hardware anomaly 05-00-0165". The run-time libraries are not required to
** workaround this anomaly.
*/

.MESSAGE/SUPPRESS 5504;
#endif


#define   OFFSET_STACK_ARGS         (12+4)
#define   STACK_ADDRESS_MATRIX_Y    (OFFSET_STACK_ARGS)
#define   STACK_COLUMNS_MATRIX_Y    (OFFSET_STACK_ARGS+4)
#define   STACK_ADDRESS_MATRIX_OUT  (OFFSET_STACK_ARGS+8)


.GLOBAL __matmmlt_fr32;
.TYPE   __matmmlt_fr32,STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

__matmmlt_fr32:

   /* Initialize */
      
      [--SP] = R7;                      // Save reserved registers

      P0 = R0;                          // address matrix x
      P2 = R2;                          // loop counter cx (columns in matrix x)
      R3 = [SP+STACK_ADDRESS_MATRIX_Y];
      B1 = R3;                       
      R3 = [SP+STACK_COLUMNS_MATRIX_Y];
      P1 = R3;                          // loop counter cy (columns in matrix y)

      CC = R1 <= 0;                     // exit if rows in matrix x <= 0
      IF CC JUMP .done;
      
      CC = R2 <= 0;                     // exit if columns in matrix x <= 0
      IF CC JUMP .done;

      CC = R3 <= 0;                     // exit if columns in matrix y <= 0
      IF CC JUMP .done;


      R0 = R3 << 2;                     // size columns in matrix y
      R2 <<= 2;                         // size columns in matrix x       

      I1 = B1;                          // position column vector matrix y
      B0 = P0;                          // position row vector matrix x
      I0 = P0;

      R3 *= R2;                         // size matrix y

      M1 = R0;                          // offset next column in matrix y
      L1 = R3;                          // size circular buffer matrix y
      L0 = R2;                          // size circular buffer matrix x
      
      R3 = [SP+STACK_ADDRESS_MATRIX_OUT];                  
      I2 = R3;

      R7 = 0x80 (Z);                    // set R7 to a constant which will
                                        // be used below as a rounding factor
      

      /* Loop for number of rows in matrix x */
.loop_rx:

         P0 = P0 + (P2 << 2);           // next base row vector matrix x
         R1 += -1;                      // decrement loop counter rx  

         // loop for number of columns in matrix y 
         LSETUP( .loop_cy_start, .loop_cy_end ) LC0 = P1;
.loop_cy_start:

            A1 = R7.L * R7.L (M),       // fill A1 with rounding constant 0x4000
            A0 = R7.L * R7.L            // fill A0 with rounding constant 0x8000
            || R2 = [I0++] || R3 = [I1++M1];

         /* Calculate Acc1 = (XHi * YLo), Acc0 + = (XLo * YLo) */

            // loop for number of columns in matrix x
            LSETUP(.loop_cx_lolo_hilo,.loop_cx_lolo_hilo) LC1 = P2;
.loop_cx_lolo_hilo:
               A1 += R2.H * R3.L (M), A0 += R2.L * R3.L (FU)
               || R2 = [I0++] || R3 = [I1++M1];

            A0 = A0 >> 16;              // shift correct LoLo
            A0 += A1;                   // add LoLo to Sum HiLo
            A1 = A0;                    // move sum into the correct accumulator
            A0 = 0;                     // reset accumulator


         /* Calculate Acc1 + = (YHi * XLo), Acc0 + = (YHi * XHi) */

            // loop for number of columns in matrix x
            LSETUP(.loop_cx_hihi_lohi,.loop_cx_hihi_lohi) LC1 = P2;
.loop_cx_hihi_lohi:
               A1 += R3.H * R2.L (M), A0 += R3.H * R2.H
               || R2 = [I0++] || R3 = [I1++M1];

            A1 = A1 >>> 15 || I0 -= 4;
            R0 = (A0 += A1) || I1-= M1;
            I1 += 4;

.loop_cy_end:
            [I2++] = R0;                // store matrix product 

         CC = R1 <= 0;                  // loop until loop counter rx expired
         I1 = B1;                       // position column vector matrix y
         B0 = P0;                       // position row vector matrix x
         I0 = P0;

         IF !CC JUMP .loop_rx (BP);


.done:

      L0 = 0;                           // reset circular buffers
      L1 = 0;

      R7 = [SP++];                      // restore register before returning

      RTS;

.__matmmlt_fr32.end:

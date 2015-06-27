/****************************************************************************
 *
 * transpm16.asm : $Revision: 1.2 $
 *
 * (c) Copyright 2000-2010 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: transpm - matrix transpose (16-bit data)

    Synopsis:

        #include <matrix.h>

        void transpm_fr16 (const fract16  matrix[],
                           int            rows,
                           int            columns,
                           fract16        transpose[]);

    Description:

        The transpm_fr16 function computes the transpose of input matrix
        matrix[][] and stores the result in transpose[][]. The dimensions
        of matrix matrix[][] are rows and columns. The resulting matrix
        transpose[][] is of dimensions columns and rows.

    Error Conditions:

        If either rows or columns are less than or equal to zero, the
        function will return without modifying the matrix transpose[][].

    Algorithm:

        The following equation applies:

            transpose[cx][rx] = matrix[rx][cx]

                where rx = 0, 1, ..., rows-1
                      cx = 0, 1, ..., columns-1

    Implementation:

        For optimum performance, the input and output matrices have to be
        declared in two different data memory banks to avoid data bank
        collision.

    Example:

        #include <matrix.h>
        #define ROWS      8
        #define COLUMNS  16

        fract16 in[ROWS * COLUMNS];
        fract16 out[COLUMNS * ROWS];

        transpm_fr16 (in, ROWS, COLUMNS, out);

    Cycle Counts:

        42 + (columns * (rows + 4))

        (Measured for an ADSP-BF532 using version 5.0.0.35 of the ADSP-BF5xx
        Family Simulator and includes the overheads involved in calling
        the library procedure as well as the costs associated with argument
        passing).

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libGroup      = matrix.h;
.file_attr FuncName      = __transpm16;
.file_attr libFunc       = __transpm_fr16;
.file_attr libFunc       = transpm_fr16;
.file_attr libName       = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
#endif

#if !defined(__ADSPLPBLACKFIN__)
/* Suppress Assembler Warning ea5504 "Memory store instruction use may trigger
** hardware anomaly 05-00-0165". The run-time libraries are not required to
** workaround this anomaly.
*/

.MESSAGE/SUPPRESS 5504;
#endif

.GLOBAL __transpm16;
.GLOBAL __transpm_fr16;

.TYPE   __transpm16, STT_FUNC;
.TYPE   __transpm_fr16, STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

__transpm16:
__transpm_fr16:

      P0 = R0;                         // Address input array A

      R0 = PACK (R2.H, R2.L)
      || R3 = [SP+12];                 // Address output array C

      I1 = R3;

      CC = R1 <= 0;                    // Exit if the number of rows <= 0
      IF CC JUMP .done;

      CC = R2 <= 0;                    // Exit if the number of columns <= 0
      IF CC JUMP .done;


      R0 *= R1;

      CC = R0 == 1;                    // Check if rows = columns = 1
      IF CC JUMP .scalar;              // If TRUE branch to .scalar


      R3 = R2 << 1;                    // Compute the space required for one
                                       // row of the matrix

      P2 = R2;                         // Loop counter columns
      P1 = R3;                         // Offset next column in input matrix

      [--SP] = P5;                     // Preserve reserved register

      P5 = R1;                         // Loop counter rows
      I0 = P0;                         // Preserve address


      /* Traverse input column by column */
      LSETUP (.iter_columns_start, .iter_columns_end) LC0 = P2;

.iter_columns_start:   
         R0 = W[P0++P1](X);

         /* Traverse current column row by row, copy elements */
         LSETUP (.iter_rows, .iter_rows) LC1 = P5;

.iter_rows: 
            R0 = W[P0++P1](X) || W[I1++] = R0.L; 

#if defined(__WORKAROUND_INFINITE_STALL_202)
/* After 2 possible dual dag load/stores, need 2 prefetch loads to avoid the
** anomaly (since the first prefetch is still part of the anomaly sequence).
*/
         PREFETCH[SP];
         PREFETCH[SP];
#endif

         I0 += 2;                      // Point to the next column 
                                       // in the first row
.iter_columns_end:     
         P0 = I0;                      // Re-position input pointer

      P5 = [SP++];                     // Restore preserved register

.done:
      RTS;


.scalar:

   /* Handle special case: rows = columns = 1 */
      R0 = W[P0] (Z);
      W[I1] = R0.L;

      RTS;

.__transpm_fr16.end:
.__transpm16.end:


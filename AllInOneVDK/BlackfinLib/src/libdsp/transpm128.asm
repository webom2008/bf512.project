/****************************************************************************
 *
 * transpm128.asm : $Revision: 1.2.10.1 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ***************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: transpm - matrix transpose (128-bit data)

    Synopsis:

        #include <matrix.h>
        void ctranspmd (const complex_long_double  matrix[],
                       int                         rows,
                       int                         columns,
                       complex_long_double         transpose[]);

    Description:

        The transpm128 function computes the transpose of input matrix
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

        The first element of the input matrix is copied to the first element
        of the output matrix transpose, and then the remainder of transpose
        is accessed as a circular buffer where successive elements in the
        input matrix are copied to the next 'row-th' element in the output
        matrix.

        For optimum performance, the input and output matrices have to be
        declared in two different data memory banks to avoid data bank
        collision.

    Example:

        #include <matrix.h>
        #define ROWS      8
        #define COLUMNS  16

        complex_long_double  in[ROWS * COLUMNS];
        complex_long_double  out[COLUMNS * ROWS];

        ctranspmd (in, ROWS, COLUMNS, out);

    Cycle Counts:

        87 + (((rows * columns) - 1) * 4)

        (Measured for an ADSP-BF532 using version 5.0.0.35 of the ADSP-BF5xx
        Family Simulator and includes the overheads involved in calling
        the library procedure as well as the costs associated with argument
        passing).

*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libGroup      = matrix.h;
.file_attr FuncName      = __transpm128;
.file_attr libFunc       = ctranspmd;
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

.GLOBAL __transpm128;
.TYPE   __transpm128, STT_FUNC;

.SECTION/DOUBLEANY program;
.ALIGN 2;

__transpm128:

   /* Read parameters */

      P0 = R0;                       // P0 will be used to access matrix
      R3 = [SP+12];                  // load &transpose
      I0 = R3;                       // I0 will be used to access transpose

   /* Validate rows and columns */

      CC = R1 <= 0;
      IF CC JUMP .return;
      CC = R2 <= 0;
      IF CC JUMP .return;

      R0  = R2;                      // copy columns
      CC  = R2 == 1;
      R2 *= R1;                      // size = rows * columns
      IF !CC R0 = R1;                // step size output array
                                     // if columns == 1 => R0 = 1
                                     // otherwise R0 = number of rows

      R1 = R0 << 4                   // step size expressed as bytes
      || R0 = [P0++];

      R0 = [P0++] || [I0++] = R0;

      R1 += -12;

      R0 = [P0++] || [I0++] = R0;
      [I0++] = R0;

      M0 = R1;                       // byte index from one column to the next
                                     // !! M0 must not exceed ((rows*cols)-1)

   /* Handle the first element */

      R2 += -1;                      // size = size - 1;
      P1 = R2;                       // loop counter = size
      CC = R2 == 0;

      R2 = R2 << 4                   // convert size into bytes
      || R0 = [P0++];

      [I0++M0] = R0;

      IF CC JUMP .return;            // exit if rows = columns = 1

   /* Configure the output as a circular buffer */

      R3 += 16;                      // &transpose[1]
      B0 = R3;                       // circular buffer starts at &transpose[1]
                                     // !! ensure that I0 within range
                                     //    B0 <= I0 < B0 + L0 before enabling
                                     //    circular buffer

      L0 = R2;                       // circular buffer length
                                     // = ((rows * cols) - 1) * 4

   /* Transpose the matrix */

#if defined(__WORKAROUND_AVOID_DAG1)
      // 05-00-0114, 05-00-0141, 05-00-0005 and 05-00-0024

      // Loop for number of elements - 1
      LSETUP (.loop_head,.loop_tail) LC0 = P1;
.loop_head:
         R0 = [P0++];
         R1 = [P0++];
         R2 = [P0++];
         R3 = [P0++];
         [I0++] = R0;
         [I0++] = R1;
         [I0++] = R2;
.loop_tail:
         [I0++M0] = R3;

#else

      R0 = [P0++];

      // Loop for number of elements - 1
      LSETUP (.loop_head2,.loop_tail2) LC0 = P1;
.loop_head2:
         R1 = [P0++] || [I0++] = R0;
         R2 = [P0++] || [I0++] = R1;
         R3 = [P0++] || [I0++] = R2;         
.loop_tail2:
         R0 = [P0++] || [I0++M0] = R3;

#if defined(__WORKAROUND_INFINITE_STALL_202)
/* After 2 possible dual dag load/stores, need 2 prefetch loads to avoid the
** anomaly (since the first prefetch is still part of the anomaly sequence).
*/
      PREFETCH[SP];
      PREFETCH[SP];

#endif /* __WORKAROUND_INFINITE_STALL_202 */
#endif /* __WORKAROUND_AVOID_DAG1 */

      L0 = 0;                        // reset the length register to zero

.return:

      RTS;

.__transpm128.end:

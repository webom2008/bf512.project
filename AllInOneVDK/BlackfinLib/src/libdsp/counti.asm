/*****************************************************************************
 *
 * counti.asm : $Revision
 *
 * (c) Copyright 2006-2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: countones - count one bits in word

    Synopsis:

        #include <math.h>

        int countones (int parm);
        int lcountones(long int parm);

    Description:

        The countones function counts the number of bits set in the
        argument parm. For this function, the result will be in the
        range [0 - 32].

        The header file math.h defines the function as a built-in function
        and so this is an intrinsic function for which the the compiler will
	normally generate in-lined code. However if the switch -no-builtin
        is specified the compiler will ignore the intrinsic version of the
        function and will instead generate code to reference this library
        function.

        The functions countones and lcountones are identical functions.

    Error Conditions:

        There are no error conditions.

    Parameters:

        R0 - the input argument

    Algorithm:

        The function uses the ONES instruction; a generic C-written
        implementation of the function would be:

            int result = 0;

            while (parm != 0) {
                if (parm < 0)
                    result++;

                parm = parm << 1;
            }

    Cycle Counts:

        16 cycles (measured for a BF532 using version 4.5.0.17 of the
        ADSP-BF5xx Family Simulator and includes the overheads involved
        in calling the library procedure as well as the costs associated
        with argument passing)

    Code Size: 8 bytes

 *****************************************************************************
#endif

      .file "counti.asm";

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.file_attr libGroup      = math.h;
.file_attr libGroup      = math_bf.h;
.file_attr libName       = libdsp;
.file_attr FuncName      = __countones;
.file_attr FuncName      = __lcountones;

.file_attr libFunc       = countones;
.file_attr libFunc       = __countones;
.file_attr libFunc       = lcountones;
.file_attr libFunc       = __lcountones;

.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";

#endif

#include <sys/anomaly_macros_rtl.h>

      .section/DOUBLEANY program;
      .align 2;

      .global __countones;
      .type   __countones,STT_FUNC;

      .global __lcountones;
      .type   __lcountones,STT_FUNC;

__countones:
__lcountones:

      R0.L = ONES R0;
      R0   = R0.L (X);
#if WA_05000371
      NOP;
      NOP;
#endif
      RTS;

.__lcountones.end:
.__countones.end:

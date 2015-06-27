/*****************************************************************************
 *
 * adi_fatal_error.asm : $Revision: 1.4.14.1 $
 *
 * Copyright (C) 2009 Analog Devices, Inc. All Rights Reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

   Function: adi_fatal_error - fatal error handler.

   Synopsis:

      #include <stdlib.h>
      void adi_fatal_error(int general_code,
                           int specific_code,
                           int value);

   Description:

      Record that a fatal error has occurred, write the details of the
      error to the global variables adi_fatal_error_general_code,
      adi_fatal_error_specific_code and adi_fatal_error_value.
      The return address from the last call will be written to
      adi_fatal_error_pc. This function should be jumped to rather than
      called to preserve the return address for the call into the previous
      function.

   Error conditions:

      None.

   Implementation notes:

      Since this function enters an infinite loop and will therefor never
      return, no registers or paramaters are preserved to the stack.

 *****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libName=libevent;
.FILE_ATTR FuncName=__adi_fatal_error;
.FILE_ATTR libFunc=_adi_fatal_error;
.FILE_ATTR libFunc=__adi_fatal_error;
.FILE_ATTR prefersMem=internal;
.FILE_ATTR prefersMemNum="30";
#endif

.EXTERN _adi_fatal_error_general_code;
.EXTERN _adi_fatal_error_specific_code;
.EXTERN _adi_fatal_error_value;
.EXTERN _adi_fatal_error_pc;

.SECTION program;
.ALIGN 2

_adi_fatal_error:
      CLI R3;
      P1.L = _adi_fatal_error_general_code;
      P1.H = _adi_fatal_error_general_code;
      [P1] = R0;
      P1.L = _adi_fatal_error_specific_code;
      P1.H = _adi_fatal_error_specific_code;
      [P1] = R1;
      P1.L = _adi_fatal_error_value;
      P1.H = _adi_fatal_error_value;
      [P1] = R2;
      R3 = RETS;
      P1.L = _adi_fatal_error_pc;
      P1.H = _adi_fatal_error_pc;
      [P1] = R3;

__fatal_error: 
      IDLE;
      JUMP __fatal_error;
.__fatal_error.end:
._adi_fatal_error.end:

.GLOBAL _adi_fatal_error;
.TYPE _adi_fatal_error, STT_FUNC;
.GLOBAL __fatal_error;

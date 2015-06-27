/*****************************************************************************
 *
 * call_fatal_error.asm : $Revision: 1.4.14.1 $
 *
 * Copyright (C) 2009 Analog Devices, Inc. All Rights Reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

   Function: call_fatal_error - CRT wrapper for __adi_fatal_error.

   Description:

      Wrapper routine that calls _adi_fatal_error with the general code
      RunTimeError and value of 0.

   Error conditions:

      None.

   Implementation notes:

      Using the wrapper reduces the code size over configuring all
      of the paramaters for each error condition.
      The performance overhead of the additional call is inconsequential
      as this function will only be called when there is an unrecoverable
      error. 

 *****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libName=libevent;
.file_attr FuncName=__call_fatal_error;
.file_attr libFunc=_call_fatal_error;
.file_attr libFunc=__call_fatal_error;
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";
#endif

#include <sys/fatal_error_code.h>
.extern _adi_fatal_error;

.section program;
.align 2

__call_fatal_error:
       R0 = _AFE_G_RunTimeError;
       R2 = 0;
       JUMP.X _adi_fatal_error;
.__call_fatal_error.end:
.global __call_fatal_error;
.type __call_fatal_error, STT_FUNC;


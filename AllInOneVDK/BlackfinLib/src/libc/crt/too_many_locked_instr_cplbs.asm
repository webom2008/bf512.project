/*****************************************************************************
 *
 * too_many_locked_instr_cplbs.asm : $Revision: 1.4.14.1 $
 *
 * Copyright (C) 2009 Analog Devices, Inc. All Rights Reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

   Function: too_many_locked_instr_cplbs.

   Description:

      CPLB initialisation has detected that too many instruction cplbs have
      been configured as locked, preventing any other cplbs to be used.
      Jump to _adi_fatal_error to report non-recoverable error.

   Error conditions:

      None.

   Implementation notes:

      Jump to wrapper call_fatal_error to set up duplicate parameters.

 *****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libName=libevent;
.FILE_ATTR FuncName=too_many_locked_instruction_cplbs;
.FILE_ATTR libFunc=too_many_locked_instruction_cplbs;
.FILE_ATTR prefersMem=internal;
.FILE_ATTR prefersMemNum="30";
#endif

#include <sys/fatal_error_code.h>
.EXTERN __call_fatal_error;

.SECTION program;
.ALIGN 2

too_many_locked_instruction_cplbs:
       R1 = _AFE_S_TooManyLockedInstructionCPLB;
       JUMP.X __call_fatal_error;
.too_many_locked_instruction_cplbs.end:

.GLOBAL too_many_locked_instruction_cplbs;
.TYPE too_many_locked_instruction_cplbs, STT_FUNC;


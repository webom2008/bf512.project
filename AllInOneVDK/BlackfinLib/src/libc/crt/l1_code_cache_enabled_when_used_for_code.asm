/*****************************************************************************
 *
 * l1_code_cache_enabled_when_used_for_code.asm : $Revision: 1.4.14.1 $
 *
 * Copyright (C) 2009 Analog Devices, Inc. All Rights Reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

   Function: l1_code_cache_enabled_when_used_for_code.

   Description:

      CPLB initialisation has detected that the l1 code cache has been
      enabled despite the section being used to map code.
      Jump to _adi_fatal_error to report non-recoverable error.

   Error conditions:

      None.

   Implementation notes:

      Jump to wrapper call_fatal_error to set up duplicate parameters.

 *****************************************************************************
#endif

/* l1_code_cache_enabled_when_l1_used_for_code.s:
 *
 * Copyright (C) 2009 Analog Devices, Inc. All Rights Reserved.
 *
 * l1 code cache has been enabled though l1 is used for code,
 * report a fatal error (via __call_fatal error to minimize code size) */

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libName=libevent;
.FILE_ATTR FuncName=l1_code_cache_enabled_when_l1_used_for_code;
.FILE_ATTR libFunc=l1_code_cache_enabled_when_l1_used_for_code;
.FILE_ATTR prefersMem=internal;
.FILE_ATTR prefersMemNum="30";
#endif

#include <sys/fatal_error_code.h>
.EXTERN __call_fatal_error;

.SECTION program;
.ALIGN 2

l1_code_cache_enabled_when_l1_used_for_code:
       R1 = _AFE_S_L1CodeCacheEnabledWhenL1UsedForCode;
       JUMP.X __call_fatal_error;
.l1_code_cache_enabled_when_l1_used_for_code.end:

.GLOBAL l1_code_cache_enabled_when_l1_used_for_code;
.TYPE l1_code_cache_enabled_when_l1_used_for_code, STT_FUNC;


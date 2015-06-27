/*****************************************************************************
 *
 * l1_data_a_cache_enabled_when_used_for_data.asm : $Revision: 1.4.14.1 $
 *
 * Copyright (C) 2009 Analog Devices, Inc. All Rights Reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

   Function: l1_data_a_cache_enabled_when_used_for_code.

   Description:

      CPLB initialisation has detected that the data A cache has been
      enabled despite the section being used to map data.
      Jump to _adi_fatal_error to report non-recoverable error.

   Error conditions:

      None.

   Implementation notes:

      Jump to wrapper call_fatal_error to set up duplicate parameters.

 *****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libName=libevent;
.FILE_ATTR FuncName=l1_data_a_cache_enabled_when_used_for_data;
.FILE_ATTR libFunc=l1_data_a_cache_enabled_when_used_for_data;
.FILE_ATTR prefersMem=internal;
.FILE_ATTR prefersMemNum="30";
#endif

#include <sys/fatal_error_code.h>
.EXTERN __call_fatal_error;

.SECTION program;
.ALIGN 2

l1_data_a_cache_enabled_when_used_for_data:
       R1 = _AFE_S_L1DataACacheEnabledWhenUsedForData;
       JUMP.X __call_fatal_error;
.l1_data_a_cache_enabled_when_used_for_data.end:

.GLOBAL l1_data_a_cache_enabled_when_used_for_data;
.TYPE l1_data_a_cache_enabled_when_used_for_data, STT_FUNC;


/*
** CPLB miss handler, for all-locked CPLBs.
** Copyright (C) 2003-2007 Analog Devices Inc. All Rights Reserved.
*/

.file_attr libName=libevent;
.file_attr FuncName=__cplb_miss_all_locked;
.file_attr libFunc=__cplb_miss_all_locked;
.file_attr libFunc=_cplb_miss_all_locked;
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";

#include <sys/fatal_error_code.h>
.extern __call_fatal_error;

.section program;
.align 2;
__cplb_miss_all_locked:
	// This function is called by the default exception
	// handler when a CPLB Miss exception has occurred,
	// and the default handler is unable to find an
	// active CPLB entry which can be replaced, since
	// every active entry is currently locked.
	// When using CPLBs, you must ensure that some
	// CPLBs are replacable, for when Miss events occur.

        /* Call _adi_fatal_error (via __call_fatal_error) instead 
         * of looping locally */
        R1 = _AFE_S_CPLBMissAllLocked;
        JUMP.X __call_fatal_error;

.__cplb_miss_all_locked.end:
.global __cplb_miss_all_locked;
.type __cplb_miss_all_locked, STT_FUNC;

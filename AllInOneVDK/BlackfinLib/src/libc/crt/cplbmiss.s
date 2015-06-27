/*
** CPLB miss handler.
** Copyright (C) 2003-2007 Analog Devices Inc. All Rights Reserved.
*/

.file_attr libName=libevent;
.file_attr libFunc=_cplb_miss_without_replacement;
.file_attr FuncName=__cplb_miss_without_replacement;
.file_attr libFunc=__cplb_miss_without_replacement;
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";

#include <sys/fatal_error_code.h>
.extern __call_fatal_error;

.section program;
.align 2;
__cplb_miss_without_replacement:
	// This function is called by the default exception
	// handler when a CPLB Miss exception has occurred,
	// and the default handler is unable to find a
	// CPLB entry in the table which corresponds to the
	// address which caused the miss.
	// When using CPLBs, you must ensure that all areas
	// of memory that you access are covered by suitable
	// program or data CPLBs.

        /* Call _adi_fatal_error (via __call_fatal_error) rather 
         * than looping locally */
        R1 = _AFE_S_CPLBMissWithoutReplacement;
        JUMP.X __call_fatal_error;

.__cplb_miss_without_replacement.end:
.global __cplb_miss_without_replacement;
.type __cplb_miss_without_replacement, STT_FUNC;

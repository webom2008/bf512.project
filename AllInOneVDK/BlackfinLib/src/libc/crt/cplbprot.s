/*
** CPLB protection violation handler.
** Copyright (C) 2003-2007 Analog Devices Inc. All Rights Reserved.
*/

.file_attr libName=libevent;
.file_attr FuncName=__cplb_protection_violation;
.file_attr libFunc=_cplb_protection_violation;
.file_attr libFunc=__cplb_protection_violation;
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";

#include <sys/fatal_error_code.h>
.extern __call_fatal_error;

.section program;
.align 2;
__cplb_protection_violation:
	// This function is called by the CPLB event handler
	// when a Protection Violation has been raised, and
	// it does not apply to the first write of a clean
	// Write Back page.
	// When a page is cached in Write Back mode, the CPLB
	// "Dirty" flag is significant. The page starts out
	// "Clean". The first write to the page triggers a
	// Protection Violation exception, and the exception
	// handler marks the page as "Dirty", before allowing
	// the write to proceed. If the page is later evicted
	// from the CPLBs, the "Dirty" flag will ensure that
	// all pending writes are first flushed from the cache.
	// If the exception is a Protection Violation, and yet
	// does not correspond to a write to a clean, Write Back
	// page currently in the CPLBs, then it really *is* a
	// protection violation, and this handler is invoked.

        /* call _adi_fatal_error (via __call_fatal_error) rather 
         * than looping locally */
        R1 = _AFE_S_CPLBProtectionViolation;
        JUMP.X __call_fatal_error;

.__cplb_protection_violation.end:
.global __cplb_protection_violation;
.type __cplb_protection_violation, STT_FUNC;

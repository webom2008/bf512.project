/*
** CPLB double-hit handler.
** Copyright (C) 2010 Analog Devices Inc. All Rights Reserved.
*/

.file_attr libName=libevent;
.file_attr FuncName=__cplb_double_hit;
.file_attr libFunc=_cplb_double_hit;
.file_attr libFunc=__cplb_double_hit;
.file_attr prefersMem=external;
.file_attr prefersMemNum="70";

#include <sys/fatal_error_code.h>
.extern __call_fatal_error;

.section program;
.align 2;
__cplb_double_hit:
        // This function is called by the CPLB event handler
        // when a DCPLB or ICPLB lookup finds more than one
        // valid CPLB applies to an accessed address.
        // This condition can arise temporarily when an instruction
        // crosses a page boundary; a miss on fetching the second
        // part of the instruction will install a second instance
        // of the ICPLB for the page covering the first part of the
        // instruction. The handler checks for this circumstance and
        // handles it, returning CPLB_REPLACED as a result. If the
        // handler returns CPLB_DOUBLE_HIT, it means that it really
        // is a case of two different CPLBs covering the same space.

        /* call _adi_fatal_error (via __call_fatal_error) rather 
         * than looping locally */
        R1 = _AFE_S_CPLBDoubleHit;
        JUMP.X __call_fatal_error;

.__cplb_double_hit.end:
.global __cplb_double_hit;
.type __cplb_double_hit, STT_FUNC;

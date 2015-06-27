#pragma once
#ifndef __NO_BUILTIN
#pragma system_header /* sys_alloc.h */
#endif
/************************************************************************
 *
 * sys_alloc.h
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *
 ************************************************************************/

#ifndef _SYS_ALLOC_H
#define _SYS_ALLOC_H
/* sys_alloc.h internal header */

#include <stdlib.h>
#include <string.h>
#include <yvals.h>

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_1_1:"ADI Header allows external linkage name 'extern asm'")
#pragma diag(suppress:misra_rule_5_7:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_6_3:"ADI header allows basic types")
#pragma diag(suppress:misra_rule_8_5:"ADI header allows inline functions")
#pragma diag(suppress:misra_rule_19_4:"ADI Header allows non-MISRA compliant macro definition")
#endif /* _MISRA_RULES */

_C_LIB_DECL
void *_sys_calloc(size_t size, size_t count);
void *_sys_malloc(size_t size);
void _sys_free(void *ptr);

_END_C_LIB_DECL

#if defined(__ADSP21000__) && !defined(_ADI_THREADS)
extern int _which_heap;
#define __ADI_DEFAULT_HEAP (_which_heap)
#else
#define __ADI_DEFAULT_HEAP 0
#endif

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

#endif /* _SYS_ALLOC_H */

/****************************************************************************
 *
 * sys_free.c
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 * System free - free memory allocated by _sys_calloc or _sys_malloc.
 *
 ****************************************************************************/

#include "sys_alloc.h"

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=_sys_free")
#pragma file_attr(  "libFunc=__sys_free")
#pragma file_attr(  "libFunc=_adi_rtl_get_tls_ptr")
#pragma file_attr(  "libFunc=adi_rtl_get_tls_ptr")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#endif

void _sys_free(void *ptr)
{
#if defined(_ADI_HEAP_DEBUG_BUILD)
  free_impl(ptr);
#else
  _heap_free(0,ptr);
#endif
}

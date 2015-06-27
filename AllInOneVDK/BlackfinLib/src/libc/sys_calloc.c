/****************************************************************************
 *
 * sys_calloc.c
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 * System calloc - allocate zero'd memory or call fatal error.
 *
 ****************************************************************************/

#include "sys_alloc.h"
#include <sys/fatal_error_code.h>

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=_sys_calloc")
#pragma file_attr(  "libFunc=__sys_calloc")
#pragma file_attr(  "libFunc=_adi_rtl_get_tls_ptr")
#pragma file_attr(  "libFunc=adi_rtl_get_tls_ptr")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#endif

void *_sys_calloc(size_t size, size_t count)
{
  size = size * count;
  void *ptr = 
#if defined(_ADI_HEAP_DEBUG_BUILD)
              _heap_malloc_impl(__ADI_DEFAULT_HEAP, size);
#else
              _heap_malloc(__ADI_DEFAULT_HEAP, size);
#endif

  if(!ptr)
    adi_fatal_error(_AFE_G_LibraryError, 
                    _AFE_S_InsufficientHeapForLibrary, size);

  memset(ptr, '\0', size);

  return ptr;
}

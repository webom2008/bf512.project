/****************************************************************************
 *
 * sys_malloc.c
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 * System malloc - allocate memory or call fatal error.
 *
 ****************************************************************************/

#include "sys_alloc.h"
#include <sys/fatal_error_code.h>

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=_sys_malloc")
#pragma file_attr(  "libFunc=__sys_malloc")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#endif

void *_sys_malloc(size_t size)
{
  void *ptr = 
#if defined(_ADI_HEAP_DEBUG_BUILD)
              _heap_malloc_impl(__ADI_DEFAULT_HEAP, size);
#else
              _heap_malloc(__ADI_DEFAULT_HEAP, size);
#endif
  if(!ptr)
    adi_fatal_error(_AFE_G_LibraryError, 
                    _AFE_S_InsufficientHeapForLibrary, size);
  return ptr;
}


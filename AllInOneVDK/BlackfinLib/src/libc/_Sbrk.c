/************************************************************************
 *
 * _Sbrk.c: $Revision: 3543 $
 *
 * (c) Copyright 2000-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
/*
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
/*
** Request more memory from the system heap->
** This function only allocates memory; "freeing" is a fiction maintained
** by the higher-level malloc, which maintains a free-list of deallocated
** blocks. This function doesn't maintain any block list. It just
** keeps increasing the allocated space until it runs out of room.
**
** The heap sizes are determined by the LDF file, which will contain
** something like this:
**
** MEMORY
** {
**    HEAP { TYPE(RAM) START(0xmmmmmmmm) END(0xnnnnnnnn) WIDTH(8) }
** }
** SECTIONS
** {
**    heap
**    {
**        ldf_heap_space = .;
**        ldf_heap_end = ldf_heap_space + MEMORY_SIZEOF(HEAP);
**    } >HEAP
** }
**
*/
#pragma file_attr( "libGroup=stdlib.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=_heap_initheaps")
#pragma file_attr(  "libFunc=__heap_initheaps")
#pragma file_attr(  "libFunc=_heap_Sbrk")
#pragma file_attr(  "libFunc=__heap_Sbrk")
#pragma file_attr(  "libFunc=_Sbrk")
#pragma file_attr(  "libFunc=__Sbrk")
#pragma file_attr(  "libFunc=malloc")
#pragma file_attr(  "libFunc=_malloc")
#pragma file_attr(  "libFunc=calloc")
#pragma file_attr(  "libFunc=_calloc")
#pragma file_attr(  "libFunc=realloc")
#pragma file_attr(  "libFunc=_realloc")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#include <heapinfo.h>
#include "xsync.h"

extern unsigned long heap_table[];

void _heap_initheaps(void)
{
  _LOCK_GLOBALS();
  /* The __nheaps check is needed here to make this threadsafe. */
  if (__nheaps == 0) {
    /* Copy across the base, length and user-id of each of the
       heaps defined in the LDF file. */
    unsigned long *hptr = heap_table;
    while (*hptr) { // null base indicates end of default heaps
      char *base = (char *)(*hptr++);
      unsigned long length = *hptr++;
      unsigned long userid = (int)(*hptr++);
      _heap_install_impl(base, length, userid, 1);
    }
  }
  _UNLOCK_GLOBALS();
}

/*
** Process a request for INCR more bytes, returning a pointer to
** the start of the newly-allocated space if successful. If we
** have run out of space, return -1 as a pointer.
*/
#pragma optimize_for_speed
void *_heap_Sbrk(int idx, int incr)
{
    char *current_break;
    unsigned long uincr, space_left;
    heapdata *heap;

    if (idx < 0 || __nheaps <= idx)
        return (void *)-1;  // Bad heap index

    heap = heap_address_from_index(idx);
    if (!heap)
        return (void *)-1;  // Bad heap

    current_break = heap->current;

    /* check whether there's space left in the heap. Be careful
       of "sign bits" for high addresses, and of large requests
       wrapping the 32-bit address when added to the current level */

    uincr = (unsigned long)incr;
    space_left = (heap->base + heap->length) -
                 current_break;

    if (space_left < uincr)
        return (void *)-1;

    /* Return the current level, and increase */

    heap->current += incr;
    return (void *)current_break;
}

void *_Sbrk(int incr)
{
    return _heap_Sbrk(0, incr);
}

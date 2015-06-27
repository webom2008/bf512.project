#include <heapinfo.h>
#include "xsync.h"

#if defined(_ADI_HEAP_DEBUG_BUILD)
#include<adi_heap_debug.h>
#endif

#pragma file_attr( "libGroup=stdlib.h")
#pragma file_attr(  "libName=libc")

#pragma file_attr(  "libFunc=_heap_install")
#pragma file_attr(  "libFunc=__heap_install")

#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

int _heap_install(void *base, size_t length, int userid)
{
  if (__nheaps == 0)
    _heap_initheaps();

#if defined(_ADI_HEAP_DEBUG_BUILD)
  CHECK_INIT;

  /* check if we're using heap in ISR */
  CHECK(_IS_IN_ISR(),__dbg_in_isr, 0x0, "heap_install in ISR",)
#endif
  
  _LOCK_GLOBALS();
  int new_user_id = _heap_install_impl(base, length, userid, 0);

#if defined(_ADI_HEAP_DEBUG_BUILD)
  /* don't register the new heap if we fail to create it */
  if(new_user_id != -1)
    __register_new_heap(new_user_id);
#endif

  _UNLOCK_GLOBALS();

  return new_user_id;
}


int _heap_install_impl(void *base, size_t length, int userid, int in_init)
{
  const size_t min_align_mask = 0x7;  /* allocate in units of 8 bytes */

  /* Sanity checking */
  if (length < (sizeof(heapdata) + min_align_mask))
    return -1;

  heapdata *heap = __heaps.first_heap;
  heapdata *oldheap = 0;

  int i = 0;
  while (i < __nheaps) {
    if (!heap)
      return -1;    /* __nheaps is wrong */
    if (heap->userid == userid)
      return -1;    /* this id is already in use! */
    oldheap = heap;
    heap = heap->next_heap;
    i++;
  }

  if (oldheap && oldheap->next_heap)
    return -1;    /* __nheaps is wrong */

  heapdata *newheap = (heapdata*)base;
  size_t aligned_base = (size_t)base;
  aligned_base += sizeof(heapdata);  /* reserve admin space at start */
  aligned_base += min_align_mask;    /* then align upwards so that first */
  aligned_base &= ~min_align_mask;   /* alloc is on the right boundary */
  newheap->base = newheap->current = (char *)aligned_base;
  newheap->length = length - (aligned_base - (size_t)base);
  newheap->userid = userid;
  newheap->next_heap = NULL;
#ifdef _FAST_MULTI_HEAPS
  newheap->_Aldata._Plast = 0;
  newheap->_Aldata._Head = 0;
#endif
  __nheaps++;
  if (oldheap) {
    oldheap->next_heap = newheap;
  } else {
    __heaps.first_heap = newheap;
  }
  return i;
}

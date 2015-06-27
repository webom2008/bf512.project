#include <heapinfo.h>
#include "xsync.h"

#pragma file_attr( "libGroup=stdlib.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=_heap_init")
#pragma file_attr(  "libFunc=__heap_init")
#pragma file_attr(  "libFunc=malloc")
#pragma file_attr(  "libFunc=_malloc")
#pragma file_attr(  "libFunc=calloc")
#pragma file_attr(  "libFunc=_calloc")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#if defined(_ADI_HEAP_DEBUG_BUILD)
#include "adi_heap_debug.h"
#endif

int _heap_init(int idx)
{
  if (idx < 0 || __nheaps <= idx)
    return -2;  // Bad heap index

#if defined(_ADI_HEAP_DEBUG_BUILD)
  __heap_info *hinfo;
  __alloc_info *ainfo;

  /* check if we're using heap in ISR */
  CHECK(_IS_IN_ISR(),__dbg_in_isr, 0x0, "heap_init in ISR",)

  /* check heap is valid */
  hinfo = __get_heap_info_from_index(idx, true);
  CHECK(!hinfo,__dbg_invalid_input,0x0,"re-init of invalid heap",;);

  _LOCK_GLOBALS();

  /* record call stack and heap index for any corruption reports */
  __get_call_stack();
  __heap_out_data.heapindex = idx;
  __heap_out_data.type = __dbg_init;

  /* mark all blocks on heap as free */
  ainfo = hinfo->first_alloc;
  while(ainfo)
  {
    CHECK((!__verify_alloc(ainfo)), __dbg_block_is_corrupt, ainfo->start,
          "re-init of corrupt heap",
          ainfo->state = __dbg_block_is_corrupt;);
    if(!__is_free_type(ainfo->type))
    {
      ainfo->type = __dbg_free;
      __set_guard_region(ainfo);
    }
    ainfo = ainfo->next;
  }


  /* log the heap_init event */
  __heap_out_data.state = __dbg_ok;
  __heap_out_data.start = 0x0;
  __heap_out_data.corrupt_address = 0x0;
  __heap_out_data.corrupt_value = 0x0;
  __heap_debug_write_trace();
  __clear_output_data(true);

  _UNLOCK_GLOBALS();

#endif

  heapdata *heap = heap_address_from_index(idx);

  _LOCK_GLOBALS();

  /* Don't allow malloc to resume any scans, when looking
  for free space to remove, because we might have just
  removed the Cells it would start with. */

#ifdef _FAST_MULTI_HEAPS
  heap->_Aldata._Plast = 0;  /* don't resume next scan */
  heap->_Aldata._Head = 0;  /* clear free list */
#else
  _Aldata._Plast = 0;  /* don't resume next scan */
  _Aldata._Head = 0;  /* clear free list */
#endif

  /* Reset the Sbrk pointer for this heap */
  heap->current = heap->base;

  _UNLOCK_GLOBALS();
  return 0;
}

#include <heapinfo.h>
#include "xsync.h"

#pragma file_attr( "libGroup=stdlib.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=_space_unused")
#pragma file_attr(  "libFunc=__space_unused")
#pragma file_attr(  "libFunc=_heap_space_unused")
#pragma file_attr(  "libFunc=__heap_space_unused")
#pragma file_attr(  "libFunc=_peak_size")
#pragma file_attr(  "libFunc=__peak_size")
#pragma file_attr(  "libFunc=_heap_peak_size")
#pragma file_attr(  "libFunc=__heap_peak_size")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

/* Ensure global data is in shared memory for multicore programs. */
#pragma file_attr("ThreadPerCoreSharing=MustShare")

volatile heapinfo __heaps;
volatile int __nheaps;

#ifdef _FAST_MULTI_HEAPS
/* Heap data stored in heap header */
#else
_Altab _Aldata = {0};  /* heap initially empty */
#endif

/* This function returns the amount of allocatable free space on the first heap
** (idx 0). Will return -1 if the heap hasn't been setup yet (i.e. nothing has
** been malloced */
int _space_unused(){
  return _heap_space_unused(0);
}

/* This function returns the amount of allocatable free space on the heap
** specified. Will return -1 if the heap hasn't been setup yet (i.e. nothing
** has been malloced) or if the heap index is invalid */
int _heap_space_unused(int idx){
  unsigned int totalFree;
  _Cell* freePtr;
  heapdata *heap;

  if (!__nheaps)
      _heap_initheaps();
  
  /* check boundary conditions */
  if (idx > __nheaps || idx < 0) return -1;

  heap = heap_address_from_index(idx);
  if (!heap) return -1;

  totalFree=(heap->base + heap->length) - heap->current;

  /* now we need to traverse the free list and work out how much space there
  ** is in the heap->.. */
#ifdef _FAST_MULTI_HEAPS
  for(freePtr=heap->_Aldata._Head; freePtr; freePtr=freePtr->_Next)
      totalFree+=freePtr->_Size;
#else
  for(freePtr=_Aldata._Head; freePtr; freePtr=freePtr->_Next){
    if (freePtr->_Id==idx)
      totalFree+=freePtr->_Size;
  }
#endif
  return totalFree;
}

/* This function returns the largest size that the default heap grew to 
** before the call to this function. Will return -1 if the heap has not
** been set up yet.*/
int _peak_size() {
  return _heap_peak_size(0);
}

/*  This function returns the peak value of the heap size for a given heap.
 ** Will reutrn -1 if the heap has not been set up yet.
 */
int _heap_peak_size(int idx)
{
  heapdata *heap;
  if ( !__nheaps)
    _heap_initheaps();

  /* Check boundary conditions */
  if (idx > __nheaps || idx < 0) return -1;
  heap = heap_address_from_index(idx);

  return (heap->current - heap->base);
}

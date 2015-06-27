#include <heapinfo.h>
#pragma file_attr( "libGroup=stdlib.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=heap_lookup")
#pragma file_attr(  "libFunc=_heap_lookup")
#pragma file_attr(  "libFunc=__heap_lookup")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

/*
 * Locking the heap list is not necessary here as traversing the list
 * without changing it is safe. That's because _heap_install_impl will only
 * append fully initialised heaps to the list and heaps cannot be deleted.
 */

int heap_lookup(int userid)
{
  if (__nheaps == 0)
    _heap_initheaps();
  
  heapdata *heap = __heaps.first_heap;
  int i = 0;
  while (heap) {
    if (heap->userid == userid)
      return i;
    heap = heap->next_heap;
    i++;
  }
  return -1;
}

void *heap_address_from_index(int i)
{
  if (__nheaps == 0) 
    _heap_initheaps();

  heapdata *heap = __heaps.first_heap;
  while (heap && i > 0) {
    heap = heap->next_heap;
    i--;
  }
  return heap;
}

#include <stdio.h>
#include <heapinfo.h>
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=_heap_print")
#pragma file_attr(  "libFunc=__heap_print")
#pragma file_attr(  "libFunc=_heap_print_heaps")
#pragma file_attr(  "libFunc=__heap_print_heaps")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

void _heap_print(int idx)
{
  char *base, *current, *end;
  int length, claimed;
  _Cell *q;
  heapdata *heap;

  heap = heap_address_from_index(idx);
  if (!heap) return;

  base = heap->base;
  length = heap->length;
  current = heap->current;
  end = base + length - 1;
  claimed = current - base;

  fprintf(stderr,"Heap %d (UID %d). %08x - %08x (%08x bytes, %08x claimed)\n",
    idx, heap->userid, base, end, length, claimed);
  fprintf(stderr,"Free blocks in heap:\n");
#ifdef _FAST_MULTI_HEAPS
  for (q = heap->_Aldata._Head; q; q = q->_Next)
#else
  for (q = _Aldata._Head; q; q = q->_Next)
#endif
    if (q->_Id == idx)
      fprintf(stderr,"%08x, %08x bytes\n", ((char*)q + CELL_OFF), q->_Size);
}

void _heap_print_heaps(void)
{
  int i;
  for (i = 0; i < __nheaps; i++)
    _heap_print(i);
}


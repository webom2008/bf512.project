/************************************************************************
 *
 * _primio_blkfn.h : $Revision: 5445 $
 *
 * (c) Copyright 2003-2011 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#include <ccblkfn.h>
#include <cplb.h>
#include <cplbtab.h>

#define FLUSH_CACHE(START,END) flush_cache(START,END)
#define FLUSH_CACHE_PRIMIOCB() flush_cache_for_PrimIOCB()
#define SYNCH_MEMORY()         ssync_int()
#define DISABLE_CACHE()
#define ENABLE_CACHE()

#pragma inline
#pragma always_inline
static void flush_cache_for_PrimIOCB(void) {
  /* The size of PrimIOCB is less than a single cache line flushed using
  ** flushinv instruction. We flush two line starting at PrimIOCB in case
  ** it falls over a line boundary in memory.
  ** This function is assumed to be called when interrupts are disabled.
  */
  unsigned int * ptr;
  ptr = flushinvmodup(&PrimIOCB);
  csync_int();         /* called with interrupts disabled so use _int syncs */
  flushinv(ptr);
  csync_int();
}

#pragma inline
#pragma always_inline
static void flush_cache(void *start, void *end) {
  /* Flush and invalidate the cache for a range of memory starting at 'start'
  ** and ending at 'end'.
  ** This function is assumed to be called when interrupts are disabled.
  */
    unsigned int *ptr = (unsigned int *)start;
    unsigned int *last = (unsigned int *)end;
    int n;

    ssync_int();       /* called with interrupts disabled so use _int syncs */

    /* Loop Count:
     *   FLUSHINV[Px++] = Invalidates one 32-byte cache line at a time
     *   (last - ptr)   = Distance between addresses in bytes
     *                 => Every iteration decrements distance by 32 bytes
     * One extra final iteration is done to ensure that the data
     * at the end of the range which may fall over a line boundary
     * is completely flushed.
     */
    for( n = ((last - ptr - 1)/32) + 1; n >= 0; n--) {
      ptr = flushinvmodup(ptr);
      csync_int();
    }
}


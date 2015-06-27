/*
** Copyright (C) 2001 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
** Blackfin implementation of signal() and interrupt().
**/
#pragma file_attr( "libGroup=signal.h")
#pragma file_attr( "libGroup=exception.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=signal")
#pragma file_attr(  "libFunc=_signal")
#pragma file_attr(  "libFunc=interrupt")
#pragma file_attr(  "libFunc=_interrupt")
#pragma file_attr(  "libFunc=set_vector_entry")
#pragma file_attr(  "libFunc=_set_vector_entry")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#include <signal.h>
#include <errno.h>
#include <sys/exception.h>
#include <sys/excause.h>
#include "xsignal.h"
#include "xsync.h"

static _Sigfun *set_vector_entry(int, _Sigfun *, int);
extern void exit(int);	/* to avoid warnings */

/*
** ANSI C signal() interface.
*/

_Sigfun *signal(int which, _Sigfun *fn)
{
  return set_vector_entry(which, fn, /* once_only=*/1);
}

/*
** ADI extension interrupt() interface.
*/

_Sigfun *interrupt(int which, _Sigfun *fn)
{
  return set_vector_entry(which, fn, /* once_only=*/0);
}


static _Sigfun *set_vector_entry(int which, _Sigfun *fn, int once_only)
{
  _Sigfun *old;
  vector_entry *i;
  if (which < 0 || which >= _NSIG) {
    errno = (int)SIG_ERR;
    return SIG_ERR;
  }
  i = &_vector_table[which];
  _LOCK_GLOBALS();
  old = i->fn;
  i->fn = fn;
  i->once_only = once_only;
  _UNLOCK_GLOBALS();
  return old;
  /* It would be nice to enable/disable the appropriate interrupt at
  this juncture, but that's a supervisor-only operation. We could
  check whether we're in supervisor mode, but that's *also* a
  supervisor-only only operation. So we just make do with returning
  immediately for ignored signals. */
}

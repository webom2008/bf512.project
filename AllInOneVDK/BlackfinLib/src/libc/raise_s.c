/*
** Copyright (C) 2001 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
** Frio implementation of signal's raise() function.
**/

#include <signal.h>
#include <sys/exception.h>
#include <sys/excause.h>
#include "xsignal_s.h"
#pragma file_attr( "libGroup=signal.h")
#pragma file_attr(  "libName=libsmall")
#pragma file_attr(  "libFunc=raise")
#pragma file_attr(  "libFunc=_raise")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

extern void exit(int);	/* to avoid warnings */

/*
** ANSI C function to raise a signal.
** ADI library treats this as a request to raise a genuine interrupt
** when possible, so we do that. We don't call the registered
** functions directly, unless they're not real events.
*/

int raise(int which)
{
  _Sigfun *fn;
  unsigned int ptr;
  if (which < 0 || which >= _NSIG)
    return -1;
  if (which <= _MAX_REAL_SIG) {
    return raise_interrupt((interrupt_kind)which, which, 0, 0, 0);
  }
  fn = _vector_table[which];
  if ((fn == 0) || (fn == SIG_IGN))
    return 0;
  ptr = (unsigned int)fn;
  if ((ptr & 1) == 0)
    _vector_table[which] = 0;
  ptr &= ~1;
  fn = (_Sigfun *)ptr;
  (*fn)(which);
  return 0;
}

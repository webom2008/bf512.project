/*
** Copyright (C) 2001 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
** Frio implementation of signal's raise() function.
**/

#pragma file_attr( "libGroup=signal.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=raise")
#pragma file_attr(  "libFunc=_raise")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
#include <signal.h>
#include <sys/exception.h>
#include <sys/excause.h>
#include "xsignal.h"

extern void exit(int);	/* to avoid warnings */

/*
** ANSI C function to raise a signal.
** ADI library treats this as a request to raise a genuine interrupt
** when possible, so we do that. We don't call the registered
** functions directly, unless they're not real events.
*/

int raise(int which)
{
  vector_entry *i;
  _Sigfun *fn;
  if (which < 0 || which >= _NSIG)
    return -1;
  i = &_vector_table[which];
  fn = i->fn;
  if (fn == SIG_IGN)
    return 0;
  /* If it's not a real signal, then we have to provide some handling. */
  if (which > _MAX_REAL_SIG) {
    /* No action registered, so close down, either politely or not */
    if (fn == SIG_DFL) {
      switch (which) {
        case SIGILL:
        case SIGBUS:
        case SIGFPE:
        case SIGSEGV:
        case SIGTERM:
        case SIGINT:
          exit(-1);
      }
    } else {
      if (i->once_only)
        i->fn = SIG_DFL;
      /* Something is registered, so call it. */
      (*fn)(which);
      return 0;
    }
  }
  /* It is a real signal, so arrange for it to be invoked. */
  switch (which) {
    case SIGEMU:
      return 0;		/* not supported */
    case SIGEVNT:
      return -1;	/* needs additional parameters - like which event */
    case SIGNMI:
      return raise_interrupt(ik_nmi, which, 0, 0, 0);
    default:
      return raise_interrupt((interrupt_kind)which, which, 0, 0, 0);
  }
}

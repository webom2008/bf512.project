/*
** Copyright (C) 2001 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
** Blackfin implementation of signal() and interrupt().
**/
#pragma file_attr( "libGroup=signal.h")
#pragma file_attr( "libGroup=exception.h")
#pragma file_attr(  "libName=libsmall")
#pragma file_attr(  "libFunc=signal")
#pragma file_attr(  "libFunc=_signal")
#pragma file_attr(  "libFunc=interrupt")
#pragma file_attr(  "libFunc=_interrupt")
#pragma file_attr(  "libFunc=set_vector_entry")
#pragma file_attr(  "libFunc=_set_vector_entry")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

/* Ensure global data is in shared memory for multicore programs. */
#pragma file_attr("ThreadPerCoreSharing=MustShare")

#include <signal.h>
#include <errno.h>
#include <sys/exception.h>
#include "xsignal_s.h"
#include "xsync.h"

static _Sigfun *set_vector_entry(int, _Sigfun *, int);
EX_INTERRUPT_HANDLER(despint);

/*
** ANSI C signal() interface.
*/

_Sigfun *signal(int which, _Sigfun *fn)
{
  return set_vector_entry(which, fn, /* recurring=*/0);
}

/*
** ADI extension interrupt() interface.
*/

_Sigfun *interrupt(int which, _Sigfun *fn)
{
  return set_vector_entry(which, fn, /* recurring=*/1);
}

static _Sigfun *set_vector_entry(int which, _Sigfun *fn, int recurring)
{
  _Sigfun *old;

#ifdef _LIBTPC
  /* The thread-per-core model does not support signal handlers that are
   * triggered by hardware interrupts because the interrupt vector tables
   * are core-local while signal semantics should be global.
   * On the BF561 at least, setting vectors on the other core is not
   * possible without some sort of inter-core communication protocol.
   */
  if (which <= _MAX_REAL_SIG || which >= _NSIG) {
#else
  if (which < 0 || which >= _NSIG) {
#endif
    /* not a recognised signal so report an error */
    errno = (int)SIG_ERR;
    return SIG_ERR;
  }
  
  _LOCK_GLOBALS();
  old = _vector_table[which]; 
  if (old != SIG_IGN) {	
    /* mask off "recurring" flag unless the handler is SIG_IGN */
    old = (_Sigfun *) ((unsigned)old & ~1);	
  }
  if (fn == SIG_IGN) {
    /* ignore the signal */
    _vector_table[which] = SIG_IGN;
    if (which <= _MAX_REAL_SIG) {
      /* disable corresponding hardware interrupt */
      unsigned int bits = ~(1<<which);
      __asm("CLI %1; %1 = %1 & %0; STI %1;\n" : : "d" (bits), "?d" (0));
    }
  } else if (fn == SIG_DFL) {
    /* Use the default handler */	
    _vector_table[which] = SIG_DFL;  	
    if (which <= _MAX_REAL_SIG)
      register_handler((interrupt_kind)which, despint);
  } else {
    /* enter the handling routine in the signal handler table */ 
    unsigned int ptr = (unsigned int)fn;
    ptr |= recurring;  
    _vector_table[which] = (_Sigfun *)ptr;
    if (which <= _MAX_REAL_SIG)
      register_handler((interrupt_kind)which, despint);
  }
  _UNLOCK_GLOBALS();
  
  return old;
}

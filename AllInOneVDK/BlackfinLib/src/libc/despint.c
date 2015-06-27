/*
** Copyright (C) 2001 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
** Blackfin despatcher for signal() and interrupt().
**/
#pragma file_attr( "libName=libevent")
#pragma file_attr( "libFunc=despatch_interrupt") 
#pragma file_attr( "libFunc=_despatch_interrupt")
#pragma file_attr( "prefersMem=any")
#pragma file_attr( "prefersMemNum=50")

#include <signal.h>
#include "xsignal.h"

extern void exit(int);	/* to avoid warnings */

/*
** Despatcher for signals - this gets called by the default hardware
** vector functions.
*/

void despatch_interrupt(int which, int value)
{
  vector_entry *i;
  _Sigfun *fn;
  if (which < 0 || which > _MAX_REAL_SIG)
    return;	/* shouldn't have been invoked through this entry */
  i = &_vector_table[which];
  fn = i->fn;
  if (fn == SIG_IGN && which != SIGNMI)
    return;
  if (fn == SIG_DFL) {
    exit(-1);
  }
  if (i->once_only && fn != SIG_DFL && fn != SIG_IGN)
    i->fn = SIG_DFL;
  (void)(*fn)(value);
  return;
}

/*
** Diagnostic exception handler. Displays information about
** the exception that occurred.
** Copyright (C) 2003 Analog Devices Inc. All Rights Reserved.
*/

#pragma file_attr(  "libName=libeventdbg")
#pragma file_attr(  "libFunc=_unknown_exception_occured")
#pragma file_attr(  "libFunc=__unknown_exception_occured")
#pragma file_attr(  "prefersMem=internal")
#pragma file_attr(  "prefersMemNum=30")
#include <ccblkfn.h>
#include <sys/exception.h>

extern void _Exit(void);

void _unknown_exception_occurred(void)
{
  interrupt_info iinfo[1];
  get_interrupt_info(ik_exception, iinfo);
  _ex_report_event(iinfo);
  _Exit();
  /* NOTREACHED */
}

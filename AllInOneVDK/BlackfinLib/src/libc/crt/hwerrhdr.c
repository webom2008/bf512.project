/*
** Diagnostic hardware error handler. Displays information about
** the hardware error that occurred.
** Copyright (C) 2003 Analog Devices Inc. All Rights Reserved.
*/

#pragma file_attr(  "libName=libeventdbg")
#pragma file_attr(  "libFunc=_hardware_error_handler")
#pragma file_attr(  "libFunc=__hardware_error_handler")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
#include <ccblkfn.h>
#include <sys/exception.h>

void _hardware_error_handler(void)
{
  interrupt_info iinfo[1];
  get_interrupt_info(ik_hardware_err, iinfo);
  _ex_report_event(iinfo);
  _Exit();
  /* NOTREACHED */
}

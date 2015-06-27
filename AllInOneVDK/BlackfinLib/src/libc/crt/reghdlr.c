/* Copyright (C) 2000 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
/*
** Registration of new Exception handling routines.
*/

#pragma file_attr(  "libName=libevent")
#pragma file_attr(  "libFunc=register_handler")
#pragma file_attr(  "libFunc=_register_handler")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
#include <sys/exception.h>
#include <sys/excause.h>


/*
** Register an interrupt handler within the EVT.
** Return previous value if there was one.
*/

ex_handler_fn register_handler(interrupt_kind kind, ex_handler_fn fn)
{
  return (ex_handler_fn)raise_interrupt(ik_exception, EX_SYS_REQ,
    EX_SYSREQ_REG_ISR, (int)kind, (int)fn);
}

/* Copyright (C) 2001 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
/* Variants of _halt() and _abort() that enter the idle state, and never
** leave. They do write a nice message beforehand, though.
** Intended for use on programs that are linked for running on the visual
** simulator (the default).
*/
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=_halt_processor")
#pragma file_attr(  "libFunc=__halt_processor")
#pragma file_attr(  "libFunc=_abort_processor")
#pragma file_attr(  "libFunc=__abort_processor")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <ccblkfn.h>

void _halt_processor(void)
{
  for (;;) {
    idle();
  }
  /* NOTREACHED */
}

void _abort_processor(void)
{
  for (;;) {
    idle();
  }
  /* NOTREACHED */
}

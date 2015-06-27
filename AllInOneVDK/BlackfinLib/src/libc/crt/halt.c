/* Copyright (C) 2001 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
/* Variants of _halt() and _abort() that use pseudo-instructions to cause
** the execution thread to completely disappear.
** Intended for use on programs that are linked for running on the 
** command-line simulator (requires special linkage).
*/
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=_halt_processor")
#pragma file_attr(  "libFunc=__halt_processor")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
#include <ccblkfn.h>

void _halt_processor(void)
{
#ifdef __ADSPBF535__
  __builtin_halt();
#else
  sys_halt();
#endif
  /* NOTREACHED */
}

void _abort_processor(void)
{
#ifdef __ADSPBF535__
  __builtin_abort();
#else
  sys_abort();
#endif
  /* NOTREACHED */
}

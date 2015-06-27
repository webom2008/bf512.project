/*
** Copyright (C) 2003 Analog Devices, Inc. All Rights Reserved.
*/
#pragma file_attr(  "libName=libeventdbg")
#pragma file_attr(  "libFunc=_install_default_handlers")
#pragma file_attr(  "libFunc=__install_default_handlers")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

/*
** Although the default configuration does not install handlers
** for anything beyond switching to lowest supervisor mode
** (IVG15) and exceptions for CPLB misses, it does call a
** routine to optionally install any others. This version of
** the routine installs a diagnostic handler for hardware
** errors, since hardware errors are a common symptom of
** programming errors (references to invalid memory spaces
** raises a hardware error, if CPLBs do not trap the reference
** first).
*/

#pragma file_attr("requiredForROMBoot")
#include <sys/platform.h>

extern void _hardware_error_handler(void);

int _install_default_handlers(int imask)
{
  *pEVT5 = (void *)_hardware_error_handler;
  imask |= (1<<5);
  return imask;
}

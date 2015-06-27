/************************************************************************
 *
 * xatexit.c: $Revision: 1.13.14.1 $
 *
 * Atexit slots for system functions.
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#pragma file_attr( "libGroup=stdlib.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=_atexit")
#pragma file_attr(  "libFunc=atexit")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

/* Ensure global data is in shared memory for multicore programs. */
#pragma file_attr("ThreadPerCoreSharing=MustShare")

#include "xatexit.h"

atexit_func_t __sys_atexit_funcs[_SYS_ATEXIT_SLOTS];

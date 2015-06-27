/************************************************************************
 *
 * atexit.c: $Revision: 1.13.14.1 $
 *
 * The atexit() function.
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
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

#include <stdlib.h>
#include "xatexit.h"
#include "xsync.h"

#ifdef __ADSP21000__
 #define SLOTS 33
#else
 #define SLOTS NATS
#endif

static atexit_func_t funcs[SLOTS];
static size_t count = 0;

static void
invoke_funcs(void)
{
    while (count)
        funcs[--count]();
}

int
atexit(atexit_func_t func)
{
    int ok;
    _LOCK_GLOBALS();
    ok = count < SLOTS;
    if (ok) {
        funcs[count++] = func;
        __sys_atexit_funcs[_SYS_ATEXIT_C_SLOT] = invoke_funcs;
    }
    _UNLOCK_GLOBALS();
    return ok - 1;
}

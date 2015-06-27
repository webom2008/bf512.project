/************************************************************************
 *
 * xatexit.h: $Revision: 1.14.16.3 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 * Function pointers to be invoked at exit.
 *
 ************************************************************************/

#ifndef _XATEXIT_H
#define _XATEXIT_H

#include <stdlib.h>

/* This enumeration determines the order and number of system functions that
 * need to be invoked at exit. Each module is allocated a slot in the
 * __sys_atexit_funcs array where it needs to write the address of its
 * wrapup function.
 * THE ORDERING OF THESE IS IMPORTANT!
 */
enum {
    _SYS_ATEXIT_C_SLOT,   /* standard C atexit() functions */
    _SYS_ATEXIT_CPP_SLOT, /* C++ static destructors */
#if !defined(__ADSP21000__)
    /* Instrumented profiling is not supported on SHARC */
    _SYS_ATEXIT_INSTRPROF_SLOT,  /* instrprof flush */
    /* Streams on SHARC are not flushed at exit.
     * Instead, printf() flushes every time.
     */
    _SYS_ATEXIT_STDIO_SLOT,      /* stdio stream flush */
#endif
    _SYS_ATEXIT_SLOTS     /* number of slots */
};

typedef void (*atexit_func_t)(void);
extern atexit_func_t __sys_atexit_funcs[_SYS_ATEXIT_SLOTS];

#endif /* _XATEXIT_H */

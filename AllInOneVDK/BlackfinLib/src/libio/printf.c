/************************************************************************
 *
 * printf.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#if defined(__ADSPBLACKFIN__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=printf")
#pragma file_attr(  "libFunc=_printf")
#else
#pragma file_attr(  "libFunc=_printf64")
#pragma file_attr(  "libFunc=__printf64")
#endif
#else
#if defined(__ADSP21000__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=printf32")
#pragma file_attr(  "libFunc=__printf32")
#else
#pragma file_attr(  "libFunc=printf64")
#pragma file_attr(  "libFunc=__printf64")
#endif
#endif
#endif
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include <stdarg.h>

#include "_stdio.h"

int _theputchar(FargT, char);

int
printf(const char *fmt, ...)
{
    int outsize;
    va_list ap;

    /* return failure if format is NULL pointer */ 
    if (!fmt)
        return -1;

    _LOCK_FILE(stdout);

    va_start(ap,fmt);
    outsize=_doprnt (fmt, ap, _theputchar, stdout);
    va_end(ap);

#if defined(__ADSP21000__)
    // Always flush output on SHARC, where streams aren't flushed at exit.
    _fflush(stdout);
#endif

    _UNLOCK_FILE(stdout);

    return outsize;
}

// end of file

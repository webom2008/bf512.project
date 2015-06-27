/************************************************************************
 *
 * fscanf.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#if defined(__ADSPBLACKFIN__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=fscanf")
#pragma file_attr(  "libFunc=_fscanf")
#else
#pragma file_attr(  "libFunc=_fscanf64")
#pragma file_attr(  "libFunc=__fscanf64")
#endif
#else /* platform not valid */
#if defined(__ADSP21000__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=fscan32")
#pragma file_attr(  "libFunc=__fscan32")
#else
#pragma file_attr(  "libFunc=fscan64")
#pragma file_attr(  "libFunc=__fscan64")
#endif
#endif
#endif
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "stdio.h"
#include "stdarg.h"
#include "_stdio.h"

static int
_thegetchar(int ch, void* str)
{
    return ch == DO_GETC ? fgetc((FILE*)str) : ungetc(ch, (FILE*)str);
}

int
fscanf(FILE* str, const char* fmt, ...)
{
    int ans;
  
    va_list ap;

    if (!str || !fmt)
        return EOF;

    _LOCK_FILE(str);

    va_start(ap, fmt);
    ans = _doscan(fmt, ap, &_thegetchar, str);
    va_end(ap);

    _UNLOCK_FILE(str);

    return ans;
}

// end of file

/************************************************************************
 *
 * fprintf.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#if defined(__ADSPBLACKFIN__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=fprintf")
#pragma file_attr(  "libFunc=_fprintf")
#else
#pragma file_attr(  "libFunc=_fprintf64")
#pragma file_attr(  "libFunc=__fprintf64")
#endif
#else /* platform not valid */
#if defined(__ADSP21000__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=fprintf32")
#pragma file_attr(  "libFunc=__fprintf32")
#else
#pragma file_attr(  "libFunc=fprintf64")
#pragma file_attr(  "libFunc=__fprintf64")
#endif
#endif
#endif
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "_stdio.h"

int _theputchar(FargT, char);

int
fprintf(FILE *str, const char *fmt, ...)
{
    int outsize;
    va_list ap;

    /* return failure if file or format are NULL pointers */
    if (!str || !fmt)
        return -1;

    _LOCK_FILE(str);

    va_start (ap,fmt);
    outsize=_doprnt (fmt, ap, _theputchar, str);
    va_end(ap);

    _UNLOCK_FILE(str);

    return outsize;
}

// end of file

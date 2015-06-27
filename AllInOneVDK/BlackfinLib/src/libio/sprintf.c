/************************************************************************
 *
 * sprintf.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#if defined(__ADSPBLACKFIN__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=sprintf")
#pragma file_attr(  "libFunc=_sprintf")
#else
#pragma file_attr(  "libFunc=_sprintf64")
#pragma file_attr(  "libFunc=__sprintf64")
#endif
#else /* platform not valid */
#if defined(__ADSP21000__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=sprintf32")
#pragma file_attr(  "libFunc=__sprintf32")
#else
#pragma file_attr(  "libFunc=sprintf64")
#pragma file_attr(  "libFunc=__sprintf64")
#endif
#endif
#endif
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "_stdio.h"

int _sprintfputchar(FargT, char);

int
sprintf(char *buf, const char *fmt, ...)
{
    /* No locking required for printing into a string. */
    
    int outsize;
    va_list ap;

    /* return failure if buf or format opinters are NULL */
    if (!buf || !fmt)
      return -1;

    va_start(ap,fmt);
    outsize = _doprnt (fmt, ap, _sprintfputchar, &buf);
    va_end(ap);

    *buf = 0;
    return outsize;
}

// end of file

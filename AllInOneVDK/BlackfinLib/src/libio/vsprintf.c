/************************************************************************
 *
 * vsprintf.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#if defined(__ADSPBLACKFIN__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=vsprintf")
#pragma file_attr(  "libFunc=_vsprintf")
#else
#pragma file_attr(  "libFunc=_vsprintf64")
#pragma file_attr(  "libFunc=__vsprintf64")
#endif
#else /* platform not valid */
#if defined(__ADSP21000__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=vsprintf32")
#pragma file_attr(  "libFunc=__vsprintf32")
#else
#pragma file_attr(  "libFunc=vsprintf64")
#pragma file_attr(  "libFunc=__vsprintf64")
#endif
#endif
#endif
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "_stdio.h"

int _sprintfputchar(FargT, char);

int
vsprintf(char *buf, const char *fmt, va_list ap)
{
    /* No locking required for printing into a string. */

    int outsize;

    /* return failure if any NULL pointers present */
    if (!buf || !fmt || !ap)
      return -1;

    outsize = _doprnt (fmt, ap, _sprintfputchar, &buf);
    *buf = 0;
    return outsize;
}

// end of file

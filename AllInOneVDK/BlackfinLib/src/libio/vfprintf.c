/************************************************************************
 *
 * vfprintf.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#if defined(__ADSPBLACKFIN__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=vfprintf")
#pragma file_attr(  "libFunc=_vfprintf")
#else
#pragma file_attr(  "libFunc=_vfprintf64")
#pragma file_attr(  "libFunc=__vfprintf64")
#endif
#else /* platform not valid */
#if defined(__ADSP21000__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=vfprintf32")
#pragma file_attr(  "libFunc=__vfprintf32")
#else
#pragma file_attr(  "libFunc=vfprintf64")
#pragma file_attr(  "libFunc=__vfprintf64")
#endif
#endif
#endif
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "_stdio.h"

int _theputchar(FargT, char);

int
vfprintf(FILE *str, const char *fmt, char *ap)
{
    int res;

    /* return failure if any NULL pointers present */
    if (!str || !fmt || !ap)
      return -1;

    _LOCK_FILE(str);
    res = _doprnt(fmt, ap, _theputchar, str);
    _UNLOCK_FILE(str);
    return res;
}

// end of file

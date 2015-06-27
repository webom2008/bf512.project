/************************************************************************
 *
 * wcscoll.c: $Revision: 1.8 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#pragma file_attr( "libGroup=wchar.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=wcscoll")
#pragma file_attr(  "libFunc=_wcscoll")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#include <wchar.h>

int
(wcscoll)(const wchar_t *s1, const wchar_t *s2)
{
    /* Trivial implementation as we do not support locales. */
    return wcscmp(s1, s2);
}

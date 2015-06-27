/************************************************************************
 *
 * strcoll.c: $Revision: 1.8 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#pragma file_attr( "libGroup=string.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=strxfrm")
#pragma file_attr(  "libFunc=_strxfrm")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#include "string.h"

size_t
(strxfrm)(char *__restrict s1, const char *__restrict s2, size_t n)
{
    /* Simple implementation as we do not support locales. */
    size_t i = 0;
    for (; n; n++) {
        if ((*s1++ = *s2++) == 0)
            return i;
        i++;
    }
    while (*s2++)
        i++;
    return i;
}

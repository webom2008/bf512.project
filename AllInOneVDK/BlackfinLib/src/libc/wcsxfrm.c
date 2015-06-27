/************************************************************************
 *
 * wcsxfrm.c: $Revision: 1.8 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#pragma file_attr( "libGroup=wchar.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=wcsxfrm")
#pragma file_attr(  "libFunc=_wcsxfrm")
#pragma file_attr(  "libFunc=_Wcsxfrmx")
#pragma file_attr(  "libFunc=__Wcsxfrmx")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#include <wchar.h>

size_t
(wcsxfrm)(wchar_t *_Restrict s1, const wchar_t *_Restrict s2, size_t n)
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

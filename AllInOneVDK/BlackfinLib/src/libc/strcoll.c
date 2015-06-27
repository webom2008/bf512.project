/************************************************************************
 *
 * strcoll.c: $Revision: 1.8 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#pragma file_attr( "libGroup=string.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=strcoll")
#pragma file_attr(  "libFunc=_strcoll")
#pragma file_attr(  "libFunc=strcmp")
#pragma file_attr(  "libFunc=_strcmp")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#include <string.h>

int
(strcoll)(const char *s1, const char *s2)
{
    /* Trivial implementation as we do not support locales. */
    return strcmp(s1, s2);
}

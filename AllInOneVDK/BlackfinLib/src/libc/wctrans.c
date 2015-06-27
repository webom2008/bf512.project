/************************************************************************
 *
 * wctrans.c: $Revision: 1.8 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#pragma file_attr( "libGroup=wctype.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=wctrans")
#pragma file_attr(  "libFunc=_wctrans")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#include <wctype.h>
#include <string.h>

wctrans_t
wctrans(const char *name)
{
    /* Simple implementation as we do not support locales. */
    if (strcmp(name, "tolower") == 0)
      return 1;
    else if (strcmp(name, "toupper") == 0)
      return 2;
    else
      return 0;
}

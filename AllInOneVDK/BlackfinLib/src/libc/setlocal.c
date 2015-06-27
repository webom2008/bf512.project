/************************************************************************
 *
 * setlocal.c: $Revision: 1.8 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#pragma file_attr( "libGroup=locale.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=setlocale")
#pragma file_attr(  "libFunc=_setlocale")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <locale.h>

char *
setlocale(int cat, const char *name) 
{
    if (!name || !name[0] || (name[0] == 'C' && !name[1]))
        return "C";
    else
        return 0;
}

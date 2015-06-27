/************************************************************************
 *
 * puts.c : $Revision: 4 $
 *
 * (c) Copyright 1998-2003 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=puts")
#pragma file_attr(  "libFunc=_puts")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "_stdio.h"

#include "stdio.h"

int
(puts)(const char* s)
{
    return fputs(s, stdout) < 0 || fputc('\n', stdout) < 0 ? EOF : 0;
}

// end of file

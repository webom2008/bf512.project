/************************************************************************
 *
 * putc.c : $Revision: 4 $
 *
 * (c) Copyright 1998-2003 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=putc")
#pragma file_attr(  "libFunc=_putc")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "stdio.h"

#include "_stdio.h"

int
(putc)(int c, FILE* str)
{
  return fputc(c, str);
}

// end of file

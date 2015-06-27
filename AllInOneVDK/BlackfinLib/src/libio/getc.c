/************************************************************************
 *
 * getc.c : $Revision: 4 $
 *
 * (c) Copyright 1998-2003 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=getc")
#pragma file_attr(  "libFunc=_getc")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "_stdio.h"

#include <stdio.h>

int
(getc)(FILE* str)
{
  return fgetc(str);
}

// end of file

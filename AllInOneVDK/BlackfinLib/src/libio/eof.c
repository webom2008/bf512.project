/************************************************************************
 *
 * eof.c : $Revision: 4 $
 *
 * (c) Copyright 1998-2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=feof")
#pragma file_attr(  "libFunc=_feof")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "_stdio.h"

#include "stdio.h"

int
feof(FILE* str)
{
  /* Return 0 for NULL pointer */
  if (!str)
    return 0;

  return str->_Mode & M_EOF;
}

// end of file

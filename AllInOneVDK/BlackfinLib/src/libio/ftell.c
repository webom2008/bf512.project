/************************************************************************
 *
 * ftell.c : $Revision: 3544 $
 *
 * (c) Copyright 2003-2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=ftell")
#pragma file_attr(  "libFunc=_ftell")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>

long ftell(FILE *str)
{
  fpos_t offset;
  int res;

  res = fgetpos(str, &offset);
  if (res)
    return -1L;
  else
#ifdef __ADSPBLACKFIN__
    return offset._Off;
#else
    return offset;
#endif
}

// end of file

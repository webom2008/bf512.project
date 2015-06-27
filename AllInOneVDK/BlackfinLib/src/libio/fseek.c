/************************************************************************
 *
 * fseek.c : $Revision: 3544 $
 *
 * (c) Copyright 2003-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=fseek")
#pragma file_attr(  "libFunc=_fseek")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include "_stdio.h"

int fseek(FILE *str, long offset, int whence)
{
  if (!str)
    return -1;
  _LOCK_FILE(str);
  int res = _doseek (str, offset, whence);
  _UNLOCK_FILE(str);
  return res;
}

// end of file

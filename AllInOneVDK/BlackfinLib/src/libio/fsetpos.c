/************************************************************************
 *
 * fsetpos.c : $Revision: 3544 $
 *
 * (c) Copyright 2003-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=fsetpos")
#pragma file_attr(  "libFunc=_fsetpos")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include "_stdio.h"

int
fsetpos(FILE *str, const fpos_t *pos)
{
  int res;

  if (!str || !pos)
    return -1;

  _LOCK_FILE(str);

#ifdef __ADSPBLACKFIN__
  res = _doseek (str, pos->_Off,  SEEK_SET);
#else
  res = _doseek (str, *pos,  SEEK_SET);
#endif

  _UNLOCK_FILE(str);
  return res;
}

// end of file

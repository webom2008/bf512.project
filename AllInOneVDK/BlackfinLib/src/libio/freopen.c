/************************************************************************
 *
 * freopen.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=freopen")
#pragma file_attr(  "libFunc=_freopen")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include <stdlib.h>

#include "_stdio.h"

FILE*
freopen(const char* name, const char* mods, FILE* str)
{
  FILE* fptr;
  unsigned int mode;

  if (!mods || !str)
    return NULL;

  _LOCK_FILE(str);

  mode = str->_Mode & M_FREE_FILE;
  str->_Mode &= ~M_FREE_FILE;
  _fclose(str);
  str->_Mode = mode;
  fptr = _doopenfile(name, mods, str);

  _UNLOCK_FILE(str);

  return fptr;

}

// end of file

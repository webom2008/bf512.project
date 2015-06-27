/************************************************************************
 *
 * fopen.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=fopen")
#pragma file_attr(  "libFunc=_fopen")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "_stdio.h"

#include <stdio.h>
#include <stdlib.h>

FILE*
fopen(const char* name, const char* mods)
{
  FILE* str = NULL;
  FILE* fptr;
  size_t  i;

  if (!name || !mods)
    return NULL;

  _LOCK_FILES();

  for (i = 0; i < FOPEN_MAX; i++)
  {
    if (_Files[i] == NULL)
    {
      str = (FILE*)malloc(sizeof(FILE));
      if (str == NULL)
        break;
#if _FILE_OP_LOCKS
      if (!_INIT_MUTEX((MUTEX *)&str->_Mutex, true)) {
        free(str);
        str = NULL;
        break;
      }
#endif
      _Files[i] = str;
      str->_Mode = M_FREE_FILE;
      break;
    }
    else if (_Files[i]->_Mode == 0)
    {
      str = _Files[i];
      break;
    }
  }

  if (str)
    fptr = _doopenfile(name, mods, str);
  else
    fptr = NULL;

  _UNLOCK_FILES();
  return fptr;
}

// end of file

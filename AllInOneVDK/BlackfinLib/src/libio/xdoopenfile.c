/************************************************************************
 *
 * xdoopenfile.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2008 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=_doopenfile")
#pragma file_attr(  "libFunc=__doopenfile")
#pragma file_attr(  "libFunc=fopen")
#pragma file_attr(  "libFunc=_fopen")
#pragma file_attr(  "libFunc=freopen")
#pragma file_attr(  "libFunc=_freopen")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include <stdlib.h>
#include <device_int.h>

#include "_stdio.h"

FILE*
_doopenfile(const char* name, const char* mods, FILE* str)
{

  int mode;          /* dev_open mode */

  str->fileID = -1;
  /*str->tmpnam = NULL;*/
  str->bufadr = NULL;
  str->nback = 0;
  str->_Mode = (str->_Mode & M_FREE_FILE) |
      (*mods == 'r' ? M_OPENR :
       *mods == 'w' ? (M_OPENW | M_CREATE | M_TRUNCATE) :
       *mods == 'a' ? (M_OPENW | M_CREATE | M_OPENA) :
       0);

  if ((str->_Mode & M_OPENRW) == 0)
  {
    _fclose(str);
    return NULL;
  }

  while ((*++mods == 'b') || (*mods == '+'))
  {
    if (*mods == 'b')
    {
      if (str->_Mode & M_BINARY)
        break;
      str->_Mode |= M_BINARY;
    } 
    else
    {
      if ((str->_Mode & M_OPENRW) == M_OPENRW)
        break;
      str->_Mode |= M_OPENRW;
    }
  }

  mode = str->_Mode;

#if defined(__ADSPBLACKFIN__)
  static const int map_rw_access[4] = {
    0, _dev_rdonly, _dev_wronly, _dev_rdwr
  };

  int new_mode = map_rw_access[mode & M_OPENRW];

  if (mode & M_OPENA)
    new_mode |= _dev_append;
  if (mode & M_TRUNCATE)
    new_mode |= _dev_truncate;
  if (mode & M_CREATE)
    new_mode |= _dev_create;
  if (mode & M_BINARY)
    new_mode |= _dev_binary;
  else
    new_mode |= _dev_text;
    
  mode = new_mode;
#endif

  str->fileID = _dev_open(__default_io_device,(char *)name,mode);

  if (str->fileID < 0)
  {
    _fclose(str);
    return NULL;
  }

#if (BYTES_PER_WORD==1)
  str->_Next = &str->onechar;
#else
  str->_Next = (str->_Mode & M_BINARY) ? (byte_addr_t) &str->onechar
                                       : BYTE_ADDR(&str->onechar);
#endif
  str->_Buf = str->_Next;
  str->_Rend = str->_Wend = str->_Next;
  return str;
}

// end of file

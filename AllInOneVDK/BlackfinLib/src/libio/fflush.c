/************************************************************************
 *
 * fflush.c : $Revision: 3543 $
 *
 * (c) Copyright 1998-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=fflush")
#pragma file_attr(  "libFunc=_fflush")
#pragma file_attr(  "libFunc=__fflush")
#pragma file_attr(  "libFunc=_fflush_all")
#pragma file_attr(  "libFunc=__fflush_all")
#pragma file_attr(  "libFunc=fclose")
#pragma file_attr(  "libFunc=_fclose")
#pragma file_attr(  "libFunc=fputc")
#pragma file_attr(  "libFunc=_fputc")
#pragma file_attr(  "libFunc=fputs")
#pragma file_attr(  "libFunc=_fputs")
#pragma file_attr(  "libFunc=fsetpos")
#pragma file_attr(  "libFunc=_fsetpos")
#pragma file_attr(  "libFunc=fwrite")
#pragma file_attr(  "libFunc=_fwrite")
#pragma file_attr(  "libFunc=printf")
#pragma file_attr(  "libFunc=_printf")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include <stdlib.h>

#include "_stdio.h"
#include "device_int.h"

int
fflush (FILE* str)
{
  if (str == NULL)
    return _fflush_all();
  else {
    int res;
    _LOCK_FILE(str);
    res = _fflush(str);
    _UNLOCK_FILE(str);
    return res;
  }
}

int
_fflush_all(void)
{
  int res = 0;
  int i;
  _LOCK_FILES();
  for (i = 0; i < FOPEN_MAX; i++) {
    if (_Files[i]) {
      if (fflush(_Files[i]) < 0)
        res = EOF;
    }
  }
  _UNLOCK_FILES();
  return res;
}

int
_fflush (FILE* str)
{
  int n;
  byte_addr_t s;

  if (!(str->_Mode & M_LAST_WRITE))
    return 0;

  if ((BYTES_PER_WORD!=1) && (str->_Mode & M_BINARY)) {
    for (s = str->_Buf; s < str->_Next; s += (n/BYTES_PER_WORD))
    {
      n = _dev_write(str->fileID,
                         (unsigned char*) s,
                         (str->_Next - s) * BYTES_PER_WORD);
      if (n <= 0)
      {
        str->_Next = str->_Wend = str->_Buf;
        str->_Mode |= M_ERROR;
        return EOF;
      }
    }

    str->_Wend = str->_Next = str->_Buf;

  } else {

    for (s = str->_Buf; s < str->_Next; s += n)
    {
      n = _dev_write(str->fileID,
                          (unsigned char*) WORD_ADDR(s),
                          str->_Next - s);
      if (n <= 0)
      {
        str->_Next = str->_Wend = str->_Buf;
        str->_Mode |= M_ERROR;
        return EOF;
      }
    }

    str->_Wend = str->_Next = str->_Buf;

  }

  str->_Mode &= ~M_LAST_WRITE;
  return 0;

} /* fflush */

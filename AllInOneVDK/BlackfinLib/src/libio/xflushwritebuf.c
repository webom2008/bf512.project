/************************************************************************
 *
 * xflushwritebuf.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=_flushwritebuf")
#pragma file_attr(  "libFunc=__flushwritebuf")
#pragma file_attr(  "libFunc=fputc")
#pragma file_attr(  "libFunc=_fputc")
#pragma file_attr(  "libFunc=fputs")
#pragma file_attr(  "libFunc=_fputs")
#pragma file_attr(  "libFunc=fwrite")
#pragma file_attr(  "libFunc=_fwrite")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_stdio.h"
#include "xatexit.h"

int
_flushwritebuf(FILE* str)
{
  int res;

  if (str->_Next < str->_Wend)
    return 0;
  else if (str->_Mode & M_LAST_WRITE) {
    if ((res = _fflush(str)) < 0)
      return res;
  }
  else if ((str->_Mode & (M_OPENW | M_LAST_READ)) != M_OPENW)
  {
    str->_Mode = M_ERROR;
    return -1;
  }

  if (!str->bufadr)
  {
    if ((str->bufadr = (unsigned char*)malloc(BUFSIZ)) == NULL)
    {
      str->bufadr = &str->onechar;
      str->_Bend = str->bufadr + 1;
    }
    else
    {
      str->_Mode |= M_FREE_BUFFER;
      str->_Bend = str->bufadr + BUFSIZ;
    }

    str->_Buf = (str->_Mode & M_BINARY) ? (byte_addr_t) str->bufadr
                                        : BYTE_ADDR(str->bufadr);
  }

  str->_Next = str->_Rend = str->_Buf;
  str->_Wend = (str->_Mode & M_BINARY) ? (byte_addr_t) str->_Bend
                                       : BYTE_ADDR(str->_Bend);
  str->_Mode |= M_LAST_WRITE;

#if !defined(__ADSP21000__)
  // Make sure any buffered data is flushed at exit.
  // Due to code space considerations, this is not done on SHARC,
  // where printf() is flushed every time to compensate for this.
    __sys_atexit_funcs[_SYS_ATEXIT_STDIO_SLOT] = (atexit_func_t)_fflush_all;
#endif

  return 0;

} /* _flushwritebuf */

// end of file

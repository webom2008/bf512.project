/************************************************************************
 *
 * setvbuf.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=setvbuf")
#pragma file_attr(  "libFunc=_setvbuf")
#pragma file_attr(  "libFunc=setbuf")
#pragma file_attr(  "libFunc=_setbuf")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include <stdlib.h>
#include "_stdio.h"

int
setvbuf(FILE* str, char* cbuf, int smode, size_t sz)
{
  int mode;
  unsigned char* ucbuf = (unsigned char*) cbuf;

  if (!str)
    return -1;  /* NULL stream pointer - return failure */

  if (str->_Mode & (M_LAST_READ | M_LAST_WRITE))
    return -1;  /* Can't change the buffer after a read or write */

  _LOCK_FILE(str);

  mode =
    smode == _IOFBF ? 0 :
    smode == _IOLBF ? M_LINE_BUFFERING :
    smode == _IONBF ? M_NO_BUFFERING :
      -1;

  if (mode == -1)
  {
    _UNLOCK_FILE(str);
    return -1;
  }

  if (sz == 0)
  {
    ucbuf = &str->onechar;
    sz = 1;
  }

  if (ucbuf == NULL)
  {
    if ((ucbuf = (unsigned char*)malloc(sz)) == NULL)
    {
      _UNLOCK_FILE(str);
      return -1;
    }
    else
      mode |= M_FREE_BUFFER;
  }

  if (str->_Mode & M_FREE_BUFFER)
  {
    free(str->bufadr);
    str->_Mode &= ~M_FREE_BUFFER;
  }

  str->_Mode = (str->_Mode & ~(M_LINE_BUFFERING | M_NO_BUFFERING)) | mode;
  str->_Bend = ucbuf + sz;
  str->bufadr = ucbuf;

  str->_Buf = (str->_Mode & M_BINARY) ? (byte_addr_t) str->bufadr
                                    : BYTE_ADDR(str->bufadr);

  str->_Next = str->_Rend = str->_Wend = str->_Buf;

  _UNLOCK_FILE(str);
  return 0;
}

// end of file

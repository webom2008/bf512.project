/************************************************************************
 *
 * fputs.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=fputs")
#pragma file_attr(  "libFunc=_fputs")
#pragma file_attr(  "libFunc=puts")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include <string.h>

#include "_stdio.h"

int
(fputs)(const char* s, FILE* str)
{

  char linebuf; 

#if (BYTES_PER_WORD!=1)
  char binary;
#endif

  /* Return failure if NULL pointer present */
  if (!s || !str)
    return EOF;

  linebuf = (str->_Mode & M_LINE_BUFFERING);

#if (BYTES_PER_WORD!=1)
  binary = (str->_Mode & M_BINARY);
#endif

  _LOCK_FILE(str);

  while (*s != 0)
  {
    if (str->_Next >= str->_Wend)
      if (_flushwritebuf(str) < 0) {
        _UNLOCK_FILE(str);
        return EOF;
      }

#if (BYTES_PER_WORD==1)
    *((unsigned char*) (str->_Next++)) = *s;
#else
    if (binary)
      *((unsigned char*) (str->_Next++)) = *s;
    else
    {
      DEPOSIT_BYTE(*WORD_ADDR(str->_Next), str->_Next, *s);
      str->_Next++;
    }
#endif

    str->_Mode |= M_LAST_WRITE;

    if (linebuf && (*s == '\n'))
      if (_fflush(str))
      {
        _UNLOCK_FILE(str);
        return EOF;
      }
    s++;
  }

  if (str->_Mode & M_NO_BUFFERING)
  {
    if (_fflush(str)) {
      _UNLOCK_FILE(str);
      return EOF;
    }
  }
  
  _UNLOCK_FILE(str);
  return 0;
}

// end of file

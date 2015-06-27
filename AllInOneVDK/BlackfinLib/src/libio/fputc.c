/************************************************************************
 *
 * fputc.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=fputc")
#pragma file_attr(  "libFunc=_fputc")
#pragma file_attr(  "libFunc=__fputc")
#pragma file_attr(  "libFunc=putchar")
#pragma file_attr(  "libFunc=_putchar")
#pragma file_attr(  "libFunc=putc")
#pragma file_attr(  "libFunc=_putc")
#pragma file_attr(  "libFunc=puts")
#pragma file_attr(  "libFunc=_puts")
#pragma file_attr(  "libFunc=snprintf")
#pragma file_attr(  "libFunc=_snprintf")
#pragma file_attr(  "libFunc=sprintf")
#pragma file_attr(  "libFunc=_sprintf")
#pragma file_attr(  "libFunc=vsnprintf")
#pragma file_attr(  "libFunc=_vsnprintf")
#pragma file_attr(  "libFunc=vsprintf")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>

#include "_stdio.h"

/*wrapper function*/
int
fputc(int c, FILE* str)
{
  _LOCK_FILE(str);
  int tmp = _fputc(c,str);
  _UNLOCK_FILE(str);
  return tmp;
}


/*actual implementation*/
int
_fputc(int c, FILE* str)
{
  unsigned char ch = c;

  if (!str)
    return EOF;

  if (str->_Next >= str->_Wend)
  {
    if (_flushwritebuf(str) < 0)
      return EOF;
  }

#if (BYTES_PER_WORD==1)
  *((unsigned char*) str->_Next++) = ch;
#else

  if (str->_Mode & M_BINARY)
    *((unsigned char*) str->_Next++) = ch;
  else
  {
    DEPOSIT_BYTE(*WORD_ADDR(str->_Next), str->_Next, ch);
    str->_Next++;
  }

#endif

  str->_Mode |= M_LAST_WRITE;

  if (str->_Mode & (M_LINE_BUFFERING | M_NO_BUFFERING))
  {
    if ((str->_Mode & M_NO_BUFFERING) || (ch == '\n'))
      if (_fflush(str))
        return EOF;
  }

  return ch;
}

// end of file

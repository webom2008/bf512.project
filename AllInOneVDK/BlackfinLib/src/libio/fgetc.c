/************************************************************************
 *
 * fgetc.c : $Revision: 3543 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=fgetc")
#pragma file_attr(  "libFunc=_fgetc")
#pragma file_attr(  "libFunc=fread")
#pragma file_attr(  "libFunc=_fread")
#pragma file_attr(  "libFunc=fscanf")
#pragma file_attr(  "libFunc=_fscanf")
#pragma file_attr(  "libFunc=getc")
#pragma file_attr(  "libFunc=_getc")
#pragma file_attr(  "libFunc=getchar")
#pragma file_attr(  "libFunc=_getchar")
#pragma file_attr(  "libFunc=scanf")
#pragma file_attr(  "libFunc=_scanf")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>

#include "_stdio.h"

int
fgetc(FILE* str)
{
  unsigned char c0;

  /* Return failure if FILE pointer is NULL */
  if (!str)
    return EOF;

  _LOCK_FILE(str);

  if (str->nback > 0) {
    if (!--str->nback)
      str->_Rend = str->rsave;
    _UNLOCK_FILE(str);
    return str->_Back[str->nback];
  }
  else if (str->_Next >= str->_Rend && _fillreadbuf(str) <= 0) {
    _UNLOCK_FILE(str);
    return EOF;
  }

#if (BYTES_PER_WORD==1)
  c0 = *((unsigned char*) str->_Next++);
#else

  if (str->_Mode & M_BINARY) {
    c0 = *((unsigned char*) str->_Next++);
  } else {
    c0 = FETCH_BYTE(*WORD_ADDR(str->_Next), str->_Next);
    str->_Next++;
  }
#endif

  _UNLOCK_FILE(str);

  return c0;
}

// end of file

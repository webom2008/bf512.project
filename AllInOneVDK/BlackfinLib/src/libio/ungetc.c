/************************************************************************
 *
 * ungetc.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=ungetc")
#pragma file_attr(  "libFunc=_ungetc")
#pragma file_attr(  "libFunc=fscanf")
#pragma file_attr(  "libFunc=_fscanf")
#pragma file_attr(  "libFunc=scanf")
#pragma file_attr(  "libFunc=_scanf")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "_stdio.h"

#include <stdio.h>
#include <stdlib.h>

int
ungetc(int c, FILE* str)
{
  /* Return failure if FILE pointer is NULL */
  if (!str)
    return EOF;

  _LOCK_FILE(str);

  if ((c == EOF) || (c == READ_LIMIT) ||
      sizeof(str->_Back) <= str->nback  ||
      (str->_Mode & (M_OPENR | M_LAST_WRITE)) != M_OPENR)
  {
    _UNLOCK_FILE(str);
    return EOF;
  }

  str->_Mode = str->_Mode & ~M_EOF | M_LAST_READ;

  if (str->nback == 0)
  {
    str->rsave = str->_Rend;
    str->_Rend = str->_Buf;
  }

  str->_Back[str->nback++] = c;

  _UNLOCK_FILE(str);

  return (unsigned char) c;
}
  
// end of file

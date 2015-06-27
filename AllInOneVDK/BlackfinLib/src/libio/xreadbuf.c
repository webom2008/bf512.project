/************************************************************************
 *
 * xreadbuf.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")

#pragma file_attr(  "libFunc=_fillreadbuf")
#pragma file_attr(  "libFunc=__fillreadbuf")
#pragma file_attr(  "libFunc=fread")
#pragma file_attr(  "libFunc=_fread")
#pragma file_attr(  "libFunc=fgetc")
#pragma file_attr(  "libFunc=_fgetc")
#pragma file_attr(  "libFunc=fgets")
#pragma file_attr(  "libFunc=_fgets")
#pragma file_attr(  "libFunc=fscanf")
#pragma file_attr(  "libFunc=_fscanf")
#pragma file_attr(  "libFunc=scanf")
#pragma file_attr(  "libFunc=_scanf")

#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "_stdio.h"
#include "device_int.h"

int
_fillreadbuf(FILE* str)
{
  int n;

  if (str->_Next < str->_Rend)
    return 1;
  else if (str->_Mode & M_EOF)
    return 0;
  else if ((str->_Mode & (M_OPENR | M_LAST_WRITE)) != M_OPENR)
  {
    str->_Mode |= M_ERROR;
    return -1;
  }

  if (str->bufadr == NULL)
  {
    if ((str->bufadr = malloc(BUFSIZ)) == NULL)
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

  str->_Next = str->_Rend = str->_Wend = str->_Buf;

  /* Returns number of bytes read */
  n = _dev_read(str->fileID,
                     (char *) str->bufadr,
                     BYTES_PER_WORD * (str->_Bend - str->bufadr));
  if (n < 0)
  {
    str->_Mode |= M_ERROR;
    return -1;
  }
  else if (n == 0)
  {
    str->_Mode = (str->_Mode & ~M_LAST_READ) | M_EOF;
    return 0;
  }
  else
  {
    if (str->_Mode & M_BINARY)
    {

#if (BYTES_PER_WORD != 1)

      unsigned char *const ptr = &(str->bufadr[(n-1)/BYTES_PER_WORD]);
      int                  rem = n & (BYTES_PER_WORD-1);

      if (rem != 0)
      {
        *ptr &= (UCHAR_MAX >> ((BYTES_PER_WORD - rem) * 8));
             // organize padding of last incomplete word
      }

#endif

      n = (n + (BYTES_PER_WORD-1)) / BYTES_PER_WORD;
    }
    str->_Mode |= M_LAST_READ;
    str->_Rend += n;
    return 1;
  }
}

// end of file

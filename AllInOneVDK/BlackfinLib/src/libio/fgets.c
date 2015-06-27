/************************************************************************
 *
 * fgets.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=fgets")
#pragma file_attr(  "libFunc=_fgets")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include <string.h>

#include "_stdio.h"

char*
fgets(char* buf, int n, FILE* str)
{
  unsigned char* s = (unsigned char*) buf;
  unsigned char  c;

#if (BYTES_PER_WORD!=1)
  unsigned char binary;
#endif

  if (!str || (n <= 1))
    return NULL;

#if (BYTES_PER_WORD!=1)
  binary = (str->_Mode & M_BINARY);
#endif

  _LOCK_FILE(str);

  while (--n > 0 && str->nback)
  {
    *s = str->_Back[--str->nback];
    if (!str->nback)
      str->_Rend = str->rsave;

    if (*s++ == '\n')
    {
      *s = '\0';

      _UNLOCK_FILE(str);
      return buf;
    }
  }

  while (n > 0)
  {
    if (str->_Next >= str->_Rend)
    {
      if (_fillreadbuf(str) < 0)
      {
        _UNLOCK_FILE(str);
        return NULL;
      } else if (str->_Mode & M_EOF)
        break;
    }

#if (BYTES_PER_WORD==1)
    c = *((unsigned char*) str->_Next++);
#else
    if (binary) {
      c = *((unsigned char*) str->_Next++);
    } else {
      c = FETCH_BYTE(*WORD_ADDR(str->_Next), str->_Next);
      str->_Next++;
    }
#endif

    if ((*(s++) = c) == '\n')
    {
      *s = '\0';

      _UNLOCK_FILE(str);
      return buf;
    }

    n--;
  }

  /* EOF processing here */
  if (s == (unsigned char*) buf)
  {
    _UNLOCK_FILE(str);
    return NULL;
  } else {
    *s = '\0';

    _UNLOCK_FILE(str);
    return buf;
  }
}

// end of file

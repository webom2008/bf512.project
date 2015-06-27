/************************************************************************
 *
 * sscanf.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#if defined(__ADSPBLACKFIN__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=sscanf")
#pragma file_attr(  "libFunc=_sscanf")
#else
#pragma file_attr(  "libFunc=_sscanf64")
#pragma file_attr(  "libFunc=__sscanf64")
#endif
#else /* platform not valid */
#if defined(__ADSP21000__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=sscanf32")
#pragma file_attr(  "libFunc=__sscanf32")
#else
#pragma file_attr(  "libFunc=sscanf64")
#pragma file_attr(  "libFunc=__sscanf64")
#endif
#endif
#endif
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include <stdarg.h>
#include "_stdio.h"


static int
_thegetstringchar(int ch, void* string)
{
  const char** s = (const char**) string;

  if (ch < 0)
  {
    if ((ch == EOF) || (ch == READ_LIMIT) || (**s == '\0'))
      return EOF;
    else 
      return *(*s)++;
  }
  else
    --(*s);

  return ch;
}

int
sscanf(const char* buf, const char* fmt, ...)
{
  /* No locking required for scanning a string. */

  int ans;
  va_list ap;

  /* return failure if buf or format pointers are NULL */
  if (!buf || !fmt)
    return -1;

  va_start(ap, fmt);
  ans = _doscan(fmt, ap, &_thegetstringchar, &buf);
  va_end(ap);

  return ans;
}


// end of file

/************************************************************************
 *
 * xlonglong_to_string.c : $Revision: 3544 $
 *
 * (c) Copyright 2003 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=_long_long_to_string")
#pragma file_attr(  "libFunc=__long_long_to_string")
#pragma file_attr(  "libFunc=printf")
#pragma file_attr(  "libFunc=_printf")
#pragma file_attr(  "libFunc=sprintf")
#pragma file_attr(  "libFunc=_sprintf")
#pragma file_attr(  "libFunc=fprintf")
#pragma file_attr(  "libFunc=_fprintf")
#pragma file_attr(  "libFunc=fputs")
#pragma file_attr(  "libFunc=_fputs")
#pragma file_attr(  "libFunc=fputc")
#pragma file_attr(  "libFunc=_fputc")
#pragma file_attr(  "libFunc=puts")
#pragma file_attr(  "libFunc=_puts")
#pragma file_attr(  "libFunc=putc")
#pragma file_attr(  "libFunc=_putc")
#pragma file_attr(  "libFunc=putchar")
#pragma file_attr(  "libFunc=_putchar")
#pragma file_attr(  "libFunc=vsprintf")
#pragma file_attr(  "libFunc=_vsprintf")
#pragma file_attr(  "libFunc=vsnprintf")
#pragma file_attr(  "libFunc=_vsnprintf")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#if _LONG_LONG
#include "xprnt.h"

char*
_long_long_to_string(long long val, int hradix, Bool upcase, char* buf, int buflen)
{
  char* bp = buf + buflen;
  *--bp = '\0';

  if ( val == 0 ) {
    *--bp = '0';
  } else {

    int lowbit;
    const char* table = upcase ? __hex_digits_upper : __hex_digits_lower;

    while (val)
    {
      lowbit = val & 1;
      val = (val >> 1) & ~LLHIBIT;
      *--bp = table[val % hradix * 2 + lowbit];
      val = val / hradix;
    }
  }

  return bp;
}
#endif
    
// end of file

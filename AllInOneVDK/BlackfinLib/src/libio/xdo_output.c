/************************************************************************
 *
 * xdo_output.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=_do_output")
#pragma file_attr(  "libFunc=__do_output")
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

#include "xprnt.h"
#include <string.h>

#if defined(__ADSPBLACKFIN__)
#include <math.h>
#else
#include <stdlib.h>
#endif

int
_do_output(FormatT* format, FuncT func, FargT farg)
{
  int outcount;

  int prefix_len;
  int buffer_len = strlen(format->buffer);
  int suffix_len;
  int nblank=0;

  if (format->fcode=='s') {
    prefix_len = 0;
    suffix_len = 0;

    if ( format->precision>=0 )
      buffer_len = min(buffer_len,format->precision);

    nblank = format->width - buffer_len;

  } else {

    prefix_len = strlen(format->prefix);
    suffix_len = strlen(format->suffix);

    if (format->rzero < 0)
      format->rzero = 0;
    if (format->lzero < 0)
      format->lzero = 0;

    if ( format->width )
      nblank = format->width
              - format->rzero
              - suffix_len
              - buffer_len
              - format->lzero
              - prefix_len
              - format->odo
              - (format->odoFlag == kNone ? 0 : 1);
  }

  if ( nblank<0 )
    nblank = 0;

  outcount=nblank+prefix_len+buffer_len+format->rzero+format->lzero+suffix_len;

  if (format->bZero && nblank>0)
  {
    format->lzero += nblank;
    nblank=0;
  }

  if (!format->bLeftJust && nblank>0 )
    _pad(func, farg, ' ', nblank);

  if ( prefix_len>0 ) 
    _put_string(func, farg, format->prefix, prefix_len);

  if ( format->lzero>0 ) 
    _pad(func, farg, '0', format->lzero);

  if (format->odoFlag == kLeft)
  {
    (*func)(farg, '.');
    _pad(func, farg, '0', format->odo);
    outcount+=(format->odo+1);
  }

  _put_string(func, farg, format->buffer, buffer_len);

  if (format->odoFlag == kRight)
  {
    _pad(func, farg, '0', format->odo);
    (*func)(farg, '.');
    outcount+=(format->odo+1);
  }

  if ( format->rzero>0 ) 
    _pad(func, farg, '0', format->rzero);

  if ( suffix_len>0 )
    _put_string(func, farg, format->suffix, suffix_len);

  if (format->bLeftJust && nblank>0)
    _pad(func, farg, ' ', nblank);

  return(outcount);
}

// end of file

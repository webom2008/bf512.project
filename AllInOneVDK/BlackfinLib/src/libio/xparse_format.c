/************************************************************************
 *
 * parse_format.c : $Revision: 5128 $
 *
 * (c) Copyright 1998-2011 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")

#pragma file_attr(  "libFunc=_parse_format")
#pragma file_attr(  "libFunc=__parse_format")
#pragma file_attr(  "libFunc=printf")
#pragma file_attr(  "libFunc=_printf")
#pragma file_attr(  "libFunc=sprintf")
#pragma file_attr(  "libFunc=_sprintf")
#pragma file_attr(  "libFunc=fprintf")
#pragma file_attr(  "libFunc=_fprintf")
#pragma file_attr(  "libFunc=vsprintf")
#pragma file_attr(  "libFunc=_vsprintf")
#pragma file_attr(  "libFunc=vsnprintf")
#pragma file_attr(  "libFunc=_vsnprintf")

#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")
#endif

#include "xprnt.h"

#include <string.h>

const char*
_parse_format(const char* fp, FormatT* format)
{
  Bool done = false;

  format->bPlus = format->bLeftJust = 
  format->bBlank = format->bSharp = 
  format->bZero = false;

  format->prefix = format->suffix = "";
  format->modFlag = ModFlag_None;
  format->odoFlag = kNone;
  format->odo = 0;

  format->lzero = format->rzero = 0;

  while (!done)
  {
    done = true;
    switch (*fp++)
    {
      case '+':
        format->bPlus = true;
        done = false; 
        break;
      case '-':
        format->bLeftJust = true;
        done = false;
        break;
      case ' ':
        format->bBlank = true;
        done = false;
        break;
      case '#':
        format->bSharp = true;
        done = false;
        break;
      case '0':
        format->bZero = true;
        done = false;
        break;
      default:
        fp--;
    }
  }

  fp = _parse_field(fp, format, &format->width);

  if (format->width < 0) {
    /* "A negative field width argument is taken as a - flag followed
       by a positive field width." */
    format->bLeftJust = true;
    format->width = -format->width;
  }
    
  if (*fp == '.')
    fp = _parse_field(fp+1, format, &format->precision);
  else
    format->precision = -1;

  switch (*fp)
  {
      case 'l':    /* "ell" */
            fp++;
#if _LONG_LONG
            if (*fp == 'l') { /* i.e. we have a long long */
               format->modFlag = ModFlag_ll;
               fp++;
            } else
#endif
               format->modFlag = ModFlag_l;
            break;
      case 'L':    /* "ell" */
            format->modFlag = ModFlag_L;
            fp++;
            break;
      case 'h':
            format->modFlag = ModFlag_h;
            fp++;
  }

  format->fcode = *fp++;

  if (format->fcode == 'D')
  {
    format->fcode = 'd';
    format->modFlag = ModFlag_l;
  }

  /*  The standard explicitly specifies how to handle certain conflicts */

  if (format->bPlus) {
    /* if the space and the + flags are used the space flag is ignored */
    format->bBlank = false;
  }
  if (format->bLeftJust) {
    /* if the 0 and - flags appear, 0 is ignored */
    format->bZero =  false;
  }
  if (format->precision >= 0 && strchr("diouxX", format->fcode) != NULL) {
    /* for d, i, o, u, x and X if a precision is specified, the 0 flag 
    ** is ignored
    */
    format->bZero = false;
  }

  return fp;
}

// end of file

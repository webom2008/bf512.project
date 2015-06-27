/************************************************************************
 *
 * xparse_field.c : $Revision: 4 $
 *
 * (c) Copyright 1998-2003 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=_parse_field")
#pragma file_attr(  "libFunc=__parse_field")
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

#include "xprnt.h"

const char*
_parse_field(const char* fp, FormatT* format, int* field)
{
  if (*fp == '*')
  {
    *field = FETCH(format, int);
    fp++;
  }
  else
  {
    *field = 0;
    while (isdigit(*fp))
        *field = *field * 10 + tonumber(*fp++);
  }
  return fp;
}

// end of file

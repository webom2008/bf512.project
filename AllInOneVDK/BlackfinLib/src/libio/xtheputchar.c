/************************************************************************
 *
 * xtheputchar.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=_theputchar")
#pragma file_attr(  "libFunc=__theputchar")
#pragma file_attr(  "libFunc=fprintf")
#pragma file_attr(  "libFunc=_fprintf")
#pragma file_attr(  "libFunc=printf")
#pragma file_attr(  "libFunc=_printf")
#pragma file_attr(  "libFunc=vfprintf")
#pragma file_attr(  "libFunc=_vfprintf")
#pragma file_attr(  "libFunc=vprintf")
#pragma file_attr(  "libFunc=_vprintf")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "_stdio.h"

extern int _fputc(int c, FILE* str);

int _theputchar(FargT str, char c)
{
    return _fputc(c, (FILE*)str);
}

// end of file

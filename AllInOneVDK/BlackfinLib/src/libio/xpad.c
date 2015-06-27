/************************************************************************
 *
 * xpad.c : $Revision: 4 $
 *
 * (c) Copyright 1998-2003 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=_pad")
#pragma file_attr(  "libFunc=__pad")
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

void
_pad(FuncT func, FargT farg, char padchar, int count)
{
    for (; count > 0; count--)
    (*func)(farg, padchar);
}

// end of file

/************************************************************************
 *
 * xhexdigits.c : $Revision: 1.3 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 * Constants used for int -> hex conversion.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#if defined(__ADSPBLACKFIN__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=_doprnt_32")
#pragma file_attr(  "libFunc=__doprnt_32")
#else
#pragma file_attr(  "libFunc=__doprnt_64")
#pragma file_attr(  "libFunc=___doprnt_64")
#endif
#else
#if defined(__ADSP21000__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=_doprnt_32")
#pragma file_attr(  "libFunc=__doprnt_32")
#else
#pragma file_attr(  "libFunc=_doprnt_64")
#pragma file_attr(  "libFunc=__doprnt_64")
#endif
#endif
#endif
#pragma file_attr(  "libGroup=stdio.h")
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

const char __hex_digits_lower[] = "0123456789abcdef";
const char __hex_digits_upper[] = "0123456789ABCDEF";

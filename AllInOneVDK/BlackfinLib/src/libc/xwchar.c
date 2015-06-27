/****************************************************************************
 *
 * xwchar.c : $Revision: 1.4.24.1 $
 *
 * Thread-local data for the wchar conversion functions.
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ****************************************************************************/

#pragma file_attr("libGroup=wchar.h")
#pragma file_attr("libName=libc")
#pragma file_attr("libFunc=mbrlen")
#pragma file_attr("libFunc=mbtowc")
#pragma file_attr("libFunc=mbrtowc")
#pragma file_attr("libFunc=mbsrtowcs")
#pragma file_attr("libFunc=wctomb")
#pragma file_attr("libFunc=wcrtomb")
#pragma file_attr("libFunc=wcsrtombs")
#pragma file_attr("prefersMem=any")
#pragma file_attr("prefersMemNum=50")

#include "xwchar.h"

_TLV_DEF(__wchar_tlv_index, __mbstates_t, __mbstates);

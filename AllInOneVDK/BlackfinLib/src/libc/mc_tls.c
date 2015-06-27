/****************************************************************************
 *
 * mc_tls.c : $Revision: 1.9.14.4 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 * Thread-local storage for the runtime libraries
 * in the thread-per-core model.
 *
 ****************************************************************************/

#pragma file_attr("libName=libc")
#pragma file_attr("libFunc=adi_rtl_get_tls_ptr")
#pragma file_attr("libFunc=_adi_rtl_get_tls_ptr")
#pragma file_attr("libFunc=strtok")
#pragma file_attr("libFunc=_strtok")
#pragma file_attr("libFunc=asctime")
#pragma file_attr("libFunc=_asctime")
#pragma file_attr("libFunc=ctime")
#pragma file_attr("libFunc=_ctime")
#pragma file_attr("libFunc=mktime")
#pragma file_attr("libFunc=_mktime")
#pragma file_attr("libFunc=mktime")
#pragma file_attr("libFunc=_mktime")
#pragma file_attr("libFunc=gmtime")
#pragma file_attr("libFunc=_gmtime")
#pragma file_attr("libFunc=localtime")
#pragma file_attr("libFunc=_localtime")
#pragma file_attr("libFunc=rand")
#pragma file_attr("libFunc=_rand")
#pragma file_attr("libFunc=srand")
#pragma file_attr("libFunc=_srand")
#if !defined(__ADSP21000__)
#pragma file_attr("libFunc=strerror")
#pragma file_attr("libFunc=_strerror")
#pragma file_attr("libFunc=mbrlen")
#pragma file_attr("libFunc=mbtowc")
#pragma file_attr("libFunc=mbrtowc")
#pragma file_attr("libFunc=mbsrtowcs")
#pragma file_attr("libFunc=wctomb")
#pragma file_attr("libFunc=wcrtomb")
#pragma file_attr("libFunc=wcsrtombs")
#endif
#pragma file_attr("prefersMem=any")
#pragma file_attr("prefersMemNum=50")

#include <sys/adi_rtl_sync.h>

#include "mc_sync.h"
#include "xsync.h"

static void *blocks[__NUM_CORES__][_TLV_NUM];

void *
adi_rtl_get_tls_ptr(void)
{
    return blocks[adi_core_id()];
}

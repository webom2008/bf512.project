/****************************************************************************
 *
 * vdk_tls.c: $Revision: 1.7.14.6 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 * Thread-local storage on VDK.
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

#include <sys/fatal_error_code.h>
#include "vdk_sync.h"
#include "xsync.h"

/* VDK thread-local storage key */
static int key = VDK_kTLSUnallocated;

/* TLS block for startup "thread" */
static void *startup_tls[_TLV_NUM];

static void
cleanup(void *tls_p)
{
    /* Cleanup function for the VDK thread-local storage slot:
     * allow the libraries to free anything linked from the TLS block before
     * freeing the block itself.
     */
    adi_rtl_destroy_tls_cb(tls_p);
    _sys_free(tls_p);
}

void *
adi_rtl_get_tls_ptr(void)
{
    if (!vdkMainMarker)
        return startup_tls;
    if (key != VDK_kTLSUnallocated) {
        void *tls_p = VDK_GetThreadSlotValue(key);
        if (tls_p)
            return tls_p;
    }
    else if (!VDK_AllocateThreadSlotEx(&key, cleanup)) {
        /* AllocateThreadSlotEx returning false can mean either that
         * another thread got there sooner and has already allocated a key,
         * which is fine, or that no more slots are available, in which case
         * key remains VDK_kTLSUnallocated and we have no choice but to bail
         * out.
         */
        if (key == VDK_kTLSUnallocated)
            adi_fatal_error(_AFE_G_LibraryError,
                            _AFE_S_TLSSlotAllocFailed, 0);
    }
    void *tls_p = _sys_calloc(_TLV_NUM, sizeof(void *));
    VDK_SetThreadSlotValue(key, tls_p);
    return tls_p;
}

/************************************************************************
 *
 * xtlv.c: $Revision: 3619 $
 *
 * The thread-local storage destructor.
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#pragma file_attr("libName=libc")
#pragma file_attr("libFunc=adi_rtl_destroy_tls_cb")
#pragma file_attr("libFunc=_adi_rtl_destroy_tls_cb")
#pragma file_attr("prefersMem=external")
#pragma file_attr("prefersMemNum=70")

#if !defined(_ADI_THREADS)
  #error This should not be built without -threads
#endif /* defined(_ADI_THREADS) */

#include "xsync.h"
#include <sys/fatal_error_code.h>

void (*__eh_tlv_dtor)(void *);

void
adi_rtl_destroy_tls_cb(void *tls_p)
{
    void **tlvs = tls_p;
    
    // Don't crash if we're given a null pointer for whatever reason.
    if (!tls_p)
        return;
    
    // Invoke exception handling destructor if necessary.
    if (__eh_tlv_dtor && tlvs[__eh_tlv_index])
            (*__eh_tlv_dtor)(tlvs[__eh_tlv_index]);
    
    // Free any heap-allocated TLVs.
    int i;
    for (i = __first_heap_tlv_index; i < _TLV_NUM; i++) {
        if (tlvs[i])
            _sys_free(tlvs[i]);
    }
}

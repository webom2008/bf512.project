/****************************************************************************
 *
 * xmutex.c : $Revision: 1.6.14.1 $
 *
 * Helper routine for initialising a mutex on first use and acquiring it.
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ****************************************************************************/

#pragma file_attr("libName=libc")
#pragma file_attr("libFunc=__check_and_acquire_mutex")
#pragma file_attr("libFunc=___check_and_acquire_mutex")
#pragma file_attr("prefersMem=any")
#pragma file_attr("prefersMemNum=50")

#if !defined(_ADI_THREADS)
  #error This should not be built without -threads
#endif /* defined(_ADI_THREADS) */

#include "xsync.h"
#include <sys/fatal_error_code.h>

void
__check_and_acquire_mutex(adi_rtl_mutex *mutex_p, bool io)
{
    /* Avoid the globals lock if already initialised */
    if (*mutex_p == ADI_RTL_INVALID_MUTEX) {
        bool ok = true;
        adi_rtl_lock_globals();
        /* Double-check in case another thread jumped in. */
        if (*mutex_p == ADI_RTL_INVALID_MUTEX)
            ok = adi_rtl_init_mutex(mutex_p, io);
        adi_rtl_unlock_globals();
        if (!ok)
            adi_fatal_error(_AFE_G_LibraryError,
                            _AFE_S_InsufficientHeapForLibrary, 0);
    }
    adi_rtl_acquire_mutex(mutex_p);
}

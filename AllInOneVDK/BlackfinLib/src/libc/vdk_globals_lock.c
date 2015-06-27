/****************************************************************************
 *
 * vdk_globals_lock.c : $Revision: 1.6.14.4 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 * The globals lock on VDK.
 *
 ****************************************************************************/

#pragma file_attr("libName=libc")
#pragma file_attr("libFunc=adi_rtl_lock_globals")
#pragma file_attr("libFunc=_adi_rtl_lock_globals")
#pragma file_attr("libFunc=adi_rtl_unlock_globals")
#pragma file_attr("libFunc=_adi_rtl_unlock_globals")
#pragma file_attr("libFunc=malloc")
#pragma file_attr("libFunc=_malloc")
#pragma file_attr("libFunc=calloc")
#pragma file_attr("libFunc=_calloc")
#pragma file_attr("libFunc=realloc")
#pragma file_attr("libFunc=_realloc")
#pragma file_attr("libFunc=free")
#pragma file_attr("libFunc=_free")
#pragma file_attr("libFunc=atexit")
#pragma file_attr("libFunc=_atexit")
#pragma file_attr("libFunc=signal")
#pragma file_attr("libFunc=_signal")
#pragma file_attr("libFunc=interrupt")
#pragma file_attr("libFunc=_interrupt")
#pragma file_attr("prefersMem=any")
#pragma file_attr("prefersMemNum=50")

#include <sys/adi_rtl_sync.h>

#include "vdk_sync.h"

/* Implements the globals lock as unscheduled regions. */
 
void
adi_rtl_lock_globals(void)
{
    if (vdkMainMarker)
        VDK_PushUnscheduledRegion();
}

void
adi_rtl_unlock_globals(void)
{
    if (vdkMainMarker)
        VDK_PopUnscheduledRegion();
}

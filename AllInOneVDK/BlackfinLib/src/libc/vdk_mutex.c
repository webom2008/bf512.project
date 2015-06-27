/****************************************************************************
 *
 * vdk_mutex.c : $Revision: 1.6.14.5 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 * Mutexes mapped to VDK.
 *
 ****************************************************************************/

#pragma file_attr("libName=libc")
#pragma file_attr("libFunc=adi_rtl_init_mutex")
#pragma file_attr("libFunc=_adi_rtl_init_mutex")
#pragma file_attr("libFunc=adi_rtl_deinit_mutex")
#pragma file_attr("libFunc=_adi_rtl_deinit_mutex")
#pragma file_attr("libFunc=adi_rtl_acquire_mutex")
#pragma file_attr("libFunc=_adi_rtl_acquire_mutex")
#pragma file_attr("libFunc=adi_rtl_release_mutex")
#pragma file_attr("libFunc=_adi_rtl_release_mutex")
#pragma file_attr("prefersMem=any")
#pragma file_attr("prefersMemNum=50")

#include <sys/adi_rtl_sync.h>

#include "vdk_sync.h"
#include <stdlib.h>

struct VDK_Mutex { void *dummy[5]; };

bool
adi_rtl_init_mutex(adi_rtl_mutex *mutex_p, bool io)
{
    VDK_Mutex *vdk_mutex_p = malloc(sizeof(VDK_Mutex));
    if (vdk_mutex_p)
        VDK_InitMutex(vdk_mutex_p, sizeof(VDK_Mutex), false);
    *mutex_p = (adi_rtl_mutex) vdk_mutex_p;
    return vdk_mutex_p;
}

void
adi_rtl_deinit_mutex(adi_rtl_mutex *mutex_p)
{
    VDK_Mutex *vdk_mutex_p = (VDK_Mutex *)(*mutex_p);
    if (vdk_mutex_p) {
        VDK_DeInitMutex(vdk_mutex_p);
        free(vdk_mutex_p);
        *mutex_p = 0;
    }
}

void
adi_rtl_acquire_mutex(adi_rtl_mutex *mutex_p)
{
    if (vdkMainMarker)
        VDK_AcquireMutex((VDK_MutexID) *mutex_p);
}

void
adi_rtl_release_mutex(adi_rtl_mutex *mutex_p)
{
    if (vdkMainMarker)
        VDK_ReleaseMutex((VDK_MutexID) *mutex_p);
}

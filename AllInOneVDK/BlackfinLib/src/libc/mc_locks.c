/****************************************************************************
 *
 * mc_locks.c : $Revision: 1.7.14.5 $
 *
 * Multicore implementation of adi_rtl_sync locking routines.
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ****************************************************************************/

#pragma file_attr("libName=libc")
#pragma file_attr("libFunc=adi_rtl_lock_globals")
#pragma file_attr("libFunc=_adi_rtl_lock_globals")
#pragma file_attr("libFunc=adi_rtl_unlock_globals")
#pragma file_attr("libFunc=_adi_rtl_unlock_globals")
#pragma file_attr("libFunc=adi_rtl_init_mutex")
#pragma file_attr("libFunc=_adi_rtl_init_mutex")
#pragma file_attr("libFunc=adi_rtl_deinit_mutex")
#pragma file_attr("libFunc=_adi_rtl_deinit_mutex")
#pragma file_attr("libFunc=adi_rtl_acquire_mutex")
#pragma file_attr("libFunc=_adi_rtl_acquire_mutex")
#pragma file_attr("libFunc=adi_rtl_release_mutex")
#pragma file_attr("libFunc=_adi_rtl_release_mutex")
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

#include "mc_sync.h"

#pragma inline
#pragma always_inline
static void
mc_acquire_mutex(mc_mutex *mutex_p)
{
    int core_id = adi_core_id();
    if (mutex_p->count == 0 || mutex_p->owner != core_id) {
        adi_acquire_lock((testset_t *)&mutex_p->lock);
        mutex_p->owner = core_id;
    }
    mutex_p->count++;
}

#pragma inline
#pragma always_inline
static void
mc_release_mutex(mc_mutex *mutex_p)
{
    mutex_p->count--;
    if (mutex_p->count == 0)
        adi_release_lock((testset_t *)&mutex_p->lock);
}


/* adi_rtl_sync mutexes. */

#ifdef _LIBMC

/* For libmc, used in the five-project model, map all mutex operations to a
 * single lock, as before. This way we ca ensure that the lock is in shared
 * memory by putting it into the mc_data section.
 */

#pragma section("mc_data")
extern mc_mutex __mc_io_mutex;

bool
adi_rtl_init_mutex(adi_rtl_mutex *mutex_p, bool io)
{
    return true;
}

void
adi_rtl_deinit_mutex(adi_rtl_mutex *mutex_p)
{
}

void
adi_rtl_acquire_mutex(adi_rtl_mutex *mutex_p)
{
    mc_acquire_mutex(&__mc_io_mutex);
}

void
adi_rtl_release_mutex(adi_rtl_mutex *mutex_p)
{
    mc_release_mutex(&__mc_io_mutex);
}

#else

/* For the thread-per-core model, implement each mutex indepedently. */

bool
adi_rtl_init_mutex(adi_rtl_mutex *mutex_p, bool io)
{
    *mutex_p = 0;
    return true;
}

void
adi_rtl_deinit_mutex(adi_rtl_mutex *mutex_p)
{
    *mutex_p = ADI_RTL_INVALID_MUTEX;
}

void
adi_rtl_acquire_mutex(adi_rtl_mutex *mutex_p)
{
    mc_acquire_mutex((mc_mutex *)mutex_p);
}

void
adi_rtl_release_mutex(adi_rtl_mutex *mutex_p)
{
    mc_release_mutex((mc_mutex *)mutex_p);
}

#endif


/* Map the adi_rtl_sync globals lock to a multicore mutex. */

extern mc_mutex __mc_globals_mutex;

void
adi_rtl_lock_globals(void)
{
    mc_acquire_mutex(&__mc_globals_mutex);
}

void
adi_rtl_unlock_globals(void)
{
    mc_release_mutex(&__mc_globals_mutex);
}

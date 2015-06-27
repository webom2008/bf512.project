/****************************************************************************
 *
 * mc_xlocks.c : $Revision: 1.1.2.1 $
 *
 * Multicore lock data.
 *
 * (c) Copyright 2010 Analog Devices, Inc.  All rights reserved.
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

#include "mc_sync.h"

#pragma file_attr("ThreadPerCoreSharing=MustShare")

#ifdef _LIBMC
#pragma section("mc_data")
mc_mutex __mc_io_mutex;
#endif

#ifdef _LIBMC
#pragma section("mc_data")
#endif
mc_mutex __mc_globals_mutex;

/****************************************************************************
 *
 * mc_xlots.c : $Revision: 1.3.14.3 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 * Core-local storage data used by mc_slots.c.
 *
 ****************************************************************************/

#pragma file_attr("libName=libc")
#pragma file_attr("libFunc=__mc_slot_dtors")
#pragma file_attr("libFunc=___mc_slot_dtors")
#pragma file_attr("libFunc=__mc_slot_dtors_lock")
#pragma file_attr("libFunc=___mc_slot_dtors_lock")
#pragma file_attr("libFunc=adi_obtain_mc_slot")
#pragma file_attr("libFunc=_adi_obtain_mc_slot")
#pragma file_attr("libFunc=adi_free_mc_slot")
#pragma file_attr("libFunc=_adi_free_mc_slot")
#pragma file_attr("libFunc=adi_set_mc_value")
#pragma file_attr("libFunc=_adi_set_mc_value")
#pragma file_attr("libFunc=adi_get_mc_value")
#pragma file_attr("libFunc=_adi_get_mc_value")
#pragma file_attr("libFunc=__destroy_mc_values")
#pragma file_attr("libFunc=___destroy_mc_values")
#pragma file_attr("prefersMem=any")
#pragma file_attr("prefersMemNum=50")

#include "mc_sync.h"
#include <mc_data.h>

/* Ensure global data is in shared memory for multicore programs. */
#pragma file_attr("ThreadPerCoreSharing=MustShare")

#ifdef _LIBMC
#pragma default_section(ALLDATA, "mc_data")
#endif

mc_slot_dtor_t __mc_slot_dtors[_MC_NO_OF_SLOTS];
testset_t __mc_slot_dtors_lock;

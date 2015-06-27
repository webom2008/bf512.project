/****************************************************************************
 *
 * mc_slots.c : $Revision: 1.4.14.3 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 * Core-local storage interface for users.
 *
 ****************************************************************************/

#pragma file_attr("libName=libc")
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

#include <mc_data.h>
#include "mc_sync.h"
#include <stdbool.h>

/* The slot destructor table, which also indicates which slots are in use,
 * needs to be shared between cores. The same goes for the lock protecting
 * it. These are defined accordingly in mc_xslots.c.
 * The actual data in the slots does not need to be shared.
 */
void *__mc_slots[__NUM_CORES__][_MC_NO_OF_SLOTS];

/* A null entry in the dtor table indicates an unused slot, whereas the
 * NO_DTOR constant here represents an in-use slot without a destructor.
 */
#define NO_DTOR ((mc_slot_dtor_t)-1)

int
adi_obtain_mc_slot(int *key_p, mc_slot_dtor_t dtor)
{
    adi_acquire_lock(&__mc_slot_dtors_lock);
    if (*key_p == INT_MIN) {
        int k;
        for (k = 0; k < _MC_NO_OF_SLOTS; k++) {
            if (!__mc_slot_dtors[k]) {
                *key_p = k;
                __mc_slot_dtors[k] = dtor ?: NO_DTOR;
                adi_release_lock(&__mc_slot_dtors_lock);
                return true;
            }
        }
    }
    adi_release_lock(&__mc_slot_dtors_lock);
    return false;
}

int
adi_free_mc_slot(int key)
{
    if (key < 0 || key >= _MC_NO_OF_SLOTS || !__mc_slot_dtors[key])
        return false;
    __mc_slot_dtors[key] = 0;
    return true;
}

int
adi_set_mc_value(int key, void *val)
{
    if (key < 0 || key >= _MC_NO_OF_SLOTS || !__mc_slot_dtors[key])
        return false;
    __mc_slots[adi_core_id()][key] = val;
    return true;
}

void * 
adi_get_mc_value(int key)
{
    if (key < 0 || key >= _MC_NO_OF_SLOTS || !__mc_slot_dtors[key])
        return 0;
    return __mc_slots[adi_core_id()][key];
}


/* Core-local storage destructor. To be invoked by (adi_core_)exit. */

void
__destroy_mc_values(void)
{
    int core = adi_core_id();
    int i = 4;
    bool again;
    do {
        // Up to four destructor iterations
        again = false;
        int k;
        for (k = 0; k < _MC_NO_OF_SLOTS; k++) {
            void *val = __mc_slots[core][k];
            __mc_slots[core][k] = 0;
            mc_slot_dtor_t dtor = __mc_slot_dtors[k];
            if (dtor && dtor != NO_DTOR && val) {
                (*dtor)(val);
                again = true;
            }
        }
    } while (again && --i);
}

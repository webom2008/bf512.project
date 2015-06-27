/************************************************************************
 *
 * mc_sync.h: $Revision: 1.7.16.3 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 * Common support for multicore bindings.
 *
 ************************************************************************/

#ifndef _MC_SYNC_H
#define _MC_SYNC_H

#if __NUM_CORES__ < 2
#error This header should be used for multicore processors only.
#endif

#ifndef _ADI_THREADS
 #error This header should only be used with _ADI_THREADS defined.
#endif

#if !defined(_LIBTPC) && !defined(_LIBMC)
#error This header should only be used in libtpc or libmc.
#endif

#include <ccblkfn.h>

/* Type for representing multicore mutexes in a single word.
 * Alongside the actual lock, which is operated with testset, this contains
 * owner and count fields to implement recursive semantics.
 */
typedef struct {
    short lock;
    char owner;
    unsigned char count;
} mc_mutex;

/* Destructor table for core-local storage slots, along with its lock.
 * Null entries represent unallocated slots.
 */
typedef void (*mc_slot_dtor_t)(void *);
extern mc_slot_dtor_t __mc_slot_dtors[];
extern testset_t __mc_slot_dtors_lock;

#endif

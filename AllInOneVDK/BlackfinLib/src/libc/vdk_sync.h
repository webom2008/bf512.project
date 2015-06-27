/****************************************************************************
 *
 * vdk_sync.h: $Revision: 1.8.14.3 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 * Implementation header for the VDK-adi_rtl_sync binding.
 *
 ****************************************************************************/

#ifndef _VDK_SYNC_H
#define _VDK_SYNC_H

#if !defined(_ADI_THREADS)
  #error This should only be used with -threads
#endif

#include <limits.h>
#include <stdbool.h>

/* Startup indicator */

extern bool vdkMainMarker;


/* Thread-local storage */

enum { VDK_kTLSUnallocated = INT_MIN };

#pragma linkage_name _AllocateThreadSlotEx__3VDKFPiPFPv_v
bool VDK_AllocateThreadSlotEx(int *, void(*dtor)(void *));

#pragma linkage_name _SetThreadSlotValue__3VDKFiPv
bool VDK_SetThreadSlotValue(int, void *);

#pragma linkage_name _GetThreadSlotValue__3VDKFi
void *VDK_GetThreadSlotValue(int);


/* Unscheduled regions */

#pragma linkage_name _PushUnscheduledRegion__3VDKFv
void VDK_PushUnscheduledRegion(void);

#pragma linkage_name _PopUnscheduledRegion__3VDKFv
void VDK_PopUnscheduledRegion(void);


/* Mutexes */

typedef struct VDK_Mutex VDK_Mutex;
typedef enum { INVALID_VDK_MUTEX_ID = -1 } VDK_MutexID;

#pragma linkage_name _InitMutex__3VDKFPQ2_3VDK5MutexUib
void VDK_InitMutex(VDK_Mutex *, unsigned int size, bool log);

#pragma linkage_name _DeInitMutex__3VDKFPQ2_3VDK5Mutex
void VDK_DeInitMutex(VDK_Mutex *);

#pragma linkage_name _AcquireMutex__3VDKF7MutexID
void VDK_AcquireMutex(VDK_MutexID);

#pragma linkage_name _ReleaseMutex__3VDKF7MutexID
void VDK_ReleaseMutex(VDK_MutexID);


#endif

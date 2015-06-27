/************************************************************************
 *
 * devIOtab.c : $Revision: 3543 $
 *
 * (c) Copyright 2003-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=device.h")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

/* Ensure global data is in shared memory for multicore programs. */
#pragma file_attr("ThreadPerCoreSharing=MustShare")

#include "dev_tab.h"

sDevTab DeviceIOtable[MAXFD];

#if defined(_ADI_THREADS) && _FILE_OP_LOCKS
/* These do not need to be initialised to ADI_RTL_INVALID_MUTEX,
 * because they get initialised before use by __init_devdrv.
 */
adi_rtl_mutex __devdrv_mutexs[MAXFD];
#endif

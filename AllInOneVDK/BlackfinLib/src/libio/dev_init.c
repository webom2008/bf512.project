/************************************************************************
 *
 * dev_init.c : $Revision: 3543 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=device.h")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

/* Ensure global data is in shared memory for multicore programs. */
#pragma file_attr("ThreadPerCoreSharing=MustShare")

#include "dev_tab.h"

/*
 * Initialises a device driver with DevDrvTable index i.
 */
bool
__init_devdrv(DevEntry *dev, int i)
{
    /* Check that this device driver does not try to claim a standard device
     * that has already been taken.
     */
    if ((dev->stdinfd  != dev_not_claimed && DEVDRV(STDIN_FD)  != -1) ||
        (dev->stdoutfd != dev_not_claimed && DEVDRV(STDOUT_FD) != -1) ||
        (dev->stderrfd != dev_not_claimed && DEVDRV(STDERR_FD) != -1)
    )
        return false;
    
    /* Invoke device initialisation routine. */
    int init_res = dev->init(dev);
    if (init_res == -1)
        return false;
    
#if _FILE_OP_LOCKS
    /* Initialise the device's lock */
    if (init_res != _DEV_IS_THREADSAFE) {
        if (!_INIT_MUTEX(&__devdrv_mutexs[i], true))
            return false;
    }
#endif
    
    /* Claim any standard devices. */
    if (dev->stdinfd != dev_not_claimed) {
        DEVDRV(STDIN_FD) = i;
        DEVFD(STDIN_FD) = dev->stdinfd;
    }
    if (dev->stdoutfd != dev_not_claimed) {
        DEVDRV(STDOUT_FD) = i;
        DEVFD(STDOUT_FD) = dev->stdoutfd;
    }
    if (dev->stderrfd != dev_not_claimed) {
        DEVDRV(STDERR_FD) = i;
        DEVFD(STDERR_FD) = dev->stderrfd;
    }
    
    return true;
}

bool __devtabs_initialized = false;

#if _FILE_OP_LOCKS
_DEF_MUTEX(__devtabs_mutex)
#endif

void
__init_devtabs(void)
{
#if _FILE_OP_LOCKS
    _CHECK_AND_ACQUIRE_MUTEX(&__devtabs_mutex, true);

    /* Return if another thread got here first */
    if (__devtabs_initialized) {
        _UNLOCK_DEVTABS();
        return;
    }
#endif

    /* Initialise file table. */
    int fd; 
    for (fd = 0; fd < MAXFD; fd++) {
        DEVFD(fd) = -1;
        DEVDRV(fd) = -1;
    }
    
    /* Initialise device drivers. */
    DevEntry *dev;
    int i;
    for (i = 0; i < MAXDEV && (dev = DevDrvTable[i]) != 0; i++)
        __init_devdrv(dev, i);
    
    __devtabs_initialized = true;
    
    _UNLOCK_DEVTABS();
}

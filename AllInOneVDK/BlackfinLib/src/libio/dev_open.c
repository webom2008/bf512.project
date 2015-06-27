/************************************************************************
 *
 * dev_open.c : $Revision: 3543 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=device_int.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=_dev_open")
#pragma file_attr(  "libFunc=__dev_open")
#pragma file_attr(  "libFunc=fopen")
#pragma file_attr(  "libFunc=_fopen")
#pragma file_attr(  "libFunc=freopen")
#pragma file_attr(  "libFunc=_freopen")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <device.h>
#include "device_int.h"
#include "dev_tab.h"

/*
@subsection dev_open
Opens a file using the device described by the first argument
The open() associated by the device referenced by the first
argument is invoked.
*/
int
_dev_open(int drv_id, char *name, int mode)
{
    _INIT_DEVTABS();
    
    // Look up the device driver by ID.
    DevEntry *dep;
    int drv = 0;
    for (;;) {
        if (!(dep = DevDrvTable[drv]))
            return -1;  // Reached end marker.
        if (drv_id == dep->DeviceID)
            break;  // Found the driver.
        if (++drv == MAXDEV)  // Reached end of the table. 
            return -1;
    }
    
    // Find a free file descriptor.
    _LOCK_DEVTABS();
    int fd = 0;
    while (DEVDRV(fd) >= 0) {
        if (++fd == MAXFD) {
            // None available.
            _UNLOCK_DEVTABS();
            return -1;
        }
    }
    DEVDRV(fd) = drv;   // Found one: grab it.
    _UNLOCK_DEVTABS();
    
    // Invoke the driver's open function.
    _LOCK_DEVDRV(drv);
    int devfd = dep->open(name, mode);
    _UNLOCK_DEVDRV(drv);

    if (devfd == -1) {
        // Driver failed to open the file: release the FD and return failure.
        DEVDRV(fd) = -1;
        return -1;
    }
    
    DEVFD(fd) = devfd;
    return fd;
}

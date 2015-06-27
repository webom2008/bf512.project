/************************************************************************
 *
 * dev_dup.c : $Revision: 3543 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#include <device.h>
#include <device_int.h>
#include "dev_tab.h"
#pragma file_attr( "libGroup=device_int.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=_dev_dup")
#pragma file_attr(  "libFunc=__dev_dup")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

/*
@subsection dev_dup
Makes a copy of the structure indexed by the file descriptor
and places it in the first open slot on the table.
*/
int
_dev_dup(int fd)
{
    if (!FDOK(fd))
        return -1;
    
    _INIT_DEVTABS();
    int drv = DEVDRV(fd);
    if (drv < 0)
        return -1;
    
    // Find a free file descriptor.
    _LOCK_DEVTABS();
    int fd2 = 0;
    while (DEVDRV(fd2) >= 0) {
        if (++fd2 == MAXFD) {
            // None available.
            _UNLOCK_DEVTABS();
            return -1;
        }
    }
    DEVDRV(fd2) = drv;  // Found one: grab it.
    DEVFD(fd2) = DEVFD(fd);
    _UNLOCK_DEVTABS();

    return fd2;
}

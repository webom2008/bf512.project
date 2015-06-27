/************************************************************************
 *
 * dev_close.c : $Revision: 3543 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=device_int.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=_dev_close")
#pragma file_attr(  "libFunc=__dev_close")
#pragma file_attr(  "libFunc=fclose")
#pragma file_attr(  "libFunc=_fclose")
#pragma file_attr(  "libFunc=freopen")
#pragma file_attr(  "libFunc=_freopen")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <device.h>
#include <device_int.h>
#include "dev_tab.h"

/*
@subsection dev_close
Closes the file associated with the file descriptor.
The close() routine associated with the device is invoked
to close the file.
*/      
int
_dev_close(int fd)
{
    if (!FDOK(fd))
        return -1;
    
    _INIT_DEVTABS();
    int drv = DEVDRV(fd);
    if (drv < 0)
        return -1;

    int devfd = DEVFD(fd);
    DEVDRV(fd) = -1;
    
    _LOCK_DEVDRV(drv);
    int ret = DevDrvTable[drv]->close(devfd);
    _UNLOCK_DEVDRV(drv);
    return ret;
}

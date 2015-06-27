/************************************************************************
 *
 * dev_seek.c : $Revision: 3543 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=device_int.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=_dev_seek")
#pragma file_attr(  "libFunc=__dev_seek")
#pragma file_attr(  "libFunc=fseek")
#pragma file_attr(  "libFunc=_fseek")
#pragma file_attr(  "libFunc=fgetpos")
#pragma file_attr(  "libFunc=_fgetpos")
#pragma file_attr(  "libFunc=rewind")
#pragma file_attr(  "libFunc=_rewind")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <device.h>
#include <device_int.h>
#include "dev_tab.h"

/*
@subsection dev_seek
Change the position of the file position pointer associated with
the file descriptor.
*/
long
_dev_seek(int fd, long offset, int whence)
{
    if (!FDOK(fd))
        return -1;
    
    _INIT_DEVTABS();
    int drv = DEVDRV(fd);
    if (drv <  0)
        return -1;
    
    _LOCK_DEVDRV(drv);
    int ret = DevDrvTable[drv]->seek(DEVFD(fd), offset, whence);
    _UNLOCK_DEVDRV(drv);
    return ret;
}

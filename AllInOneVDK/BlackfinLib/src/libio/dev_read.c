/************************************************************************
 *
 * dev_read.c : $Revision: 3543 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=device_int.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=_dev_read")
#pragma file_attr(  "libFunc=__dev_read")
#pragma file_attr(  "libFunc=fgets")
#pragma file_attr(  "libFunc=_fgets")
#pragma file_attr(  "libFunc=fgetc")
#pragma file_attr(  "libFunc=_fgetc")
#pragma file_attr(  "libFunc=fscanf")
#pragma file_attr(  "libFunc=_fscanf")
#pragma file_attr(  "libFunc=scanf")
#pragma file_attr(  "libFunc=_scanf")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <device.h>
#include <device_int.h>
#include "dev_tab.h"

/*
@subsection dev_read
Read a string from a file referenced by the file descriptor
*/
int
_dev_read(int fd, void *buf, int size)
{
    if (!FDOK(fd))
        return -1;
    
    _INIT_DEVTABS();
    int drv = DEVDRV(fd);
    if (drv < 0)
        return -1;
    
    _LOCK_DEVDRV(drv);
    int ret = DevDrvTable[drv]->read(DEVFD(fd), buf, size);
    _UNLOCK_DEVDRV(drv);
    return ret;
}

/************************************************************************
 *
 * dev_write.c : $Revision: 3543 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=device_int.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=_dev_write")
#pragma file_attr(  "libFunc=__dev_write")
#pragma file_attr(  "libFunc=printf")
#pragma file_attr(  "libFunc=_printf")
#pragma file_attr(  "libFunc=fwrite")
#pragma file_attr(  "libFunc=_fwrite")
#pragma file_attr(  "libFunc=fprintf")
#pragma file_attr(  "libFunc=_fprintf")
#pragma file_attr(  "libFunc=fputs")
#pragma file_attr(  "libFunc=_fputs")
#pragma file_attr(  "libFunc=fputc")
#pragma file_attr(  "libFunc=_fputc")
#pragma file_attr(  "libFunc=fflush")
#pragma file_attr(  "libFunc=_fflush")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <device.h>
#include <device_int.h>
#include "dev_tab.h"

/*
@subsection dev_write
Write a string to the file referenced by the file descriptor.
*/
int
_dev_write(int fd, void *buf, int size)
{
    if (!FDOK(fd))
        return -1;
    
    _INIT_DEVTABS();
    int drv = DEVDRV(fd);
    if (drv < 0)
        return -1;
    
    _LOCK_DEVDRV(drv);
    int ret = DevDrvTable[drv]->write(DEVFD(fd), buf, size);
    _UNLOCK_DEVDRV(drv);
    return ret;
}

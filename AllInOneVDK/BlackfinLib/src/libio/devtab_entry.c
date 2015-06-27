/************************************************************************
 *
 * devtab_entry.c : $Revision: 3543 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=device.h")
#pragma file_attr( "libGroup=device_int.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=add_devtab_entry")
#pragma file_attr(  "libFunc=_add_devtab_entry")
#pragma file_attr(  "libFunc=get_default_io_device")
#pragma file_attr(  "libFunc=_get_default_io_device")
#pragma file_attr(  "libFunc=_set_default_io_device")
#pragma file_attr(  "libFunc=set_default_io_device")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "dev_tab.h"

/* Add a new device driver to the table */

int
add_devtab_entry(DevEntry *dev)
{
    int dev_id = dev->DeviceID;
    if (dev_id < 0 ||
        !dev->init  || !dev->seek || !dev->open || 
        !dev->close || !dev->read || !dev->write
    )
        return -1;
    
    _INIT_DEVTABS();
    _LOCK_DEVTABS();
    int i;
    for (i = 0; i < MAXDEV; i++) {
        if (!DevDrvTable[i]) {
            // Found empty slot.
            if (!__init_devdrv(dev, i))
                break;  // Initialization failed.
            DevDrvTable[i] = dev;
            _UNLOCK_DEVTABS();
            return dev_id;
        }
        else if (dev_id == DevDrvTable[i]->DeviceID)
            break;  // Device ID already taken.
    }
    _UNLOCK_DEVTABS();
    return -1;  // Failure.
}

int get_default_io_device(void)
{
    return __default_io_device;
}

void set_default_io_device(int dev_id)
{
    __default_io_device = dev_id;
}

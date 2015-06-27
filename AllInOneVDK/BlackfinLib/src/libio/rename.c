/************************************************************************
 *
 * rename.c : $Revision: 3544 $
 *
 * (c) Copyright 2003-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=rename")
#pragma file_attr(  "libFunc=_rename")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include "_primio.h"

int
rename(const char *old_name, const char *new_name)
{
    /* Place a call directly into the PrimIO routine.
     * Note that since this doesn't use a File Descriptor we avoid using
     * the generalised FileI/O.
     * Without file/device-specific locks, PrimIO does its own locking.
     */
    int result;

    /* Return failure if NULL pointers */
    if (!old_name || !new_name)
        return -1;

#if !_FILE_OP_LOCKS
    _LOCK_FILES();
#endif
    result = PRIMITIVE_RENAME((char *)old_name, (char *)new_name);
#if !_FILE_OP_LOCKS
    _UNLOCK_FILES();
#endif

    return result;
}

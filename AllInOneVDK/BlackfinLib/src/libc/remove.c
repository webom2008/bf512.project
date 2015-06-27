/************************************************************************
 *
 * remove.c: $Revision: 1.8 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=remove")
#pragma file_attr(  "libFunc=_remove")
#pragma file_attr(  "libFunc=fclose")
#pragma file_attr(  "libFunc=_fclose")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include <xsyslock.h>

extern int PRIMITIVE_REMOVE(const char *name);

int
remove(const char *name)
{
    /* Place a call directly into the PrimIO routine.
     * Note that since this doesn't use a File Descriptor we avoid using
     * the generalised FileI/O.
     * Without file/device-specific locks, PrimIO does its own locking.
     */
    int result;

    if (!name)
        return -1;

#if !_FILE_OP_LOCKS
    _Locksyslock(_LOCK_STREAM);
#endif
    result = PRIMITIVE_REMOVE(name);
#if !_FILE_OP_LOCKS
    _Unlocksyslock(_LOCK_STREAM);
#endif

    return result;
}

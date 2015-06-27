/************************************************************************
 *
 * xprim_open.c : $Revision: 5445 $
 *
 * (c) Copyright 1998-2011 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=PRIMITIVE_OPEN")
#pragma file_attr(  "libFunc=_PRIMITIVE_OPEN")
#pragma file_attr(  "libFunc=fopen")
#pragma file_attr(  "libFunc=_fopen")
#pragma file_attr(  "libFunc=freopen")
#pragma file_attr(  "libFunc=_freopen")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")
#endif

#include <stdlib.h>

#include <string.h>
#include "_stdio.h"
#include "_primio.h"
#include "_primio_intrpt.h"

#if defined(__ADSPBLACKFIN__)
#include "_primio_blkfn.h"
#endif


int
PRIMITIVE_OPEN(const char* name, int mode)
{
    int fd;
    interrupt_state istate;

    _LOCK_PRIMIO();
    PrimIOCB.op = PRIM_OPEN;
    PrimIOCB.fileID = strlen(name);
    PrimIOCB.buf = (unsigned char*) name;
    PrimIOCB.flags = (0x3F & mode) | M_STRLEN_PROVIDED;

    PrimIOCB.more = NULL;

    DISABLE_INTERRUPTS(istate);
#if defined(__ADSPBLACKFIN__)
    FLUSH_CACHE_PRIMIOCB();
    DISABLE_CACHE();
    SYNCH_MEMORY();
#endif
    _primIO();
    fd = PrimIOCB.fileID;
#if defined(__ADSPBLACKFIN__)
    ENABLE_CACHE();
#endif
    ENABLE_INTERRUPTS(istate);
    _UNLOCK_PRIMIO();

    return fd;
}

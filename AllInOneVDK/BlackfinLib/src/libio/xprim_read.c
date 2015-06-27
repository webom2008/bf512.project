/************************************************************************
 *
 * xprim_read.c : $Revision: 5445 $
 *
 * (c) Copyright 1998-2011 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=PRIMITIVE_READ")
#pragma file_attr(  "libFunc=_PRIMITIVE_READ")
#pragma file_attr(  "libFunc=fread")
#pragma file_attr(  "libFunc=_fread")
#pragma file_attr(  "libFunc=fgetc")
#pragma file_attr(  "libFunc=_fgetc")
#pragma file_attr(  "libFunc=fgets")
#pragma file_attr(  "libFunc=_fgets")
#pragma file_attr(  "libFunc=fscanf")
#pragma file_attr(  "libFunc=_fscanf")
#pragma file_attr(  "libFunc=scanf")
#pragma file_attr(  "libFunc=_scanf")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")
#endif

#include <stdlib.h>

#include "_stdio.h"
#include "_primio.h"
#include "_primio_intrpt.h"

#if defined(__ADSPBLACKFIN__)
#include "_primio_blkfn.h"
#endif

int
PRIMITIVE_READ(int fileID, unsigned char* buf, size_t n)
{
    int res;
    interrupt_state istate;

    _LOCK_PRIMIO();
    PrimIOCB.op = PRIM_READ;
    PrimIOCB.fileID = fileID;
    PrimIOCB.flags = BYTES_PER_WORD;
    PrimIOCB.buf = (volatile unsigned char*) buf;
    PrimIOCB.nDesired = n;
    PrimIOCB.nCompleted = 0;
    PrimIOCB.more = NULL;

    DISABLE_INTERRUPTS(istate);
#if defined(__ADSPBLACKFIN__)
    FLUSH_CACHE(buf, buf+n);
    FLUSH_CACHE_PRIMIOCB();
    DISABLE_CACHE();
    SYNCH_MEMORY();
#endif
    _primIO();
    res = PrimIOCB.nCompleted;
#if defined(__ADSPBLACKFIN__)
    ENABLE_CACHE();
#endif
    ENABLE_INTERRUPTS(istate);
    _UNLOCK_PRIMIO();

    return res;
}

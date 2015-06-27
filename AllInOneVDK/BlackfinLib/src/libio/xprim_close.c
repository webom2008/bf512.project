/************************************************************************
 *
 * xprim_close.c : $Revision: 5445 $
 *
 * (c) Copyright 1998-2011 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=PRIMITIVE_CLOSE")
#pragma file_attr(  "libFunc=_PRIMITIVE_CLOSE")
#pragma file_attr(  "libFunc=fclose")
#pragma file_attr(  "libFunc=_fclose")
#pragma file_attr(  "libFunc=freopen")
#pragma file_attr(  "libFunc=_freopen")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")
#endif

#include <stdlib.h>

#include "_primio.h"
#include "_primio_intrpt.h"

#if defined(__ADSPBLACKFIN__)
#include "_primio_blkfn.h"
#endif

int
PRIMITIVE_CLOSE(int fileID)
{
    interrupt_state istate;

    _LOCK_PRIMIO();
    PrimIOCB.op = PRIM_CLOSE;
    PrimIOCB.fileID = fileID;
    PrimIOCB.flags = 0;          /* (not used) */

    PrimIOCB.more = NULL;

    DISABLE_INTERRUPTS(istate);
#if defined(__ADSPBLACKFIN__)
    FLUSH_CACHE_PRIMIOCB();
    DISABLE_CACHE();
    SYNCH_MEMORY();
#endif
    _primIO();
#if defined(__ADSPBLACKFIN__)
    ENABLE_CACHE();
#endif
    ENABLE_INTERRUPTS(istate);
    _UNLOCK_PRIMIO();

    return 0;
}

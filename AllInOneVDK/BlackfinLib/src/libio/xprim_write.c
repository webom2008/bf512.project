/************************************************************************
 *
 * xprim_write.c : $Revision: 5445 $
 *
 * (c) Copyright 1998-2011 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=PRIMITIVE_WRITE")
#pragma file_attr(  "libFunc=_PRIMITIVE_WRITE")
#pragma file_attr(  "libFunc=printf")
#pragma file_attr(  "libFunc=_printf")
#pragma file_attr(  "libFunc=sprintf")
#pragma file_attr(  "libFunc=_sprintf")
#pragma file_attr(  "libFunc=fprintf")
#pragma file_attr(  "libFunc=_fprintf")
#pragma file_attr(  "libFunc=fputs")
#pragma file_attr(  "libFunc=_fputs")
#pragma file_attr(  "libFunc=fputc")
#pragma file_attr(  "libFunc=_fputc")
#pragma file_attr(  "libFunc=puts")
#pragma file_attr(  "libFunc=_puts")
#pragma file_attr(  "libFunc=putc")
#pragma file_attr(  "libFunc=_putc")
#pragma file_attr(  "libFunc=putchar")
#pragma file_attr(  "libFunc=_putchar")
#pragma file_attr(  "libFunc=vsprintf")
#pragma file_attr(  "libFunc=_vsprintf")
#pragma file_attr(  "libFunc=vsnprintf")
#pragma file_attr(  "libFunc=_vsnprintf")
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
PRIMITIVE_WRITE(int fileID, int aligned, unsigned char* buf, size_t n)
{
    interrupt_state istate;

    _LOCK_PRIMIO();
    PrimIOCB.op = PRIM_WRITE;
    PrimIOCB.fileID = fileID;
    PrimIOCB.flags = BYTES_PER_WORD | (aligned ? 0x10 : 0);
    PrimIOCB.buf = buf;
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
#if defined(__ADSPBLACKFIN__)
    ENABLE_CACHE();
#endif
    ENABLE_INTERRUPTS(istate);
    _UNLOCK_PRIMIO();

    return n; // assume to always pass
}

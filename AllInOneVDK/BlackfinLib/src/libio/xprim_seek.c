/************************************************************************
 *
 * xprim_seek.c : $Revision: 5445 $
 *
 * (c) Copyright 1998-2011 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=PRIMITIVE_SEEK")
#pragma file_attr(  "libFunc=_PRIMITIVE_SEEK")
#pragma file_attr(  "libFunc=fseek")
#pragma file_attr(  "libFunc=_fseek")
#pragma file_attr(  "libFunc=fgetpos")
#pragma file_attr(  "libFunc=_fgetpos")
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

long
PRIMITIVE_SEEK(int fileID, long offset, int whence)
{
    long res;
    interrupt_state istate;

    _LOCK_PRIMIO();
    PrimIOCB.op = PRIM_SEEK;
    PrimIOCB.fileID = fileID;
    PrimIOCB.nDesired = offset;
    PrimIOCB.nCompleted = 0;

    /* we need to map between the Stdio Whence mode and the val that PrimIO
       is expecting.
     */
    if ( whence == 0 )
      PrimIOCB.flags = M_SEEK_SET;
    else if ( whence == 1 )
      PrimIOCB.flags = M_SEEK_CUR;
    else if ( whence == 2 )
      PrimIOCB.flags = M_SEEK_END;
    else return -1; /* invalid mode */

    PrimIOCB.more = NULL;

    DISABLE_INTERRUPTS(istate);
#if defined(__ADSPBLACKFIN__)
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

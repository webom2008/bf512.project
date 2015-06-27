/************************************************************************
 *
 * xprim.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=_primIO")
#pragma file_attr(  "libFunc=__primIO")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

/* Ensure global data is in shared memory for multicore programs. */
#pragma file_attr("ThreadPerCoreSharing=MustShare")

#include <stdio.h>

#include "_stdio.h"
#include "_primio.h"

#if defined(__ADSPBLACKFIN__)
#pragma section("voldata")
#endif
PrimIOCB_T    PrimIOCB;

#if _FILE_OP_LOCKS
_DEF_MUTEX(__primio_mutex)
#endif

#if defined(__ADSPBLACKFIN__)
#pragma section("voldata")
#endif
volatile int _lone_SHARC = 1;    /* Will be reset by DLK, if present */

#if defined(__ADSPBLACKFIN__)
#pragma section("voldata")
#endif
void* volatile _Godot = NULL;    /* Will be read by DLK, if present */

void _primIO(void)
{
    /* If the debugger is present, it will put a breakpoint on this
       label, and none of this code will execute.  

       If the DLK is being used, then it will tell us by resetting 
       _lone_SHARC, and we'll interact through the _Godot pointer.

       If we're all alone, we'll just return without doing anything,
       which (by convention) is interpreted as an I/O failure.
    */

    if (!_lone_SHARC)
    {
        _Godot = &PrimIOCB;
        while (_Godot != NULL)    /* Waiting for Godot */
        ;
    }
}

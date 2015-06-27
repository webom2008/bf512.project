/************************************************************************
 *
 * xfiles.c : $Revision: 3544 $
 *
 * (c) Copyright 2003-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

/* Ensure global data is in shared memory for multicore programs. */
#pragma file_attr("ThreadPerCoreSharing=MustShare")

#include <stdio.h>

#include "_stdio.h"

FILE _Stdin = {
    ._Mode = M_OPENR,
    .fileID = 0,
    ._Next = (byte_addr_t) NULL,
    ._Rend = (byte_addr_t) NULL,
    ._Wend = (byte_addr_t) NULL,
    ._Buf =  (byte_addr_t) NULL,
    ._Bend = (byte_addr_t) NULL,
    .bufadr = NULL,
#if defined(_ADI_THREADS) && _FILE_OP_LOCKS
    ._Mutex = (void *) ADI_RTL_INVALID_MUTEX
#endif
};

FILE _Stdout = {
    ._Mode = M_OPENW | M_LINE_BUFFERING,
    .fileID = 1,
    ._Next = (byte_addr_t) NULL,
    ._Rend = (byte_addr_t) NULL,
    ._Wend = (byte_addr_t) NULL,
    ._Buf =  (byte_addr_t) NULL,
    ._Bend = (byte_addr_t) NULL,
    .bufadr = NULL,
#if defined(_ADI_THREADS) && _FILE_OP_LOCKS
    ._Mutex = (void *) ADI_RTL_INVALID_MUTEX
#endif
};

FILE _Stderr = {
    ._Mode = M_OPENW | M_NO_BUFFERING,
    .fileID = 2,
    ._Next = (byte_addr_t) NULL,
    ._Rend = (byte_addr_t) NULL,
    ._Wend = (byte_addr_t) NULL,
    ._Buf =  (byte_addr_t) NULL,
    ._Bend = (byte_addr_t) NULL,
    .bufadr = NULL,
#if defined(_ADI_THREADS) && _FILE_OP_LOCKS
    ._Mutex = (void *) ADI_RTL_INVALID_MUTEX
#endif
};

FILE* _Files[FOPEN_MAX] = {&_Stdin, &_Stdout, &_Stderr};
_DEF_MUTEX(_Files_mutex)


// end of file

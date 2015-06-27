/************************************************************************
 *
 * errno.c: $Revision: 1.10 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/* errno access */
#pragma file_attr( "libGroup=errno.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=_Geterrno")
#pragma file_attr(  "libFunc=__Geterrno")
#pragma file_attr(  "libFunc=perror")
#pragma file_attr(  "libFunc=_perror")
#pragma file_attr(  "libFunc=strtol")
#pragma file_attr(  "libFunc=_strtol")
#pragma file_attr(  "libFunc=strtoll")
#pragma file_attr(  "libFunc=_strtoll")
#pragma file_attr(  "libFunc=strtoul")
#pragma file_attr(  "libFunc=_strtoul")
#pragma file_attr(  "libFunc=strtoull")
#pragma file_attr(  "libFunc=_strtoull")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#include <errno.h>

#include "xsync.h"

#undef errno

_STD_BEGIN

_TLV_DEF(__errno_tlv_index, int, errno);

// For compatibility with VDSP <= 5.0.
int *
_Geterrno(void)
{
    return &_TLV(errno);
}

_STD_END

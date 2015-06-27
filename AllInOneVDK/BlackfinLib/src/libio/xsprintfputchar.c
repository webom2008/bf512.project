/************************************************************************
 *
 * xsprintfputchar.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=_sprintfputchar")
#pragma file_attr(  "libFunc=__sprintfputchar")
#pragma file_attr(  "libFunc=sprintf")
#pragma file_attr(  "libFunc=_sprintf")
#pragma file_attr(  "libFunc=vsprintf")
#pragma file_attr(  "libFunc=_vsprintf")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "_stdio.h"

int _sprintfputchar (FargT bufptrptr, char c)
{
    *(*(char**)bufptrptr)++ = c;
    return (int) c;
}

// end of file

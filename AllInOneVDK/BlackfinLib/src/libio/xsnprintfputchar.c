/************************************************************************
 *
 * xsnprintfputchar.c : $Revision: 3544 $
 *
 * (c) Copyright 2005 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=_snprintfputchar")
#pragma file_attr(  "libFunc=__snprintfputchar")
#pragma file_attr(  "libFunc=snprintf")
#pragma file_attr(  "libFunc=_snprintf")
#pragma file_attr(  "libFunc=vsnprintf")
#pragma file_attr(  "libFunc=_vsnprintf")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "_stdio.h"

extern int
_snprintfputchar (FargT bufptrptr, char c)
{

    FILE_buff *buff_description = bufptrptr;

    if (buff_description->pos < buff_description->n)
        buff_description->buf[buff_description->pos++] = c;

    return (int) c;
}

// end of file

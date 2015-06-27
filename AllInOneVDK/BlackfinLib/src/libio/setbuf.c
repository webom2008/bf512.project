/************************************************************************
 *
 * setbuf.c : $Revision: 4 $
 *
 * (c) Copyright 1998-2003 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=setbuf")
#pragma file_attr(  "libFunc=_setbuf")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "_stdio.h"

#include "stdio.h"
#include "stdlib.h"

void
setbuf(FILE* str, char* buf)
{
    setvbuf(str, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}

// end of file

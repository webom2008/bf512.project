/************************************************************************
 *
 * rewind.c : $Revision: 3544 $
 *
 * (c) Copyright 2003-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=rewind")
#pragma file_attr(  "libFunc=_rewind")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdio.h>
#include "_stdio.h"

void rewind (FILE *str)
{	
  /* Check for NULL pointer */
  if (!str)
    return;

  _LOCK_FILE(str);
  (void) _doseek (str, 0L, SEEK_SET);
  str->_Mode &= ~M_ERROR;
  _UNLOCK_FILE(str);
}

// end of file

/************************************************************************
 *
 * clearerr.c : $Revision: 3543 $
 *
 * (c) Copyright 1998-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libFunc=clearerr")
#pragma file_attr(  "libFunc=_clearerr")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "_stdio.h"

void clearerr(FILE *str)
{
  // Just return if the stream is NULL
  if (!str) return;

  _LOCK_FILE(str);
  
  str->_Mode &= ~M_EOF;       /* reset EOF indicator   */
  str->_Mode &= ~M_ERROR;     /* reset error indicator */

  _UNLOCK_FILE(str);
}

// end of file

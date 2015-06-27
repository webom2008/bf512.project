/************************************************************************
 *
 * gmtime.c : $Revision: 3542 $
 *
 * (c) Copyright 2002-2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :  gmtime() function for C standard time.h header
 */
#pragma file_attr( "libGroup=time.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=gmtime")
#pragma file_attr(  "libFunc=_gmtime")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <time.h>
#include "xget_tm.h"

struct tm *
gmtime(const time_t *secs)
{

  /* Converts a calendar time into a broken-down time in terms of
   * Greenwich Mean Time (Coordinated Universal Time or UTC)
   */

  return __broken_down_time(*secs, 1, NULL);
}

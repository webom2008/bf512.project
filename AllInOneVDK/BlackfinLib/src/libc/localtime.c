/************************************************************************
 *
 * localtime.c : $Revision: 3542 $
 *
 * (c) Copyright 2002-2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr( "libGroup=time.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=localtime")
#pragma file_attr(  "libFunc=_localtime")
#pragma file_attr(  "libFunc=ctime")
#pragma file_attr(  "libFunc=_ctime")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

/*
 * Description :  localtime() function for C standard time.h header
 */

#include <time.h>
#include "xget_tm.h"

struct tm *
localtime(const time_t *secs)
{

  /* Converts a calendar time into a broken-down time in terms of
   * local time. This implementation of time.h does not support either
   * daylight saving or time zones and hence this function always
   * converts the calendar time as Greenwich Mean Time (UTC).
   */

  return __broken_down_time(*secs, 0, NULL);
}

/************************************************************************
 *
 * asctime.c : $Revision: 3542 $
 *
 * (c) Copyright 2002-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description: Implementation of the asctime() function for C standard 
 *              time.h header
 */

#pragma file_attr( "libGroup=time.h")
#pragma file_attr(  "libName=libc")
#if defined(__ADSPBLACKFIN__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=_asctime")
#pragma file_attr(  "libFunc=asctime")
#else
#pragma file_attr(  "libFunc=__asctimed")
#pragma file_attr(  "libFunc=_asctimed")
#endif
#else
#if defined(__ADSP21000__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=__asctimef")
#pragma file_attr(  "libFunc=asctimef")
#else
#pragma file_attr(  "libFunc=__asctimed")
#pragma file_attr(  "libFunc=_asctimed")
#endif
#else /* platform not valid */
#warning please sort out attributes for your platform
#endif
#endif
#pragma file_attr(  "libFunc=ctime")
#pragma file_attr(  "libFunc=_ctime")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
#include <time.h>
#include <stdio.h>

#include "xsync.h"


typedef char buf_t[26];
static _TLV_INLINE _TLV_DEF(__asctime_tlv_index, buf_t, buf);

char
*asctime(const struct tm *t)
{

/* Return a pointer to a text string which represents the broken-down
 * time pointed to by the argument t.
 */

  char*   pbuf = _TLV(buf);

  static const char day_name[7][3] = {
    "Sun" , "Mon" , "Tue" , "Wed" ,
    "Thu" , "Fri" , "Sat"
  };

  static const char mon_name[12][3] = {
    "Jan" , "Feb" , "Mar" , "Apr" ,
    "May" , "Jun" , "Jul" , "Aug" ,
    "Sep" , "Oct" , "Nov" , "Dec"
  };

  sprintf(pbuf, "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",
    day_name[t->tm_wday],
    mon_name[t->tm_mon],
    t->tm_mday,
    t->tm_hour,
    t->tm_min,
    t->tm_sec,
    t->tm_year + 1900);

  return pbuf;
}

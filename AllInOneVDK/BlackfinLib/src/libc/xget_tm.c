/************************************************************************
 *
 * xget_tm.c : $Revision: 3542 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description : Internal run time library function to convert a
 *               calendar time into a <tm struct> used by time.h.
 */

#pragma file_attr( "libGroup=time.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=__broken_down_time")
#pragma file_attr(  "libFunc=___broken_down_time")
#pragma file_attr(  "libFunc=gmtime")
#pragma file_attr(  "libFunc=_gmtime")
#pragma file_attr(  "libFunc=localtime")
#pragma file_attr(  "libFunc=_localtime")
#pragma file_attr(  "libFunc=mktime")
#pragma file_attr(  "libFunc=_mktime")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
#include <time.h>

#include "xget_tm.h"
#include "xdays_to_mon.h"

#include "xsync.h"

#define SECS_PER_DAY		86400
#define SECS_PER_HOUR		3600
#define LEAP_DAYS_TO_1970	17

static _TLV_INLINE _TLV_DEF(__tm_tlv_index, struct tm, t);

struct tm *
__broken_down_time(time_t secs, char isgmtime, struct tm *tptr)
{

/* Return a pointer to a struct tm corresponding to the calendar
 * time <c>. It is assumed that time_t is a signed long; negative
 * values are valid.
 *
 * A value of 0 for secs represents the base date which corresponds to
 *
 *         Thu Jan  1 00:00:00 1970
 *
 * and the largest calendar time is equivalent to MAX_INT which is
 * equivalent to the number of seconds in 68 years, and represents the
 * date
 *
 *         Tue Jan 19 03:14:07 2038
 *
 * The earliest date that can be represented by a calendar time based
 * on a signed long is equivalent to MIN_INT and is equivalent to a
 * date in 1902 (== 1970 - 68)
 *
 * If the parameter isgmtime is 0, then the broken-down time returned
 * is expressed as local time, otherwise the broken-down time returned
 * will be with respect to Coordinated Universal Time (UTC). This
 * implementation of time.h does not support either daylight saving or
 * time zones and hence this function always converts the calendar time
 * as Greenwich Mean Time (UTC).
 *
 */

  long days;
  long extra_days;
  int  year;               /* the year since 1900 */

  if (tptr == NULL)
    tptr = &_TLV(t);

  /* Account for days between 1900 and 1970, since base of calendar
   * time (in our implementation) is the start of 1970.
   */

  if (secs < 0)
    days = ((secs + 1) / SECS_PER_DAY) - 1;
  else
    days = secs / SECS_PER_DAY;

  days += (70 * 365L) + LEAP_DAYS_TO_1970;
  tptr->tm_wday = (days + 1) % 7;

  /* Find the year, accounting for leap days, and looping at most
   * once (note that 1900 did not have a leap day).
   */
  for (year = days / 365; ; --year) {
    extra_days = year * 365L;
    extra_days += (year > 0) ? (year - 1) / 4
                             : (year / 4);

    if (days >= extra_days) break;
  }
  days -= extra_days;
  tptr->tm_year = year;
  tptr->tm_yday = days;

  {
    const short *cur_days_to_mon = DAYS_TO_MONTH(year);
    int mon = 12;

    /* Find the month */

    while (days < cur_days_to_mon[--mon]);
    tptr->tm_mon = mon;
    /* Day of month is 1-biased */
    tptr->tm_mday = 1 + days - cur_days_to_mon[mon];
  }

  secs %= SECS_PER_DAY;
  if (secs < 0)
    secs = SECS_PER_DAY + secs;

  tptr->tm_hour = secs / SECS_PER_HOUR;
  secs %= SECS_PER_HOUR;
  tptr->tm_min = secs / 60;
  tptr->tm_sec = secs % 60;
  tptr->tm_isdst = -1;          /* we don't support the daylight saving time */

  return tptr;

}

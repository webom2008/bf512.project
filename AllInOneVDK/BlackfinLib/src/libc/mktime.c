/************************************************************************
 *
 * mktime.c : $Revision: 3542 $
 *
 * (c) Copyright 2002-2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :  This is an implementation of the mktime() function
 *                for C standard time.h header
 */

/*
 *  Since we don't know anything about daylight time (more accurately,
 *  the alternate time zone), stop adjusting the return time_t value to
 *  reflect a one-hour advance when the incoming tm_isdst flag is postitive.
 *  Set the tm_isdst flag to -1 to indicate we don't know what the daylight
 *  time state is (and to agree with localtime()). As a result, the time
 *  functions now don't use the incoming tm_isdst value: all incoming
 *  struct tm values are interpreted as standard time, and all outgoing
 *  values are standard time.
 */

#pragma file_attr( "libGroup=time.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=mktime")
#pragma file_attr(  "libFunc=_mktime")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")
#include <time.h>
#include <limits.h>

#include "xget_tm.h"
#include "xdays_to_mon.h"

#define SECS_PER_HOUR		(60 * 60)
#define SECS_PER_DAY		((SECS_PER_HOUR) * 24)
#define SECS_PER_YEAR		((SECS_PER_DAY) * 365)

#if defined(__ADSPTS__) || defined(__ADSPBLACKFIN__)

typedef long long int work_t;

	/* The arithmetic required to decompose a broken-down time into a
	** calendar time will be performed using 64-bit fixed point arithmetic
	** with operands that are based on 32-bit data - these operations will
	** not overflow
	*/

#define CHECK_LONG_ADD_OVERFLOW(LHS,RHS)
#define CHECK_INT_ADD_OVERFLOW(LHS,RHS)
#define CHECK_INT_MUL_OVERFLOW(LHS,CONST)

#define CHECK_TIME_T_OVERFLOW(T)            \
  if (((T) > INT_MAX) || ((T) < INT_MIN)) { \
    return (time_t)-1;                      \
  }

#elif defined(__ADSP21000__)

typedef int work_t;

	/* On SHARC, the arithmetic performed by the mktime function will be
	** based on 32-bit fixed point arithmetic - we shall therefore have
	** to check for overflow
	 */

/* Macros to test if arithmetic with two work variables will overflow */

#define CHECK_INT_ADD_OVERFLOW(LHS,RHS)               \
  if ( ((LHS) > 0) && ((INT_MAX - (LHS)) < (RHS)) ||  \
       ((LHS) < 0) && ((INT_MIN - (LHS)) > (RHS)) ) { \
    return (time_t)-1;                                \
  }

#define CHECK_INT_MUL_OVERFLOW(LHS,CONST)               \
  if ( ((LHS) > 0) && ((INT_MAX / (CONST)) < (LHS)) ||  \
       ((LHS) < 0) && ((INT_MIN / (CONST)) > (LHS)) ) { \
    return (time_t)-1;                                  \
  }

#define CHECK_LONG_ADD_OVERFLOW(LHS,RHS) CHECK_INT_ADD_OVERFLOW(LHS,RHS)
#define CHECK_TIME_T_OVERFLOW(T)

#else

#error "This architecture is not supported"

#endif


static int
get_extra_days(int year, int month)
{

  /* Find the number of "extra" days required to get to the beginning
  ** of the specified month; the year is assumed to be relative to 1970.
  ** "Extra" days are those beyond an integral number of 365-day years.
  */

  int leap_days;

  /* Leap years are taken to be years divisible by 4 except
   * 1900. Reasonable in [1801,2099]. We don't do leap seconds.
   */

  if (year > 0)
    leap_days = (year + 1) / 4;
  else
    leap_days = (year - 2) / 4;

  return DAYS_TO_MONTH(1970 + year)[month] + leap_days;

/* For example:
**
**     for 1972, year = 2 and leap_days = 0 and DAYS_TO_MONTH uses _ldays_to_mon
**     for 1973, year = 3 and leap_days = 1 and DAYS_TO_MONTH uses _days_to_mon
*/

}


time_t
mktime(struct tm *t)
{

  /* Return a calendar time corresponding to the values in the struct
  ** tm * argument, interpreted as local time. (However this implementation
  ** of time.h does not support either daylight saving or time zones and
  ** hence this function always interpret the argument as Greenwich Mean
  ** Time (UTC)). The tm_wday and tm_yday fields are ignored on input, and
  ** the other fields can have values outside their normal ranges.
  **
  ** On return, the fields of the struct should in their normal ranges.
  **
  ** Return -1 if the time can't be represented.
  */

  int years_in_mon;            /* number of years represented in t->tm_mon */
  int tm_mon;                  /* t->tm_mon within the defined range       */
  int leap_days_in_years;

  work_t tm_year;              /* t->tm_year with bias (of 1970) removed   */
  work_t years;                /* years represented by tm_year + tm_mon    */

  work_t w1;                   /* work variables for  */
  work_t w2;                   /*      decomposing *t */

  time_t cal_time;             /* the result variable */

  /* Use tm_year and tm_mon to work out the year */

  years_in_mon = t->tm_mon / 12;
  tm_mon = t->tm_mon - (years_in_mon * 12);
  if (tm_mon < 0) {
    tm_mon += 12;
    years_in_mon--;
  }

  tm_year = t->tm_year - 70;
  CHECK_LONG_ADD_OVERFLOW(tm_year,years_in_mon);
  years = tm_year + years_in_mon;

  /* Work out the extra leap days included in the year */

  leap_days_in_years = get_extra_days(years,tm_mon);

  /* Decompose broken-down time into a calendar time */

  CHECK_INT_MUL_OVERFLOW(leap_days_in_years,SECS_PER_DAY);
  w1 = leap_days_in_years * SECS_PER_DAY;

  CHECK_INT_MUL_OVERFLOW(years,SECS_PER_YEAR)
  w2 = years * SECS_PER_YEAR;

  CHECK_INT_ADD_OVERFLOW(w1,w2);
  w2 += w1;

  CHECK_INT_MUL_OVERFLOW(t->tm_mday,SECS_PER_DAY)
  w1 = (t->tm_mday - 1) * SECS_PER_DAY;

  CHECK_INT_ADD_OVERFLOW(w1,w2);
  w2 += w1;

  CHECK_INT_MUL_OVERFLOW(t->tm_hour,SECS_PER_HOUR)
  w1 = t->tm_hour * SECS_PER_HOUR;

  CHECK_INT_ADD_OVERFLOW(w1,w2);
  w2 += w1;

  CHECK_INT_MUL_OVERFLOW(t->tm_min,60)
  w1 = t->tm_min * 60;

  CHECK_INT_ADD_OVERFLOW(w1,w2);
  w2 += w1;

  CHECK_INT_ADD_OVERFLOW(w2,t->tm_sec);
  w2 += t->tm_sec;

  /* Apply the effects of any Daylight Saving Time */

  if (t->tm_isdst > 0) {
    CHECK_INT_ADD_OVERFLOW(w2,(-1) * SECS_PER_HOUR);
    w2 += (-1) * SECS_PER_HOUR;
  }

  /* Check the decomposed time is valid */

  CHECK_TIME_T_OVERFLOW(w2)
  cal_time = w2;

  /* and normalise the broken-down time */

  __broken_down_time(cal_time, 0, t);
  return cal_time;

}

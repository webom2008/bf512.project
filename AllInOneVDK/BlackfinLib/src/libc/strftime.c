/************************************************************************
 *
 * strftime.c : $Revision: 4 $
 *
 * (c) Copyright 2002-2005 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :  This is an implementation of the strftime() function
 *                for C standard time.h header
 */
#pragma file_attr( "libGroup=time.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=strftime")
#pragma file_attr(  "libFunc=_strftime")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <time.h>
#include <string.h>


static const char *const long_day[7] = {
  "Sunday"   , "Monday"  ,"Tuesday",
  "Wednesday", "Thursday", "Friday",
  "Saturday"
};

static const char *const long_mon[12] = {
  "January", "February", "March"    ,
  "April"  , "May"     , "June"     ,
  "July"   , "August"  , "September",
  "October", "November", "December"
};

static size_t strftime_get_length(const char *format, const struct tm *t);
static void   strftime_int2dec(char **bufp, int val, int digits, char filler);
static void do_strftime(char *buf, const char *format, const struct tm *t);


extern size_t
strftime(char *buf, size_t buf_size, const char *format, const struct tm *t)
{
  /* Format the broken-down time <t> into <buf> according to the format
  ** string <format>. If the formatted date, including terminating null,
  ** fits in <buf_size> characters, return the size of formatted date
  ** (excluding the terminating null); otherwise return 0. Only the
  ** "C" locale is supported.
  **
  **     Note this implementation of time.h does not know about time zones.
  **
  ** Extensions that are supported by this function are:
  **
  **     %C  The century of the year (POSIX.2-1992 and ISO C99)
  **     %D  The date represented as mm/dd/yy (POSIX.2-1992 and ISO C99)
  **     %e  The day of the month, padded with a space character (compare
  **         with %d) (POSIX.2-1992 and ISO C99)
  **     %F  The date represented as yyyy-mm-dd (POSIX.1-2001 and ISO C99)
  **     %h  The same as %b - the abbreviated name of the month (POSIX.2-199 s
  **         2 and ISO C99)
  **     %k  The hour as a 24-hour clock, padded with a space character (GNU
  **         extension)
  **     %l  The hour as a 12-hour clock, padded with a space character (GNU
  **         extension)
  **     %n  The newline character (POSIX.2-1992 and ISO C99)
  **     %P  Either am or pm (compare with %p which is either AM or PM) (GNU
  **         extension)
  **     %R  The time represented as hh:mm (POSIX.1-2001 and ISO C99)
  **     %r  The time represented as hh:mm:ss {A|P}M (POSIX.2-1992 and ISO C99)
  **     %T  The time represented using %H:%M:%S (POSIX.2-1992 and ISO C99)
  **     %t  The tab character (POSIX.2-1992 and ISO C99)
  */

  size_t size = strftime_get_length(format, t);

  if (buf_size <= size)
    return 0;

  do_strftime(buf, format, t);

  return size;
}


static size_t
strftime_get_length(const char *format, const struct tm *t)
{

  /* Compute the size of the formatted string, excluding the
  ** terminating null.
  */

  size_t size = 0;

  while (1) {
    switch (*format++) {
      case '\0':
        return size;
      case '%':
        switch (*format++) {

          case '\0': /* '%' not followed by letter -- illegal */
            return size + 1; /* we will print '%' */

          case 'a': /* Sun */
          case 'b': /* Jan */
          case 'h': /* Jan (POSIX.2-1992 and ISO C90) */
          case 'j': /* day of year */
            size += 3;
            break;

          case 'A': /* Sunday */
            size += strlen(long_day[t->tm_wday]);
            break;

          case 'B': /* January */
            size += strlen(long_mon[t->tm_mon]);
            break;

          case 'c': /* Tue Jan 01 01:01:01 1970 */
            size += 24;
            break;

          case 'C': /* century */
          case 'd': /* day of month */
          case 'e': /* day of month but padded with a space character */
          case 'H': /* hour of 24 hr day */
          case 'I': /* hour of 12 hr day */
          case 'k': /* hour of 24 hr day but padded with a space character */
          case 'l': /* hour of 12 hr day but padded with a space character */
          case 'm': /* month of the year */
          case 'M': /* minutes after hour */
          case 'P': /* am/pm */
          case 'p': /* AM/PM */
          case 'S': /* seconds after minute */
          case 'U': /* week of year (Sunday start) */
          case 'W': /* week of year (Monday start) */
          case 'y': /* year of century */
            size += 2;
            break;

          case 'w': /* day of week */
            ++size;
            break;

          case 'D': /* date as mm/dd/yy */
          case 'x': /* date representation for the current locale - mm/dd/yy */
          case 'T': /* time representation using %H:%M:%S */
          case 'X': /* 01:01:01 */
            size += 8;
            break;

          case 'F': /* date as yyyy-mm-dd */
            size += 10;
            break;

          case 'R': /* hh:mm */
            size += 5;
            break;

          case 'r': /* hh:mm:ss {A|P}M */
            size += 11;
            break;

          case 'Y': /* 1970 */
            size += 4;
            break;

          case 'Z': /* time zone -- not supported */
            break;

          case 'n': /* newline character */
          case 't': /* the tab character */
          case '%': /* the '%' character */
            ++size;
            break;

          default: /* illegal -- we will print '%' followed by letter */
            size += 2;
            break;
        }
        break;
      default: /* not part of a conversion specifier */
        ++size;
    }
  }
}


static void
do_strftime(char *buf, const char *format, const struct tm *t)
{

  /* Format the time info into the buffer -- no checking for overflow */

  int tval;

  while (1) {
    switch (*format++) {
      case '\0':
        *buf = '\0';
        return;
      case '%':
        switch (*format++) {

          case '\0': /* '%' not followed by letter -- illegal */
            *buf++ = '%';
            *buf = '\0';
            return;

          case 'A': /* Sunday */
            strcpy(buf, long_day[t->tm_wday]);
            buf += strlen(long_day[t->tm_wday]);
            break;

          case 'a': /* Sun */
            memcpy(buf, long_day[t->tm_wday], 3);
            buf += 3;
            break;

          case 'B': /* January */
            strcpy(buf, long_mon[t->tm_mon]);
            buf += strlen(long_mon[t->tm_mon]);
            break;

          case 'b': /* Jan */
          case 'h': /* Jan (POSIX.2-1992 and ISO C90) */
            memcpy(buf, long_mon[t->tm_mon], 3);
            buf += 3;
            break;

          case 'C': /* century */
            tval = (t->tm_year + 1900) / 100;
            goto decimal_2_z;

          case 'c': /* Tue Jan 01 01:01:01 1970 */
            do_strftime(buf, "%a %b %d %X %Y", t);
            buf += 24;
            break;

          case 'd': /* day of month */
            tval = t->tm_mday;
            goto decimal_2_z;

          case 'e': /* day of the month padded with a space character */
            tval = t->tm_mday;

decimal_2_b:
            strftime_int2dec(&buf, tval, 2, ' ');
            break;

          case 'F': /* date as yyyy-mm-dd */
            do_strftime(buf, "%Y-%m-%d", t);
            buf += 10;
            break;

          case 'H': /* hour of 24 hr day */
            tval = t->tm_hour;
            goto decimal_2_z;

          case 'I': /* hour of 12 hr day */
            tval = t->tm_hour;
            if (tval == 0)
              tval = 12;
            else if (tval != 12)
              tval %= 12;
            goto decimal_2_z;

          case 'j': /* day of year */
            strftime_int2dec(&buf, t->tm_yday + 1, 3, '0');
            break;

          case 'k': /* hour of 24 hr day padded with a space character */
            tval = t->tm_hour;
            goto decimal_2_b;

          case 'l': /* hour of 12 hr day padded with a space character */
            tval = t->tm_hour;
            if (tval == 0)
              tval = 12;
            else if (tval != 12)
              tval %= 12;
            goto decimal_2_b;

          case 'M': /* minutes after hour */
            tval = t->tm_min;
            goto decimal_2_z;

          case 'm': /* month of the year */
            tval = t->tm_mon + 1;
            goto decimal_2_z;

          case 'n': /* newline character */
            *buf++ = '\n';
            break;

          case 'P': /* am/pm */
            *buf++ = (t->tm_hour < 12) ? 'a' : 'p';
            *buf++ = 'm';
            break;

          case 'p': /* AM/PM */
            *buf++ = (t->tm_hour < 12) ? 'A' : 'P';
            *buf++ = 'M';
            break;

          case 'R': /* hh:mm */
            do_strftime(buf, "%H:%M", t);
            buf += 5;
            break;

          case 'r': /* hh:mm:ss {A|P}M */
            do_strftime(buf, "%I:%M:%S %p", t);
            buf += 11;
            break;

          case 'S': /* seconds after minute */
            tval = t->tm_sec;
            goto decimal_2_z;

          case 't': /* the tab character */
            *buf++ = '\t';
            break;

          case 'U': /* week of year (Sunday start) */
            tval = (t->tm_yday - t->tm_wday + 7) / 7;
            goto decimal_2_z;

          case 'W': /* week of year (Monday start) */
            tval = (t->tm_wday + 6) % 7;
            tval = (t->tm_yday - tval + 7) / 7;
            goto decimal_2_z;

          case 'y': /* year of century */
            tval = t->tm_year % 100;
            goto decimal_2_z;

decimal_2_z:
            strftime_int2dec(&buf, tval, 2, '0');
            break;

          case 'w': /* day of week */
            strftime_int2dec(&buf, t->tm_wday, 1, '0');
            break;

          case 'D': /* 01/25/70 */
          case 'x': /* 01/25/70 */
            do_strftime(buf, "%m/%d/%y", t);
            buf += 8;
            break;

          case 'T': /* time representation using %H:%M:%S */
          case 'X': /* 01:01:01 */
            do_strftime(buf, "%H:%M:%S", t);
            buf += 8;
            break;

          case 'Y': /* 1970 */
            strftime_int2dec(&buf, t->tm_year + 1900, 4, '0');
            break;

          case 'Z': /* time zone -- not supported */
            break;

          case '%': /* the '%' character */
            *buf++ = '%';
            break;

          default: /* illegal -- we will print '%' followed by letter */
            *buf++ = '%';
            *buf++ = *(format - 1);
            break;
        }
        break;
      default: /* not part of a conversion specifier */
        *buf++ = *(format - 1);
        break;
    }
  }
}


static void
strftime_int2dec(char **bufp, int val, int digits, char filler)
{

  /* Convert an int value into ascii form. <bufp> is a pointer to a pointer
  ** to the first location in the destination buffer. If <val> is negative,
  ** results are very strange - one could get non-printing characters - but
  ** that won't happen for valid input. If <val> won't fit in <digits> digits,
  ** it gets truncated. We always write exactly <digits> digits. If <val> is
  ** smaller in scale than digits, then <val> will be padded with <filler> in
  ** the destination buffer.
  **
  ** Note that <*bufp> is updated to point to the next unwritten position in
  ** the buffer.
  **/

  char *buf = *bufp;

  *bufp += digits;
  buf[--digits] = val % 10 + '0'; /* ones digit */

  while (digits--)
    buf[digits] = (val < 10) ? filler
                             : (val /= 10) % 10 + '0';

}

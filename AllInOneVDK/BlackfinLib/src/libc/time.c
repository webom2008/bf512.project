/************************************************************************
 *
 * time.c : $Revision: 4 $
 *
 * (c) Copyright 2002 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :  This is an implementation of the time() function 
 *                for C standard time.h header
 */

#pragma file_attr( "libGroup=time.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=time")
#pragma file_attr(  "libFunc=_time")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
#include <time.h>

/* Return the current calendar time. We always return -1,
 * indicating that the calendar time is not available (as allowed
 * by the C standard). To support determining the current calendar
 * date and time would require an API to set the current date and
 * time, plus use of the timer to track clock time.
 */
time_t time(time_t *t)
{
  if (0 != t) 
    *t = (time_t)-1;

  return (time_t)-1;
}

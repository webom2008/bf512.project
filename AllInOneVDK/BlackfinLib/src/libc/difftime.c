/************************************************************************
 *
 * difftime.c : $Revision: 3542 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Implementation of the  difftime() function for
 *                 C standard time.h header
 */

#pragma file_attr( "libGroup=time.h")
#pragma file_attr(  "libName=libc")

#if defined(__ADSPBLACKFIN__)
# if defined(__DOUBLES_ARE_FLOATS__)
#  pragma file_attr(  "libFunc=_difftime")
#  pragma file_attr(  "libFunc=difftime")
# else
#  pragma file_attr(  "libFunc=__difftimed")
#  pragma file_attr(  "libFunc=_difftimed")
# endif
#elif defined(__ADSP21000__)
# if defined(__DOUBLES_ARE_FLOATS__)
#  pragma file_attr(  "libFunc=__difftimef")
#  pragma file_attr(  "libFunc=difftimef")
# else
#  pragma file_attr(  "libFunc=__difftimed")
#  pragma file_attr(  "libFunc=_difftimed")
# endif
#else /* platform not valid */
# warning please sort out attributes for your platform
#endif

#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#include <time.h>
#include <limits.h>

/* Return the difference <t1> - <t0> between two calendar times.
 */

extern double
difftime(time_t t1, time_t t0)
{

/* This function calculates the difference between two calendar times.
**
** By default, the double data type represents a 32-bit, single precision,
** floating-point, value - this data type is normally insufficient for
** preserving all of the bits associated with the difference between two
** calendar times, particularly if the difference represents more than 97
** days. It is recommended therefore that the switch -double-size-64 is
** used if calling this function.
**
** (It assumes that the time_t type is a signed data type).
*/

#if defined(__ADSPTS__) || defined(__ADSPBLACKFIN__)
    long long int diff = (t1 - t0);

    /* If 64-bit fixed-point arithmetic is available, then use this data
    ** type to cope with the maximum possible difference between two values
    ** of type time_t
    */

#else
    long int diff = (t1 - t0);
#endif

   return (double)(diff);

}

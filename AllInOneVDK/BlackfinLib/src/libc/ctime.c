/************************************************************************
 *
 * ctime.c : $Revision: 3542 $
 *
 * (c) Copyright 2002-2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :  This function implements the ctime() function for
 *                C standard time.h header
 */

#pragma file_attr( "libGroup=time.h")
#pragma file_attr(  "libName=libc")

#if defined(__ADSPBLACKFIN__)
# if defined(__DOUBLES_ARE_FLOATS__)
#  pragma file_attr(  "libFunc=_ctime")
#  pragma file_attr(  "libFunc=ctime")
# else
#  pragma file_attr(  "libFunc=__ctimed")
#  pragma file_attr(  "libFunc=_ctimed")
# endif
#elif defined(__ADSP21000__)
# if defined(__DOUBLES_ARE_FLOATS__)
#  pragma file_attr(  "libFunc=__ctimef")
#  pragma file_attr(  "libFunc=ctimef")
# else
#  pragma file_attr(  "libFunc=__ctimed")
#  pragma file_attr(  "libFunc=_ctimed")
# endif
#else /* platform not valid */
# warning please sort out attributes for your platform
#endif

#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <time.h>

extern char *
ctime(const time_t *t)
{

  /* Return a pointer to a string representing the calendar time <t> */

  return asctime(localtime(t));
}

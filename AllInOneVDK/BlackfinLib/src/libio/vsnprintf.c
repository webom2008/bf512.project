/************************************************************************
 *
 * vsnprintf.c : $Revision: 3544 $
 *
 * (c) Copyright 2005-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)

#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libio")

#if defined(__ADSPBLACKFIN__)
# pragma file_attr( "libGroup=stdio_bf.h")
# if defined(__DOUBLES_ARE_FLOATS__)
#  pragma file_attr(  "libFunc=vsnprintf")
#  pragma file_attr(  "libFunc=_vsnprintf")
# else
#  pragma file_attr(  "libFunc=vsnprintf")
#  pragma file_attr(  "libFunc=_vsnprintf64")
#  pragma file_attr(  "libFunc=__vsnprintf64")
# endif

#elif defined(__ADSP21000__)
# pragma file_attr( "libGroup=stdio_21xxx.h")
# if defined(__DOUBLES_ARE_FLOATS__)
#  pragma file_attr(  "libFunc=vsnprintf")
#  pragma file_attr(  "libFunc=_vsnprintf32")
#  pragma file_attr(  "libFunc=__vsnprintf32")
# else
#  pragma file_attr(  "libFunc=vsnprintf")
#  pragma file_attr(  "libFunc=_vsnprintf64")
#  pragma file_attr(  "libFunc=__vsnprintf64")
# endif
#else /* platform not valid */

#warning please sort out attributes for your platform
#endif

#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#endif

#include "_stdio.h"

extern int _snprintfputchar(FargT, char);

int
vsnprintf(char *buf, size_t n, const char *fmt, va_list ap)
{
    int outsize;

    FILE_buff buff_description;

    /* Return failure if any NULL pointers present */

    if ((fmt == NULL) || (ap == NULL))
      return -1;

/* note that this function is not made into an atomic operation
** in a multithreaded environment because it does not access any
** streams - this requires that all underlying formatting routines
** are thread-safe.
*/

    /* Perform formatting */

    buff_description.buf = buf;    /* ptr to output array                  */
    buff_description.n   = n;      /* size of output array                 */
    buff_description.pos = 0;      /* current position within output array */

    outsize = _doprnt(fmt, ap, _snprintfputchar, &buff_description);

    /* Tidy-up */

    if (n)
    {
        if (buf == NULL)
            return 0;

        if (n > outsize)
            buf[outsize] = '\0';
        else
            buf[n - 1] = '\0';
    }

    return outsize;
}

// end of file

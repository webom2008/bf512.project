/************************************************************************
 *
 * xprnt.c : $Revision: 3802 $
 *
 * (c) Copyright 1998-2005 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr(  "libName=libio")
#if defined(__ADSPBLACKFIN__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=_doprnt_32")
#pragma file_attr(  "libFunc=__doprnt_32")
#else
#pragma file_attr(  "libFunc=__doprnt_64")
#pragma file_attr(  "libFunc=___doprnt_64")
#endif
#else /* platform not valid */
#if defined(__ADSP21000__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=_doprnt_32")
#pragma file_attr(  "libFunc=__doprnt_32")
#else
#pragma file_attr(  "libFunc=_doprnt_64")
#pragma file_attr(  "libFunc=__doprnt_64")
#endif
#endif
#endif
#pragma file_attr(  "libGroup=stdio.h")
#pragma file_attr(  "libFunc=printf")
#pragma file_attr(  "libFunc=_printf")
#pragma file_attr(  "libFunc=sprintf")
#pragma file_attr(  "libFunc=_sprintf")
#pragma file_attr(  "libFunc=fprintf")
#pragma file_attr(  "libFunc=_fprintf")
#pragma file_attr(  "libFunc=fputs")
#pragma file_attr(  "libFunc=_fputs")
#pragma file_attr(  "libFunc=fputc")
#pragma file_attr(  "libFunc=_fputc")
#pragma file_attr(  "libFunc=puts")
#pragma file_attr(  "libFunc=_puts")
#pragma file_attr(  "libFunc=putc")
#pragma file_attr(  "libFunc=_putc")
#pragma file_attr(  "libFunc=putchar")
#pragma file_attr(  "libFunc=_putchar")
#pragma file_attr(  "libFunc=vsprintf")
#pragma file_attr(  "libFunc=_vsprintf")
#pragma file_attr(  "libFunc=vsnprintf")
#pragma file_attr(  "libFunc=_vsnprintf")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <string.h>

#include "xprnt.h"

int
_doprnt(const char* formatString, va_list args, FuncT func, FargT farg)
{
    const char *cp = formatString;
    FormatT format = {
        NULL, 0,0,0,0,0, "", "", "", 
        ModFlag_None, kNone, 0, false, false, false, false, false
    };
    int outsize=0;

    format.args = args;

    while (*cp != '\0') {
        if (*cp != '%') {
            (*func)(farg, *cp++);
             outsize+=1;
        } else {
            cp++;
            cp = _parse_format(cp, &format);

            switch (format.fcode) {
              case 'd':
              case 'u':
              case 'i':
                outsize+=_print_fixed(&format, 5, func, farg);
                break;
              case 'o':
                outsize+=_print_fixed(&format, 4, func, farg);
                break;
              case 'p':
              case 'X':
              case 'x':
                outsize+=_print_fixed(&format, 8, func, farg);
                break;
#if defined(__FIXED_POINT_ALLOWED)
              case 'r':
              case 'R':
#if !defined(__FX_NO_ACCUM)
              case 'k':
              case 'K':
#endif /* __FX_NO_ACCUM */
#ifdef _ADI_FX_LIBIO
                outsize+=print_fx(&format, func, farg);
#else
                format.precision = -1;
                outsize+=_print_fixed(&format, 8, func, farg);
#endif /* _ADI_FX_LIBIO */
                break;
#endif /* __FIXED_POINT_ALLOWED */
              case 'A':
              case 'a':
                outsize+=_print_a_float(&format, func, farg);
                break;
              case 'G':
              case 'g':
              case 'e':
              case 'E':
              case 'f':
              case 'F':
                outsize+=_print_float(&format, func, farg);
                break;

              case 'c':
                {
                    char ch = FETCH((&format), int);
                    if ( format.width>0 ) {
                        char chbuf[2];
                        chbuf[0] = ch;
                        chbuf[1] = '\0';
                        format.buffer = chbuf;
                        outsize += _do_output(&format,func,farg);
                    } else {
                        (*func)(farg, ch);
                        outsize+=1;
                    }
                }
                break;

              case 'n':
                {
                    void* ptr;

                    ptr = FETCH((&format), void*);
                    if (format.modFlag == ModFlag_l) {
                        *((long *) ptr) = outsize;
                    } else
#if _LONG_LONG && !defined(__LIBIO_LITE)
                    if (format.modFlag == ModFlag_ll) {
                        *((long long *) ptr) = outsize;
                    } else
#endif
                    if (format.modFlag == ModFlag_h) {
                        *((short *) ptr) = outsize;
                    } else {
                        *((int *) ptr) = outsize;
                    }
                }
                break;

              case 's':
                {
                    char* s;

                    s = FETCH((&format), char*);
                    if (s == NULL)
                        s = "(null)";
                    format.buffer=s;
                    if ((format.width==0) && (format.precision<0)) {
                        /* Faster route for "%s" */
                        format.precision = strlen(s);
                        _put_string( func,farg,s,format.precision );
                        outsize+=format.precision;
                    } else {
                        /* Standard route for
                         * "%[flags][width].[precision]s"
                         */
                        outsize+=_do_output( &format,func,farg );
                    }
                }
                break;
                
              case '\0': /* Premature end of format string */
                cp--;
                break;
          
              default: /* case '%': */
                (*func)(farg, format.fcode);
                outsize+=1;
                break;
            }
        }
    }
    return outsize;
}

// end of file

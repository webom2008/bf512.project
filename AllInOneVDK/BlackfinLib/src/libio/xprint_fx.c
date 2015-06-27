/************************************************************************
 *
 * xprint_fx.c : $Revision: 1.8.4.1 $
 *
 * (c) Copyright 2008-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr ("libName=libio")
#pragma file_attr ("libGroup=stdio.h")

#pragma file_attr ("libFunc=_print_fx")
#pragma file_attr ("libFunc=__print_fx")
#pragma file_attr ("libFunc=printf")
#pragma file_attr ("libFunc=_printf")
#pragma file_attr ("libFunc=sprintf")
#pragma file_attr ("libFunc=_sprintf")
#pragma file_attr ("libFunc=fprintf")
#pragma file_attr ("libFunc=_fprintf")
#pragma file_attr ("libFunc=fputs")
#pragma file_attr ("libFunc=_fputs")
#pragma file_attr ("libFunc=fputc")
#pragma file_attr ("libFunc=_fputc")
#pragma file_attr ("libFunc=puts")
#pragma file_attr ("libFunc=_puts")
#pragma file_attr ("libFunc=putc")
#pragma file_attr ("libFunc=_putc")
#pragma file_attr ("libFunc=putchar")
#pragma file_attr ("libFunc=_putchar")
#pragma file_attr ("libFunc=vsprintf")
#pragma file_attr ("libFunc=_vsprintf")
#pragma file_attr ("libFunc=vsnprintf")
#pragma file_attr ("libFunc=_vsnprintf")

#pragma file_attr ("prefersMem=external")
#pragma file_attr ("prefersMemNum=70")
#endif

#include <string.h>
#include <stdfix.h>
#include "xprnt.h"
#include "xlua_to_string.h"

#if MAXCVT < 40
/* 40 is 32 fract digits, 6 int digits, decimal point and null terminator
*/
#error MAXCVT too small
#endif

int
print_fx(FormatT* format, FuncT func, FargT farg)
{
    char _cvtbuf[MAXCVT+1];
    int saturated = 0;

    int signed_fcode = (format->fcode == 'r') || (format->fcode == 'k');
#if !defined(__FX_NO_ACCUM)
    int is_accum = (format->fcode == 'k') || (format->fcode == 'K');
#endif
    largest_signed_fixed_point_t val;
    largest_unsigned_fixed_point_t uval;

    /* default precision to 6 if it is not specified */
    if (format->precision < 0)
        format->precision = 6;

    /* Extract the value from printf's argument list */

#if !defined(__FX_NO_ACCUM)
    if (is_accum)
    {
#if ACCUM_FBIT != LACCUM_FBIT || ACCUM_IBIT != LACCUM_IBIT
        if (format->modFlag == ModFlag_l)
        {
            if (signed_fcode)
            {
                val = FETCH(format, long _Accum);
            }
            else
            {
                uval = FETCH(format, unsigned long _Accum);
            }
        }
        else
#endif
#if ACCUM_FBIT != SACCUM_FBIT || ACCUM_IBIT != SACCUM_IBIT
        if (format->modFlag == ModFlag_h)
        {
            if (signed_fcode)
            {
                val = FETCH(format, short _Accum);
            }
            else
            {
                uval = FETCH(format, unsigned short _Accum);
            }
        }
        else
#endif
        {
            if (signed_fcode)
            {
                val = FETCH(format, _Accum);
            }
            else
            {
                uval = FETCH(format, unsigned _Accum);
            }
        }
    }
    else
#endif
    {
        unsigned int bitpattern = FETCH(format, unsigned int);
#if FRACT_FBIT != LFRACT_FBIT
        if (format->modFlag == ModFlag_l)
        {
#if LFRACT_FBIT > 32
#error This routine only works for fracts smaller or equal to int size
#endif
            if (signed_fcode)
            {
                val = lrbits((int_lr_t)bitpattern);
            }
            else
            {
                uval = ulrbits((uint_ulr_t)bitpattern);
            }
        }
        else
#endif
#if FRACT_FBIT != SFRACT_FBIT
#if SFRACT_FBIT > 32
#error This routine only works for fracts smaller or equal to int size
#endif
        if (format->modFlag == ModFlag_h)
        {
            if (signed_fcode)
            {
                val = hrbits((int_hr_t)bitpattern);
            }
            else
            {
                uval = uhrbits((uint_uhr_t)bitpattern);
            }
        }
        else
#endif
        {
#if FRACT_FBIT > 32
#error This routine only works for fracts smaller or equal to int size
#endif
            if (signed_fcode)
            {
                val = rbits((int_r_t)bitpattern);
            }
            else
            {
                uval = urbits((uint_ur_t)bitpattern);
            }
        }
    }

    /* Examine the sign of the value */

    if (signed_fcode)
    {
         int is_negative = val < (largest_signed_fixed_point_t)0;
         if (is_negative)
         {
             if (-(-val) != val)
             {
                 saturated = 1;
             }
             uval = -val;
         }
         else
         {
             uval = val;
         }
         _set_sign_prefix(format, is_negative);
    }

    format->buffer = xlua_to_string(uval, saturated, _cvtbuf,
                                    format->precision, format->bSharp);

    return _do_output(format, func, farg);
}

// end of file

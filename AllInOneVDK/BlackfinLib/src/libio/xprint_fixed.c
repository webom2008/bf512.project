/************************************************************************
 *
 * xprint_fixed.c : $Revision: 5128 $
 *
 * (c) Copyright 1998-2011 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr ("libName=libio")
#pragma file_attr ("libGroup=stdio.h")

#pragma file_attr ("libFunc=_print_fixed")
#pragma file_attr ("libFunc=__print_fixed")
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
#include "xprnt.h"
#include <limits.h>

#if defined(__FIXED_POINT_ALLOWED) && !defined(_ADI_FX_LIBIO)
#include <stdfix.h>
#include "xfxdecdata.h"
#endif

int
_print_fixed(FormatT* format, int hradix, FuncT func, FargT farg)
{
  char* bp;
  char _cvtbuf[MAXCVT+1];
  int bIsLong = (format->modFlag == ModFlag_l);
  int bIsShort = (format->modFlag == ModFlag_h);
  int bIsLongLong = (format->modFlag == ModFlag_ll);
#if _LONG_LONG && !defined(__LIBIO_LITE)
  long long ll_val;
#endif
  long val;
  int  valzero; /* always set to 0 for %p, else set to 1 if value is a zero */
  int  n;
  int  signed_fcode = (format->fcode == 'd') || (format->fcode == 'i');
  int  dontprintzero = 0;

#if defined(__LIBIO_LITE)
  if (bIsLongLong) {
      /* Consumes a long long and places the string "%ll<format>"
       * on the output buffer.
       */
      bp = _cvtbuf;
      FETCH(format, long long);
      bp[0] = '%'; 
      bp[1] = 'l'; 
      bp[2] = 'l'; 
      bp[3] = format->fcode; 
      bp[4] = '\0';
      format->buffer = bp;
      return( _do_output(format, func, farg) );
  }
#endif

#if defined(__FIXED_POINT_ALLOWED) && !defined(_ADI_FX_LIBIO)
  /* the code here is dependent on the various sizes of fractional types.
  ** Since we want to print out the values similarly to %x, we don't set
  ** signed_fcode even for the signed types.
  */ 
#if !defined(__FX_NO_ACCUM)
  int bIsAccum = 0;
  if (format->fcode == 'k' || format->fcode == 'K')
  {
      bIsAccum = bIsLongLong = 1;
  }
  else if (format->fcode == 'r' || format->fcode == 'R')
#else
  if (format->fcode == 'r' || format->fcode == 'R')
#endif
  {
      if (bIsLong && sizeof(int) == sizeof(int_lr_t))
      {
          bIsLong = 0;
      }
      else if (!bIsShort && sizeof(int) > sizeof(int_hr_t))
      {
          bIsShort = 1;
      }
  }
#endif

  if (format->precision < 0)
      format->precision = 1; /* default precision to 1 if it is not specified */

  /* Extract the value from printf's argument list */

#if _LONG_LONG && !defined(__LIBIO_LITE)
  if (bIsLongLong)
  {
      FETCH_LONG_LONG(ll_val,format);
#if defined(__FIXED_POINT_ALLOWED) && !defined(_ADI_FX_LIBIO) && !defined(__FX_NO_ACCUM)
      if (bIsAccum)
      {
          int extra_bits = 0;
          /* always zero-extend to long long */
#if ULACCUM_FBIT != UACCUM_FBIT || ULACCUM_IBIT != UACCUM_IBIT
          if (bIsLong)
          {
              extra_bits = (sizeof(ll_val) * CHAR_BIT)
                         - ULACCUM_FBIT - ULACCUM_IBIT;
          }
          else
#endif
#if USACCUM_FBIT != UACCUM_FBIT || USACCUM_IBIT != UACCUM_IBIT
          if (bIsShort)
          {
              extra_bits = (sizeof(ll_val) * CHAR_BIT)
                         - USACCUM_FBIT - USACCUM_IBIT;
          }
          else
#endif
          {
              extra_bits = (sizeof(ll_val) * CHAR_BIT)
                         - UACCUM_FBIT - UACCUM_IBIT;
          }
          ll_val <<= extra_bits;
          ll_val = (unsigned long long)ll_val >> extra_bits;
      }
#endif
  }
  else
#endif

  if (bIsLong)
  {
       val = FETCH(format, long);

  } else if (signed_fcode)
  {
       val = FETCH(format, int);
  } else {
       val = FETCH(format, unsigned);
  }

  /* Examine the sign of the value */

  if (signed_fcode)
  {
#if _LONG_LONG && !defined(__LIBIO_LITE)
    if (bIsLongLong)
    {
       _set_sign_prefix(format, ll_val < 0);
       if (ll_val < 0 && ll_val != LLHIBIT)
          ll_val = -ll_val;
    }
    else
#endif
    {
       _set_sign_prefix(format, val < 0);
       if (val < 0 && val != HIBIT)
           val = -val;
    }
  }

  if (format->fcode == 'p')
  {

      /* Process the Pointer Conversion Code */

#if defined(__ADSPBLACKFIN__)
      format->fcode = 'x';
#else
      format->fcode = 'X';
#endif
      format->bZero = 0;
      format->precision = 8;

      valzero = 0;

  } else {

      /* Process 'd' 'i' 'x' and 'o' Conversion Codes
      **
      **(don't print anything if both the precision and the value are zero
      ** unless the # flag is used with %o to force the 1st character to be
      ** a zero).
      */

#if _LONG_LONG && !defined(__LIBIO_LITE)
      if (bIsLongLong)
          valzero = (ll_val == 0);
      else
#endif
      valzero = (val == 0);

      if ((format->precision == 0) && (valzero) &&
          ((format->fcode != 'o') || (format->bSharp==0)))
          dontprintzero = 1;

      if ((!signed_fcode) && bIsShort)
      {
          unsigned long uval = val;
          uval <<= ((sizeof(uval) - sizeof(short)) * CHAR_BIT);
          uval >>= ((sizeof(uval) - sizeof(short)) * CHAR_BIT);
          val = uval;
      }
  }

  if ((format->bSharp) && (!valzero))
  {

      /* Implement the "Alternate Form" for the x and X conversion codes */

      if (format->fcode == 'x') {
          format->prefix = "0x";
      } else {
          if (format->fcode == 'X') {
              format->prefix = "0X";
          }
      }
  }

  if (format->bZero)
  {
      n = format->width - strlen(format->prefix);
      if (n > format->precision)
          format->precision = n;
  }

  if (dontprintzero) {
      format->buffer = "";
  } else {
#if _LONG_LONG && !defined(__LIBIO_LITE)
      if (bIsLongLong)
          bp = _long_long_to_string(ll_val,
                                    hradix,
                                    format->fcode == 'X', _cvtbuf, MAXDIGS+1);
       else
#endif
          bp = _long_to_string(val, 
                               hradix, 
                               format->fcode == 'X', _cvtbuf, MAXDIGS+1);

      format->buffer = bp;
      format->lzero = bp + format->precision - (_cvtbuf + MAXDIGS);
  }

  if ((format->bSharp) && (!valzero)) {

      /* Implement the "Alternate Form" for the o conversion codes */

      if (format->fcode == 'o')
      {
          if (format->lzero < 1)
              format->lzero = 1;      /* raise the precision for octal */
      }
  }
  return _do_output(format, func, farg);
}

// end of file

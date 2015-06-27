/************************************************************************
 *
 * xprint_a_float.c : $Revision: 5128 $
 *
 * (c) Copyright 2003-2011 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr(  "libName=libio")
#if defined(__ADSPBLACKFIN__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=_print_a_float_32")
#pragma file_attr(  "libFunc=__print_a_float_32")
#pragma file_attr(  "libFunc=_print_float_32")
#pragma file_attr(  "libFunc=__print_float_32")
#else
#pragma file_attr(  "libFunc=__print_a_float_64")
#pragma file_attr(  "libFunc=___print_a_float_64")
#pragma file_attr(  "libFunc=__print_float_64")
#pragma file_attr(  "libFunc=___print_float_64")
#endif
#else /* platform not valid */
#if defined(__ADSP21000__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr(  "libFunc=__print_a_float_32")
#else
#pragma file_attr(  "libFunc=__print_a_float_64")
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

#endif

#if !defined(__NOT_GNU_3_2_COMPATIBLE__)

#include <math.h>
#include "xprnt.h"

static void
_mantissa_to_hex(long val, int nibbles, int upper_case, char * buf )
{
   const char* table = upper_case ? __hex_digits_upper : __hex_digits_lower;
   
   buf[--nibbles] = '\0';

   for (; nibbles >= 0; nibbles--, val>>=4)
      buf[nibbles] = table[val & 0x0f];
}

int
_print_a_float(FormatT* format, FuncT func, FargT farg)
{
   double val;

   long   sign;
   int    exp_sign = '+';
   int    exp_char;         /* either 'p' or 'P' depending on case of %a */
   int    x_char;           /* either 'x' or 'X' depending on case of %a */
   int    sign_char;        /* char used to denote sign in the buffer    */
   int    exp;
   int    bias;
   int    upper_case;
   long   temp;
   long   biased_exponent;
   bool   precision_specified;

   long int rounding;

#if defined(__DOUBLES_ARE_FLOATS__)
   long mantissa;
#else
   char bIsLong = (format->modFlag == ModFlag_L);
#endif

#if defined(__DOUBLES_ARE_FLOATS__)
#define EXPBUF_LEN   MAXESIZ+1
#else
#define EXPBUF_LEN   MAXESIZ+2
#endif

   char   expbuf[EXPBUF_LEN];
   char*  ep;
   char   reformatbuf[MAXCVT+2];    /* allow one extra char for '.' */
   char   _cvtbuf[MAXCVT+1];

   char*  p = reformatbuf;
   char*  mp = _cvtbuf;

   char   prefixbuf[4];
   char*  pf = &prefixbuf[0];


#if !defined(__DOUBLES_ARE_FLOATS__)
#if defined(__ADSPBLACKFIN__)

   /* Little endian */
   enum byte_order { eLSW, eMSW };

#elif defined(__ADSP21000__)

   /* Big  endian */
   enum byte_order { eMSW, eLSW };

#else
#error "Must specify byte order for 64-bit long doubles."
#endif

   unsigned long int *px = ((unsigned long int *)(&val));
   unsigned long int mantissa_msw;
   unsigned long int mantissa_lsw;

#endif

#if defined(__DOUBLES_ARE_FLOATS__)
   if (format->modFlag == ModFlag_L) {
     /* Discard the 64-bit arg and print out the format specifier */
     val = FETCH(format, long double);
     format->buffer = mp;
     mp[0] = 'L'; 
     mp[1] = format->fcode; 
     mp[2] = '\0'; 
     return( _do_output(format, func, farg) );
   } 
#endif 

#if defined(__DOUBLES_ARE_FLOATS__) || !defined(__ADSPBLACKFIN__)
   /* fetch float value */
   val = FETCH(format, double);
   sign =  ((*(long *)&val) & 0x80000000);
#else
   /* fetch long double value */
   FETCH_LONG_DOUBLE(val,format);
   sign = (px[eMSW] & 0x80000000);
#endif

#if !defined(__DOUBLES_ARE_FLOATS__)
   mantissa_msw = (px[eMSW] & 0x000fffff);
   mantissa_lsw = px[eLSW];
#endif

   if (format->precision < 0)
   {
       precision_specified = false;
#if defined(__DOUBLES_ARE_FLOATS__)
       format->precision = 6;
#else
       format->precision = bIsLong ? 13 : 6;
#endif
   } else {
       precision_specified = true;
   }

   if (format->fcode == 'A')
   {
       upper_case = 1;
       exp_char = 'P';
       x_char = 'X';
   } else {
       upper_case = 0;
       exp_char = 'p';
       x_char = 'x';
   }

   _set_sign_prefix (format,(sign != 0));

   if (isnan(val))  
   {
       /* Print a '+' or a '-' or a ' ' or nothing as per our format flags.
       ** The -, + and space flags have their usual meanings even for Inf
       ** and NaN.
       */
       format->buffer = mp;
#if defined(__ADSPBLACKFIN__)
       if (upper_case)
       {
           mp[0] = 'N'; mp[1] = 'A'; mp[2] = 'N'; mp[3] = '\0';
       }
       else
       {
           mp[0] = 'n'; mp[1] = 'a'; mp[2] = 'n'; mp[3] = '\0';
       }
#else
       mp[0] = 'N'; mp[1] = 'a'; mp[2] = 'N'; mp[3] = '\0';
#endif
       /* The # and 0 flags have no effect on NaN and infinity printing 
       ** but they are not referenced again so the fields aren't explicitly 
       ** cleared.
       */
   }
   else if (isinf(val))
   {
       /* Print a '+' or a '-' or a ' ' or nothing as per our format flags.
       ** The -, + and space flags have their usual meanings even for Inf
       ** and NaN.
       */
       format->buffer = mp;
#if defined(__ADSPBLACKFIN__)
       if (upper_case)
       {
           mp[0] = 'I'; mp[1] = 'N'; mp[2] = 'F';
       }
       else
       {
           mp[0] = 'i'; mp[1] = 'n'; mp[2] = 'f';
       }
       mp[3] = '\0';
#else
       mp[0] = 'I'; mp[1] = 'n'; mp[2] = 'f';
       mp[3] = '\0';
#endif
       /* The # and 0 flags have no effect on NaN and infinity printing 
       ** but they are not referenced again so the fields aren't explicitly 
       ** cleared.
       */
   }
   else
   {

   sign_char = format->prefix[0];
   if (sign_char)
   {
      *pf++ = sign_char;
   }

   *pf++ = '0';
   *pf++ = x_char;
   *pf = '\0';

   format->prefix = prefixbuf;

#if defined(__DOUBLES_ARE_FLOATS__)
       bias = 127;
#else
       bias = 1023;
#endif
       format->buffer = reformatbuf;

#if !defined(__DOUBLES_ARE_FLOATS__) && defined(__ADSPBLACKFIN__)
       temp = px[eMSW];
#else
       temp = *(long *)&val;
#endif

#if defined(__DOUBLES_ARE_FLOATS__)
       biased_exponent = (temp >> 23) & 0xff;
       mantissa = temp & 0x7fffff; 
#else
       biased_exponent = (temp >> 20) & 0x7ff;
#endif

/* Test for zero value input or denormalized number */  
       if (biased_exponent == 0)
       {
#if defined(__DOUBLES_ARE_FLOATS__)
           if (mantissa == 0)
#else
           if ((mantissa_msw == 0) && (mantissa_lsw == 0))
#endif
           {
               /* Zero value input */
               exp = 0;
           }
           else
           {
               /* Denormalized input */ 
               exp = 1 - bias;
           }

           *p++ = '0';

       }
       else
       {
           *p++ = '1';

           exp = biased_exponent - bias;
       }

#if defined(__DOUBLES_ARE_FLOATS__)
       if ( !(mantissa || (format->precision < 6) || format->bSharp) ) 
#else
       if ( (!mantissa_msw && !mantissa_lsw) &&
             !(bIsLong && (format->precision < 13)) &&
             !(!bIsLong && (format->precision < 6)) &&
             !format->bSharp ) 
#endif
       {
           format->precision = 0;
       }
       else
       {
           /* do any rounding if needed */
#if defined(__DOUBLES_ARE_FLOATS__)
           if ( (format->precision != 0) && (format->precision < 6) ) 
           {
               rounding = (biased_exponent - ( 4 * format->precision)) - 1;
               if (rounding > 0 ) 
               {
                   rounding = (rounding << 23) | sign;
                   val = val + (*(float *)(&rounding));

                   /* Extract the mantissa and exponent 
                    * from the new rounded value 
                    */
                   temp = *(long *)&val;
                   mantissa = temp & 0x7fffff; 
                   biased_exponent = (temp >> 23) & 0xff;
                   exp = biased_exponent - bias;
               }
           }
#else
           if ( (format->precision != 0) &&
                ( (bIsLong && (format->precision < 13)) ||
                  (!bIsLong && (format->precision < 6)) ) )
           {
               rounding = (48 - (4 * format->precision)) ;
               if (rounding <= 28) 
               {
                   /* Apply rounding to the least significant 
                    * part of the mantissa 
                    */
                   mantissa_lsw = mantissa_lsw + (8 << rounding);

                   if (mantissa_lsw < px[eLSW]) 
                   {
                       /* Overflow has occurred */
                       mantissa_msw = mantissa_msw + 1;
                   }
               }
               else 
               {
                   /* Apply rounding to the most significant 
                    * part of the mantissa 
                    */
                   mantissa_msw = mantissa_msw + ( 8 << (rounding - 32));
               }

               if (mantissa_msw > 0x000fffff) 
               {
                   /* Overflow has occurred in the most significant 
                    * part of the mantissa 
                    */
                   exp = exp + 1;
                   mantissa_msw = mantissa_msw + 0x000fffff;
               }
           }
#endif
       }

       /* Convert the mantissa to a string of hexadecimal digits */
#if defined(__DOUBLES_ARE_FLOATS__)
       _mantissa_to_hex ( (mantissa << 1),6,upper_case, mp);
#else
       /* For 64-bit values we have to convert 
        * the mantissa in two stages 
        */
       _mantissa_to_hex (mantissa_msw,5,upper_case, mp);
       _mantissa_to_hex (mantissa_lsw,8,upper_case, &mp[5]);
#endif

       if (format->precision != 0  || format->bSharp) 
       {
           *p++ = '.';

           format->rzero = format->precision;
           while (format->rzero > 0 && *mp != '\0')
           {
               --format->rzero;
               *p++ = *mp;
               mp++;
           }
           while(*(p-1) == '0') {
               ++format->rzero;
               --p;
           } 
       }
       *p = '\0';

       if (exp < 0)
       {
           exp = -exp;
           exp_sign = '-';
       }

       ep = _long_to_string(exp, 5, false, expbuf, EXPBUF_LEN);
       *(--ep) = exp_sign;
       *(--ep) = exp_char;

       format->suffix = ep;

       if (precision_specified == false) {
           /* if the precision is missing trailing zeros may be omitted */
           format->rzero = 0;
       }

   }
   return( _do_output(format, func, farg) );
}

#endif  /* __NOT_GNU_3_2_COMPATIBLE__ */

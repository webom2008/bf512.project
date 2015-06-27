/************************************************************************
 *
 * xprnt.h 
 *
 * (c) Copyright 2000-2006 Analog Devices, Inc.  All rights reserved.
 * $Revision: 3544 $
 ************************************************************************/

#ifndef XPRNT_H
#define XPRNT_H

#include "_stdio.h"
#include <stdarg.h>

typedef char    Bool;

#ifndef true
#define true    1
#endif
#ifndef false
#define false   0
#endif

#define FLOAT   1

#if FLOAT

/*
   Some internal functions are sensitive to the size of type double. The
   #define's below are used to modify the names of their entry points to
   include the size of double that they support
*/

#if defined(__DOUBLES_ARE_FLOATS__)
#define _decimal_scale   _decimal_scale_32
#define _decimal_digits  _decimal_digits_32
#define _decimal_digits2 _decimal_digits2_32
#else
#define _decimal_scale   _decimal_scale_64
#define _decimal_digits  _decimal_digits_64
#define _decimal_digits2 _decimal_digits2_64
#endif

    extern double _decimal_scale(double arg, int* decpt, int* sign);
    /* If arg is 0.9999 and we ask for three digits, _decimal_digits
       returns true and puts "1" in the buffer.  If we ask for four
       digits, it returns false and puts "9999" in the buffer.  The
       return value is a signal to move the decimal point. */

    extern int _decimal_digits(double arg, int ndigits, char* buf);
    extern int _decimal_digits2(double arg, int ndigits, char* buf,int);

#endif

/* A long with only the high-order bit turned on */
#define    HIBIT    0x80000000L
#if _LONG_LONG
/* A long long with only the high-order bit turned on */
#define    LLHIBIT    0x8000000000000000LL
#endif

#define    isdigit(x)    ((x) - '0'>=0 && (x) - '0'<=9)
#define    tonumber(x)   ((x)-'0')
#define    todigit(x)    ((x)+'0')


#define MAXDIGS    30
#define MAXCVT     47
#define MAXFCVT    30
#define MAXESIZ    5

typedef
    struct {
    va_list    args;    /* Used for communicating with subroutines */

    int    precision;
    int    width;
    char   fcode;
    int    lzero;
    int    rzero;

    char*  prefix;
    char*  buffer;
    char*  suffix;

    enum {ModFlag_None,
          ModFlag_h,
          ModFlag_l,
          ModFlag_ll,
          ModFlag_L}               modFlag;
    enum {kNone, kLeft, kRight}    odoFlag;
    int    odo;

    Bool   bZero;
    Bool   bLeftJust;
    Bool   bBlank;
    Bool   bSharp;
    Bool   bPlus;
    }
    FormatT;



/* UNSAFE!! (but handy).  Usage:  val = FETCH(format, long);
    Note that this generates multiple statements, and so can't be a
    pure expression.  Note also that both args get evaluated more than
    once. */

#define FETCH(FORMAT, TYP)     \
    *(TYP*) (FORMAT)->args;    \
    (FORMAT)->args += sizeof(TYP)

#if _LONG_LONG 
/* similar to that for FETCH_LONG_DOUBLE (see below) */
   #define FETCH_LONG_LONG(A,FORMAT) \
       (A) = va_arg((FORMAT)->args,long long)

   #define FETCH_UNSIGNED_LONG_LONG(A,FORMAT) \
       (A) = va_arg((FORMAT)->args,unsigned long long)
#endif

#if !defined(__DOUBLES_ARE_FLOATS__) && defined(__ADSPBLACKFIN__)

/* The FETCH macro, although fast, cannot be used to extract values of
   type long double, as they may be preceded by a hole to align the
   value on a double-word boundary. In these circumstances, use the
   macro FETCH_LONG_DOUBLE defined below.

   Usage: FETCH_LONG_DOUBLE(val,format);
*/

   #define FETCH_LONG_DOUBLE(A,FORMAT) \
       (A) = va_arg((FORMAT)->args,long double)

#endif

/*
   Some internal functions are sensitive to the size of type double. The
   #define's below are used to modify the names of their entry points to
   include the size of double that they support
*/

#if defined(__DOUBLES_ARE_FLOATS__)
#define _print_a_float  _print_a_float_32
#define _print_float  _print_float_32
#define _doprnt         _doprnt_32
#else
#define _print_a_float  _print_a_float_64
#define _print_float _print_f_float_64
#define _doprnt         _doprnt_64
#endif

void _pad(FuncT, FargT, char, int);
void _put_string(FuncT, FargT, const char*, int);
int _do_output(FormatT*, FuncT, FargT);
const char* _parse_field(const char* fp, FormatT* format, int* field);
const char* _parse_format(const char* fp, FormatT* format);
void _set_sign_prefix(FormatT* format, Bool isNegative);
char* _long_to_string(long val, int hradix, Bool upcase, char* buf, int buflen);
#ifdef _LONG_LONG
char* _long_long_to_string(long long val, int hradix, Bool upcase, char* buf, int buflen);
#endif
char _long_to_char(long val, int hradix, Bool upcase);
int _print_fixed(FormatT* format, int hradix, FuncT func, FargT farg);
int _print_a_float(FormatT* format, FuncT func, FargT farg);
int _print_float(FormatT* format, FuncT func, FargT farg);
int _doprnt(const char* formatString, va_list args, FuncT func, FargT farg);

#ifdef __FIXED_POINT_ALLOWED
#pragma linkage_name __print_fx
int print_fx(FormatT* format, FuncT func, FargT farg);
#endif

/* following defined in xprnt.c
*/
extern const char __hex_digits_lower[];
extern const char __hex_digits_upper[];

#pragma always_inline
static __inline void _set_sign_prefix(FormatT* format, Bool isNegative)
{
  if (isNegative)
    format->prefix = "-";
  else if (format->bPlus)
    format->prefix = "+";
  else if (format->bBlank)
    format->prefix = " ";
}

#endif

// end of file

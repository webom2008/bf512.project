/************************************************************************
 *
 * xscan.c : $Revision: 3544 $
 *
 * (c) Copyright 1998-2008 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr ("libName=libio")
#pragma file_attr ("libGroup=stdio.h")

#if defined(__ADSPBLACKFIN__)

#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr ("libFunc=_doscan_32")
#pragma file_attr ("libFunc=__doscan_32")
#else
#pragma file_attr ("libFunc=__doscan_64")
#pragma file_attr ("libFunc=___doscan_64")
#endif

#else
#if defined(__ADSP21000__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr ("libFunc=_doscan_32")
#pragma file_attr ("libFunc=__doscan_32")
#else
#pragma file_attr ("libFunc=_doscan_64")
#pragma file_attr ("libFunc=__doscan_64")
#endif
#endif
#endif

#pragma file_attr ("libFunc=fread")
#pragma file_attr ("libFunc=_fread")
#pragma file_attr ("libFunc=fgetc")
#pragma file_attr ("libFunc=_fgetc")
#pragma file_attr ("libFunc=fgets")
#pragma file_attr ("libFunc=_fgets")
#pragma file_attr ("libFunc=fscanf")
#pragma file_attr ("libFunc=_fscanf")
#pragma file_attr ("libFunc=scanf")
#pragma file_attr ("libFunc=_scanf")

#pragma file_attr ("prefersMem=external")
#pragma file_attr ("prefersMemNum=70")
#endif

#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "_stdio.h"

typedef struct {
    va_list             args;
    ScanFnT             getfn;
    ScanContextT        context;

    const char*         fmt;

    int                 nread;

    int                 assign;
    int                 width;
    char                mod;
    char                spec;
}       ScanConversionT;


#define FMAX            100     /* Widest supported number field */

/* Macro City */

// Fetches an argument of type TYP from the argument array held in
// CVT (of type ScanConversionT *)
#define FETCH_ARG(LHS,CVT, TYP)     \
    do {(LHS) = *(TYP*) (CVT)->args; (CVT)->args += sizeof(TYP);} while (0)

// Assigns the scanned value (VAL) through an argument pointer
#define ASSIGN_ARG(CVT, TYP, VAL)       \
    **(TYP*) (CVT)->args = (VAL);       \
    (CVT)->args += sizeof(TYP)

// These are the basic get character and unget character macros.
#define GET1(CVT)       ((CVT)->nread++,(CVT)->getfn(DO_GETC, (CVT)->context))
#define UNGET1(CH, CVT) ((CVT)->nread--,(CVT)->getfn(CH, (CVT)->context))

// Tests whether there are NREAD0 more characters available from this
// width-limited field.
#define TOO_MANY(CVT, NREAD0) ((CVT)->nread - (NREAD0) >= (CVT)->width)

// Width-limited versions of get character and unget character
#define GET1W(CVT, NREAD0) (TOO_MANY(CVT, NREAD0) ? READ_LIMIT : GET1(CVT))
#define UNGET1W(CH, CVT) ((CH) > READ_LIMIT ? UNGET1(CH, CVT) : 0)

#define S_CONSUME(NXT) do { *p++ = ch; state = NXT; } while (0)
#define S_EPSILON(NXT) do { UNGET1W(ch, cvt); state = NXT; } while (0)
#define S_ERROR        do { UNGET1W(ch, cvt); state = ERROR; } while (0)
#define S_ACCEPT       do { UNGET1(ch, cvt); state = ACCEPT; } while (0)

/*

   >[ S1 ] --- +- ---> [ S2 ] --- 0 ---> [[ S3 ]] --- xX ---> [ S4 ]-\
       \                  > \              \                         |
        ------ e --------/   \              \                        |
                              e               e          digit     digit
                               \               \          |  |       |
                                \               \----->   \  v       |
                                 \--------------------> [[ S5 ]] <-- /



*/


static const char digits[] = "0123456789ABCDEFabcdef";
               // a set of permitted digits
               //     - the first 2 are binary digits
               //     - the first 8 are octal digits
               //     - the first 10 are decimal digits
               //     - and all 22 are hexadecimal digits

// Returns 1 (success), 0 (failure), or EOF
static int
scanInt(ScanConversionT* cvt)
{
    char buf[FMAX+1];
    char* p = buf;

    int ndigits;
    int base = cvt->spec == 'd' || cvt->spec == 'u'     ? 10 :
               cvt->spec == 'i'                         ? 0  :
               cvt->spec == 'o'                         ? 8  :
                 /* 'x', 'X', 'p' */                      16;

    int nread0 = cvt->nread;
    int ch;
    enum {S1, S2, S3, S4, S5, ACCEPT, ERROR} state = S1;

    ndigits = base == 0 || base == 10 ? 10 :
              base == 8               ? 8  :
                                      22;  /* 10 digits + 12 hex */
    if (cvt->width <= 0)
        cvt->width = FMAX;

    do {
        ch = GET1W(cvt, nread0);

        switch (state)
        {
          case S1:
            if (ch == '+' || ch == '-') S_CONSUME(S2);
            else                        S_EPSILON(S2);
            break;

          case S2:
            if (ch == '0') S_CONSUME(S3);
            else           S_EPSILON(S5);
            break;

          case S3:
            if ((ch == 'x' || ch == 'X') && (base == 0 || base == 16))
                { base = 16; ndigits = 22; S_CONSUME(S4); }
            else { if (base == 0) base = ndigits = 8; S_EPSILON(S5); }
            break;

          case S4:
            if (memchr(digits, ch, ndigits)) S_CONSUME(S5);
            else                             S_ERROR;
            break;

          case S5:
            if (memchr(digits, ch, ndigits)) S_CONSUME(S5);
            else                             S_ACCEPT;
            break;
        }
    } while (state != ERROR && state != ACCEPT);

    *p = '\0';

    if (state != ACCEPT || p == buf)    // State machine accepts empty strings,
                                        // so, we reject them explicitly here.
        return ch < 0 ? EOF : 0;

    if (cvt->assign)
    {
        if (cvt->spec == 'd' || cvt->spec == 'i')
        {
#if _LONG_LONG && !defined(__LIBIO_LITE)
#if !defined(__NOT_GNU_3_2_COMPATIBLE__)
            if ((cvt->mod == '#') || (cvt->mod == 'j'))
#else
            if (cvt->mod == '#') 
#endif
            {
               long long l = strtoll(buf, (char **)NULL, base);
               ASSIGN_ARG(cvt, long long *, l);
            }
            else {
#endif
            long l = strtol(buf, (char **)NULL, base);

            if (cvt->mod == 'h') { ASSIGN_ARG(cvt, short *, l); }
            else if (cvt->mod == 'l') { ASSIGN_ARG(cvt, long *, l); }
#if !defined(__NOT_GNU_3_2_COMPATIBLE__)
            else if (cvt->mod == '/')
            {   ASSIGN_ARG(cvt, char *, l); }
            else if (cvt->mod == 'j')
#if _LONG_LONG && !defined(__LIBIO_LITE)
            {   ASSIGN_ARG(cvt, long long *, cvt->nread);  }
#else
            {   ASSIGN_ARG(cvt, long *, cvt->nread);  }            
#endif
#endif
            else { ASSIGN_ARG(cvt, int *, l); }
#if _LONG_LONG && !defined(__LIBIO_LITE)
            }
#endif
        }
        else
        {
#if _LONG_LONG && !defined(__LIBIO_LITE)
#if !defined(__NOT_GNU_3_2_COMPATIBLE__)
            if ((cvt->mod == '#') || (cvt->mod == 'j'))
#else
            if (cvt->mod == 'j')
#endif
            {
               unsigned long long ul = strtoull(buf, (char **)NULL, base);
               ASSIGN_ARG(cvt, unsigned long long *, ul);
            }
            else  {
#endif
            unsigned long ul = strtoul(buf, (char **)NULL, base);

            if (cvt->spec == 'p')
            {
                ASSIGN_ARG(cvt, void**, (void*) ul);
            } else if (cvt->mod == 'h') {
                ASSIGN_ARG(cvt, unsigned short *, ul);
            }
#if !defined(__NOT_GNU_3_2_COMPATIBLE__)
            else if (cvt->mod == '/')
            {   ASSIGN_ARG(cvt, unsigned char *, ul); }
#if _LONG_LONG && !defined(__LIBIO_LITE)
            else if (cvt->mod == 'j')
            {   ASSIGN_ARG(cvt, unsigned long long *, ul);   }
#endif
            else if (cvt->mod == 'l' || cvt->mod == 'j')
#else            
            else if (cvt->mod == 'l')
#endif
            {
                ASSIGN_ARG(cvt, unsigned long *, ul);
            } else {
                ASSIGN_ARG(cvt, unsigned int *, ul);
            }
#if _LONG_LONG && !defined(__LIBIO_LITE)
            }
#endif
        }
    }

    return 1;
}


static int
scanFloat(ScanConversionT* cvt)
{
    char buf[FMAX+1];
    char *p = buf;

    int nread0 = cvt->nread;
    int ch;

#if !defined(__NOT_GNU_3_2_COMPATIBLE__)

      /* State        Current Status            Required Action */
    enum
    {
          S1  ,    // Uninitialized             Handle an optional sign
          S2  ,    // No leading sign           Check for '0X'
          S3  ,    // Have a leading '0'        Check for 'X'
          S4  ,    // Have a leading digit      Continue parsing
          S5  ,    // Have a leading '.'        Need a digit
          S6  ,    // Have 0X                   Continue parsing hex float
          S7  ,    // Have a valid '.'          Continue parsing for an exponent
          S8  ,    // Have a 'E' or 'P'         Handle an optional sign
          S9  ,    // No leading exponent sign  Parse unsigned int
          S10 ,    // Have a valid exponent     Continue parsing for a digit
        ACCEPT,
        ERROR
    } state = S1;

    int ndigits  = 10;     /* initially size of the set of permitted digits */
    int exp_type = 'e';    /* initially the designator for an exponent      */

#else
    enum {S1, S2, S3, S4, S8, S9, S10, ACCEPT, ERROR} state = S1;
#endif

    if (cvt->width <= 0)
        cvt->width = FMAX;

    do {
        ch = GET1W(cvt, nread0);

        switch (state)
        {
          case S1:
            if (ch == '+' || ch == '-') S_CONSUME(S2);
            else                        S_EPSILON(S2);
            break;

#if !defined(__NOT_GNU_3_2_COMPATIBLE__)
          case S2:

            /* Parse an unsigned number */

            if      (ch == '0')   S_CONSUME(S3);
            else if (isdigit(ch)) S_CONSUME(S4);
            else if (ch == '.')   S_CONSUME(S5);
            else S_ERROR;
            break;

          case S3:

            /* Have a leading zero - check for 0x or 0X */

            if ((ch | ' ') == 'x')
            {
                ndigits  = sizeof(digits);
                exp_type = 'p';
                S_CONSUME(S6);
            } else {
                S_EPSILON(S4);
            }
            break;

          case S4:

            /* After a valid digit - Continue parsing */

            if      (ch == '.')                   S_CONSUME(S7);
            else if (memchr(digits, ch, ndigits)) S_CONSUME(S4);
            else                                  S_EPSILON(S7);
            break;

          case S5:

            /* After a leading '.' - a digit is required */

            if (memchr(digits, ch, ndigits)) S_CONSUME(S7);
            else S_ERROR;
            break;

          case S6:

            /* After 0x (or 0X) - a digit or '.' is required */

            if      (ch == '.')                   S_CONSUME(S5);
            else if (memchr(digits, ch, ndigits)) S_CONSUME(S4);
            else                                  S_ERROR;
            break;

          case S7:

            /* Have a valid number - accept more digits or an exponent */

            if      ((ch | ' ') == exp_type)      S_CONSUME(S8);
            else if (memchr(digits, ch, ndigits)) S_CONSUME(S7);
            else if (exp_type == 'e')             S_ACCEPT;
            else                                  S_ERROR;
            break;
#else

          case S2:

            /* Parse an unsigned number */

            if   (isdigit(ch))  S_CONSUME(S3);
            else if (ch == '.') S_CONSUME(S4);
            else                S_ERROR;
            break;

          case S3:

            /* After a valid digit - Continue parsing */

            if      (isdigit(ch))       S_CONSUME(S3);
            else if (ch == '.')         S_CONSUME(S4);
            else if ((ch | ' ') == 'e') S_CONSUME(S8);
            else                        S_ACCEPT;
            break;

          case S4:

            /* After a valid digit or '.' - Continue parsing */

            if      (isdigit(ch))       S_CONSUME(S4);
            else if ((ch | ' ') == 'e') S_CONSUME(S8);
            else                        S_ACCEPT;
            break;
#endif

          case S8:

            /* After an exponent designator - parse an exponent */

            if (ch == '+' || ch == '-') S_CONSUME(S9);
            else                        S_EPSILON(S9);
            break;

          case S9:

            /* Expect an unsigned exponent */

            if (isdigit(ch)) S_CONSUME(S10);
            else             S_ERROR;
            break;

          case S10:

            /* After a valid exponent - continue parsing */

            if (isdigit(ch)) S_CONSUME(S10);
            else             S_ACCEPT;
            break;
        }
    } while ((state != ERROR) && (state != ACCEPT));

    *p = '\0';

    if (state != ACCEPT)
        return ch < 0 ? EOF : 0;

    if (cvt->assign)
    {
        double d = strtod(buf, (char **)NULL);

        if      (cvt->mod == 'l') { ASSIGN_ARG(cvt, double *, d); }
        else if (cvt->mod == 'L') { ASSIGN_ARG(cvt, long double *, d); }
        else                      { ASSIGN_ARG(cvt, float *, d); }
    }

    return 1;
}

#if defined(__FIXED_POINT_ALLOWED)
#if defined(_ADI_FX_LIBIO)

#include <stdfix.h>
#include "xstrtofx.h"

typedef struct {
    ScanConversionT* cvt;
    char prev_char;
    int nread0;
    bool input_failure;
} libio_scanf_string_data_t;

/* this function returns the next character to be processed, advancing
** by one in the input string.
*/
static char 
read_char_libio_scanf(void *cl, char curr_c,
                      int *speculatively_read)
{
    
    libio_scanf_string_data_t *string_info = (libio_scanf_string_data_t *)cl;
    char c = GET1W(string_info->cvt, string_info->nread0);
    string_info->prev_char = curr_c;
    *speculatively_read += 1;
    return c;
}

/* this function backtracks by replacing the given character at the
** end and retreating by one character in the input. It returns the
** previous character.
*/
static char
replace_char_libio_scanf(void *cl, char c, int *speculatively_read)
{
    libio_scanf_string_data_t *string_info = (libio_scanf_string_data_t *)cl;
    char prev;
    if (string_info->prev_char != EOF) 
    {
        prev = string_info->prev_char;
        UNGET1W((int)c, string_info->cvt);
        string_info->prev_char = EOF;
    }
    else
    {
        prev = '0'; /* ok for any subsequent processing */
        string_info->input_failure = true;
    }
    *speculatively_read -= 1;
    return prev;
}

/* this function stores the current position (the end of the valid
** portion of the input) for safe-keeping.
*/
static void
set_end_position_libio_scanf(void *cl)
{
    /* no need to record end ptr */
}

/* this function records an overflow state.
*/
static void
set_overflow_libio_scanf(void *cl)
{
    /* no need to record overflow */
}


int
scanFixed(ScanConversionT* cvt)
{
    libio_scanf_string_data_t string_info;
    int return_value = 0;
    char ch;
    int dummy;

    string_info.cvt = cvt;
    string_info.nread0 = cvt->nread;
    string_info.input_failure = false;

    if (cvt->width <= 0)
        cvt->width = INT_MAX;

    ch = read_char_libio_scanf(&string_info, '\0', &dummy);
    replace_char_libio_scanf(&string_info, ch, &dummy);
    if (ch == (char)EOF)
    {
        return_value = EOF;
    }
    else
    {
        largest_unsigned_fixed_point_container_t val;
        switch (cvt->spec) {
        case 'r':
#if SFRACT_FBIT != FRACT_FBIT
            if (cvt->mod == 'h')
            {
                val = xstrtofx(&string_info,
                               read_char_libio_scanf,
                               replace_char_libio_scanf,
                               set_end_position_libio_scanf,
                               set_overflow_libio_scanf,
                               SFRACT_FBIT,
                               true,
                               SFRACT_MAX);
                if (cvt->assign && !string_info.input_failure)
                {
                    ASSIGN_ARG(cvt, short fract *, hrbits((int_hr_t)val));
                }
            }
            else 
#endif
#if LFRACT_FBIT != FRACT_FBIT
            if (cvt->mod == 'l')
            {
                val = xstrtofx(&string_info,
                               read_char_libio_scanf,
                               replace_char_libio_scanf,
                               set_end_position_libio_scanf,
                               set_overflow_libio_scanf,
                               LFRACT_FBIT,
                               true,
                               LFRACT_MAX);
                if (cvt->assign && !string_info.input_failure)
                {
                    ASSIGN_ARG(cvt, long fract *, lrbits((int_lr_t)val));
                }
            }
            else
#endif
            {
                val = xstrtofx(&string_info,
                               read_char_libio_scanf,
                               replace_char_libio_scanf,
                               set_end_position_libio_scanf,
                               set_overflow_libio_scanf,
                               FRACT_FBIT,
                               true,
                               FRACT_MAX);
                if (cvt->assign && !string_info.input_failure)
                {
                    ASSIGN_ARG(cvt, fract *, rbits((int_r_t)val));
                }
            }
            break;
        case 'R':
#if USFRACT_FBIT != UFRACT_FBIT
            if (cvt->mod == 'h')
            {
                val = xstrtofx(&string_info,
                               read_char_libio_scanf,
                               replace_char_libio_scanf,
                               set_end_position_libio_scanf,
                               set_overflow_libio_scanf,
                               USFRACT_FBIT,
                               false,
                               USFRACT_MAX);
                if (cvt->assign && !string_info.input_failure)
                {
                    ASSIGN_ARG(cvt, unsigned short fract *, 
                               uhrbits((uint_uhr_t)val));
                }
            }
            else
#endif
#if ULFRACT_FBIT != UFRACT_FBIT
            if (cvt->mod == 'l')
            {
                val = xstrtofx(&string_info,
                               read_char_libio_scanf,
                               replace_char_libio_scanf,
                               set_end_position_libio_scanf,
                               set_overflow_libio_scanf,
                               ULFRACT_FBIT,
                               false,
                               ULFRACT_MAX);
                if (cvt->assign && !string_info.input_failure)
                {
                    ASSIGN_ARG(cvt, unsigned long fract *,
                               ulrbits((uint_ulr_t)val));
                }
            }
            else
#endif
            {
                val = xstrtofx(&string_info,
                               read_char_libio_scanf,
                               replace_char_libio_scanf,
                               set_end_position_libio_scanf,
                               set_overflow_libio_scanf,
                               UFRACT_FBIT,
                               false,
                               UFRACT_MAX);
                if (cvt->assign && !string_info.input_failure)
                {
                    ASSIGN_ARG(cvt, unsigned fract *,
                               urbits((uint_ur_t)val));
                }
            }
            break;
#if !defined(__FX_NO_ACCUM)
        case 'k':
#if SACCUM_FBIT != ACCUM_FBIT || SACCUM_IBIT != ACCUM_IBIT
            if (cvt->mod == 'h')
            {
                val = xstrtofx(&string_info,
                               read_char_libio_scanf,
                               replace_char_libio_scanf,
                               set_end_position_libio_scanf,
                               set_overflow_libio_scanf,
                               SACCUM_FBIT,
                               true,
                               SACCUM_MAX);
                if (cvt->assign && !string_info.input_failure)
                {
                    ASSIGN_ARG(cvt, short accum *, hkbits((int_hk_t)val));
                }
            }
            else
#endif
#if LACCUM_FBIT != ACCUM_FBIT || LACCUM_IBIT != ACCUM_IBIT
            if (cvt->mod == 'l')
            {
                val = xstrtofx(&string_info,
                               read_char_libio_scanf,
                               replace_char_libio_scanf,
                               set_end_position_libio_scanf,
                               set_overflow_libio_scanf,
                               LACCUM_FBIT,
                               true,
                               LACCUM_MAX);
                if (cvt->assign && !string_info.input_failure)
                {
                    ASSIGN_ARG(cvt, long accum *, lkbits((int_lk_t)val));
                }
            }
            else
#endif
            {
                val = xstrtofx(&string_info,
                               read_char_libio_scanf,
                               replace_char_libio_scanf,
                               set_end_position_libio_scanf,
                               set_overflow_libio_scanf,
                               ACCUM_FBIT,
                               true,
                               ACCUM_MAX);
                if (cvt->assign && !string_info.input_failure)
                {
                    ASSIGN_ARG(cvt, accum *, kbits((int_k_t)val));
                }
            }
            break;
        case 'K':
#if USACCUM_FBIT != UACCUM_FBIT || USACCUM_IBIT != UACCUM_IBIT
            if (cvt->mod == 'h')
            {
                val = xstrtofx(&string_info,
                               read_char_libio_scanf,
                               replace_char_libio_scanf,
                               set_end_position_libio_scanf,
                               set_overflow_libio_scanf,
                               USACCUM_FBIT,
                               false,
                               USACCUM_MAX);
                if (cvt->assign && !string_info.input_failure)
                {
                    ASSIGN_ARG(cvt, unsigned short accum *,
                               uhkbits((uint_uhk_t)val));
                }
            }
            else
#endif
#if ULACCUM_FBIT != UACCUM_FBIT || ULACCUM_IBIT != UACCUM_IBIT
            if (cvt->mod == 'l')
            {
                val = xstrtofx(&string_info,
                               read_char_libio_scanf,
                               replace_char_libio_scanf,
                               set_end_position_libio_scanf,
                               set_overflow_libio_scanf,
                               ULACCUM_FBIT,
                               false,
                               ULACCUM_MAX);
                if (cvt->assign && !string_info.input_failure)
                {
                    ASSIGN_ARG(cvt, unsigned long accum *, ulkbits(val));
                }
            }
            else
#endif
            {
                val = xstrtofx(&string_info,
                               read_char_libio_scanf,
                               replace_char_libio_scanf,
                               set_end_position_libio_scanf,
                               set_overflow_libio_scanf,
                               UACCUM_FBIT,
                               false,
                               UACCUM_MAX);
                if (cvt->assign && !string_info.input_failure)
                {
                    ASSIGN_ARG(cvt, unsigned accum *, ukbits((uint_uk_t)val));
                }
            }
            break;
#endif /* __FX_NO_ACCUM */
        }
        if (!string_info.input_failure)
        {
            return_value = 1;
        }
    }
    return return_value;
}

#endif
#endif

static int     /* Returns true if the field scanned correctly */
_scan1(ScanConversionT* cvt)
{
    int     ch;

    if (!strchr((const char *)"cn[%", (int)(cvt->spec)))
    {
        while (isspace(ch = GET1(cvt)))
            ;
        UNGET1(ch, cvt);
    }

    switch (cvt->spec)
    {
      case 'c':
        {
            char* p;

            if (cvt->assign)
                FETCH_ARG(p,cvt, char*);

            if (cvt->width == 0)
                cvt->width = 1;
            for (; cvt->width > 0; --cvt->width)
            {
                if ((ch = GET1(cvt)) < 0)
                    return EOF;
                else if (cvt->assign)
                    *p++ = ch;
            }
        }
        return 1;

      case '%':

        /* Any "%%" sequence in the format must match a '%' in the input */

        if ((ch = GET1(cvt)) == '%')
            return 1;
        UNGET1(ch, cvt);
        return 0;

      case 'p': case 'd': case 'i':
      case 'o': case 'u': case 'x': case 'X':
        return scanInt(cvt);

#if !defined(__NOT_GNU_3_2_COMPATIBLE__)
      case 'a': case 'A': case 'e': case 'E': case 'f': case 'g': case 'G':
#else
      case 'e': case 'E': case 'f': case 'g': case 'G':
#endif
        return scanFloat(cvt);

#if defined(__FIXED_POINT_ALLOWED)
#if defined(_ADI_FX_LIBIO)
      case 'r': case 'R':
#if !defined(__FX_NO_ACCUM)
      case 'k': case 'K':
#endif
        return scanFixed(cvt);
#endif
#endif

      case 'n':
        if (cvt->assign)
        {
            if (cvt->mod == 'h')
            {   ASSIGN_ARG(cvt, short *, cvt->nread); }
            else if (cvt->mod == 'l')
            {   ASSIGN_ARG(cvt, long *, cvt->nread);  }
#if !defined(__NOT_GNU_3_2_COMPATIBLE__)
            else if (cvt->mod == '/')
            {   ASSIGN_ARG(cvt, char *, cvt->nread);  }
#if _LONG_LONG && !defined(__LIBIO_LITE)
            else if ((cvt->mod == '#') || (cvt->mod == 'j'))
            {   ASSIGN_ARG(cvt, long long *, cvt->nread);  }
#endif
#endif
            else
            {   ASSIGN_ARG(cvt, int *, cvt->nread);   }
        }
        return 1;

      case 's':
        {
            int nread0 = cvt->nread;
            char* p;
            if (cvt->assign)
                FETCH_ARG(p,cvt, char *);

            if (cvt->width <= 0)
                cvt->width = INT_MAX;

            while (!TOO_MANY(cvt, nread0))
            {
                if ((ch = GET1(cvt)) < 0)
                    break;
                else if (isspace(ch))
                {
                    UNGET1(ch, cvt);
                    break;
                }
                else if (cvt->assign)
                    *p++ = ch;
            }

            if (cvt->assign)
                *p = '\0';

            if (ch < 0 && ((cvt->nread-nread0)==1))
                return 0;
        }
        return 1;

      case '[':
        {
            int nread0 = cvt->nread;
            int compl = *cvt->fmt == '^';
            const char *stop;             /* ptr to matching ']'             */
            const char *start;            /* temporary pointer into scanlist */
            size_t setn;                  /* length of scanlist              */
            char* p;

            if (cvt->assign)
                FETCH_ARG(p,cvt, char *);

            if (compl)
                cvt->fmt++;

            start = cvt->fmt;
            if (*start == ']')
                start++;
            stop = strchr(start,']');
            if (stop == NULL)
                return 0;
            setn = stop - cvt->fmt;

            if (cvt->width <= 0)
                cvt->width = INT_MAX;

            while (!TOO_MANY(cvt, nread0))
            {
                if ((ch = GET1(cvt)) < 0)
                    break;
                else if (compl ? memchr(cvt->fmt, ch, setn) != NULL
                               : memchr(cvt->fmt, ch, setn) == NULL)
                {
                    UNGET1(ch, cvt);
                    break;
                }
                else if (cvt->assign)
                    *p++ = ch;
            }

            if (cvt->assign)
                *p = '\0';

            cvt->fmt = stop + 1;
        }
        return 1;

      default:
        return 0;
    }
}

int
_doscan(const char* fmtString, va_list args, ScanFnT getfn, ScanContextT context)
{
    const char* cp = fmtString;
    int         ch;
    int nconversions = 0;
    ScanConversionT cvt;


    cvt.args = args;
    cvt.getfn = getfn;
    cvt.context = context;
    cvt.nread = 0;

    while (*cp != '\0')
    {
        if (*cp == '%')
        {
            cp++;
            cvt.assign = *cp != '*' ? 1 : (cp++, 0);
            cvt.width = 0;
            for (; isdigit(*cp); cp++)
                cvt.width = cvt.width * 10 + (*cp - '0');
#if !defined(__NOT_GNU_3_2_COMPATIBLE__)
            cvt.mod = strchr("hlLjzt", *cp) ? *cp++ : 0;
            if (cvt.mod == 'h' && *cp == 'h') /* i.e. 'hh' for char input */
              cvt.mod = '/', *cp++;
#else
            cvt.mod = strchr("hlL", *cp) ? *cp++ : 0;
#endif
#if _LONG_LONG && !defined(__LIBIO_LITE)
            if (cvt.mod == 'l' && *cp == 'l') /* i.e. 'll' for long long input */
               cvt.mod = '#', *cp++;
#endif
            cvt.spec = *cp++;

            cvt.fmt = cp;

            if ((ch = _scan1(&cvt)) > 0)
            {
                cp = cvt.fmt;

                if (cvt.assign && (cvt.spec != 'n') && (cvt.spec != '%'))
                         /* The return value is the number of input items
                         ** assigned excluding %n (and %%)
                         */
                    nconversions++;
            }
            else if (ch < 0 && nconversions == 0)
                return EOF;
            else
            {
                return nconversions;
            }
        }
        else if (isspace(*cp))
        {
            while (isspace(*cp))
                cp++;
            while (isspace(ch = GET1(&cvt)))
                ;
            UNGET1(ch, &cvt);
        }
        else if ((ch = GET1(&cvt)) != *cp++)
        {
            UNGET1(ch, &cvt);
            return nconversions;
        }
        /* else a literal match succeeded */
    }

    return nconversions;

}

// end of file

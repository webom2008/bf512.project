/************************************************************************
 *
 * xgetfx.c : $Revision: 1.8.4.1 $
 *
 * (c) Copyright 2008-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/* _Getfx function */

#if !defined(__NO_FILE_ATTRIBUTES__)

#pragma file_attr( "libGroup=stdio.h")
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=_Getfx")
#pragma file_attr(  "libFunc=__Getfx")
#pragma file_attr(  "libFunc=fscanf")
#pragma file_attr(  "libFunc=_fscanf")
#pragma file_attr(  "libFunc=scanf")
#pragma file_attr(  "libFunc=_scanf")
#pragma file_attr(  "libFunc=sscanf")
#pragma file_attr(  "libFunc=_sscanf")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#endif

#include <stdlib.h>
#include <string.h>
#include <stdfix.h>

#ifdef _MISRA_RULES
#pragma diag(suppress:misra_rule_6_3)
/* Suppress Rule 6.3 (advisory) whereby typedefs that indicate
** size and signedness should be used in place of the basic types.
**
** Rationale: The C/C++ manual documents the prototypes of the library
** functions using the intrinsic C data types and these prototypes must
** correspond to the actual prototypes that are defined in the header
** files. In addition, Rule 8.3 requires that the type of the result
** and the type of the parameters that are specified in a function
** definition and a function declaration are identical.
*/

#pragma diag(suppress:misra_rule_5_6)
#pragma diag(suppress:misra_rule_5_7)
/* Suppress rules 5.6 and 5.7 (advisory) whereby no identifier name shall
** be reused or used in different namespaces.
**
** Rationale: the Dinkumware headers do not obey this rule and we do not
** wish to change them just now.
*/

/* FIXME: temporarily suppress this rule until bug is fixed.
*/
#pragma diag(suppress:misra_rule_10_1_a)

#pragma diag(suppress:misra_rule_11_4)
#pragma diag(suppress:misra_rule_17_4)
#pragma diag(suppress:misra_rule_10_5)
#pragma diag(suppress:misra_rule_12_11)
/* Suppress rules which prevent us from doing e.g.
**   *va_arg(px->ap, short fract *) = x
**
** Rationale: we need to store the value into the given location, which
** has generic type, and this is the way it is done throughout Dinkumware.
*/
#endif

#include "xstdio.h"
#include "xdinkum_scanf_string_info.h"
#include "xstrtofx.h"

_STD_BEGIN

int 
_Getfx(_Sft *px)
{	/* get a fixed-point value for _Scanf */
    int return_value = 0;
    char ch;
    int dummy;
    dinkum_scanf_string_info_t string_info;
    string_info.px = px;
    string_info.input_failure = false;
    px->nget = (0 < px->width) ? px->width : INT_MAX;
    ch = read_char_dinkum_scanf(&string_info, '\0', &dummy);
    replace_char_dinkum_scanf(&string_info, ch, &dummy);
    if (ch == (char)EOF)
    {
        return_value = EOF; 
    }
    else
    {
        largest_unsigned_fixed_point_container_t val;
        if (*px->s == 'r')
        {
#if SFRACT_FBIT != FRACT_FBIT
            if (px->qual == 'h')
            {
                val = xstrtofx(&string_info,
                               read_char_dinkum_scanf,
                               replace_char_dinkum_scanf,
                               set_end_position_dinkum_scanf,
                               set_overflow_dinkum_scanf,
                               SFRACT_FBIT,
                               true,
                               SFRACT_MAX);
                if ((!string_info.input_failure) && (px->noconv == '\0'))
                {
                    px->stored = (char)1;
                    *va_arg(px->ap, short fract *) = hrbits((int_hr_t)val);
                }
            }
            else
#endif
#if LFRACT_FBIT != FRACT_FBIT
            if (px->qual == 'l')
            {
                val = xstrtofx(&string_info,
                               read_char_dinkum_scanf,
                               replace_char_dinkum_scanf,
                               set_end_position_dinkum_scanf,
                               set_overflow_dinkum_scanf,
                               LFRACT_FBIT,
                               true,
                               LFRACT_MAX);
                if ((!string_info.input_failure) && (px->noconv == '\0'))
                {
                    px->stored = (char)1;
                    *va_arg(px->ap, long fract *) = lrbits((int_lr_t)val);
                }
            }
            else
#endif
            {
                val = xstrtofx(&string_info,
                               read_char_dinkum_scanf,
                               replace_char_dinkum_scanf,
                               set_end_position_dinkum_scanf,
                               set_overflow_dinkum_scanf,
                               FRACT_FBIT,
                               true,
                               FRACT_MAX);
                if ((!string_info.input_failure) && (px->noconv == '\0'))
                {
                    px->stored = (char)1;
                    *va_arg(px->ap, fract *) = rbits((int_r_t)val);
                }
            }
        }
        else if (*px->s == 'R')
        {
#if USFRACT_FBIT != UFRACT_FBIT
            if (px->qual == 'h')
            {
                val = xstrtofx(&string_info,
                               read_char_dinkum_scanf,
                               replace_char_dinkum_scanf,
                               set_end_position_dinkum_scanf,
                               set_overflow_dinkum_scanf,
                               USFRACT_FBIT,
                               false,
                               USFRACT_MAX);
                if ((!string_info.input_failure) && (px->noconv == '\0'))
                {
                    px->stored = (char)1;
                    *va_arg(px->ap, unsigned short fract *)
                         = uhrbits((uint_uhr_t)val);
                }
            }
            else
#endif
#if ULFRACT_FBIT != UFRACT_FBIT
            if (px->qual == 'l')
            {
                val = xstrtofx(&string_info,
                               read_char_dinkum_scanf,
                               replace_char_dinkum_scanf,
                               set_end_position_dinkum_scanf,
                               set_overflow_dinkum_scanf,
                               ULFRACT_FBIT,
                               false,
                               ULFRACT_MAX);
                if ((!string_info.input_failure) && (px->noconv == '\0'))
                {
                    px->stored = (char)1;
                    *va_arg(px->ap, unsigned long fract *)
                         = ulrbits((uint_ulr_t)val);
                }
            }
            else
#endif
            {
                val = xstrtofx(&string_info,
                               read_char_dinkum_scanf,
                               replace_char_dinkum_scanf,
                               set_end_position_dinkum_scanf,
                               set_overflow_dinkum_scanf,
                               UFRACT_FBIT,
                               false,
                               UFRACT_MAX);
                if ((!string_info.input_failure) && (px->noconv == '\0'))
                {
                    px->stored = (char)1;
                    *va_arg(px->ap, unsigned fract *)
                         = urbits((uint_ur_t)val);
                }
            }
        }
#if !defined(__FX_NO_ACCUM)
        else if (*px->s == 'k')
        {
#if SACCUM_FBIT != ACCUM_FBIT || SACCUM_IBIT != ACCUM_IBIT
            if (px->qual == 'h')
            {
                val = xstrtofx(&string_info,
                               read_char_dinkum_scanf,
                               replace_char_dinkum_scanf,
                               set_end_position_dinkum_scanf,
                               set_overflow_dinkum_scanf,
                               SACCUM_FBIT,
                               true,
                               SACCUM_MAX);
                if ((!string_info.input_failure) && (px->noconv == '\0'))
                {
                    px->stored = (char)1;
                    *va_arg(px->ap, short accum *) = hkbits((int_hk_t)val);
                }
            }
            else
#endif
#if LACCUM_FBIT != ACCUM_FBIT || LACCUM_IBIT != ACCUM_IBIT
            if (px->qual == 'l')
            {
                val = xstrtofx(&string_info,
                               read_char_dinkum_scanf,
                               replace_char_dinkum_scanf,
                               set_end_position_dinkum_scanf,
                               set_overflow_dinkum_scanf,
                               LACCUM_FBIT,
                               true,
                               LACCUM_MAX);
                if ((!string_info.input_failure) && (px->noconv == '\0'))
                {
                    px->stored = (char)1;
                    *va_arg(px->ap, long accum *) = lkbits((int_lk_t)val);
                }
            }
            else
#endif
            {
                val = xstrtofx(&string_info,
                               read_char_dinkum_scanf,
                               replace_char_dinkum_scanf,
                               set_end_position_dinkum_scanf,
                               set_overflow_dinkum_scanf,
                               ACCUM_FBIT,
                               true,
                               ACCUM_MAX);
                if ((!string_info.input_failure) && (px->noconv == '\0'))
                {
                    px->stored = (char)1;
                    *va_arg(px->ap, accum *) = kbits((int_k_t)val);
                }
            }
        }
        else if (*px->s == 'K')
        {
#if USACCUM_FBIT != UACCUM_FBIT || UACCUM_IBIT != ACCUM_IBIT
            if (px->qual == 'h')
            {
                val = xstrtofx(&string_info,
                               read_char_dinkum_scanf,
                               replace_char_dinkum_scanf,
                               set_end_position_dinkum_scanf,
                               set_overflow_dinkum_scanf,
                               USACCUM_FBIT,
                               false,
                               USACCUM_MAX);
                if ((!string_info.input_failure) && (px->noconv == '\0'))
                {
                    px->stored = (char)1;
                    *va_arg(px->ap, unsigned short accum *)
                         = uhkbits((uint_uhk_t)val);
                }
            }
            else
#endif
#if ULACCUM_FBIT != UACCUM_FBIT || ULACCUM_IBIT != UACCUM_IBIT
            if (px->qual == 'l')
            {
                val = xstrtofx(&string_info,
                               read_char_dinkum_scanf,
                               replace_char_dinkum_scanf,
                               set_end_position_dinkum_scanf,
                               set_overflow_dinkum_scanf,
                               ULACCUM_FBIT,
                               false,
                               ULACCUM_MAX);
                if ((!string_info.input_failure) && (px->noconv == '\0'))
                {
                    px->stored = (char)1;
                    *va_arg(px->ap, unsigned long accum *)
                      = ulkbits((uint_ulk_t)val);
                }
            }
            else
#endif
            {
                val = xstrtofx(&string_info,
                               read_char_dinkum_scanf,
                               replace_char_dinkum_scanf,
                               set_end_position_dinkum_scanf,
                               set_overflow_dinkum_scanf,
                               UACCUM_FBIT,
                               false,
                               UACCUM_MAX);
                if ((!string_info.input_failure) && (px->noconv == '\0'))
                {
                    px->stored = (char)1;
                    *va_arg(px->ap, unsigned accum *)
                         = ukbits((uint_uk_t)val);
                }
            }
        }
#endif /* __FX_NO_ACCUM */
        else
        {
            /* should assert */
        }
        if (!string_info.input_failure)
        {
            return_value = 1;
        }
    }
    return return_value;
}
_STD_END

/************************************************************************
 *
 * xlua_to_string.c: $Revision: 1.8.4.1 $
 *
 * (c) Copyright 2008-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)

#pragma file_attr ("libName=libc")
#pragma file_attr ("libGroup=stdio.h")

#pragma file_attr ("libFunc=xlua_to_string")
#pragma file_attr ("libFunc=___xlua_to_string")
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
#endif

#include "xlua_to_string.h"

char*
_long_to_string(long val, int hradix, bool upcase, char* buf, int buflen);


char *
xlua_to_string(largest_unsigned_fixed_point_t a, bool saturated, char buf[],
               int precision, bool hash)
{
    int i;
    fx_decimal_part_t p[NUM_PARTS];
    char *buf_out;
#if MAX_DECIMAL_INTEGER_DIGITS == 0
    /* we always want at least one int digit to hold the '0'
    */
    int fract_part_offset = 2;
#else
    int fract_part_offset = MAX_DECIMAL_INTEGER_DIGITS + 1;
#endif
    int carry_bit = -1; /* no carry */

    /* if value passed in is saturated, that means we pass in 0xff ffff fffe
    ** but we want 0x100 0000 0000. So skip all the computation and carry
    ** one to the units.
    */
    if (!saturated)
    {

      largest_unsigned_fixed_point_container_t fa
        = BITS_LARGEST_UNSIGNED_FX(a); 

      for (i = 0; i < NUM_PARTS; i++)
      {
          p[i] = 0U; 
      }

      for (i = 0; i < NUM_FRACT_BITS; i++)
      {
          if ((fa & 1U) != 0U)
          {
              int j;
              for (j = 0; j < NUM_PARTS; j++)
              {
                  p[j] += __fx_decimal_part[j][i+1]; 
              }
          }
          fa >>= 1U;
      }

      for (i = 0; i < (NUM_PARTS - 1); i++)
      {
          if (p[i] >= MAX_PER_PART)
          {
              p[i+1] += (p[i] / (fx_decimal_part_t)MAX_PER_PART);
              p[i] = (p[i] % (fx_decimal_part_t)MAX_PER_PART);
          }
      }
  
    }
    else
    {

      carry_bit = -2;

    }

    /* first the integer part
    */
#if MAX_DECIMAL_INTEGER_DIGITS == 0
    /* we know the integer_part is zero, so nothing to print.
    */
    buf_out = buf;
    buf[0] = '0';
#else
    {
        unsigned int integer_part = (unsigned int)a;
        buf_out = _long_to_string((long)integer_part, /*hradix*/5, false, buf,
                                  MAX_DECIMAL_INTEGER_DIGITS+1);
    }
#endif
    buf[fract_part_offset - 1] = '.';

    /* now the fractional part
    */
    /* first zero all the positions, because _long_to_string doesn't
    ** put in the leading zeros. It also writes in a null-terminator that
    ** we don't want.
    */
    memset(&buf[fract_part_offset], '0', (unsigned long)NUM_FRACT_BITS);

    if (!saturated)
    {
        for (i = 0; i < NUM_PARTS; i++)
        {
            _long_to_string((long)p[NUM_PARTS-1-i], /*hradix*/5, false,
                            &buf[fract_part_offset + (i * DIGITS_PER_PART)],
                            DIGITS_PER_PART+1);
            buf[fract_part_offset + ((i+1) * DIGITS_PER_PART)] = '0';
        }
    }

    if ((int)buf[fract_part_offset + precision] >= (int)'5')
    {
        carry_bit = (precision == 0) ? -2 : (precision - 1);
    }

    if (carry_bit != -1 /* -1 means no carry */)
    {
        /* round up. Do it on the string itself.
        */
        int digit = fract_part_offset + carry_bit;
        while (digit >= 0)
        {
            if (buf[digit] == '.')
            {
                /* carry to units */
            }
            else if (buf[digit] == '9')
            {
                buf[digit] = '0'; 
            }
            else
            {
                buf[digit] += 1; 
                break;
            }
            digit--;
        }
    }

    if ((!hash) && (precision == 0))
    {
        buf[fract_part_offset - 1] = '\0';
    }
    else
    {
        buf[fract_part_offset + precision] = '\0';
    }

    return buf_out;
}

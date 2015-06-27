/************************************************************************
 *
 * xfxdecdata.c : $Revision: 1.5.24.1 $
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_FILE_ATTRIBUTES__)
#pragma file_attr ("libName=libio")
#pragma file_attr ("libGroup=stdio.h")

#pragma file_attr ("libFunc=_xstrtofx")
#pragma file_attr ("libFunc=__xstrtofx")
#pragma file_attr ("libFunc=strtofxhr")
#pragma file_attr ("libFunc=_strtofxhr")
#pragma file_attr ("libFunc=strtofxr")
#pragma file_attr ("libFunc=_strtofxr")
#pragma file_attr ("libFunc=strtofxlr")
#pragma file_attr ("libFunc=_strtofxlr")
#pragma file_attr ("libFunc=strtofxuhr")
#pragma file_attr ("libFunc=_strtofxuhr")
#pragma file_attr ("libFunc=strtofxur")
#pragma file_attr ("libFunc=_strtofxur")
#pragma file_attr ("libFunc=strtofxulr")
#pragma file_attr ("libFunc=_strtofxulr")
#pragma file_attr ("libFunc=strtofxhk")
#pragma file_attr ("libFunc=_strtofxhk")
#pragma file_attr ("libFunc=strtofxk")
#pragma file_attr ("libFunc=_strtofxk")
#pragma file_attr ("libFunc=strtofxlk")
#pragma file_attr ("libFunc=_strtofxlk")
#pragma file_attr ("libFunc=strtofxuhk")
#pragma file_attr ("libFunc=_strtofxuhk")
#pragma file_attr ("libFunc=strtofxuk")
#pragma file_attr ("libFunc=_strtofxuk")
#pragma file_attr ("libFunc=strtofxulk")
#pragma file_attr ("libFunc=_strtofxulk")
#pragma file_attr ("libFunc=_lua_to_string")
#pragma file_attr ("libFunc=__lua_to_string")
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

#include "xfxdecdata.h"

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_9_2)
#endif /* _MISRA_RULES */

#if NUM_FRACT_BITS != 32
#error xfxdecdata only currently defined for 32 fract bits
#endif

/*
** Here are the decimal values of a single bit 1<<n, interpreted as
** a fractional value.
** 
** 31 0.5
** 30 0.25
** 29 0.125
** 28 0.0625
** 27 0.03125
** 26 0.015625
** 25 0.0078125
** 24 0.00390625
** 23 0.00195312 5
** 22 0.00097656 25
** 21 0.00048828 125
** 20 0.00024414 0625
** 19 0.00012207 03125
** 18 0.00006103 515625
** 17 0.00003051 7578125
** 16 0.00001525 87890625
** 15 0.00000762 93945312 5
** 14 0.00000381 46972656 25
** 13 0.00000190 73486328 125
** 12 0.00000095 36743164 0625
** 11 0.00000047 68371582 03125
** 10 0.00000023 84185791 015625
** 9  0.00000011 92092895 5078125
** 8  0.00000005 96046447 75390625
** 7  0.00000002 98023223 87695312 5
** 6  0.00000001 49011611 93847656 25
** 5  0.00000000 74505805 96923828 125
** 4  0.00000000 37252902 98461914 0625
** 3  0.00000000 18626451 49230957 03125
** 2  0.00000000 09313225 74615478 515625
** 1  0.00000000 04656612 87307739 2578125
** 0  0.00000000 02328306 43653869 62890625
** -1 0.00000000 01164028 21826934 81445312
** 
*/

/* The first element in the arrays represents 2^(-33) That's useful to
** have as we use it to tell which way we should round the final bit.
*/

fx_decimal_part_t __fx_decimal_part[NUM_PARTS][NUM_FRACT_BITS+1] = {
  {
    81445312U,
    62890625U,
    25781250U,
    51562500U,
     3125000U,
     6250000U,
    12500000U,
    25000000U,
    50000000U
    /* the rest are zero */
  },
  {
    21826934U,
    43653869U,
    87307739U,
    74615478U,
    49230957U,
    98461914U,
    96923828U,
    93847656U,
    87695312U,
    75390625U,
    50781250U,
     1562500U,
     3125000U,
     6250000U,
    12500000U,
    25000000U,
    50000000U
    /* the rest are zero */ 
  },
  {
     1164028U,
     2328306U,
     4656612U,
     9313225U,
    18626451U,
    37252902U,
    74505805U,
    49011611U,
    98023223U,
    96046447U,
    92092895U,
    84185791U,
    68371582U,
    36743164U,
    73486328U,
    46972656U,
    93945312U,
    87890625U,
    75781250U,
    51562500U,
     3125000U,
     6250000U,
    12500000U,
    25000000U,
    50000000U
    /* rest are zero */
  },
  {
           0U,
           0U,
           0U,
           0U,
           0U,
           0U,
           0U,
           1U,
           2U,
           5U,
          11U,
          23U,
          47U,
          95U,
         190U,
         381U,
         762U,
        1525U,
        3051U,
        6103U,
       12207U,
       24414U,
       48828U, 
       97656U, 
      195312U,
      390625U,
      781250U,
     1562500U, 
     3125000U,
     6250000U,
    12500000U, 
    25000000U,
    50000000U
  }
};

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

/* End of file */

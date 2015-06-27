/************************************************************************
 *
 * xstrtofx.h 
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 * $Revision: 1.6.6.1 $
 ************************************************************************/

#ifndef XSTRTOFX_H
#define XSTRTOFX_H

#include "xfxdecdata.h"
#include <stdbool.h>

largest_unsigned_fixed_point_container_t
xstrtofx(void *cl,
         char (*read_char)(void *, char, int *),
         char (*replace_char)(void *, char, int *),
         void (*set_end_position)(void *),
         void (*set_overflow)(void *),
         int targ_fract_bits,
         bool is_signed,
         largest_unsigned_fixed_point_t upper_saturation_limit);

#endif

/* End of file */

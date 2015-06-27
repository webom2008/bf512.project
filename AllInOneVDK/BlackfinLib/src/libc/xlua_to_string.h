/************************************************************************
 *
 * xlua_to_string.h 
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 * $Revision: 1.6.6.1 $
 ************************************************************************/

#ifndef XLUA_TO_STRING_H
#define XLUA_TO_STRING_H

#include <stdbool.h>
#include "xfxdecdata.h"

#pragma linkage_name ___xlua_to_string
char *xlua_to_string(largest_unsigned_fixed_point_t a, bool saturated,
                     char buf[], int precision, bool hash);

#endif

/* End of file */

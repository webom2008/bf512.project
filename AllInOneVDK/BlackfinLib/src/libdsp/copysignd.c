/************************************************************************
 *
 * copysignd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :  Copies the sign of the second argument to the first
 *                The function returns -|x|,  if reference value < 0
 *                                      |x|,  otherwise
 *
 *                Domain      : Full Range
 */

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =copysignd")
#pragma file_attr("libFunc  =__copysignd")
#pragma file_attr("libFunc  =copysign")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <math.h>

#include "util.h"


DOUBLE                            /*{ ret - copysign(x)                }*/
copysignd
(
  DOUBLE x,                       /*{ (i) - Input `x`                  }*/
  DOUBLE ref_val                  /*{ (i) - Input `reference argument` }*/
)
{
     return ( ref_val < 0 ? -fabsd(x) : fabsd(x) );
}


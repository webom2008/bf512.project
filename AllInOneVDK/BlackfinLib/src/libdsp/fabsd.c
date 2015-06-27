/************************************************************************
 *
 * fabsd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Return the the absolute value of the input argument.
 *
 *                 Domain      : Full Range
 *                 Accuracy    : 0 bits in error.
 */

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =__fabsd")
#pragma file_attr("libFunc  =fabsd")
#pragma file_attr("libFunc  =fabs")
#pragma file_attr("libFunc  =fabsl")
/* Some library functions call this function.  However an inline version will
** be used so don't list them here */
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <math.h>

#include "util.h"


DOUBLE                            /*{ ret - fabsd(x)      }*/
fabsd
(
  DOUBLE x                        /*{ (i) - Input value x }*/
)
{
  return ( x < 0 ? -x : x );
}

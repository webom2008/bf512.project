/************************************************************************
 *
 * fmaxd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Find maximum value of two long double inputs
 *                 The function fmax(x,y) returns  x,  if x > y
 *                                                 y,  otherwise
 *
 *                 Domain      : Full Range
 *                 Accuracy    : 0 bits in error.
 */

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libGroup =math_bf.h")
#pragma file_attr("libFunc  =__fmaxd")
#pragma file_attr("libFunc  =fmaxd")
#pragma file_attr("libFunc  =fmax")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <math.h>

#include "util.h"


DOUBLE                            /*{ ret - fmaxd(x,y)    }*/
fmaxd
(
  DOUBLE x,                       /*{ (i) - Input value x }*/
  DOUBLE y                        /*{ (i) - Input value y }*/
)
{
      return ( x > y ? x : y );    
}


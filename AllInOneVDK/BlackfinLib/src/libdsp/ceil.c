// Copyright (C) 2000 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel 
// Collaboration Agreement (ADI/Intel Confidential)

  /*-------------------------------------------------------------------------

  Purpose     : This function computes the ceiling of a 32-bit float input.
  Description : This function rounds up to the next highest whole number that
                is greater than or equal to x.

  Domain      : Full Range

  Accuracy    : ~ Relative error: 0 bits in error
                ~ Assumption: there is no error in the input value

  Data Memory : ~  0
  Prog Memory : ~ 21
  Cycles      : ~ 27 - max
                ~ 21 - min

  Notes       : N/A
  _____________________________________________________________________{!EHDR}
*/

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =__ceilf")
#pragma file_attr("libFunc  =ceilf")
#pragma file_attr("libFunc  =ceil")
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <float.h>
#include <math.h>
#include "util.h"
#include <math_const.h>

FLOAT                               /*{ ret - ceil(x)}*/
_ceilf(
    FLOAT x                         /*{ (i) - value for which to compute ceilf }*/
)
{
    FLOAT y;

    /*{ y = |x| }*/
    y = x;
    if (x < (FLOAT)0.0)
    {
        y = -y;
    }

    /*{ if x > 2^24 (max 23-bit int), result = x }*/
    if (y >= (FLOAT)16777216.0)
    {
        return x;
    }

    /*{ y = truncate(x) }*/
    y = TO_FLOAT(TO_LONG(x));

    /*{ if y < x, result = result + 1.0 }*/
    if (y < x)
    {
        y = ADD(y, 1.0);
    }

    return y;
}

/* end of file */

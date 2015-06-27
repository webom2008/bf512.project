/************************************************************************
 *
 * ceild.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description    : This file contains the 64-bit implementation of ceil().
 *                  This function rounds up to the next highest whole 
 *                  number that is greater than or equal to x.
 *
 *                  Domain      : Full Range
 *                  Accuracy    : 0 bits in error 
 */

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =__ceild")
#pragma file_attr("libFunc  =ceild")
#pragma file_attr("libFunc  =ceil")
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <math.h>

#include "util.h"


DOUBLE                            /*{ ret - ceild(x)      }*/
ceild
(
  DOUBLE x                        /*{ (i) - input value x }*/
)
{
    DOUBLE y;
    LONG exp;
    LONG bitsToClear;
    LONG *yPtr = (LONG *)&y;

    /*{ y = |x| }*/
    y = x;
    if (x < 0.0)
    {
        y = -y;
    }

    /*{ exp = exponent of y }*/
    exp = (yPtr[1] & 0x7ff00000) >> 20;
    exp = exp - 1023;

    /*{ if exponent is negative }*/
    if (exp < 0)
    {
        /*{ if x < 0, return 0 }*/
        /*{ else x >= 0, return 1 }*/
        y = (x <= 0.0) ? 0.0 : 1.0;
        return y;
    }

    /*{ if exp > 52 then there are no bits to clear, so return x }*/
    if (exp > 52)
    {
        return x;
    }

    bitsToClear = 52 - exp;

    /*{ if bitsToClear > 31 }*/
    if (bitsToClear > 31)
    {
        /*{ clear all 32 lower bits of y }*/
        /*{ bitsToClear = bitsToClear - 32 }*/
        /*{ clear bitsToClear bits from 32 upper bits of y }*/
        bitsToClear = bitsToClear - 32;
        yPtr[0] = 0;   /* clear all lower bits */
        yPtr[1] = yPtr[1] & (0xffffffff << bitsToClear);
    }
    /*{ else }*/
    else
    {
        /*{ clear bitsToClear bits from 32 lower bits of y }*/
        yPtr[0] = yPtr[0] & (0xffffffff << bitsToClear);
    }

    /*{ if (x < 0) }*/
    if (x < 0.0)
    {
        /*{ y = -y }*/
        y = -y;
    }
    /*{ else }*/
    else
    {
        /*{ if y != x, y = y + 1 }*/
        if (y != x)
        {
            y = ADDD(y, 1.0);
        }
    }

    /*{ return y }*/ 
    return y;
}

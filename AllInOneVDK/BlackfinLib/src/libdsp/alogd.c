/************************************************************************
 *
 * alogd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description    : This file contains the 64-bit implementation of alog().
 *                  The function calculates the natural (base e) anti-log 
 *                  of its argument.
 *
 *                  Domain      : as for exp
 *                  Notes       : An anti-log function performs the reverse of
 *                                a log function and is therefore equivalent
 *                                to an exponentation operation.
 */

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libGroup =math_bf.h")
#pragma file_attr("libFunc  =__alogd")
#pragma file_attr("libFunc  =alogd")
#pragma file_attr("libFunc  =alog")
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <math.h>

#include "math_const.h"
#include "util.h"


DOUBLE                            /*{ ret - acosd(x)       }*/
alogd
(
  DOUBLE x                        /*{ (i) - input value x  }*/
)
{
    /*{ return result }*/
    return expd(x);
}

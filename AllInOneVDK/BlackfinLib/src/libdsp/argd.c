/************************************************************************
 *
 * argd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Calculate the phase of a complex number.
 */

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =__argd")
#pragma file_attr("libFunc  =argd")
#pragma file_attr("libFunc  =arg")
/* Called by cartesiand */
#pragma file_attr("libFunc  =cartesiand")
#pragma file_attr("libFunc  =__cartesiand")
#pragma file_attr("libFunc  =cartesian")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <complex.h>

#include <math.h>


long double                       /*{ ret - Phase                  }*/
argd
(
  complex_long_double  a          /*{ (i) - Complex input `a`      }*/
)
{
    long double  p;


    /*{ Call atan2(imag portion of `a`, real portion of `a`) }*/
    p = atan2d(a.im, a.re);

    return (p);
}

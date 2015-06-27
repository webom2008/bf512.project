/************************************************************************
 *
 * cexpd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Calculate the complex exponential of a real number.
 *                       c.re = cos(a), c.im = sin(a) 
 */

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =cexpd") 
#pragma file_attr("libFunc  =__cexpd")     //from complex.h
#pragma file_attr("libFunc  =cexp")        //from complex.h

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <complex.h>

#include <math.h>


complex_long_double               /*{ ret - Complex exponential    }*/
cexpd
(
  long double a                   /*{ (i) - Real Exponent a        }*/
)
{
    complex_long_double  c;

    /*{ Take cosine and sine of `a` }*/
    c.re = cosd(a);
    c.im = sind(a);

    /*{ Return result }*/
    return (c);
}

/************************************************************************
 *
 * csubd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Complex subtraction
 */

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =csubd")
#pragma file_attr("libFunc  =__csubd")
#pragma file_attr("libFunc  =csub")
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <complex.h>


complex_long_double               /*{ ret - Complex difference     }*/
csubd
(
  complex_long_double a,          /*{ (i) - Complex input `a`      }*/
  complex_long_double b           /*{ (i) - Complex input `b`      }*/
)
{
    complex_long_double  c;

    /*{ Subtract real portion of `b` from real portion of `a` }*/
    c.re = a.re - b.re;

    /*{ Subtract imag portion of `b` from imag portion of `a` }*/
    c.im = a.im - b.im;

    return (c);
}

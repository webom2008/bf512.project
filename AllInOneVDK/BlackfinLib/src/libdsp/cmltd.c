/************************************************************************
 *
 * cmltd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Complex multiplication
 */

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =cmltd")
#pragma file_attr("libFunc  =__cmltd")
#pragma file_attr("libFunc  =cmlt")
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <complex.h>


complex_long_double               /*{ ret - Complex product        }*/
cmltd
(
  complex_long_double a,          /*{ (i) - Complex input `a`      }*/
  complex_long_double b           /*{ (i) - Complex input `b`      }*/
)
{
    complex_long_double  c;

    /*{ Subtract imag portion of `a` times imag portion of `b` from real 
        portion of `a` times real portion of `b` }*/
    c.re = a.re * b.re - a.im * b.im;

    /*{ Add imag portion of `a` times real portion of `b` to real portion of
        `a` times imag portion of `b` }*/
    c.im = a.re * b.im + a.im * b.re;

    return (c);
}

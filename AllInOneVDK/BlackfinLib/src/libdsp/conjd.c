/************************************************************************
 *
 * conjd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Complex Conjugate
 */

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =conjd")
#pragma file_attr("libFunc  =__conjd")
#pragma file_attr("libFunc  =conj")
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <complex.h>


complex_long_double               /*{ ret - Complex conjungate 'a' }*/
conjd
(
  complex_long_double a           /*{ (i) - Complex input `a`      }*/
)
{
    complex_long_double  c;

    /*( Negate the imag portion of `a` }*/
    c.re = a.re;
    c.im = -a.im;

    return (c);
}

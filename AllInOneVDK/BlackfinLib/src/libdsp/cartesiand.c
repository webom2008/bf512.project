/************************************************************************
 *
 * cartesiand.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Cartesian to polar conversion
 */

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =cartesiand")
#pragma file_attr("libFunc  =__cartesiand")     //from complex.h
#pragma file_attr("libFunc  =cartesian")        //from complex.h

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <complex.h>

#include <math.h>

long double                       /*{ ret - Magnitude              }*/
cartesiand
(
  complex_long_double a,          /*{ (i) - Complex input `a`      }*/
  long double*  phase             /*{ (o) - Phase                  }*/
)
{
    *phase = argd(a);             /* compute phase     */
    return( cabsd(a) );           /* compute magnitude */
}


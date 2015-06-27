/************************************************************************
 *
 * cvecsmltd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :  Complex Vector Scalar Multiplication
 */

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =__cvecsmltd")
#pragma file_attr("libFunc  =cvecsmltd")
#pragma file_attr("libFunc  =cvecsmlt")
/* Called by cmatsmlt and cmatsmltf */
#pragma file_attr("libGroup =matrix.h")
#pragma file_attr("libFunc  =cmatsmltd")
#pragma file_attr("libFunc  =cmatsmlt")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <vector.h>

#include <complex.h>


void
cvecsmltd
(
  const complex_long_double a[],  /*{ (i) - Input vector `a[]`           }*/
  complex_long_double b,          /*{ (i) - Input scalar `b`             }*/
  complex_long_double c[],        /*{ (o) - Output vector `c[]`          }*/
  int n                           /*{ (i) - Number of elements in vector }*/
)
{
    int i;


    /*{ Multiply `b` with each element of vector `a[]` and store
        in vector `c[]`. }*/
    for (i = 0; i < n; i++)
    {
        c[i].re = (a[i].re * b.re) - (a[i].im * b.im);
        c[i].im = (a[i].re * b.im) + (a[i].im * b.re);
    }
}

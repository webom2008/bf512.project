/************************************************************************
 *
 * cvecsaddd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :  Complex Vector Scalar Addition
 */

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =__cvecsaddd")
#pragma file_attr("libFunc  =cvecsaddd")
#pragma file_attr("libFunc  =cvecsadd")
/* Called by cmatsaddd and cmatsadd */
#pragma file_attr("libGroup =matrix.h")
#pragma file_attr("libFunc  =cmatsaddd")
#pragma file_attr("libFunc  =cmatsadd")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <vector.h>

#include <complex.h>


void
cvecsaddd
(
  const complex_long_double a[],  /*{ (i) - Input vector `a[]`           }*/
  complex_long_double b,          /*{ (i) - Input scalar `b`             }*/
  complex_long_double c[],        /*{ (o) - Output vector `c[]`          }*/
  int n                           /*{ (i) - Number of elements in vector }*/
)
{
    int i;


    /*{ Add `b` to each element of vector `a[]` and store
        in vector `c[]`. }*/
    for (i = 0; i < n; i++)
    {
        c[i].re = a[i].re + b.re;
        c[i].im = a[i].im + b.im;
    }
}

/************************************************************************
 *
 * cvecdotd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Complex Vector Dot Product
 */

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =__cvecdotd")
#pragma file_attr("libFunc  =cvecdotd")
#pragma file_attr("libFunc  =cvecdot")
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <vector.h>

#include <complex.h>


complex_long_double               /*{ ret - Complex dot product          }*/
cvecdotd
(
  const complex_long_double a[],  /*{ (i) - Input vector `a[]`           }*/
  const complex_long_double b[],  /*{ (i) - Input vector `b[]`           }*/
  int n                           /*{ (i) - Number of elements in vector }*/
)
{
    int  i;
    complex_long_double  acc;


    /*{ Initialize accumulator }*/
    acc.re = 0.0L;
    acc.im = 0.0L;

    /*{ Multiply each element of vector `a[]` with each element of
        vector `b[]` and accumulate result. }*/
    for (i = 0; i < n; i++)
    {
        acc.re += ((a[i].re * b[i].re) - (a[i].im * b[i].im));
        acc.im += ((a[i].re * b[i].im) + (a[i].im * b[i].re));
    }

    return (acc);
}

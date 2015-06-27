/************************************************************************
 *
 * vecdotd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Vector Dot Product
 */

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =vecdotd")
#pragma file_attr("libFunc  =__vecdotd")   // from vector.h
#pragma file_attr("libFunc  =vecdot")      // from vector.h
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <vector.h>

long double                       /*{ ret - Dot product                  }*/
vecdotd
(
  const long double a[],          /*{ (i) - Input vector `a[]`           }*/
  const long double b[],          /*{ (i) - Input vector `b[]`           }*/
  int n                           /*{ (i) - Number of elements in vector }*/
)
{
    int  i;
    long double  acc = 0.0L;


    /*{ Multiply each element of vector `a[]` with each element of
        vector `b[]` and accumulate result. }*/
    for (i = 0; i < n; i++)
    {
        acc += (a[i] * b[i]);
    }

    return (acc);
}

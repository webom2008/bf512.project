/************************************************************************
 *
 * vecvmltd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Vector Vector Multiplication
 */

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =vecvmltd")
#pragma file_attr("libFunc  =__vecvmltd") // from vector.h
#pragma file_attr("libFunc  =vecvmlt")    // from vector.h
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <vector.h>

void
vecvmltd
(
  const long double a[],          /*{ (i) - Input vector  `a[]`          }*/
  const long double b[],          /*{ (i) - Input vector  `b[]`          }*/
  long double c[],                /*{ (o) - Output vector `c[]`          }*/
  int n                           /*{ (i) - Number of elements in vector }*/
)
{
    int i;


    /*{ Multiply each element of vector `a[]` with each element
        of vector `b[]` and store the result in vector `c[]`. }*/
    for (i = 0; i < n; i++)
    {
        c[i] = a[i] * b[i];
    }
}


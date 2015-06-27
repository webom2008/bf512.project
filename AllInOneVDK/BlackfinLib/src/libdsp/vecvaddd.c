/************************************************************************
 *
 * vecvaddd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Vector Vector Addition
 */

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libGroup =matrix.h")
#pragma file_attr("libFunc  =vecvaddd")
#pragma file_attr("libFunc  =__vecvaddd")   // from vector.h
#pragma file_attr("libFunc  =vecvadd")      // from vector.h
#pragma file_attr("libFunc  =matmaddd")     // from matrix.h
#pragma file_attr("libFunc  =matmadd")      // from matrix.h
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <vector.h>


void
vecvaddd
(
  const long double a[],          /*{ (i) - Input vector  `a[]`          }*/
  const long double b[],          /*{ (i) - Input vector  `b[]`          }*/
  long double c[],                /*{ (o) - Output vector `c[]`          }*/
  int n                           /*{ (i) - Number of elements in vector }*/
)
{
    int i;


    /*{ Add each element of vector `a[]` is added with each element
        of vector `b[]` and stored in vector `c[]`. }*/
    for (i = 0; i < n; i++)
    {
        c[i] = a[i] + b[i];
    }
}


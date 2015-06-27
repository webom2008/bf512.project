/************************************************************************
 *
 * vecvsubd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Vector Vector Subtraction
 */

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libGroup =matrix.h")
#pragma file_attr("libFunc  =vecvsubd")
#pragma file_attr("libFunc  =__vecvsubd")  // from vector.h
#pragma file_attr("libFunc  =vecvsub")     // from vector.h
#pragma file_attr("libFunc  =matmsubd")    // from matrix.h
#pragma file_attr("libFunc  =matmsub")     // from matrix.h
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <vector.h>

void
vecvsubd
(
  const long double a[],          /*{ (i) - Input vector  `a[]`          }*/
  const long double b[],          /*{ (i) - Input vector  `b[]`          }*/
  long double c[],                /*{ (o) - Output vector `c[]`          }*/
  int n                           /*{ (i) - Number of elements in vector }*/
)
{
    int i;


    /*{ Each element of vector `b[]` is subtracted from each element
        of vector `a[]` and stored in vector `c[]`. }*/
    for (i = 0; i < n; i++)
    {
        c[i] = a[i] - b[i];
    }
}

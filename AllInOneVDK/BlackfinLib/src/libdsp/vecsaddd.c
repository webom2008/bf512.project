/************************************************************************
 *
 * vecsaddd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Vector Scalar Addition
 */

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libGroup =matrix.h")
#pragma file_attr("libFunc  =vecsaddd")
#pragma file_attr("libFunc  =__vecsaddd")   // from vector.h
#pragma file_attr("libFunc  =vecsadd")      // from vector.h
#pragma file_attr("libFunc  =matsaddd")     // from matrix.h
#pragma file_attr("libFunc  =matsadd")      // from matrix.h
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <vector.h>

void
vecsaddd
(
  const long double a[],          /*{ (i) - Input vector  `a`            }*/
  long double b,                  /*{ (i) - Input scalar  `b`            }*/
  long double c[],                /*{ (o) - Output vector `c`            }*/
  int n                           /*{ (i) - Number of elements in vector }*/
)
{
    int i;


    /*{ Add `b` to each element of vector `a[]` and store
        in vector `c[]`. }*/
    for (i = 0; i < n; i++)
    {
        c[i] = a[i] + b; 
    }
}


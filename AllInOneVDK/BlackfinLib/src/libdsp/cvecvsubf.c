// Copyright (C) 2000, 2001 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/****************************************************************************
  Func name   : cvecvsubf

  Purpose     : Complex vector - vector subtraction for float data types.
  Description : This function subtracts each element i in vector `b[]` from 
                each element i in input vector `a[]` and stores the result in 
                vector `c[]`, where 0 <= i < n and n = length a[] = length b[]

****************************************************************************/

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =__cvecvsubf")
#pragma file_attr("libFunc  =cvecvsubf")
#pragma file_attr("libFunc  =cvecvsub")
/* Called by cmatmsubf and cmatmsub */
#pragma file_attr("libGroup =matrix.h")
#pragma file_attr("libFunc  =cmatmsubf")
#pragma file_attr("libFunc  =cmatmsub")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")


#include <complex.h> 
#include <vector.h>

void 
_cvecvsubf(
	const complex_float a[],   /*{ (i) - Input vector `a[]` }*/
	const complex_float b[],   /*{ (i) - Input vector `b[]` }*/
	complex_float c[],         /*{ (o) - Output vector `c[]` }*/
	int n                      /*{ (i) - Number of elements in vector }*/
)
{
    int i;

    /*{ Subtract each element of vector `b[]` from each element of 
        vector `a[]` and store in vector `c[]`. }*/
    for (i = 0; i < n; i++)
    {
	    c[i].re = a[i].re - b[i].re;
	    c[i].im = a[i].im - b[i].im;
    }
}
/*end of file*/



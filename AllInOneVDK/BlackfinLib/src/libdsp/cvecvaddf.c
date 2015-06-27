// Copyright (C) 2000, 2001 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/*****************************************************************************
  Func name   : cvecvaddf

  Purpose     : Complex vector + vector addition for float data types.
  Description : This function adds each element i of vector `a[]` to each 
                element i of input vector `b[]` and stores the result in 
                vector `c[]`, where 0 <= i < n and n = length a[] = length b[]

*****************************************************************************/

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =__cvecvaddf")
#pragma file_attr("libFunc  =cvecvaddf")
#pragma file_attr("libFunc  =cvecvadd")
/* Called by cmatmaddf and cmatmadd */
#pragma file_attr("libGroup =matrix.h")
#pragma file_attr("libFunc  =cmatmaddf")
#pragma file_attr("libFunc  =cmatmadd")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")


#include <complex.h>
#include <vector.h>

void
_cvecvaddf(
	const complex_float a[],   /*{ (i) - Input vector `a[]` }*/
	const complex_float b[],   /*{ (i) - Input vector `b[]` }*/
	complex_float c[],         /*{ (o) - Output vector `c[]` }*/
	int n                      /*{ (i) - Number of elements in vector }*/
)
{
    int i;

    /*{ Each element of vector `a[]` is added with each element of vector 
        `b[]` and stored in vector `c[]`. }*/
    for (i = 0; i < n; i++)
    {
	    c[i].re = a[i].re + b[i].re;
	    c[i].im = a[i].im + b[i].im;
    }
}
/*end of file*/


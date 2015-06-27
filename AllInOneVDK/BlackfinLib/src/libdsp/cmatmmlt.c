// Copyright (C) 2000, 2001 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/***************************************************************************
   File: cmatmmlt.c
 
   This function calculates the multiplication of two complex matrices.

***************************************************************************/

#pragma file_attr("libGroup =matrix.h")
#pragma file_attr("libFunc  =__cmatmmltf")
#pragma file_attr("libFunc  =cmatmmltf")
#pragma file_attr("libFunc  =cmatmmlt")
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <matrix.h>

void _cmatmmltf(const complex_float *a, int n, int k,
		const complex_float *b, int m, 
		complex_float *c )
{
   /* a is a complex matrix with n rows and k columns 
      b is a complex matrix with k rows and m columns 
      c is the complex output matrix with n rows and m columns
    */

    int i,l,j;  
    complex_float sum;

    if( m<=0 || n <= 0 || k<=0)
        return;

    for (i = 0; i < n; i++)
    {
        for (j = 0; j < m; j++)
        {
	    sum.re = 0.0;
	    sum.im = 0.0;
            for (l = 0; l < k; l++)
            {
	      /* multiply each row (=n) in matrix a with
		 every columnn (m) of matrix b
		 i.e. a(i,0) * b(0,m) + a(i,1) * b(1,m) + .. + a(i,k) * b(k,m)
			where:  matrix(row,column)
		 using complex multiplication:
		 a * b = (a.re,a.im) * (b.re,b.im) = 
			 ((a.re*b.re - a.im*b.im),(a.re*b.im + b.re*a.im)) 
              */
	      sum.re += a[i * k + l].re * b[l * m + j].re -
			a[i * k + l].im * b[l * m + j].im; 
	      sum.im += a[i * k + l].re * b[l * m + j].im +
			a[i * k + l].im * b[l * m + j].re; 
	    }  
            c[i * m + j].re = sum.re; 
            c[i * m + j].im = sum.im;
        }
    }
    return;
}
/*end of file*/

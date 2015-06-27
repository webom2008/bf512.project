// Copyright (C) 2000-2009 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/***************************************************************************
   File: crosscorr.c

   Cross correlation of two floating point vectors.

***************************************************************************/

#pragma file_attr("libGroup =stats.h")
#pragma file_attr("libFunc  =crosscorrf")
#pragma file_attr("libFunc  =__crosscorrf")
#pragma file_attr("libFunc  =crosscorr")
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <stats.h>

void _crosscorrf( const float *a, const float *b, int n, int m, float *c )
{
  /* a and b are the input vectors 
     n is the size of vectors
     m is the number of bins 
     c is the output vector.
  */
    int i,j;

    if (n <= 0 || m <= 0)
    {
      return;
    }

    //This for loop calculates the cross correlation of two vectors.
    for (i=0; i < m; i++)
    {
      c[i] = 0.0;
      for (j = 0; j < n-i; j++)
      {   
        c[i] += (a[j] * b[j+i]);			
      }
      c[i] = c[i]/n;
    }

}

/*end of file*/

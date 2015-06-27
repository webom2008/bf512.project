// Copyright (C) 2000, 2001 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/**************************************************************************
   File: autocoh.c

   Auto Coherrance of a vector.

***************************************************************************/
#pragma file_attr("libGroup =stats.h")
#pragma file_attr("libFunc  =__autocohf")
#pragma file_attr("libFunc  =autocohf")
#pragma file_attr("libFunc  =autocoh")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <stats.h>

void _autocohf(const float *a, int n, int m, float *c)
{   
  /* a is the input vector
     n is the size of vector
     m is the number of bins
     c is the output vector.
  */
    int i,j;
    float mean =0;

    if (n <= 0 || m <= 0)
    {
      return;
    }

    /*{ Calculate the mean value of input vector }*/
    for (i = 0; i < n; i++)
    {
        mean += a[i];
    }
    mean = mean / n;

    for (i=0; i < m; i++)
    {
        /*Calculate the autocorrelation of input vector */
        c[i] = 0.0;
        for (j = 0; j < n-i; j++)
        {
            c[i] += (a[j] * a[j+i]);
        }

        c[i] = c[i] / n;
        /*Autocorrelation minus the squared mean*/
        c[i] -= mean * mean;
    }
  
}
/* end of file */

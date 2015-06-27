// Copyright (C) 2000, 2001 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/****************************************************************************
   File: rms.c

   Calculates the root mean square value for the floating point input vector.

***************************************************************************/

#pragma file_attr("libGroup =stats.h")
#pragma file_attr("libFunc  =rmsf")       //from stats.h
#pragma file_attr("libFunc  =__rmsf")
#pragma file_attr("libFunc  =rms")        //from stats.h

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <stats.h>
#include <math.h>
#include "means.h"


float _rmsf(const float *a, int n) 
{
  // a is the input vector and n is its size.
      float m;

      if(n <= 0)
        return 0.0;

      m = _meansf (a, n);
      m = sqrtf(m);   // m is the rms value of a
	  
      return m;
}
/*end of file*/

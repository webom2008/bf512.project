// Copyright (C) 2000, 2001 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/****************************************************************************
   Func Name:    normf

   Description:  normalizing the complex input a

****************************************************************************/

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =normf")       //from complex.h
#pragma file_attr("libFunc  =__normf")
#pragma file_attr("libFunc  =norm")        //from complex.h

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <math.h>
#include <complex.h>

complex_float _normf(complex_float a )
{
   complex_float  c;
   float          d;

   d = cabsf(a);

   if( d == 0.0F )
   {
      c.re = 0.0F;
      c.im = 0.0F;
   }
   else
   {
      d = 1.0F / d;
      c.re = a.re * d;
      c.im = a.im * d;
   }

   return (c);
}

/*end of file*/

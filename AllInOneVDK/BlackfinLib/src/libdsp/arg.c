// Copyright (C) 2000, 2001 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/*********************************************************************
   Func Name:    argf

   Description:  return phase of the complex input a

*********************************************************************/

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =__argf")
#pragma file_attr("libFunc  =argf")
#pragma file_attr("libFunc  =arg")
/* Called by cartesian */
#pragma file_attr("libFunc  =cartesianf")
#pragma file_attr("libFunc  =__cartesianf")
#pragma file_attr("libFunc  =cartesian")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <math.h>
#include <complex.h>

float _argf( complex_float a )
{
  float arg;

  arg = atan2f(a.im, a.re);
  return(arg);
}

/* end of file */

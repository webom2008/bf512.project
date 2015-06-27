// Copyright (C) 2000, 2001 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/***************************************************************************
   File: cexp.c
  
   complex exponential for floating point input

****************************************************************************/

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =cexpf")       //from complex.h
#pragma file_attr("libFunc  =__cexpf")
#pragma file_attr("libFunc  =cexp")        //from complex.h

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <math.h>
#include <complex.h>

complex_float _cexpf(float a )
{
    complex_float c;

    c.re = cosf(a);
    c.im = sinf(a);
    return (c);
}

/*end of file*/

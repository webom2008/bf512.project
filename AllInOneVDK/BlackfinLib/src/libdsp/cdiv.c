// Copyright (C) 2000, 2001 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/****************************************************************************
   File: cdiv.c

   Complex floating point division

****************************************************************************/

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =__cdivf")
#pragma file_attr("libFunc  =cdivf")
#pragma file_attr("libFunc  =cdiv")
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <complex.h>
#include <math.h>

complex_float _cdivf( complex_float a, complex_float b )
{
    complex_float   c;
    float           fractio, denum;

    /*
        This function performs a complex division of two numbers:

                       a.real * b.real + a.imag * b.imag
              c.real = ---------------------------------
                       b.real * b.real + b.imag * b.imag

                       a.imag * b.real - a.real * b.imag
              c.imag = ---------------------------------
                       b.real * b.real + b.imag * b.imag

       To prevent avoidable overflows, underflow or loss of precision,
       the following alternative algorithm is used:

       If |b.re| >= |b.im|
         c.re = (a.re + a.im * (b.im / b.re)) / (b.re + b.im * (b.im / b.re));
         c.im = (a.im - a.re * (b.im / b.re)) / (b.re + b.im * (b.im / b.re));

       Else    // |b.re| < |b.im|
         c.re = (a.re * (b.re / b.im) + a.im) / (b.re * (b.re / b.im) + b.im);
         c.im = (a.im * (b.re / b.im) - a.re) / (b.re * (b.re / b.im) + b.im);
     */

    if( (b.re == 0) && (b.im == 0) )
    {
       // return 0
       c.re = 0.0F;
       c.im = 0.0F;
    }
    else if (b.re == 0)
    {
       c.re =   a.im / b.im;
       c.im = -(a.re / b.im);
    }
    else if (b.im == 0)
    {
       c.re =   a.re / b.re;
       c.im =   a.im / b.re;
    }
    else if( fabsf(b.re) >= fabsf(b.im) )
    {
       fractio = b.im / b.re;
       denum   = 1.0F / (b.re + b.im * fractio);
       c.re    = (a.re + a.im * fractio) * denum;
       c.im    = (a.im - a.re * fractio) * denum;
    }
    else
    {
       fractio = b.re / b.im;
       denum   = 1.0F / (b.re * fractio + b.im);
       c.re    = (a.re * fractio + a.im) * denum;
       c.im    = (a.im * fractio - a.re) * denum;
    }

    return (c);
}
/* end of file */

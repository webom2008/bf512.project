// Copyright (C) 2000, 2001 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/****************************************************************************
   Func Name:     polarf

   Description:   This function takes the magnitude and phase, 
                  describing a complex number result in polar
                  notation, as input argument.
                  The output argument is a complex number in 
                  cartesian notation:

                      a.real = Magnitude * cos(Phase);
                      a.imag = Magnitude * sin(Phase);

****************************************************************************/

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =polarf")
#pragma file_attr("libFunc  =__polarf")     //from complex.h
#pragma file_attr("libFunc  =polar")        //from complex.h

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <math.h>
#include <complex.h>

complex_float _polarf(float mag, float phase )
{
    
    complex_float result;

    result.re = (float)(mag * cosf(phase));
    result.im = (float)(mag * sinf(phase));

    return (result);
}

/*end of file*/

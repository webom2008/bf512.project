// Copyright (C) 2000, 2001 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/**************************************************************************** 
  Func name   : fminf

  Purpose     : This function returns the lesser of 2 input values.
  Description : 

  Domain      : ~ x = [-3.4e38 ... 3.4e38]
                ~ y = [-3.4e38 ... 3.4e38]

 *****************************************************************************/

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libGroup =math_bf.h")
#pragma file_attr("libFunc  =__fminf")
#pragma file_attr("libFunc  =fminf")
#pragma file_attr("libFunc  =fmin")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <math.h>

float                      /*{ ret - min of (x, y) }*/
_fminf(
    float x,               /*{ (i) - input parameter 1 }*/
    float y                /*{ (i) - input parameter 2 }*/
)
{

    /*{ result = y }*/
    float result = y;

    /*{ if x < y, result = x }*/
    if (x < y)
    {
        result = x;
    }

    /*{ return result }*/
    return result;
}

/*end of file*/

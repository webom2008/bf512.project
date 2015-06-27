// Copyright (C) 2000, 2001 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/******************************************************************************
  Func name   : max

  Purpose     : This function returns the greater of 2 integer input values.
  Description : 

  Domain      : ~ x = [-MAX_INT ... MAX_INT]
                ~ y = [-MAX_INT ... MAX_INT]

******************************************************************************/

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libGroup =math_bf.h")
#pragma file_attr("libFunc  =__max")
#pragma file_attr("libFunc  =max")     //from math_bf.h

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <math.h>

int                        /*{ ret - max of (x, y) }*/
_max(
    int x,                 /*{ (i) - input parameter 1 }*/
    int y                  /*{ (i) - input parameter 2 }*/
)
{

    /*{ result = y }*/
    int result = y;

    /*{ if x > y, result = x }*/
    if (x > y)
    {
        result = x;
    }

    /*{ return result }*/
    return result;
}

/*end of file*/

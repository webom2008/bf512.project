// Copyright (C) 2004 Analog Devices Inc., All Rights Reserved.
/******************************************************************************
  Func name   : llmin

  Purpose     : This function returns the smaller of 2 long long input values.

  Domain      : ~ a = [LLONG_MIN ... LLONG_MAX]
                ~ b = [LLONG_MIN ... LLONG_MAX]

******************************************************************************/

#pragma file_attr("libGroup =math_bf.h")
#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =__llmin")
#pragma file_attr("libFunc  =llmin")     //from math_bf.h

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <math.h>

long long  _llmin( long long  a, long long  b )
{
    long long  min_val;

    min_val = b;
    if (a < b)
    {
        min_val = a;
    }
    return  min_val;
}

/*end of file*/

// Copyright (C) 2000 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel 
// Collaboration Agreement (ADI/Intel Confidential)

/*______________________________________________________________________

  Func name   : logf

  ----------------------------------------------------------------------------

  Purpose     : This function computes ln(x) using 32-bit float precision.
  Description : The algorithm used to implement this function is adapted from
                Cody & Waite, "Software Manual for the Elementary Functions",
                Prentice-Hall, New Jersey, 1980.

  Domain      : x = [0 ... 3.4e38]

  Accuracy    : ~ Relative error:
                ~ * 3 bits of error or less over entire range
                ~ Assumption: there is no error in the input value

  Data Memory : ~   0
  Prog Memory : ~  92
  Cycles      : ~ 100 - max
                ~  20 - min

  Notes       : For x outside the domain, this function returns -3.4e38
  _____________________________________________________________________
*/

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =__logf")
#pragma file_attr("libFunc  =logf")
#pragma file_attr("libFunc  =log")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <float.h>
#include <math.h>
#include "util.h"
#include <math_const.h>

FLOAT                                /*{ ret - log(x) }*/
_logf(
    FLOAT x                          /*{ (i) - value to do logf on }*/
)
{
    FLOAT result;
    FLOAT f, w, z, a, b, r;
    FLOAT znum, zden;
    LONG n;
    FLOAT xn;
    LONG *xPtr = (LONG *)&x;
    FLOAT *fPtr = &x;

    /*{ if x < 0.0, return -3.4e38 }*/
    if (x <= (FLOAT)0.0)
    {
        result = (FLOAT)-FLT_MAX;
        return result;
    }

    /*{ n = exponent part of x }*/
    n = (*xPtr >> 23);
    n = n - 126;

    /*{ f = fractional part of x }*/
    /* f = setxp(x, 0) -- done by setting exponent to 126 */
    *xPtr = (*xPtr & 0x807fffff) | (126 << 23);
    f = *fPtr;

    /*{ if f > sqrt(0.5) }*/
    if (f > (FLOAT)ROOT_HALF)
    {
        /*{ znum = f - 1 }*/
        znum = SUB(f, 0.5);
        znum = SUB(znum, 0.5);
        /*{ zden = f * 0.5 + 0.5 }*/
        zden = MPY(f, 0.5);
        zden = ADD(zden, 0.5);
    }
    /*{ else }*/
    else
    {
        /*{ n = n - 1 }*/
        n = n - 1;
        /*{ znum = f - 0.5 }*/
        znum = SUB(f, 0.5);
        /*{ zden = f * 0.5 + 0.5 }*/
        zden = MPY(znum, 0.5);
        zden = ADD(zden, 0.5);
    }
    xn = (FLOAT)n;

    /*{ z = znum/zden }*/
    z = DIV(znum, zden);

    /*{ w = z * z }*/
    w = MPY(z, z);

    /*{ R(z) = z + z * r(w) }*/
    /*{!INDENT}*/
    /*{ r(w) = w * A(w)/B(w) }*/
    /*{ A(w) = a1 * w + a0 }*/
    a = MPY(LOGA_COEF1, w);
    a = ADD(a, LOGA_COEF0);

    /*{ B(w) = w + b0 }*/
    b = ADD(w, LOGB_COEF0);

    r = DIV(a, b);
    r = MPY(r, w);
    r = MPY(r, z);
    r = ADD(r, z);
    /*{!OUTDENT}*/

    /*{ result = n *c + R(z) }*/
    /* using higher precision calculation */
    result = MPY(xn, LN2_C1);
    result = ADD(result, r);
    r = MPY(xn, LN2_C2);
    result = ADD(result, r);

    /*{ return result }*/
    return result;
}

/*end of file*/

/************************************************************************
 *
 * sinhd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :  This file contains the 64-bit implementation of sinh().
 *                The algorithm used to implement this function is adapted
 *                from Cody & Waite, "Software Manual for the Elementary
 *                Functions", Prentice-Hall, New Jersey, 1980.
 *
 *                Domain      : x = [-709.7 ... +709.7]
 *                                For x outside the domain, this function 
 *                                returns +-1.7e308.
 *                Accuracy    : Primary range (-1 to 1)
 *                                1 bits of error or less
 *                              Outside primary range
 *                                3 bits of error or less
 */

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =sinhd")
#pragma file_attr("libFunc  =__sinhd")     //from math.h
#pragma file_attr("libFunc  =sinh")        //from math.h

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <math.h>

#include "math_const.h"
#include "util.h"

DOUBLE                            /*{ ret - sinhd(x)      }*/
sinhd
(
  DOUBLE x                        /*{ (i) - input value x }*/
)
{
    DOUBLE y, w, z;
    DOUBLE f, xnum, xden;
    DOUBLE result;
    DOUBLE sign = 1.0;

    /*{ y = |x| }*/
    y = x;
    /*{ sign = 1.0 }*/
    /*{ if x < 0, sign = -sign }*/
    if (x < (DOUBLE)0.0)
    {
        y = -y;
        sign = -sign;
    }

    /*{ if (y > 1.0) }*/
    if (y > (DOUBLE)1.0)
    {
        /*{ if (y > XDOUBLE_MAX_EXP) }*/
        if (y > XDOUBLE_MAX_EXP)
        {
            /*{ w = y - ln(v) }*/
            w = SUBD(y, LN_V);

            /*{ if w > XDOUBLE_MAX_EXP }*/
            if (w > XDOUBLE_MAX_EXP)
            {
                /*{ result = +1.7e308 }*/
                result = LDBL_MAX;
                /*{ if sign < 0, result = -result }*/
                if (sign < 0.0)
                {
                    result = -result;
                }
                /*{ return result }*/
                return result;
            }

            /*{ z = exp(w) }*/
            z = expd(w);

            /*{ result = (v/2) * z }*/
            /* using higher precision computation */
            result = MPYD(V_2_MINUS1, z);
            result = ADDD(result, z);
        }
        /*{ else y <= XDOUBLE_MAX_EXP }*/
        else
        {
            /*{ z = exp(y) }*/
            z = expd(y);

            /*{ result = ((z - 1 / z) / 2 }*/
            result = DIVD(-0.5, z);
            z = MPYD(0.5, z);
            result = ADDD(z, result);
        }

     }
    /*{ else y <= 1.0 }*/
    else
    {
        /*{ if y < eps, result = y }*/
        if (y < LDBL_EPSILON)
        {
            result = y;
        }
        else
        {
            /*{ result = y + y * R(x^2) }*/
            /*{!INDENT}*/
            /*{ R(f) = f*P(f)/Q(f) }*/

            /*{ f = x * x }*/
            f = MPYD(x, x);

            /*{ P(f) = ((p3 * f + p2) * f + p1) * f + p0 }*/
            xnum = MPYD(SINHDP_COEF3, f);
            xnum = ADDD(xnum, SINHDP_COEF2);
            xnum = MPYD(xnum, f);
            xnum = ADDD(xnum, SINHDP_COEF1);
            xnum = MPYD(xnum, f);
            xnum = ADDD(xnum, SINHDP_COEF0);

            /*{ Q(f) = ((f + q2) *f + q1) * f + q0 }*/
            xden = ADDD(f, SINHDQ_COEF2);
            xden = MPYD(xden, f);
            xden = ADDD(xden, SINHDQ_COEF1);
            xden = MPYD(xden, f);
            xden = ADDD(xden, SINHDQ_COEF0);
            /*{!OUTDENT}*/

            result = DIVD(xnum, xden);
            result = MPYD(result, f);
            result = MPYD(result, y);
            result = ADDD(result, y);
        }
    }

    /*{ if sign < 0, result = -result }*/
    if (sign < 0.0)
    {
        result = -result;
    }
 
    /*{ return result }*/
    return result;
}
